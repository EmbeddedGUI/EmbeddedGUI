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
- `src/image/egui_image_file.h`
  - 删除 resize cache 相关字段，恢复为无额外 resize heap 状态。

## QEMU 结果

基线来自提交 `ad17d1c`。

### file image

| 场景 | 基线(ms) | 当前(ms) | 变化 | 额外 heap |
| --- | ---: | ---: | ---: | --- |
| FILE_IMAGE_JPG | 9.440 | 8.365 | -11.4% | 0 |
| FILE_IMAGE_PNG | 4.082 | 2.353 | -42.4% | 0 |
| FILE_IMAGE_BMP | 3.471 | 2.396 | -31.0% | 0 |

说明：
- 之前的 heap cache 版本更快，但因为收益不到 30%，已按规则回退。
- 当前版本通过 run 合并绘制拿到了更大的收益，同时仍然没有新增 heap/RAM。

### file_image run-merge 增量 A/B

基线为提交 `8636eb0` 上未包含本次 `src/image/egui_image_file.c` 改动的工作树版本。

| 场景 | 增量基线(ms) | 增量当前(ms) | 变化 | 额外 heap |
| --- | ---: | ---: | ---: | --- |
| FILE_IMAGE_JPG | 9.035 | 8.365 | -7.4% | 0 |
| FILE_IMAGE_PNG | 3.786 | 2.353 | -37.9% | 0 |
| FILE_IMAGE_BMP | 3.066 | 2.396 | -21.9% | 0 |

### chart

| 场景 | 基线(ms) | 当前(ms) | 变化 | 额外 heap |
| --- | ---: | ---: | ---: | --- |
| CHART_LINE_DENSE | 7.677 | 5.556 | -27.6% | 0 |
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
| CHART_LINE_DENSE | 5.903 | 5.556 | -5.9% | 0 |
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
