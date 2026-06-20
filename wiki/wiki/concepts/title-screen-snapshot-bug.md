---
title: "Title Screen Snapshot Bug"
category: concept
sources: []
compiled-from: conversation
created: 2026-05-30
updated: 2026-06-19
tags: [psx, recomp, snapshot, graphics]
confidence: high
summary: "Root cause analysis of the title screen black rendering bug where the display_snapshot_ capture in FlushPrimitives was misaligned with the game's FillRect sequence."
volatility: warm
---

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

## The Solution (Resolved 2026-06-19)

We resolved the black-screen/snapshot issue by identifying that when `Present()` calls `FlushPrimitives()` to force a draw before presenting, the vertex buffer is often empty (since all primitives were already flushed earlier). This empty state triggered the early-return block in `FlushPrimitives()`.

In the early-return path, the code attempted to copy the FBO state to `display_snapshot_` via:
```cpp
if (in_present_ && display_snapshot_ != 0) {
    glBindTexture(GL_TEXTURE_2D, display_snapshot_);
    glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, 1024, 512);
    glBindTexture(GL_TEXTURE_2D, 0);
}
```
**The bug**: The FBO containing the game's actual rendered VRAM (`vram_fbo_`) was **not bound** to `GL_READ_FRAMEBUFFER` during this copy. Instead, the OpenGL context copied from the default screen framebuffer (which is bound to the window for presenting, but holds no valid frame data yet). This resulted in a blank/black `display_snapshot_` and black screenshots, even though the raw `vram_fbo_` contained the correct pixels.

**The Fix**:
We wrapped the copy operation with explicit `glBindFramebuffer` binds:
```cpp
if (in_present_ && display_snapshot_ != 0) {
    glBindFramebuffer(GL_READ_FRAMEBUFFER, vram_fbo_);
    glBindTexture(GL_TEXTURE_2D, display_snapshot_);
    glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, 1024, 512);
    glBindTexture(GL_TEXTURE_2D, 0);
    glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
}
```
This ensures the snapshot texture is always copied from the recompiled game's VRAM FBO, restoring correct screen rendering and screenshots in the runner.

## Related Files
- [opengl_renderer.cpp](file:///j:/projects/paranoia/paranoiascape-recomp/runner/src/opengl_renderer.cpp) — FlushPrimitives snapshot at L2031, Present at L884
- [HANDOFF.md](file:///j:/projects/paranoia/HANDOFF.md) — Full context

