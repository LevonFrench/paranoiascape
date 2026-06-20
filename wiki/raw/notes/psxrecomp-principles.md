---
title: "PSXRecomp Philosophy and Principles"
source: "j:/projects/paranoia/psxrecomp/PRINCIPLES.md and psxrecomp/DEBUG.md"
type: notes
ingested: 2026-06-19
tags: [psx, recomp, debugging, principles]
summary: "Philosophical principles, divergence constraints, stale hypothesis boundaries, and tool lifecycles for static recompilation debugging."
---

# Core Philosophy
1. We do not guess.
2. We do not explore blindly.
3. We do not fix symptoms.
4. We identify:
   - The exact point of divergence.
   - The exact state difference.
   - The exact instruction or function responsible.
   - Then we fix that — and only that.

# State Over Theory
If two systems behave differently, then their state differs.
All debugging reduces to:
- Capturing state.
- Comparing state.
- Finding the first difference.
Do not theorize causes without state evidence.

# First Divergence (Critical)
Never debug the final symptom. Always find:
> The FIRST moment where expected != actual
If you are not identifying the first divergence, you are doing it wrong.

# Stale Hypothesis Rule
If a handoff contains stale claims contradicted by new logs, **update the problem statement before changing code**. Never continue debugging from an invalidated hypothesis. If there are contradictions or stale data, STOP and inform the user immediately before proceeding.

# Minimal Fixes
The correct fix:
- Smallest possible change.
- Matches original system behavior.
- Invalid: clamping, skipping logic, hardcoding values.
- Valid: fixing the producing logic, fixing execution order, reproducing missing state.

# E2E Completion Rule
When debugging boot/runtime flows, **never accept an internal milestone as success unless the full user-visible loop closes.**
- "counter reached 0" is NOT success if the shell still retries
- "IRQ fired" is NOT success if the waiting code still loops
- "callback returned" is NOT success if the owning state machine does not advance
- "data structure written" is NOT success if the consumer still rejects it
- "handler installed" is NOT success if the VBlank counter never increments
- "chain head populated" is NOT success if pixels don't reach the screen

Required method:
1. Define full end-to-end success criteria FIRST — what does the USER see when it works?
2. Trace producer AND consumer — follow the data from origin to final consumer
3. Find the first semantic divergence — where does the consumer's view differ from expected?
4. Fix subsystem behaviour — correct the root cause in the producing subsystem
5. Rerun the full flow — verify the entire loop closes, not just the intermediate step
6. Reject any patch that only changes intermediate state without closing the loop

# Tooling Rules
- **No Stubs — Ever**: If execution reaches unknown code, STOP, identify target, and fix discovery/codegen. Never simulate behavior.
- **Distrust Tooling**: Validate tools on first use. Run a known-good query, check output by hand, verify file paths resolve where expected.
- **Oracles**: Truth comes from Ghidra for what the code is supposed to do (static analysis) and DuckStation for what real PS1 hardware does at runtime (dynamic oracle). Use both.
- **Bail on server gaps**: If the TCP debug server cannot answer a question, STOP. Do not guess, do not write raw logs. Ask user to add TCP debug commands.
- **Tool Lifecycle Control**: Agent must build, launch, and close all emulators/runtimes automatically.
  - Build native: `(cd runtime && cmake --build build)` (with `PATH=/c/msys64/mingw64/bin:$PATH`)
  - Launch native: `./runtime/build/psx-runtime.exe &`
  - Close: `taskkill /F /IM psx-runtime.exe`
