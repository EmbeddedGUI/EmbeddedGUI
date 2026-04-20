# QEMU Size And Memory Report

- Commit: `e82db1af`
- Date: 2026-04-20T14:21:23.810316
- Build target: `PORT=qemu CPU_ARCH=cortex-m0plus`
- Runtime target: `qemu-system-arm -machine mps2-an385 -cpu cortex-m3`
- Scope: `HelloBasic/*`, `HelloSimple`, `HelloPerformance`, `HelloShowcase`, `HelloStyleDemo`, `HelloVirtual(virtual_stage_showcase)`
- Static size scope: Map input sections from repo-side `src/` + `example/` objects only; exclude toolchain libraries, `driver/`, `porting/`; `ram_bytes` excludes `PFB`, and `pfb_bytes` still comes from each case-local `main.map` `.bss.pfb_area`.
- Successful apps: 68
- Failed apps: 0

## Overview

- Smallest Code: **HelloBasic(divider)** (12464 bytes)
- Largest Code: **HelloPerformance** (292604 bytes)
- Smallest RAM: **HelloBasic(thermostat)** (94 bytes)
- Largest RAM: **HelloBasic(svg)** (37761 bytes)
- Smallest Heap Peak: **HelloBasic(compass)** (0 bytes)
- Largest Heap Peak: **HelloPerformance** (5060 bytes)
- Smallest Stack Peak: **HelloPerformance** (432 bytes)
- Largest Stack Peak: **HelloBasic(svg)** (8192 bytes)

## Charts

![Size Report](images/size_report.png)

## Detailed Table

| App | Code | Resource | RAM | PFB | Heap Idle | Heap Init Peak | Heap Interaction Peak | Heap Peak | Stack Peak | Actions | Total ROM |
|-----|------|----------|-----|-----|-----------|----------------|-----------------------|-----------|------------|---------|-----------|
| HelloBasic(compass) | 32048 | 10449 | 232 | 4800 | 0 | 0 | 0 | 0 | 1224 | 1 | 42497 |
| HelloBasic(pattern_lock) | 34284 | 9628 | 421 | 4800 | 0 | 0 | 0 | 0 | 1152 | 5 | 43912 |
| HelloBasic(mp4) | 44060 | 3149250 | 1256 | 4800 | 0 | 0 | 488 | 488 | 1232 | 3 | 3193310 |
| HelloBasic(animated_image) | 32168 | 33634 | 176 | 4800 | 0 | 0 | 0 | 0 | 1040 | 1 | 65802 |
| HelloBasic(enhanced_widgets) | 63256 | 18819 | 1869 | 4800 | 0 | 0 | 0 | 0 | 1352 | 3 | 82075 |
| HelloBasic(combobox) | 30324 | 25182 | 645 | 4800 | 0 | 0 | 0 | 0 | 1232 | 13 | 55506 |
| HelloBasic(anim) | 24600 | 4082 | 504 | 4800 | 0 | 0 | 0 | 0 | 828 | 3 | 28682 |
| HelloBasic(activity_ring) | 37068 | 5782 | 240 | 4800 | 0 | 0 | 0 | 0 | 1236 | 1 | 42850 |
| HelloBasic(file_image) | 73092 | 17247 | 2341 | 4800 | 32 | 32 | 32 | 32 | 1080 | 1 | 90339 |
| HelloBasic(gauge) | 39212 | 6712 | 444 | 4800 | 0 | 0 | 0 | 0 | 1344 | 1 | 45924 |
| HelloBasic(card) | 31460 | 9561 | 1052 | 4800 | 0 | 0 | 0 | 0 | 1152 | 1 | 41021 |
| HelloBasic(thermostat) | 29432 | 46624 | 94 | 6400 | 0 | 0 | 0 | 0 | 1208 | 6 | 76056 |
| HelloBasic(mask) | 84436 | 43794 | 560 | 4800 | 0 | 44 | 0 | 44 | 4072 | 1 | 128230 |
| HelloBasic(svg) | 46416 | 14635 | 37761 | 4800 | 0 | 0 | 0 | 0 | 8192 | 7 | 61051 |
| HelloBasic(lyric_scroller) | 39060 | 9760 | 541 | 4800 | 0 | 0 | 0 | 0 | 1136 | 3 | 48820 |
| HelloBasic(textinput) | 38884 | 15706 | 3718 | 4800 | 0 | 0 | 0 | 0 | 1176 | 9 | 54590 |
| HelloBasic(image) | 51104 | 133897 | 1756 | 4800 | 0 | 488 | 596 | 596 | 8192 | 5 | 185001 |
| HelloBasic(analog_clock) | 25368 | 4617 | 240 | 4800 | 0 | 0 | 0 | 0 | 1032 | 1 | 29985 |
| HelloBasic(deferred_image) | 38524 | 19164 | 1553 | 4800 | 178 | 178 | 178 | 178 | 1080 | 3 | 57688 |
| HelloBasic(rotation) | 23844 | 9423 | 500 | 1600 | 0 | 0 | 0 | 0 | 1112 | 7 | 33267 |
| HelloBasic(autocomplete) | 30160 | 9552 | 329 | 4800 | 0 | 0 | 0 | 0 | 1232 | 5 | 39712 |
| HelloBasic(textblock) | 37052 | 9494 | 208 | 4800 | 0 | 0 | 0 | 0 | 1056 | 1 | 46546 |
| HelloVirtual(virtual_stage_showcase) | 187284 | 68162 | 8118 | 40960 | 2852 | 2852 | 2980 | 2980 | 2664 | 19 | 255446 |
| HelloPerformance | 292604 | 1838279 | 3162 | 1536 | 0 | 0 | 5060 | 5060 | 432 | 8 | 2130883 |
| HelloSimple | 19552 | 6785 | 260 | 4800 | 0 | 0 | 0 | 0 | 956 | 3 | 26337 |
| HelloStyleDemo | 126200 | 102550 | 5850 | 19200 | 0 | 0 | 0 | 0 | 2632 | 4 | 228750 |
| HelloShowcase | 165244 | 66056 | 14604 | 40960 | 0 | 0 | 0 | 0 | 2432 | 13 | 231300 |
| HelloBasic(button_img) | 35904 | 37007 | 94 | 4800 | 0 | 68 | 68 | 68 | 760 | 3 | 72911 |
| HelloBasic(segmented_control) | 30916 | 16829 | 477 | 4800 | 0 | 0 | 0 | 0 | 1272 | 9 | 47745 |
| HelloBasic(button) | 24988 | 18033 | 513 | 4800 | 0 | 0 | 0 | 0 | 1144 | 5 | 43021 |
| HelloBasic(arc_slider) | 28384 | 5275 | 413 | 4800 | 0 | 0 | 0 | 0 | 1252 | 5 | 33659 |
| HelloBasic(button_matrix) | 29856 | 16687 | 179 | 4800 | 0 | 0 | 0 | 0 | 1304 | 4 | 46543 |
| HelloBasic(checkbox) | 27372 | 16696 | 461 | 4800 | 0 | 0 | 0 | 0 | 1136 | 5 | 44068 |
| HelloBasic(digital_clock) | 31372 | 9295 | 192 | 4800 | 0 | 0 | 0 | 0 | 1040 | 1 | 40667 |
| HelloBasic(divider) | 12464 | 420 | 328 | 4800 | 0 | 0 | 0 | 0 | 752 | 1 | 12884 |
| HelloBasic(heart_rate) | 32824 | 20511 | 280 | 4800 | 0 | 0 | 0 | 0 | 2368 | 1 | 53335 |
| HelloBasic(gridlayout) | 27084 | 3983 | 424 | 4800 | 0 | 0 | 0 | 0 | 900 | 4 | 31067 |
| HelloBasic(label) | 20428 | 6070 | 376 | 4800 | 0 | 0 | 0 | 0 | 1080 | 1 | 26498 |
| HelloBasic(image_button) | 25380 | 25220 | 465 | 4800 | 0 | 0 | 0 | 0 | 1144 | 5 | 50600 |
| HelloBasic(led) | 18884 | 3627 | 440 | 4800 | 0 | 0 | 0 | 0 | 876 | 1 | 22511 |
| HelloBasic(line) | 18896 | 496 | 216 | 4800 | 0 | 0 | 0 | 0 | 984 | 1 | 19392 |
| HelloBasic(linearlayout) | 30492 | 9290 | 224 | 4800 | 0 | 0 | 0 | 0 | 1080 | 1 | 39782 |
| HelloBasic(menu) | 20968 | 11170 | 117 | 4800 | 0 | 0 | 0 | 0 | 1128 | 5 | 32138 |
| HelloBasic(list) | 27884 | 14432 | 1730 | 4800 | 0 | 0 | 0 | 0 | 1084 | 6 | 42316 |
| HelloBasic(mini_calendar) | 29600 | 9467 | 123 | 4800 | 0 | 0 | 0 | 0 | 1128 | 3 | 39067 |
| HelloBasic(notification_badge) | 23948 | 13978 | 440 | 4800 | 0 | 0 | 0 | 0 | 1104 | 1 | 37926 |
| HelloBasic(number_picker) | 25076 | 21852 | 509 | 4800 | 0 | 0 | 0 | 0 | 1136 | 5 | 46928 |
| HelloBasic(progress_bar) | 19144 | 3634 | 360 | 4800 | 0 | 0 | 0 | 0 | 836 | 1 | 22778 |
| HelloBasic(page_indicator) | 33508 | 13178 | 516 | 4800 | 0 | 0 | 0 | 0 | 1176 | 4 | 46686 |
| HelloBasic(radio_button) | 27608 | 21459 | 489 | 4800 | 0 | 0 | 0 | 0 | 1128 | 5 | 49067 |
| HelloBasic(scale) | 22556 | 5946 | 168 | 4800 | 0 | 0 | 0 | 0 | 1128 | 1 | 28502 |
| HelloBasic(roller) | 21336 | 6255 | 449 | 4800 | 0 | 0 | 0 | 0 | 1112 | 5 | 27591 |
| HelloBasic(scroll) | 32436 | 9425 | 396 | 4800 | 0 | 0 | 0 | 0 | 1184 | 2 | 41861 |
| HelloBasic(spangroup) | 19224 | 5981 | 160 | 4800 | 0 | 0 | 0 | 0 | 1088 | 1 | 25205 |
| HelloBasic(spinner) | 26436 | 4734 | 440 | 4800 | 0 | 0 | 0 | 0 | 932 | 1 | 31170 |
| HelloBasic(slider) | 20936 | 3849 | 381 | 4800 | 0 | 0 | 0 | 0 | 916 | 5 | 24785 |
| HelloBasic(stopwatch) | 32436 | 9313 | 272 | 4800 | 0 | 0 | 0 | 0 | 1080 | 1 | 41749 |
| HelloBasic(switch) | 27640 | 16148 | 429 | 4800 | 0 | 0 | 0 | 0 | 1120 | 5 | 43788 |
| HelloBasic(tab_bar) | 34188 | 16781 | 540 | 4800 | 0 | 0 | 0 | 0 | 1176 | 4 | 50969 |
| HelloBasic(table) | 23140 | 5991 | 592 | 4800 | 0 | 0 | 0 | 0 | 1104 | 1 | 29131 |
| HelloBasic(tileview) | 30300 | 9487 | 440 | 4800 | 0 | 0 | 0 | 0 | 1080 | 4 | 39787 |
| HelloBasic(toggle_button) | 24716 | 17904 | 461 | 4800 | 0 | 0 | 0 | 0 | 1136 | 5 | 42620 |
| HelloBasic(viewpage) | 32488 | 9427 | 400 | 4800 | 0 | 0 | 0 | 0 | 1176 | 4 | 41915 |
| HelloBasic(viewpage_cache) | 31408 | 9397 | 176 | 4800 | 352 | 352 | 528 | 528 | 1160 | 4 | 40805 |
| HelloBasic(window) | 23004 | 14137 | 461 | 4800 | 0 | 0 | 0 | 0 | 1120 | 2 | 37141 |
| HelloBasic(circular_progress_bar) | 35480 | 10578 | 408 | 4800 | 0 | 0 | 0 | 0 | 1256 | 1 | 46058 |
| HelloBasic(chips) | 30864 | 14317 | 345 | 4800 | 0 | 0 | 0 | 0 | 1312 | 4 | 45181 |
| HelloBasic(stepper) | 25640 | 14310 | 381 | 4800 | 0 | 0 | 0 | 0 | 1152 | 6 | 39950 |