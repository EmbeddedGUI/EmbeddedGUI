# QEMU Size And Memory Report

- Commit: `b1c1a9d`
- Date: 2026-04-19T04:30:05.130322
- Build target: `PORT=qemu CPU_ARCH=cortex-m0plus`
- Runtime target: `qemu-system-arm -machine mps2-an385 -cpu cortex-m3`
- Scope: `HelloBasic/*`, `HelloSimple`, `HelloPerformance`, `HelloShowcase`, `HelloStyleDemo`, `HelloVirtual(virtual_stage_showcase)`
- Static size scope: Map input sections from repo-side `src/` + `example/` objects only; exclude toolchain libraries, `driver/`, `porting/`; `ram_bytes` excludes `PFB`, and `pfb_bytes` still comes from each case-local `main.map` `.bss.pfb_area`.
- Successful apps: 66
- Failed apps: 0

## Overview

- Smallest Code: **HelloBasic(divider)** (10336 bytes)
- Largest Code: **HelloPerformance** (297532 bytes)
- Smallest RAM: **HelloBasic(menu)** (910 bytes)
- Largest RAM: **HelloBasic(svg)** (38547 bytes)
- Smallest Heap Peak: **HelloBasic(compass)** (0 bytes)
- Largest Heap Peak: **HelloBasic(svg)** (650881 bytes)
- Smallest Stack Peak: **HelloPerformance** (432 bytes)
- Largest Stack Peak: **HelloBasic(image)** (8192 bytes)

## Charts

![Size Report](images/size_report.png)

## Detailed Table

| App | Code | Resource | RAM | PFB | Heap Idle | Heap Init Peak | Heap Interaction Peak | Heap Peak | Stack Peak | Actions | Total ROM |
|-----|------|----------|-----|-----|-----------|----------------|-----------------------|-----------|------------|---------|-----------|
| HelloBasic(compass) | 29360 | 10372 | 1017 | 4800 | 0 | 0 | 0 | 0 | 1136 | 1 | 39732 |
| HelloBasic(mp4) | 42604 | 3149173 | 2904 | 4800 | 0 | 0 | 900 | 900 | 1096 | 3 | 3191777 |
| HelloBasic(enhanced_widgets) | 59552 | 18742 | 2598 | 4800 | 0 | 0 | 0 | 0 | 1264 | 3 | 78294 |
| HelloBasic(anim) | 21936 | 4005 | 1253 | 4800 | 0 | 0 | 0 | 0 | 780 | 3 | 25941 |
| HelloBasic(textblock) | 34204 | 9417 | 997 | 4800 | 0 | 0 | 0 | 0 | 1016 | 1 | 43621 |
| HelloBasic(activity_ring) | 34724 | 6069 | 993 | 4800 | 0 | 0 | 0 | 0 | 1204 | 1 | 40793 |
| HelloBasic(gauge) | 36536 | 6635 | 1229 | 4800 | 0 | 0 | 0 | 0 | 1256 | 1 | 43171 |
| HelloBasic(card) | 28556 | 9484 | 1809 | 4800 | 0 | 0 | 0 | 0 | 1072 | 1 | 38040 |
| HelloBasic(combobox) | 27508 | 25105 | 1422 | 4800 | 0 | 0 | 0 | 0 | 1144 | 13 | 52613 |
| HelloBasic(mask) | 83876 | 43717 | 1398 | 4800 | 0 | 28 | 0 | 28 | 4032 | 1 | 127593 |
| HelloBasic(animated_image) | 31956 | 33557 | 998 | 4800 | 0 | 0 | 0 | 0 | 968 | 1 | 65513 |
| HelloBasic(textinput) | 35492 | 15629 | 4463 | 4800 | 0 | 0 | 0 | 0 | 1088 | 9 | 51121 |
| HelloBasic(image) | 54592 | 133820 | 3356 | 4800 | 13571 | 14471 | 108629 | 108629 | 8192 | 5 | 188412 |
| HelloBasic(lyric_scroller) | 35816 | 9683 | 1314 | 4800 | 0 | 0 | 0 | 0 | 1072 | 3 | 45499 |
| HelloBasic(analog_clock) | 23036 | 4540 | 993 | 4800 | 0 | 0 | 0 | 0 | 1000 | 1 | 27576 |
| HelloBasic(file_image) | 76292 | 17170 | 3251 | 4800 | 94 | 94 | 94 | 94 | 1300 | 1 | 93462 |
| HelloBasic(rotation) | 21688 | 9346 | 2077 | 1600 | 0 | 0 | 0 | 0 | 1032 | 7 | 31034 |
| HelloBasic(svg) | 50496 | 14558 | 38547 | 4800 | 187980 | 495206 | 650881 | 650881 | 8192 | 7 | 65054 |
| HelloBasic(autocomplete) | 27316 | 9475 | 1114 | 4800 | 0 | 0 | 0 | 0 | 1144 | 5 | 36791 |
| HelloVirtual(virtual_stage_showcase) | 178848 | 68331 | 8819 | 40960 | 2756 | 2756 | 2872 | 2872 | 2600 | 19 | 247179 |
| HelloPerformance | 297532 | 1838882 | 5850 | 1536 | 0 | 0 | 255441 | 255441 | 432 | 263 | 2136414 |
| HelloSimple | 16996 | 6708 | 1049 | 4800 | 0 | 0 | 0 | 0 | 844 | 3 | 23704 |
| HelloShowcase | 158712 | 66343 | 14949 | 40960 | 0 | 0 | 0 | 0 | 2408 | 13 | 225055 |
| HelloStyleDemo | 120412 | 102837 | 6531 | 19200 | 0 | 0 | 0 | 0 | 2560 | 4 | 223249 |
| HelloBasic(button) | 22200 | 17956 | 1294 | 4800 | 0 | 0 | 0 | 0 | 1072 | 5 | 40156 |
| HelloBasic(arc_slider) | 26048 | 5562 | 1158 | 4800 | 0 | 0 | 0 | 0 | 1212 | 5 | 31610 |
| HelloBasic(button_matrix) | 27088 | 16610 | 972 | 4800 | 0 | 0 | 0 | 0 | 1192 | 4 | 43698 |
| HelloBasic(digital_clock) | 28368 | 9218 | 981 | 4800 | 0 | 0 | 0 | 0 | 968 | 1 | 37586 |
| HelloBasic(checkbox) | 24660 | 16619 | 1242 | 4800 | 0 | 0 | 0 | 0 | 1056 | 5 | 41279 |
| HelloBasic(divider) | 10336 | 343 | 1073 | 4800 | 0 | 0 | 0 | 0 | 752 | 1 | 10679 |
| HelloBasic(heart_rate) | 29976 | 20434 | 1065 | 4800 | 0 | 0 | 0 | 0 | 2320 | 1 | 50410 |
| HelloBasic(gridlayout) | 24284 | 3906 | 1205 | 4800 | 0 | 0 | 0 | 0 | 876 | 4 | 28190 |
| HelloBasic(label) | 17944 | 5993 | 1153 | 4800 | 0 | 0 | 0 | 0 | 1000 | 1 | 23937 |
| HelloBasic(image_button) | 22504 | 25143 | 1242 | 4800 | 0 | 0 | 0 | 0 | 1072 | 5 | 47647 |
| HelloBasic(led) | 16508 | 3550 | 1185 | 4800 | 0 | 0 | 0 | 0 | 836 | 1 | 20058 |
| HelloBasic(line) | 16752 | 419 | 969 | 4800 | 0 | 0 | 0 | 0 | 968 | 1 | 17171 |
| HelloBasic(linearlayout) | 27496 | 9213 | 1009 | 4800 | 0 | 0 | 0 | 0 | 1000 | 1 | 36709 |
| HelloBasic(list) | 24980 | 14355 | 2459 | 4800 | 0 | 0 | 0 | 0 | 1040 | 6 | 39335 |
| HelloBasic(menu) | 18456 | 11093 | 910 | 4800 | 0 | 0 | 0 | 0 | 1048 | 5 | 29549 |
| HelloBasic(mini_calendar) | 26844 | 9390 | 916 | 4800 | 0 | 0 | 0 | 0 | 1056 | 3 | 36234 |
| HelloBasic(notification_badge) | 21220 | 13901 | 1217 | 4800 | 0 | 0 | 0 | 0 | 1024 | 1 | 35121 |
| HelloBasic(number_picker) | 22544 | 21775 | 1286 | 4800 | 0 | 0 | 0 | 0 | 1064 | 5 | 44319 |
| HelloBasic(progress_bar) | 16756 | 3557 | 1109 | 4800 | 0 | 0 | 0 | 0 | 780 | 1 | 20313 |
| HelloBasic(radio_button) | 24948 | 21382 | 1266 | 4800 | 0 | 0 | 0 | 0 | 1040 | 5 | 46330 |
| HelloBasic(page_indicator) | 30300 | 13101 | 1289 | 4800 | 0 | 0 | 0 | 0 | 1080 | 4 | 43401 |
| HelloBasic(scale) | 20068 | 5869 | 957 | 4800 | 0 | 0 | 0 | 0 | 1056 | 1 | 25937 |
| HelloBasic(roller) | 18832 | 6178 | 1226 | 4800 | 0 | 0 | 0 | 0 | 1024 | 5 | 25010 |
| HelloBasic(scroll) | 29324 | 9348 | 1173 | 4800 | 0 | 0 | 0 | 0 | 1096 | 2 | 38672 |
| HelloBasic(spangroup) | 16776 | 5904 | 953 | 4800 | 0 | 0 | 0 | 0 | 1008 | 1 | 22680 |
| HelloBasic(spinner) | 24100 | 5021 | 1185 | 4800 | 0 | 0 | 0 | 0 | 892 | 1 | 29121 |
| HelloBasic(slider) | 18520 | 3772 | 1130 | 4800 | 0 | 0 | 0 | 0 | 876 | 5 | 22292 |
| HelloBasic(stopwatch) | 29424 | 9236 | 1057 | 4800 | 0 | 0 | 0 | 0 | 1000 | 1 | 38660 |
| HelloBasic(switch) | 24840 | 16071 | 1210 | 4800 | 0 | 0 | 0 | 0 | 1048 | 5 | 40911 |
| HelloBasic(table) | 20632 | 5914 | 1385 | 4800 | 0 | 0 | 0 | 0 | 1040 | 1 | 26546 |
| HelloBasic(tab_bar) | 30980 | 16704 | 1313 | 4800 | 0 | 0 | 0 | 0 | 1080 | 4 | 47684 |
| HelloBasic(toggle_button) | 22024 | 17827 | 1238 | 4800 | 0 | 0 | 0 | 0 | 1064 | 5 | 39851 |
| HelloBasic(tileview) | 27288 | 9410 | 1217 | 4800 | 0 | 0 | 0 | 0 | 1000 | 4 | 36698 |
| HelloBasic(viewpage) | 29380 | 9350 | 1177 | 4800 | 0 | 0 | 0 | 0 | 1080 | 4 | 38730 |
| HelloBasic(window) | 20256 | 14060 | 1234 | 4800 | 0 | 0 | 0 | 0 | 1032 | 2 | 34316 |
| HelloBasic(viewpage_cache) | 28400 | 9320 | 965 | 4800 | 320 | 320 | 480 | 480 | 1064 | 4 | 37720 |
| HelloBasic(circular_progress_bar) | 32860 | 10865 | 1185 | 4800 | 0 | 0 | 0 | 0 | 1204 | 1 | 43725 |
| HelloBasic(button_img) | 35416 | 36930 | 924 | 4800 | 0 | 60 | 60 | 60 | 760 | 3 | 72346 |
| HelloBasic(chips) | 28052 | 14240 | 1130 | 4800 | 0 | 0 | 0 | 0 | 1256 | 4 | 42292 |
| HelloBasic(pattern_lock) | 31356 | 9551 | 1206 | 4800 | 0 | 0 | 0 | 0 | 1072 | 5 | 40907 |
| HelloBasic(segmented_control) | 28076 | 16752 | 1258 | 4800 | 0 | 0 | 0 | 0 | 1160 | 9 | 44828 |
| HelloBasic(stepper) | 22820 | 14233 | 1166 | 4800 | 0 | 0 | 0 | 0 | 1072 | 6 | 37053 |