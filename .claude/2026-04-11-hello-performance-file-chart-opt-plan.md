# 2026-04-11 HelloPerformance file/chart 场景优化计划

## 目标

- 继续优化新加入的 `FILE_IMAGE_*` 与 `CHART_*` 性能场景。
- 明确区分“控件级优化”和“场景级 PFB 策略优化”。
- 以 QEMU 结果为唯一性能依据，避免 PC 计时误导。
- 改动完成后做 runtime check，并抽查截图。

## 本轮策略

1. 保留 file image 已验证有效的缩放映射缓存优化。
2. 保留 bar/scatter 已验证有效的 work-region 裁剪。
3. 回退 line chart 的逐段上层裁剪，恢复批量 polyline 路径，只保留点标记裁剪。
4. 对 `CHART_PIE_DENSE` 单独启用 `logical96` 场景级 PFB hint。

## 具体改动

### 1. file image

- 在 `src/image/egui_image_file.c` / `src/image/egui_image_file.h` 中增加 resize `x/y` 映射缓存。
- 目标是避免缩放路径每像素重复做除法。
- 缓存按目标尺寸和源尺寸失效/复用，释放逻辑放在 `deinit`。

### 2. chart bar / scatter

- 在 `src/widget/egui_view_chart_bar.c` 中基于 `base_view_work_region` 做分组级和柱条级裁剪。
- 在 `src/widget/egui_view_chart_scatter.c` 中基于点半径做工作区裁剪。
- 同时修正 work-region 读取方式，消除 `const` 告警。

### 3. chart line

- 在 `src/widget/egui_view_chart_line.c` 中恢复原有 bounded polyline 批量绘制路径。
- 不再在控件层逐段重复做 segment bbox 判断。
- 保留点标记裁剪，减少 PFB 场景下无效 circle draw。

### 4. chart pie

- 在 `example/HelloPerformance/uicode.c` 中给 `CHART_PIE_DENSE` 增加 `logical96` hint。
- 保持 `FILE_IMAGE_*` 继续使用 `logical192`，避免互相干扰。

## QEMU 结果

基线来自已提交版本 `ad17d1c`。

| 场景 | 基线(ms) | 本轮(ms) | 变化 |
| --- | ---: | ---: | ---: |
| FILE_IMAGE_JPG | 9.440 | 8.862 | -6.1% |
| FILE_IMAGE_PNG | 4.082 | 3.504 | -14.2% |
| FILE_IMAGE_BMP | 3.471 | 2.893 | -16.7% |
| CHART_LINE_DENSE | 7.677 | 6.646 | -13.4% |
| CHART_BAR_DENSE | 3.968 | 3.559 | -10.3% |
| CHART_SCATTER_DENSE | 3.303 | 2.517 | -23.8% |
| CHART_PIE_DENSE | 9.208 | 8.939 | -2.9% |

## 验证

- `python scripts/code_runtime_check.py --app HelloPerformance --timeout 10 --keep-screenshots`
- 结果：`ALL PASSED`
- 抽查截图：
  - `FILE_IMAGE_JPG`
  - `FILE_IMAGE_PNG`
  - `FILE_IMAGE_BMP`
  - `CHART_LINE_DENSE`
  - `CHART_BAR_DENSE`
  - `CHART_SCATTER_DENSE`
  - `CHART_PIE_DENSE`

## 结论

- 这轮优化在 file image 与 chart 四类新增场景上都取得了稳定收益。
- line chart 回归已经消除，并比原基线更快。
- pie scene 通过场景级 PFB hint 获得了小幅但稳定的 QEMU 改善。
