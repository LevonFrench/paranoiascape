/* cd_audio.cpp — PS1 CD-DA (Redbook) streaming audio playback using WAV files
 *
 * Simulates PS1 CD audio by streaming pre-extracted WAV files.
 * Uses WinMM waveOut to play 44100Hz 16-bit stereo PCM.
 */
#include "cd_audio.h"
#include <windows.h>
#include <mmsystem.h>
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

static HWAVEOUT           g_wave = NULL;
static WAVEHDR            g_hdrs[NUM_BUFS];
static int16_t            g_pcm[NUM_BUFS][SAMPLES_PER_BUF * CHANNELS];
static HANDLE             g_done_event = NULL;

static std::thread        g_thread;
static std::atomic<bool>  g_running{false};
static std::atomic<int>   g_next_track{-1};
static std::atomic<bool>  g_stop_requested{false};

struct RiffHeader {
    char riff[4];
    uint32_t riffSize;
    char wave[4];
};

struct ChunkHeader {
    char id[4];
    uint32_t size;
};

static void audio_thread_func() {
    FILE* current_wav = nullptr;
    int current_track = -1;

    while (g_running) {
        int target_track = g_next_track.exchange(-1);
        if (target_track != -1) {
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
            if (current_wav) {
                fclose(current_wav);
                current_wav = nullptr;
            }
            g_stop_requested = false;
        }

        if (current_wav) {
            // Find a free buffer
            int free_buf = -1;
            for (int i = 0; i < NUM_BUFS; i++) {
                if (g_hdrs[i].dwFlags & WHDR_DONE) {
                    free_buf = i;
                    break;
                }
            }

            if (free_buf != -1) {
                size_t read_bytes = fread(g_pcm[free_buf], 1, BYTES_PER_BUF, current_wav);
                if (read_bytes == 0) {
                    // Loop or stop? PS1 CD-DA usually loops if requested, but for now we stop.
                    fclose(current_wav);
                    current_wav = nullptr;
                } else {
                    if (read_bytes < BYTES_PER_BUF) {
                        memset((uint8_t*)g_pcm[free_buf] + read_bytes, 0, BYTES_PER_BUF - read_bytes);
                    }
                    g_hdrs[free_buf].dwBufferLength = BYTES_PER_BUF;
                    waveOutWrite(g_wave, &g_hdrs[free_buf], sizeof(WAVEHDR));
                }
            } else {
                // Wait for a buffer to finish
                WaitForSingleObject(g_done_event, 100);
            }
        } else {
            // Idle
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

    g_done_event = CreateEventA(NULL, FALSE, FALSE, NULL);

    WAVEFORMATEX wfx = {0};
    wfx.wFormatTag = WAVE_FORMAT_PCM;
    wfx.nChannels = CHANNELS;
    wfx.nSamplesPerSec = SAMPLE_RATE;
    wfx.wBitsPerSample = 16;
    wfx.nBlockAlign = (wfx.nChannels * wfx.wBitsPerSample) / 8;
    wfx.nAvgBytesPerSec = wfx.nSamplesPerSec * wfx.nBlockAlign;

    if (waveOutOpen(&g_wave, WAVE_MAPPER, &wfx, (DWORD_PTR)g_done_event, 0, CALLBACK_EVENT) != MMSYSERR_NOERROR) {
        fprintf(stderr, "[CD-DA] Failed to open waveOut device\n");
        return;
    }

    for (int i = 0; i < NUM_BUFS; i++) {
        g_hdrs[i].lpData = (LPSTR)g_pcm[i];
        g_hdrs[i].dwBufferLength = BYTES_PER_BUF;
        g_hdrs[i].dwFlags = 0;
        waveOutPrepareHeader(g_wave, &g_hdrs[i], sizeof(WAVEHDR));
        g_hdrs[i].dwFlags |= WHDR_DONE; /* Mark initially free */
    }

    g_running = true;
    g_thread = std::thread(audio_thread_func);
}

void cd_audio_play_track(int track) {
    g_next_track = track;
}

void cd_audio_stop(void) {
    g_stop_requested = true;
}

void cd_audio_shutdown(void) {
    if (!g_running) return;
    g_running = false;
    if (g_thread.joinable()) {
        g_thread.join();
    }

    if (g_wave) {
        waveOutReset(g_wave);
        for (int i = 0; i < NUM_BUFS; i++) {
            waveOutUnprepareHeader(g_wave, &g_hdrs[i], sizeof(WAVEHDR));
        }
        waveOutClose(g_wave);
        g_wave = NULL;
    }
    if (g_done_event) {
        CloseHandle(g_done_event);
        g_done_event = NULL;
    }
}

} /* extern "C" */
