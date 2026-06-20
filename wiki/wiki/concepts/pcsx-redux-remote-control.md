---
title: "PCSX-Redux Remote Control"
category: concept
sources:
  - raw\notes\pcsx-redux-overview.md
created: 2026-06-19
updated: 2026-06-19
tags: [psx, recomp, emulator, pcsx-redux, tcp, remote]
confidence: high
summary: "Interfacing with the PCSX-Redux HTTP control server, executing memory and register diagnostics via Python CLI, and active-low pad button decoding."
volatility: warm
---

# PCSX-Redux Remote Control

PCSX-Redux hosts an HTTP REST web server at `http://localhost:8079` to expose register structures, memory access, and execution flow endpoints.

## 1. HTTP Endpoint Bindings
The REST server maps standard API requests:
- **RAM operations**: Read and write access via `/api/v1/cpu/ram/raw` (accepting query parameters like offset and size).
- **VRAM operations**: Dump the graphics frame buffers using `/api/v1/gpu/vram/raw`.
- **Execution control**: Pause and resume commands utilizing `/api/v1/execution-flow?function=pause/resume&type=shell`.

## 2. Python Diagnostic Scripting (`pcsx_cmd.py`)
A command-line interface communicates with the REST API to provide diagnostic tools:
- **Memory Dumping (`read`, `u32`, `u16`)**: Displays RAM addresses as aligned hex values with ASCII formatting.
- **Pattern Scanning (`scan`)**: Scans memory ranges for target byte strings.
- **Polling (`watch`)**: Periodically reads specific variables and reports changes.
- **Differential snapshots (`diff`)**: Serializes memory segments to a local binary file (`.ram_snapshot.bin`) and diffs changes on subsequent runs.
- **VRAM Dump (`vram`)**: Extracts the 1024x512 16bpp VRAM array to disc as a binary payload.

## 3. Active-Low Button Decoding (`pad`)
PS1 controller polling maps digital buttons as inverted active-low bits (where a `0` bit implies the button is active/pressed). The pad buffer starts at `0x800BBDEC` with a 12-byte stride. Buttons are parsed as a 16-bit mask:
- **Bit 0**: Select
- **Bit 3**: Start
- **Bit 4-7**: D-Pad (Up, Right, Down, Left)
- **Bit 8-11**: Shoulder Buttons (L2, R2, L1, R1)
- **Bit 12-15**: Action Buttons (Triangle, Circle, Cross, Square)

## See Also
- [[psxrecomp-tcp-telemetry|PSXRecomp TCP Telemetry]] ([PSXRecomp TCP Telemetry](../concepts/psxrecomp-tcp-telemetry.md))
- [[recompilation-alternatives-csharp-psycross|Recompilation Alternatives: C# and PsyCross]] ([Recompilation Alternatives: C# and PsyCross](../topics/recompilation-alternatives-csharp-psycross.md))
