# 2026-04-11 HelloPerformance bar/pie 可见窗口与中心像素优化

## 目标

- 在不增加额外 RAM/heap 的前提下，继续优化 `HelloPerformance` 里的重型 chart 场景。
- 本轮优先处理 `CHART_BAR_DENSE` 和 `CHART_PIE_DENSE` 的 PFB 重复开销。

## 保留改动

- `src/widget/egui_view_chart_bar.c`
  - 在 bar 绘制前，基于当前 `base_view_work_region` 反推出可见 group 索引窗口
  - 每个 tile 只遍历当前水平可见的 bar group，不再固定从 `0..n_points-1` 全量扫描后再做 `continue/break`
  - 不增加额外缓存，也不改变 bar 的绘制顺序和视觉结果
- `src/widget/egui_view_chart_pie.c`
  - pie 的中心像素修补只在当前 tile 真正包含圆心时才执行
  - 去掉非中心 tile 上无意义的 `draw_rectangle_fill(1x1)` 调用

## 未保留实验

- `CHART_PIE_DENSE` 逻辑 PFB A/B
  - 强制逻辑 `64x12`：`2.177 ms`
  - 强制逻辑 `96x8`：`2.332 ms`
  - 强制逻辑 `128x6`：`2.575 ms`
  - 当前默认逻辑 walk：`2.177 ms`
  - `64x12` 与默认持平，收益不足以增加 shipped 复杂度；`96x8/128x6` 明显回归，因此不保留

## 增量 A/B

基线为提交 `98846d4`，即已经保留 `line/scatter` 映射快路径、但尚未做本次 bar/pie 微优化的版本。

| 场景 | 增量基线(ms) | 增量当前(ms) | 变化 | 额外 heap |
| --- | ---: | ---: | ---: | --- |
| CHART_BAR_DENSE | 1.330 | 1.318 | -0.9% | 0 |
| CHART_PIE_DENSE | 2.184 | 2.177 | -0.3% | 0 |

## 当前关键结果

- `python scripts/perf_analysis/main.py --profile cortex-m3`
  - `FILE_IMAGE_JPG = 0.330 ms`
  - `FILE_IMAGE_PNG = 0.330 ms`
  - `FILE_IMAGE_BMP = 0.330 ms`
  - `CHART_LINE_DENSE = 1.321 ms`
  - `CHART_BAR_DENSE = 1.318 ms`
  - `CHART_SCATTER_DENSE = 1.167 ms`
  - `CHART_PIE_DENSE = 2.177 ms`

## 验证

- `python scripts/perf_analysis/main.py --profile cortex-m3`
  - 结果：`256` 个场景全部通过
- `python scripts/code_runtime_check.py --app HelloPerformance --timeout 10 --keep-screenshots`
  - 结果：`ALL PASSED`
- `make all APP=HelloUnitTest PORT=pc_test`
- `output\main.exe`
  - 结果：`357/357 passed`

## 截图抽查

- `runtime_check_output/HelloPerformance/default/frame_0254.png`
- `runtime_check_output/HelloPerformance/default/frame_0256.png`

结论：

- `bar / pie` 场景显示正常
- 没有看到柱状图缺列、legend 异常、pie 中心漏绘或扇区缺块
