# FMV Boot Bypass & Title Screen (Updated 2026-05-30)

## TL;DR
FMV playback is now FULLY WORKING. The original bypass (skipping func_8001E468) has been replaced with a proper FFmpeg-based MDEC decoder. LOGO.STR and MOVTEST.STR play at 15fps with correct 320×240 RGB555 output.

## History

### Original Fix (2026-05-10)
Two bugs were fixed that unblocked the boot sequence:
1. **SIO `write_half` bug**: The SIO SPI state machine was only wired to byte writes, but the game uses halfword stores. Fixed by adding state machine logic to `write_half`.
2. **FMV streaming stall**: `func_8001E468` was skipped entirely because CD hardware stubs returned zero data, producing black MDEC frames.

### Current State (2026-05-30)
- `func_8001E468` is now INTERCEPTED (not skipped). It calls `fmv_player_seek()` to start async FFmpeg decode of the target STR file.
- FMV frames are decoded on a background thread and uploaded to VRAM at (0,0) for direct display.
- `fmv_force_display_area()` forces 320×240 mode during FMV.
- `fmv_refresh_vram()` re-uploads FMV data in FlushPrimitives to survive sprite overwrites.
- FMV ends naturally at EOF (691 frames for MOVTEST.STR, 100 for LOGO.STR).

## FMV Pipeline Architecture
```
func_8001E468 intercepted
  → fmv_player_seek(filename, LBA)
  → background thread: FFmpeg decode → g_rgb555 buffer
  → fmv_player_tick() called from VSync hook
    → uploads to (0,0) for framebuffer display
    → [DISABLED] upload_indexed_frame() converts to 8-bit at (640,0)
    → fmv_force_display_area(0, 0, 320, 240)
    → 15fps throttle via QueryPerformanceCounter
```

## Key Addresses
| Address | Role |
|---------|------|
| `0x8001E468` | FMV streaming setup — NOW INTERCEPTED, calls fmv_player_seek() |
| `0x8001E6C4` | MDEC output target config (VRAM 0,0 w=640 h=240) |
| `0x8001EFE8` | CD DMA sector poll — tick function for async FMV |
| `0x1F801040` | SIO TX register (halfword writes tracked) |
| `0x1F801044` | SIO STAT register (returns 0x0287) |

## STR File Inventory
| File | Content | Frames | Notes |
|------|---------|--------|-------|
| LOGO.STR | Developer logo | ~100 | Plays first |
| MOVTEST.STR | Intro cinematic | 691 | Plays second, fades to white at end |
| SMG.STR | Screaming Mad George BTS | ~3000 | In-game bonus content |
| PARAFNMV.STR | In-game FMV | ~500 | Gameplay cutscene |
