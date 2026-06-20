---
title: "Input Automation Sio"
category: concept
sources: []
compiled-from: conversation
created: 2026-05-10
updated: 2026-06-19
tags: [psx, recomp, input, sio]
confidence: high
summary: "Detailed analysis of the SIO pad polling logic, active-low edge triggers, and the button sequence needed to bypass the Main Menu overlay."
volatility: warm
---

# Paranoiascape Input Automation & SIO Emulation

## The SIO Emulation Problem
Paranoiascape ignores standard BIOS `PadRead` memory intercepts for its main menu navigation, and instead heavily relies on polling the Joypad RX hardware register directly (`0x1F801040`) via `FUN_8008d60c`.

When attempting to emulate the SIO hardware behavior in `runtime.c`, we encountered a blocking loop: the game sends `0x01`, expects `ACK` (DSR), sends `0x42`, and continues. However, because our static SIO intercept returned an infinite `ACK` (Bit 7 set to 1 in `1F801044` STAT), the game's SPI state machine timed out, assuming the controller was broken. This timeout caused `FUN_8008d60c` to return `0x0000` (no buttons pressed), wiping out any manual pad injection attempts.

## The Solution
Instead of implementing full, cycle-accurate SIO interrupts and DSR toggles, the most reliable way to inject input is to bypass the native SIO polling code entirely:

1. **Intercept `0x8008d60c`**: Hook the polling function in `psx_override_dispatch`.
2. **Return 1**: By returning `1` from the intercept, we skip the execution of the native SIO code. This prevents the game from executing its timeout loop and returning a blank `0000` pad state.
3. **Manual Injection (`inject_pad_state`)**: We manually write our desired pad bytes directly into the game's high-level controller buffers:
    - `0x13E5EC` - `pad`
    - `0x13E5EE` - `pressed` (Edge Trigger)
    - `0x13E5F0` - `released` (Edge Trigger)

## The Active-Low Edge Trigger Trap
Through manual testing with PCSX-Redux, it was discovered that injecting `F7FFF7FF` (`pad = 0xFFF7`, `pressed = 0xFFF7`) to `0x8013E5EC` successfully forced a menu transition.

**Crucial Finding**: Paranoiascape expects **BOTH** the `pad` AND the `pressed` (edge trigger) buffers to be **Active LOW**. 
If you calculate `pressed` as an active HIGH mask (e.g., `0x0008` for START) and inject it, the game will ignore the input entirely because `0x0000 & 0x0008 = 0`.

The correct calculation for injection is:
```c
uint16_t pad = ~(uint16_t)override;
uint16_t pressed = ~(~pad & s_last_pad); /* Active LOW! */
```
For an override like START (`0x0008`), `pad` becomes `0xFFF7` and `pressed` becomes `0xFFF7`.

## SIO Halfword Access Bug & Title Screen Gating

During the boot FMV and Title Screen sequence, the game state machine does not query `0x8008D60C`. Instead, it uses a low-level SIO polling routine (`func_8008D330`) which queries the Joypad RX register (`0x1F801040`) using **halfword read instructions** (`LH`/`LHU`).

In our recompiler's runtime memory map, `read_byte` was previously the only place mocking SIO register reads from `0x1F801040`. Consequently, `read_half(0x1F801040)` would return `0x0000` (representing disconnected hardware). The Title Screen menu loop would thus believe no controller was connected and completely ignore inputs.

We solved this by adding SIO data mocking into `read_half` inside `runner/src/runtime.c`:
```c
else if (phys == 0x1F801040u) {
    uint16_t pad = ~g_pad1_state;
    extern int debug_server_get_input_override(void);
    int override = debug_server_get_input_override();
    if (override != -1) {
        pad = ~(uint16_t)override;
    }

    if (s_sio_state == 0) val = 0xFF;
    else if (s_sio_state == 1) val = 0xFF;
    else if (s_sio_state == 2) val = 0x41;
    else if (s_sio_state == 3) val = 0x5A;
    else if (s_sio_state == 4) val = (uint8_t)(pad & 0xFF);
    else if (s_sio_state == 5) val = (uint8_t)((pad >> 8) & 0xFF);
    else val = 0xFF;
}
```

## Automation Sequence
With the recompiler pad injection properly configured as Active LOW and SIO reads fully mocked in both `read_byte` and `read_half`, the automation script `play_game.py` correctly drives the transition from the Title Screen into Stage 1 gameplay.

The correct Japanese region button sequence is:
1. `START` (Proceed past title)
2. `CIRCLE` ("New Game")
3. `DOWN` (Navigate)
4. `CIRCLE` ("Options" / Confirm)
5. `CROSS` (Back/Cancel)

Using `play_game.py` with the debug server input overrides now correctly bypasses all menus and loads the stage assets successfully.
