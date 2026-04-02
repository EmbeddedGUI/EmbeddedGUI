# QEMU Size And Memory Report

- Commit: `587c274`
- Date: 2026-04-03T00:29:23.579139
- Build target: `PORT=qemu CPU_ARCH=cortex-m0plus`
- Runtime target: `qemu-system-arm -machine mps2-an385 -cpu cortex-m3`
- Scope: `HelloBasic(button,image,label)`, `HelloSimple`, `HelloShowcase`, `HelloVirtual(virtual_stage_showcase)`
- Static size scope: Map input sections from repo-side `src/` + `example/` objects only; exclude toolchain libraries, `driver/`, `porting/`; `ram_bytes` excludes `PFB`, and `pfb_bytes` still comes from `.bss.pfb_area`.
- Successful apps: 6
- Failed apps: 0

## Overview

- Smallest Code: **HelloBasic(label)** (16876 bytes)
- Largest Code: **HelloVirtual(virtual_stage_showcase)** (178084 bytes)
- Smallest RAM: **HelloBasic(label)** (1973 bytes)
- Largest RAM: **HelloShowcase** (18929 bytes)
- Smallest Heap Peak: **HelloBasic(button)** (0 bytes)
- Largest Heap Peak: **HelloVirtual(virtual_stage_showcase)** (2872 bytes)
- Smallest Stack Peak: **HelloSimple** (788 bytes)
- Largest Stack Peak: **HelloVirtual(virtual_stage_showcase)** (2608 bytes)

## Charts

![Size Report](images/size_report.png)

## Detailed Table

| App | Code | Resource | RAM | PFB | Heap Idle | Heap Init Peak | Heap Interaction Peak | Heap Peak | Stack Peak | Actions | Total ROM |
|-----|------|----------|-----|-----|-----------|----------------|-----------------------|-----------|------------|---------|-----------|
| HelloBasic(button) | 22440 | 17956 | 2378 | 4800 | 0 | 0 | 0 | 0 | 1040 | 5 | 40396 |
| HelloBasic(image) | 44480 | 72176 | 3434 | 4800 | 900 | 900 | 980 | 980 | 1080 | 3 | 116656 |
| HelloBasic(label) | 16876 | 5993 | 1973 | 4800 | 0 | 0 | 0 | 0 | 968 | 1 | 22869 |
| HelloSimple | 17932 | 6708 | 2001 | 4800 | 0 | 0 | 0 | 0 | 788 | 3 | 24640 |
| HelloShowcase | 157988 | 64858 | 18929 | 40960 | 0 | 0 | 0 | 0 | 2400 | 13 | 222846 |
| HelloVirtual(virtual_stage_showcase) | 178084 | 66846 | 12939 | 40960 | 2756 | 2756 | 2872 | 2872 | 2608 | 19 | 244930 |