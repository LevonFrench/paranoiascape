---
title: "PSXRecomp TCP Debug Client Specifications"
source: "j:/projects/paranoia/psxrecomp/TCP_COMMANDS.md"
type: notes
ingested: 2026-06-19
tags: [psx, recomp, debugging, tcp, commands]
summary: "Protocol definitions, command sets, registry peeking, telemetry details, and slow-frame diagnostics."
---

# TCP Debug Server Protocol
- Format: JSON over newline (one object per line)
- Request: `{"id": N, "cmd": "<command>", ...params}`
- Success: `{"id": N, "ok": true, ...data}`
- Failure: `{"id": N, "ok": false, "error": "<msg>"}`
- Ports:
  - Native: `4370` (`runtime/src/debug_server.c`)
  - DuckStation (oracle): `4371` (`duckstation/src/core/psxrecomp_debug_server.cpp`)

# CLI Client
- Ping native: `python tools/debug_client.py ping`
- Ping DuckStation: `python tools/debug_client.py --ds ping`
- Compare both (diff state live): `python tools/debug_client.py compare <cmd>`

# Command Inventory
- `regs`: Get 32 GPRs + HI + LO + PC (native includes COP0, I_STAT, I_MASK)
- `read_ram`: Read bytes as hex (up to 2MB in one line)
- `write_ram`: Write bytes to RAM
- `read_scratch`: Read scratchpad (0x1F800000)
- `read_vram` / `vram_peek`: Read 16-bit VRAM pixels (max 128x128)
- `gpu_state`: Display area, depth, draw offset, GPUSTAT, clip rect
- `sio_state`: SIO registers, pad protocol state, and history
- `irq_state`: I_STAT, I_MASK, and native interrupt chains
- `dma_state`: DPCR, DICR, and all 7 channels (madr/bcr/chcr)
- `watch` / `unwatch`: Memory watchpoints
- `set_input` / `clear_input`: Override active-low buttons
- `screenshot`: Capture display to BMP
- `wtrace_range` / `wtrace_dump`: RAM-write tracing (ring buffer of 1024 writes)
- `mmio_dump` / `gp1_dump`: MMIO and GP1 write rings (always-on write tracing)
- `pc_break`: Execute breakpoint in DuckStation

# Telemetry
- **Serve-Stall**: `tcp_send_stall_ms` delta indicates client observer interference (e.g. mega-dumps on slow connection block emulator thread).
- **Continuation Bails**: 
  - `bail_first` - contract violations (e.g., wild returns where $ra or $sp mismatch call site)
  - `bail_resolved` - unwinds resolved at enclosing call site
  - `bail_flattened` - unwinds that fall back to outer dispatch loop
  - `bail_anomaly` - bail flag at exception entry (should be 0)
