---
title: "PsyCross SPU and GTE"
category: concept
sources:
  - raw\notes\psycross-overview.md
created: 2026-06-19
updated: 2026-06-19
tags: [psx, recomp, sdk, gte, spu, openal]
confidence: high
summary: "PsyCross software GTE and PGXP-Z vertex caching, OpenGL VRAM emulation, and OpenAL ADPCM SPU-AL audio mixer."
volatility: warm
---

# PsyCross SPU and GTE

PsyCross (Psy-X) provides high-level replacements for the PlayStation 1 math and audio hardware, facilitating native cross-platform compilation of games that rely on standard Psy-Q SDK interfaces.

## 1. GTE and PGXP-Z Software Emulation
PsyCross translates MIPS Geometry Transformation Engine (GTE) operations and vector registers in software (`src/gte`):
- **Vertex Caching**: Re-implements vector transformations and math macros inside `libgte`.
- **PGXP-Z Support**: Implements high-precision GTE vertex caching with modern 3D perspective transforms. It hooks into GTE projection math to support Z-buffering (depth buffer checks), eliminating the visual polygon-jitter and texture-warping inherent to original PS1 hardware rendering.
- **VRAM & Shader Emulation**: Feeds calculated projection quads into an OpenGL renderer (`src/render`), mirroring PS1 drawing behaviors on modern GPUs.

## 2. Audio Processing (SPU-AL)
PS1 games process ADPCM compressed sound streams through SPU register writes:
- **Audio Mixer**: Re-implements the original `libspu` functionality using OpenAL (`src/psx/libspu.c`).
- **SPU-AL Pipeline**: Decodes ADPCM samples at runtime and streams them directly through OpenAL-soft buffers.
- **Limitations**: The framework lacks native SPU Attack-Decay-Sustain-Release (ADSR) envelope simulation, which can cause instrument release phases to cut off prematurely in some games.

## See Also
- [[recompilation-alternatives-csharp-psycross|Recompilation Alternatives: C# and PsyCross]] ([Recompilation Alternatives: C# and PsyCross](../topics/recompilation-alternatives-csharp-psycross.md))
- [[psyz-porting-primitives|PSY-Z Porting Primitives]] ([PSY-Z Porting Primitives](../concepts/psyz-porting-primitives.md))
