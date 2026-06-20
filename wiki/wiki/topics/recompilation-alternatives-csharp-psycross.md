---
title: "Recompilation Alternatives: C# and PsyCross"
category: topic
sources:
  - raw\notes\recompone-overview.md
  - raw\notes\psycross-overview.md
created: 2026-06-19
updated: 2026-06-19
tags: [psx, recomp, design, comparisons, alternatives]
confidence: high
summary: "Comparative analysis of PS1 recompilation and porting paths, comparing PSXRecomp, RecompOne, and PsyCross SDK abstraction models."
volatility: warm
---

# Recompilation Alternatives: C# and PsyCross

When porting legacy PlayStation 1 software to modern host operating systems, developers have three primary choices: C-based static recompilation (PSXRecomp), C#-based static recompilation (RecompOne), and source-level API replacement (PsyCross / PSY-Z).

## 1. Abstraction Level Comparison
The tools operate at fundamentally different positions on the compiler-vs-porting spectrum:

| Metric | PSXRecomp | RecompOne | PsyCross / PSY-Z |
|---|---|---|---|
| **Primary Language** | C / C++ | C# | C |
| **Input Material** | MIPS binary (ISO / EXE) | MIPS binary (ISO / ELF) | Original game source code |
| **Approach** | Static binary recompile (LLE) | Static binary recompile (LLE) | API replacement layer (HLE) |
| **GTE / GPU Path** | Native OpenGL wrapper | Native C# SDL wrapper | OpenGL & PGXP-Z depth checks |
| **BIOS Path** | Recompiled SCPH1001 ROM | Emulated BIOS calls | Emulated SDK interfaces |

## 2. Recompiler Designs: C vs C#
Both **PSXRecomp** and **RecompOne** statically translate binary MIPS instructions.
- **Performance**: C-based execution (PSXRecomp) compiled natively has lower overhead. C#-based execution (RecompOne) relies on a JIT-compiler runtime and managed memory (`CpuContext` objects), which is cleaner to trace but introduces garbage-collection and JIT compilation overhead.
- **Overlay Management**:
  - PSXRecomp recompiles overlays dynamically on the fly into cached DLLs.
  - RecompOne generates separate class definitions for each overlay block beforehand using an ELF mapping, handling routing via an overlay dispatch table.
- **Modding / Function Patching**:
  - PSXRecomp hooks functions in runtime code (`runtime.c`).
  - RecompOne allows clean JSON mapping redirects to redirect execution directly to custom C# methods compiled into the output project.

## 3. Static Recompilation vs API Replacement (PsyCross)
**PsyCross** and **PSY-Z** do not translate MIPS binary code; they replace the underlying compiler SDK (PSY-Q) entirely.
- **Binary requirements**: API replacement requires complete access to the game's original C source code, whereas recompilers operate on retail binaries directly.
- **Execution accuracy**: Recompilers execute the original compiled instruction pathways block-by-block. PsyCross replaces them with high-level equivalents, introducing potential discrepancies if the game relies on undocumented hardware side-effects or compiler optimizations.
- **Render enhancements**: PsyCross's PGXP-Z provides high-fidelity vertex caching and native depth-buffering, solving the visual polygon-jitter and texture-distortion common in PS1 games.

## See Also
- [[recompone-overlay-dispatch|RecompOne Overlay Dispatch]] ([RecompOne Overlay Dispatch](../concepts/recompone-overlay-dispatch.md))
- [[psycross-spu-and-gte|PsyCross SPU and GTE]] ([PsyCross SPU and GTE](../concepts/psycross-spu-and-gte.md))
- [[psxrecomp-framework-architecture|PSXRecomp Framework Architecture]] ([PSXRecomp Framework Architecture](../topics/psxrecomp-framework-architecture.md))
