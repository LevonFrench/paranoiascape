---
title: "PSXRecomp TCP Telemetry"
category: concept
sources:
  - raw\notes\psxrecomp-debug-client.md
created: 2026-06-19
updated: 2026-06-19
tags: [psx, recomp, debugging, tcp, telemetry]
confidence: high
summary: "TCP Debug protocol specifications, command inventory, memory trace registers, continuation bails telemetry, and timing overhead."
volatility: warm
---

# PSXRecomp TCP Telemetry

The recompiled PC runtime (`psx-runtime.exe`) incorporates a TCP debug server on port `4370` to expose internal state dynamically. A patched emulator instance (e.g. Beetle or DuckStation on port `4371`) acts as the dynamic "oracle" to serve reference state.

## 1. Divergence Comparison CLI
Using `debug_client.py`, the developer can target either backend or run comparisons live:
```bash
# Compare register states
python tools/debug_client.py compare regs

# Compare RAM regions
python tools/debug_client.py compare read_ram addr=0x800DFEE0 len=64
```
If the tool lacks commands to retrieve specific data, add a handler directly in `runtime/src/debug_server.c` instead of using `printf` logging.

## 2. Advanced Trace Telemetry
To locate memory writers without freezing execution, the server implements always-on trace rings:
- **Write Tracing (`wtrace_range`)**: Specifies a RAM range to capture. Up to 1024 writes are held in a circular buffer along with the Return Address (`$ra`).
- **MMIO & GP1 Logs**: Tracks all hardware register writes (0x1F801xxx) and dedicated GP1 display command history (up to 512K entries) to debug timing issues or display mode changes.

## 3. Continuation Bails Telemetry
During dynamic call execution, a call-contract guard validates MIPS return addresses:
- **Call Contract**: A recompiled C continuation can only execute if the guest has actually returned to the call site ($ra == return address, $sp == caller's sp).
- **Continuation Bails**: If a wild return (e.g., jump table branches or chest freezes) violates the contract, a "bail" unwind is triggered, discarding stale C frames.
- **Bail Telemetry (in `psx_freeze_heartbeat.json`)**:
  - `bail_first`: Number of contract violations detected.
  - `bail_resolved`: Unwinds resolved at matching enclosing C stack frames.
  - `bail_flattened`: Unwinds that reached the outermost dispatch loop and were safely re-dispatched.

## 4. Telemetry Stalls
Because the TCP server runs on the main emulation loop thread, large transfer payloads (such as dump commands) introduce timing overhead. Serve-stalls are tracked in `tcp_send_stall_ms`. To avoid watchdog timeouts or visual stutter, large memory dumps should be redirected to files (`*_dump_file` commands) instead of sockets.

## See Also
- [[psxrecomp-debugging-discipline|PSXRecomp Debugging Discipline]] ([PSXRecomp Debugging Discipline](../concepts/psxrecomp-debugging-discipline.md))
- [[psxrecomp-framework-architecture|PSXRecomp Framework Architecture]] ([PSXRecomp Framework Architecture](../topics/psxrecomp-framework-architecture.md))
