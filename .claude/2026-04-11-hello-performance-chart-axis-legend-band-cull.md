# HelloPerformance chart 公共层 band 级早退优化记录

## 背景

- 基线提交：`9f924bc`
- 目标场景：
  - `CHART_LINE_DENSE`
  - `CHART_BAR_DENSE`
  - `CHART_SCATTER_DENSE`
- 约束：
  - 以 QEMU 结果为准
  - 不增加 heap
  - 不保留收益不成立的实验

## 本轮保留改动

文件：`src/widget/egui_view_chart_common.c`

### 1. X 轴 band 级早退

- 在 `egui_chart_draw_x_axis_categorical()` 和 `egui_chart_draw_x_axis_continuous()` 里，先判断当前 work region 是否与以下 band 相交：
  - 底部 tick band
  - plot 区域内的 grid band
  - 底部 label band
- 若三者都不相交，则整段 X 轴绘制直接返回
- 若仅 label band 不相交，则跳过 `egui_chart_int_to_str()` 与 label rect 判断

### 2. Y 轴 band 级早退

- 在 `egui_chart_draw_axes()` 中拆开判断：
  - X/Y 轴主轴线
  - Y 轴 tick band
  - plot 区域内的 Y grid band
  - 左侧 Y label band
- 当前 tile 不相交时，直接跳过对应分支
- `y_label_w` 只在需要绘制 Y 轴标签时计算一次，不再每个 tick 重复算

### 3. legend band 级早退

- 在 `egui_chart_draw_legend_series()` 中增加 legend 整体 band 判断
- 当前 tile 不相交时，直接跳过整段 legend 循环

## 结果

### QEMU 单轮 chart 场景

| 场景 | 基线 | 当前 |
|------|------|------|
| `CHART_LINE_DENSE` | `1.321 ms` | `1.190 ms` |
| `CHART_BAR_DENSE` | `1.318 ms` | `1.145 ms` |
| `CHART_SCATTER_DENSE` | `1.167 ms` | `1.043 ms` |
| `CHART_PIE_DENSE` | `2.143 ms` | `2.143 ms` |

### 提升幅度

- `CHART_LINE_DENSE` 约提升 `9.9%`
- `CHART_BAR_DENSE` 约提升 `13.1%`
- `CHART_SCATTER_DENSE` 约提升 `10.6%`
- `CHART_PIE_DENSE` 持平

## 验证

执行命令：

```bash
python scripts/perf_analysis/main.py --profile cortex-m3 --filter CHART_
python scripts/code_runtime_check.py --app HelloPerformance --timeout 10 --keep-screenshots
make all APP=HelloUnitTest PORT=pc_test
output\main.exe
python scripts/perf_analysis/main.py --profile cortex-m3
```

结果：

- `HelloPerformance` 运行时检查通过
- `HelloUnitTest` 357/357 通过
- 全量 `cortex-m3` profile 通过

当前关键场景：

- `CHART_LINE_DENSE = 1.190 ms`
- `CHART_BAR_DENSE = 1.145 ms`
- `CHART_SCATTER_DENSE = 1.043 ms`
- `CHART_PIE_DENSE = 2.143 ms`
- `ARC_FILL = 0.825 ms`
- `FILE_IMAGE_JPG/PNG/BMP = 0.330/0.330/0.330 ms`

## 放弃的实验

### line thin-path 单次映射逐段绘制

- 尝试把 `line_width <= 1` 的 `draw_polyline + tail line` 改成统一单次映射逐段 `egui_canvas_draw_line()`
- QEMU 结果：`CHART_LINE_DENSE 1.321 -> 7.641 ms`
- 结论：严重退化，已回退

## 后续建议

- chart 下一轮优先继续看公共层固定成本，而不是再调 `bar/scatter` 的 logical PFB hint
- 如果继续做 `line`，优先分析 `marker` 与 axis/grid 的重复工作，不要直接替换现有 polyline 路径
