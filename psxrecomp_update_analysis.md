# Upstream PSXRecomp Update Analysis

We have analyzed the recent commits in the upstream `mstan/psxrecomp` repository (`psxrecomp`) and reviewed the new `flafmg/RecompOne` C# recompiler repository. Here is a breakdown of the new features/fixes and their applicability to the `paranoiascape-recomp` port.

---

## 1. Upstream `psxrecomp` Key Updates & Applicability

The upstream repository has undergone several significant updates related to rendering, safety/hardening, interpreter/recompiler boundary contracts, and telemetry:

| Commit / Feature | Upstream Description | Applicability to `paranoiascape-recomp` |
| :--- | :--- | :--- |
| **Bilinear Texture Filtering & SSAA**<br>(`34561b5`, `2a05369`, `9474765`) | Optional bilinear texture filtering, configurable internal-resolution supersampling, and red/blue swap fixes in the software renderer. | **Low/None**<br>Our runner uses a custom hardware-accelerated OpenGL renderer ([opengl_renderer.cpp](file:///j:/projects/paranoia/paranoiascape-recomp/runner/src/opengl_renderer.cpp)) instead of the software renderer (`gpu_sw_renderer.c`). These options are already natively supported or handled differently via OpenGL. |
| **Interpreter Stop Address Return Contract**<br>(`c496dc1`) | The interpreter (`dirty_ram_dispatch`) now accepts a `stop_addr` return contract target. When local flow or branches reach it, the interpreter exits immediately to hand control back to the native dispatch loop. This prevents double-execution stack frame bugs. | **High**<br>We maintain a custom MIPS interpreter ([mips_interpret](file:///j:/projects/paranoia/paranoiascape-recomp/runner/src/runtime.c#L1710)) and dispatch handler ([call_by_address](file:///j:/projects/paranoia/paranoiascape-recomp/runner/src/runtime.c#L1938)). If we experience crashes during transition callbacks between recompiled C code and interpreted overlays, implementing a return contract boundary check will prevent stack corruption and double-runs. |
| **DMA & GPU Hardening**<br>(`9cd98b0`) | Validates BCR-derived transfer lengths on DMA kicks to prevent wild loops. Caps CD overlay-capture spans. Routes all fatal DMA/GPU exits to a recoverable `psx_fatal_halt` so debug rings remain active for TCP query post-mortem. | **High**<br>We emulate DMA2 (GPU) and DMA4 (SPU) inside [runtime.c](file:///j:/projects/paranoia/paranoiascape-recomp/runner/src/runtime.c#L1011). Adding similar transfer length validation and wrapping unimplemented cases/GPU command crashes in a custom diagnostic halt (instead of `exit(1)`) will prevent the recompiler from dropping its connection, keeping the TCP debug server responsive. |
| **BIOS Vector-Table Discovery Roots**<br>(`97d2528`) | Seeds A0/B0/C0 kernel functions (e.g. `strncat`) from the ROM vector tables as discovery roots so function discovery picks them up natively, preventing them from falling back to runtime stubs. | **Low**<br>Our port HLE-emulates BIOS calls directly in [runtime.c](file:///j:/projects/paranoia/paranoiascape-recomp/runner/src/runtime.c#L2043) and does not compile the BIOS itself. |
| **Turbo Load Audio Muting**<br>(`7249123`) | Mutes SPU output during turbo loads/fast-forwarding to prevent audio stutter/pitch-distortion noise. | **Medium**<br>A helpful QoL feature if we decide to implement turbo load-skips for game verification. |
| **Tooling & Observability Rings**<br>(`2d6a75d`, `aa536b8`, `31377b6`) | Adds a dedicated GP1 command trace ring, serve-stall logging, and automatic dump-on-crash logs. | **Medium**<br>These are excellent tools for tracing SIO/GPU hang regressions. |

---

## 2. RecompOne (C# Recompiler) Context

The cloned `RecompOne` repository is a separate project inspired by `N64Recomp` and `XenonRecomp` written in **C#** (translates MIPS to C#). 
- **Code Compatibility**: Since it uses C# and a different runtime layer, we cannot copy or merge code directly.
- **Logical Insights**: It contains custom audio/SPU buffer limits and CD streaming pause fixes that could serve as logical references if we encounter audio stutter or sector-loading pause stalls.

---

## Recommended Actions

If we want to port useful features from the upstream update, we should focus on:
1. **DMA Length Validation**: Add checks to DMA block size calculation in [runtime.c](file:///j:/projects/paranoia/paranoiascape-recomp/runner/src/runtime.c#L1066) to verify they do not exceed 2MB.
2. **Safe Fatal-Halt Routing**: Replace abrupt `exit(1)` or `abort()` calls in GPU/SPU/DMA parsing with a diagnostic warning and wait-state so that the debug TCP server on port `4370` doesn't drop.
