---
title: "Memory Map"
category: reference
sources: []
compiled-from: conversation
created: 2026-05-01
updated: 2026-06-19
tags: [psx, recomp, memory, map]
confidence: high
summary: "Comprehensive reference of internal state variables, global variables, hardware MMIO registers, and recompiled MIPS function intercepts for Paranoiascape."
volatility: warm
---

# Paranoiascape Memory Address Map

This wiki maps out key RAM addresses, overlay states, global variables, hardware MMIO registers, and recompiled MIPS function intercepts.

---

## 1. Internal & Overlay State Machine Variables

| RAM Address | Variable / Name | Values & Meaning |
|-------------|-----------------|------------------|
| `0x8013DC60` | Primary Overlay State (`osm`) | Tracks overlay state machine:<br>• `1`: Boot Logo / Intro Movie playback<br>• `15`: Title Screen / Pre-Menu State<br>• `3`: Main Menu<br>• `7`: Transitioning to New Game |
| `0x801349D4` | Secondary State | Changes from `5` to `4` during transition to New Game. |
| `0x80008580` | Secondary Menu State | Tracked during menu navigation to start gameplay. |
| `0x8009E150 + 1104` | `gp + 1104` | Stage state machine variable. |
| `0x8009E150 + 1100` | `gp + 1100` | Countdown timer (init=1800, decrements per frame). |
| `0x8009E150 + 852` | `gp + 852` | Button state variable checked for stage/level transitions. |

---

## 2. CD-ROM Loading & FMV Sync Data

| RAM Address | Name / Purpose | Details |
|-------------|----------------|---------|
| `0x80091FB4` | Direct CD-ROM File Table | Table of 12-byte entries parsed by `CdInit`:<br>• `offset 0`: `name_ptr` (pointer to string)<br>• `offset 4`: `lba` (sector pos)<br>• `offset 8`: `size` (bytes)<br>Matches `LOGO.STR`, `MOVTEST.STR`, `PARAFNMV.STR`, and `SMG.STR`. |
| `0x800A5560` | Pointer to GPUSTAT | Holds register address `0x1F801814`. |
| `0x800A5578` | Pointer to OTC DMA | Holds register address `0x1F8010E8`. |
| `0x800A5594` | FMV Exit VSync Target | Holds VSync target value (start counter + 240) representing maximum loop duration. |
| `0x800A5598` | FMV Frame Counter | Incrementing frame tick counter checked by `func_800804F4`. |

---

## 3. Input Buffers

| RAM Address | Name / Purpose | Details |
|-------------|----------------|---------|
| `0x8013E5EC` | SIO Pad Polling Buffer | 6-byte buffer updated by hardware SIO bit-bang loops:<br>• Bytes 0-1: pad state (active-low)<br>• Bytes 2-3: pressed buttons edge-trigger<br>• Bytes 4-5: released buttons edge-trigger |

---

## 4. Audio & Math Precomputations

| RAM Address | Name / Purpose | Details |
|-------------|----------------|---------|
| `0x8009FC38` | precomputed sin/cos BSS table | 4096 entries (16KB) constructed during InitGeom. Stored as packed `{cos(high16), sin(low16)}` in 4.12 fixed-point. |
| `0x800A0438` | precomputed master sin/cos table | Precompiled table in `.rodata`. |
| `0x80137300` | SDK Default Coordinate Matrix | 32-byte global identity matrix (3x3 rotation scale + translation) used by SDK projection. Initialized to identity by recomp runner. |

---

## 5. Hardware MMIO Registers

| Register Address | Name | Role in Paranoiascape |
|------------------|------|-----------------------|
| `0x1F801040` | SIO0 TX/RX data register | Directly bit-banged for controller polling. |
| `0x1F8010E8` | OTC DMA Channel Control | Polled for bit 24 (Start/Busy flag) to wait for OTC clear. |
| `0x1F801814` | GPUSTAT Status register | Read to check DMA/command execution readiness. |

---

## 6. MIPS Function Intercept Table

| Address | Function | Purpose |
|---------|----------|---------|
| `0x800804C0` | `func_800804C0` | FMV key skip check function (intercepted to force skip logic on START/CROSS). |
| `0x800804F4` | `func_800804F4` | FMV/MDEC display loop frame check (monitors elapsed frames and VSync). |
| `0x8007F754` | `func_8007F754` | Gameplay FMV player function. |
| `0x8007F444` | `func_8007F444` | Boot logo FMV player function. |
| `0x8001E468` | `func_8001E468` | Secondary loader initiating CD streaming decode for STR files. |
| `0x80051434` | `func_80051434` | VBlank interrupt callback handler. |
| `0x80050B14`<br>`0x80050B04` | `VSync` | Busy-wait frame pace function. Intercepted to drive free-running ticks, present frames, and run background async loops. |
| `0x800197C8` | `func_800197C8` | LoadTask CD queue reader (copies sector data to RAM buffer). |
| `0x80019704` | `func_80019704` | Primary CD load orchestrator. |
