# QEMU Size And Memory Report

- Commit: `82697d75`
- Date: 2026-05-10T19:36:15.745933
- Build target: `PORT=qemu CPU_ARCH=cortex-m0plus`
- Runtime target: `qemu-system-arm -machine mps2-an385 -cpu cortex-m3`
- Scope: `HelloBasic/*`, `HelloSimple`, `HelloPerformance`, `HelloShowcase`, `HelloStyleDemo`, `HelloVirtual(virtual_stage_showcase)`
- Static size scope: Map input sections from repo-side `src/` + `example/` objects only; exclude toolchain libraries, `driver/`, `porting/`; `ram_bytes` excludes `PFB`, and `pfb_bytes` still comes from each case-local `main.map` `.bss.pfb_area`.
- Successful apps: 72
- Failed apps: 0

## Overview

- Smallest Code: **HelloBasic(divider)** (12492 bytes)
- Largest Code: **HelloPerformance** (271112 bytes)
- Smallest RAM: **HelloBasic(button_img)** (98 bytes)
- Largest RAM: **HelloShowcase** (23112 bytes)
- Smallest Heap Peak: **HelloBasic(activity_ring)** (0 bytes)
- Largest Heap Peak: **HelloBasic(svg)** (29266 bytes)
- Smallest Stack Peak: **HelloPerformance** (432 bytes)
- Largest Stack Peak: **HelloBasic(mask)** (4168 bytes)

## Charts

![Size Report](images/size_report.png)

## Detailed Table

| App | Code | Resource | RAM | PFB | Heap Idle | Heap Init Peak | Heap Interaction Peak | Heap Peak | Stack Peak | Actions | Total ROM |
|-----|------|----------|-----|-----|-----------|----------------|-----------------------|-----------|------------|---------|-----------|
| HelloBasic(activity_ring) | 36372 | 5687 | 244 | 4800 | 0 | 0 | 0 | 0 | 1340 | 1 | 42059 |
| HelloBasic(analog_clock) | 25172 | 4549 | 244 | 4800 | 0 | 0 | 0 | 0 | 1128 | 1 | 29721 |
| HelloBasic(anim) | 24520 | 4018 | 532 | 4800 | 0 | 0 | 0 | 0 | 932 | 3 | 28538 |
| HelloBasic(animated_image) | 34212 | 33853 | 180 | 4800 | 0 | 276 | 0 | 276 | 1144 | 1 | 68065 |
| HelloBasic(arc_slider) | 28192 | 5211 | 417 | 4800 | 0 | 0 | 0 | 0 | 1356 | 5 | 33403 |
| HelloBasic(autocomplete) | 31896 | 9763 | 333 | 4800 | 0 | 276 | 276 | 276 | 1336 | 5 | 41659 |
| HelloBasic(button) | 27172 | 18277 | 521 | 4800 | 0 | 276 | 276 | 276 | 1248 | 5 | 45449 |
| HelloBasic(button_img) | 37984 | 37224 | 98 | 4800 | 0 | 68 | 68 | 68 | 844 | 3 | 75208 |
| HelloBasic(button_matrix) | 30656 | 16617 | 183 | 4800 | 0 | 276 | 276 | 276 | 1376 | 7 | 47273 |
| HelloBasic(card) | 33180 | 9811 | 1056 | 4800 | 0 | 276 | 0 | 276 | 1256 | 1 | 42991 |
| HelloBasic(checkbox) | 28620 | 16626 | 469 | 4800 | 0 | 276 | 276 | 276 | 1240 | 5 | 45246 |
| HelloBasic(chips) | 32552 | 14528 | 349 | 4800 | 0 | 276 | 276 | 276 | 1448 | 4 | 47080 |
| HelloBasic(circular_progress_bar) | 36276 | 10481 | 412 | 4800 | 0 | 276 | 0 | 276 | 1344 | 1 | 46757 |
| HelloBasic(combobox) | 31148 | 25108 | 649 | 4800 | 0 | 276 | 276 | 276 | 1336 | 13 | 56256 |
| HelloBasic(compass) | 32832 | 10384 | 236 | 4800 | 0 | 276 | 0 | 276 | 1328 | 1 | 43216 |
| HelloBasic(deferred_image) | 39484 | 19343 | 1557 | 4800 | 178 | 429 | 178 | 429 | 1184 | 3 | 58827 |
| HelloBasic(digital_clock) | 33116 | 9515 | 196 | 4800 | 0 | 276 | 0 | 276 | 1144 | 1 | 42631 |
| HelloBasic(dirty_passthrough_activity) | 44852 | 10613 | 14900 | 4800 | 0 | 276 | 276 | 276 | 1584 | 20 | 55465 |
| HelloBasic(dirty_passthrough_container) | 37344 | 9814 | 1680 | 4800 | 0 | 276 | 276 | 276 | 1400 | 3 | 47158 |
| HelloBasic(dirty_passthrough_page) | 41448 | 10433 | 14569 | 4800 | 0 | 276 | 276 | 276 | 1536 | 14 | 51881 |
| HelloBasic(divider) | 12492 | 363 | 332 | 4800 | 0 | 0 | 0 | 0 | 668 | 1 | 12855 |
| HelloBasic(enhanced_widgets) | 65364 | 18707 | 1873 | 4800 | 0 | 276 | 276 | 276 | 1448 | 3 | 84071 |
| HelloBasic(file_image) | 38728 | 12944 | 2196 | 4800 | 32 | 32 | 32 | 32 | 1216 | 1 | 51672 |
| HelloBasic(focus_navigation) | 53656 | 21533 | 5321 | 4800 | 0 | 276 | 276 | 276 | 1392 | 30 | 75189 |
| HelloBasic(gauge) | 40280 | 6645 | 448 | 4800 | 0 | 276 | 0 | 276 | 1448 | 1 | 46925 |
| HelloBasic(gridlayout) | 27856 | 3911 | 428 | 4800 | 0 | 0 | 0 | 0 | 1004 | 4 | 31767 |
| HelloBasic(heart_rate) | 33968 | 20431 | 284 | 4800 | 0 | 276 | 276 | 276 | 2456 | 1 | 54399 |
| HelloBasic(image) | 62324 | 134234 | 1760 | 4800 | 2867 | 3631 | 6687 | 6687 | 4128 | 5 | 196558 |
| HelloBasic(image_button) | 26692 | 25130 | 473 | 4800 | 0 | 276 | 276 | 276 | 1248 | 5 | 51822 |
| HelloBasic(label) | 22436 | 6341 | 380 | 4800 | 0 | 276 | 0 | 276 | 1184 | 1 | 28777 |
| HelloBasic(led) | 18740 | 3564 | 444 | 4800 | 0 | 0 | 0 | 0 | 980 | 1 | 22304 |
| HelloBasic(line) | 19024 | 417 | 220 | 4800 | 0 | 0 | 0 | 0 | 1080 | 1 | 19441 |
| HelloBasic(linearlayout) | 32220 | 9559 | 228 | 4800 | 0 | 276 | 0 | 276 | 1184 | 1 | 41779 |
| HelloBasic(list) | 31644 | 15720 | 1738 | 4800 | 0 | 276 | 276 | 276 | 1304 | 6 | 47364 |
| HelloBasic(lyric_scroller) | 41124 | 10027 | 545 | 4800 | 0 | 276 | 276 | 276 | 1240 | 3 | 51151 |
| HelloBasic(mask) | 85620 | 43694 | 564 | 4800 | 0 | 320 | 0 | 320 | 4168 | 1 | 129314 |
| HelloBasic(menu) | 22248 | 11093 | 121 | 4800 | 0 | 276 | 276 | 276 | 1232 | 9 | 33341 |
| HelloBasic(mini_calendar) | 30400 | 9357 | 127 | 4800 | 0 | 276 | 276 | 276 | 1232 | 3 | 39757 |
| HelloBasic(mp4) | 46132 | 3149498 | 1260 | 4800 | 0 | 0 | 764 | 764 | 1304 | 3 | 3195630 |
| HelloBasic(notification_badge) | 25024 | 13892 | 444 | 4800 | 0 | 276 | 0 | 276 | 1208 | 1 | 38916 |
| HelloBasic(number_picker) | 26160 | 21767 | 513 | 4800 | 0 | 276 | 276 | 276 | 1240 | 5 | 47927 |
| HelloBasic(page_indicator) | 35240 | 13393 | 520 | 4800 | 0 | 276 | 276 | 276 | 1280 | 4 | 48633 |
| HelloBasic(pattern_lock) | 35988 | 9801 | 425 | 4800 | 0 | 276 | 276 | 276 | 1256 | 5 | 45789 |
| HelloBasic(progress_bar) | 21024 | 4019 | 413 | 4800 | 0 | 0 | 0 | 0 | 972 | 8 | 25043 |
| HelloBasic(punch_region) | 33496 | 19004 | 1000 | 4800 | 0 | 276 | 276 | 276 | 1248 | 4 | 52500 |
| HelloBasic(radio_button) | 28720 | 21385 | 497 | 4800 | 0 | 276 | 276 | 276 | 1232 | 5 | 50105 |
| HelloBasic(roller) | 22408 | 6191 | 453 | 4800 | 0 | 276 | 276 | 276 | 1216 | 5 | 28599 |
| HelloBasic(rotation) | 26744 | 9667 | 504 | 1600 | 0 | 276 | 276 | 276 | 1216 | 7 | 36411 |
| HelloBasic(scale) | 23652 | 5856 | 172 | 4800 | 0 | 276 | 0 | 276 | 1232 | 1 | 29508 |
| HelloBasic(scroll) | 34628 | 9675 | 400 | 4800 | 0 | 276 | 276 | 276 | 1272 | 2 | 44303 |
| HelloBasic(segmented_control) | 31728 | 16720 | 481 | 4800 | 0 | 276 | 276 | 276 | 1344 | 9 | 48448 |
| HelloBasic(slider) | 20744 | 3785 | 385 | 4800 | 0 | 0 | 0 | 0 | 1012 | 5 | 24529 |
| HelloBasic(spangroup) | 20344 | 5883 | 164 | 4800 | 0 | 276 | 0 | 276 | 1192 | 1 | 26227 |
| HelloBasic(spinner) | 26308 | 4677 | 444 | 4800 | 0 | 0 | 0 | 0 | 1028 | 1 | 30985 |
| HelloBasic(stepper) | 27668 | 14506 | 385 | 4800 | 0 | 276 | 276 | 276 | 1256 | 6 | 42174 |
| HelloBasic(stopwatch) | 34176 | 9562 | 276 | 4800 | 0 | 276 | 0 | 276 | 1184 | 1 | 43738 |
| HelloBasic(svg) | 36164 | 9500 | 749 | 2400 | 17289 | 22273 | 29266 | 29266 | 2912 | 7 | 45664 |
| HelloBasic(switch) | 28596 | 16076 | 437 | 4800 | 0 | 276 | 276 | 276 | 1224 | 5 | 44672 |
| HelloBasic(tab_bar) | 35960 | 17011 | 544 | 4800 | 0 | 276 | 276 | 276 | 1280 | 4 | 52971 |
| HelloBasic(table) | 24232 | 5901 | 596 | 4800 | 0 | 276 | 0 | 276 | 1208 | 1 | 30133 |
| HelloBasic(textblock) | 38160 | 9394 | 212 | 4800 | 0 | 276 | 0 | 276 | 1160 | 1 | 47554 |
| HelloBasic(textinput) | 45524 | 16381 | 4193 | 4800 | 0 | 276 | 276 | 276 | 1328 | 14 | 61905 |
| HelloBasic(tileview) | 32012 | 9760 | 444 | 4800 | 0 | 276 | 276 | 276 | 1184 | 4 | 41772 |
| HelloBasic(toggle_button) | 26040 | 17819 | 469 | 4800 | 0 | 276 | 276 | 276 | 1240 | 5 | 43859 |
| HelloBasic(viewpage) | 34248 | 9675 | 404 | 4800 | 0 | 276 | 276 | 276 | 1280 | 4 | 43923 |
| HelloBasic(viewpage_cache) | 33696 | 9722 | 180 | 4800 | 352 | 628 | 804 | 804 | 1264 | 4 | 43418 |
| HelloBasic(window) | 25044 | 14410 | 466 | 4800 | 0 | 276 | 276 | 276 | 1224 | 2 | 39454 |
| HelloPerformance | 271112 | 1834709 | 3017 | 1536 | 0 | 0 | 5336 | 5336 | 432 | 8 | 2105821 |
| HelloShowcase | 180328 | 70900 | 23112 | 40960 | 0 | 276 | 276 | 276 | 2552 | 15 | 251228 |
| HelloSimple | 16068 | 2494 | 169 | 2400 | 0 | 0 | 0 | 0 | 908 | 3 | 18562 |
| HelloStyleDemo | 132080 | 102407 | 6378 | 19200 | 0 | 276 | 276 | 276 | 2736 | 4 | 234487 |
| HelloVirtual(virtual_stage_showcase) | 197368 | 72037 | 8735 | 40960 | 3000 | 3276 | 3412 | 3412 | 2744 | 19 | 269405 |