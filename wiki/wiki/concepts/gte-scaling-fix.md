---
title: "GTE Register Scaling Fix"
category: concept
sources: []
compiled-from: conversation
created: 2026-06-19
updated: 2026-06-19
tags: [gte, rendering, bugfix]
confidence: high
summary: "Detailed explanation of GTE register scaling mismatch and fix for the 3D projection collapse in Paranoiascape."
volatility: cold
---

# GTE Register Scaling Fix

This document records the GTE emulator register scaling mismatch that caused 3D projection to collapse all vertex coordinates to a single point at the center of the screen, rendering the 3D corridor invisible.

## Problem Description

In the recompiled runner's GTE emulator ([gte.cpp](file:///j:/projects/paranoia/paranoiascape-recomp/runner/src/gte.cpp)), the data registers `IR1..3` (regs 9-11) and `MAC1..3` (regs 25-27) were internally stored and used in unscaled form (0 fractional bits, e.g., representing $1.0$ as $1$).

However, the guest MIPS CPU expects these values to follow the hardware GTE spec, where `IR1..3` are `s3.12` signed fixed-point values (scaled by $4096$, e.g., representing $1.0$ as $4096$).

This discrepancy had two critical side-effects:
1. When the guest MIPS CPU wrote coordinates or values to `IR1..3` or `MAC1..3` via `mtc2`, it wrote them scaled by $4096$. The GTE emulator then performed calculations using these numbers as if they were unscaled (making them 4096 times larger than expected).
2. The GTE control registers `TR` (translation), `BK` (background color), and `FC` (far color) are written by the guest CPU already scaled by $4096$. The GTE emulator double-shifted them left by 12 (`<< 12`), making them 4096 times larger yet again.

As a result, translation was scaled by $2^{24}$, whereas rotated vertex coordinates were only scaled by $2^{12}$ (or unscaled), causing translation to completely dominate the projection formulas, collapsing all projected vertices `SXY` to the viewport center offset.

## Resolution

We modified [gte.cpp](file:///j:/projects/paranoia/paranoiascape-recomp/runner/src/gte.cpp) to enforce consistent scaling boundaries:

### 1. Inbound Register Writes (`gte_mtc2`)
Scale down GTE data registers written by the MIPS CPU:
- **`IR1..3`**: `gte->IR1 = static_cast<int16_t>(static_cast<int16_t>(value & 0xFFFF) >> 12);`
- **`MAC1..3`**: `gte->MAC1 = static_cast<int32_t>(value) >> 12;`

### 2. Outbound Register Reads (`gte_mfc2`)
Scale up values read by the MIPS CPU:
- **`IR1..3`**: `return static_cast<int32_t>(gte->IR1) << 12;`
- **`MAC1..3`**: `return gte->MAC1 << 12;`

### 3. Removal of Double Shifts on Control Registers
Removed `<< 12` shifts from `gte->TR`, `gte->BK`, and `gte->FC` values inside:
- `gte_rtps_internal`
- `light_color`
- `gte_mvmva`

### 4. Far Color Interpolation Correct scaling
Adjusted `depth_cue_from_ir` to subtract `IR` scaled to 12-bit fixed point:
`int64_t mac1 = (int64_t)gte->FC[0] - ((int64_t)gte->IR1 << 12);`

## Verification Results

After compiling, the runner successfully navigated past the title screen and loaded Stage 1 gameplay. Milestone screenshots (`auto_f3000.png`, `auto_f5000.png`, and `auto_f10000.png`) contain highly detailed 3D rendering (over 70,000 to 110,000 non-zero decompressed bytes), confirming that the geometry projection is correctly rendering the 3D corridor.
