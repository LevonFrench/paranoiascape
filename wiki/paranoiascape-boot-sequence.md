# Paranoiascape Boot Sequence Root Cause Analysis

## 1. The Load Address Hardcode Bug
The PSXRecomp runtime (`main_runner.cpp`) and the recompiler (`main_psx.cpp`) were originally hardcoded to use `0x80010000` as the binary load address, an artifact of the Tomba! project. 
However, Paranoiascape (`SLPS_013.75`) specifies a load address of `0x8000F000` in its PS-X EXE header. 

**Symptoms:**
- The runner shifted the loaded MIPS executable `0x1000` bytes into RAM.
- When the runner attempted to execute the entry point at `0x80012E2C`, it executed string literal data (e.g. `CdlR`) instead of MIPS instructions, causing `mips_interpret` to silently fail.

**Fix:**
- Updated `main_runner.cpp` to dynamically parse the load address from the header (offset `0x18`).
- Updated `call_by_address` in `runtime.c` to accept `kseg0 >= 0x8000F000u` instead of `0x80010000u`.

## 2. The BSS Loop Stack Overflow (Silent Crash)
After fixing the load address, the runner successfully executed MIPS instructions at `0x80012E2C`, but immediately crashed with `Exit Code 1` without printing a stack trace.

**Root Cause:**
- The entry point `0x80012E2C` is missing a standard MIPS prologue. The static recompiler (`SLPS_013.75_full.c`) erroneously absorbed it into an earlier function (`func_80012E24`).
- Because `0x80012E2C` is not a registered compiled function, `call_by_address` falls back to the MIPS interpreter.
- The entry point contains a memory clearing loop that wipes BSS memory from `0x800B6C68` to `0x8014E778`.
- In the compiled code (`block_80012E3C`), the `BNE` loop branch was generated as `call_by_address(cpu, 0x80012E3Cu);` because it was treated as an inter-block jump.
- `mips_interpret` hits the JAL/branch and recursively calls `call_by_address`, which recursively calls `mips_interpret`. Over 237,252 loop iterations, this blows out the native C stack, resulting in a silent crash.

## 3. Next Steps
To resolve this recursion, the static recompiler MUST be re-run with `0x80012E2C` specified as a forced entry point:
1. Update `recompiler/src/main_psx.cpp` to include `analyzer.add_forced_entry(0x80012E2Cu);`.
2. Remove any remaining `0x80010000` boundary checks in `recompiler/src/main_psx.cpp` that prevent `0x8000Fxxx` functions from being parsed.
3. Re-run `build.bat` to regenerate `SLPS_013.75_full.c` and `SLPS_013.75_dispatch.c`.
