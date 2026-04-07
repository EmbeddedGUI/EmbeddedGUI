# Widget Feature QEMU Code Size Report

- Commit: `21b6890`
- Date: 2026-04-05T08:31:12.386532
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
| BASELINE | 11660 | 568 | 24 | 6024 | 12228 |

## Widget Summary

| Widget Or Set | Delta Text | Delta Rodata | Delta Data | Delta Bss | Delta ROM |
|---------------|-----------:|-------------:|-----------:|----------:|----------:|
| SLIDER | +7444 | +816 | +0 | +76 | +8260 |
| SWITCH | +6540 | +824 | +0 | +92 | +7364 |
| PAGE_INDICATOR | +3120 | +820 | +0 | +80 | +3940 |
| STEPPER | +3268 | +892 | +0 | +96 | +4160 |
| CHECKBOX | +6936 | +812 | +0 | +100 | +7748 |
| RADIO_BUTTON | +6132 | +816 | +0 | +96 | +6948 |
| PROGRESS_BAR | +6972 | +820 | +0 | +76 | +7792 |
| TOGGLE_BUTTON | +4492 | +1196 | +0 | +96 | +5688 |
| NOTIFICATION_BADGE | +4500 | +1156 | +0 | +88 | +5656 |
| BUTTON | +4696 | +1192 | +0 | +92 | +5888 |
| IMAGE_BUTTON | +5892 | +1188 | +0 | +96 | +7080 |
| CIRCULAR_PROGRESS_BAR | +13208 | +1724 | +0 | +80 | +14932 |
| GAUGE | +17020 | +1712 | +0 | +88 | +18732 |
| ACTIVITY_RING | +11844 | +1380 | +0 | +80 | +13224 |
| ANALOG_CLOCK | +9200 | +1004 | +0 | +80 | +10204 |
| LINE_WIDGET | +5972 | +820 | +0 | +72 | +6792 |
| CHART_LINE | +9160 | +1300 | +0 | +128 | +10460 |
| CHART_PIE | +7380 | +1576 | +0 | +80 | +8956 |
| INDICATOR_WIDGET_SET | +8984 | +1096 | +0 | +332 | +10080 |
| RING_WIDGET_SET | +19180 | +1920 | +0 | +248 | +21100 |
| LINE_VISUAL_WIDGET_SET | +13312 | +1676 | +0 | +272 | +14988 |
| ALL_WIDGET_PROBES | +33988 | +3588 | +0 | +1532 | +37576 |

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
| BASELINE | `(none)` | 11660 | 568 | 24 | 6024 | 12228 |
| SLIDER | `-DEGUI_SIZE_PROBE_WIDGET_SLIDER=1` | 19104 | 1384 | 24 | 6100 | 20488 |
| SWITCH | `-DEGUI_SIZE_PROBE_WIDGET_SWITCH=1` | 18200 | 1392 | 24 | 6116 | 19592 |
| PAGE_INDICATOR | `-DEGUI_SIZE_PROBE_WIDGET_PAGE_INDICATOR=1` | 14780 | 1388 | 24 | 6104 | 16168 |
| STEPPER | `-DEGUI_SIZE_PROBE_WIDGET_STEPPER=1` | 14928 | 1460 | 24 | 6120 | 16388 |
| CHECKBOX | `-DEGUI_SIZE_PROBE_WIDGET_CHECKBOX=1` | 18596 | 1380 | 24 | 6124 | 19976 |
| RADIO_BUTTON | `-DEGUI_SIZE_PROBE_WIDGET_RADIO_BUTTON=1` | 17792 | 1384 | 24 | 6120 | 19176 |
| PROGRESS_BAR | `-DEGUI_SIZE_PROBE_WIDGET_PROGRESS_BAR=1` | 18632 | 1388 | 24 | 6100 | 20020 |
| TOGGLE_BUTTON | `-DEGUI_SIZE_PROBE_WIDGET_TOGGLE_BUTTON=1` | 16152 | 1764 | 24 | 6120 | 17916 |
| NOTIFICATION_BADGE | `-DEGUI_SIZE_PROBE_WIDGET_NOTIFICATION_BADGE=1` | 16160 | 1724 | 24 | 6112 | 17884 |
| BUTTON | `-DEGUI_SIZE_PROBE_WIDGET_BUTTON=1` | 16356 | 1760 | 24 | 6116 | 18116 |
| IMAGE_BUTTON | `-DEGUI_SIZE_PROBE_WIDGET_IMAGE_BUTTON=1` | 17552 | 1756 | 24 | 6120 | 19308 |
| CIRCULAR_PROGRESS_BAR | `-DEGUI_SIZE_PROBE_WIDGET_CPB=1` | 24868 | 2292 | 24 | 6104 | 27160 |
| GAUGE | `-DEGUI_SIZE_PROBE_WIDGET_GAUGE=1` | 28680 | 2280 | 24 | 6112 | 30960 |
| ACTIVITY_RING | `-DEGUI_SIZE_PROBE_WIDGET_ACTIVITY_RING=1` | 23504 | 1948 | 24 | 6104 | 25452 |
| ANALOG_CLOCK | `-DEGUI_SIZE_PROBE_WIDGET_ANALOG_CLOCK=1` | 20860 | 1572 | 24 | 6104 | 22432 |
| LINE_WIDGET | `-DEGUI_SIZE_PROBE_WIDGET_LINE=1` | 17632 | 1388 | 24 | 6096 | 19020 |
| CHART_LINE | `-DEGUI_SIZE_PROBE_WIDGET_CHART_LINE=1` | 20820 | 1868 | 24 | 6152 | 22688 |
| CHART_PIE | `-DEGUI_SIZE_PROBE_WIDGET_CHART_PIE=1` | 19040 | 2144 | 24 | 6104 | 21184 |
| INDICATOR_WIDGET_SET | `-DEGUI_SIZE_PROBE_WIDGET_SLIDER=1 -DEGUI_SIZE_PROBE_WIDGET_SWITCH=1 -DEGUI_SIZE_PROBE_WIDGET_PAGE_INDICATOR=1 -DEGUI_SIZE_PROBE_WIDGET_STEPPER=1` | 20644 | 1664 | 24 | 6356 | 22308 |
| RING_WIDGET_SET | `-DEGUI_SIZE_PROBE_WIDGET_CPB=1 -DEGUI_SIZE_PROBE_WIDGET_GAUGE=1 -DEGUI_SIZE_PROBE_WIDGET_ACTIVITY_RING=1` | 30840 | 2488 | 24 | 6272 | 33328 |
| LINE_VISUAL_WIDGET_SET | `-DEGUI_SIZE_PROBE_WIDGET_LINE=1 -DEGUI_SIZE_PROBE_WIDGET_CHART_LINE=1 -DEGUI_SIZE_PROBE_WIDGET_ANALOG_CLOCK=1` | 24972 | 2244 | 24 | 6296 | 27216 |
| ALL_WIDGET_PROBES | `-DEGUI_SIZE_PROBE_WIDGET_SLIDER=1 -DEGUI_SIZE_PROBE_WIDGET_SWITCH=1 -DEGUI_SIZE_PROBE_WIDGET_PAGE_INDICATOR=1 -DEGUI_SIZE_PROBE_WIDGET_STEPPER=1 -DEGUI_SIZE_PROBE_WIDGET_CHECKBOX=1 -DEGUI_SIZE_PROBE_WIDGET_RADIO_BUTTON=1 -DEGUI_SIZE_PROBE_WIDGET_PROGRESS_BAR=1 -DEGUI_SIZE_PROBE_WIDGET_TOGGLE_BUTTON=1 -DEGUI_SIZE_PROBE_WIDGET_NOTIFICATION_BADGE=1 -DEGUI_SIZE_PROBE_WIDGET_BUTTON=1 -DEGUI_SIZE_PROBE_WIDGET_IMAGE_BUTTON=1 -DEGUI_SIZE_PROBE_WIDGET_CPB=1 -DEGUI_SIZE_PROBE_WIDGET_GAUGE=1 -DEGUI_SIZE_PROBE_WIDGET_ACTIVITY_RING=1 -DEGUI_SIZE_PROBE_WIDGET_ANALOG_CLOCK=1 -DEGUI_SIZE_PROBE_WIDGET_LINE=1 -DEGUI_SIZE_PROBE_WIDGET_CHART_LINE=1 -DEGUI_SIZE_PROBE_WIDGET_CHART_PIE=1` | 45648 | 4156 | 24 | 7556 | 49804 |

## Reproduce

```bash
python scripts/size_analysis/widget_feature_size_to_doc.py
```

Raw JSON is written to `output/widget_feature_size_results.json`.
