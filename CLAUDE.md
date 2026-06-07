# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Identity

This repo ports the PS1 game **Paranoia Scape (SLPS_013.75, Japan)** to native PC using the **PSXRecomp** static recompilation framework (MIPS R3000A -> C -> native x64). It is **not** an emulator: every JAL becomes a direct C call. PSXRecomp was originally tailored for Tomba!, so the runtime still contains Tomba-specific intercepts that must be replaced for Paranoiascape — see `AGENTS.md` §13.

Entry artifacts:
- `paranoiascape-recomp/` — PSXRecomp framework + game-specific glue (this is the actual source tree to work in).
- `wiki/` — current state of reverse-engineering work; **read `wiki/project-handoff.md` before changing anything game-specific**.
- `AGENTS.md` — debugging discipline rules. **Treat §0 as binding** for any work in this repo.

## Two-Stage Build

The build is two CMake projects, run in order. They live under `paranoiascape-recomp/`.

### Stage 1 — Recompiler (MIPS -> C generator)

Builds `PSXRecomp.exe` (Ninja, MinGW-w64 GCC, C++20):

```bash
cmake -S paranoiascape-recomp/recompiler -B paranoiascape-recomp/build/recompiler -G Ninja
ninja -C paranoiascape-recomp/build/recompiler
```

Run it against the headerless EXE to regenerate `paranoiascape-recomp/generated/SLPS_013.75_full.c` and `_dispatch.c`:

```bash
paranoiascape-recomp/build/recompiler/PSXRecomp.exe isos/extracted/SLPS_013.75 \
    --extra-funcs paranoiascape-recomp/discovered_functions.log
```

Forced entry points (e.g. `0x80012E2C` — see `wiki/paranoiascape-boot-sequence.md`) live in `paranoiascape-recomp/recompiler/src/main_psx.cpp`. Editing those requires Stage 1 rebuild + regeneration before Stage 2.

### Stage 2 — Runner (links generated C into native exe)

Two configured trees exist. Either works; both produce `recomp.exe`:

- **`paranoiascape-recomp/runner/build/`** — Visual Studio 16 2019. The `.bat` launchers use this output (`runner/build/Release/recomp.exe`).
- **`paranoiascape-recomp/build/runner/`** — Ninja.

VS rebuild:
```powershell
cmake --build paranoiascape-recomp/runner/build --config Release
```

The runner's `CMakeLists.txt` directly links `../generated/SLPS_013.75_full.c` and `../generated/SLPS_013.75_dispatch.c`. **These are output of Stage 1 — never edit them by hand** (see AGENTS.md Rule 0b).

### Run

Two batch files at repo root:
- `run_recomp.bat` / `test_recomp.bat` — launches the recompiled native build with the disc image.
- `run_emulator.bat` — launches PCSX-Redux on the same ISO (ground-truth comparison).

Both expect the EXE at `isos/extracted/SLPS_013.75` and the cue at `isos/Paranoia Scape (Japan).cue`. FMV requires `avcodec-*.dll`, `avformat-*.dll`, `avutil-*.dll`, `swresample-*.dll`, `swscale-*.dll` next to `recomp.exe`.

There is no test suite. Verification is screen + log + VRAM-dump comparison against PCSX-Redux.

## Architecture That Spans Files

- **MIPS not in the static recomp falls back to an interpreter in `runner/src/runtime.c`.** This is the source of the boot-time stack overflow documented in `wiki/paranoiascape-boot-sequence.md`: the interpreter and `call_by_address` mutually recurse on tight loops, so addresses like `0x80012E2C` (entry without a standard prologue) must be added as forced entries in the recompiler so a real C function exists.
- **Game-specific behavior is injected via two seams**, never by editing generated code:
  1. `psx_override_dispatch()` runtime intercepts in `runner/src/runtime.c` (BIOS / PsyQ replacements, address-specific hooks).
  2. The `game_extras.h` interface implemented in `runner/src/extras.cpp` (window title, EXE filename, init/frame hooks, debug commands).
- **VBlank is the heartbeat.** PSXRecomp does not emulate the PS1 interrupt controller; every VBlank-driven side effect (thread scheduling, GPU busy clear, controller polling, sound ticks) is manually replicated in the VSync intercept. New stalls after frame 1 should always be investigated as missing VBlank work first.
- **Input is hardware-level on this game.** Paranoiascape bypasses BIOS `PadRead` and bit-bangs SIO0 directly. The decoded controller buffer lives at RAM `0x8013E5EC` (active-low, little-endian). For automation, write that address via `pcsx-redux/pcsx_cmd.py` rather than chasing the SIO state machine. See `wiki/paranoiascape-input-graphics.md`.
- **Title screen is in 24-bit color (640x480 interlaced).** The current OpenGL fragment shader reads 24-bit VRAM as 15bpp, producing horizontal squash. Don't waste cycles fixing this — bypass to 320x240 gameplay via injected Start/Circle.

## Required External Tooling

These are not optional; AGENTS.md treats them as ground truth. Both run on `localhost`:

- **GhidraMCP HTTP API on `:8080`** — disassembly + xrefs. **Before writing any intercept, query Ghidra**, not the generated C. AGENTS.md §0a-1 lists working endpoints (`/segments`, `/xrefs_to`, `/xrefs_from`, `/decompile`).
- **PCSX-Redux REST API on `:8079`** — live RAM/VRAM, execution control. Wrapper: `pcsx-redux/pcsx_cmd.py` (read/write/scan/watch/vram). The MCP SSE endpoint is `:8078`. **Do not use Lua-based MCP** — Lua hooks halt when the emulator pauses, exactly when introspection is needed. PCSX-Redux config requires Debugger + Web Server (8079) + GDB Server (3333) all enabled.

## Repository Hygiene Notes

- The repository root is cluttered with one-off investigation scripts (`find_pad*.py`, `gdb_*.py`, `check_vram*.py`, log files multiple GB in size). These are scratch artifacts from active debugging sessions, not part of the build. Don't reorganize them, but don't read them assuming they reflect current state either — confirm against `wiki/` and `AGENTS.md`.
- `toukonold/` is a reference snapshot of the previous Tomba! port and is not built.
- Crash logs (`crash.log`, `recomp_log.txt`, `run*.log`) are intentionally kept around as forensic artifacts; some are hundreds of MB. Don't `cat` them.
- `paranoiascape-recomp/.gitignore` excludes `*.png` and `*.bmp` — captured screenshots from the auto-capture system are local-only.

## Operating Rules (from AGENTS.md — non-negotiable)

These override the default "move fast" instinct. Before any code change in this project:

1. **Ghidra first, code second.** Don't speculate about what a function does. Query GhidraMCP.
2. **Never hand-edit `paranoiascape-recomp/generated/*.c`.** All fixes go through `psx_override_dispatch()` or the recompiler config.
3. **Reproduce before fixing.** Capture the broken behavior (screenshot, log, VRAM dump) and identify the divergent address before proposing a fix.
4. **One fix, one variable.** Build, run, capture, compare — then change the next thing.
5. **Don't bypass `CdSync` / `CdReady`.** Forcing them to return instantly skips the FIFO copy and corrupts MSF/file-size data downstream. Fix the underlying register emulation instead.
6. **Scrutinize hardcoded `0x800XXXXX` addresses in `runtime.c`** — many are leftover Tomba hooks that are wrong for Paranoiascape.
