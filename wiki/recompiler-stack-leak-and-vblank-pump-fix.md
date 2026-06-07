# Recompiler Stack Pointer Leak & VBlank Pump Deadlock Fix

Documenting the two major technical achievements that resolved systemic memory corruption and gameplay loading stalls.

---

## 1. MIPS-to-C Stack Pointer Leak Fix

### Symptom & Probing
During execution, the console would flood with infinite warnings:
`non supported code: a1=0x00000000 a2=0x801EE560 ra=0x8007441C`
This warning indicates that the interpreter was called to execute unrecognized/uncompiled instructions at `0x801EE560`. 
Investigation of RAM showed that `0x801EE560` holds the 3D model data for the spinning eyeballs on the Title Screen. Due to stack corruption, the game's return addresses and parameters were offset, leading execution to jump straight into the middle of raw TMD model data.

### Root Cause Analysis
During translation of MIPS jump tables and unrecognized indirect jumps, the `PSXRecomp` recompiler generated early return branches. In these early return paths:
1. The function's stack frame was allocated on entry: `cpu->sp = cpu->sp - func.stack_frame_size`.
2. Upon hitting an unrecognized jump or falling through a switch-case jump table to the `default:` fallback, the compiler generated a standard C `return;` statement.
3. **The bug**: The stack frame was *never* restored before these early returns. Every time a switch-case fallback or unrecognized branch was hit, the stack pointer leaked by `func.stack_frame_size` bytes.

### The Fix
We modified the recompiler code generator (`recompiler/src/code_generator.cpp`) to inject a stack pointer restoration statement before generating any early returns:
```cpp
// Inject stack pointer restoration before early returns
output_line("cpu->sp = cpu->sp + " + std::to_string(func.stack_frame_size) + ";");
output_line("return;");
```
- Rebuilt `PSXRecomp.exe` and regenerated the game's source files (`SLPS_013.75_full.c` and `SLPS_013.75_dispatch.c`).
- Static analysis confirmed all 131 jump-table stack leaks were successfully patched.
- Verified that the eyeball 3D models render correctly, stack pointer (`cpu->sp`) remains completely stable, and the console warning spam has been eliminated.

---

## 2. Gameplay Loading Deadlock & 60Hz VBlank Pump

### Symptom & Probing
When triggering the transition to gameplay at the Title Screen (frame ~1516), the recompiler would hang indefinitely. Frame ticks stopped incrementing, and CPU logs showed the recompiler spinning endlessly in a single loop.

### Root Cause Analysis
During gameplay loading, the game's main orchestrator `func_800130D4` loops internally at `block_800130EC` waiting for the progress indicator variable `gp + 436` (located at RAM `0x8009F278`) to become non-zero. 

On real hardware:
1. The CPU polls `gp + 436`.
2. Asynchronous VBlank hardware interrupts periodically fire, invoking the interrupt handler `func_80051434`.
3. The interrupt handler executes registered background callbacks (such as the CD-ROM loading progress task `func_80019514`).
4. The loading progress callback increments `gp + 436` once the file chunks are loaded in RAM.
5. The polling loop exits.

In the recompiled PC runner:
- `PSXRecomp` does not emulate hardware interrupts asynchronously. Instead, it hooks the game's custom VSync function to pump interrupts at the end of every frame display.
- Because `gp + 436 == 0` bypassed the VSync call, the recompiled runner stayed inside the polling spin loop *without ever returning to the VSync pump*.
- As a result, the VBlank interrupt handler never executed, keeping the loading callback from firing and locking the game in an infinite deadlock.

### The Fix
We implemented a **60Hz VBlank interrupt pump** inside the runtime spin loop address `0x80055F44u` in `runner/src/runtime.c`:
1. We intercept the spin function at `0x80055F44u` where the game is waiting.
2. If the loading bypass flag `gp + 436` is `0`, we check if `1/60th` of a second has elapsed since the last simulated interrupt.
3. If the timer has ticked, we save the CPU register state (`cpu->regs`), call the VBlank interrupt handler `func_80051434(cpu)` directly, and then restore the CPU registers.
4. This simulates asynchronous hardware interrupts, driving the background task queue forward.
5. Once the loading tasks are completed, `gp + 436` is set to non-zero, and the spin loop naturally exits.

### Verification
- Verified that the game successfully transitions from the Title Screen and Main Menu past frame 1516.
- The game now drops the display resolution down from `640x480` (menu/title mode) to `320x240` (gameplay mode) and continues VSync ticks indefinitely.
