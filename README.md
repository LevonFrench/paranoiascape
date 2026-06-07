# Paranoiascape PC Port

Native PC port of the PlayStation 1 game **Paranoia Scape (SLPS_013.75, Japan)** using static MIPS-to-C recompilation.

## Overview

This repository contains:
- **`paranoiascape-recomp/`**: The recompiler framework and C++ runner/porting layer wrapper.
- **`wiki/`**: In-depth reverse engineering notes, boot sequence guides, state machine descriptions, and hardware emulation details (SIO controller, VBlank interrupt loop, MDEC video decoders).
- **`CLAUDE.md`**: Build and run guides for the recompiler and runner compilation stages.
- **`AGENTS.md`**: Behavioral/debugging guidelines and rules for developing on this recompiler.

## Key Features

- **Static Recompilation**: MIPS R3000A instructions are pre-translated directly into C code, making the port run as a native x86-64 executable instead of using a standard emulator interpreter loop.
- **Hardware-Level SIO Emulation**: Custom serial input override directly interacting with the game's hardware registers.
- **VBlank Interrupt Pump**: Periodic interrupt pump simulating CPU register state-saves and handlers to resolve async loading stalls.
- **FFmpeg-based MDEC Decoding**: Seamless integration of 15bpp and 24bpp background/video decompression.

## Getting Started

Refer to [CLAUDE.md](file:///j:/projects/paranoia/CLAUDE.md) for full setup, build tools, compilation commands (Stage 1 recompiler and Stage 2 runner), and run configurations.
