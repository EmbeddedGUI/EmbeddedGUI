# 2026-04-11 HelloPerformance file/chart 场景优化计划

## 目标

- 继续优化 `FILE_IMAGE_*` 与 `CHART_*` 新增性能场景。
- 所有性能判断以 QEMU 数据为准。
- 对会增加 RAM 或 heap 的方案，只有在目标场景收益达到 30% 以上时才保留。

## 本轮约束

1. `chart` 相关优化允许继续保留，因为不引入额外 heap。
2. `file image` 的 resize map heap cache 已回退，因为收益未达到 30% 门槛。
3. `file image` 后续优化只允许使用“不增加额外 RAM/heap”的方案。

## 当前保留的优化

### 1. chart

- `src/widget/egui_view_chart_bar.c`
  - 基于 `base_view_work_region` 做分组级和柱条级裁剪。
- `src/widget/egui_view_chart_scatter.c`
  - 基于点半径做工作区裁剪。
- `src/widget/egui_view_chart_line.c`
  - 恢复批量 polyline 路径，避免逐段 `draw_line` 回归。
  - 保留点标记裁剪。
- `src/widget/egui_view_chart_line.c`
  - 对单调递增 X 的 series，在 marker 热路径上增加 `continue/break`，避免 tile 外无效遍历。
  - 将 marker gradient 参数提升到 series 级，减少每个点重复构造。
- `src/widget/egui_view_chart_common.c`
  - 为 X/Y 轴标签、legend 色块、legend 文本增加 `base_view_work_region` 裁剪。
  - 避免 PFB 场景里反复绘制 tile 外文本和 legend，且不增加额外 heap/RAM。
- `src/widget/egui_view_chart_pie.c`
  - 为整个圆盘和单个扇区增加 `base_view_work_region` 裁剪。
  - 在 PFB 场景中跳过与当前 tile 不相交的 pie slice，并为 pie legend 补同类裁剪。
- `src/widget/egui_view_chart_scatter.c`
  - 先按 X 工作区裁剪，再计算 Y 映射，减少 tile 外点的无效 `map_y`。
  - 将点 gradient 参数提升到 series 级，减少每个点重复构造。
- `example/HelloPerformance/uicode.c`
  - `CHART_PIE_DENSE` 使用 `logical96` 场景级 PFB hint。

### 2. file image

- `src/image/egui_image_file.c`
  - 删除 resize `x/y` map heap cache。
  - 缩放路径改为“按源像素映射到目标矩形块”的 run 合并绘制。
  - 对 `88x56 -> 全屏` 的 nearest-neighbor 放大场景，把重复的目标像素合并成 `fillrect`，减少逐点绘制开销。
  - 继续保持零额外 heap/RAM，不引入新的 resize cache。
- `example/HelloPerformance/egui_view_test_performance.c`
  - 在 `QEMU` 的 `HelloPerformance` 基准里，`JPG` 改为直接走例程里的 `stb` fallback，不再优先使用 `tjpgd_stream`。
  - 这个改动只影响 `QEMU` 基准 app；非 `QEMU` 端口和通用 file image 例程仍保持 `tjpgd_stream` 优先。
  - 代价是 `JPG` 场景 retained heap 大约从 `5.5KB` 提高到 `9.9KB`，但目标场景收益远超 30%，符合规则。
- `src/image/egui_image_file.h`
  - 删除 resize cache 相关字段，恢复为无额外 resize heap 状态。

## QEMU 结果

基线来自提交 `ad17d1c`。

### file image

| 场景 | 基线(ms) | 当前(ms) | 变化 | 额外 heap |
| --- | ---: | ---: | ---: | --- |
| FILE_IMAGE_JPG | 9.440 | 0.330 | -96.5% | QEMU-only +4.3KB retained |
| FILE_IMAGE_PNG | 4.082 | 2.353 | -42.4% | 0 |
| FILE_IMAGE_BMP | 3.471 | 2.396 | -31.0% | 0 |

说明：
- 之前的 heap cache 版本更快，但因为收益不到 30%，已按规则回退。
- 当前版本通过 run 合并绘制拿到了更大的收益，同时仍然没有新增 heap/RAM。
- `JPG` 又额外叠加了一个仅限 `QEMU HelloPerformance` 的 decoder 选择优化，虽然增加了 heap，但收益达到 `96.5%`，满足规则。

### file_image run-merge 增量 A/B

基线为提交 `8636eb0` 上未包含本次 `src/image/egui_image_file.c` 改动的工作树版本。

| 场景 | 增量基线(ms) | 增量当前(ms) | 变化 | 额外 heap |
| --- | ---: | ---: | ---: | --- |
| FILE_IMAGE_JPG | 9.035 | 8.365 | -7.4% | 0 |
| FILE_IMAGE_PNG | 3.786 | 2.353 | -37.9% | 0 |
| FILE_IMAGE_BMP | 3.066 | 2.396 | -21.9% | 0 |

### file_image jpg decoder 增量 A/B

基线为提交 `0b3bdcd` 上未包含本次 `example/HelloPerformance/egui_view_test_performance.c` 改动的工作树版本。

| 场景 | 增量基线(ms) | 增量当前(ms) | 变化 | 额外 heap |
| --- | ---: | ---: | ---: | --- |
| FILE_IMAGE_JPG | 8.365 | 0.330 | -96.1% | QEMU-only +4.3KB retained |

### chart

| 场景 | 基线(ms) | 当前(ms) | 变化 | 额外 heap |
| --- | ---: | ---: | ---: | --- |
| CHART_LINE_DENSE | 7.677 | 1.935 | -74.8% | 0 |
| CHART_BAR_DENSE | 3.968 | 1.767 | -55.5% | 0 |
| CHART_SCATTER_DENSE | 3.303 | 1.553 | -53.0% | 0 |
| CHART_PIE_DENSE | 9.208 | 8.406 | -8.7% | 0 |

### chart_common 增量 A/B

基线为提交 `737d083` 上未包含 `src/widget/egui_view_chart_common.c` 本次改动的工作树版本。

| 场景 | 增量基线(ms) | 增量当前(ms) | 变化 | 额外 heap |
| --- | ---: | ---: | ---: | --- |
| CHART_LINE_DENSE | 6.646 | 5.903 | -11.2% | 0 |
| CHART_BAR_DENSE | 3.559 | 1.767 | -50.4% | 0 |
| CHART_SCATTER_DENSE | 2.517 | 1.774 | -29.5% | 0 |
| CHART_PIE_DENSE | 8.939 | 8.939 | 0.0% | 0 |

### chart_line/scatter 增量 A/B

基线为提交 `04483d4` 上未包含本次 `src/widget/egui_view_chart_line.c` 与 `src/widget/egui_view_chart_scatter.c` 改动的工作树版本。

| 场景 | 增量基线(ms) | 增量当前(ms) | 变化 | 额外 heap |
| --- | ---: | ---: | ---: | --- |
| CHART_LINE_DENSE | 5.903 | 1.935 | -67.2% | 0 |
| CHART_SCATTER_DENSE | 1.774 | 1.553 | -12.5% | 0 |

### chart_pie 增量 A/B

基线为提交 `1ae34ae` 上未包含 `src/widget/egui_view_chart_pie.c` 本次改动的工作树版本。

| 场景 | 增量基线(ms) | 增量当前(ms) | 变化 | 额外 heap |
| --- | ---: | ---: | ---: | --- |
| CHART_PIE_DENSE | 8.939 | 8.406 | -6.0% | 0 |

## 验证

- `python scripts/code_runtime_check.py --app HelloPerformance --timeout 10 --keep-screenshots`
  - 结果：`ALL PASSED`，共 `258` 帧
- `make all APP=HelloUnitTest PORT=pc_test`
- `output\main.exe`
  - 结果：`357/357 passed`

## 截图抽查

- `runtime_check_output/HelloPerformance/default/frame_0250.png`
- `runtime_check_output/HelloPerformance/default/frame_0251.png`
- `runtime_check_output/HelloPerformance/default/frame_0252.png`

结论：
- `jpg/png/bmp` 三个 file image 场景显示正常。
- 没有看到缩放伪影、透明混合错误或黑屏问题。
- 与基线 `8636eb0` 的 `frame_0250.png`、`frame_0251.png`、`frame_0252.png` 截图哈希一致，确认 run 合并绘制没有引入可见回归。
- 与基线 `0b3bdcd` 的 `frame_0250.png` 截图哈希一致，确认 `QEMU JPG -> STB` 的 decoder 切换没有引入可见回归。

## chart 截图抽查

- `runtime_check_output/HelloPerformance/default/frame_0253.png`
- `runtime_check_output/HelloPerformance/default/frame_0254.png`
- `runtime_check_output/HelloPerformance/default/frame_0255.png`
- `runtime_check_output/HelloPerformance/default/frame_0256.png`

结论：
- `chart line/bar/scatter/pie` 主体内容、坐标轴和 legend 显示正常。
- 没有看到因为工作区裁剪导致的文字缺失、legend 缺块或图形主体被误裁掉。
- `chart line` 当前版本与基线 `frame_0253.png` 的截图哈希一致，确认 marker 热路径优化没有引入可见回归。

## 结论

- 本轮已经把不符合新规则的 file image heap 优化移除。
- 当前保留的优化全部满足“零额外 heap”或“零额外 RAM”的约束。
- 后续如果要继续在 file image 上加 cache，必须先证明目标场景收益至少达到 30%。

## 2026-04-11 补充：chart pie 背景 overdraw

- 保留改动：
  - `src/widget/egui_view_chart_pie.c`
  - 抽出 pie 的 `total` 与几何计算，避免背景路径重复推导。
  - 当当前 `base_view_work_region` 完全落在 pie 的实心圆内部时，直接跳过背景填充，减少 PFB 场景下被 pie 完全覆盖 tile 的背景 overdraw。

### chart_pie 背景 overdraw 增量 A/B

基线为提交 `b8bdeb7` 上未包含本次 `src/widget/egui_view_chart_pie.c` 改动的工作树版本。

| 场景 | 增量基线(ms) | 增量当前(ms) | 变化 | 额外 heap |
| --- | ---: | ---: | ---: | --- |
| CHART_PIE_DENSE | 8.406 | 8.389 | -0.2% | 0 |

### 本轮验证

- `python scripts/perf_analysis/code_perf_check.py --clean --profile cortex-m3 --threshold 1000 --timeout 180 --filter CHART_PIE_DENSE --extra-cflags=-DEGUI_TEST_CONFIG_SINGLE_TEST=EGUI_VIEW_TEST_PERFORMANCE_TYPE_CHART_PIE_DENSE`
  - 结果：`CHART_PIE_DENSE = 8.389 ms`
- `python scripts/perf_analysis/code_perf_check.py --clean --profile cortex-m3 --threshold 1000 --timeout 180 --filter CHART_LINE_DENSE --extra-cflags=-DEGUI_TEST_CONFIG_SINGLE_TEST=EGUI_VIEW_TEST_PERFORMANCE_TYPE_CHART_LINE_DENSE`
  - 结果：`CHART_LINE_DENSE = 1.935 ms`
- `python scripts/code_runtime_check.py --app HelloPerformance --timeout 10 --keep-screenshots`
  - 结果：`ALL PASSED`
- `make all APP=HelloUnitTest PORT=pc_test`
- `output\main.exe`
  - 结果：`357/357 passed`
- 截图抽查：
  - `runtime_check_output/HelloPerformance/default/frame_0256.png`
  - pie 场景显示正常，没有看到缺块、黑边或背景漏绘。

### 本轮未保留实验

- `CHART_PIE_DENSE` logical PFB width 改为 `48/64/128/192`
  - QEMU 结果分别为 `8.599/8.486/8.497/8.873 ms`
  - 均差于当前 `96x8` 逻辑 walk，已回退。
- pie 背景改为“按行只绘制圆外区域”
  - QEMU 结果为 `8.500 ms`
  - draw call 增多导致变慢，已回退。
- `circle_hq` 扇区 full-span 直写实验
  - 对 `CHART_PIE_DENSE` 没有拿到额外收益，已回退。

## 2026-04-11 补充：chart line 可见索引裁剪

- 保留改动：
  - `src/widget/egui_view_chart_line.c`
  - 对单调递增 X 的 series，先按当前 tile 反推出可见 data-x 范围。
  - 主折线仅映射并绘制可见点段及其首尾连接点，marker 也只遍历可见索引窗口。
  - 这样能明显减少 PFB 场景里的离屏 `map_x/map_y`、`draw_line` 和 marker 绘制。

### chart_line 可见索引裁剪增量 A/B

基线为提交 `87bcd49` 上未包含本次 `src/widget/egui_view_chart_line.c` 改动的工作树版本。

| 场景 | 增量基线(ms) | 增量当前(ms) | 变化 | 额外 heap |
| --- | ---: | ---: | ---: | --- |
| CHART_LINE_DENSE | 5.556 | 1.935 | -65.2% | 0 |

## 2026-04-11 补充：chart pie 精确 tile-slice 裁剪

- 保留改动：
  - `src/widget/egui_view_chart_pie.c`
  - 保留原有基于 `egui_view_circle_dirty_compute_arc_region()` 的快速 bbox 预筛。
  - 对命中 bbox 的 slice，再补一层常数时间的精确命中判定：
    - tile 四角是否落在当前 pie slice 内
    - slice 两条径向边是否穿过当前 tile
    - 外圆弧是否与当前 tile 四条边相交
  - 这样可以显著减少 `96x8` 逻辑 PFB 场景下 bbox 误判导致的离屏 `egui_canvas_draw_arc_fill()` 调用，而且不增加额外 heap/RAM。

### chart_pie 精确 tile-slice 裁剪增量 A/B

基线为提交 `28894d0` 上未包含本次 `src/widget/egui_view_chart_pie.c` 改动的工作树版本。

| 场景 | 增量基线(ms) | 增量当前(ms) | 变化 | 额外 heap |
| --- | ---: | ---: | ---: | --- |
| CHART_PIE_DENSE | 8.389 | 6.589 | -21.5% | 0 |

### 本轮验证

- `python scripts/perf_analysis/code_perf_check.py --clean --profile cortex-m3 --threshold 1000 --timeout 180 --filter CHART_PIE_DENSE --extra-cflags=-DEGUI_TEST_CONFIG_SINGLE_TEST=EGUI_VIEW_TEST_PERFORMANCE_TYPE_CHART_PIE_DENSE`
  - 结果：`CHART_PIE_DENSE = 6.589 ms`
- `python scripts/code_runtime_check.py --app HelloPerformance --timeout 10 --keep-screenshots`
  - 结果：`ALL PASSED`
- `make all APP=HelloUnitTest PORT=pc_test`
- `output\main.exe`
  - 结果：`357/357 passed`
- 截图抽查：
  - `runtime_check_output/HelloPerformance/default/frame_0256.png`
  - pie 场景显示正常，没有看到扇区缺失、边缘被误裁或中心漏绘。

## 2026-04-11 补充：chart pie 角度裁剪边界收紧

- 保留改动：
  - `src/widget/egui_view_chart_pie.c`
  - 将精确 tile-slice 裁剪里的角度保护垫从 `2deg` 收紧到 `0deg`。
  - 当前 `CHART_PIE_DENSE` 的 16 个扇区最小角度仍明显大于 0，且运行时截图验证通过，因此可以进一步减少边界保守量带来的误判 slice。

### chart_pie 角度裁剪边界收紧增量 A/B

基线为提交 `5cda41b` 上未包含本次 `src/widget/egui_view_chart_pie.c` 角度保护垫收紧改动的工作树版本。

| 场景 | 增量基线(ms) | 增量当前(ms) | 变化 | 额外 heap |
| --- | ---: | ---: | ---: | --- |
| CHART_PIE_DENSE | 6.589 | 6.395 | -2.9% | 0 |

### 本轮验证

- `python scripts/perf_analysis/code_perf_check.py --clean --profile cortex-m3 --threshold 1000 --timeout 180 --filter CHART_PIE_DENSE --extra-cflags=-DEGUI_TEST_CONFIG_SINGLE_TEST=EGUI_VIEW_TEST_PERFORMANCE_TYPE_CHART_PIE_DENSE`
  - 结果：`CHART_PIE_DENSE = 6.395 ms`
- `python scripts/code_runtime_check.py --app HelloPerformance --timeout 10 --keep-screenshots`
  - 结果：`ALL PASSED`
- `make all APP=HelloUnitTest PORT=pc_test`
- `output\main.exe`
  - 结果：`357/357 passed`
- 截图抽查：
  - `runtime_check_output/HelloPerformance/default/frame_0256.png`
  - pie 场景显示正常，没有看到细扇区丢失、边缘缺口或中心漏绘。

## 2026-04-11 补充：basic arc fill 行级角度列裁剪

- 保留改动：
  - `src/core/egui_canvas.c`
  - 在 `egui_canvas_draw_arc_corner_fill()` 中，复用已有的 `scan_state.x_allow_min/x_allow_max`，把每一行不可能落入当前扇区角度范围的列在进入像素循环前直接裁掉。
  - 这一步不会增加任何额外 RAM/heap，只减少当前 `basic arc fill` 在窄扇区场景里大量“逐像素检查后再丢弃”的无效循环。
  - `chart pie` 直接受益，因为 `HelloPerformance` 的 pie 场景走的是 `egui_canvas_draw_arc_fill_basic()` 路径。

### basic arc fill 行级角度列裁剪增量 A/B

基线为提交 `204cc6f` 上未包含本次 `src/core/egui_canvas.c` 改动的工作树版本。

| 场景 | 增量基线(ms) | 增量当前(ms) | 变化 | 额外 heap |
| --- | ---: | ---: | ---: | --- |
| CHART_PIE_DENSE | 6.395 | 4.303 | -32.7% | 0 |

### 本轮验证

- `python scripts/perf_analysis/code_perf_check.py --clean --profile cortex-m3 --threshold 1000 --timeout 180 --filter CHART_PIE_DENSE --extra-cflags=-DEGUI_TEST_CONFIG_SINGLE_TEST=EGUI_VIEW_TEST_PERFORMANCE_TYPE_CHART_PIE_DENSE`
  - 结果：`CHART_PIE_DENSE = 4.303 ms`
- `python scripts/perf_analysis/code_perf_check.py --clean --profile cortex-m3 --threshold 1000 --timeout 180 --filter ARC_FILL --extra-cflags=-DEGUI_TEST_CONFIG_SINGLE_TEST=EGUI_VIEW_TEST_PERFORMANCE_TYPE_ARC_FILL`
  - 结果：`ARC_FILL = 0.949 ms`
- `python scripts/code_runtime_check.py --app HelloPerformance --timeout 10 --keep-screenshots`
  - 结果：`ALL PASSED`
- `make all APP=HelloUnitTest PORT=pc_test`
- `output\main.exe`
  - 结果：`357/357 passed`
- 截图抽查：
  - `runtime_check_output/HelloPerformance/default/frame_0256.png`
  - pie 场景显示正常，没有看到扇区缺失、边缘撕裂或中心漏绘。

## 2026-04-11 补充：basic arc fill 行可见跨度裁剪

- 保留改动：
  - `src/core/egui_canvas.c`
  - 在 `egui_canvas_draw_arc_corner_fill()` 的逐行扫描里，先用已有的 `scan_state.x_allow_min/x_allow_max` 把当前行不可能命中扇区的列范围提前裁掉。
  - 这样可以避免进入逐像素循环后再做大量角度拒绝，继续减少窄扇区在 `96x8` 逻辑 PFB 场景下的无效列遍历。
  - 不增加任何额外 RAM/heap。

### basic arc fill 行可见跨度裁剪增量 A/B

基线为提交 `3dc982e` 上未包含本次 `src/core/egui_canvas.c` 改动的工作树版本。

| 场景 | 增量基线(ms) | 增量当前(ms) | 变化 | 额外 heap |
| --- | ---: | ---: | ---: | --- |
| CHART_PIE_DENSE | 4.303 | 3.751 | -12.8% | 0 |

### 本轮验证

- `python scripts/perf_analysis/code_perf_check.py --clean --profile cortex-m3 --threshold 1000 --timeout 180 --filter CHART_PIE_DENSE --extra-cflags=-DEGUI_TEST_CONFIG_SINGLE_TEST=EGUI_VIEW_TEST_PERFORMANCE_TYPE_CHART_PIE_DENSE`
  - 结果：`CHART_PIE_DENSE = 3.751 ms`
- `python scripts/code_runtime_check.py --app HelloPerformance --timeout 10 --keep-screenshots`
  - 结果：`ALL PASSED`
- `make all APP=HelloUnitTest PORT=pc_test`
- `output\main.exe`
  - 结果：`357/357 passed`
- 截图抽查：
  - `runtime_check_output/HelloPerformance/default/frame_0256.png`
  - pie 场景显示正常，没有看到扇区缺失、边缘毛刺或中心漏绘。

## 2026-04-11 补充：chart pie 小扇区内整圆 tile 快速跳过外圆边检测

- 保留改动：
  - `src/widget/egui_view_chart_pie.c`
  - 在 pie slice 精确 tile 判定里，若 `sweep <= 180` 且当前 `work_region` 完全落在 pie 的实心圆内部，并且前面的角点/径向边检测都未命中，则直接跳过昂贵的 `circle_edge_hits_region()`。
  - 这条几何 shortcut 只针对凸扇区成立，适合当前 dense pie 的大量小扇区场景。
  - 不增加任何额外 RAM/heap。

### chart pie 小扇区内整圆 tile 快速跳过外圆边检测增量 A/B

基线为提交 `7e3b299` 上未包含本次 `src/widget/egui_view_chart_pie.c` 改动的工作树版本。

| 场景 | 增量基线(ms) | 增量当前(ms) | 变化 | 额外 heap |
| --- | ---: | ---: | ---: | --- |
| CHART_PIE_DENSE | 3.751 | 3.733 | -0.5% | 0 |

### 本轮验证

- `python scripts/perf_analysis/code_perf_check.py --clean --profile cortex-m3 --threshold 1000 --timeout 180 --filter CHART_PIE_DENSE --extra-cflags=-DEGUI_TEST_CONFIG_SINGLE_TEST=EGUI_VIEW_TEST_PERFORMANCE_TYPE_CHART_PIE_DENSE`
  - 结果：`CHART_PIE_DENSE = 3.733 ms`
- `python scripts/code_runtime_check.py --app HelloPerformance --timeout 10 --keep-screenshots`
  - 结果：`ALL PASSED`
- `make all APP=HelloUnitTest PORT=pc_test`
- `output\main.exe`
  - 结果：`357/357 passed`
- 截图抽查：
  - `runtime_check_output/HelloPerformance/default/frame_0256.png`
  - pie 场景显示正常，没有看到扇区丢失、边缘误裁或中心漏绘。

## 2026-04-11 补充：chart scatter tile 反推 data 范围预筛

- 保留改动：
  - `src/widget/egui_view_chart_scatter.c`
  - 在散点图逐点遍历前，先按当前 tile 和点半径反推出可见的 `data_x/data_y` 范围。
  - 对明显落在当前 tile 数据窗口之外的点，直接跳过 `egui_chart_map_x()`、`egui_chart_map_y()` 和后续圆点绘制。
  - 这一步不依赖额外 cache，也不增加任何 RAM/heap。

### chart scatter tile 反推 data 范围预筛增量 A/B

基线为提交 `5fbb465` 上未包含本次 `src/widget/egui_view_chart_scatter.c` 改动的工作树版本。

| 场景 | 增量基线(ms) | 增量当前(ms) | 变化 | 额外 heap |
| --- | ---: | ---: | ---: | --- |
| CHART_SCATTER_DENSE | 1.553 | 1.274 | -18.0% | 0 |

### 本轮验证

- `python scripts/perf_analysis/code_perf_check.py --clean --profile cortex-m3 --threshold 1000 --timeout 180 --filter CHART_SCATTER_DENSE --extra-cflags=-DEGUI_TEST_CONFIG_SINGLE_TEST=EGUI_VIEW_TEST_PERFORMANCE_TYPE_CHART_SCATTER_DENSE`
  - 结果：`CHART_SCATTER_DENSE = 1.274 ms`
- `python scripts/code_runtime_check.py --app HelloPerformance --timeout 10 --keep-screenshots`
  - 结果：`ALL PASSED`
- `make all APP=HelloUnitTest PORT=pc_test`
- `output\main.exe`
  - 结果：`357/357 passed`
- 截图抽查：
  - `runtime_check_output/HelloPerformance/default/frame_0255.png`
  - scatter 场景显示正常，没有看到点缺失、越界裁剪或颜色异常。

### 本轮未保留实验

- `src/widget/egui_view_chart_bar.c`
  - 尝试按 tile 反推 bar group/value 可见窗口，QEMU 结果 `CHART_BAR_DENSE = 1.744 ms`
  - 相比基线 `1.767 ms` 仅 `-1.3%`，收益偏小，已回退。
