# Paranoiascape Session Handoff

## Date: 2026-06-06 (SIO Pad Polling & Title Gating Resolution)

## Current Status

### What Works
- **SIO Joypad Halfword Read Mock**:
  - Identified that the Title Screen uses low-level SIO hardware polling (`func_8008D330`) reading from `0x1F801040` as a halfword (`LH`/`LHU`), which was returning `0x0000` (disconnected).
  - Implemented SIO pad and signature mock logic in `read_half(0x1F801040u)` in [runtime.c](file:///j:/projects/paranoia/paranoiascape-recomp/runner/src/runtime.c).
  - Validation runs verify that automated input sequences now successfully bypass the Title Screen and Main Menu.
- **Stage 1 Gameplay Entrance**:
  - The game successfully processes menu inputs, selects the level from the 3D Stage Select diorama menu, and enters Stage 1 gameplay.
  - Verified via VRAM screenshots that the HUD renders and Stage 1 gameplay 3D models (eyeball entity) load correctly.
- **MIPS-to-C Stack Pointer Leak Fix**:
  - Modified the Stage 1 recompiler (`PSXRecomp.exe`) to restore the stack pointer (`cpu->sp = cpu->sp + func.stack_frame_size`) prior to switch-case fallback and unrecognized indirect return branches.
  - Stack pointer remains stable, eyeball 3D models render correctly, and warning spam is eliminated.
- **60Hz VBlank Interrupt Deadlock Fix**:
  - Implemented a 60Hz VBlank interrupt pump in `runtime.c` at address `0x80055F44u` to execute `func_80051434(cpu)` while preserving register states.
  - Resolves the loader deadlock past frame 1516.
- **Unified Clean Repository**:
  - Initialized clean repository on branch `main` at remote origin `https://github.com/LevonFrench/paranoiascape.git` containing only custom-written code and documentation.

---

## Next Steps

1. **Validate Keyboard Input Mapping**:
   - Check if physical keyboard inputs map correctly to SIO emulation inputs inside GLFW window polling, enabling normal keyboard-driven gameplay.
2. **Verify Stage 1 Gameplay Stability**:
   - Play further into Stage 1 to check for any rendering errors, MDEC video playback timing bugs, or audio synchronicity issues.
3. **Review Renderer Performance & Snapshots**:
   - Verify that display snapshot rendering timing does not exhibit flickering or display glitches during real-time movement.
