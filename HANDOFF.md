# Paranoiascape Session Handoff

## Date: 2026-06-06 (Fix Recompiler Stack Pointer Leak)

## Current Status

### What Works
- **MIPS-to-C Stack Pointer Leak Fix**:
  - Identified a systemic stack pointer (`cpu->sp`) leak in the recompiler's generated early returns (jump table `default` fallback branches and unrecognized indirect jumps).
  - Modified the Stage 1 recompiler (`PSXRecomp.exe`) to restore the stack pointer (`cpu->sp = cpu->sp + func.stack_frame_size`) prior to these early return statements.
  - Rebuilt the recompiler and regenerated `SLPS_013.75_full.c` and `SLPS_013.75_dispatch.c`.
  - Static analysis confirmed all 131 jump-table stack leaks were successfully patched. The only early returns left are split-function mid-func jumps, which correctly inherit the stack frame and thus should not restore the stack frame at the jump site.
- **Stage 2 Runner Rebuild**:
  - Rebuilt the Stage 2 runner (`recomp.exe`) using Ninja.
- **Runtime Verification**:
  - Ran the game up to frame ~3000.
  - The infinite loop console warning spam `non supported code: a1=0x00000000 a2=0x801EE560 ra=0x8007441C` has been completely eliminated.
  - Verified from automated screenshots (e.g. `auto_f1400.png` and `auto_f2000.png`) that the title screen rendering functions correctly.
  - The stack pointer remains stable throughout runtime, and the eyeball 3D models render correctly without clobbering RAM.

---

## Next Steps (Priority Order)

1. **Verify gameplay input and progression**:
   - Ensure inputs can navigate menus and start the game properly.
   - **Note on Automation:** The `paranoiascape-recomp/play_game.py` script currently hangs in an infinite while loop waiting for `0x0000000F` from RAM address `0x8013DC60` because the debug server returns RAM reads in little-endian format (which outputs `0f000000`). Modifying the condition to look for `0f000000` will unblock the automated menu navigation script.
   - Run additional gameplay tests to confirm that no other stack pointer leaks are triggered in other sections of the game.
2. **Track gameplay frame rates and rendering**:
   - Verify that 3D visuals and backgrounds remain stable as the player progresses through Stage 1.
