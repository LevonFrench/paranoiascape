---
title: "PsyCross (Psy-X) SDK Compatibility Layer"
source: "j:/projects/paranoia/PsyCross/README.md"
type: notes
ingested: 2026-06-19
tags: [psx, recomp, sdk, library, gte, spu]
summary: "High-level API compatibility layer for PSY-Q SDK on PC, utilizing software GTE (PGXP-Z) and OpenAL sound mixer."
---

# Psy-Cross (Psy-X) Overview
Psy-Cross is a compatibility SDK for building and executing Psy-Q PlayStation games natively across other platforms (such as PC and Web via Emscripten).

## Implementation Details
- **High-Level API**: Translates standard PlayStation API calls into modern, cross-platform APIs.
- **Header Files**: Provides replacement headers compatible with the Psy-Q SDK.
- **GTE Emulation**: Implements GTE in software. Supports PGXP-Z (precise GTE vertex caching with modern 3D perspective transforms and Z-buffering).
- **SPU Sound**: Re-implements `libspu` via OpenAL (SPU-AL) to perform ADPCM decoding.
- **GPU Drawing**: Emulates PS1 VRAM and handles PS1 polygon/drawing lists in OpenGL.
- **CD-ROM**: Supports ISO 9660 BIN/CUE images through standard Playback APIs (`libcd`).

## Directory Layout
- `src/gpu`: Polygon and drawing lists processing.
- `src/gte`: Software GTE and PGXP-Z implementation.
- `src/render`: OpenGL rendering and VRAM buffers.
- `src/psx`: Core replacements for PsyQ APIs (libgte, libgpu, libspu, libcd).
- `include/psx`: Headers for replacement libraries.
- `include/PsyX`: Window, configuration, and rendering interfaces.
