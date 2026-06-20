---
title: "Psyz Psycross Analysis"
category: reference
sources: []
compiled-from: conversation
created: 2026-05-15
updated: 2026-06-19
tags: [psx, recomp, sdk, library]
confidence: high
summary: "Analysis of the PSY-Z SDK samples, matching decompilations, and PsyCross library components to guide porting in paranoiascape-recomp."
volatility: warm
---

# PSY-Z vs PsyCross: What We Can Actually Use

## Architecture Comparison

### How Our Runner Works (PSXRecomp)
Our runner is a **static recompiler**. The game's MIPS code is compiled to native C. COP2 (GTE) instructions are intercepted at the instruction level — each `cop2` opcode calls `gte_execute()` in `gte.cpp`, which operates on raw register state (`gte_data[32]`, `gte_ctrl[32]`) on the `CPUState`. PsyQ **library functions** (like `RotMatrix`, `CdSearchFile`) are compiled MIPS code that the recompiled game calls natively — they issue COP2 instructions that flow through our GTE, or they poke hardware registers that we intercept in `runtime.c`.

### How PSY-Z Works
PSY-Z is a **source-level porting library**. It replaces PsyQ headers so that a game's **C source code** compiles against PSY-Z's native implementations instead of the original SDK. It provides high-level functions (`RotMatrix`, `InitGeom`, `CdSearchFile`) as native C. Its GTE is a software simulation using static variables (not register-level).

### How PsyCross Works
PsyCross is also a **source-level porting library**, similar in purpose to PSY-Z but more mature in certain areas. It has a full OpenGL renderer, PGXP-Z support, ISO 9660 filesystem parsing, and SPU with ADPCM via OpenAL. Its GTE implementation (`PsyX_GTE.cpp`) uses the exact same register-level approach as real hardware, including the division lookup table.

## What We Can Use

### 1. GTE (Geometry Transformation Engine) — NOTHING TO CHANGE
**Verdict: Keep our `gte.cpp`. Use PSY-Z/PsyCross as cross-reference only.**

Our `gte.cpp` already implements all 20+ GTE commands at the COP2 instruction level (RTPS, RTPT, NCLIP, AVSZ3, AVSZ4, MVMVA, NCDS, NCDT, NCCS, NCCT, NCS, NCT, DPCS, DPCT, DPCL, INTPL, CDP, CC, OP, SQR, GPF, GPL). This is exactly the right architecture for a recompiler — the game's compiled MIPS code issues COP2 instructions, and we execute them.

Neither PSY-Z nor PsyCross can replace this because they work at the **library API level** (`RotMatrix()`, `RotTransPers()`), not the instruction level. Our game never calls `RotMatrix()` as a C function — it executes the compiled MIPS assembly of `RotMatrix`, which issues COP2 instructions.

**Cross-reference value:**
- PsyCross's `PsyX_GTE.cpp` uses the **hardware division lookup table** (lines 122-158), which is more accurate than our simplified `(H*0x20000/SZ3+1)/2` formula in `gte_divide()`. If we see division artifacts, we should adopt PsyCross's table.
- PSY-Z's `libgte.c` is cleaner to read for understanding the math (good documentation reference), but has incomplete implementations (`RotMatrixX/Y/Z/YXZ`, `MulMatrix`, `ScaleMatrix`, `TransposeMatrix`, `VectorNormal` are all `NOT_IMPLEMENTED`).

### 2. CD-ROM (File System) — USE PSYCROSS'S ISO 9660 PARSER
**Verdict: Extract PsyCross's `iso9660.h` + `CdSearchFile` + `CdRead`/`CdReadSync` logic.**

This is the biggest win. Our `runtime.c` currently has fragile, game-specific CD intercepts that:
- Hardcode file table addresses (`0x80091FB4`)
- Manually extract filename/LBA from game-specific 12-byte entries
- Bypass `CdSync`/`CdReady` with instant returns (Rule 0g violation)

PsyCross's `LIBCD.C` has a working ISO 9660 parser that:
- Reads the root directory at sector 22 (`CD_ROOT_DIRECTORY_SECTOR`)
- Walks the TOC entries to find files by path
- Returns proper `CdlFILE` with MSF position and size
- Has a command queue for `CdRead`/`CdReadSync` that reads sectors from a `.bin` image file

PSY-Z's `libcd.c` has a MORE complete CUE/BIN parser (parses multi-track CUE sheets), full CD command dispatch (`CD_cw`), and XA audio decoding, but `CdSearchFile` itself is `NOT_IMPLEMENTED`. Its low-level `CD_cw` command handler is well-structured though.

**Strategy:**
1. Take PsyCross's `iso9660.h` struct definitions and `CdSearchFile` implementation
2. Take PSY-Z's CUE sheet parser for multi-track support
3. Wire them into our `runtime.c` intercepts for the game's `CdSearchFile`/`CdRead` calls
4. Point them at our Paranoiascape `.bin/.cue` image

### 3. XA Audio Decoding — USE PSY-Z
**Verdict: Extract PSY-Z's XA ADPCM decoder for FMV audio.**

PSY-Z has a complete XA ADPCM decoder (lines 475-570 of `libcd.c`) with:
- 4-bit stereo and mono decoding
- Proper filter coefficients (`xa_pos[4]`, `xa_neg[4]`)
- Hermite interpolation for 37800→44100 Hz resampling
- Streaming pull model (`xa_pull_samples`)

PsyCross's XA support is incomplete (marked as `TODO` in README).

This is directly useful for our FMV audio when we get STR playback working properly.

### 4. SPU (Sound Processing) — USE PSY-Z (LATER)
**Verdict: Defer. Neither is a drop-in for our recompiler architecture.**

Both libraries implement SPU at the API level. PSY-Z has `libspu.c` + `psyz_spu.c`. PsyCross has `LIBSPU.C` with OpenAL backend. Neither operates at the hardware register level (`0x1F801C00`) that our recompiler needs.

When we tackle audio, we'll need to intercept the game's SPU register writes and route them to one of these backends.

### 5. GPU (Rendering) — KEEP OURS
**Verdict: Keep our `opengl_renderer.cpp` and `gpu_interpreter.cpp`.**

Both PSY-Z and PsyCross implement GPU at the PsyQ API level (`DrawOTag`, `ClearImage`, etc.). Our renderer works at the GP0 command level — intercepting raw GPU commands from DMA. This is the correct architecture for a recompiler.

PsyCross's renderer (`PsyX_render.cpp`, 49KB) is interesting as a reference for OpenGL polygon rendering techniques, but it's designed around the linked-list primitive model, not raw GP0 command interpretation.

### 6. Math Tables — USE PSY-Z DECOMP
**Verdict: Use `decomp/src/libgte/sincos.c` and `ratan.c` as ground truth.**

PSY-Z's decomp directory contains exact matching decompilations of the original PsyQ math tables. These are the **actual** sin/cos/atan tables used by the hardware, byte-for-byte identical. If our `rsin`/`rcos` implementations have precision issues, these are the authoritative reference.

PsyCross also has these tables in `rcossin_tbl.h` (67KB) and `ratan_tbl.h` (9KB).

### 7. PsyQ Library Function Decompilations — USE PSY-Z DECOMP
**Verdict: Use as Ghidra cross-reference.**

PSY-Z's `decomp/` contains decompiled PsyQ source that matches the original binaries. When we encounter a PsyQ function in Ghidra and can't figure out what it does, we can look it up here. This is equivalent to having the PsyQ source code.

Key files in `decomp/src/`:
- `libgte/` — All GTE wrapper functions (380+ files)
- `libcd/` — CD library internals  
- `libgpu/` — GPU library internals
- `libetc/` — VSync, PadRead, etc.

## Summary Table

| Subsystem | Our Current | PSY-Z | PsyCross | Recommendation |
|-----------|------------|-------|----------|----------------|
| GTE (COP2 instructions) | ✅ Complete | ❌ Wrong level | ❌ Wrong level | **Keep ours** |
| GTE division table | Simplified | Simplified | ✅ HW-accurate | Cross-ref PsyCross |
| CD filesystem (ISO 9660) | ❌ Game-specific hacks | CUE parser only | ✅ Full ISO parser | **Use PsyCross** |
| CD command dispatch | ❌ Bypass hacks | ✅ Full `CD_cw` | Partial | **Use PSY-Z** |
| XA ADPCM audio | ❌ Not implemented | ✅ Complete | ❌ TODO | **Use PSY-Z** |
| SPU sound | ❌ Not implemented | Partial | Partial (OpenAL) | Defer |
| GPU rendering | ✅ Custom FBO | ❌ Wrong level | ❌ Wrong level | **Keep ours** |
| Math tables (sin/cos) | Custom | ✅ Exact decomp | ✅ Tables included | Cross-ref both |
| PsyQ function reference | Ghidra only | ✅ 380+ decomps | None | **Use PSY-Z decomp** |

## Immediate Action Items

1. **CD Loading (Priority 1):** Extract PsyCross's `iso9660.h` + `CdSearchFile` and wire it to read our Paranoiascape `.bin` file. This replaces the `0x80091FB4` file table hack in `runtime.c` and eliminates the Rule 0g sync bypass violations.

2. **GTE Division (Priority 2):** If we see geometry artifacts during Stage 1, adopt PsyCross's hardware-accurate division lookup table to replace our simplified formula.

3. **XA Audio (Priority 3):** When FMV audio is needed, drop in PSY-Z's XA decoder.

4. **Reference (Ongoing):** Use PSY-Z's `decomp/` as a second source alongside Ghidra when reverse-engineering PsyQ functions.
