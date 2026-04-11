# 2026-04-11 chart bar tick-step and gradient hoist

## 背景

- 延续 `f90ac54 perf: chunk dense line polyline drawing` 之后的 QEMU 性能优化。
- 目标场景继续限定在 `HelloPerformance` 新增的 chart/file image 场景。
- 规则保持不变：
  - 只看 QEMU；
  - 优先零额外内存优化；
  - 涉及 RAM/heap 变化的方案，没有 30% 以上收益不保留；
  - 不动无关脏文件。

## 本次保留改动

### 1. `src/widget/egui_view_chart_bar.c`

- 把 enhanced bar 的渐变构造从“每个 bar 一次”提升到“每个 series 一次”。
- 把 bar 的 Y 映射改成局部 fast path，避免每根柱子都走通用 `egui_chart_map_y()`。
- 把 `group_x` 改成递增推进，减少重复乘法。

### 2. `src/widget/egui_view_chart_common.c`

- 修正 categorical X 轴无视 `tick_step` 的问题。
- `bar dense` 场景虽然配置了 `tick_step=4`，此前仍会把 32 个类目全部画出来。
- 现在 categorical 轴会按 `tick_step` 采样绘制 tick/grid/label，和 continuous 轴的语义对齐。

## 量化结果

### 单场景

- `CHART_BAR_DENSE`: `1.145 ms -> 0.755 ms`
- 改善约 `34.1%`

### 其他关键场景复测

- `CHART_LINE_DENSE = 1.188 ms`
- `CHART_SCATTER_DENSE = 1.043 ms`
- `CHART_PIE_DENSE = 2.143 ms`
- `FILE_IMAGE_JPG/PNG/BMP = 0.330/0.330/0.330 ms`

说明：

- 本次收益几乎全部来自 categorical X 轴开始真正尊重 `tick_step=4`，bar dense 的底部刻度/网格/标签固定开销显著下降。
- `file image` 本轮没有保留改动，基线维持不变。

## 验证

### QEMU

- `python scripts/perf_analysis/main.py --profile cortex-m3 --filter CHART_BAR_DENSE`
- `python scripts/perf_analysis/main.py --profile cortex-m3`

### Runtime

- `python scripts/code_runtime_check.py --app HelloPerformance --timeout 10 --keep-screenshots`
- `python scripts/code_runtime_check.py --app HelloChart --timeout 10 --keep-screenshots`

### Unit test

- `make all APP=HelloUnitTest PORT=pc_test`
- `output/main.exe`
- 结果：`357/357 passed`

### 截图目检

- `runtime_check_output/HelloPerformance/default/frame_0254.png`
  - `CHART_BAR_DENSE` 已按 `0, 4, 8, ...` 采样显示 X 轴标签，布局正常。
- `runtime_check_output/HelloChart/default/`
  - 运行时回归通过，未见 chart 页面崩溃或空白。

## 结论

- 这是一个应保留的零额外内存优化。
- 其中 `tick_step` 修正同时也是行为语义修正，不只是 benchmark 定向裁剪。
