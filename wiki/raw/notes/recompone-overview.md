---
title: "RecompOne Overview and C# Recompilation"
source: "j:/projects/paranoia/RecompOne/README.md"
type: notes
ingested: 2026-06-19
tags: [psx, recomp, csharp, overlays, patches]
summary: "Description of the RecompOne recompiler mapping MIPS to C# operations, managing class-based overlay dispatch tables, and implementing JSON function overrides."
---

# RecompOne Overview
RecompOne is an alternative PS1 static recompiler that translates PlayStation 1 executables (MIPS) into native C# code and wraps them with a hardware runtime layer.

## How it Works
MIPS instructions are translated into C# equivalents operating on a `CpuContext` and memory interface:
- Arithmetic: `addiu t0, t0, 0x10` -> `c.T0 = c.T0 + 0x10u;`
- Memory: `lw t0, 0x10(sp)` -> `c.T0 = mem.Read32(c.SP + 0x10u);`

## Overlay Handling
PS1 memory limits require overlays. RecompOne handles overlays by:
- Defining them in a JSON config file.
- Generating a separate C# class for every overlay.
- Maintaining an overlay dispatch table.
- At runtime, the dispatcher dynamically maps active functions to ensure virtual addresses resolve to the currently loaded overlay code.

## Function Patching
RecompOne supports replacing functions entirely via patches registered in the recomp configuration:
- Config: `{"address": "800553C4", "name": "MyPatch.MyClass.MyMethod"}`
- Patches are compiled into the native C# project to override problem functions (such as self-modifying code) or inject base mods and QoL features.

## Stance on AI
The author states a strict policy: RecompOne is not vibe-coded. AI was not used to write the codebase. Ports generated via AI-guided code generation without manual validation are explicitly unsupported by the maintainer.
