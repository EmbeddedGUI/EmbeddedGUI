# QEMU Size And Memory Report

- Commit: `81a737b`
- Date: 2026-04-08T14:21:28.082046
- Build target: `PORT=qemu CPU_ARCH=cortex-m0plus`
- Runtime target: `qemu-system-arm -machine mps2-an385 -cpu cortex-m3`
- Scope: `HelloBasic/*`, `HelloSimple`, `HelloPerformance`, `HelloShowcase`, `HelloStyleDemo`, `HelloVirtual(virtual_stage_showcase)`
- Static size scope: Map input sections from repo-side `src/` + `example/` objects only; exclude toolchain libraries, `driver/`, `porting/`; `ram_bytes` excludes `PFB`, and `pfb_bytes` still comes from each case-local `main.map` `.bss.pfb_area`.
- Successful apps: 64
- Failed apps: 0

## Overview

- Smallest Code: **HelloPerformance** (0 bytes)
- Largest Code: **HelloVirtual(virtual_stage_showcase)** (165888 bytes)
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
| HelloBasic(compass) | 27768 | 10372 | 1017 | 4800 | 0 | 0 | 0 | 0 | 1112 | 1 | 38140 |
| HelloBasic(mp4) | 38364 | 3149137 | 2083 | 4800 | 0 | 0 | 1724 | 1724 | 1072 | 3 | 3187501 |
| HelloBasic(enhanced_widgets) | 57688 | 18742 | 2594 | 4800 | 0 | 0 | 0 | 0 | 1240 | 3 | 76430 |
| HelloBasic(anim) | 21896 | 4005 | 1253 | 4800 | 0 | 0 | 0 | 0 | 780 | 3 | 25901 |
| HelloBasic(textblock) | 32524 | 9417 | 997 | 4800 | 0 | 0 | 0 | 0 | 984 | 1 | 41941 |
| HelloBasic(image) | 42164 | 72176 | 2003 | 4800 | 824 | 1724 | 1804 | 1804 | 1088 | 3 | 114340 |
| HelloBasic(activity_ring) | 34516 | 5502 | 993 | 4800 | 0 | 0 | 0 | 0 | 1224 | 1 | 40018 |
| HelloBasic(gauge) | 34948 | 6635 | 1229 | 4800 | 0 | 0 | 0 | 0 | 1232 | 1 | 41583 |
| HelloBasic(card) | 26964 | 9484 | 1809 | 4800 | 0 | 0 | 0 | 0 | 1048 | 1 | 36448 |
| HelloBasic(combobox) | 25916 | 25105 | 1422 | 4800 | 0 | 0 | 0 | 0 | 1120 | 13 | 51021 |
| HelloBasic(mask) | 79648 | 43705 | 1398 | 4800 | 0 | 28 | 0 | 28 | 4032 | 1 | 123353 |
| HelloBasic(animated_image) | 27608 | 33545 | 998 | 4800 | 0 | 0 | 0 | 0 | 944 | 1 | 61153 |
| HelloBasic(textinput) | 33900 | 15629 | 4463 | 4800 | 0 | 0 | 0 | 0 | 1064 | 9 | 49529 |
| HelloBasic(lyric_scroller) | 34136 | 9683 | 1314 | 4800 | 0 | 0 | 0 | 0 | 1072 | 3 | 43819 |
| HelloBasic(analog_clock) | 23000 | 4540 | 993 | 4800 | 0 | 0 | 0 | 0 | 996 | 1 | 27540 |
| HelloBasic(rotation) | 20096 | 9346 | 2077 | 1600 | 0 | 0 | 0 | 0 | 1008 | 7 | 29442 |
| HelloBasic(autocomplete) | 25724 | 9475 | 1114 | 4800 | 0 | 0 | 0 | 0 | 1120 | 5 | 35199 |
| HelloVirtual(virtual_stage_showcase) | 165888 | 67210 | 8819 | 40960 | 2756 | 2756 | 2872 | 2872 | 2600 | 19 | 233098 |
| HelloPerformance | 0 | 0 | 0 | 1536 | 0 | 0 | 6028 | 6028 | 432 | 239 | 0 |
| HelloSimple | 0 | 0 | 0 | 4800 | 0 | 0 | 0 | 0 | 820 | 3 | 0 |
| HelloStyleDemo | 0 | 0 | 0 | 19200 | 0 | 0 | 0 | 0 | 2560 | 4 | 0 |
| HelloShowcase | 0 | 0 | 0 | 40960 | 0 | 0 | 0 | 0 | 2408 | 13 | 0 |
| HelloBasic(button) | 20608 | 17956 | 1294 | 4800 | 0 | 0 | 0 | 0 | 1048 | 5 | 38564 |
| HelloBasic(arc_slider) | 25840 | 4995 | 1158 | 4800 | 0 | 0 | 0 | 0 | 1212 | 5 | 30835 |
| HelloBasic(button_matrix) | 25496 | 16610 | 972 | 4800 | 0 | 0 | 0 | 0 | 1168 | 4 | 42106 |
| HelloBasic(checkbox) | 23068 | 16619 | 1242 | 4800 | 0 | 0 | 0 | 0 | 1032 | 5 | 39687 |
| HelloBasic(digital_clock) | 26776 | 9218 | 981 | 4800 | 0 | 0 | 0 | 0 | 944 | 1 | 35994 |
| HelloBasic(divider) | 10296 | 343 | 1073 | 4800 | 0 | 0 | 0 | 0 | 752 | 1 | 10639 |
| HelloBasic(heart_rate) | 28384 | 20434 | 1065 | 4800 | 0 | 0 | 0 | 0 | 2320 | 1 | 48818 |
| HelloBasic(gridlayout) | 17684 | 3582 | 1173 | 4800 | 0 | 0 | 0 | 0 | 876 | 4 | 21266 |
| HelloBasic(label) | 16352 | 5993 | 1153 | 4800 | 0 | 0 | 0 | 0 | 976 | 1 | 22345 |
| HelloBasic(image_button) | 21268 | 25170 | 1242 | 4800 | 0 | 0 | 0 | 0 | 1048 | 5 | 46438 |
| HelloBasic(led) | 16468 | 3550 | 1185 | 4800 | 0 | 0 | 0 | 0 | 836 | 1 | 20018 |
| HelloBasic(line) | 16716 | 419 | 969 | 4800 | 0 | 0 | 0 | 0 | 968 | 1 | 17135 |
| HelloBasic(linearlayout) | 25904 | 9213 | 1009 | 4800 | 0 | 0 | 0 | 0 | 976 | 1 | 35117 |
| HelloBasic(menu) | 16864 | 11093 | 910 | 4800 | 0 | 0 | 0 | 0 | 1024 | 5 | 27957 |
| HelloBasic(list) | 23388 | 14355 | 2459 | 4800 | 0 | 0 | 0 | 0 | 1016 | 6 | 37743 |
| HelloBasic(mini_calendar) | 25252 | 9390 | 916 | 4800 | 0 | 0 | 0 | 0 | 1032 | 3 | 34642 |
| HelloBasic(notification_badge) | 19628 | 13901 | 1217 | 4800 | 0 | 0 | 0 | 0 | 1000 | 1 | 33529 |
| HelloBasic(number_picker) | 20952 | 21775 | 1286 | 4800 | 0 | 0 | 0 | 0 | 1040 | 5 | 42727 |
| HelloBasic(progress_bar) | 16716 | 3557 | 1109 | 4800 | 0 | 0 | 0 | 0 | 780 | 1 | 20273 |
| HelloBasic(radio_button) | 23356 | 21382 | 1266 | 4800 | 0 | 0 | 0 | 0 | 1016 | 5 | 44738 |
| HelloBasic(page_indicator) | 28708 | 13101 | 1289 | 4800 | 0 | 0 | 0 | 0 | 1056 | 4 | 41809 |
| HelloBasic(scale) | 18476 | 5869 | 957 | 4800 | 0 | 0 | 0 | 0 | 1032 | 1 | 24345 |
| HelloBasic(roller) | 17240 | 6178 | 1226 | 4800 | 0 | 0 | 0 | 0 | 1000 | 5 | 23418 |
| HelloBasic(scroll) | 27732 | 9348 | 1173 | 4800 | 0 | 0 | 0 | 0 | 1072 | 2 | 37080 |
| HelloBasic(spangroup) | 15184 | 5904 | 953 | 4800 | 0 | 0 | 0 | 0 | 984 | 1 | 21088 |
| HelloBasic(slider) | 18132 | 3772 | 1114 | 4800 | 0 | 0 | 0 | 0 | 876 | 5 | 21904 |
| HelloBasic(spinner) | 23892 | 4454 | 1185 | 4800 | 0 | 0 | 0 | 0 | 916 | 1 | 28346 |
| HelloBasic(stopwatch) | 27832 | 9236 | 1057 | 4800 | 0 | 0 | 0 | 0 | 976 | 1 | 37068 |
| HelloBasic(switch) | 23248 | 16071 | 1210 | 4800 | 0 | 0 | 0 | 0 | 1024 | 5 | 39319 |
| HelloBasic(table) | 19040 | 5914 | 1385 | 4800 | 0 | 0 | 0 | 0 | 1016 | 1 | 24954 |
| HelloBasic(tab_bar) | 29388 | 16704 | 1313 | 4800 | 0 | 0 | 0 | 0 | 1088 | 4 | 46092 |
| HelloBasic(tileview) | 25696 | 9410 | 1217 | 4800 | 0 | 0 | 0 | 0 | 976 | 4 | 35106 |
| HelloBasic(toggle_button) | 20432 | 17827 | 1238 | 4800 | 0 | 0 | 0 | 0 | 1040 | 5 | 38259 |
| HelloBasic(viewpage) | 27788 | 9350 | 1177 | 4800 | 0 | 0 | 0 | 0 | 1056 | 4 | 37138 |
| HelloBasic(window) | 18664 | 14060 | 1234 | 4800 | 0 | 0 | 0 | 0 | 1008 | 2 | 32724 |
| HelloBasic(viewpage_cache) | 26848 | 9320 | 965 | 4800 | 320 | 320 | 480 | 480 | 1040 | 4 | 36168 |
| HelloBasic(circular_progress_bar) | 31100 | 10298 | 1185 | 4800 | 0 | 0 | 0 | 0 | 1204 | 1 | 41398 |
| HelloBasic(button_img) | 31056 | 36918 | 924 | 4800 | 0 | 60 | 60 | 60 | 760 | 3 | 67974 |
| HelloBasic(chips) | 26460 | 14240 | 1130 | 4800 | 0 | 0 | 0 | 0 | 1200 | 4 | 40700 |
| HelloBasic(pattern_lock) | 29764 | 9551 | 1206 | 4800 | 0 | 0 | 0 | 0 | 1048 | 5 | 39315 |
| HelloBasic(segmented_control) | 26484 | 16752 | 1258 | 4800 | 0 | 0 | 0 | 0 | 1136 | 9 | 43236 |
| HelloBasic(stepper) | 21228 | 14233 | 1166 | 4800 | 0 | 0 | 0 | 0 | 1048 | 6 | 35461 |