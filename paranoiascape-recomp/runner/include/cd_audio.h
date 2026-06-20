#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// Initialize the CD-DA audio system, discovering tracks in the given base path
void cd_audio_init(const char* cue_path);

// Play a specific CD track (e.g. 2 for track 2)
void cd_audio_play_track(int track);

// Stop playback
void cd_audio_stop(void);

// Set volume (0.0 to 1.0)
void cd_audio_set_volume(float v);

// Shutdown the CD-DA audio system
void cd_audio_shutdown(void);

// Consume decoded stereo samples from the circular buffer. Returns number of sample pairs read.
size_t cd_audio_read_pcm(int16_t* out, size_t pairs);

#ifdef __cplusplus
}
#endif
