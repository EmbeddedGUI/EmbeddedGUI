# QEMU Size And Memory Report

- Commit: `e60544f`
- Date: 2026-03-31T06:56:15.706118
- Build target: `PORT=qemu CPU_ARCH=cortex-m0plus`
- Runtime target: `qemu-system-arm -machine mps2-an385 -cpu cortex-m3`
- Scope: HelloBasic 全子 case、HelloSimple、HelloPerformance、HelloShowcase、HelloStyleDemo、HelloVirtual(virtual_stage_showcase)
- Successful apps: 64
- Failed apps: 0

## Overview

- Smallest Code: **HelloBasic(divider)** (15040 bytes)
- Largest Code: **HelloPerformance** (267512 bytes)
- Smallest RAM: **HelloPerformance** (708 bytes)
- Largest RAM: **HelloShowcase** (14960 bytes)
- Smallest Heap Peak: **HelloBasic(activity_ring)** (0 bytes)
- Largest Heap Peak: **HelloPerformance** (6644 bytes)
- Smallest Stack Peak: **HelloPerformance** (432 bytes)
- Largest Stack Peak: **HelloBasic(mask)** (4124 bytes)

## Charts

![Size Report](images/size_report.png)

## Detailed Table

| App | Code | Resource | RAM | PFB | Heap Idle | Heap Init Peak | Heap Interaction Peak | Heap Peak | Stack Peak | Actions | Total ROM |
|-----|------|----------|-----|-----|-----------|----------------|-----------------------|-----------|------------|---------|-----------|
| HelloBasic(activity_ring) | 46400 | 6472 | 1060 | 4800 | 0 | 0 | 0 | 0 | 1232 | 1 | 52872 |
| HelloBasic(analog_clock) | 33512 | 4968 | 1060 | 4800 | 0 | 0 | 0 | 0 | 1116 | 1 | 38480 |
| HelloBasic(anim) | 29248 | 4468 | 1316 | 4800 | 0 | 0 | 0 | 0 | 752 | 3 | 33716 |
| HelloBasic(animated_image) | 57728 | 37200 | 1076 | 4800 | 0 | 0 | 0 | 0 | 1024 | 1 | 94928 |
| HelloBasic(arc_slider) | 35812 | 5440 | 1212 | 4800 | 0 | 0 | 0 | 0 | 1272 | 5 | 41252 |
| HelloBasic(autocomplete) | 41148 | 25376 | 1600 | 4800 | 0 | 0 | 0 | 0 | 1192 | 5 | 66524 |
| HelloBasic(button) | 31892 | 26488 | 1348 | 4800 | 0 | 0 | 0 | 0 | 1128 | 5 | 58380 |
| HelloBasic(button_img) | 62220 | 53996 | 1000 | 4800 | 0 | 60 | 60 | 60 | 908 | 3 | 116216 |
| HelloBasic(button_matrix) | 38404 | 25208 | 1048 | 4800 | 0 | 0 | 0 | 0 | 1264 | 4 | 63612 |
| HelloBasic(card) | 40492 | 11144 | 1844 | 4800 | 0 | 0 | 0 | 0 | 1136 | 1 | 51636 |
| HelloBasic(checkbox) | 40516 | 26432 | 1300 | 4800 | 0 | 0 | 0 | 0 | 1112 | 5 | 66948 |
| HelloBasic(chips) | 41888 | 25500 | 1616 | 4800 | 0 | 0 | 0 | 0 | 1296 | 4 | 67388 |
| HelloBasic(circular_progress_bar) | 47628 | 18848 | 1232 | 4800 | 0 | 0 | 0 | 0 | 1248 | 1 | 66476 |
| HelloBasic(combobox) | 38944 | 25376 | 1480 | 4800 | 0 | 0 | 0 | 0 | 1192 | 13 | 64320 |
| HelloBasic(compass) | 42512 | 10840 | 1088 | 4800 | 0 | 0 | 0 | 0 | 1192 | 1 | 53352 |
| HelloBasic(digital_clock) | 40132 | 9672 | 1056 | 4800 | 0 | 0 | 0 | 0 | 1024 | 1 | 49804 |
| HelloBasic(divider) | 15040 | 732 | 1132 | 4800 | 0 | 0 | 0 | 0 | 752 | 1 | 15772 |
| HelloBasic(enhanced_widgets) | 72696 | 27648 | 2596 | 4800 | 0 | 0 | 0 | 0 | 1320 | 3 | 100344 |
| HelloBasic(gauge) | 47852 | 14568 | 1280 | 4800 | 0 | 0 | 0 | 0 | 1208 | 1 | 62420 |
| HelloBasic(gridlayout) | 35924 | 21148 | 1268 | 4800 | 0 | 0 | 0 | 0 | 904 | 4 | 57072 |
| HelloBasic(heart_rate) | 42984 | 20904 | 1136 | 4800 | 0 | 0 | 0 | 0 | 2344 | 1 | 63888 |
| HelloBasic(image) | 78760 | 72632 | 2036 | 4800 | 824 | 1724 | 1804 | 1804 | 1168 | 3 | 151392 |
| HelloBasic(image_button) | 37952 | 25308 | 1296 | 4800 | 0 | 0 | 0 | 0 | 1136 | 5 | 63260 |
| HelloBasic(label) | 26892 | 6368 | 1216 | 4800 | 0 | 0 | 0 | 0 | 1056 | 1 | 33260 |
| HelloBasic(led) | 23604 | 4020 | 1244 | 4800 | 0 | 0 | 0 | 0 | 856 | 1 | 27624 |
| HelloBasic(line) | 22904 | 800 | 1036 | 4800 | 0 | 0 | 0 | 0 | 1000 | 1 | 23704 |
| HelloBasic(linearlayout) | 39260 | 9668 | 1080 | 4800 | 0 | 0 | 0 | 0 | 1056 | 1 | 48928 |
| HelloBasic(list) | 34848 | 26680 | 2460 | 4800 | 0 | 0 | 0 | 0 | 1080 | 6 | 61528 |
| HelloBasic(mask) | 95660 | 43972 | 1468 | 4800 | 0 | 28 | 0 | 28 | 4124 | 1 | 139632 |
| HelloBasic(menu) | 26920 | 22052 | 984 | 4800 | 0 | 0 | 0 | 0 | 1104 | 5 | 48972 |
| HelloBasic(mini_calendar) | 38084 | 9656 | 992 | 4800 | 0 | 0 | 0 | 0 | 1112 | 3 | 47740 |
| HelloBasic(mp4) | 71384 | 3149652 | 2116 | 4800 | 0 | 0 | 1724 | 1724 | 1152 | 3 | 3221036 |
| HelloBasic(notification_badge) | 31392 | 25196 | 1280 | 4800 | 0 | 0 | 0 | 0 | 1104 | 1 | 56588 |
| HelloBasic(number_picker) | 32512 | 21940 | 1344 | 4800 | 0 | 0 | 0 | 0 | 1128 | 5 | 54452 |
| HelloBasic(page_indicator) | 42356 | 25508 | 1344 | 4800 | 0 | 0 | 0 | 0 | 1136 | 4 | 67864 |
| HelloBasic(pattern_lock) | 46092 | 26724 | 1692 | 4800 | 0 | 0 | 0 | 0 | 1128 | 5 | 72816 |
| HelloBasic(progress_bar) | 23912 | 5224 | 1168 | 4800 | 0 | 0 | 0 | 0 | 760 | 1 | 29136 |
| HelloBasic(radio_button) | 36232 | 25248 | 1328 | 4800 | 0 | 0 | 0 | 0 | 1096 | 5 | 61480 |
| HelloBasic(roller) | 27384 | 6332 | 1280 | 4800 | 0 | 0 | 0 | 0 | 1080 | 5 | 33716 |
| HelloBasic(rotation) | 34268 | 26636 | 2556 | 1600 | 0 | 0 | 0 | 0 | 1088 | 7 | 60904 |
| HelloBasic(scale) | 30024 | 6240 | 1032 | 4800 | 0 | 0 | 0 | 0 | 1112 | 1 | 36264 |
| HelloBasic(scroll) | 41044 | 9808 | 1240 | 4800 | 0 | 0 | 0 | 0 | 1152 | 2 | 50852 |
| HelloBasic(segmented_control) | 38880 | 25352 | 1320 | 4800 | 0 | 0 | 0 | 0 | 1216 | 9 | 64232 |
| HelloBasic(slider) | 25076 | 5220 | 1168 | 4800 | 0 | 0 | 0 | 0 | 896 | 5 | 30296 |
| HelloBasic(spangroup) | 25716 | 6272 | 1032 | 4800 | 0 | 0 | 0 | 0 | 952 | 1 | 31988 |
| HelloBasic(spinner) | 34388 | 5416 | 1244 | 4800 | 0 | 0 | 0 | 0 | 908 | 1 | 39804 |
| HelloBasic(stepper) | 41100 | 26736 | 1652 | 4800 | 0 | 0 | 0 | 0 | 1128 | 6 | 67836 |
| HelloBasic(stopwatch) | 41116 | 9692 | 1128 | 4800 | 0 | 0 | 0 | 0 | 1056 | 1 | 50808 |
| HelloBasic(switch) | 36088 | 21156 | 1268 | 4800 | 0 | 0 | 0 | 0 | 1104 | 5 | 57244 |
| HelloBasic(tab_bar) | 42656 | 25544 | 1376 | 4800 | 0 | 0 | 0 | 0 | 1136 | 4 | 68200 |
| HelloBasic(table) | 30596 | 6284 | 1464 | 4800 | 0 | 0 | 0 | 0 | 1096 | 1 | 36880 |
| HelloBasic(textblock) | 36240 | 9856 | 1064 | 4800 | 0 | 0 | 0 | 0 | 968 | 1 | 46096 |
| HelloBasic(textinput) | 46660 | 27844 | 4380 | 4800 | 0 | 0 | 0 | 0 | 1144 | 9 | 74504 |
| HelloBasic(thermostat) | 39504 | 47000 | 1392 | 6400 | 0 | 0 | 0 | 0 | 1088 | 6 | 86504 |
| HelloBasic(tileview) | 38996 | 9868 | 1280 | 4800 | 0 | 0 | 0 | 0 | 1056 | 4 | 48864 |
| HelloBasic(toggle_button) | 31748 | 25252 | 1296 | 4800 | 0 | 0 | 0 | 0 | 1120 | 5 | 57000 |
| HelloBasic(viewpage) | 41092 | 9808 | 1240 | 4800 | 0 | 0 | 0 | 0 | 1136 | 4 | 50900 |
| HelloBasic(viewpage_cache) | 40072 | 9780 | 1040 | 4800 | 304 | 304 | 456 | 456 | 1120 | 4 | 49852 |
| HelloBasic(window) | 30324 | 25276 | 1296 | 4800 | 0 | 0 | 0 | 0 | 1088 | 2 | 55600 |
| HelloSimple | 31484 | 23892 | 1124 | 4800 | 0 | 0 | 0 | 0 | 1128 | 3 | 55376 |
| HelloPerformance | 267512 | 1795432 | 708 | 1536 | 0 | 0 | 6644 | 6644 | 432 | 222 | 2062944 |
| HelloShowcase | 167000 | 82380 | 14960 | 40960 | 0 | 0 | 0 | 0 | 2424 | 13 | 249380 |
| HelloStyleDemo | 119756 | 118156 | 6748 | 19200 | 0 | 0 | 0 | 0 | 2592 | 4 | 237912 |
| HelloVirtual(virtual_stage_showcase) | 179660 | 83324 | 8668 | 2400 | 2536 | 2536 | 2648 | 2648 | 2648 | 19 | 262984 |