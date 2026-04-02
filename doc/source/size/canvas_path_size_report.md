# Canvas Path QEMU Code Size Report

- Commit: `09190fa`
- Date: 2026-04-02T11:13:30.470551
- Build target: `APP=HelloSizeAnalysis APP_SUB=canvas_path_probe PORT=qemu CPU_ARCH=cortex-m0plus`
- Measurement method: compile a dedicated probe app, then rewrite the app-local probe config header to force-link a single canvas scene.
- Scope: static qemu ELF sections only (`.text/.rodata/.data/.bss`). No runtime heap/stack is measured here.

## 说明

- 这份报告回答的是“某条 canvas 渲染路径被链接进来后，会额外增加多少 `.text/.rodata/.data/.bss`”。
- 这不是业务应用的最终体积报告，最终交付仍然要看 `size_report.md`。
- 各场景之间存在共享依赖，不能把多个场景的增量简单相加。
- `Delta Text` 更适合看代码体积；`Delta ROM = Delta Text + Delta Rodata` 更适合作为 flash 成本口径。
- HQ 的 `line/circle/arc` 路径单独统计在 `hq_size_report.md`，这里不重复。

## Baseline

| Variant | Text | Rodata | Data | Bss | Total ROM |
|---------|-----:|-------:|-----:|----:|----------:|
| BASELINE | 11420 | 568 | 24 | 5920 | 11988 |

## Increment Summary

| Scene | Delta Text | Delta Rodata | Delta Data | Delta Bss | Delta ROM |
|-------|-----------:|-------------:|-----------:|----------:|----------:|
| RECT_FILL | +0 | +0 | +0 | +0 | +0 |

## Scene Definition

| Scene | Representative Functions | Description |
|-------|--------------------------|-------------|
| RECT_FILL | `egui_canvas_draw_rectangle_fill` | 矩形填充路径。 |

## Detailed Variants

| Variant | Probe Config | Text | Rodata | Data | Bss | Total ROM |
|---------|-------------------|-----:|-------:|-----:|----:|----------:|
| BASELINE | `(none)` | 11420 | 568 | 24 | 5920 | 11988 |
| RECT_FILL | `-DEGUI_SIZE_PROBE_RECT_FILL_PATH=1` | 11420 | 568 | 24 | 5920 | 11988 |

## Reproduce

```bash
python scripts/size_analysis/canvas_path_size_to_doc.py
```

Raw JSON is written to `output/canvas_path_size_results.json`.
