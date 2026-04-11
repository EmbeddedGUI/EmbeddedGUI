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
- `src/widget/egui_view_chart_common.c`
  - 为 X/Y 轴标签、legend 色块、legend 文本增加 `base_view_work_region` 裁剪。
  - 避免 PFB 场景里反复绘制 tile 外文本和 legend，且不增加额外 heap/RAM。
- `src/widget/egui_view_chart_pie.c`
  - 为整个圆盘和单个扇区增加 `base_view_work_region` 裁剪。
  - 在 PFB 场景中跳过与当前 tile 不相交的 pie slice，并为 pie legend 补同类裁剪。
- `example/HelloPerformance/uicode.c`
  - `CHART_PIE_DENSE` 使用 `logical96` 场景级 PFB hint。

### 2. file image

- `src/image/egui_image_file.c`
  - 删除 resize `x/y` map heap cache。
  - 缩放路径改为“单次初始化 + 增量步进”的无额外内存映射。
  - `alpha/no-alpha` 与 `mask/no-mask` 分开走循环，减少热点里的分支判断。
- `src/image/egui_image_file.h`
  - 删除 resize cache 相关字段，恢复为无额外 resize heap 状态。

## QEMU 结果

基线来自提交 `ad17d1c`。

### file image

| 场景 | 基线(ms) | 当前(ms) | 变化 | 额外 heap |
| --- | ---: | ---: | ---: | --- |
| FILE_IMAGE_JPG | 9.440 | 9.035 | -4.3% | 0 |
| FILE_IMAGE_PNG | 4.082 | 3.786 | -7.3% | 0 |
| FILE_IMAGE_BMP | 3.471 | 3.066 | -11.7% | 0 |

说明：
- 之前的 heap cache 版本更快，但因为收益不到 30%，已按规则回退。
- 当前版本虽然绝对收益较小，但没有新增 heap，因此允许保留。

### chart

| 场景 | 基线(ms) | 当前(ms) | 变化 | 额外 heap |
| --- | ---: | ---: | ---: | --- |
| CHART_LINE_DENSE | 7.677 | 5.903 | -23.1% | 0 |
| CHART_BAR_DENSE | 3.968 | 1.767 | -55.5% | 0 |
| CHART_SCATTER_DENSE | 3.303 | 1.774 | -46.3% | 0 |
| CHART_PIE_DENSE | 9.208 | 8.406 | -8.7% | 0 |

### chart_common 增量 A/B

基线为提交 `737d083` 上未包含 `src/widget/egui_view_chart_common.c` 本次改动的工作树版本。

| 场景 | 增量基线(ms) | 增量当前(ms) | 变化 | 额外 heap |
| --- | ---: | ---: | ---: | --- |
| CHART_LINE_DENSE | 6.646 | 5.903 | -11.2% | 0 |
| CHART_BAR_DENSE | 3.559 | 1.767 | -50.4% | 0 |
| CHART_SCATTER_DENSE | 2.517 | 1.774 | -29.5% | 0 |
| CHART_PIE_DENSE | 8.939 | 8.939 | 0.0% | 0 |

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

## chart 截图抽查

- `runtime_check_output/HelloPerformance/default/frame_0253.png`
- `runtime_check_output/HelloPerformance/default/frame_0254.png`
- `runtime_check_output/HelloPerformance/default/frame_0255.png`
- `runtime_check_output/HelloPerformance/default/frame_0256.png`

结论：
- `chart line/bar/scatter/pie` 主体内容、坐标轴和 legend 显示正常。
- 没有看到因为工作区裁剪导致的文字缺失、legend 缺块或图形主体被误裁掉。

## 结论

- 本轮已经把不符合新规则的 file image heap 优化移除。
- 当前保留的优化全部满足“零额外 heap”或“零额外 RAM”的约束。
- 后续如果要继续在 file image 上加 cache，必须先证明目标场景收益至少达到 30%。
