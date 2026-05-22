# Widget Feature QEMU Code Size Report

- Commit: `1fe189eb`
- Date: 2026-05-21T16:08:32.440547
- Build target: `APP=HelloSizeAnalysis APP_SUB=widget_feature_probe PORT=qemu CPU_ARCH=cortex-m0plus`
- Measurement method: compile a dedicated widget probe app, then rewrite the app-local probe config header to force-link a real widget or widget set.
- Scope: widget-level static qemu ELF sections only (`.text/.rodata/.data/.bss`).

## Notes

- This report answers how much ROM and RAM a real widget or widget set costs to bring in.
- The probe uses real widget `init_with_params` and setter paths rather than canvas-only stand-ins.
- Widgets share base/view/theme/canvas paths, so widget-set totals must not be computed by summing single-widget deltas.

## Baseline

| Variant | Text | Rodata | Data | Bss | Total ROM |
|---------|-----:|-------:|-----:|----:|----------:|
| BASELINE | 14540 | 596 | 1368 | 6372 | 15136 |

## Widget Summary

| Widget Or Set | Delta Text | Delta Rodata | Delta Data | Delta Bss | Delta ROM |
|---------------|-----------:|-------------:|-----------:|----------:|----------:|
| SLIDER | +8720 | +784 | +0 | +76 | +9504 |
| SWITCH | +11984 | +1120 | +0 | +96 | +13104 |
| PAGE_INDICATOR | +8412 | +1120 | +0 | +92 | +9532 |
| STEPPER | +8536 | +1124 | +0 | +104 | +9660 |
| CHECKBOX | +12584 | +6328 | +0 | +104 | +18912 |
| RADIO_BUTTON | +11816 | +6328 | +0 | +108 | +18144 |
| PROGRESS_BAR | +7948 | +784 | +0 | +72 | +8732 |
| TOGGLE_BUTTON | +9364 | +6372 | +0 | +104 | +15736 |
| NOTIFICATION_BADGE | +9004 | +6340 | +0 | +100 | +15344 |
| BUTTON | +10424 | +6756 | +0 | +96 | +17180 |
| IMAGE_BUTTON | +9976 | +6392 | +0 | +104 | +16368 |
| CIRCULAR_PROGRESS_BAR | +20360 | +7472 | +0 | +92 | +27832 |
| GAUGE | +28816 | +7528 | +0 | +96 | +36344 |
| ACTIVITY_RING | +25232 | +2712 | +0 | +84 | +27944 |
| ANALOG_CLOCK | +14356 | +1204 | +0 | +84 | +15560 |
| LINE_WIDGET | +8452 | +132 | +0 | +72 | +8584 |
| CHART_LINE | +16192 | +6460 | +0 | +136 | +22652 |
| CHART_PIE | +18068 | +7532 | +0 | +88 | +25600 |
| INDICATOR_WIDGET_SET | +15008 | +1256 | +0 | +352 | +16264 |
| RING_WIDGET_SET | +38900 | +8420 | +0 | +264 | +47320 |
| LINE_VISUAL_WIDGET_SET | +24728 | +7016 | +0 | +292 | +31744 |
| ALL_WIDGET_PROBES | +65300 | +9892 | +0 | +1612 | +75192 |

## Definition

| Widget Or Set | Included Items | Description |
|---------------|----------------|-------------|
| SLIDER | `egui_view_slider` | Real slider widget cost. |
| SWITCH | `egui_view_switch` | Real switch widget cost. |
| PAGE_INDICATOR | `egui_view_page_indicator` | Real page indicator widget cost. |
| STEPPER | `egui_view_stepper` | Real stepper widget cost. |
| CHECKBOX | `egui_view_checkbox` | Real checkbox widget cost. |
| RADIO_BUTTON | `egui_view_radio_button` | Real radio button widget cost. |
| PROGRESS_BAR | `egui_view_progress_bar` | Real progress bar widget cost. |
| TOGGLE_BUTTON | `egui_view_toggle_button` | Real toggle button widget cost. |
| NOTIFICATION_BADGE | `egui_view_notification_badge` | Real notification badge widget cost. |
| BUTTON | `egui_view_button` | Real button widget cost. |
| IMAGE_BUTTON | `egui_view_image_button` | Real image button widget cost. |
| CIRCULAR_PROGRESS_BAR | `egui_view_circular_progress_bar` | Real circular progress bar widget cost. |
| GAUGE | `egui_view_gauge` | Real gauge widget cost. |
| ACTIVITY_RING | `egui_view_activity_ring` | Real activity ring widget cost. |
| ANALOG_CLOCK | `egui_view_analog_clock` | Real analog clock widget cost. |
| LINE_WIDGET | `egui_view_line` | Real line widget cost. |
| CHART_LINE | `egui_view_chart_line` | Real chart-line widget cost. |
| CHART_PIE | `egui_view_chart_pie` | Real chart-pie widget cost. |
| INDICATOR_WIDGET_SET | `slider`<br>`switch`<br>`page_indicator`<br>`stepper` | A common indicator widget set. |
| RING_WIDGET_SET | `circular_progress_bar`<br>`gauge`<br>`activity_ring` | A ring and gauge widget set. |
| LINE_VISUAL_WIDGET_SET | `line`<br>`chart_line`<br>`analog_clock` | A line, clock and data-visual widget set. |
| ALL_WIDGET_PROBES | `all widget probes` | All widget probes enabled together. |

## Detailed Variants

| Variant | Probe Config | Text | Rodata | Data | Bss | Total ROM |
|---------|-------------------|-----:|-------:|-----:|----:|----------:|
| BASELINE | `(none)` | 14540 | 596 | 1368 | 6372 | 15136 |
| SLIDER | `-DEGUI_SIZE_PROBE_WIDGET_SLIDER=1` | 23260 | 1380 | 1368 | 6448 | 24640 |
| SWITCH | `-DEGUI_SIZE_PROBE_WIDGET_SWITCH=1` | 26524 | 1716 | 1368 | 6468 | 28240 |
| PAGE_INDICATOR | `-DEGUI_SIZE_PROBE_WIDGET_PAGE_INDICATOR=1` | 22952 | 1716 | 1368 | 6464 | 24668 |
| STEPPER | `-DEGUI_SIZE_PROBE_WIDGET_STEPPER=1` | 23076 | 1720 | 1368 | 6476 | 24796 |
| CHECKBOX | `-DEGUI_SIZE_PROBE_WIDGET_CHECKBOX=1` | 27124 | 6924 | 1368 | 6476 | 34048 |
| RADIO_BUTTON | `-DEGUI_SIZE_PROBE_WIDGET_RADIO_BUTTON=1` | 26356 | 6924 | 1368 | 6480 | 33280 |
| PROGRESS_BAR | `-DEGUI_SIZE_PROBE_WIDGET_PROGRESS_BAR=1` | 22488 | 1380 | 1368 | 6444 | 23868 |
| TOGGLE_BUTTON | `-DEGUI_SIZE_PROBE_WIDGET_TOGGLE_BUTTON=1` | 23904 | 6968 | 1368 | 6476 | 30872 |
| NOTIFICATION_BADGE | `-DEGUI_SIZE_PROBE_WIDGET_NOTIFICATION_BADGE=1` | 23544 | 6936 | 1368 | 6472 | 30480 |
| BUTTON | `-DEGUI_SIZE_PROBE_WIDGET_BUTTON=1` | 24964 | 7352 | 1368 | 6468 | 32316 |
| IMAGE_BUTTON | `-DEGUI_SIZE_PROBE_WIDGET_IMAGE_BUTTON=1` | 24516 | 6988 | 1368 | 6476 | 31504 |
| CIRCULAR_PROGRESS_BAR | `-DEGUI_SIZE_PROBE_WIDGET_CPB=1` | 34900 | 8068 | 1368 | 6464 | 42968 |
| GAUGE | `-DEGUI_SIZE_PROBE_WIDGET_GAUGE=1` | 43356 | 8124 | 1368 | 6468 | 51480 |
| ACTIVITY_RING | `-DEGUI_SIZE_PROBE_WIDGET_ACTIVITY_RING=1` | 39772 | 3308 | 1368 | 6456 | 43080 |
| ANALOG_CLOCK | `-DEGUI_SIZE_PROBE_WIDGET_ANALOG_CLOCK=1` | 28896 | 1800 | 1368 | 6456 | 30696 |
| LINE_WIDGET | `-DEGUI_SIZE_PROBE_WIDGET_LINE=1` | 22992 | 728 | 1368 | 6444 | 23720 |
| CHART_LINE | `-DEGUI_SIZE_PROBE_WIDGET_CHART_LINE=1` | 30732 | 7056 | 1368 | 6508 | 37788 |
| CHART_PIE | `-DEGUI_SIZE_PROBE_WIDGET_CHART_PIE=1` | 32608 | 8128 | 1368 | 6460 | 40736 |
| INDICATOR_WIDGET_SET | `-DEGUI_SIZE_PROBE_WIDGET_SLIDER=1 -DEGUI_SIZE_PROBE_WIDGET_SWITCH=1 -DEGUI_SIZE_PROBE_WIDGET_PAGE_INDICATOR=1 -DEGUI_SIZE_PROBE_WIDGET_STEPPER=1` | 29548 | 1852 | 1368 | 6724 | 31400 |
| RING_WIDGET_SET | `-DEGUI_SIZE_PROBE_WIDGET_CPB=1 -DEGUI_SIZE_PROBE_WIDGET_GAUGE=1 -DEGUI_SIZE_PROBE_WIDGET_ACTIVITY_RING=1` | 53440 | 9016 | 1368 | 6636 | 62456 |
| LINE_VISUAL_WIDGET_SET | `-DEGUI_SIZE_PROBE_WIDGET_LINE=1 -DEGUI_SIZE_PROBE_WIDGET_CHART_LINE=1 -DEGUI_SIZE_PROBE_WIDGET_ANALOG_CLOCK=1` | 39268 | 7612 | 1368 | 6664 | 46880 |
| ALL_WIDGET_PROBES | `-DEGUI_SIZE_PROBE_WIDGET_SLIDER=1 -DEGUI_SIZE_PROBE_WIDGET_SWITCH=1 -DEGUI_SIZE_PROBE_WIDGET_PAGE_INDICATOR=1 -DEGUI_SIZE_PROBE_WIDGET_STEPPER=1 -DEGUI_SIZE_PROBE_WIDGET_CHECKBOX=1 -DEGUI_SIZE_PROBE_WIDGET_RADIO_BUTTON=1 -DEGUI_SIZE_PROBE_WIDGET_PROGRESS_BAR=1 -DEGUI_SIZE_PROBE_WIDGET_TOGGLE_BUTTON=1 -DEGUI_SIZE_PROBE_WIDGET_NOTIFICATION_BADGE=1 -DEGUI_SIZE_PROBE_WIDGET_BUTTON=1 -DEGUI_SIZE_PROBE_WIDGET_IMAGE_BUTTON=1 -DEGUI_SIZE_PROBE_WIDGET_CPB=1 -DEGUI_SIZE_PROBE_WIDGET_GAUGE=1 -DEGUI_SIZE_PROBE_WIDGET_ACTIVITY_RING=1 -DEGUI_SIZE_PROBE_WIDGET_ANALOG_CLOCK=1 -DEGUI_SIZE_PROBE_WIDGET_LINE=1 -DEGUI_SIZE_PROBE_WIDGET_CHART_LINE=1 -DEGUI_SIZE_PROBE_WIDGET_CHART_PIE=1` | 79840 | 10488 | 1368 | 7984 | 90328 |

## Reproduce

```bash
python scripts/size_analysis/main.py widget-feature-size-to-doc
```

Raw JSON is written to `output/widget_feature_size_results.json`.
