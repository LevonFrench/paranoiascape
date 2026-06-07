# Paranoiascape Knowledge Wiki

Welcome to the Paranoiascape reverse engineering and recompilation knowledge base.

## Core Documentation

*   [Project Handoff (May 2026)](../HANDOFF.md)
    *   High-level overview of current project state, recent technical milestones, and immediate next steps for incoming developers.
*   [Boot Sequence & Load Addresses](./paranoiascape-boot-sequence.md)
    *   Details the `0x8000F000` load address fix and the BSS clearing loop recursion bug.
*   [Input Injection & 24-bit Graphics Analysis](./paranoiascape-input-graphics.md)
    *   Explains the SIO polling buffer at `0x8013E5EC` and how to use `pcsx_cmd.py` to bypass menu gates.
    *   Root cause analysis of the squashed Title Screen graphics (`color_depth_24bit` missing fragment shader support).
*   [IRQ Chain Dispatch & CD-Sync Boot Block](./paranoiascape-irq-chain-and-cd-spin.md)
    *   2026-05-07 boot regression: stalls in `func_80052800` CD-sync spin. Documents the `psx_dispatch_int_chain` PsyQ-style dispatcher and CD-sync bypass strategy.
*   [Stage 1 Loading & Rendering Pipeline](./stage1-loading-and-rendering.md)
    *   2026-05-08: LoadTask indexing bug fix (`a0 + a1*32`), complete Stage 1 file manifest, CD-sync bypass table, and the current rendering stall analysis (gameplay loop runs but no GPU draw primitives emitted).
*   [Stage 1 Rendering Stall details](../HANDOFF.md)
    *   2026-05-10: **CORRECTED.** Game never enters Stage 1. Overlay at `0x80134xxx` drives entire game loop but never transitions past menu state. Zero Stage 1 files loaded, zero rendering primitives emitted. Root cause is input/state-machine gap in overlay — NOT a CD sync violation. Next step: emulator ground truth to find correct button sequence.
*   [State Machine & Input Update](./paranoiascape-state-machine-update.md)
    *   2026-05-10: Discovered that `0x80008580` is not the main state variable. Removed the false `runtime.c` injection hack. The engine naturally stays at `0x00000000`. We must find the correct SIO pad sequence to natively trigger the Stage 1 transition.
*   [Input Automation & SIO Emulation](./input-automation-sio.md)
    *   2026-05-10: Explains the active-low requirement for pad edge triggers (`pressed` = `0xFFF7`), the SIO `0x1F801040` polling timeout bypass, and the correct Japanese region button sequence to bypass the Main Menu overlay.
*   [FMV Boot Bypass & Title Screen](./fmv-bypass-title-screen.md)
    *   2026-05-10: **BREAKTHROUGH.** Two bugs fixed: (1) SIO `write_half` missing state machine logic caused infinite polling loop; (2) `func_8001E468` FMV streaming stalled forever on zero CD data. Skipping FMV entirely lets the game boot to the 3D spinning eye Title Screen with loaded textures.
*   [Handoff: White Area Bug](./handoff.md)
    *   2026-05-11: **ROOT CAUSE IDENTIFIED.** Two bugs: (1) FMV decoder outputs all-black (MDEC decode zeroed), (2) FMV BG quads sample sprite index data instead of decoded FMV. VRAM textures and CLUTs confirmed valid via diagnostic probes. TP=10/12 filter removed. 121 texture uploads verified during boot.
*   [Red/Green Stripe Artifact Fix](../knowledge/paranoiascape-stripe-artifact-fix/artifacts/stripe-fix.md)
    *   2026-05-13: **SUPERSEDED.** The 4-bit filter was a workaround. True root cause: 8-bit FMV quads sampling stale VRAM because FMV uploaded to wrong Y-coordinate, overwriting hand textures. Game speed fix (`glfwSwapInterval(1)`) and continuous `osm=15` forcing remain valid.
*   [FMV 8-Bit Rendering Pipeline](../knowledge/paranoiascape-fmv-8bit-rendering/artifacts/fmv-rendering-analysis.md)
    *   2026-05-14: **MDEC DECODER BROKEN.** All 4 STR files have real content: LOGO.STR (developer logo), MOVTEST.STR (intro cinematic), SMG.STR (Screaming Mad George behind-the-scenes documentary), PARAFNMV.STR (in-game FMV). Decoder outputs black for valid video. FMV VRAM target fixed to (640,0).
*   [FBO Rendering Pipeline Bug](../knowledge/paranoiascape-fbo-rendering-bug/artifacts/fbo-rendering-bug.md)
    *   2026-05-14: **FIXED.** Textured primitives were failing due to FBO clear timings. Fixed by implementing `display_snapshot_` texture to capture FBO state post-draw.
*   [Rendering & Performance Stabilization](../knowledge/paranoiascape-rendering-stabilization/artifacts/rendering-stabilization.md)
    *   2026-05-15: **STABILIZED.** Fixed FBO double-buffering flickering by snapshotting only the `display_area` when overlapping. Replaced massive CPU bottleneck in `FillRect` (2M+ calls/frame) with `memset` optimization. Restored `fmv_vram_upload` to satisfy game's internal MDEC state machine, eliminating the 13GB `non supported code` log spam. Removed the `osm=15` title screen forcing hack to allow natural progression to Stage 1.
*   [PSY-Z vs PsyCross Analysis](./psyz-psycross-analysis.md)
    *   2026-05-15: Deep analysis of both porting libraries against our recompiler architecture. **Key findings:** Our GTE and GPU are at the right abstraction level — keep them. PsyCross's ISO 9660 parser can replace our hacky CD file table intercepts. PSY-Z's XA ADPCM decoder and PsyQ decompilations are the main value adds.
*   [MDEC Decoder & FMV Pipeline Fix](../knowledge/paranoiascape-async-fmv-title-screen/artifacts/vram-wrapping-failure.md)
    *   2026-05-30: Three bugs fixed/identified: (1) FBO reallocation bug (glTexImage2D→glTexSubImage2D) **FIXED**. (2) Wrong 24-bit flag for MOVTEST.STR removed — game uses 8-bit CLUT compositing, expects 15-bit RGB555 **FIXED**. (3) MDEC DMA timing mismatch — game sprite uploads overwrite FMV at x≥640 before rendering. Deferred upload (`fmv_refresh_vram`) added to FlushPrimitives **WORKING**.
*   [Title Screen Snapshot Timing Bug](./title-screen-snapshot-bug.md)
    *   2026-05-30: **PRIMARY BLOCKER.** After FMV ends, title screen renders BLACK despite valid content in VRAM FBO. Root cause: `display_snapshot_` capture in FlushPrimitives is misaligned with the game's multi-FillRect rendering pattern. FillRect clears the FBO after snapshot capture, and subsequent FillRects don't trigger new snapshots. VRAM dumps confirm 8-bit CLUT quads + 3D geometry ARE rendered. CLUT quantizer (15-bit→8-bit) implemented but disabled pending this fix.

## PSXRecomp General Rules (from AGENTS.md)
1. **Ghidra is Ground Truth**: Always verify disassembly before guessing behavior.
2. **Never Edit Generated Code**: Do not hand-edit `generated/` files. Use `psx_override_dispatch` or recompiler configs.
3. **Reproduce Before Fixing**: Capture screens and logs of the exact problem before writing code.
4. **One Fix, One Variable**: Only change one thing at a time.
5. **Know the VBlank Callback Chain**: Missing VBlank callbacks are the #1 cause of stalls.
6. **Hardware Sync**: Do not bypass `CdSync`/`CdReady` with instant returns.
