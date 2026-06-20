/* xa_audio.cpp — PS1 XA-ADPCM streaming audio playback
 *
 * How PS1 XA audio works:
 *   The disc contains Mode 2 Form 2 sectors with submode bit 2 (0x04) set.
 *   These are interleaved with video/data sectors.  When the game does
 *   CdlSeekL to a streaming region, the hardware auto-delivers audio sectors
 *   to the SPU as the head reads past them.
 *
 * Our implementation:
 *   A background thread reads raw 2352-byte sectors from the BIN file.
 *   Audio sectors (submode & 0x04) are decoded from XA-ADPCM to 16-bit PCM.
 *   Decoded audio is fed to WinMM waveOut for playback.
 *   xa_audio_seek() is called on every CdlSeekL; if the new region has no
 *   audio within 20 sectors, playback stops naturally.
 *
 * Channel filtering:
 *   PS1 XA multiplexes up to 32 channels on a single disc.  The game uses
 *   CdlSetFilter to select one file/channel; the CD hardware only routes
 *   matching sectors to the SPU.  We emulate this by locking to the first
 *   audio sector seen after each seek and ignoring all others.  Decoding
 *   mixed channels trashes the IIR filter state and produces garbled audio.
 *
 * WAV capture:
 *   Decoded PCM is written to C:/temp/xa_capture.wav (up to 30 seconds)
 *   in the audio thread so the main thread is never blocked.  The file can
 *   be compared spectrally against a reference MP3 to verify decode quality.
 *
 * Audio format: stereo, 37800 Hz, 16-bit signed PCM (codingInfo=0x01).
 */
#include "xa_audio.h"
#include <windows.h>
#include <cstdio>
#include <cstring>
#include <atomic>
#include <thread>

/* Raw sector constants */
static constexpr int RAW_SECTOR      = 2352;
static constexpr int XA_SUBHDR_OFF   = 16;   /* sub-header at bytes 16-23 */
static constexpr int XA_DATA_OFF     = 24;   /* user data payload at byte 24 */
static constexpr int XA_DATA_SIZE    = 2324; /* payload size */
static constexpr int XA_SOUND_GROUPS = 18;   /* sound groups per sector */
static constexpr int SUBMODE_AUDIO   = 0x04; /* submode bit: audio sector */

/* Decoded PCM: stereo 37800 Hz 16-bit (codingInfo bit2=0 → 37800Hz) */
static constexpr int SAMPLE_RATE     = 37800;
/* waveOut output rate — 44100 Hz avoids Windows resampling from 37800→48000.
 * Many devices support 44100 natively; the 37800→44100 ratio (6:7) is simple
 * and we handle it ourselves with linear interpolation before submission. */
static constexpr int OUTPUT_RATE     = 44100;
static constexpr int CHANNELS        = 2;
/* Each XA sector → 18 groups × 4 blocks × 28 samples = 2016 stereo pairs */
static constexpr int SAMPLES_PER_SECTOR = 2016;

/* WinMM double-buffer config */
static constexpr int NUM_BUFS        = 4;
static constexpr int SAMPLES_PER_BUF = 4096; /* stereo sample pairs per buffer */
static constexpr int BYTES_PER_BUF   = SAMPLES_PER_BUF * CHANNELS * sizeof(int16_t);

/* XA ADPCM filter coefficients (×64 fixed point, denominator = 64) */
static const int K0[5] = {   0, 60, 115,  98, 122 };
static const int K1[5] = {   0,  0, -52, -55, -60 };

/* WAV capture limit */
static constexpr uint32_t WAV_CAPTURE_PAIRS = (uint32_t)SAMPLE_RATE * 30; /* 30 seconds */

/* ---- global state ---- */
static float              g_volume = 0.5f; /* 0.0-1.0, default 50% */
static FILE*              g_bin  = nullptr;

static std::thread             g_thread;
static std::atomic<bool>       g_running{false};
static std::atomic<uint32_t>   g_seek_lba{UINT32_MAX};

/* Channel lock — set to first audio file/channel seen after each seek */
static uint8_t  g_xa_file    = 0xFF;  /* 0xFF = not yet locked */
static uint8_t  g_xa_channel = 0xFF;

/* WAV capture (written from audio thread only) */
static FILE*     g_wav_out     = nullptr;
static uint32_t  g_wav_samples = 0;   /* stereo pairs written */

/* ---- Circular Buffer ---- */
static constexpr int RING_BUF_SAMPLES = SAMPLES_PER_BUF * 8; // 32768 stereo samples
static int16_t g_ring_buf[RING_BUF_SAMPLES * CHANNELS];
static std::atomic<size_t> g_ring_write{0};
static std::atomic<size_t> g_ring_read{0};

/* ---------------------------------------------------------------------------
 * XA-ADPCM sector decode
 *
 * Layout of each 128-byte sound group (4-bit stereo):
 *   Bytes  0- 3 : copy of bytes 4-7 (error recovery duplicate)
 *   Bytes  4-11 : sound parameters — 4 blocks × 2 bytes (L then R per block)
 *                  L params for block blk: grp[4 + blk*2]     (nibble=0)
 *                  R params for block blk: grp[4 + blk*2 + 1] (nibble=1)
 *                  bits 3-0 = shift_factor, bits 5-4 = filter_index (0-3)
 *   Bytes 12-15 : copy of bytes 8-11 (error recovery duplicate)
 *   Bytes 16-127: sample data — 28 bytes per block, stride 4
 *                  sample byte for block blk, sample i: grp[16 + blk + i*4]
 *                  L sample = low  nibble (bits 3-0) of that byte
 *                  R sample = high nibble (bits 7-4) of that byte
 *
 * 18 groups × 4 blocks × 28 samples = 2016 stereo pairs per sector.
 * --------------------------------------------------------------------------- */
static int xa_decode_sector(const uint8_t* data,  /* XA_DATA_SIZE bytes */
                             int16_t* out,          /* 2016 stereo pairs output */
                             int32_t prevL[2],
                             int32_t prevR[2])
{
    int pair = 0; /* output stereo pair index */

    for (int g = 0; g < XA_SOUND_GROUPS; g++) {
        const uint8_t* grp = data + g * 128;

        for (int blk = 0; blk < 4; blk++) {
            /* Sound parameters: L at grp[4+blk*2], R at grp[4+blk*2+1] */
            uint8_t hdrL = grp[4 + blk * 2];
            uint8_t hdrR = grp[4 + blk * 2 + 1];
            int shiftL  = 12 - (hdrL & 0x0F);
            int shiftR  = 12 - (hdrR & 0x0F);
            int filterL = (hdrL >> 4) & 0x03;
            int filterR = (hdrR >> 4) & 0x03;

            /* 28 sample bytes per block at stride 4.
             * L = low nibble, R = high nibble of the SAME byte. */
            for (int i = 0; i < 28; i++) {
                uint8_t b = grp[16 + blk + i * 4];
                int nibL = b & 0x0F;
                int nibR = (b >> 4) & 0x0F;

                int32_t sL = (int32_t)((nibL >= 8) ? (nibL - 16) : nibL);
                sL = (sL << shiftL);
                sL += (K0[filterL] * prevL[0] + K1[filterL] * prevL[1] + 32) >> 6;
                if (sL >  32767) sL =  32767;
                if (sL < -32768) sL = -32768;
                prevL[1] = prevL[0];  prevL[0] = sL;

                int32_t sR = (int32_t)((nibR >= 8) ? (nibR - 16) : nibR);
                sR = (sR << shiftR);
                sR += (K0[filterR] * prevR[0] + K1[filterR] * prevR[1] + 32) >> 6;
                if (sR >  32767) sR =  32767;
                if (sR < -32768) sR = -32768;
                prevR[1] = prevR[0];  prevR[0] = sR;

                out[pair * 2]     = (int16_t)sL;
                out[pair * 2 + 1] = (int16_t)sR;
                pair++;
            }
        }
    }
    return pair; /* = 18 groups × 4 blocks × 28 = 2016 */
}

static size_t get_free() {
    size_t w = g_ring_write.load(std::memory_order_relaxed);
    size_t r = g_ring_read.load(std::memory_order_relaxed);
    if (w >= r) return RING_BUF_SAMPLES - 1 - (w - r);
    return r - w - 1;
}

static size_t ring_write(const int16_t* data, size_t pairs) {
    size_t w = g_ring_write.load(std::memory_order_relaxed);
    size_t r = g_ring_read.load(std::memory_order_relaxed);
    size_t free_slots = (w >= r) ? (RING_BUF_SAMPLES - 1 - (w - r)) : (r - w - 1);
    size_t to_write = (pairs < free_slots) ? pairs : free_slots;
    for (size_t i = 0; i < to_write; i++) {
        g_ring_buf[w * 2]     = data[i * 2];
        g_ring_buf[w * 2 + 1] = data[i * 2 + 1];
        w = (w + 1) % RING_BUF_SAMPLES;
    }
    g_ring_write.store(w, std::memory_order_release);
    return to_write;
}

extern "C" size_t xa_audio_read_pcm(int16_t* out, size_t pairs) {
    size_t w = g_ring_write.load(std::memory_order_acquire);
    size_t r = g_ring_read.load(std::memory_order_relaxed);
    size_t avail = (w >= r) ? (w - r) : (RING_BUF_SAMPLES - (r - w));
    size_t to_read = (pairs < avail) ? pairs : avail;
    for (size_t i = 0; i < to_read; i++) {
        out[i * 2]     = g_ring_buf[r * 2];
        out[i * 2 + 1] = g_ring_buf[r * 2 + 1];
        r = (r + 1) % RING_BUF_SAMPLES;
    }
    g_ring_read.store(r, std::memory_order_release);
    return to_read;
}

/* ---------------------------------------------------------------------------
 * Audio stream worker thread
 * --------------------------------------------------------------------------- */
static void audio_thread()
{
    uint8_t  raw[RAW_SECTOR];
    int16_t  sect_pcm[SAMPLES_PER_SECTOR * CHANNELS];
    int32_t  prevL[2] = {0, 0}, prevR[2] = {0, 0};

    int sect_pos  = 0; /* drain position within sect_pcm (in stereo pairs) */
    int sect_rem  = 0; /* undrained stereo pairs from last decoded sector */
    uint32_t lba  = UINT32_MAX;
    int no_audio  = 0; /* consecutive non-audio sectors counter */

    int32_t  resamp_phase = 0;

    while (g_running.load(std::memory_order_relaxed)) {

        /* Pick up new seek request */
        uint32_t req = g_seek_lba.exchange(UINT32_MAX, std::memory_order_acq_rel);
        if (req != UINT32_MAX) {
            g_ring_write.store(0, std::memory_order_relaxed);
            g_ring_read.store(0, std::memory_order_relaxed);
            if (req == 0) {
                lba = UINT32_MAX;
                prevL[0] = prevL[1] = prevR[0] = prevR[1] = 0;
                sect_pos = sect_rem = 0;
                no_audio = 0;
                resamp_phase = 0;
                g_xa_file    = 0xFF;
                g_xa_channel = 0xFF;
                printf("[XA] Stop\n");
                fflush(stdout);
            } else {
                lba = req;
                prevL[0] = prevL[1] = prevR[0] = prevR[1] = 0;
                sect_pos = sect_rem = 0;
                no_audio = 0;
                resamp_phase = 0;
                g_xa_file    = 0xFF;
                g_xa_channel = 0xFF;
                if (g_wav_out) {
                    fseek(g_wav_out, 44, SEEK_SET);
                    g_wav_samples = 0;
                }
                printf("[XA] Seek → LBA %u\n", lba);
                fflush(stdout);
            }
        }

        if (lba == UINT32_MAX) { Sleep(10); continue; }

        if (sect_rem > 0) {
            size_t free_slots = get_free();
            if (free_slots == 0) {
                Sleep(5);
                continue;
            }

            float vol = g_volume;
            int16_t temp_buf[512 * 2];
            int to_gen = (free_slots < 512) ? (int)free_slots : 512;
            int produced = 0;
            while (produced < to_gen && sect_rem > 0) {
                int16_t cL = sect_pcm[sect_pos * 2];
                int16_t cR = sect_pcm[sect_pos * 2 + 1];
                int16_t nL = (sect_rem > 1) ? sect_pcm[(sect_pos + 1) * 2]     : cL;
                int16_t nR = (sect_rem > 1) ? sect_pcm[(sect_pos + 1) * 2 + 1] : cR;
                int32_t lerpL = (int32_t)cL + (int32_t)(nL - cL) * resamp_phase / OUTPUT_RATE;
                int32_t lerpR = (int32_t)cR + (int32_t)(nR - cR) * resamp_phase / OUTPUT_RATE;
                temp_buf[produced * 2]     = (int16_t)(lerpL * vol);
                temp_buf[produced * 2 + 1] = (int16_t)(lerpR * vol);
                produced++;
                resamp_phase += SAMPLE_RATE;
                while (resamp_phase >= OUTPUT_RATE) {
                    resamp_phase -= OUTPUT_RATE;
                    sect_pos++;
                    sect_rem--;
                }
            }
            ring_write(temp_buf, produced);
            continue;
        }

        /* Read next sector */
        long off = (long)((uint64_t)lba * RAW_SECTOR);
        if (fseek(g_bin, off, SEEK_SET) != 0 ||
            fread(raw, 1, RAW_SECTOR, g_bin) != RAW_SECTOR) {
            lba = UINT32_MAX;
            continue;
        }

        uint8_t submode  = raw[XA_SUBHDR_OFF + 2];
        uint8_t file_num = raw[XA_SUBHDR_OFF + 0];
        uint8_t chan_num = raw[XA_SUBHDR_OFF + 1];

        if (submode & SUBMODE_AUDIO) {
            if (g_xa_file == 0xFF) {
                g_xa_file    = file_num;
                g_xa_channel = chan_num;
                uint8_t coding = raw[XA_SUBHDR_OFF + 3];
                printf("[XA] Locked to file=%u ch=%u codingInfo=0x%02X (%s %s %s)\n",
                       file_num, chan_num, coding,
                       (coding & 0x01) ? "stereo" : "mono",
                       ((coding >> 2) & 0x03) == 0 ? "37800Hz" : "18900Hz",
                       ((coding >> 4) & 0x03) == 0 ? "4-bit" : "8-bit");
                fflush(stdout);
            }

            no_audio = 0;

            if (file_num == g_xa_file && chan_num == g_xa_channel) {
                xa_decode_sector(raw + XA_DATA_OFF, sect_pcm, prevL, prevR);
                sect_pos = 0;
                sect_rem = SAMPLES_PER_SECTOR;

                if (g_wav_out && g_wav_samples < WAV_CAPTURE_PAIRS) {
                    uint32_t space = WAV_CAPTURE_PAIRS - g_wav_samples;
                    uint32_t n = ((uint32_t)SAMPLES_PER_SECTOR < space)
                                 ? (uint32_t)SAMPLES_PER_SECTOR : space;
                    fwrite(sect_pcm, sizeof(int16_t) * CHANNELS, n, g_wav_out);
                    g_wav_samples += n;
                }
            }
        } else {
            if (++no_audio > 20) {
                printf("[XA] No audio sectors near LBA %u — stopping\n", lba);
                fflush(stdout);
                lba = UINT32_MAX;
                continue;
            }
        }

        lba++;
    }

    printf("[XA] Audio thread exiting\n");
    fflush(stdout);
}

/* ---------------------------------------------------------------------------
 * Public C interface
 * --------------------------------------------------------------------------- */
extern "C" {

void xa_audio_init(const char* bin_path)
{
    g_bin = fopen(bin_path, "rb");
    if (!g_bin) {
        fprintf(stderr, "[XA] Cannot open BIN: %s\n", bin_path);
        return;
    }
    printf("[XA] Opened %s\n", bin_path);

    /* Open WAV capture file — written from audio thread, no lock needed */
    g_wav_samples = 0;
    g_wav_out = fopen("./debug/xa_capture.wav", "wb");
    if (g_wav_out) {
        /* Write RIFF WAV header; data_size fields are placeholders — fixed at shutdown */
        uint32_t data_size   = 0;
        uint32_t riff_size   = 36 + data_size;
        uint16_t num_ch      = 2;
        uint32_t sample_rate = (uint32_t)SAMPLE_RATE;
        uint32_t byte_rate   = sample_rate * 2 * 2;
        uint16_t block_align = 4;
        uint16_t bits        = 16;
        uint16_t pcm_type    = 1;
        uint32_t fmt_size    = 16;
        fwrite("RIFF",       1, 4, g_wav_out);
        fwrite(&riff_size,   4, 1, g_wav_out);
        fwrite("WAVE",       1, 4, g_wav_out);
        fwrite("fmt ",       1, 4, g_wav_out);
        fwrite(&fmt_size,    4, 1, g_wav_out);
        fwrite(&pcm_type,    2, 1, g_wav_out);
        fwrite(&num_ch,      2, 1, g_wav_out);
        fwrite(&sample_rate, 4, 1, g_wav_out);
        fwrite(&byte_rate,   4, 1, g_wav_out);
        fwrite(&block_align, 2, 1, g_wav_out);
        fwrite(&bits,        2, 1, g_wav_out);
        fwrite("data",       1, 4, g_wav_out);
        fwrite(&data_size,   4, 1, g_wav_out);
        printf("[XA] WAV capture: ./debug/xa_capture.wav (up to 30s)\n");
    } else {
        printf("[XA] WARNING: could not open ./debug/xa_capture.wav for writing\n");
    }

    g_ring_write.store(0, std::memory_order_relaxed);
    g_ring_read.store(0, std::memory_order_relaxed);

    g_running = true;
    g_thread  = std::thread(audio_thread);

    printf("[XA] Audio ready (decode %d Hz → ring buffer)\n", SAMPLE_RATE);
    fflush(stdout);
}

void xa_audio_set_volume(float v)
{
    if (v < 0.0f) v = 0.0f;
    if (v > 1.0f) v = 1.0f;
    g_volume = v;
    printf("[XA] Volume set to %.0f%%\n", v * 100.0f);
    fflush(stdout);
}

void xa_audio_seek(uint32_t lba)
{
    g_seek_lba.store(lba, std::memory_order_release);
}

void xa_audio_shutdown(void)
{
    g_running = false;
    if (g_thread.joinable()) g_thread.join();

    /* Finalize WAV capture header with actual data size */
    if (g_wav_out) {
        uint32_t data_size = g_wav_samples * (uint32_t)(CHANNELS * sizeof(int16_t));
        uint32_t riff_size = 36 + data_size;
        fseek(g_wav_out, 4,  SEEK_SET);  fwrite(&riff_size, 4, 1, g_wav_out);
        fseek(g_wav_out, 40, SEEK_SET);  fwrite(&data_size, 4, 1, g_wav_out);
        fclose(g_wav_out);
        g_wav_out = nullptr;
        printf("[XA] WAV saved: %u stereo pairs = %.1f sec\n",
               g_wav_samples, (float)g_wav_samples / (float)SAMPLE_RATE);
    }

    if (g_bin) {
        fclose(g_bin);
        g_bin = nullptr;
    }
}

} /* extern "C" */
