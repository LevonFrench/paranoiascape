/* cd_audio.cpp — PS1 CD-DA (Redbook) streaming audio playback using WAV files
 *
 * Simulates PS1 CD audio by streaming pre-extracted WAV files.
 * Uses WinMM waveOut to play 44100Hz 16-bit stereo PCM.
 */
#include "cd_audio.h"
#include <windows.h>
#include <cstdio>
#include <cstring>
#include <atomic>
#include <thread>
#include <string>

static constexpr int SAMPLE_RATE     = 44100;
static constexpr int CHANNELS        = 2;
static constexpr int NUM_BUFS        = 4;
static constexpr int SAMPLES_PER_BUF = 4096; /* stereo pairs */
static constexpr int BYTES_PER_BUF   = SAMPLES_PER_BUF * CHANNELS * sizeof(int16_t);

static std::string g_base_dir;

static std::thread        g_thread;
static std::atomic<bool>  g_running{false};
static std::atomic<int>   g_next_track{-1};
static std::atomic<bool>  g_stop_requested{false};
static float              g_volume = 0.5f; /* 0.0-1.0, default 50% */

/* ---- Circular Buffer ---- */
static constexpr int RING_BUF_SAMPLES = SAMPLES_PER_BUF * 8; // 32768 stereo samples
static int16_t g_ring_buf[RING_BUF_SAMPLES * CHANNELS];
static std::atomic<size_t> g_ring_write{0};
static std::atomic<size_t> g_ring_read{0};

struct RiffHeader {
    char riff[4];
    uint32_t riffSize;
    char wave[4];
};

struct ChunkHeader {
    char id[4];
    uint32_t size;
};

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

extern "C" size_t cd_audio_read_pcm(int16_t* out, size_t pairs) {
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

static void audio_thread_func() {
    FILE* current_wav = nullptr;
    int current_track = -1;

    while (g_running) {
        int target_track = g_next_track.exchange(-1);
        if (target_track != -1) {
            g_ring_write.store(0, std::memory_order_relaxed);
            g_ring_read.store(0, std::memory_order_relaxed);
            if (current_wav) {
                fclose(current_wav);
                current_wav = nullptr;
            }
            if (target_track > 0) {
                char path[512];
                snprintf(path, sizeof(path), "%s/extractedwav/Paranoia Scape (Japan) (Track %02d).wav", g_base_dir.c_str(), target_track);
                current_wav = fopen(path, "rb");
                if (current_wav) {
                    printf("[CD-DA] Playing track %d: %s\n", target_track, path);
                    // Skip to data chunk
                    RiffHeader riff;
                    if (fread(&riff, 1, sizeof(riff), current_wav) == sizeof(riff)) {
                        ChunkHeader chunk;
                        while (fread(&chunk, 1, sizeof(chunk), current_wav) == sizeof(chunk)) {
                            if (memcmp(chunk.id, "data", 4) == 0) {
                                break;
                            }
                            fseek(current_wav, chunk.size, SEEK_CUR);
                        }
                    }
                } else {
                    printf("[CD-DA] Track %d not found: %s\n", target_track, path);
                }
            }
            current_track = target_track;
        }

        if (g_stop_requested) {
            g_ring_write.store(0, std::memory_order_relaxed);
            g_ring_read.store(0, std::memory_order_relaxed);
            if (current_wav) {
                fclose(current_wav);
                current_wav = nullptr;
            }
            g_stop_requested = false;
        }

        if (current_wav) {
            size_t free_slots = get_free();
            if (free_slots >= 1024) {
                int16_t temp_buf[1024 * 2];
                size_t read_bytes = fread(temp_buf, 1, 1024 * sizeof(int16_t) * 2, current_wav);
                if (read_bytes == 0) {
                    fclose(current_wav);
                    current_wav = nullptr;
                } else {
                    size_t read_pairs = read_bytes / (sizeof(int16_t) * 2);
                    if (read_pairs < 1024) {
                        memset(temp_buf + read_pairs * 2, 0, (1024 - read_pairs) * sizeof(int16_t) * 2);
                    }
                    
                    // Scale volume
                    float vol = g_volume;
                    for (size_t i = 0; i < 1024; i++) {
                        temp_buf[i * 2]     = (int16_t)(temp_buf[i * 2] * vol);
                        temp_buf[i * 2 + 1] = (int16_t)(temp_buf[i * 2 + 1] * vol);
                    }
                    
                    ring_write(temp_buf, 1024);
                }
            } else {
                Sleep(5);
            }
        } else {
            Sleep(10);
        }
    }

    if (current_wav) {
        fclose(current_wav);
    }
}

extern "C" {

void cd_audio_init(const char* cue_path) {
    if (!cue_path) return;

    // Determine base directory from cue path
    std::string path(cue_path);
    size_t last_slash = path.find_last_of("\\/");
    if (last_slash != std::string::npos) {
        g_base_dir = path.substr(0, last_slash);
    } else {
        g_base_dir = ".";
    }
    
    // We expect the WAV files to be in <g_base_dir>/extractedwav/

    g_ring_write.store(0, std::memory_order_relaxed);
    g_ring_read.store(0, std::memory_order_relaxed);

    g_running = true;
    g_thread = std::thread(audio_thread_func);
    printf("[CD-DA] Audio ready (WAV streaming → ring buffer)\n");
    fflush(stdout);
}

void cd_audio_play_track(int track) {
    g_next_track = track;
}

void cd_audio_stop(void) {
    g_stop_requested = true;
}

void cd_audio_set_volume(float v) {
    if (v < 0.0f) v = 0.0f;
    if (v > 1.0f) v = 1.0f;
    g_volume = v;
}

void cd_audio_shutdown(void) {
    if (!g_running) return;
    g_running = false;
    if (g_thread.joinable()) {
        g_thread.join();
    }
}

} /* extern "C" */
