# Title Screen Rendering — Display Snapshot Timing Bug

## Date: 2026-05-30

## Summary
After FMV playback ends (~f1383), the title screen renders BLACK despite valid content existing in the VRAM FBO. Root cause: the `display_snapshot_` capture timing is misaligned with the game's multi-pass rendering.

## The Problem

The game submits 8-12 DMA chains per frame at the title screen. Each chain contains:
```
E3/E4/E5/E1/E2/E6 (setup) + FillRect(0,0,640,480) (clear to black)
```

The `FillRectangle()` function calls `FlushPrimitives()` before rendering the fill quad. If there are pending polygon vertices from a previous DMA chain, FlushPrimitives:
1. Draws them to the FBO
2. Captures display_snapshot_ via glCopyTexSubImage2D
3. Clears vertex_buffer_

Then FillRect renders BLACK over the FBO.

The NEXT DMA chain also calls FillRect → FlushPrimitives (now empty → returns early, NO snapshot update) → FillRect renders black again.

By the time `Present()` runs and calls FlushPrimitives (empty → returns early), display_snapshot_ contains the LAST snapshot from step 2 above. But that snapshot was immediately followed by a FillRect clear, then MORE FillRects. The snapshot is stale.

## Why VRAM Dumps Show Content But Screenshots Don't

- `SaveVRAMDumpBMP()` reads directly from `vram_fbo_` (the raw FBO). At the moment of capture, the FBO may contain freshly-drawn content from the current DMA chain.
- `SaveScreenshotBMP()` reads from `display_snapshot_` (the snapshot texture). This only updates during FlushPrimitives with non-empty vertex_buffer_.
- Since the snapshot captures happen BETWEEN FillRect clears, the captured content gets wiped by the next FillRect before Present() can display it.

## Evidence

- `auto_f1700.png`: 2538 bytes (nearly all black) — from display_snapshot_
- `vram_auto_f1700.png`: 18KB+ (has purple 8-bit quads + 3D geometry) — from vram_fbo_
- Same frame, same run, wildly different content.

## Potential Fixes

### Option A: Skip snapshot during FillRect-triggered flush
Add a flag to distinguish FillRect-initiated FlushPrimitives from Present()-initiated ones. Only capture snapshot from Present().

### Option B: Capture snapshot in Present() from FBO directly
Instead of relying on FlushPrimitives, have Present() call FlushPrimitives then capture from the FBO before FillRect can clear it. But this may not help if FillRect already ran.

### Option C: Delay FillRect clearing
Buffer FillRect commands and execute them at the START of the next frame instead of immediately. This preserves the rendered content for snapshot capture.

## Related Files
- [opengl_renderer.cpp](file:///j:/projects/paranoia/paranoiascape-recomp/runner/src/opengl_renderer.cpp) — FlushPrimitives snapshot at L2066, FillRectangle at L565, Present at L859
- [HANDOFF.md](file:///j:/projects/paranoia/HANDOFF.md) — Full context
