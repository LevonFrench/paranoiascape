---
title: "PCSX-Redux Web Control CLI"
source: "j:/projects/paranoia/pcsx-redux/pcsx_cmd.py"
type: notes
ingested: 2026-06-19
tags: [psx, recomp, emulator, pcsx-redux, tcp, cli]
summary: "Interfacing with the emulator web API via HTTP requests on port 8079, CLI commands list, and pad button active-low decoding logic."
---

# PCSX-Redux API Control
PCSX-Redux runs an HTTP REST server at `http://localhost:8079` to allow remote control of memory and execution flow.

## CLI Scripts (`pcsx_cmd.py`)
A helper Python script targets these endpoints to query or update emulator state:
- **`status`**: Queries execution flow info (`/api/v1/execution-flow`)
- **`read <addr> [size]`**: Fetches raw RAM contents (`/api/v1/cpu/ram/raw`) and renders as pretty-printed hex dumps.
- **`write <addr> <hex_bytes>`**: Posts hex-string data to RAM via `/api/v1/cpu/ram/raw?offset=<phys_addr>&size=<len>`.
- **`scan <addr> <size> <hex>`**: Performs a substring scan of byte patterns inside RAM.
- **`watch <addr>`**: Periodically reads a memory offset and logs values on change.
- **`pause` / `resume`**: Pauses and resumes CPU emulation.
- **`vram [file]`**: Dumps 1024x512 16bpp raw VRAM buffer from `/api/v1/gpu/vram/raw`.
- **`diff <addr> <size>`**: Compares memory region with a cached snapshot on-disk (`.ram_snapshot.bin`).

## Controller Inputs
- Address: Pad buffer starts at `0x800BBDEC` with a 12-byte stride for 24 entries.
- Logic: Buttons mask is active-low (cleared bit = pressed button).
- Bits:
  - 0: Select, 3: Start, 4: Up, 5: Right, 6: Down, 7: Left, 8: L2, 9: R2, 10: L1, 11: R1, 12: Triangle, 13: Circle, 14: Cross, 15: Square.
