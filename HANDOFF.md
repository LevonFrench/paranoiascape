# Paranoiascape Session Handoff

## Date: 2026-06-11 (FMV Skipping & Keyboard Input Integration)

## Current Status

### What Works
- **FMV Skip Hook Integration**:
  - Identified that the recompiler only dispatches function entry points via `psx_override_dispatch`. The previous skip intercept address `0x800804F4u` was an inner block label and was never executed, leaving the FMV loop unskippable by physical input.
  - Relocated the intercept to the function entry point `0x800804C0u` in `runner/src/runtime.c`.
  - Configured the hook to query physical keyboard inputs (`g_pad1_state`) when no debug socket override is present. Pressing **ENTER** (Start) or **Z** (Cross) now successfully triggers the skip.
  - Added pointer bounds checking for `0x800A5560` to prevent access violations during early boot when the pointer is uninitialized/invalid.
- **Stage 1 Gameplay Entered**:
  - The runner successfully skips all boot/cinematic FMVs using the new hook, displays the Title Screen, processes the menu selections, and transitions into Stage 1 gameplay.
  - Level loading loop completes without any deadlock issues.

---

## Next Steps

1. **Test Stage 1 Controls**:
   - Ask the user to run the game and confirm that moving and attacking using Arrow keys and Z/X/C/V buttons works interactively in the window.
2. **Verify Level Completion & Progressions**:
   - Play through Stage 1 to check if level transition triggers (e.g. Stage 2 load) work stably.
3. **Verify Audio/Video Performance**:
   - Verify that CD-DA audio and in-game FMVs continue playing cleanly.
