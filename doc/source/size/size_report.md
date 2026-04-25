# QEMU Size And Memory Report

- Commit: `a98fa1b3`
- Date: 2026-04-25T09:34:25.516335
- Build target: `PORT=qemu CPU_ARCH=cortex-m0plus`
- Runtime target: `qemu-system-arm -machine mps2-an385 -cpu cortex-m3`
- Scope: `HelloBasic/*`, `HelloSimple`, `HelloPerformance`, `HelloShowcase`, `HelloStyleDemo`, `HelloVirtual(virtual_stage_showcase)`
- Static size scope: Map input sections from repo-side `src/` + `example/` objects only; exclude toolchain libraries, `driver/`, `porting/`; `ram_bytes` excludes `PFB`, and `pfb_bytes` still comes from each case-local `main.map` `.bss.pfb_area`.
- Successful apps: 68
- Failed apps: 0

## Overview

- Smallest Code: **HelloBasic(divider)** (12040 bytes)
- Largest Code: **HelloPerformance** (261292 bytes)
- Smallest RAM: **HelloBasic(thermostat)** (98 bytes)
- Largest RAM: **HelloBasic(svg)** (37765 bytes)
- Smallest Heap Peak: **HelloBasic(compass)** (0 bytes)
- Largest Heap Peak: **HelloBasic(svg)** (651081 bytes)
- Smallest Stack Peak: **HelloPerformance** (432 bytes)
- Largest Stack Peak: **HelloBasic(image)** (8192 bytes)

## Charts

![Size Report](images/size_report.png)

## Detailed Table

| App | Code | Resource | RAM | PFB | Heap Idle | Heap Init Peak | Heap Interaction Peak | Heap Peak | Stack Peak | Actions | Total ROM |
|-----|------|----------|-----|-----|-----------|----------------|-----------------------|-----------|------------|---------|-----------|
| HelloBasic(image) | 50692 | 133805 | 1760 | 4800 | 13715 | 14203 | 108473 | 108473 | 8192 | 5 | 184497 |
| HelloBasic(compass) | 31628 | 10394 | 236 | 4800 | 0 | 0 | 0 | 0 | 1240 | 1 | 42022 |
| HelloBasic(pattern_lock) | 33812 | 9535 | 425 | 4800 | 0 | 0 | 0 | 0 | 1168 | 5 | 43347 |
| HelloBasic(animated_image) | 31748 | 33527 | 180 | 4800 | 0 | 0 | 0 | 0 | 1056 | 1 | 65275 |
| HelloBasic(enhanced_widgets) | 62768 | 18509 | 1873 | 4800 | 0 | 0 | 0 | 0 | 1376 | 3 | 81277 |
| HelloBasic(combobox) | 29924 | 25126 | 649 | 4800 | 0 | 0 | 0 | 0 | 1248 | 13 | 55050 |
| HelloBasic(anim) | 24344 | 4018 | 532 | 4800 | 0 | 0 | 0 | 0 | 844 | 3 | 28362 |
| HelloBasic(thermostat) | 29056 | 46562 | 98 | 6400 | 0 | 0 | 0 | 0 | 1224 | 6 | 75618 |
| HelloBasic(activity_ring) | 36720 | 5723 | 244 | 4800 | 0 | 0 | 0 | 0 | 1252 | 1 | 42443 |
| HelloBasic(file_image) | 36572 | 12626 | 2196 | 4800 | 32 | 32 | 32 | 32 | 1096 | 1 | 49198 |
| HelloBasic(card) | 31012 | 9491 | 1056 | 4800 | 0 | 0 | 0 | 0 | 1168 | 1 | 40503 |
| HelloBasic(mp4) | 43688 | 3149174 | 1260 | 4800 | 0 | 0 | 488 | 488 | 1216 | 3 | 3192862 |
| HelloBasic(mask) | 84064 | 43714 | 564 | 4800 | 0 | 44 | 0 | 44 | 4088 | 1 | 127778 |
| HelloBasic(lyric_scroller) | 38688 | 9703 | 545 | 4800 | 0 | 0 | 0 | 0 | 1152 | 3 | 48391 |
| HelloBasic(textinput) | 39000 | 15678 | 3722 | 4800 | 0 | 0 | 0 | 0 | 1192 | 9 | 54678 |
| HelloBasic(svg) | 45960 | 14543 | 37765 | 4800 | 188156 | 495390 | 651081 | 651081 | 8192 | 7 | 60503 |
| HelloBasic(gauge) | 38784 | 6661 | 448 | 4800 | 0 | 0 | 0 | 0 | 1360 | 1 | 45445 |
| HelloBasic(divider) | 12040 | 363 | 332 | 4800 | 0 | 0 | 0 | 0 | 768 | 1 | 12403 |
| HelloBasic(analog_clock) | 25024 | 4559 | 244 | 4800 | 0 | 0 | 0 | 0 | 1080 | 1 | 29583 |
| HelloBasic(deferred_image) | 37324 | 19039 | 1557 | 4800 | 178 | 178 | 178 | 178 | 1096 | 3 | 56363 |
| HelloBasic(autocomplete) | 29716 | 9455 | 333 | 4800 | 0 | 0 | 0 | 0 | 1248 | 5 | 39171 |
| HelloBasic(textblock) | 36676 | 9410 | 212 | 4800 | 0 | 0 | 0 | 0 | 1104 | 1 | 46086 |
| HelloBasic(rotation) | 24084 | 9353 | 504 | 1600 | 0 | 0 | 0 | 0 | 1128 | 7 | 33437 |
| HelloVirtual(virtual_stage_showcase) | 187804 | 71977 | 8127 | 40960 | 2852 | 2852 | 2980 | 2980 | 2664 | 19 | 259781 |
| HelloPerformance | 261292 | 1834839 | 3017 | 1536 | 0 | 0 | 5060 | 5060 | 432 | 8 | 2096131 |
| HelloSimple | 19604 | 6796 | 264 | 4800 | 0 | 0 | 0 | 0 | 908 | 3 | 26400 |
| HelloStyleDemo | 125756 | 102219 | 5906 | 19200 | 0 | 0 | 0 | 0 | 2648 | 4 | 227975 |
| HelloShowcase | 164444 | 69833 | 14608 | 40960 | 0 | 0 | 0 | 0 | 2464 | 15 | 234277 |
| HelloBasic(button_img) | 35504 | 36910 | 98 | 4800 | 0 | 68 | 68 | 68 | 776 | 3 | 72414 |
| HelloBasic(segmented_control) | 30500 | 16772 | 481 | 4800 | 0 | 0 | 0 | 0 | 1256 | 9 | 47272 |
| HelloBasic(button) | 24716 | 17963 | 521 | 4800 | 0 | 0 | 0 | 0 | 1160 | 5 | 42679 |
| HelloBasic(arc_slider) | 28036 | 5217 | 417 | 4800 | 0 | 0 | 0 | 0 | 1268 | 5 | 33253 |
| HelloBasic(button_matrix) | 29448 | 16599 | 183 | 4800 | 0 | 0 | 0 | 0 | 1288 | 4 | 46047 |
| HelloBasic(checkbox) | 27120 | 16640 | 469 | 4800 | 0 | 0 | 0 | 0 | 1152 | 5 | 43760 |
| HelloBasic(digital_clock) | 30960 | 9191 | 196 | 4800 | 0 | 0 | 0 | 0 | 1056 | 1 | 40151 |
| HelloBasic(heart_rate) | 32468 | 20455 | 284 | 4800 | 0 | 0 | 0 | 0 | 2384 | 1 | 52923 |
| HelloBasic(gridlayout) | 26656 | 3929 | 428 | 4800 | 0 | 0 | 0 | 0 | 916 | 4 | 30585 |
| HelloBasic(label) | 19992 | 6015 | 380 | 4800 | 0 | 0 | 0 | 0 | 1096 | 1 | 26007 |
| HelloBasic(image_button) | 25108 | 25144 | 473 | 4800 | 0 | 0 | 0 | 0 | 1160 | 5 | 50252 |
| HelloBasic(led) | 18588 | 3576 | 444 | 4800 | 0 | 0 | 0 | 0 | 892 | 1 | 22164 |
| HelloBasic(line) | 18568 | 417 | 220 | 4800 | 0 | 0 | 0 | 0 | 1008 | 1 | 18985 |
| HelloBasic(linearlayout) | 30064 | 9233 | 228 | 4800 | 0 | 0 | 0 | 0 | 1096 | 1 | 39297 |
| HelloBasic(menu) | 20580 | 11091 | 121 | 4800 | 0 | 0 | 0 | 0 | 1144 | 5 | 31671 |
| HelloBasic(list) | 27440 | 14328 | 1734 | 4800 | 0 | 0 | 0 | 0 | 1104 | 6 | 41768 |
| HelloBasic(mini_calendar) | 29212 | 9379 | 127 | 4800 | 0 | 0 | 0 | 0 | 1144 | 3 | 38591 |
| HelloBasic(notification_badge) | 23520 | 13916 | 444 | 4800 | 0 | 0 | 0 | 0 | 1120 | 1 | 37436 |
| HelloBasic(number_picker) | 24656 | 21791 | 513 | 4800 | 0 | 0 | 0 | 0 | 1152 | 5 | 46447 |
| HelloBasic(progress_bar) | 18800 | 3574 | 364 | 4800 | 0 | 0 | 0 | 0 | 852 | 1 | 22374 |
| HelloBasic(page_indicator) | 33072 | 13077 | 520 | 4800 | 0 | 0 | 0 | 0 | 1192 | 4 | 46149 |
| HelloBasic(radio_button) | 27500 | 21399 | 497 | 4800 | 0 | 0 | 0 | 0 | 1144 | 5 | 48899 |
| HelloBasic(scale) | 22156 | 5866 | 172 | 4800 | 0 | 0 | 0 | 0 | 1144 | 1 | 28022 |
| HelloBasic(roller) | 20916 | 6201 | 453 | 4800 | 0 | 0 | 0 | 0 | 1128 | 5 | 27117 |
| HelloBasic(scroll) | 32016 | 9351 | 400 | 4800 | 0 | 0 | 0 | 0 | 1200 | 2 | 41367 |
| HelloBasic(spangroup) | 18836 | 5897 | 164 | 4800 | 0 | 0 | 0 | 0 | 1104 | 1 | 24733 |
| HelloBasic(spinner) | 26152 | 4679 | 444 | 4800 | 0 | 0 | 0 | 0 | 948 | 1 | 30831 |
| HelloBasic(slider) | 20584 | 3795 | 385 | 4800 | 0 | 0 | 0 | 0 | 932 | 5 | 24379 |
| HelloBasic(stopwatch) | 32016 | 9238 | 276 | 4800 | 0 | 0 | 0 | 0 | 1096 | 1 | 41254 |
| HelloBasic(switch) | 27396 | 16094 | 437 | 4800 | 0 | 0 | 0 | 0 | 1136 | 5 | 43490 |
| HelloBasic(tab_bar) | 33744 | 16693 | 544 | 4800 | 0 | 0 | 0 | 0 | 1192 | 4 | 50437 |
| HelloBasic(table) | 22752 | 5911 | 596 | 4800 | 0 | 0 | 0 | 0 | 1120 | 1 | 28663 |
| HelloBasic(tileview) | 29856 | 9434 | 444 | 4800 | 0 | 0 | 0 | 0 | 1096 | 4 | 39290 |
| HelloBasic(toggle_button) | 24456 | 17843 | 469 | 4800 | 0 | 0 | 0 | 0 | 1152 | 5 | 42299 |
| HelloBasic(viewpage) | 32064 | 9351 | 404 | 4800 | 0 | 0 | 0 | 0 | 1192 | 4 | 41415 |
| HelloBasic(viewpage_cache) | 31508 | 9400 | 180 | 4800 | 352 | 352 | 528 | 528 | 1176 | 4 | 40908 |
| HelloBasic(window) | 22604 | 14092 | 466 | 4800 | 0 | 0 | 0 | 0 | 1136 | 2 | 36696 |
| HelloBasic(circular_progress_bar) | 35064 | 10509 | 412 | 4800 | 0 | 0 | 0 | 0 | 1256 | 1 | 45573 |
| HelloBasic(chips) | 30404 | 14222 | 349 | 4800 | 0 | 0 | 0 | 0 | 1328 | 4 | 44626 |
| HelloBasic(stepper) | 25156 | 14201 | 385 | 4800 | 0 | 0 | 0 | 0 | 1168 | 6 | 39357 |