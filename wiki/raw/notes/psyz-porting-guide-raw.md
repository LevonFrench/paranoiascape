---
title: "PSY-Z SDK Porting Guide"
source: "j:/projects/paranoia/psyz/PORTING_GUIDE.md and psyz/README.md"
type: notes
ingested: 2026-06-19
tags: [psx, recomp, sdk, library, porting]
summary: "Drop-in library compatibility patterns, MSVC compiler quirks, pointer sizing, Nugget Makefiles, and Unix file conflicts."
---

# PSY-Z Core Purpose
PSY-Z is a porting library in C that acts as a drop-in replacement for the PSY-Q runtime. It compiles original PS1 code to run natively on PC (Windows/Linux) in 32-bit and 64-bit architectures, prioritizing API compatibility over hardware-accurate emulation.

# Key Code Modifications
- **Ordered Tables**: Original PSY-Q `u_long* ot` must be replaced with `OT_TYPE* ot` because memory structures behave differently on 64-bit systems.
- **Custom GPU Primitives**: Struct field `u_long tag` must be changed to `O_TAG tag`.
- **Pointer Storage & Data Types**: Use `u_long` (64-bit in PSY-Z for 64-bit platforms) or pointer types (`s32*`) to store addresses. Storing pointers in `u32` or `s32` causes truncation crashes on 64-bit builds.
- **Header Includes**: Include `#include <psyz.h>` first, followed by `<libgpu.h>`, `<libgte.h>`, etc.
- **Symbol Overlaps**: Avoid overlapping memory symbols (e.g., separate symbols `D_800A1230[2]` and `D_800A1234` overlap in memory on PS1 but break on PC porting libraries).
- **POSIX Name Conflicts**:
  - `open`, `close`, `read`, `write`, `lseek`, `O_RDONLY`, `O_CREAT` exist on PS1 SDK but have different flag values and path formats (e.g., `bu00:`) compared to POSIX standards.
  - PSY-Z provides wrappers (e.g. `psyz_open` internally map to `open`) and custom flags in `<romio.h>` (e.g. `FWRITE`, `FCREAT`). Avoid including `<fcntl.h>`.

# Platform Specifics
- **MSVC (Windows)**:
  - MSVC keeps `long` as 32-bit even on 64-bit Windows, but PSY-Z's `u_long` is still 64-bit on 64-bit builds.
  - MSVC does not support zero-length arrays or anonymous parameters (`void foo(int)` -> `void foo(int value)`).
  - Use `clang-cl` if MSVC struggles with GNU extensions.
- **CMake Integration**:
  - Link against `psyz` library, define target compile flags `-Ipsyz/include` and target definition `__psyz`.
