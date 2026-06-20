---
title: "PSXRecomp Advanced Timing and Overlay Designs"
source: "j:/projects/paranoia/psxrecomp/docs/CYCLE_TIMING_ARCH.md and overlay-recompilation-design.md"
type: notes
ingested: 2026-06-19
tags: [psx, recomp, architecture, timing, overlays]
summary: "Underlying timing frameworks, peripheral clocks scheduler, candidate-list dynamic overlays caching, and call-contract returns."
---

# Cycle-Driven Timing Architecture
- **Problem**: The recompiled runtime historically lacked a concept of PS1 wall-clock cycle count, resulting in timing drifts and SIO polling register lockups.
- **Solution**: Introducing `psx_cycle_count` in `psx_cycles.c/.h` advanced by `psx_advance_cycles(int cycles)`.
- **Peripheral scheduler**: Single clock advances SIO byte-shifting (~1088 cycles/byte), GPU NTSC deadlines (~564,480 cycles/frame), SIO interrupt ACK timing (~170 cycles), CD-ROM timers, and root counters.
- **Recompiler block timing**: Every emitted basic block calculates its MIPS instruction cycles and emits `psx_advance_cycles(<cycles>)`. For accuracy, blocks are split at every MMIO read to advance cycles before reading SIO status registers.

# Dynamic Overlay Cache Design
- **Problem**: RAM-loaded overlays cannot be statically recompiled ahead-of-time.
- **Failover Interpreter**: Any code loaded into RAM pages can fall back to the slow MIPS dirty-RAM interpreter (`dirty_ram_interp.c`).
- **Dynamic Caching**: Captured execution bytes are compiled offline to PC DLLs, keyed by content hash. The runtime loads these DLLs at startup and maps the callable entries.
- **Candidate-List Table**: PC addresses map to candidate native functions (`PC -> [native_entries]`) to resolve cases where different overlays reuse the same virtual address ranges (e.g. Village and Overworld).
- **Dependency Set**: Validity is tracked per-function by hashing the function's code bytes against live RAM (gated by fast page generation counters). When RAM is overwritten, the corresponding page code is invalidated.
- **Call-Contract Returns**: Intercepting JAL/JALR calls from interpreted code requires enforcing a call contract (validate return address and caller stack pointer). A mismatch bails/unwinds to native execution loop to prevent stack leaks.
