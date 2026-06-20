---
title: "PSY-Z SDK Integration"
category: topic
sources:
  - raw\notes\psyz-porting-guide-raw.md
created: 2026-06-19
updated: 2026-06-19
tags: [psx, recomp, sdk, integration, nugget]
confidence: high
summary: "PSY-Z library integration patterns, build system targets, POSIX conflicts abstraction, and MSVC platform differences."
volatility: warm
---

# PSY-Z SDK Integration

PSY-Z is a library designed as a source-level replacement for the PSY-Q SDK. It enables PS1 applications to compile directly for modern host systems (Windows/Linux) using native toolchains rather than recompiling MIPS binaries.

## 1. Abstraction Architecture
Unlike a static recompiler, PSY-Z is a native C porting layer:
- **Core Library (`src/psyz`)**: Re-implements the standard PSY-Q library API. Platform-neutral calls redirect to internal platform wrappers (e.g. `LoadImage` calls `MyLoadImage`).
- **Platform Layer (`src/platform`)**: Contains platform-specific implementations. Statically links the required files based on target settings.
- **PSY-Q Decomp (`decomp/src`)**: Matches original PSY-Q 4.0 object files exactly through decompilation.

## 2. POSIX and Unix-Name Conflicts
The original PSY-Q SDK shares naming conventions with standard POSIX headers for file and card I/O operations (e.g. `open`, `close`, `read`, `write`, `O_RDONLY`, `O_CREAT`). However, flag values (such as `O_CREAT`) and path formats (`bu00:`) differ significantly.

**PSY-Z Abstraction Layer**:
- **Avoid POSIX headers**: Do not include `<fcntl.h>` in application files to prevent conflicting macro definitions.
- **Alternative Flags**: Use custom ROMIO definitions like `FWRITE`, `FCREAT`, and `FREAD` from `<romio.h>` instead of standard POSIX flags.
- **Auto-Wrapper**: Function calls like `open` are preprocessor-mapped to `psyz_open` to translate card and path strings before hitting host platform syscalls.

## 3. Build Integration
PSY-Z targets compile platforms using CMake and Make-based systems:
- **Nugget Integration**: The nugget framework can download and unpack PSY-Q SDK components and compile the `psyq-obj-parser` utility, helping with cross-compilation configurations.
- **CMake Settings**: Add `psyz` as a subdirectory, compile with `-Ipsyz/include`, and define `__psyz`.

## 4. Platform Quirks
- **MSVC Limitations**: MSVC does not support GNU C extensions such as zero-length arrays. Anonymous arguments in function declarations must have parameter names.
- **MSVC Pointer Sizes**: While standard MSVC maintains `long` as 32-bit on 64-bit systems, PSY-Z uses a custom `u_long` that scales to 64-bit to prevent truncation. If MSVC build issues persist, use the `clang-cl` compiler option.

## See Also
- [[psyz-porting-primitives|PSY-Z Porting Primitives]] ([PSY-Z Porting Primitives](../concepts/psyz-porting-primitives.md))
- [[psyz-pointer-and-symbol-safety|PSY-Z Pointer and Symbol Safety]] ([PSY-Z Pointer and Symbol Safety](../concepts/psyz-pointer-and-symbol-safety.md))
