---
title: "Stage1 Loading And Rendering"
category: topic
sources: []
compiled-from: conversation
created: 2026-05-08
updated: 2026-06-19
tags: [psx, recomp, load, rendering]
confidence: high
summary: "Details of the Stage 1 file loading manifest, LoadTask indexing bug fix, CD-sync bypass table, and initial rendering stall analysis."
volatility: warm
---

# Stage 1 Loading & Rendering Pipeline

Status as of 2026-05-08. Stage 1 assets load successfully into RAM and VRAM. The game loop runs but produces no GPU draw primitives.

## Loading Pipeline — RESOLVED

### Two CD Loading Code Paths

Paranoiascape uses two separate file loading mechanisms:

1. **`func_80019704` → `func_80019750` (CdSearchFile) → `func_800197C8` (LoadTask)**
   The game's primary loading path. Called at boot for `SOUND/JJ.VH`, `SOUND/JJ.VB`, `MBG/TITLE.BIN`, and during stage transitions for all stage assets.

2. **`func_8001E468` → `func_80081038` (CD poll loop)**
   Secondary loading path used during stage transitions. Reads a file table at `0x80091FB4 + gp[900]*12`, calls CdSearchFile, then enters a hardware polling loop waiting for `gp+1060 == 1`.

### LoadTask Index Bug (Fixed)

`func_800197C8` takes two arguments: base pointer in `a0` and index in `a1`. The task struct is at `a0 + a1*32`. Our intercept originally used `a0` directly (ignoring `a1`), which worked for index 0 files but failed for files at higher indices (like `STAGE1/ST1_1TIM.BIN` at index 17).

**Fix**: `task_ptr = cpu->a0 + (cpu->a1 * 32)` in `runtime.c` line ~3211.

### Stage 1 File Manifest

All files load successfully via the LoadTask intercept:

| File | LBA | Size | Dest | Purpose |
|------|-----|------|------|---------|
| `SOUND/JJ.VH` | 1339 | 23,584 | `0x801F2000` | Sound header |
| `SOUND/JJ.VB` | 1210 | 262,448 | `0x80170000` | Sound body |
| `STAGE1/ST1_1TIM.BIN` | 1890 | 294,832 | `0x80140000` | Stage textures |
| `OBJECT/CHR00TIM.BIN` | 6033 | 122,080 | `0x80140000` | Character textures |
| `STAGE1/CR1_1TIM.BIN` | 1596 | 12,432 | `0x80140000` | Stage textures (set 2) |
| `OBJECT/CHR00TMD.BIN` | 6093 | 127,764 | `0x801D0000` | Character 3D models |
| `STAGE1/ST1_1TMD.BIN` | 2034 | 388,276 | `0x80158000` | Stage 3D models |
| `STAGE1/ST1_1ANM.BIN` | 1876 | 27,868 | `0x80140000` | Stage animations |

### CD-Sync Bypass Functions

Five PsyQ CD-sync functions are bypassed (return `v0=2` = CdlComplete):

| Address | Name/Role |
|---------|-----------|
| `0x80052800` | CdReadyResult — primary sync spin |
| `0x80052A80` | CdReady variant |
| `0x80052D50` | CD_cw variant (added 2026-05-08 — was causing post-load stall) |
| `0x80052F1C` | CD_init sync |
| `0x80053618` | CD_init high-level |

All callers observed: `ra=0x80052E10` (boot CD-init) and `ra=0x80051EDC` / `ra=0x80052010` (stage loading).

## Rendering Pipeline — CURRENT BLOCKER

### Symptom

After all Stage 1 assets load (~frame 13300), the game enters `func_8004C9AC` (the Stage 1 gameplay loop, 1324 bytes). This loop:

1. Reads the state machine at `gp+1104`
2. Branches through a complex state tree (states 0-3, plus countdown timeouts)
3. Calls rendering functions: `func_8004CF58`, `func_8007DEB0`, `func_8007E6E0`, `func_8007E514`, `func_8007E4A4`
4. Calls `func_80050B04` (VSync) per frame
5. Jumps back to `block_8004CBEC`

But **zero GPU draw primitives are emitted** after frame ~894. The VRAM shows textures loaded correctly (sprite sheets, character art visible in right half), but the framebuffer region (x=0-319) stays black.

### Key observations

- `DrawArea TL` and `DrawArea BR` were set to `(1023, 0)` during the title screen — this is a degenerate scissor rect that clips ALL rendering.
- After the stage transition, no new `GP0:E3`/`GP0:E4` (DrawArea) commands are observed.
- The single `FillRect(0,0,320,480,black)` at frame 13087 is the transition screen clear.
- The game runs millions of frames with the degenerate scissor rect never being reset.

### Root cause hypothesis

The game's rendering system uses DMA to transfer the Ordering Table (OT) to the GPU. On real PS1, this happens via DMA Channel 2 (GPU). In PSXRecomp, DMA transfers are intercepted and processed synchronously. If the DMA-to-GPU pathway is not correctly wired for OT submission, the game builds OTs every frame but they never reach the GPU command processor.

Alternatively, the rendering functions inside `func_8004C9AC` may depend on state that wasn't properly initialized because the CD-sync bypasses return too quickly (before the game's init sequence completes).

### Investigation checklist

1. **Check DMA Channel 2 (GPU-OT)**: Verify `0x1F8010A0` / `0x1F8010A4` / `0x1F8010A8` writes are being intercepted
2. **Check `DrawArea` state**: The degenerate `(1023,0)` scissor from the title screen is never reset
3. **Check `func_8004CF58`**: This is called every frame from the gameplay loop — it should be submitting the OT
4. **Check `gp+852`**: This is the button state variable read by the state machine; may need proper SIO injection
5. **Verify OT DMA**: The game likely calls `DrawOTag` or similar to flush the ordering table — trace this

### Key addresses for this analysis

| Address | Meaning |
|---------|---------|
| `0x8004C9AC` | Stage 1 gameplay loop (main function) |
| `gp+1104` (`0x8009E150+1104`) | Stage state machine variable |
| `gp+1100` | Countdown timer (init=1800, decrements per frame) |
| `gp+852` | Button state (read for Start/Circle transitions) |
| `0x8004CF58` | Per-frame rendering function (called from gameplay loop) |
| `0x8004CED8` | Timeout handler (called when countdown reaches 0) |
| `0x80134990` | Display buffer state pointer (s0 in gameplay loop) |
| `0x801348C0` | OT/display list base (s1 in gameplay loop) |

### Recent Findings (2026-05-08)

1. **Format String Vulnerability in BIOS Printf**:
   - Fix: `runtime.c` printf overrides print `fmt='%s'` literally and dump raw `a1`-`a3` as hex.

2. **"Non supported code" = TMD Primitive Parser Failure**:
   - Originates from `func_8006ACF4` / `func_8006B58C` (TMD 3D model renderer).
   - Parser reads cmd byte at `s0+3`. Valid opcodes: `0x20`-`0x3D`. Byte `0x00` hits error handler.
   - `a3=0x1F800000` is the scratchpad base, not a hardware register issue.

3. **Root Cause: Recompiler Split-Function Register Corruption**:
   - `func_8006ACF4` (2200 bytes) split into two C functions by the recompiler.
   - Piece 2 (`func_8006B58C`, 2396 bytes) contains the primitive loop and epilogue.
   - Mid-function blocks `0x8006B924`, `0x8006B458`, `0x8006BEB8` NOT in dispatch table.
   - `call_by_address()` falls to MIPS interpreter which takes over permanently.
   - Interpreter corrupts `s2/s4/s6` in caller `func_80015FDC` (TMD object loop).
   - Result: TMD objects #2+ get `a0=0x8198xxxx` (texture data, not model data).

4. **Fix Applied (2026-05-09)**: Re-ran the recompiler (forced entries for `0x8006B924`, `0x8006B458`, `0x8006BEB8` were already in `main_psx.cpp` but the generated code was stale). All four split targets now in the dispatch table (1540 entries total). "Non supported code" errors eliminated. Awaiting visual verification of TMD rendering after title screen bypass.

