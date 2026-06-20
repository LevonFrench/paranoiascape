---
title: "PSY-Z Pointer and Symbol Safety"
category: concept
sources:
  - raw\notes\psyz-porting-guide-raw.md
created: 2026-06-19
updated: 2026-06-19
tags: [psx, recomp, sdk, alignment, overlaps]
confidence: high
summary: "Data type choices for pointer storage (32-bit truncation vs 64-bit safe pointers) and handling memory symbol overlaps."
volatility: warm
---

# PSY-Z Pointer and Symbol Safety

When compiling legacy PlayStation 1 game code for modern architectures, pointer size mismatch and memory allocation layout differences represent severe risks.

## 1. 64-Bit Pointer Truncation
In the PSY-Q SDK, `u_long` and pointers are both 32-bit, which led developers to frequently store address pointers in `u32` or `s32` types:
```c
u32 myAddress = (u32)&someBuffer; // Safe on PS1
```
On 64-bit systems, compiling this logic truncates the 64-bit address pointer to a 32-bit integer. When dereferenced, this truncated address results in access violations and immediate program crashes.

**PSY-Z Type Rules**:
- Never store pointers in `s32` or `u32` variables.
- Use explicit pointer types (`s32*`) or variables typed as `u_long` (which PSY-Z configures to match native platform pointer size).
- If `u_long` was originally used in the game code to store raw 32-bit values (not addresses), change its declaration to `unsigned int` or `u32` to avoid wasting memory and misaligning structures on 64-bit builds.

## 2. Symbol Memory Overlaps
PS1 compiler toolchains allowed developers to declare variables that overlap in physical memory:
```c
// Problematic compilation overlap (common in original PS1 builds)
s32 D_800A1230[2]; // Allocates 8 bytes (0x800A1230 to 0x800A1237)
s32 D_800A1234;    // Overlaps at 0x800A1234
```
While this compiles on the PS1, modern compilers assign independent, non-overlapping memory blocks to these variables. As a result, writing to `D_800A1234` will no longer modify the index element `D_800A1230[1]`, resulting in broken game state machines.

**Correction Rules**:
- Eliminate overlapping symbol definitions.
- Access the array elements directly (e.g. `D_800A1230[1]`) rather than creating a separate overlapping symbol address pointer.

## See Also
- [[psyz-porting-primitives|PSY-Z Porting Primitives]] ([PSY-Z Porting Primitives](../concepts/psyz-porting-primitives.md))
- [[psyz-sdk-integration|PSY-Z SDK Integration]] ([PSY-Z SDK Integration](../topics/psyz-sdk-integration.md))
