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

// Shutdown the CD-DA audio system
void cd_audio_shutdown(void);

#ifdef __cplusplus
}
#endif
