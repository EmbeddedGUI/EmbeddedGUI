# Canvas Path QEMU Code Size Report

- Commit: `1fe189eb`
- Date: 2026-05-21T16:04:33.734670
- Build target: `APP=HelloSizeAnalysis APP_SUB=canvas_path_probe PORT=qemu CPU_ARCH=cortex-m0plus`
- Measurement method: compile a dedicated probe app, then rewrite the app-local probe config header to force-link a single canvas scene.
- Scope: static qemu ELF sections only (`.text/.rodata/.data/.bss`). No runtime heap/stack is measured here.

## 说明

- 这是“渲染场景引入成本”报告，不是业务应用最终成本报告。
- 各场景之间存在依赖与重叠，不能简单把增量直接相加。
- `Delta Text` 是主要代码体积指标；`Delta ROM = Delta Text + Delta Rodata` 更适合作为 flash 成本口径。
- 这份报告只覆盖普通 canvas/render path；HQ line/circle/arc 请结合 `hq_size_report.md` 一起看。

## Baseline

| Variant | Text | Rodata | Data | Bss | Total ROM |
|---------|-----:|-------:|-----:|----:|----------:|
| BASELINE | 14652 | 596 | 1368 | 7600 | 15248 |

## Increment Summary

| Scene | Delta Text | Delta Rodata | Delta Data | Delta Bss | Delta ROM |
|-------|-----------:|-------------:|-----------:|----------:|----------:|
| RECT_STROKE | +1012 | +0 | +0 | +4 | +1012 |
| RECT_FILL | +780 | +0 | +0 | +4 | +780 |
| ROUND_RECT_STROKE | +7136 | +724 | +0 | +4 | +7860 |
| ROUND_RECT_FILL | +3604 | +724 | +0 | +4 | +4328 |
| TRIANGLE_STROKE | +4140 | +0 | +0 | +4 | +4140 |
| TRIANGLE_FILL | +4184 | +0 | +0 | +4 | +4184 |
| CIRCLE_BASIC_STROKE | +6096 | +724 | +0 | +4 | +6820 |
| CIRCLE_BASIC_FILL | +3136 | +724 | +0 | +4 | +3860 |
| ARC_BASIC_STROKE | +12856 | +1844 | +0 | +4 | +14700 |
| ARC_BASIC_FILL | +6880 | +1844 | +0 | +4 | +8724 |
| LINE | +3984 | +0 | +0 | +4 | +3984 |
| POLYLINE | +4496 | +12 | +0 | +4 | +4508 |
| GRADIENT_RECT | +3188 | +60 | +0 | +4 | +3248 |
| GRADIENT_ROUND_RECT | +6844 | +804 | +0 | +4 | +7648 |
| GRADIENT_CIRCLE | +5120 | +60 | +0 | +4 | +5180 |
| MASK_CIRCLE | +8 | +0 | +0 | +4 | +8 |
| MASK_ROUND_RECT | +8 | +0 | +0 | +4 | +8 |
| MASK_IMAGE | +8 | +0 | +0 | +4 | +8 |
| IMAGE_DRAW | +10540 | +108 | +0 | +12 | +10648 |
| IMAGE_RESIZE | +10588 | +108 | +0 | +12 | +10696 |
| IMAGE_TINT | +10600 | +108 | +0 | +12 | +10708 |
| IMAGE_ROTATE | +16164 | +476 | +0 | +12 | +16640 |
| TEXT_ROTATE | +21000 | +6432 | +0 | +12 | +27432 |
| RLE_DRAW | +8 | +0 | +0 | +4 | +8 |
| QOI_DRAW | +8480 | +148 | +0 | +12 | +8628 |
| ALL_CANVAS_PATHS | +74156 | +8228 | +0 | +12 | +82384 |

## Scene Definition

| Scene | Representative Functions | Description |
|-------|--------------------------|-------------|
| RECT_STROKE | `egui_canvas_draw_rectangle` | Rectangle stroke path. |
| RECT_FILL | `egui_canvas_draw_rectangle_fill` | Rectangle fill path. |
| ROUND_RECT_STROKE | `egui_canvas_draw_round_rectangle` | Round-rectangle stroke path. |
| ROUND_RECT_FILL | `egui_canvas_draw_round_rectangle_fill` | Round-rectangle fill path. |
| TRIANGLE_STROKE | `egui_canvas_draw_triangle` | Triangle stroke path. |
| TRIANGLE_FILL | `egui_canvas_draw_triangle_fill` | Triangle fill path. |
| CIRCLE_BASIC_STROKE | `egui_canvas_draw_circle_basic` | Basic circle stroke path. |
| CIRCLE_BASIC_FILL | `egui_canvas_draw_circle_fill_basic` | Basic circle fill path, including circle LUT cost. |
| ARC_BASIC_STROKE | `egui_canvas_draw_arc_basic` | Basic arc stroke path. |
| ARC_BASIC_FILL | `egui_canvas_draw_arc_fill_basic` | Basic arc fill path. |
| LINE | `egui_canvas_draw_line` | Basic line path. |
| POLYLINE | `egui_canvas_draw_polyline` | Basic polyline path. |
| GRADIENT_RECT | `egui_canvas_draw_rectangle_fill_gradient` | Gradient rectangle fill path. |
| GRADIENT_ROUND_RECT | `egui_canvas_draw_round_rectangle_fill_gradient` | Gradient round-rectangle fill path. |
| GRADIENT_CIRCLE | `egui_canvas_draw_circle_fill_gradient` | Gradient circle fill path. |
| MASK_CIRCLE | `egui_mask_circle_init`<br>`egui_canvas_set_mask` | Circle mask scene. |
| MASK_ROUND_RECT | `egui_mask_round_rectangle_init`<br>`egui_canvas_set_mask` | Round-rectangle mask scene. |
| MASK_IMAGE | `egui_mask_image_init`<br>`egui_mask_image_set_image`<br>`egui_canvas_set_mask` | Image mask scene. |
| IMAGE_DRAW | `egui_canvas_draw_image` | Standard image draw path. |
| IMAGE_RESIZE | `egui_canvas_draw_image_resize` | Standard image resize path. |
| IMAGE_TINT | `egui_canvas_draw_image_color` | Standard image tint path. |
| IMAGE_ROTATE | `egui_canvas_draw_image_rotate` | Image rotation and transform scene. |
| TEXT_ROTATE | `egui_canvas_draw_text_rotate` | Rotated text draw scene. |
| RLE_DRAW | `egui_canvas_draw_image(rle)` | RLE codec draw path. |
| QOI_DRAW | `egui_canvas_draw_image(qoi)` | QOI codec draw path. |
| ALL_CANVAS_PATHS | `all representative probe functions` | All representative canvas path probe scenes enabled together. |

## Detailed Variants

| Variant | Probe Config | Text | Rodata | Data | Bss | Total ROM |
|---------|-------------------|-----:|-------:|-----:|----:|----------:|
| BASELINE | `(none)` | 14652 | 596 | 1368 | 7600 | 15248 |
| RECT_STROKE | `-DEGUI_SIZE_PROBE_RECT_STROKE_PATH=1` | 15664 | 596 | 1368 | 7604 | 16260 |
| RECT_FILL | `-DEGUI_SIZE_PROBE_RECT_FILL_PATH=1` | 15432 | 596 | 1368 | 7604 | 16028 |
| ROUND_RECT_STROKE | `-DEGUI_SIZE_PROBE_ROUND_RECT_STROKE_PATH=1` | 21788 | 1320 | 1368 | 7604 | 23108 |
| ROUND_RECT_FILL | `-DEGUI_SIZE_PROBE_ROUND_RECT_FILL_PATH=1` | 18256 | 1320 | 1368 | 7604 | 19576 |
| TRIANGLE_STROKE | `-DEGUI_SIZE_PROBE_TRIANGLE_STROKE_PATH=1` | 18792 | 596 | 1368 | 7604 | 19388 |
| TRIANGLE_FILL | `-DEGUI_SIZE_PROBE_TRIANGLE_FILL_PATH=1` | 18836 | 596 | 1368 | 7604 | 19432 |
| CIRCLE_BASIC_STROKE | `-DEGUI_SIZE_PROBE_CIRCLE_BASIC_STROKE_PATH=1` | 20748 | 1320 | 1368 | 7604 | 22068 |
| CIRCLE_BASIC_FILL | `-DEGUI_SIZE_PROBE_CIRCLE_BASIC_FILL_PATH=1` | 17788 | 1320 | 1368 | 7604 | 19108 |
| ARC_BASIC_STROKE | `-DEGUI_SIZE_PROBE_ARC_BASIC_STROKE_PATH=1` | 27508 | 2440 | 1368 | 7604 | 29948 |
| ARC_BASIC_FILL | `-DEGUI_SIZE_PROBE_ARC_BASIC_FILL_PATH=1` | 21532 | 2440 | 1368 | 7604 | 23972 |
| LINE | `-DEGUI_SIZE_PROBE_LINE_PATH=1` | 18636 | 596 | 1368 | 7604 | 19232 |
| POLYLINE | `-DEGUI_SIZE_PROBE_POLYLINE_PATH=1` | 19148 | 608 | 1368 | 7604 | 19756 |
| GRADIENT_RECT | `-DEGUI_SIZE_PROBE_GRADIENT_RECT_PATH=1` | 17840 | 656 | 1368 | 7604 | 18496 |
| GRADIENT_ROUND_RECT | `-DEGUI_SIZE_PROBE_GRADIENT_ROUND_RECT_PATH=1` | 21496 | 1400 | 1368 | 7604 | 22896 |
| GRADIENT_CIRCLE | `-DEGUI_SIZE_PROBE_GRADIENT_CIRCLE_PATH=1` | 19772 | 656 | 1368 | 7604 | 20428 |
| MASK_CIRCLE | `-DEGUI_SIZE_PROBE_MASK_CIRCLE_PATH=1` | 14660 | 596 | 1368 | 7604 | 15256 |
| MASK_ROUND_RECT | `-DEGUI_SIZE_PROBE_MASK_ROUND_RECT_PATH=1` | 14660 | 596 | 1368 | 7604 | 15256 |
| MASK_IMAGE | `-DEGUI_SIZE_PROBE_MASK_IMAGE_PATH=1` | 14660 | 596 | 1368 | 7604 | 15256 |
| IMAGE_DRAW | `-DEGUI_SIZE_PROBE_IMAGE_DRAW_PATH=1` | 25192 | 704 | 1368 | 7612 | 25896 |
| IMAGE_RESIZE | `-DEGUI_SIZE_PROBE_IMAGE_RESIZE_PATH=1` | 25240 | 704 | 1368 | 7612 | 25944 |
| IMAGE_TINT | `-DEGUI_SIZE_PROBE_IMAGE_TINT_PATH=1` | 25252 | 704 | 1368 | 7612 | 25956 |
| IMAGE_ROTATE | `-DEGUI_SIZE_PROBE_IMAGE_ROTATE_PATH=1` | 30816 | 1072 | 1368 | 7612 | 31888 |
| TEXT_ROTATE | `-DEGUI_SIZE_PROBE_TEXT_ROTATE_PATH=1` | 35652 | 7028 | 1368 | 7612 | 42680 |
| RLE_DRAW | `-DEGUI_SIZE_PROBE_RLE_DRAW_PATH=1` | 14660 | 596 | 1368 | 7604 | 15256 |
| QOI_DRAW | `-DEGUI_SIZE_PROBE_QOI_DRAW_PATH=1` | 23132 | 744 | 1368 | 7612 | 23876 |
| ALL_CANVAS_PATHS | `-DEGUI_SIZE_PROBE_RECT_STROKE_PATH=1 -DEGUI_SIZE_PROBE_RECT_FILL_PATH=1 -DEGUI_SIZE_PROBE_ROUND_RECT_STROKE_PATH=1 -DEGUI_SIZE_PROBE_ROUND_RECT_FILL_PATH=1 -DEGUI_SIZE_PROBE_TRIANGLE_STROKE_PATH=1 -DEGUI_SIZE_PROBE_TRIANGLE_FILL_PATH=1 -DEGUI_SIZE_PROBE_CIRCLE_BASIC_STROKE_PATH=1 -DEGUI_SIZE_PROBE_CIRCLE_BASIC_FILL_PATH=1 -DEGUI_SIZE_PROBE_ARC_BASIC_STROKE_PATH=1 -DEGUI_SIZE_PROBE_ARC_BASIC_FILL_PATH=1 -DEGUI_SIZE_PROBE_LINE_PATH=1 -DEGUI_SIZE_PROBE_POLYLINE_PATH=1 -DEGUI_SIZE_PROBE_GRADIENT_RECT_PATH=1 -DEGUI_SIZE_PROBE_GRADIENT_ROUND_RECT_PATH=1 -DEGUI_SIZE_PROBE_GRADIENT_CIRCLE_PATH=1 -DEGUI_SIZE_PROBE_MASK_CIRCLE_PATH=1 -DEGUI_SIZE_PROBE_MASK_ROUND_RECT_PATH=1 -DEGUI_SIZE_PROBE_MASK_IMAGE_PATH=1 -DEGUI_SIZE_PROBE_IMAGE_DRAW_PATH=1 -DEGUI_SIZE_PROBE_IMAGE_RESIZE_PATH=1 -DEGUI_SIZE_PROBE_IMAGE_TINT_PATH=1 -DEGUI_SIZE_PROBE_IMAGE_ROTATE_PATH=1 -DEGUI_SIZE_PROBE_TEXT_ROTATE_PATH=1 -DEGUI_SIZE_PROBE_RLE_DRAW_PATH=1 -DEGUI_SIZE_PROBE_QOI_DRAW_PATH=1` | 88808 | 8824 | 1368 | 7612 | 97632 |

## Reproduce

```bash
python scripts/size_analysis/main.py canvas-path-size-to-doc
```

Raw JSON is written to `output/canvas_path_size_results.json`.
