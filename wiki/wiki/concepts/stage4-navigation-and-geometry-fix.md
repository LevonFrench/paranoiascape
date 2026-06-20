---
title: "Stage4 Navigation And Geometry Fix"
category: concept
sources: []
compiled-from: conversation
created: 2026-06-15
updated: 2026-06-19
tags: [psx, recomp, geometry, navigation]
confidence: high
summary: "Fixes for the SDK GsGetLsm coordinate rotation matrix collapse, GPU FillRectangle masking, and verification of Stage 4 menu navigation."
volatility: warm
---

# Stage 4 Menu Navigation & 3D Geometry Fixes

This document details the fixes implemented in June 2026 to resolve collapsed 3D geometry and VRAM texture corruption when loading and executing Stage 4 gameplay.

## 1. 3D Geometry Vertices Collapse

### Symptom
When loading Stage 4 gameplay, all 3D enemy models (skeletons) collapsed to the center of the screen `(0,0)`, rendering them invisible or as a tiny cluster of points.

### Root Cause
During level initialization, the game sets up coordinate structures for active 3D models. In some cases, the local coordinate system rotation matrix is initialized to all-zeros. When passed to the PsyQ library function `GsGetLsm` (`0x80069F44`), the zeros are loaded into the GTE matrix registers. All subsequent vertex transformations projected via GTE collapsed to the origin.

### Fix
Implemented a runtime override intercept for address `0x80069F44` inside `runtime.c`:
```c
if (phys == 0x80069F44u) {
    /* Inspect coordinate matrix. If all-zeros, patch with identity matrix */
    uint32_t coord_ptr = cpu->a0;
    uint8_t *ram = psx_get_ram();
    // Offset 4 is the start of the 3x3 rotation matrix (18 bytes in 4.12 fixed-point)
    uint32_t mat_offset = (coord_ptr + 4) & 0x1FFFFF;
    int all_zeros = 1;
    for (int i = 0; i < 18; i++) {
        if (ram[mat_offset + i] != 0) {
            all_zeros = 0;
            break;
        }
    }
    if (all_zeros) {
        /* Write identity matrix: diagonals set to 4096 (0x1000) */
        uint16_t *mat = (uint16_t*)&ram[mat_offset];
        mat[0] = 4096; // R11
        mat[4] = 4096; // R22
        mat[8] = 4096; // R33
    }
}
```
This patches the matrix to identity before delegating to native recompiled execution, maintaining valid model projection.

---

## 2. VRAM Fills & Texture Corruption

### Symptom
Stage 4 models and HUD assets loaded to VRAM but were immediately overwritten or cleared, leaving the screen completely blank during gameplay.

### Root Cause
GP0 command `0x02` (Fill Image Area) was called during loading with unmasked parameters: width `w = 64781` and height `h = 519`. The OpenGL renderer clamped these parameters to the default VRAM bounds (`1024x512`), effectively clearing the entire VRAM area and wiping loaded textures.

### Fix
Added register masking to `w` and `h` in `FillRectangle` inside `opengl_renderer.cpp`:
```cpp
w &= 0x3FF; // 10-bit width limit
h &= 0x1FF; // 9-bit height limit
```
This restricts the clear to the intended hardware limits, preventing corruption of neighboring VRAM texture pages.

---

## 3. Automation & Verification

We successfully navigated and tested the stage progression using:
```powershell
python -u scratch/test_stages.py --stage 4 --frames 13050
```

- **Stage Select Navigation**: Presses `DOWN` three times to highlight Stage 4, then confirms with `CROSS` (`0x4000`).
- **Gameplay Load**: Bypasses the tutorial screen (`osm == 10`) by pressing `START` once, and correctly transitions to active gameplay (`osm == 0`).
- **Outcome**: The game executes gameplay successfully past frame 13000. Verified that the skeletons render correctly with loaded textures.
