# QEMU Size And Memory Report

- Commit: `e0b6ff8`
- Date: 2026-04-05T18:02:35.777092
- Build target: `PORT=qemu CPU_ARCH=cortex-m0plus`
- Runtime target: `qemu-system-arm -machine mps2-an385 -cpu cortex-m3`
- Scope: `HelloBasic(button,image,label)`, `HelloSimple`, `HelloShowcase`, `HelloVirtual(virtual_stage_showcase)`
- Static size scope: Map input sections from repo-side `src/` + `example/` objects only; exclude toolchain libraries, `driver/`, `porting/`; `ram_bytes` excludes `PFB`, and `pfb_bytes` still comes from `.bss.pfb_area`.
- Successful apps: 6
- Failed apps: 0

## Overview

- Smallest Code: **HelloBasic(label)** (17180 bytes)
- Largest Code: **HelloVirtual(virtual_stage_showcase)** (178356 bytes)
- Smallest RAM: **HelloSimple** (1049 bytes)
- Largest RAM: **HelloShowcase** (14897 bytes)
- Smallest Heap Peak: **HelloBasic(button)** (0 bytes)
- Largest Heap Peak: **HelloVirtual(virtual_stage_showcase)** (2872 bytes)
- Smallest Stack Peak: **HelloSimple** (788 bytes)
- Largest Stack Peak: **HelloVirtual(virtual_stage_showcase)** (2608 bytes)

## Charts

![Size Report](images/size_report.png)

## Detailed Table

| App | Code | Resource | RAM | PFB | Heap Idle | Heap Init Peak | Heap Interaction Peak | Heap Peak | Stack Peak | Actions | Total ROM |
|-----|------|----------|-----|-----|-----------|----------------|-----------------------|-----------|------------|---------|-----------|
| HelloBasic(button) | 22744 | 17956 | 1294 | 4800 | 0 | 0 | 0 | 0 | 1040 | 5 | 40700 |
| HelloBasic(image) | 45212 | 72176 | 2003 | 4800 | 824 | 1724 | 1804 | 1804 | 1080 | 3 | 117388 |
| HelloBasic(label) | 17180 | 5993 | 1153 | 4800 | 0 | 0 | 0 | 0 | 968 | 1 | 23173 |
| HelloSimple | 18236 | 6708 | 1049 | 4800 | 0 | 0 | 0 | 0 | 788 | 3 | 24944 |
| HelloShowcase | 158260 | 64858 | 14897 | 40960 | 0 | 0 | 0 | 0 | 2400 | 13 | 223118 |
| HelloVirtual(virtual_stage_showcase) | 178356 | 66846 | 8819 | 40960 | 2756 | 2756 | 2872 | 2872 | 2608 | 19 | 245202 |