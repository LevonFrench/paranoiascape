---
title: "Paranoiascape Input Graphics"
category: topic
sources: []
compiled-from: conversation
created: 2026-05-01
updated: 2026-06-19
tags: [psx, recomp, input, graphics]
confidence: high
summary: "Explains the SIO polling buffer at 0x8013E5EC, pcsx_cmd.py injection to bypass menu gates, and root cause analysis of squashed Title Screen graphics."
volatility: warm
---

# Paranoiascape Input Injection and Title Screen Graphics

## Input Injection & Bypassing the Title Screen
Paranoiascape uses direct hardware SIO bit-banging rather than standard `PadRead` BIOS calls. The controller buffer has been identified in RAM at `0x8013E5EC` (active-low, little-endian).

To remotely control the game and bypass the gating sequences during reverse engineering, input can be injected directly into this buffer using the PCSX-Redux REST API (`pcsx_cmd.py`):

1. **Bypass Title Screen (Press Start):**
   ```bash
   python pcsx_cmd.py write 8013E5EC F7FFF7FF
   ```
   *(Release by writing `FFFFFFFF`)*

2. **Select Main Menu Option (Press Circle):**
   ```bash
   python pcsx_cmd.py write 8013E5EC DFFFFDFF
   ```
   *(Release by writing `FFFFFFFF`)*

This provides a reliable, timing-independent method to automatically navigate the game menus without relying on Lua `pad_cmd.txt` IPC polling.

**Important Note for PSXRecomp:** While this injection works on the live PCSX-Redux emulator, injecting directly into `0x8013E5EC` within the recompiler fails to trigger a state transition. This is because Paranoiascape relies heavily on BIOS VBlank callbacks to poll SIO (`FUN_8008d60c`) and process higher-level input state transitions. Since PSXRecomp does not emulate the BIOS interrupt controller or automatically fire VBlank callbacks, the engine logic that reads `0x8013E5EC` to advance the game state machine is never executed. To bypass the Title Screen in the recompiler, the specific game state transition functions must be manually called or overridden in the VSync heartbeat pump.
## Title Screen Graphic Distortion Analysis
The Title Screen (the spinning eye) exhibits significant graphic distortion in the PSXRecomp renderer, appearing squashed horizontally with garbage rendering on the right half of the screen.

### Root Cause
1. **24-bit Color Mode:** The game sets the display mode via `GP1(08h)` to 640x480 interlaced with the `color_depth_24bit` flag enabled (bit 4). The FMV decoder writes raw 24-bit RGB pixels (3 bytes per pixel) into VRAM.
2. **Renderer Ignorance:** While `gpu_interpreter.cpp` correctly parses the 24-bit flag and passes it to `OpenGLRenderer::SetDisplayMode()`, the `display_24bit_` variable is entirely ignored by the final rendering pass (`Present()`).
3. **15-bit Interpretation:** The fragment shader reads the 24-bit VRAM data as standard 16-bit (15-bpp) pixels. Because 24-bit pixels are 1.5x larger, reading them as 16-bit compresses the image horizontally by exactly 1.5x (e.g., 640 pixels become ~426 pixels). The remaining space on the right side of the 640x480 quad is filled with uninitialized or unrelated VRAM garbage.

### Conclusion / "Different Angle"
Fixing the 24-bit rendering would require rewriting the `Present()` fragment shader to decode 24-bit VRAM packed pixels. However, because the actual 3D gameplay utilizes standard 320x240 15-bpp rendering, the "different angle" is to simply bypass the Title Screen entirely using the input injection technique described above, avoiding the need to emulate the PS1's obscure 24-bit display mode scaling.
