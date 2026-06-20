---
title: "PSY-Z Porting Primitives"
category: concept
sources:
  - raw\notes\psyz-porting-guide-raw.md
created: 2026-06-19
updated: 2026-06-19
tags: [psx, recomp, sdk, primitives, ot]
confidence: high
summary: "Compatibility constructs for original PSY-Q GPU primitives on modern platforms, mapping Ordered Tables and custom tags structures."
volatility: warm
---

# PSY-Z Porting Primitives

PlayStation 1 applications targeting the PSY-Q SDK manage GPU primitives and ordering loops through direct memory pointer manipulations. Since pointer sizes vary on modern architectures (e.g. 64-bit systems), these operations will corrupt memory if compiled directly on PC.

## 1. Ordered Tables (OT) Compatibility
The PSY-Q SDK represents an Ordered Table as an array of 32-bit pointers:
```c
u_long ot[OTSIZE]; // PSY-Q: u_long is always 32-bit
```
On modern 64-bit PC architectures, pointers are 64-bit, meaning a `u_long` (which can be 64-bit depending on target and compiler) array would double in memory footprint. This causes out-of-bounds reads and pointer corruption when library functions walk the ordering table.

**PSY-Z Solution**: Replace all instances of `u_long*` used for ordered tables with `OT_TYPE*`.
```c
OT_TYPE ot[OTSIZE]; // PSY-Z: automatically scales correctly
```

## 2. Primitive Structure Tags
GPU primitives (such as `POLY_F3`, `POLY_GT4`, etc.) in `libgpu.h` begin with a 32-bit tag structure containing link pointers and command bytes:
```c
typedef struct {
    u_long tag; // PSY-Q original
    // primitive data...
} POLY_F3;
```
If your application defines custom GPU primitives or overlay structs that duplicate this layout, the leading member must be updated to use `O_TAG` instead of `u_long` to ensure compatibility with both 32-bit and 64-bit layouts:
```c
typedef struct {
    O_TAG tag; // PSY-Z compatible
    // primitive data...
} MyCustomPrimitive;
```

## See Also
- [[psyz-pointer-and-symbol-safety|PSY-Z Pointer and Symbol Safety]] ([PSY-Z Pointer and Symbol Safety](../concepts/psyz-pointer-and-symbol-safety.md))
- [[psyz-sdk-integration|PSY-Z SDK Integration]] ([PSY-Z SDK Integration](../topics/psyz-sdk-integration.md))
