---
title: "PSXRecomp Framework Architecture"
category: topic
sources:
  - raw\notes\psxrecomp-principles.md
  - raw\notes\psxrecomp-debug-client.md
created: 2026-06-19
updated: 2026-06-19
tags: [psx, recomp, architecture, overlays, kernel]
confidence: high
summary: "PSXRecomp recompiler paths (BIOS slice translation vs game EXE generation), runtime environment architecture, and dynamic overlay caching."
volatility: warm
---

# PSXRecomp Framework Architecture

PSXRecomp operates by statically translating PlayStation 1 MIPS instruction sets into native C source code, which is then compiled into a native PC application linked against a custom PS1 hardware runtime.

## 1. Separate Translation Pipelines
PSXRecomp implements two distinct, non-overlapping compilers:
- **`psxrecomp-bios.exe` (BIOS Recompiler)**:
  - Entry point: `recompiler/src/main_bios.cpp`
  - Core logic: Uses `strict_translator.cpp` and `bios_slice_walker.cpp`
  - Input: Flat BIOS ROM (e.g. `SCPH1001.BIN` at address `0xBFC00000`)
  - Output: Recompiled C BIOS code (`generated/SCPH1001_*.c`)
- **`psxrecomp-game.exe` (Game Recompiler)**:
  - Entry point: `recompiler/src/main_psx.cpp`
  - Core logic: Uses `code_generator.cpp` and `control_flow.cpp`
  - Input: PS-X EXE binary header and code blocks
  - Output: Recompiled game source code (`<game>_full.c` and `<game>_dispatch.c`)

Because the BIOS and Game paths are completely isolated, adjustments made to one recompiler's code generation do not affect the other.

## 2. Emulation Runtime and Target Mapping
The recompiled PC executable targets the `psx-runtime` framework:
- **CMake Target Mapping**: Game-specific builds link both the recompiled game code and the recompiled BIOS code against the core runtime sources (`runtime/`).
- **Optimization Levels**: PC executables can vary between `Debug` (compiled with `-g` and no `-O` optimizations) and `Release` (`-O3 -DNDEBUG`). Due to compiler optimization behaviors with recompiled MIPS code, `Release` mode can introduce undefined behavior; thus, `Debug` builds are the primary reference.

## 3. Overlay Loading and Dynamic Caching
Many PS1 games load code (overlays) dynamically from the CD-ROM disc into RAM at runtime. Because these overlays are not present in the main executable binary, they cannot be translated ahead-of-time (AOT).
- **Dynamic Capture**: When the recompiled game calls CD load procedures, the runtime intercepts the overlay load.
- **Dynamic Codegen**: The runtime recompiles the loaded RAM block on the fly into native dynamic library DLLs.
- **Caching**: The generated DLLs are cached locally on the PC. Subsequent loads check the cache first to avoid re-compilation delays.

## See Also
- [[psxrecomp-debugging-discipline|PSXRecomp Debugging Discipline]] ([PSXRecomp Debugging Discipline](../concepts/psxrecomp-debugging-discipline.md))
- [[psxrecomp-tcp-telemetry|PSXRecomp TCP Telemetry]] ([PSXRecomp TCP Telemetry](../concepts/psxrecomp-tcp-telemetry.md))
