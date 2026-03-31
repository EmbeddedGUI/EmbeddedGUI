# QEMU Size And Memory Report

- Commit: `9943a46`
- Date: 2026-03-31T16:54:53.533712
- Build target: `PORT=qemu CPU_ARCH=cortex-m0plus`
- Runtime target: `qemu-system-arm -machine mps2-an385 -cpu cortex-m3`
- Scope: HelloBasic 全子 case、HelloSimple、HelloPerformance、HelloShowcase、HelloStyleDemo、HelloVirtual(virtual_stage_showcase)
- Successful apps: 64
- Failed apps: 0

## Overview

- Smallest Code: **HelloBasic(divider)** (14980 bytes)
- Largest Code: **HelloPerformance** (254064 bytes)
- Smallest RAM: **HelloBasic(menu)** (880 bytes)
- Largest RAM: **HelloShowcase** (14848 bytes)
- Smallest Heap Peak: **HelloBasic(activity_ring)** (0 bytes)
- Largest Heap Peak: **HelloPerformance** (7468 bytes)
- Smallest Stack Peak: **HelloPerformance** (432 bytes)
- Largest Stack Peak: **HelloBasic(mask)** (4092 bytes)

## Charts

![Size Report](images/size_report.png)

## Detailed Table

| App | Code | Resource | RAM | PFB | Heap Idle | Heap Init Peak | Heap Interaction Peak | Heap Peak | Stack Peak | Actions | Total ROM |
|-----|------|----------|-----|-----|-----------|----------------|-----------------------|-----------|------------|---------|-----------|
| HelloBasic(activity_ring) | 46340 | 6472 | 956 | 4800 | 0 | 0 | 0 | 0 | 1200 | 1 | 52812 |
| HelloBasic(analog_clock) | 33452 | 4968 | 956 | 4800 | 0 | 0 | 0 | 0 | 1084 | 1 | 38420 |
| HelloBasic(anim) | 29188 | 4468 | 1212 | 4800 | 0 | 0 | 0 | 0 | 752 | 3 | 33656 |
| HelloBasic(animated_image) | 32780 | 33916 | 972 | 4800 | 0 | 0 | 0 | 0 | 936 | 1 | 66696 |
| HelloBasic(arc_slider) | 35752 | 5440 | 1108 | 4800 | 0 | 0 | 0 | 0 | 1208 | 5 | 41192 |
| HelloBasic(autocomplete) | 35764 | 9788 | 1496 | 4800 | 0 | 0 | 0 | 0 | 1112 | 5 | 45552 |
| HelloBasic(button) | 26492 | 18104 | 1244 | 4800 | 0 | 0 | 0 | 0 | 1040 | 5 | 44596 |
| HelloBasic(button_img) | 37200 | 37212 | 896 | 4800 | 0 | 60 | 60 | 60 | 760 | 3 | 74412 |
| HelloBasic(button_matrix) | 32948 | 16804 | 944 | 4800 | 0 | 0 | 0 | 0 | 1160 | 4 | 49752 |
| HelloBasic(card) | 35140 | 9944 | 1732 | 4800 | 0 | 0 | 0 | 0 | 1040 | 1 | 45084 |
| HelloBasic(checkbox) | 35172 | 16832 | 1196 | 4800 | 0 | 0 | 0 | 0 | 1024 | 5 | 52004 |
| HelloBasic(chips) | 36432 | 14668 | 1512 | 4800 | 0 | 0 | 0 | 0 | 1192 | 4 | 51100 |
| HelloBasic(circular_progress_bar) | 42300 | 18844 | 1128 | 4800 | 0 | 0 | 0 | 0 | 1216 | 1 | 61144 |
| HelloBasic(combobox) | 33560 | 25372 | 1376 | 4800 | 0 | 0 | 0 | 0 | 1112 | 13 | 58932 |
| HelloBasic(compass) | 37184 | 10836 | 984 | 4800 | 0 | 0 | 0 | 0 | 1104 | 1 | 48020 |
| HelloBasic(digital_clock) | 34804 | 9668 | 952 | 4800 | 0 | 0 | 0 | 0 | 936 | 1 | 44472 |
| HelloBasic(divider) | 14980 | 732 | 1028 | 4800 | 0 | 0 | 0 | 0 | 752 | 1 | 15712 |
| HelloBasic(enhanced_widgets) | 67056 | 19264 | 2484 | 4800 | 0 | 0 | 0 | 0 | 1224 | 3 | 86320 |
| HelloBasic(gauge) | 42500 | 14564 | 1168 | 4800 | 0 | 0 | 0 | 0 | 1112 | 1 | 57064 |
| HelloBasic(gridlayout) | 30588 | 4364 | 1164 | 4800 | 0 | 0 | 0 | 0 | 872 | 4 | 34952 |
| HelloBasic(heart_rate) | 37656 | 20900 | 1032 | 4800 | 0 | 0 | 0 | 0 | 2312 | 1 | 58556 |
| HelloBasic(image) | 49364 | 72632 | 1932 | 4800 | 824 | 1724 | 1804 | 1804 | 1080 | 3 | 121996 |
| HelloBasic(image_button) | 32504 | 25304 | 1192 | 4800 | 0 | 0 | 0 | 0 | 1048 | 5 | 57808 |
| HelloBasic(label) | 33452 | 4968 | 956 | 4800 | 0 | 0 | 0 | 0 | 968 | 1 | 38420 |
| HelloBasic(led) | 23544 | 4020 | 1140 | 4800 | 0 | 0 | 0 | 0 | 856 | 1 | 27564 |
| HelloBasic(line) | 22844 | 800 | 932 | 4800 | 0 | 0 | 0 | 0 | 936 | 1 | 23644 |
| HelloBasic(linearlayout) | 33932 | 9664 | 976 | 4800 | 0 | 0 | 0 | 0 | 968 | 1 | 43596 |
| HelloBasic(list) | 29400 | 14652 | 2356 | 4800 | 0 | 0 | 0 | 0 | 1008 | 6 | 44052 |
| HelloBasic(mask) | 92364 | 43972 | 1364 | 4800 | 0 | 28 | 0 | 28 | 4092 | 1 | 136336 |
| HelloBasic(menu) | 21712 | 11220 | 880 | 4800 | 0 | 0 | 0 | 0 | 1016 | 5 | 32932 |
| HelloBasic(mini_calendar) | 32756 | 9652 | 888 | 4800 | 0 | 0 | 0 | 0 | 1024 | 3 | 42408 |
| HelloBasic(mp4) | 45396 | 3149652 | 2012 | 4800 | 0 | 0 | 1724 | 1724 | 1064 | 3 | 3195048 |
| HelloBasic(notification_badge) | 25936 | 14364 | 1176 | 4800 | 0 | 0 | 0 | 0 | 1008 | 1 | 40300 |
| HelloBasic(number_picker) | 27140 | 21936 | 1240 | 4800 | 0 | 0 | 0 | 0 | 1032 | 5 | 49076 |
| HelloBasic(page_indicator) | 37004 | 13564 | 1240 | 4800 | 0 | 0 | 0 | 0 | 1048 | 4 | 50568 |
| HelloBasic(pattern_lock) | 40692 | 9940 | 1588 | 4800 | 0 | 0 | 0 | 0 | 1040 | 5 | 50632 |
| HelloBasic(progress_bar) | 23852 | 4024 | 1064 | 4800 | 0 | 0 | 0 | 0 | 760 | 1 | 27876 |
| HelloBasic(radio_button) | 30880 | 21600 | 1224 | 4800 | 0 | 0 | 0 | 0 | 1040 | 5 | 52480 |
| HelloBasic(roller) | 22056 | 6328 | 1176 | 4800 | 0 | 0 | 0 | 0 | 992 | 5 | 28384 |
| HelloBasic(rotation) | 28860 | 9848 | 2452 | 1600 | 0 | 0 | 0 | 0 | 1000 | 7 | 38708 |
| HelloBasic(scale) | 24696 | 6236 | 928 | 4800 | 0 | 0 | 0 | 0 | 1024 | 1 | 30932 |
| HelloBasic(scroll) | 35716 | 9804 | 1136 | 4800 | 0 | 0 | 0 | 0 | 1064 | 2 | 45520 |
| HelloBasic(segmented_control) | 33552 | 16948 | 1216 | 4800 | 0 | 0 | 0 | 0 | 1128 | 9 | 50500 |
| HelloBasic(slider) | 25016 | 4020 | 1064 | 4800 | 0 | 0 | 0 | 0 | 864 | 5 | 29036 |
| HelloBasic(spangroup) | 20388 | 6268 | 928 | 4800 | 0 | 0 | 0 | 0 | 888 | 1 | 26656 |
| HelloBasic(spinner) | 34328 | 5416 | 1140 | 4800 | 0 | 0 | 0 | 0 | 876 | 1 | 39744 |
| HelloBasic(stepper) | 35640 | 14704 | 1548 | 4800 | 0 | 0 | 0 | 0 | 1040 | 6 | 50344 |
| HelloBasic(stopwatch) | 35788 | 9688 | 1024 | 4800 | 0 | 0 | 0 | 0 | 968 | 1 | 45476 |
| HelloBasic(switch) | 30752 | 16312 | 1164 | 4800 | 0 | 0 | 0 | 0 | 1016 | 5 | 47064 |
| HelloBasic(tab_bar) | 37324 | 17140 | 1272 | 4800 | 0 | 0 | 0 | 0 | 1048 | 4 | 54464 |
| HelloBasic(table) | 25268 | 6280 | 1360 | 4800 | 0 | 0 | 0 | 0 | 1008 | 1 | 31548 |
| HelloBasic(textblock) | 30912 | 9852 | 960 | 4800 | 0 | 0 | 0 | 0 | 872 | 1 | 40764 |
| HelloBasic(textinput) | 41288 | 15816 | 4268 | 4800 | 0 | 0 | 0 | 0 | 1056 | 9 | 57104 |
| HelloBasic(thermostat) | 34212 | 46996 | 1280 | 6400 | 0 | 0 | 0 | 0 | 992 | 6 | 81208 |
| HelloBasic(tileview) | 33668 | 9864 | 1176 | 4800 | 0 | 0 | 0 | 0 | 968 | 4 | 43532 |
| HelloBasic(toggle_button) | 26368 | 18064 | 1192 | 4800 | 0 | 0 | 0 | 0 | 1032 | 5 | 44432 |
| HelloBasic(viewpage) | 35764 | 9804 | 1136 | 4800 | 0 | 0 | 0 | 0 | 1048 | 4 | 45568 |
| HelloBasic(viewpage_cache) | 34744 | 9776 | 936 | 4800 | 304 | 304 | 456 | 456 | 1032 | 4 | 44520 |
| HelloBasic(window) | 24908 | 14444 | 1192 | 4800 | 0 | 0 | 0 | 0 | 1000 | 2 | 39352 |
| HelloSimple | 22432 | 7108 | 1020 | 4800 | 0 | 0 | 0 | 0 | 788 | 3 | 29540 |
| HelloPerformance | 254064 | 1795432 | 1780 | 1536 | 0 | 0 | 7468 | 7468 | 432 | 222 | 2049496 |
| HelloShowcase | 161244 | 65592 | 14848 | 40960 | 0 | 0 | 0 | 0 | 2384 | 13 | 226836 |
| HelloStyleDemo | 114280 | 102568 | 6636 | 19200 | 0 | 0 | 0 | 0 | 2552 | 4 | 216848 |
| HelloVirtual(virtual_stage_showcase) | 173916 | 66536 | 8556 | 2400 | 2536 | 2536 | 2648 | 2648 | 2608 | 19 | 240452 |