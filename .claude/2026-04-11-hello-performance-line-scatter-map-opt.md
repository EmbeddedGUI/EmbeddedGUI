# 2026-04-11 HelloPerformance line/scatter 映射快路径优化

## 目标

- 在不增加额外 heap 或缓存的前提下，继续优化 `HelloPerformance` 里的 chart 新场景。
- 本轮优先处理 `CHART_LINE_DENSE`，并顺带验证同类改法是否适用于 `scatter / bar`。

## 保留改动

- `src/widget/egui_view_chart_line.c`
  - 在单次 `draw_data()` 内预取 `view_x/view_y` 与 `range_x/range_y`
  - 把热路径里的 `egui_chart_map_x()` / `egui_chart_map_y()` 跨文件调用替换为当前文件内联映射
  - 可见索引窗口与可见 `data_y` 窗口复用同一组 view range，减少重复取值
- `src/widget/egui_view_chart_scatter.c`
  - 预取 `view_x/view_y` 与 `range_x/range_y`
  - 散点坐标映射改成本地内联快路径
  - 可见数据范围推导复用预取的 view range

## 未保留实验

- `src/core/egui_canvas.c`
  - 试过把 arc 边缘 signed-distance 改成按列递推，想去掉逐像素乘法
  - 单场景结果：`CHART_PIE_DENSE = 2.255 ms`
  - 相比当前基线 `2.184 ms` 回归，已回退
- `src/widget/egui_view_chart_bar.c`
  - 试过对 `bar` 也做同类 view-range 预取 + 本地内联 `map_y`
  - 单场景结果：`CHART_BAR_DENSE = 1.334 ms`
  - 相比当前基线 `1.330 ms` 轻微回归，已回退

## 增量 A/B

基线为当前已保留 `64x12` line hint、但尚未做本次 line/scatter 映射快路径的版本。

| 场景 | 增量基线(ms) | 增量当前(ms) | 变化 | 额外 heap |
| --- | ---: | ---: | ---: | --- |
| CHART_LINE_DENSE | 1.418 | 1.321 | -6.8% | 0 |
| CHART_SCATTER_DENSE | 1.188 | 1.167 | -1.8% | 0 |

## 当前关键结果

- `python scripts/perf_analysis/main.py --profile cortex-m3`
  - `FILE_IMAGE_JPG = 0.330 ms`
  - `FILE_IMAGE_PNG = 0.330 ms`
  - `FILE_IMAGE_BMP = 0.330 ms`
  - `CHART_LINE_DENSE = 1.321 ms`
  - `CHART_BAR_DENSE = 1.330 ms`
  - `CHART_SCATTER_DENSE = 1.167 ms`
  - `CHART_PIE_DENSE = 2.184 ms`

## 验证

- `python scripts/perf_analysis/main.py --profile cortex-m3`
  - 结果：`256` 个场景全部通过
- `python scripts/code_runtime_check.py --app HelloPerformance --timeout 10 --keep-screenshots`
  - 结果：`ALL PASSED`
- `make all APP=HelloUnitTest PORT=pc_test`
- `output\main.exe`
  - 结果：`357/357 passed`

## 截图抽查

- `runtime_check_output/HelloPerformance/default/frame_0253.png`
- `runtime_check_output/HelloPerformance/default/frame_0255.png`
- `runtime_check_output/HelloPerformance/default/frame_0256.png`

结论：

- `line / scatter / pie` 场景显示正常
- 没有看到折线断裂、点缺失、legend 异常或 pie 扇区缺块
