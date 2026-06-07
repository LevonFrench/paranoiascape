# IRQ Chain Dispatch & the CD-Sync Boot Block

Status as of 2026-05-07. The recompiled binary boots through entry init but stalls in a CD-ROM polling spin before reaching the title screen.

## Boot trace summary

`run_recomp.bat` produces ~48,000 `[FRAME]` events in 8s with only **51** `[CALL]` events. Boot reaches:

- Entry point `0x80012E2C` runs (forced entry — see [boot-sequence.md](./paranoiascape-boot-sequence.md))
- BSS clear, runtime setup, CdInit calls (overridden — return success)
- `ResetGraph`, GP1 display mode `0x000001` (320×240, **not** 24-bit)
- `SysEnqIntRP` registers one chain[2] handler — queue `0x8013E4D0`, handler `func_8008D0A0`, verifier at `0x8008D038`
- DMA OT processing (display list ran once)
- Final CdInit (CALL #50)
- **Stall**: `func_80052800` (CdReadyResult) spins at `block_80052888` polling `[0x80134B40]` and CD-ROM status byte at `[*0x8009F624]`

## Why it stalls

`fire_interrupt_chain()` was defined in `runtime.c` but **never called from anywhere** — vestigial helper from the Tomba port. The game's chain[2] handler can't fire without it. The handler would normally update CD result state at `0x80134B40` and the spin would exit.

Compounding: even with chain firing, the verifier `func_8008D040` returns 0 because `[0x800B6AB8]` (the CD ISR function pointer) is null at this point. The game installs that pointer **later** in boot, but boot is stuck on this spin → chicken-and-egg.

## Fix applied

`runner/src/runtime.c`:

- Replaced dead `fire_interrupt_chain` with **`psx_dispatch_int_chain(cpu, priority)`** following standard PsyQ convention:
  - Walks chain[priority], for each entry sets `cpu->v1 = queue` and calls verifier at `queue+8`
  - Runs handler at `queue+4` only if verifier returned 1
  - Re-entry guard via static `g_in_irq_dispatch` flag
- Hooked into the two CD-sync `psx_present_frame()` intercept sites: `0x80050B04`, `0x80050B14`

This is architecturally correct and required for any real CD-IRQ emulation. By itself it does **not** unblock boot (verifier returns 0 today).

## Three paths to actually unblock the title screen

In order of correctness vs. effort:

### 1. Proper CD-ROM IRQ emulation (right per AGENTS.md Rule 0g)

In `runtime.c`'s write_byte handler for `0x1F801801`:
- Implement command-response state machine for CdlGetID/CdlGetType/CdlInit/CdlSetloc/CdlReadN/CdlReadS
- Generate INT3 (acknowledge) then INT2 (complete) responses
- Update response FIFO bytes at `[0x1F801801]` so `func_800522A4` (which reads `*[0x8009F624]`) sees real index/status bits change
- Once IRQs are wired, `psx_dispatch_int_chain(cpu, 2)` will fire the verifier, which dispatches the game's CD ISR via `[0x800B6AB8]`, which updates `0x80134B40`, which exits the spin

Several hundred lines but it's the real answer.

### 2. Hook the high-level CD caller (medium effort, semi-clean)

`func_80052800` has 4 callers in `generated/SLPS_013.75_full.c`: lines **135681**, **136235**, **138124**, **139052**. Identify which one runs at boot (probably an overlay loader called from `func_80012EE8`'s main loop) and add a runtime override at that address — similar to existing overrides for `CdSearchFile` (`0x80053A04`) and `LoadFile` (`0x8001E444`).

### 3. Bypass the CD-sync spins directly (violates Rule 0g — diagnostic only)

Add overrides for `func_80052800`, `func_80052AE4`, `func_80052F1C`, `func_80053618` that return success without spinning. Useful only to discover what fails next; not a real fix.

## Key addresses

| Address | Meaning |
|---------|---------|
| `0x80012E2C` | EXE entry point (forced — no prologue) |
| `0x80012EE8` | Main game loop (infinite — `block_800130C4` is unreachable, by design) |
| `0x80050EA8` | CdInit (currently overridden, returns 1) |
| `0x80050B04` | CD-sync stub #1 (intercepted, calls present_frame + chain dispatch) |
| `0x80050B14` | CD-sync stub #2 (intercepted) |
| `0x80050E68` | CD-status stub (intercepted, returns 1) |
| `0x80052800` | CdReadyResult — **the spin** |
| `0x80052AE4`, `0x80052F1C`, `0x80053618` | Other CD-sync wait functions (same pattern) |
| `0x800522A4` | CD register reader, polls index bits (uses `*[0x8009F624]`) |
| `0x80134B40` | Game's CD-result deadline counter |
| `0x80134B44` | Game's CD-spin local counter (limit `0x003C0000`) |
| `0x8009F618` | Pointer to CD-ROM status register (game maps this to `0x1F801800`) |
| `0x8009F624` | Pointer to CD-ROM response register |
| `0x800B6AB8` | CD ISR function pointer (NULL at boot — installed later) |
| `0x800B6AF0` | Pointer used by `func_8008D040` verifier |
| `0x8013E4D0` | chain[2] queue head |
| `0x8013E4E0` | Periodic counter incremented by `func_8008D0A0` (caps at 150) |
| `0x8013E5EC` | Pad buffer (active-low, little-endian — see input-graphics.md) |

## Diagnostic technique

To find the spin location, temporarily add to the read_word heartbeat (every 100K reads):

```c
uint32_t ra = g_diag_cpu ? g_diag_cpu->ra : 0;
printf("[SPIN-DIAG] reads=%llu ra=0x%08X last_addr=0x%08X chain[0..3]={0x%08X,0x%08X,0x%08X,0x%08X}\n",
       (unsigned long long)s_rw_count, ra, addr,
       g_int_chains[0], g_int_chains[1], g_int_chains[2], g_int_chains[3]);
```

The RA value identifies the function we're spinning inside, `last_addr` shows the polled address, and `g_int_chains` shows which IRQ priorities the game has registered handlers for. Don't ship this — it logs in a hot path.
