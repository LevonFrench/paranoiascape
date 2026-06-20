---
title: "RecompOne Overlay Dispatch"
category: concept
sources:
  - raw\notes\recompone-overview.md
created: 2026-06-19
updated: 2026-06-19
tags: [psx, recomp, csharp, overlays, patches]
confidence: high
summary: "RecompOne overlay class structure, virtual address dispatch tables, and runtime function overrides."
volatility: warm
---

# RecompOne Overlay Dispatch

RecompOne translates PS1 MIPS binaries into native C# files, mapping registers and memory accesses onto a virtual `CpuContext` struct.

## 1. Class-Based Overlay Structure
Because PS1 RAM is extremely limited, games frequently swap code and constant blocks (overlays) into the same RAM addresses.
- **Segment Generation**: The recompiler reads the input ELF and translates each overlay segment into a separate C# class.
- **Dynamic Tables**: The runtime manages an **overlay dispatch table**. When an overlay load is executed, the dispatcher updates virtual address function pointers to point to the active class. This prevents address-collision bugs when different segments occupy the same RAM region.

## 2. JSON Function Overrides (Patches)
Self-modifying code or hardware-timed delays cannot be easily represented in compiled C#. RecompOne enables overriding recompiled functions via JSON patches:
- **Configuration mapping**:
  ```json
  { "address": "800553C4", "name": "MyPatch.MyClass.MyMethod" }
  ```
- **Execution redirect**: The runtime hooks the address `0x800553C4` in the dispatch table, executing the custom C# method instead of the recompiled MIPS codeblock. This serves as a mod loader mechanism for custom assets, resolutions, and bug fixes.

## See Also
- [[recompilation-alternatives-csharp-psycross|Recompilation Alternatives: C# and PsyCross]] ([Recompilation Alternatives: C# and PsyCross](../topics/recompilation-alternatives-csharp-psycross.md))
- [[psxrecomp-framework-architecture|PSXRecomp Framework Architecture]] ([PSXRecomp Framework Architecture](../topics/psxrecomp-framework-architecture.md))
