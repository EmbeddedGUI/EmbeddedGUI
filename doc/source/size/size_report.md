# QEMU Size And Memory Report

- Commit: `80ac987`
- Date: 2026-04-06T21:19:56.243500
- Build target: `PORT=qemu CPU_ARCH=cortex-m0plus`
- Runtime target: `qemu-system-arm -machine mps2-an385 -cpu cortex-m3`
- Scope: `HelloBasic(button,image,label)`, `HelloSimple`, `HelloShowcase`, `HelloVirtual(virtual_stage_showcase)`
- Static size scope: Map input sections from repo-side `src/` + `example/` objects only; exclude toolchain libraries, `driver/`, `porting/`; `ram_bytes` excludes `PFB`, and `pfb_bytes` still comes from `.bss.pfb_area`.
- Successful apps: 6
- Failed apps: 0

## Overview

- Smallest Code: **HelloBasic(label)** (16352 bytes)
- Largest Code: **HelloVirtual(virtual_stage_showcase)** (165848 bytes)
- Smallest RAM: **HelloSimple** (1049 bytes)
- Largest RAM: **HelloShowcase** (14945 bytes)
- Smallest Heap Peak: **HelloBasic(button)** (0 bytes)
- Largest Heap Peak: **HelloVirtual(virtual_stage_showcase)** (2872 bytes)
- Smallest Stack Peak: **HelloSimple** (820 bytes)
- Largest Stack Peak: **HelloVirtual(virtual_stage_showcase)** (2600 bytes)

## Charts

![Size Report](images/size_report.png)

## Detailed Table

| App | Code | Resource | RAM | PFB | Heap Idle | Heap Init Peak | Heap Interaction Peak | Heap Peak | Stack Peak | Actions | Total ROM |
|-----|------|----------|-----|-----|-----------|----------------|-----------------------|-----------|------------|---------|-----------|
| HelloBasic(button) | 20608 | 17956 | 1294 | 4800 | 0 | 0 | 0 | 0 | 1048 | 5 | 38564 |
| HelloBasic(image) | 42164 | 72176 | 2003 | 4800 | 824 | 1724 | 1804 | 1804 | 1088 | 3 | 114340 |
| HelloBasic(label) | 16352 | 5993 | 1153 | 4800 | 0 | 0 | 0 | 0 | 976 | 1 | 22345 |
| HelloSimple | 16480 | 6708 | 1049 | 4800 | 0 | 0 | 0 | 0 | 820 | 3 | 23188 |
| HelloShowcase | 145752 | 65222 | 14945 | 40960 | 0 | 0 | 0 | 0 | 2408 | 13 | 210974 |
| HelloVirtual(virtual_stage_showcase) | 165848 | 67210 | 8819 | 40960 | 2756 | 2756 | 2872 | 2872 | 2600 | 19 | 233058 |