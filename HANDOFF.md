# Paranoiascape Session Handoff — GTE Translation Scaling Fix

## Summary of Accomplishments

- **Fixed 3D Geometry Projection**:
  - Identified a critical scaling mismatch in `gte_rtps_internal` (`gte.cpp`): translation values (`TR`) were written by the MIPS CPU already scaled by 4096, but double-shifted by 12 left (`<< 12`) in the projection path, scaling them to $2^{24}$ and causing translation to collapse the 3D coordinates.
  - Removed the double-shifts from `gte->TR[0]`, `gte->TR[1]`, and `gte->TR[2]`, aligning them with the rotation scale ($2^{12}$).
- **Performance Optimization**:
  - Commented out verbose hot-path printfs in `gpu_interpreter.cpp` (`FillRect`, `CopyRect`, `DisplayMode`) and `runtime.c` (`DMA2-LL`) that were bottlenecking execution down to ~2 FPS. The runner now executes at full 60 FPS.
- **Robust Automation**:
  - Re-implemented the active gameplay check in `test_play_flipper.py` to require multiple consecutive readings of overlay state 0 (`osm == 0`), preventing premature trigger during transient transition states.
- **Visual Verification**:
  - The 3D models on the stage selection screen now render correctly with accurate perspective projection, lighting, and textures.

## Verification Artifacts

- **Screenshot**: [auto_f3300.png](file:///C:/Users/hotgh/.gemini/antigravity-ide/brain/180cfeeb-0d99-4e29-8605-5ce8b41bb11a/auto_f3300.png) shows the fleshy 3D pinball tables rendering in high-fidelity on the stage selection screen.
- **Walkthrough**: [walkthrough.md](file:///C:/Users/hotgh/.gemini/antigravity-ide/brain/180cfeeb-0d99-4e29-8605-5ce8b41bb11a/walkthrough.md) summarizes the changes.

## Next Steps

1. **Test Active In-Game Pinball Geometry**:
   - Continue playing beyond the layout selection screen to verify that in-game active 3D corridors, bumpers, and other moving elements also render correctly under the GTE fixes.
2. **Review Other GTE Matrices**:
   - Verify that rotation matrices and lighting normals (`BK`, `FC`, `LC`) do not exhibit minor scaling issues during gameplay transitions.
