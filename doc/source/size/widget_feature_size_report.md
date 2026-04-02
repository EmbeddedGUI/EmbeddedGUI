# Widget Feature QEMU Code Size Report

- Commit: `09190fa`
- Date: 2026-04-02T11:15:17.658273
- Build target: `APP=HelloSizeAnalysis APP_SUB=widget_feature_probe PORT=qemu CPU_ARCH=cortex-m0plus`
- Measurement method: compile a dedicated widget probe app, then rewrite the app-local probe config header to force-link a real widget or widget set.
- Scope: widget-level static qemu ELF sections only (`.text/.rodata/.data/.bss`).

## 说明

- 这份报告回答的是“引入某个真实 widget 需要付出多少 `ROM/RAM` 成本”。
- 这里测量的不是 canvas 替身，而是真实 widget 的 `init_with_params` 和 setter 路径。
- 由于 widget 之间会共享 `base/view/theme/canvas` 等公共代码，组合项不能按单项直接相加。

## Baseline

| Variant | Text | Rodata | Data | Bss | Total ROM |
|---------|-----:|-------:|-----:|----:|----------:|
| BASELINE | 11420 | 568 | 24 | 5920 | 11988 |

## Widget Summary

| Widget Or Set | Delta Text | Delta Rodata | Delta Data | Delta Bss | Delta ROM |
|---------------|-----------:|-------------:|-----------:|----------:|----------:|
| SLIDER | +11708 | +816 | +0 | +76 | +12524 |
| SWITCH | +11708 | +816 | +0 | +76 | +12524 |
| PAGE_INDICATOR | +11708 | +816 | +0 | +76 | +12524 |
| STEPPER | +11708 | +816 | +0 | +76 | +12524 |
| CIRCULAR_PROGRESS_BAR | +26816 | +15392 | +36 | +80 | +42208 |
| GAUGE | +26816 | +15392 | +36 | +80 | +42208 |
| ACTIVITY_RING | +34164 | +3044 | +0 | +88 | +37208 |
| ANALOG_CLOCK | +34164 | +3044 | +0 | +88 | +37208 |
| LINE_WIDGET | +10200 | +152 | +0 | +72 | +10352 |
| CHART_LINE | +10200 | +152 | +0 | +72 | +10352 |
| INDICATOR_WIDGET_SET | +10200 | +152 | +0 | +72 | +10352 |
| RING_WIDGET_SET | +50136 | +16420 | +36 | +240 | +66556 |
| LINE_VISUAL_WIDGET_SET | +50136 | +16420 | +36 | +240 | +66556 |
| ALL_WIDGET_PROBES | +66716 | +17204 | +36 | +828 | +83920 |

## Definition

| Widget Or Set | Included Items | Description |
|---------------|----------------|-------------|
| SLIDER | `egui_view_slider` | 真实 slider 控件成本。 |
| SWITCH | `egui_view_switch` | 真实 switch 控件成本。 |
| PAGE_INDICATOR | `egui_view_page_indicator` | 真实 page indicator 控件成本。 |
| STEPPER | `egui_view_stepper` | 真实 stepper 控件成本。 |
| CIRCULAR_PROGRESS_BAR | `egui_view_circular_progress_bar` | 真实 circular progress bar 控件成本。 |
| GAUGE | `egui_view_gauge` | 真实 gauge 控件成本。 |
| ACTIVITY_RING | `egui_view_activity_ring` | 真实 activity_ring 控件成本。 |
| ANALOG_CLOCK | `egui_view_analog_clock` | 真实 analog_clock 控件成本。 |
| LINE_WIDGET | `egui_view_line` | 真实 line 控件成本。 |
| CHART_LINE | `egui_view_chart_line` | 真实 chart_line 控件成本。 |
| INDICATOR_WIDGET_SET | `slider`<br>`switch`<br>`page_indicator`<br>`stepper` | 一组常见 indicator 控件组合。 |
| RING_WIDGET_SET | `circular_progress_bar`<br>`gauge`<br>`activity_ring` | 一组 ring / gauge 类控件组合。 |
| LINE_VISUAL_WIDGET_SET | `line`<br>`chart_line`<br>`analog_clock` | 一组 line / clock / data-visual 控件组合。 |
| ALL_WIDGET_PROBES | `all widget probes` | 当前 probe 覆盖的所有真实 widget 同时启用。 |

## Detailed Variants

| Variant | Probe Config | Text | Rodata | Data | Bss | Total ROM |
|---------|-------------------|-----:|-------:|-----:|----:|----------:|
| BASELINE | `(none)` | 11420 | 568 | 24 | 5920 | 11988 |
| SLIDER | `-DEGUI_SIZE_PROBE_WIDGET_SLIDER=1` | 23128 | 1384 | 24 | 5996 | 24512 |
| SWITCH | `-DEGUI_SIZE_PROBE_WIDGET_SWITCH=1` | 23128 | 1384 | 24 | 5996 | 24512 |
| PAGE_INDICATOR | `-DEGUI_SIZE_PROBE_WIDGET_PAGE_INDICATOR=1` | 23128 | 1384 | 24 | 5996 | 24512 |
| STEPPER | `-DEGUI_SIZE_PROBE_WIDGET_STEPPER=1` | 23128 | 1384 | 24 | 5996 | 24512 |
| CIRCULAR_PROGRESS_BAR | `-DEGUI_SIZE_PROBE_WIDGET_CPB=1` | 38236 | 15960 | 60 | 6000 | 54196 |
| GAUGE | `-DEGUI_SIZE_PROBE_WIDGET_GAUGE=1` | 38236 | 15960 | 60 | 6000 | 54196 |
| ACTIVITY_RING | `-DEGUI_SIZE_PROBE_WIDGET_ACTIVITY_RING=1` | 45584 | 3612 | 24 | 6008 | 49196 |
| ANALOG_CLOCK | `-DEGUI_SIZE_PROBE_WIDGET_ANALOG_CLOCK=1` | 45584 | 3612 | 24 | 6008 | 49196 |
| LINE_WIDGET | `-DEGUI_SIZE_PROBE_WIDGET_LINE=1` | 21620 | 720 | 24 | 5992 | 22340 |
| CHART_LINE | `-DEGUI_SIZE_PROBE_WIDGET_CHART_LINE=1` | 21620 | 720 | 24 | 5992 | 22340 |
| INDICATOR_WIDGET_SET | `-DEGUI_SIZE_PROBE_WIDGET_SLIDER=1 -DEGUI_SIZE_PROBE_WIDGET_SWITCH=1 -DEGUI_SIZE_PROBE_WIDGET_PAGE_INDICATOR=1 -DEGUI_SIZE_PROBE_WIDGET_STEPPER=1` | 21620 | 720 | 24 | 5992 | 22340 |
| RING_WIDGET_SET | `-DEGUI_SIZE_PROBE_WIDGET_CPB=1 -DEGUI_SIZE_PROBE_WIDGET_GAUGE=1 -DEGUI_SIZE_PROBE_WIDGET_ACTIVITY_RING=1` | 61556 | 16988 | 60 | 6160 | 78544 |
| LINE_VISUAL_WIDGET_SET | `-DEGUI_SIZE_PROBE_WIDGET_LINE=1 -DEGUI_SIZE_PROBE_WIDGET_CHART_LINE=1 -DEGUI_SIZE_PROBE_WIDGET_ANALOG_CLOCK=1` | 61556 | 16988 | 60 | 6160 | 78544 |
| ALL_WIDGET_PROBES | `-DEGUI_SIZE_PROBE_WIDGET_SLIDER=1 -DEGUI_SIZE_PROBE_WIDGET_SWITCH=1 -DEGUI_SIZE_PROBE_WIDGET_PAGE_INDICATOR=1 -DEGUI_SIZE_PROBE_WIDGET_STEPPER=1 -DEGUI_SIZE_PROBE_WIDGET_CPB=1 -DEGUI_SIZE_PROBE_WIDGET_GAUGE=1 -DEGUI_SIZE_PROBE_WIDGET_ACTIVITY_RING=1 -DEGUI_SIZE_PROBE_WIDGET_ANALOG_CLOCK=1 -DEGUI_SIZE_PROBE_WIDGET_LINE=1 -DEGUI_SIZE_PROBE_WIDGET_CHART_LINE=1` | 78136 | 17772 | 60 | 6748 | 95908 |

## Reproduce

```bash
python scripts/size_analysis/widget_feature_size_to_doc.py
```

Raw JSON is written to `output/widget_feature_size_results.json`.
