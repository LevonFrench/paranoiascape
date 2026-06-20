---
title: "PSXRecomp Debugging Discipline"
category: concept
sources:
  - raw\notes\psxrecomp-principles.md
created: 2026-06-19
updated: 2026-06-19
tags: [psx, recomp, debugging, discipline]
confidence: high
summary: "Debugging workflows and core philosophy rules for static recompilation, emphasizing state comparison, finding the first divergence, and ensuring end-to-end completion."
volatility: warm
---

# PSXRecomp Debugging Discipline

Recompilation debugging demands a rigorous discipline because minor compiler artifacts or state divergences compound rapidly over time. Traditional "printf debugging" or speculative patching ("I think this might work") introduces regressions and wastes tokens.

## 1. Core Philosophy: We Do Not Guess
Every debugging session must follow three negative rules and three positive actions:
- **No Speculative Exploration**: Do not guess what a function does or modify logic without evidence.
- **No Blind Probing**: Gather detailed logs and state before making changes.
- **No Symptom-Fixing**: Avoid hardcoding variables or clamping bounds to satisfy intermediate values (e.g. bypassing CD sync returns).

Instead, identify:
1. The **exact point of divergence** in code execution.
2. The **exact state difference** in memory or register buffers.
3. The **exact instruction or function** responsible for the state mutation.
Fix that, and only that.

## 2. Finding the First Divergence
If two systems (recompiled PC executable and emulator oracle) behave differently, then their state *must* differ. Finding the first moment where expected behavior and actual behavior diverge is the single most critical step in debugging.
- **Sync State via PC/Registers**: Do not compare cycle frames directly, as timing drifts quickly. Compare GPRs, HI/LO registers, and program counters.
- **Inspect Subsystems**: Diff MMIO sequences, DMA channels, and CPU flags.
- **Trace the Writer**: Locate the specific function that wrote the divergent bytes using memory watchpoints.

## 3. End-to-End (E2E) Verification Loop
Never accept an intermediate success metric as a resolved issue. Subsystems must be traced from their producer logic to their final consumer.
- **False Milestones**: "Counter decrements", "IRQ handler installs", or "DMA registers populate" are meaningless if pixels never reach the screen.
- **Verification Flow**:
  1. Define E2E success criteria (what does the user see when the feature works?).
  2. Trace producer state to consumer usage.
  3. Locate first semantic divergence.
  4. Fix root cause in the producing logic.
  5. Rerun full flow and verify the entire loop closes.

## See Also
- [[psxrecomp-tcp-telemetry|PSXRecomp TCP Telemetry]] ([PSXRecomp TCP Telemetry](../concepts/psxrecomp-tcp-telemetry.md))
- [[psxrecomp-framework-architecture|PSXRecomp Framework Architecture]] ([PSXRecomp Framework Architecture](../topics/psxrecomp-framework-architecture.md))
