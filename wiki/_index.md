# Paranoiascape Knowledge Base Index

Welcome to the recompiled and reverse engineered Paranoiascape knowledge base. This is the master index for all core documentation and analysis.

Last updated: 2026-06-19

## Quick Navigation

- **Compiled Articles**: [Wiki Articles Index](./wiki/_index.md)
- **Concepts**: [Concepts Index](./wiki/concepts/_index.md)
- **Topics**: [Topics Index](./wiki/topics/_index.md)
- **References**: [References Index](./wiki/references/_index.md)
- **Raw Sources**: [Raw Index](./raw/_index.md)

---

## Contents

### References

*   [[memory-map|Memory Address Map]] ([Memory Address Map](./wiki/references/memory-map.md))
    *   Comprehensive reference of internal state variables, global variables, hardware MMIO registers, and recompiled MIPS function intercepts.
*   [[psyz-psycross-analysis|PSY-Z vs PsyCross Analysis]] ([PSY-Z vs PsyCross Analysis](./wiki/references/psyz-psycross-analysis.md))
    *   Deep analysis of the PSY-Z SDK samples and PsyCross library integration.

### Topics

*   [[paranoiascape-boot-sequence|Boot Sequence & Load Addresses]] ([Boot Sequence & Load Addresses](./wiki/topics/paranoiascape-boot-sequence.md))
    *   Details the load address hardcode bug (`0x8000F000`) and the BSS clearing loop recursion stack overflow fix.
*   [[paranoiascape-input-graphics|Input Injection & 24-bit Graphics Analysis]] ([Input Injection & 24-bit Graphics Analysis](./wiki/topics/paranoiascape-input-graphics.md))
    *   Explains the SIO polling buffer at `0x8013E5EC` and the root cause of the squashed Title Screen graphics.
*   [[stage1-loading-and-rendering|Stage 1 Loading & Rendering Pipeline]] ([Stage 1 Loading & Rendering Pipeline](./wiki/topics/stage1-loading-and-rendering.md))
    *   Details of the Stage 1 file loading manifest, LoadTask indexing bug fix, CD-sync bypass table, and initial rendering stall analysis.
*   [[psxrecomp-framework-architecture|PSXRecomp Framework Architecture]] ([PSXRecomp Framework Architecture](./wiki/topics/psxrecomp-framework-architecture.md))
    *   PSXRecomp recompiler paths (BIOS slice translation vs game EXE generation), runtime environment architecture, and dynamic overlay caching.
*   [[psyz-sdk-integration|PSY-Z SDK Integration]] ([PSY-Z SDK Integration](./wiki/topics/psyz-sdk-integration.md))
    *   Integration of PSY-Z source-level porting library, Nugget system, POSIX conflicts, and MSVC considerations.
*   [[recompilation-alternatives-csharp-psycross|Recompilation Alternatives: C# and PsyCross]] ([Recompilation Alternatives: C# and PsyCross](./wiki/topics/recompilation-alternatives-csharp-psycross.md))
    *   Comparative analysis of PS1 recompilation and porting paths, comparing PSXRecomp, RecompOne, and PsyCross SDK abstraction models.
*   [[psxrecomp-cycle-timing-and-overlays|PSXRecomp Cycle Timing and Overlays]] ([PSXRecomp Cycle Timing and Overlays](./wiki/topics/psxrecomp-cycle-timing-and-overlays.md))
    *   Details of psxrecomp's cycle timing architecture (peripheral scheduler) and candidate-list based overlay validation design.

### Concepts

*   [[input-automation-sio|Input Automation & SIO Emulation]] ([Input Automation & SIO Emulation](./wiki/concepts/input-automation-sio.md))
    *   Explains active-low requirements for pad edge triggers and SIO register polling timeout bypasses.
*   [[paranoiascape-irq-chain-and-cd-spin|IRQ Chain Dispatch & CD-Sync Boot Block]] ([IRQ Chain Dispatch & CD-Sync Boot Block](./wiki/concepts/paranoiascape-irq-chain-and-cd-spin.md))
    *   Analysis of the `psx_dispatch_int_chain` dispatcher and the CD-sync spin boot block stall mitigation.
*   [[paranoiascape-state-machine-update|State Machine & Input Update]] ([State Machine & Input Update](./wiki/concepts/paranoiascape-state-machine-update.md))
    *   Details the search for the native menu state machine variable `0x80008580` and bypassing menu overlays.
*   [[fmv-bypass-title-screen|FMV Boot Bypass & Title Screen]] ([FMV Boot Bypass & Title Screen](./wiki/concepts/fmv-bypass-title-screen.md))
    *   Bypassing FMV decoding loops to allow the game to boot to the spinning eye Title Screen.
*   [[white-area-bug|White Area Bug]] ([White Area Bug](./wiki/concepts/white-area-bug.md))
    *   Root cause analysis of incorrect texture mapping where 8-bit background quads sample incorrect sprite index data.
*   [[title-screen-snapshot-bug|Title Screen Snapshot Timing Bug]] ([Title Screen Snapshot Timing Bug](./wiki/concepts/title-screen-snapshot-bug.md))
    *   Root cause of black display output due to misaligned FBO snapshot captures with multi-FillRect sequences.
*   [[recompiler-stack-leak-and-vblank-pump-fix|Recompiler Stack Pointer Leak & VBlank Pump Deadlock Fix]] ([Recompiler Stack Pointer Leak & VBlank Pump Deadlock Fix](./wiki/concepts/recompiler-stack-leak-and-vblank-pump-fix.md))
    *   Resolving recompiler interpreter stack leaks and frame-pace busy-wait deadlocks.
*   [[stage4-navigation-and-geometry-fix|Stage 4 Menu Navigation & 3D Geometry Fixes]] ([Stage 4 Menu Navigation & 3D Geometry Fixes](./wiki/concepts/stage4-navigation-and-geometry-fix.md))
    *   Fixing rotation coordinate matrix collapse and GPU rectangle masking.
*   [[psxrecomp-debugging-discipline|PSXRecomp Debugging Discipline]] ([PSXRecomp Debugging Discipline](./wiki/concepts/psxrecomp-debugging-discipline.md))
    *   Debugging workflows, first divergence comparisons, and E2E verification loops.
*   [[psxrecomp-tcp-telemetry|PSXRecomp TCP Telemetry]] ([PSXRecomp TCP Telemetry](./wiki/concepts/psxrecomp-tcp-telemetry.md))
    *   Client-side live register compares, memory watchpoints/trace registers, and continuation bails telemetry.
*   [[psyz-porting-primitives|PSY-Z Porting Primitives]] ([PSY-Z Porting Primitives](./wiki/concepts/psyz-porting-primitives.md))
    *   Ordered Tables (`OT_TYPE*`) and custom structure tags (`O_TAG`) compatibility.
*   [[psyz-pointer-and-symbol-safety|PSY-Z Pointer and Symbol Safety]] ([PSY-Z Pointer and Symbol Safety](./wiki/concepts/psyz-pointer-and-symbol-safety.md))
    *   Pointer storage sizing safety, MSVC constraints, and resolving symbol overlaps.
*   [[pcsx-redux-remote-control|PCSX-Redux Remote Control]] ([PCSX-Redux Remote Control](./wiki/concepts/pcsx-redux-remote-control.md))
    *   Interfacing with the PCSX-Redux HTTP control server, executing memory and register diagnostics via Python CLI, and active-low pad button decoding.
*   [[psycross-spu-and-gte|PsyCross SPU and GTE]] ([PsyCross SPU and GTE](./wiki/concepts/psycross-spu-and-gte.md))
    *   PsyCross software GTE and PGXP-Z vertex caching, OpenGL VRAM emulation, and OpenAL ADPCM SPU-AL audio mixer.
*   [[recompone-overlay-dispatch|RecompOne Overlay Dispatch]] ([RecompOne Overlay Dispatch](./wiki/concepts/recompone-overlay-dispatch.md))
    *   RecompOne overlay class structure, virtual address dispatch tables, and runtime function overrides.

---

## Activity Log & History

Refer to [[log|Activity Log]] ([Activity Log](./log.md)) for the detailed log of wiki operations and updates.
