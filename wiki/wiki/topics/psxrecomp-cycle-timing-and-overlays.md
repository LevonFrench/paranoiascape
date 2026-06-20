---
title: "PSXRecomp Cycle Timing and Overlays"
category: topic
sources:
  - raw\notes\psxrecomp-docs-ingestion.md
created: 2026-06-19
updated: 2026-06-19
tags: [psx, recomp, timing, overlays, design]
confidence: high
summary: "Details of psxrecomp's cycle timing architecture (peripheral scheduler) and candidate-list based overlay validation design."
volatility: warm
---

# PSXRecomp Cycle Timing and Overlays

`psxrecomp` resolves timing drifts and self-modifying code issues in recompiled execution using a central hardware clock scheduler and granular function-entry validation tables.

## 1. Cycle-Paced Timing Architecture
Original PS1 hardware processes peripheral status clocks (SIO, CD-ROM, GPU) in parallel with CPU clock ticks. Lacking a central clock, the recompiled environment will drift during long spin-loops or CPU intensive events (such as memory card reads).

- **Monotonic Clock (`psx_cycle_count`)**: Increments per CPU operation block.
- **Peripheral Scheduler**: Single entry point `psx_advance_cycles(int cycles)` coordinates peripheral clocks:
  - SIO byte shifting (1088 cycles per byte).
  - VBlank deadline checks (NTSC frame cycles).
  - CD-ROM sector read and DMA ticks.
- **Basic-Block Timing Loops**: The recompiler pre-computes instructions count for each basic block and emits `psx_advance_cycles(<cycles>)` at block boundaries.
- **MMIO Read Splits**: To maintain register state accuracy, blocks are split at every MMIO read, ensuring that cycles are advanced immediately before status registers are evaluated.

## 2. Granular Overlay Validation & Candidate lists
Because different game overlays occupy the same virtual RAM ranges, the dynamic overlay loader must validate entries without invalidating adjacent code.

- **Candidate Lookup Table**: Maps virtual addresses to a list of potential native candidates:
  ```text
  PC -> [candidate native entries]
  ```
- **Code-Only Hashing**: Validation is per-entry by hashing the candidate's reachable code bytes against active RAM. Ordinary read/write data blocks are ignored during hashing to prevent false invalidations.
- **Page Generation Counters**: Rather than computing hashes on every write operation, writes increment a page generation index. If the candidate's target pages match the current generation index during execution, native dispatch proceeds without hashing overhead.
- **Active-Entry Blacklisting**: If a native function writes to its own code range, the entry is blacklisted from native execution, and falls back to the MIPS interpreter.

## See Also
- [[psxrecomp-framework-architecture|PSXRecomp Framework Architecture]] ([PSXRecomp Framework Architecture](../topics/psxrecomp-framework-architecture.md))
- [[psxrecomp-debugging-discipline|PSXRecomp Debugging Discipline]] ([PSXRecomp Debugging Discipline](../concepts/psxrecomp-debugging-discipline.md))
