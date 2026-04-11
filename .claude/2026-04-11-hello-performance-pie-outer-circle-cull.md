# HelloPerformance pie 外圆 tile 裁剪优化记录

## 背景

- 基线提交：`f3a7511`
- 目标场景：`CHART_PIE_DENSE`
- 约束：
  - 以 QEMU 为唯一性能依据
  - 不引入额外 heap
  - 涉及 RAM 占用变化的方案若收益不足 30% 不保留

## 本轮保留改动

### 1. pie 增加外圆真实相交早退

文件：`src/widget/egui_view_chart_pie.c`

- 新增 `egui_view_chart_pie_work_region_intersects_outer_circle()`
- 在 `egui_view_chart_pie_draw_pie()` 进入 slice 循环前，先判断当前 work region 是否与 `radius + 1` 的外圆真实相交
- 目的：
  - 直接跳过“落在 pie 包围盒内，但完全不碰圆”的角落 tile
  - 避免这些 tile 继续走角度窗口与 arc 绘制路径

### 2. 重新校准 dense pie 的 logical PFB hint

文件：`example/HelloPerformance/uicode.c`

- `CHART_PIE_DENSE` 的 logical PFB hint 从默认走法改为 `64`
- 重新实测后，新的外圆裁剪与 `64x12` 逻辑走块组合略优于默认 `48x16`

## 放弃的实验

### 边界 tile 强制走精确 slice 命中判断

- 做法：只让 `inside_solid_circle` 的 tile 使用纯角度窗口快路径，其余边界 tile 回退到 `egui_view_chart_pie_slice_intersects_work_region()`
- 结果：`CHART_PIE_DENSE 2.148 -> 2.236 ms`
- 结论：几何命中判断成本高于减少的 overdraw，回退

## QEMU 结果

### 单场景 A/B

| 版本 | CHART_PIE_DENSE |
|------|-----------------|
| 基线 `f3a7511` | `2.177 ms` |
| 仅外圆真实相交早退 | `2.148 ms` |
| 外圆早退 + pie logical PFB `64` | `2.143 ms` |

### 当前保留结果

- `CHART_PIE_DENSE = 2.143 ms`
- 对比基线提升约 `1.56%`

## 验证

执行命令：

```bash
python scripts/perf_analysis/main.py --profile cortex-m3 --filter CHART_PIE_DENSE
python scripts/code_runtime_check.py --app HelloPerformance --timeout 10 --keep-screenshots
make all APP=HelloUnitTest PORT=pc_test
output\main.exe
python scripts/perf_analysis/main.py --profile cortex-m3
```

结果：

- `HelloPerformance` 运行时检查通过
- `HelloUnitTest` 357/357 通过
- 全量 `cortex-m3` profile 通过
- 关键场景：
  - `ARC_FILL = 0.825 ms`
  - `CHART_LINE_DENSE = 1.321 ms`
  - `CHART_BAR_DENSE = 1.318 ms`
  - `CHART_SCATTER_DENSE = 1.167 ms`
  - `CHART_PIE_DENSE = 2.143 ms`
  - `FILE_IMAGE_JPG/PNG/BMP = 0.330/0.330/0.330 ms`

截图输出：

- `runtime_check_output/HelloPerformance/default/frame_0254.png`
- `runtime_check_output/HelloPerformance/default/frame_0256.png`

补充检查：

- 两帧尺寸均为 `240x240`
- 非黑像素占比均为 `1.0`
- 未出现黑屏/空白统计特征

## 后续建议

- pie 下一步仍优先看“边界 tile 的更便宜误判裁剪”，但不要回到高成本几何命中判定
- 若继续尝试 PFB hint，只做不增加真实缓冲占用的逻辑走块 A/B
