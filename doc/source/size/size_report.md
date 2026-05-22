# QEMU Size And Memory Report

- Commit: `1fe189eb`
- Date: 2026-05-22T13:56:51.637805
- Build target: `PORT=qemu CPU_ARCH=cortex-m0plus`
- Runtime target: `qemu-system-arm -machine mps2-an385 -cpu cortex-m3`
- Scope: `HelloBasic/*`, `HelloSimple`, `HelloPerformance`, `HelloShowcase`, `HelloStyleDemo`, `HelloVirtual(virtual_stage_showcase)`
- Static size scope: Map input sections from repo-side `src/` + `example/` objects only; exclude toolchain libraries, `driver/`, `porting/`; `ram_bytes` excludes `PFB`, and `pfb_bytes` still comes from each case-local `main.map` `.bss.pfb_area`.
- Successful apps: 77
- Failed apps: 0

## Overview

- Smallest Code: **HelloBasic(divider)** (12488 bytes)
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
| HelloBasic(activity_ring) | 36368 | 5687 | 244 | 4800 | 0 | 0 | 0 | 0 | 1340 | 1 | 42055 |
| HelloBasic(analog_clock) | 25168 | 4549 | 244 | 4800 | 0 | 0 | 0 | 0 | 1128 | 1 | 29717 |
| HelloBasic(anim) | 24516 | 4018 | 532 | 4800 | 0 | 0 | 0 | 0 | 932 | 3 | 28534 |
| HelloBasic(animated_image) | 34208 | 33853 | 180 | 4800 | 0 | 276 | 0 | 276 | 1144 | 1 | 68061 |
| HelloBasic(arc_slider) | 28196 | 5211 | 417 | 4800 | 0 | 0 | 0 | 0 | 1356 | 5 | 33407 |
| HelloBasic(autocomplete) | 31892 | 9763 | 333 | 4800 | 0 | 276 | 276 | 276 | 1336 | 5 | 41655 |
| HelloBasic(button) | 27168 | 18277 | 521 | 4800 | 0 | 276 | 276 | 276 | 1248 | 5 | 45445 |
| HelloBasic(button_img) | 37980 | 37224 | 98 | 4800 | 0 | 68 | 68 | 68 | 844 | 3 | 75204 |
| HelloBasic(button_matrix) | 30660 | 16617 | 183 | 4800 | 0 | 276 | 276 | 276 | 1376 | 7 | 47277 |
| HelloBasic(card) | 33176 | 9811 | 1056 | 4800 | 0 | 276 | 0 | 276 | 1256 | 1 | 42987 |
| HelloBasic(checkbox) | 28616 | 16626 | 469 | 4800 | 0 | 276 | 276 | 276 | 1240 | 5 | 45242 |
| HelloBasic(chips) | 32548 | 14528 | 349 | 4800 | 0 | 276 | 276 | 276 | 1448 | 4 | 47076 |
| HelloBasic(circular_progress_bar) | 36272 | 10481 | 412 | 4800 | 0 | 276 | 0 | 276 | 1344 | 1 | 46753 |
| HelloBasic(combobox) | 31152 | 25108 | 649 | 4800 | 0 | 276 | 276 | 276 | 1336 | 13 | 56260 |
| HelloBasic(compass) | 32828 | 10384 | 236 | 4800 | 0 | 276 | 0 | 276 | 1328 | 1 | 43212 |
| HelloBasic(deferred_image) | 39480 | 19343 | 1557 | 4800 | 178 | 429 | 178 | 429 | 1184 | 3 | 58823 |
| HelloBasic(digital_clock) | 33112 | 9515 | 196 | 4800 | 0 | 276 | 0 | 276 | 1144 | 1 | 42627 |
| HelloBasic(dirty_passthrough_activity) | 44852 | 10613 | 14900 | 4800 | 0 | 276 | 276 | 276 | 1584 | 20 | 55465 |
| HelloBasic(dirty_passthrough_container) | 37344 | 9814 | 1680 | 4800 | 0 | 276 | 276 | 276 | 1400 | 3 | 47158 |
| HelloBasic(dirty_passthrough_page) | 41448 | 10433 | 14569 | 4800 | 0 | 276 | 276 | 276 | 1536 | 14 | 51881 |
| HelloBasic(divider) | 12488 | 363 | 332 | 4800 | 0 | 0 | 0 | 0 | 668 | 1 | 12851 |
| HelloBasic(encoder) | 25056 | 1053 | 436 | 4800 | 0 | 0 | 0 | 0 | 844 | 4 | 26109 |
| HelloBasic(enhanced_widgets) | 65360 | 18707 | 1873 | 4800 | 0 | 276 | 276 | 276 | 1448 | 3 | 84067 |
| HelloBasic(file_image) | 38724 | 12944 | 2196 | 4800 | 32 | 32 | 32 | 32 | 1216 | 1 | 51668 |
| HelloBasic(flexlayout) | 33124 | 9835 | 620 | 4800 | 0 | 276 | 0 | 276 | 1460 | 1 | 42959 |
| HelloBasic(focus_navigation) | 53744 | 21533 | 5321 | 4800 | 0 | 276 | 276 | 276 | 1392 | 30 | 75277 |
| HelloBasic(font_ttf) | 38312 | 7661 | 17180 | 4800 | 0 | 276 | 0 | 276 | 1144 | 1 | 45973 |
| HelloBasic(gauge) | 40276 | 6645 | 448 | 4800 | 0 | 276 | 0 | 276 | 1448 | 1 | 46921 |
| HelloBasic(gridlayout) | 27852 | 3911 | 428 | 4800 | 0 | 0 | 0 | 0 | 1004 | 4 | 31763 |
| HelloBasic(heart_rate) | 33964 | 20431 | 284 | 4800 | 0 | 276 | 276 | 276 | 2456 | 1 | 54395 |
| HelloBasic(image) | 62324 | 134234 | 1760 | 4800 | 2867 | 3631 | 6687 | 6687 | 4128 | 5 | 196558 |
| HelloBasic(image_button) | 26688 | 25130 | 473 | 4800 | 0 | 276 | 276 | 276 | 1248 | 5 | 51818 |
| HelloBasic(label) | 22432 | 6341 | 380 | 4800 | 0 | 276 | 0 | 276 | 1184 | 1 | 28773 |
| HelloBasic(led) | 18736 | 3564 | 444 | 4800 | 0 | 0 | 0 | 0 | 980 | 1 | 22300 |
| HelloBasic(line) | 19020 | 417 | 220 | 4800 | 0 | 0 | 0 | 0 | 1080 | 1 | 19437 |
| HelloBasic(linearlayout) | 32216 | 9559 | 228 | 4800 | 0 | 276 | 0 | 276 | 1184 | 1 | 41775 |
| HelloBasic(list) | 31644 | 15720 | 1738 | 4800 | 0 | 276 | 276 | 276 | 1304 | 6 | 47364 |
| HelloBasic(lyric_scroller) | 41120 | 10027 | 545 | 4800 | 0 | 276 | 276 | 276 | 1240 | 3 | 51147 |
| HelloBasic(mask) | 85616 | 43694 | 564 | 4800 | 0 | 320 | 0 | 320 | 4168 | 1 | 129310 |
| HelloBasic(menu) | 22244 | 11093 | 121 | 4800 | 0 | 276 | 276 | 276 | 1232 | 9 | 33337 |
| HelloBasic(mini_calendar) | 30396 | 9357 | 127 | 4800 | 0 | 276 | 276 | 276 | 1232 | 3 | 39753 |
| HelloBasic(mp4) | 46132 | 3149498 | 1260 | 4800 | 0 | 0 | 764 | 764 | 1304 | 3 | 3195630 |
| HelloBasic(notification_badge) | 25020 | 13892 | 444 | 4800 | 0 | 276 | 0 | 276 | 1208 | 1 | 38912 |
| HelloBasic(number_picker) | 26164 | 21767 | 513 | 4800 | 0 | 276 | 276 | 276 | 1240 | 5 | 47931 |
| HelloBasic(page_indicator) | 35236 | 13393 | 520 | 4800 | 0 | 276 | 276 | 276 | 1280 | 4 | 48629 |
| HelloBasic(pattern_lock) | 35992 | 9801 | 425 | 4800 | 0 | 276 | 276 | 276 | 1256 | 5 | 45793 |
| HelloBasic(progress_bar) | 21020 | 4019 | 413 | 4800 | 0 | 0 | 0 | 0 | 972 | 8 | 25039 |
| HelloBasic(punch_region) | 33492 | 19004 | 1000 | 4800 | 0 | 276 | 276 | 276 | 1248 | 4 | 52496 |
| HelloBasic(radio_button) | 28716 | 21385 | 497 | 4800 | 0 | 276 | 276 | 276 | 1232 | 5 | 50101 |
| HelloBasic(roller) | 22412 | 6191 | 453 | 4800 | 0 | 276 | 276 | 276 | 1216 | 5 | 28603 |
| HelloBasic(rotation) | 26744 | 9667 | 504 | 1600 | 0 | 276 | 276 | 276 | 1216 | 7 | 36411 |
| HelloBasic(scale) | 23648 | 5856 | 172 | 4800 | 0 | 276 | 0 | 276 | 1232 | 1 | 29504 |
| HelloBasic(scroll) | 34624 | 9675 | 400 | 4800 | 0 | 276 | 276 | 276 | 1272 | 2 | 44299 |
| HelloBasic(segmented_control) | 31732 | 16720 | 481 | 4800 | 0 | 276 | 276 | 276 | 1344 | 9 | 48452 |
| HelloBasic(slider) | 20748 | 3785 | 385 | 4800 | 0 | 0 | 0 | 0 | 1012 | 5 | 24533 |
| HelloBasic(spangroup) | 20340 | 5883 | 164 | 4800 | 0 | 276 | 0 | 276 | 1192 | 1 | 26223 |
| HelloBasic(spinner) | 26304 | 4677 | 444 | 4800 | 0 | 0 | 0 | 0 | 1028 | 1 | 30981 |
| HelloBasic(stepper) | 27672 | 14506 | 385 | 4800 | 0 | 276 | 276 | 276 | 1256 | 6 | 42178 |
| HelloBasic(stopwatch) | 34172 | 9562 | 276 | 4800 | 0 | 276 | 0 | 276 | 1184 | 1 | 43734 |
| HelloBasic(style_cascade) | 31696 | 9702 | 620 | 4800 | 0 | 276 | 0 | 276 | 1184 | 1 | 41398 |
| HelloBasic(subject_observer) | 33072 | 9664 | 456 | 4800 | 0 | 276 | 276 | 276 | 1288 | 4 | 42736 |
| HelloBasic(svg) | 36160 | 9500 | 749 | 2400 | 17289 | 22273 | 29266 | 29266 | 2912 | 7 | 45660 |
| HelloBasic(switch) | 28592 | 16076 | 437 | 4800 | 0 | 276 | 276 | 276 | 1224 | 5 | 44668 |
| HelloBasic(tab_bar) | 35956 | 17011 | 544 | 4800 | 0 | 276 | 276 | 276 | 1280 | 4 | 52967 |
| HelloBasic(table) | 24228 | 5901 | 596 | 4800 | 0 | 276 | 0 | 276 | 1208 | 1 | 30129 |
| HelloBasic(textblock) | 38156 | 9394 | 212 | 4800 | 0 | 276 | 0 | 276 | 1160 | 1 | 47550 |
| HelloBasic(textinput) | 45592 | 16381 | 4193 | 4800 | 0 | 276 | 276 | 276 | 1328 | 14 | 61973 |
| HelloBasic(tileview) | 32008 | 9760 | 444 | 4800 | 0 | 276 | 276 | 276 | 1184 | 4 | 41768 |
| HelloBasic(toggle_button) | 26044 | 17819 | 469 | 4800 | 0 | 276 | 276 | 276 | 1240 | 5 | 43863 |
| HelloBasic(viewpage) | 34244 | 9675 | 404 | 4800 | 0 | 276 | 276 | 276 | 1280 | 4 | 43919 |
| HelloBasic(viewpage_cache) | 33692 | 9722 | 180 | 4800 | 352 | 628 | 804 | 804 | 1264 | 4 | 43414 |
| HelloBasic(window) | 25040 | 14410 | 466 | 4800 | 0 | 276 | 276 | 276 | 1224 | 2 | 39450 |
| HelloPerformance | 271112 | 1834709 | 3017 | 1536 | 0 | 0 | 5336 | 5336 | 432 | 8 | 2105821 |
| HelloShowcase | 180428 | 70904 | 23112 | 40960 | 0 | 276 | 276 | 276 | 2552 | 15 | 251332 |
| HelloSimple | 16064 | 2494 | 169 | 2400 | 0 | 0 | 0 | 0 | 908 | 3 | 18558 |
| HelloStyleDemo | 132080 | 102407 | 6378 | 19200 | 0 | 276 | 276 | 276 | 2736 | 4 | 234487 |
| HelloVirtual(virtual_stage_showcase) | 197528 | 72041 | 8735 | 40960 | 3000 | 3276 | 3412 | 3412 | 2744 | 19 | 269569 |