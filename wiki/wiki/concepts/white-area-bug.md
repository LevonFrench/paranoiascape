---
title: "White Area Bug"
category: concept
sources: []
compiled-from: conversation
created: 2026-05-11
updated: 2026-06-19
tags: [psx, recomp, graphics, fmv]
confidence: high
summary: "Root cause analysis of the white area FMV background bug caused by FMV quads sampling incorrect sprite index data instead of decoded FMV frames."
volatility: warm
---

# White Area Rendering Artifact — RESOLVED

## Summary
The Title Screen shows a large white rectangle covering most of the 640×480 display after FMV playback ends.

## Root Cause (Confirmed 2026-05-30)

The white comes from **8-bit CLUT-indexed textured quads** sampling from VRAM (640,0). Without FMV data populating that region, all texels are 0xFFFF → index 0xFF or 0x00. **CLUT[0] = 0x7FFF = white**.

The game renders 8 background quads (tpg=(640,0), clut=(0,511)) covering (128,0)-(512,384) and 2 "PRESS START" quads (clut=(0,510)) at (296,198)-(360,262).

## Fix: CLUT Quantizer (Implemented, Disabled)

A 15-bit→8-bit CLUT quantizer was implemented in `fmv_player.cpp`:
- `build_clut_lut()`: 32768-entry LUT mapping RGB555 → nearest CLUT index. Cached by CLUT hash.
- `upload_indexed_frame()`: Converts FMV frames to 8-bit indexed format, uploads to (640,0,128,240).
- **Currently disabled** because the display_snapshot_ timing bug makes the title screen black regardless.

## Why White Specifically?
- CLUT at (0,511) has 256 entries. Entry 0 = 0x7FFF = maximum white in RGB555.
- Uninitialized VRAM at (640,0) = 0x0000 (zero-initialized). In 8-bit mode, each byte is an index. 0x00 → CLUT[0] = white.
- After FMV plays, the last frames fade to white (0xFFFF = index 0xFF). Without quantizer, raw 15-bit data is meaningless as indices.

## Key CLUT Details
| Address | Purpose | Notable Entries |
|---------|---------|-----------------|
| (0,511) | Main background CLUT | CLUT[0]=0x7FFF (white) |
| (0,510) | "PRESS START" text CLUT | Different palette |
| Both uploaded at boot (f72) via CPUToVRAM(0,511,256,1) and (0,510,256,1) |

## Current Status
The white area is a **secondary symptom**. The primary blocker is the [[title-screen-snapshot-bug|display_snapshot_ timing bug]] ([display_snapshot_ timing bug](../concepts/title-screen-snapshot-bug.md)) that makes the title screen render as black. Once the snapshot bug is fixed, the quantizer can be re-enabled to replace white with actual FMV-derived background content.
