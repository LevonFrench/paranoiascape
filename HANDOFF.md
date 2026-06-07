# Paranoiascape Session Handoff

## Date: 2026-06-06 (VBlank Deadlock Resolution & Unified Clean Git Repo)

## Current Status

### What Works
- **MIPS-to-C Stack Pointer Leak Fix**:
  - Modified the Stage 1 recompiler (`PSXRecomp.exe`) to restore the stack pointer (`cpu->sp = cpu->sp + func.stack_frame_size`) prior to switch-case fallback and unrecognized indirect return branches.
  - Regenerated code and rebuilt the runner. The stack pointer remains stable, eyeball 3D models render correctly, and warning spam is eliminated.
- **60Hz VBlank Interrupt Deadlock Fix**:
  - Game hung at frame ~1516 inside loop `func_800130D4` because `gp + 436` remained `0`, bypassing VSync and disabling VBlank callback processing.
  - Implemented a 60Hz VBlank interrupt pump in [runtime.c](file:///j:/projects/paranoia/paranoiascape-recomp/runner/src/runtime.c) at address `0x80055F44u` to execute `func_80051434(cpu)` while preserving register states.
  - Validation runs verify the deadlock is successfully broken: display resolution drops to `320x240` gameplay mode and VSync frames continue incrementing.
- **SIO Pad Overrides**:
  - Intercepted SIO commands in `runtime.c` to read socket input values from `play_game.py` natively.
- **Unified Clean Repository**:
  - Created strict [.gitignore](file:///j:/projects/paranoia/.gitignore) and [paranoiascape-recomp/.gitignore](file:///j:/projects/paranoia/paranoiascape-recomp/.gitignore) files.
  - Renamed nested `.git` to `.git_bak` to unify tracking.
  - Initialized clean repository on branch `main` at remote origin `https://github.com/LevonFrench/paranoiascape.git` containing only custom-written code and documentation.
- **Documentation**:
  - Detailed findings in [wiki/recompiler-stack-leak-and-vblank-pump-fix.md](file:///j:/projects/paranoia/wiki/recompiler-stack-leak-and-vblank-pump-fix.md).

---

## Next Steps

1. **Verify gameplay rendering stability**:
   - Verify that 3D visuals and backgrounds remain stable as the player progresses through Stage 1.
   - Run additional gameplay tests to confirm that no other stack pointer leaks or callback stalls are triggered during real-time gameplay.
2. **Review snapshot display timings**:
   - Check if gameplay rendering exhibits any frame-buffering timing glitches or black areas, particularly around menu-to-stage boundaries.
