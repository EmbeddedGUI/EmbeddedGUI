# QEMU Size And Memory Report

- Commit: `6da0b74`
- Date: 2026-04-12T11:25:09.536583
- Build target: `PORT=qemu CPU_ARCH=cortex-m0plus`
- Runtime target: `qemu-system-arm -machine mps2-an385 -cpu cortex-m3`
- Scope: `HelloBasic/*`, `HelloSimple`, `HelloPerformance`, `HelloShowcase`, `HelloStyleDemo`, `HelloVirtual(virtual_stage_showcase)`
- Static size scope: Map input sections from repo-side `src/` + `example/` objects only; exclude toolchain libraries, `driver/`, `porting/`; `ram_bytes` excludes `PFB`, and `pfb_bytes` still comes from each case-local `main.map` `.bss.pfb_area`.
- Successful apps: 66
- Failed apps: 0

## Overview

- Smallest Code: **HelloPerformance** (0 bytes)
- Largest Code: **HelloVirtual(virtual_stage_showcase)** (178840 bytes)
- Smallest RAM: **HelloPerformance** (0 bytes)
- Largest RAM: **HelloVirtual(virtual_stage_showcase)** (8819 bytes)
- Smallest Heap Peak: **HelloBasic(compass)** (0 bytes)
- Largest Heap Peak: **HelloPerformance** (6028 bytes)
- Smallest Stack Peak: **HelloPerformance** (432 bytes)
- Largest Stack Peak: **HelloBasic(mask)** (4032 bytes)

## Charts

![Size Report](images/size_report.png)

## Detailed Table

| App | Code | Resource | RAM | PFB | Heap Idle | Heap Init Peak | Heap Interaction Peak | Heap Peak | Stack Peak | Actions | Total ROM |
|-----|------|----------|-----|-----|-----------|----------------|-----------------------|-----------|------------|---------|-----------|
| HelloBasic(compass) | 29352 | 10372 | 1017 | 4800 | 0 | 0 | 0 | 0 | 1136 | 1 | 39724 |
| HelloBasic(pattern_lock) | 31348 | 9551 | 1206 | 4800 | 0 | 0 | 0 | 0 | 1072 | 5 | 40899 |
| HelloBasic(mp4) | 40444 | 3149173 | 2083 | 4800 | 0 | 0 | 1724 | 1724 | 1096 | 3 | 3189617 |
| HelloBasic(animated_image) | 29584 | 33557 | 998 | 4800 | 0 | 0 | 0 | 0 | 968 | 1 | 63141 |
| HelloBasic(enhanced_widgets) | 59544 | 18742 | 2598 | 4800 | 0 | 0 | 0 | 0 | 1264 | 3 | 78286 |
| HelloBasic(combobox) | 27500 | 25105 | 1422 | 4800 | 0 | 0 | 0 | 0 | 1144 | 13 | 52605 |
| HelloBasic(anim) | 21928 | 4005 | 1253 | 4800 | 0 | 0 | 0 | 0 | 780 | 3 | 25933 |
| HelloBasic(image) | 43976 | 72212 | 2003 | 4800 | 824 | 1724 | 1804 | 1804 | 1128 | 3 | 116188 |
| HelloBasic(activity_ring) | 34716 | 6069 | 993 | 4800 | 0 | 0 | 0 | 0 | 1204 | 1 | 40785 |
| HelloBasic(file_image) | 69332 | 17170 | 2971 | 4800 | 94 | 94 | 94 | 94 | 1220 | 1 | 86502 |
| HelloBasic(gauge) | 36532 | 6635 | 1229 | 4800 | 0 | 0 | 0 | 0 | 1256 | 1 | 43167 |
| HelloBasic(card) | 28548 | 9484 | 1809 | 4800 | 0 | 0 | 0 | 0 | 1072 | 1 | 38032 |
| HelloBasic(thermostat) | 26808 | 46547 | 895 | 6400 | 0 | 0 | 0 | 0 | 1176 | 6 | 73355 |
| HelloBasic(mask) | 81472 | 43717 | 1398 | 4800 | 0 | 28 | 0 | 28 | 4032 | 1 | 125189 |
| HelloBasic(lyric_scroller) | 35808 | 9683 | 1314 | 4800 | 0 | 0 | 0 | 0 | 1072 | 3 | 45491 |
| HelloBasic(textinput) | 35484 | 15629 | 4463 | 4800 | 0 | 0 | 0 | 0 | 1088 | 9 | 51113 |
| HelloBasic(analog_clock) | 23032 | 4540 | 993 | 4800 | 0 | 0 | 0 | 0 | 996 | 1 | 27572 |
| HelloBasic(rotation) | 21680 | 9346 | 2077 | 1600 | 0 | 0 | 0 | 0 | 1032 | 7 | 31026 |
| HelloBasic(autocomplete) | 27308 | 9475 | 1114 | 4800 | 0 | 0 | 0 | 0 | 1144 | 5 | 36783 |
| HelloBasic(textblock) | 34196 | 9417 | 997 | 4800 | 0 | 0 | 0 | 0 | 1016 | 1 | 43613 |
| HelloVirtual(virtual_stage_showcase) | 178840 | 68331 | 8819 | 40960 | 2756 | 2756 | 2872 | 2872 | 2616 | 19 | 247171 |
| HelloPerformance | 0 | 0 | 0 | 1536 | 0 | 0 | 6028 | 6028 | 432 | 257 | 0 |
| HelloSimple | 0 | 0 | 0 | 4800 | 0 | 0 | 0 | 0 | 844 | 3 | 0 |
| HelloShowcase | 0 | 0 | 0 | 40960 | 0 | 0 | 0 | 0 | 2408 | 13 | 0 |
| HelloStyleDemo | 0 | 0 | 0 | 19200 | 0 | 0 | 0 | 0 | 2560 | 4 | 0 |
| HelloBasic(button_img) | 33044 | 36930 | 924 | 4800 | 0 | 60 | 60 | 60 | 760 | 3 | 69974 |
| HelloBasic(segmented_control) | 28068 | 16752 | 1258 | 4800 | 0 | 0 | 0 | 0 | 1160 | 9 | 44820 |
| HelloBasic(button) | 22192 | 17956 | 1294 | 4800 | 0 | 0 | 0 | 0 | 1072 | 5 | 40148 |
| HelloBasic(arc_slider) | 26040 | 5562 | 1158 | 4800 | 0 | 0 | 0 | 0 | 1212 | 5 | 31602 |
| HelloBasic(button_matrix) | 27080 | 16610 | 972 | 4800 | 0 | 0 | 0 | 0 | 1192 | 4 | 43690 |
| HelloBasic(digital_clock) | 28360 | 9218 | 981 | 4800 | 0 | 0 | 0 | 0 | 968 | 1 | 37578 |
| HelloBasic(checkbox) | 24652 | 16619 | 1242 | 4800 | 0 | 0 | 0 | 0 | 1056 | 5 | 41271 |
| HelloBasic(divider) | 10328 | 343 | 1073 | 4800 | 0 | 0 | 0 | 0 | 752 | 1 | 10671 |
| HelloBasic(heart_rate) | 29968 | 20434 | 1065 | 4800 | 0 | 0 | 0 | 0 | 2304 | 1 | 50402 |
| HelloBasic(gridlayout) | 24276 | 3906 | 1205 | 4800 | 0 | 0 | 0 | 0 | 876 | 4 | 28182 |
| HelloBasic(label) | 17936 | 5993 | 1153 | 4800 | 0 | 0 | 0 | 0 | 1000 | 1 | 23929 |
| HelloBasic(image_button) | 22496 | 25143 | 1242 | 4800 | 0 | 0 | 0 | 0 | 1072 | 5 | 47639 |
| HelloBasic(led) | 16500 | 3550 | 1185 | 4800 | 0 | 0 | 0 | 0 | 836 | 1 | 20050 |
| HelloBasic(line) | 16748 | 419 | 969 | 4800 | 0 | 0 | 0 | 0 | 968 | 1 | 17167 |
| HelloBasic(linearlayout) | 27488 | 9213 | 1009 | 4800 | 0 | 0 | 0 | 0 | 1000 | 1 | 36701 |
| HelloBasic(list) | 24972 | 14355 | 2459 | 4800 | 0 | 0 | 0 | 0 | 1040 | 6 | 39327 |
| HelloBasic(menu) | 18448 | 11093 | 910 | 4800 | 0 | 0 | 0 | 0 | 1048 | 5 | 29541 |
| HelloBasic(mini_calendar) | 26836 | 9390 | 916 | 4800 | 0 | 0 | 0 | 0 | 1056 | 3 | 36226 |
| HelloBasic(notification_badge) | 21212 | 13901 | 1217 | 4800 | 0 | 0 | 0 | 0 | 1024 | 1 | 35113 |
| HelloBasic(number_picker) | 22536 | 21775 | 1286 | 4800 | 0 | 0 | 0 | 0 | 1064 | 5 | 44311 |
| HelloBasic(progress_bar) | 16748 | 3557 | 1109 | 4800 | 0 | 0 | 0 | 0 | 780 | 1 | 20305 |
| HelloBasic(radio_button) | 24940 | 21382 | 1266 | 4800 | 0 | 0 | 0 | 0 | 1040 | 5 | 46322 |
| HelloBasic(page_indicator) | 30292 | 13101 | 1289 | 4800 | 0 | 0 | 0 | 0 | 1080 | 4 | 43393 |
| HelloBasic(scale) | 20060 | 5869 | 957 | 4800 | 0 | 0 | 0 | 0 | 1056 | 1 | 25929 |
| HelloBasic(roller) | 18824 | 6178 | 1226 | 4800 | 0 | 0 | 0 | 0 | 1024 | 5 | 25002 |
| HelloBasic(scroll) | 29316 | 9348 | 1173 | 4800 | 0 | 0 | 0 | 0 | 1096 | 2 | 38664 |
| HelloBasic(spangroup) | 16768 | 5904 | 953 | 4800 | 0 | 0 | 0 | 0 | 1008 | 1 | 22672 |
| HelloBasic(slider) | 18512 | 3772 | 1130 | 4800 | 0 | 0 | 0 | 0 | 876 | 5 | 22284 |
| HelloBasic(spinner) | 24092 | 5021 | 1185 | 4800 | 0 | 0 | 0 | 0 | 892 | 1 | 29113 |
| HelloBasic(stopwatch) | 29416 | 9236 | 1057 | 4800 | 0 | 0 | 0 | 0 | 1000 | 1 | 38652 |
| HelloBasic(switch) | 24832 | 16071 | 1210 | 4800 | 0 | 0 | 0 | 0 | 1048 | 5 | 40903 |
| HelloBasic(table) | 20624 | 5914 | 1385 | 4800 | 0 | 0 | 0 | 0 | 1040 | 1 | 26538 |
| HelloBasic(tab_bar) | 30972 | 16704 | 1313 | 4800 | 0 | 0 | 0 | 0 | 1080 | 4 | 47676 |
| HelloBasic(toggle_button) | 22016 | 17827 | 1238 | 4800 | 0 | 0 | 0 | 0 | 1064 | 5 | 39843 |
| HelloBasic(tileview) | 27280 | 9410 | 1217 | 4800 | 0 | 0 | 0 | 0 | 1000 | 4 | 36690 |
| HelloBasic(viewpage) | 29372 | 9350 | 1177 | 4800 | 0 | 0 | 0 | 0 | 1080 | 4 | 38722 |
| HelloBasic(window) | 20248 | 14060 | 1234 | 4800 | 0 | 0 | 0 | 0 | 1032 | 2 | 34308 |
| HelloBasic(viewpage_cache) | 28392 | 9320 | 965 | 4800 | 320 | 320 | 480 | 480 | 1064 | 4 | 37712 |
| HelloBasic(circular_progress_bar) | 32852 | 10865 | 1185 | 4800 | 0 | 0 | 0 | 0 | 1204 | 1 | 43717 |
| HelloBasic(chips) | 28044 | 14240 | 1130 | 4800 | 0 | 0 | 0 | 0 | 1256 | 4 | 42284 |
| HelloBasic(stepper) | 22812 | 14233 | 1166 | 4800 | 0 | 0 | 0 | 0 | 1072 | 6 | 37045 |