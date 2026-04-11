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
| CHART_LINE_DENSE | 7.677 | 6.646 | -13.4% | 0 |
| CHART_BAR_DENSE | 3.968 | 3.559 | -10.3% | 0 |
| CHART_SCATTER_DENSE | 3.303 | 2.517 | -23.8% | 0 |
| CHART_PIE_DENSE | 9.208 | 8.939 | -2.9% | 0 |

## 验证

- `python scripts/code_runtime_check.py --app HelloPerformance --timeout 10 --keep-screenshots`
  - 结果：`ALL PASSED`
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

## 结论

- 本轮已经把不符合新规则的 file image heap 优化移除。
- 当前保留的优化全部满足“零额外 heap”或“零额外 RAM”的约束。
- 后续如果要继续在 file image 上加 cache，必须先证明目标场景收益至少达到 30%。
