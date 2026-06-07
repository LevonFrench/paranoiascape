/* fmv_player.h — PS1 STR v2 FMV decoder
 *
 * Decodes MPEG-1 VLC bitstreams from raw BIN sectors, performs IDCT and
 * YCbCr→RGB555 conversion, and uploads decoded frames to VRAM.
 *
 * C interface — called from runtime.c and cdrom_stub.cpp.
 */
#pragma once
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Open the BIN file for FMV reading (called alongside xa_audio_init). */
void fmv_player_init(const char* bin_path);

/* Set the starting LBA for the FMV stream (called on CdlSeekL).
 * filename is optional (may be NULL) — used only for logging. */
void fmv_player_seek(uint32_t lba, const char* filename);

/* Process one sector and decode a frame if complete.
 * Uploads decoded RGB555 frame to VRAM and calls psx_present_frame().
 * Returns 1 when frame is ready (or FMV is done), 0 if still reading. */
int fmv_player_tick(void);

/* True if a FMV seek has been issued and FMV hasn't finished yet. */
int fmv_player_is_active(void);

/* True if FMV is actively displaying frames (alias for is_active). */
int fmv_player_is_displaying(void);

/* Re-upload the current FMV frame to VRAM (640,0).  Call right before
 * rendering to ensure FMV data isn't overwritten by game sprite uploads. */
void fmv_refresh_vram(void);

/* Deactivate FMV playback (called on user skip). */
void fmv_player_stop(void);

/* Release resources. */
void fmv_player_shutdown(void);

#ifdef __cplusplus
}
#endif
