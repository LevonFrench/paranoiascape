---
title: "Paranoiascape State Machine Update"
category: concept
sources: []
compiled-from: conversation
created: 2026-05-10
updated: 2026-06-19
tags: [psx, recomp, state-machine, input]
confidence: high
summary: "Tracks the discovery of the engine state machine variables, correcting previous state injection hacks in favor of native controller inputs."
volatility: warm
---

# Paranoiascape State Machine and Input Update (2026-05-10)

## TL;DR
The assumption that the game state is tracked by `0x80008580` (with 0x01 = Boot/Title, 0x08 = Main Menu, 0x00 = Stage 1) was false. This behavior was artificially induced by a forgotten hardcoded memory injection hack in `runtime.c` that intercepted inputs. After removing the hack, the memory at `0x80008580` naturally remains `0x00000000` during the Title Screen and Menu phases, confirming that the engine's true state machine resides elsewhere (likely inside the overlay at `0x80134xxx`).

## Discoveries

### 1. The Artificial State Injection Hack
During previous debug sessions, an intercept was placed in `runtime.c` at `0x80016940`. This intercept would poll the `debug_server` for overridden controller inputs and forcefully mutate `0x80008580`:
- Pressing START (`0x0008`) forced `0x80008580 = 0x08`
- Pressing CROSS (`0x4000`) forced `0x80008580 = 0x00`

This gave the illusion that `0x80008580` was the official game state variable and that the game "acknowledged" state changes. In reality, the game was simply ignoring this variable while the debug scripts incorrectly confirmed the state transition. 

### 2. True State of 0x80008580
With the hack removed, `0x80008580` remains `0x00000000` throughout the boot sequence, FMV skip, and Title Screen loop. It does not naturally transition to `0x08` when reaching the menu. This completely invalidates `0x80008580` as the target for our "state transition" injection efforts.

### 3. SIO Input Injection is Fully Functional
The SIO hardware pad emulation in `runtime.c` (`0x1F801040`) successfully receives overrides from the debug server. The `play_game.py` script effectively uses START (`0x0800`) to skip the initial `LOGO.STR` and `MOVTEST.STR` FMVs and arrive at the Title Screen (which renders at offset `160, 120`).

## Next Steps
Since we cannot simply write to a RAM address to force the game into Stage 1, we must trigger the transition naturally through the game's menu logic.

1. **Find the True Menu Inputs:** We must identify the exact sequence of controller inputs required to navigate from the Title Screen to the Stage 1 load trigger. (e.g., Does it require START to enter the menu, then UP/DOWN to select "Start Game", then CIRCLE or CROSS to confirm?)
2. **Emulator Ground Truth:** Launch Paranoiascape in PCSX-Redux, navigate the menus manually, and record the exact button sequence.
3. **Automate the Sequence:** Update `play_game.py` to send this exact sequence of inputs to natively progress the overlay loop out of its idle state and into the Stage 1 asset loader (`func_80019704`).
