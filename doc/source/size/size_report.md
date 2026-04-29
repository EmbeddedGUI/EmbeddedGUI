# QEMU Size And Memory Report

- Commit: `db223c18`
- Date: 2026-04-29T18:26:08.664646
- Build target: `PORT=qemu CPU_ARCH=cortex-m0plus`
- Runtime target: `qemu-system-arm -machine mps2-an385 -cpu cortex-m3`
- Scope: `HelloBasic/*`, `HelloSimple`, `HelloPerformance`, `HelloShowcase`, `HelloStyleDemo`, `HelloVirtual(virtual_stage_showcase)`
- Static size scope: Map input sections from repo-side `src/` + `example/` objects only; exclude toolchain libraries, `driver/`, `porting/`; `ram_bytes` excludes `PFB`, and `pfb_bytes` still comes from each case-local `main.map` `.bss.pfb_area`.
- Successful apps: 70
- Failed apps: 0

## Overview

- Smallest Code: **HelloBasic(divider)** (11660 bytes)
- Largest Code: **HelloPerformance** (270924 bytes)
- Smallest RAM: **HelloBasic(button_img)** (98 bytes)
- Largest RAM: **HelloBasic(dirty_passthrough_activity)** (14900 bytes)
- Smallest Heap Peak: **HelloBasic(activity_ring)** (0 bytes)
- Largest Heap Peak: **HelloBasic(svg)** (29266 bytes)
- Smallest Stack Peak: **HelloPerformance** (432 bytes)
- Largest Stack Peak: **HelloBasic(mask)** (4080 bytes)

## Charts

![Size Report](images/size_report.png)

## Detailed Table

| App | Code | Resource | RAM | PFB | Heap Idle | Heap Init Peak | Heap Interaction Peak | Heap Peak | Stack Peak | Actions | Total ROM |
|-----|------|----------|-----|-----|-----------|----------------|-----------------------|-----------|------------|---------|-----------|
| HelloBasic(activity_ring) | 36340 | 5723 | 244 | 4800 | 0 | 0 | 0 | 0 | 1244 | 1 | 42063 |
| HelloBasic(analog_clock) | 24644 | 4559 | 244 | 4800 | 0 | 0 | 0 | 0 | 1064 | 1 | 29203 |
| HelloBasic(anim) | 23952 | 4018 | 532 | 4800 | 0 | 0 | 0 | 0 | 836 | 3 | 27970 |
| HelloBasic(animated_image) | 33320 | 33853 | 180 | 4800 | 0 | 276 | 0 | 276 | 1056 | 1 | 67173 |
| HelloBasic(arc_slider) | 27652 | 5217 | 417 | 4800 | 0 | 0 | 0 | 0 | 1260 | 5 | 32869 |
| HelloBasic(autocomplete) | 31352 | 9781 | 333 | 4800 | 0 | 276 | 276 | 276 | 1248 | 5 | 41133 |
| HelloBasic(button) | 26284 | 18289 | 521 | 4800 | 0 | 276 | 276 | 276 | 1168 | 5 | 44573 |
| HelloBasic(button_img) | 37088 | 37236 | 98 | 4800 | 0 | 68 | 68 | 68 | 748 | 3 | 74324 |
| HelloBasic(button_matrix) | 30072 | 16627 | 183 | 4800 | 0 | 276 | 276 | 276 | 1288 | 7 | 46699 |
| HelloBasic(card) | 32572 | 9817 | 1056 | 4800 | 0 | 276 | 0 | 276 | 1168 | 1 | 42389 |
| HelloBasic(checkbox) | 27736 | 16640 | 469 | 4800 | 0 | 276 | 276 | 276 | 1152 | 5 | 44376 |
| HelloBasic(chips) | 31960 | 14548 | 349 | 4800 | 0 | 276 | 276 | 276 | 1360 | 4 | 46508 |
| HelloBasic(circular_progress_bar) | 35680 | 10509 | 412 | 4800 | 0 | 276 | 0 | 276 | 1256 | 1 | 46189 |
| HelloBasic(combobox) | 30624 | 25126 | 649 | 4800 | 0 | 276 | 276 | 276 | 1248 | 13 | 55750 |
| HelloBasic(compass) | 32244 | 10394 | 236 | 4800 | 0 | 276 | 0 | 276 | 1240 | 1 | 42638 |
| HelloBasic(deferred_image) | 38880 | 19365 | 1557 | 4800 | 178 | 429 | 178 | 429 | 1096 | 3 | 58245 |
| HelloBasic(digital_clock) | 32516 | 9517 | 196 | 4800 | 0 | 276 | 0 | 276 | 1056 | 1 | 42033 |
| HelloBasic(dirty_passthrough_activity) | 45160 | 11779 | 14900 | 4800 | 0 | 276 | 276 | 276 | 1480 | 20 | 56939 |
| HelloBasic(dirty_passthrough_container) | 37656 | 10968 | 1680 | 4800 | 0 | 276 | 276 | 276 | 1336 | 3 | 48624 |
| HelloBasic(dirty_passthrough_page) | 41760 | 11599 | 14569 | 4800 | 0 | 276 | 276 | 276 | 1456 | 14 | 53359 |
| HelloBasic(divider) | 11660 | 363 | 332 | 4800 | 0 | 0 | 0 | 0 | 580 | 1 | 12023 |
| HelloBasic(enhanced_widgets) | 64760 | 18835 | 1873 | 4800 | 0 | 276 | 276 | 276 | 1360 | 3 | 83595 |
| HelloBasic(file_image) | 38128 | 12952 | 2196 | 4800 | 32 | 32 | 32 | 32 | 1096 | 1 | 51080 |
| HelloBasic(gauge) | 39404 | 6661 | 448 | 4800 | 0 | 276 | 0 | 276 | 1360 | 1 | 46065 |
| HelloBasic(gridlayout) | 27272 | 3929 | 428 | 4800 | 0 | 0 | 0 | 0 | 908 | 4 | 31201 |
| HelloBasic(heart_rate) | 33084 | 20455 | 284 | 4800 | 0 | 276 | 276 | 276 | 2360 | 1 | 53539 |
| HelloBasic(image) | 61456 | 134236 | 1760 | 4800 | 2867 | 3631 | 6687 | 6687 | 4040 | 5 | 195692 |
| HelloBasic(image_button) | 25724 | 25144 | 473 | 4800 | 0 | 276 | 276 | 276 | 1160 | 5 | 50868 |
| HelloBasic(label) | 21548 | 6341 | 380 | 4800 | 0 | 276 | 0 | 276 | 1096 | 1 | 27889 |
| HelloBasic(led) | 18208 | 3576 | 444 | 4800 | 0 | 0 | 0 | 0 | 884 | 1 | 21784 |
| HelloBasic(line) | 18188 | 417 | 220 | 4800 | 0 | 0 | 0 | 0 | 968 | 1 | 18605 |
| HelloBasic(linearlayout) | 31620 | 9559 | 228 | 4800 | 0 | 276 | 0 | 276 | 1096 | 1 | 41179 |
| HelloBasic(list) | 30504 | 15742 | 1734 | 4800 | 0 | 276 | 276 | 276 | 1312 | 6 | 46246 |
| HelloBasic(lyric_scroller) | 40236 | 10029 | 545 | 4800 | 0 | 276 | 276 | 276 | 1152 | 3 | 50265 |
| HelloBasic(mask) | 84732 | 43714 | 564 | 4800 | 0 | 320 | 0 | 320 | 4080 | 1 | 128446 |
| HelloBasic(menu) | 21292 | 11107 | 121 | 4800 | 0 | 276 | 276 | 276 | 1144 | 9 | 32399 |
| HelloBasic(mini_calendar) | 29828 | 9379 | 127 | 4800 | 0 | 276 | 276 | 276 | 1144 | 3 | 39207 |
| HelloBasic(mp4) | 45268 | 3149500 | 1260 | 4800 | 0 | 0 | 764 | 764 | 1216 | 3 | 3194768 |
| HelloBasic(notification_badge) | 24136 | 13916 | 444 | 4800 | 0 | 276 | 0 | 276 | 1120 | 1 | 38052 |
| HelloBasic(number_picker) | 25272 | 21791 | 513 | 4800 | 0 | 276 | 276 | 276 | 1152 | 5 | 47063 |
| HelloBasic(page_indicator) | 34648 | 13403 | 520 | 4800 | 0 | 276 | 276 | 276 | 1192 | 4 | 48051 |
| HelloBasic(pattern_lock) | 35460 | 9861 | 425 | 4800 | 0 | 276 | 276 | 276 | 1176 | 5 | 45321 |
| HelloBasic(progress_bar) | 18420 | 3574 | 364 | 4800 | 0 | 0 | 0 | 0 | 844 | 1 | 21994 |
| HelloBasic(radio_button) | 28116 | 21399 | 497 | 4800 | 0 | 276 | 276 | 276 | 1144 | 5 | 49515 |
| HelloBasic(roller) | 21528 | 6201 | 453 | 4800 | 0 | 276 | 276 | 276 | 1128 | 5 | 27729 |
| HelloBasic(rotation) | 25856 | 9679 | 504 | 1600 | 0 | 276 | 276 | 276 | 1136 | 7 | 35535 |
| HelloBasic(scale) | 22772 | 5866 | 172 | 4800 | 0 | 276 | 0 | 276 | 1144 | 1 | 28638 |
| HelloBasic(scroll) | 33992 | 9677 | 400 | 4800 | 0 | 276 | 276 | 276 | 1184 | 2 | 43669 |
| HelloBasic(segmented_control) | 31196 | 16772 | 481 | 4800 | 0 | 276 | 276 | 276 | 1256 | 9 | 47968 |
| HelloBasic(slider) | 20200 | 3795 | 385 | 4800 | 0 | 0 | 0 | 0 | 924 | 5 | 23995 |
| HelloBasic(spangroup) | 19452 | 5897 | 164 | 4800 | 0 | 276 | 0 | 276 | 1104 | 1 | 25349 |
| HelloBasic(spinner) | 25772 | 4679 | 444 | 4800 | 0 | 0 | 0 | 0 | 940 | 1 | 30451 |
| HelloBasic(stepper) | 26724 | 14527 | 385 | 4800 | 0 | 276 | 276 | 276 | 1176 | 6 | 41251 |
| HelloBasic(stopwatch) | 33572 | 9564 | 276 | 4800 | 0 | 276 | 0 | 276 | 1096 | 1 | 43136 |
| HelloBasic(svg) | 35320 | 9502 | 749 | 2400 | 17289 | 22273 | 29266 | 29266 | 2832 | 7 | 44822 |
| HelloBasic(switch) | 28012 | 16094 | 437 | 4800 | 0 | 276 | 276 | 276 | 1136 | 5 | 44106 |
| HelloBasic(tab_bar) | 35400 | 17019 | 544 | 4800 | 0 | 276 | 276 | 276 | 1192 | 4 | 52419 |
| HelloBasic(table) | 23368 | 5911 | 596 | 4800 | 0 | 276 | 0 | 276 | 1120 | 1 | 29279 |
| HelloBasic(textblock) | 37284 | 9410 | 212 | 4800 | 0 | 276 | 0 | 276 | 1104 | 1 | 46694 |
| HelloBasic(textinput) | 40828 | 16004 | 3722 | 4800 | 0 | 276 | 276 | 276 | 1200 | 9 | 56832 |
| HelloBasic(tileview) | 31412 | 9760 | 444 | 4800 | 0 | 276 | 276 | 276 | 1096 | 4 | 41172 |
| HelloBasic(toggle_button) | 25152 | 17843 | 469 | 4800 | 0 | 276 | 276 | 276 | 1152 | 5 | 42995 |
| HelloBasic(viewpage) | 33640 | 9677 | 404 | 4800 | 0 | 276 | 276 | 276 | 1192 | 4 | 43317 |
| HelloBasic(viewpage_cache) | 33092 | 9726 | 180 | 4800 | 352 | 628 | 804 | 804 | 1176 | 4 | 42818 |
| HelloBasic(window) | 24160 | 14418 | 466 | 4800 | 0 | 276 | 276 | 276 | 1136 | 2 | 38578 |
| HelloPerformance | 270924 | 1834759 | 3017 | 1536 | 0 | 0 | 5336 | 5336 | 432 | 8 | 2105683 |
| HelloShowcase | 166648 | 70159 | 14608 | 40960 | 0 | 276 | 276 | 276 | 2440 | 15 | 236807 |
| HelloSimple | 15340 | 2506 | 169 | 2400 | 0 | 0 | 0 | 0 | 820 | 3 | 17846 |
| HelloStyleDemo | 127904 | 102545 | 5906 | 19200 | 0 | 276 | 276 | 276 | 2640 | 4 | 230449 |
| HelloVirtual(virtual_stage_showcase) | 190044 | 72303 | 8127 | 40960 | 2852 | 3128 | 3256 | 3256 | 2672 | 19 | 262347 |