# QEMU Size And Memory Report

- Commit: `19e728f2`
- Date: 2026-04-26T18:24:28.117572
- Build target: `PORT=qemu CPU_ARCH=cortex-m0plus`
- Runtime target: `qemu-system-arm -machine mps2-an385 -cpu cortex-m3`
- Scope: `HelloBasic/*`, `HelloSimple`, `HelloPerformance`, `HelloShowcase`, `HelloStyleDemo`, `HelloVirtual(virtual_stage_showcase)`
- Static size scope: Map input sections from repo-side `src/` + `example/` objects only; exclude toolchain libraries, `driver/`, `porting/`; `ram_bytes` excludes `PFB`, and `pfb_bytes` still comes from each case-local `main.map` `.bss.pfb_area`.
- Successful apps: 67
- Failed apps: 0

## Overview

- Smallest Code: **HelloBasic(divider)** (11456 bytes)
- Largest Code: **HelloPerformance** (261660 bytes)
- Smallest RAM: **HelloBasic(button_img)** (98 bytes)
- Largest RAM: **HelloBasic(svg)** (37765 bytes)
- Smallest Heap Peak: **HelloBasic(activity_ring)** (0 bytes)
- Largest Heap Peak: **HelloBasic(svg)** (651355 bytes)
- Smallest Stack Peak: **HelloPerformance** (432 bytes)
- Largest Stack Peak: **HelloBasic(image)** (8192 bytes)

## Charts

![Size Report](images/size_report.png)

## Detailed Table

| App | Code | Resource | RAM | PFB | Heap Idle | Heap Init Peak | Heap Interaction Peak | Heap Peak | Stack Peak | Actions | Total ROM |
|-----|------|----------|-----|-----|-----------|----------------|-----------------------|-----------|------------|---------|-----------|
| HelloBasic(activity_ring) | 36136 | 5723 | 244 | 4800 | 0 | 0 | 0 | 0 | 1244 | 1 | 41859 |
| HelloBasic(analog_clock) | 24440 | 4559 | 244 | 4800 | 0 | 0 | 0 | 0 | 1072 | 1 | 28999 |
| HelloBasic(anim) | 23760 | 4018 | 532 | 4800 | 0 | 0 | 0 | 0 | 836 | 3 | 27778 |
| HelloBasic(animated_image) | 33116 | 33853 | 180 | 4800 | 0 | 276 | 0 | 276 | 1056 | 1 | 66969 |
| HelloBasic(arc_slider) | 27448 | 5217 | 417 | 4800 | 0 | 0 | 0 | 0 | 1260 | 5 | 32665 |
| HelloBasic(autocomplete) | 31148 | 9781 | 333 | 4800 | 0 | 276 | 276 | 276 | 1248 | 5 | 40929 |
| HelloBasic(button) | 26080 | 18289 | 521 | 4800 | 0 | 276 | 276 | 276 | 1168 | 5 | 44369 |
| HelloBasic(button_img) | 36884 | 37236 | 98 | 4800 | 0 | 68 | 68 | 68 | 748 | 3 | 74120 |
| HelloBasic(button_matrix) | 29860 | 16599 | 183 | 4800 | 0 | 276 | 276 | 276 | 1288 | 4 | 46459 |
| HelloBasic(card) | 32368 | 9817 | 1056 | 4800 | 0 | 276 | 0 | 276 | 1168 | 1 | 42185 |
| HelloBasic(checkbox) | 27532 | 16640 | 469 | 4800 | 0 | 276 | 276 | 276 | 1152 | 5 | 44172 |
| HelloBasic(chips) | 31756 | 14548 | 349 | 4800 | 0 | 276 | 276 | 276 | 1360 | 4 | 46304 |
| HelloBasic(circular_progress_bar) | 35476 | 10509 | 412 | 4800 | 0 | 276 | 0 | 276 | 1256 | 1 | 45985 |
| HelloBasic(combobox) | 30416 | 25126 | 649 | 4800 | 0 | 276 | 276 | 276 | 1248 | 13 | 55542 |
| HelloBasic(compass) | 32040 | 10394 | 236 | 4800 | 0 | 276 | 0 | 276 | 1240 | 1 | 42434 |
| HelloBasic(deferred_image) | 38676 | 19365 | 1557 | 4800 | 178 | 429 | 178 | 429 | 1096 | 3 | 58041 |
| HelloBasic(digital_clock) | 32312 | 9517 | 196 | 4800 | 0 | 276 | 0 | 276 | 1056 | 1 | 41829 |
| HelloBasic(divider) | 11456 | 363 | 332 | 4800 | 0 | 0 | 0 | 0 | 580 | 1 | 11819 |
| HelloBasic(enhanced_widgets) | 64188 | 18835 | 1873 | 4800 | 0 | 276 | 276 | 276 | 1368 | 3 | 83023 |
| HelloBasic(file_image) | 37924 | 12952 | 2196 | 4800 | 32 | 32 | 32 | 32 | 1096 | 1 | 50876 |
| HelloBasic(gauge) | 39200 | 6661 | 448 | 4800 | 0 | 276 | 0 | 276 | 1360 | 1 | 45861 |
| HelloBasic(gridlayout) | 27068 | 3929 | 428 | 4800 | 0 | 0 | 0 | 0 | 908 | 4 | 30997 |
| HelloBasic(heart_rate) | 32880 | 20455 | 284 | 4800 | 0 | 276 | 276 | 276 | 2360 | 1 | 53335 |
| HelloBasic(image) | 52052 | 134131 | 1760 | 4800 | 13715 | 14479 | 108749 | 108749 | 8192 | 5 | 186183 |
| HelloBasic(image_button) | 25520 | 25144 | 473 | 4800 | 0 | 276 | 276 | 276 | 1160 | 5 | 50664 |
| HelloBasic(label) | 21344 | 6341 | 380 | 4800 | 0 | 276 | 0 | 276 | 1096 | 1 | 27685 |
| HelloBasic(led) | 18004 | 3576 | 444 | 4800 | 0 | 0 | 0 | 0 | 884 | 1 | 21580 |
| HelloBasic(line) | 17984 | 417 | 220 | 4800 | 0 | 0 | 0 | 0 | 1000 | 1 | 18401 |
| HelloBasic(linearlayout) | 31416 | 9559 | 228 | 4800 | 0 | 276 | 0 | 276 | 1096 | 1 | 40975 |
| HelloBasic(list) | 28800 | 14654 | 1734 | 4800 | 0 | 276 | 276 | 276 | 1120 | 6 | 43454 |
| HelloBasic(lyric_scroller) | 40032 | 10029 | 545 | 4800 | 0 | 276 | 276 | 276 | 1152 | 3 | 50061 |
| HelloBasic(mask) | 84528 | 43714 | 564 | 4800 | 0 | 320 | 0 | 320 | 4080 | 1 | 128242 |
| HelloBasic(menu) | 21072 | 11091 | 121 | 4800 | 0 | 276 | 276 | 276 | 1144 | 5 | 32163 |
| HelloBasic(mini_calendar) | 29624 | 9379 | 127 | 4800 | 0 | 276 | 276 | 276 | 1144 | 3 | 39003 |
| HelloBasic(mp4) | 45040 | 3149500 | 1260 | 4800 | 0 | 0 | 764 | 764 | 1216 | 3 | 3194540 |
| HelloBasic(notification_badge) | 23932 | 13916 | 444 | 4800 | 0 | 276 | 0 | 276 | 1120 | 1 | 37848 |
| HelloBasic(number_picker) | 25068 | 21791 | 513 | 4800 | 0 | 276 | 276 | 276 | 1152 | 5 | 46859 |
| HelloBasic(page_indicator) | 34420 | 13403 | 520 | 4800 | 0 | 276 | 276 | 276 | 1192 | 4 | 47823 |
| HelloBasic(pattern_lock) | 35256 | 9861 | 425 | 4800 | 0 | 276 | 276 | 276 | 1176 | 5 | 45117 |
| HelloBasic(progress_bar) | 18216 | 3574 | 364 | 4800 | 0 | 0 | 0 | 0 | 844 | 1 | 21790 |
| HelloBasic(radio_button) | 27912 | 21399 | 497 | 4800 | 0 | 276 | 276 | 276 | 1144 | 5 | 49311 |
| HelloBasic(roller) | 21324 | 6201 | 453 | 4800 | 0 | 276 | 276 | 276 | 1128 | 5 | 27525 |
| HelloBasic(rotation) | 25652 | 9679 | 504 | 1600 | 0 | 276 | 276 | 276 | 1136 | 7 | 35331 |
| HelloBasic(scale) | 22568 | 5866 | 172 | 4800 | 0 | 276 | 0 | 276 | 1144 | 1 | 28434 |
| HelloBasic(scroll) | 33364 | 9677 | 400 | 4800 | 0 | 276 | 276 | 276 | 1200 | 2 | 43041 |
| HelloBasic(segmented_control) | 30992 | 16772 | 481 | 4800 | 0 | 276 | 276 | 276 | 1256 | 9 | 47764 |
| HelloBasic(slider) | 19996 | 3795 | 385 | 4800 | 0 | 0 | 0 | 0 | 924 | 5 | 23791 |
| HelloBasic(spangroup) | 19248 | 5897 | 164 | 4800 | 0 | 276 | 0 | 276 | 1104 | 1 | 25145 |
| HelloBasic(spinner) | 25568 | 4679 | 444 | 4800 | 0 | 0 | 0 | 0 | 940 | 1 | 30247 |
| HelloBasic(stepper) | 26520 | 14527 | 385 | 4800 | 0 | 276 | 276 | 276 | 1176 | 6 | 41047 |
| HelloBasic(stopwatch) | 33368 | 9564 | 276 | 4800 | 0 | 276 | 0 | 276 | 1096 | 1 | 42932 |
| HelloBasic(svg) | 47332 | 14869 | 37765 | 4800 | 188157 | 495390 | 651355 | 651355 | 8192 | 7 | 62201 |
| HelloBasic(switch) | 27808 | 16094 | 437 | 4800 | 0 | 276 | 276 | 276 | 1136 | 5 | 43902 |
| HelloBasic(tab_bar) | 35172 | 17019 | 544 | 4800 | 0 | 276 | 276 | 276 | 1192 | 4 | 52191 |
| HelloBasic(table) | 23164 | 5911 | 596 | 4800 | 0 | 276 | 0 | 276 | 1120 | 1 | 29075 |
| HelloBasic(textblock) | 37080 | 9410 | 212 | 4800 | 0 | 276 | 0 | 276 | 1104 | 1 | 46490 |
| HelloBasic(textinput) | 40624 | 16004 | 3722 | 4800 | 0 | 276 | 276 | 276 | 1200 | 9 | 56628 |
| HelloBasic(tileview) | 31208 | 9760 | 444 | 4800 | 0 | 276 | 276 | 276 | 1096 | 4 | 40968 |
| HelloBasic(toggle_button) | 24948 | 17843 | 469 | 4800 | 0 | 276 | 276 | 276 | 1152 | 5 | 42791 |
| HelloBasic(viewpage) | 33412 | 9677 | 404 | 4800 | 0 | 276 | 276 | 276 | 1192 | 4 | 43089 |
| HelloBasic(viewpage_cache) | 32856 | 9726 | 180 | 4800 | 352 | 628 | 804 | 804 | 1176 | 4 | 42582 |
| HelloBasic(window) | 23956 | 14418 | 466 | 4800 | 0 | 276 | 276 | 276 | 1136 | 2 | 38374 |
| HelloPerformance | 261660 | 1834798 | 3017 | 1536 | 0 | 0 | 5336 | 5336 | 432 | 8 | 2096458 |
| HelloShowcase | 166044 | 70159 | 14608 | 40960 | 0 | 276 | 276 | 276 | 2456 | 15 | 236203 |
| HelloSimple | 15136 | 2506 | 169 | 2400 | 0 | 0 | 0 | 0 | 820 | 3 | 17642 |
| HelloStyleDemo | 127352 | 102545 | 5906 | 19200 | 0 | 276 | 276 | 276 | 2640 | 4 | 229897 |
| HelloVirtual(virtual_stage_showcase) | 189440 | 72303 | 8127 | 40960 | 2852 | 3128 | 3256 | 3256 | 2656 | 19 | 261743 |