# HelloPerformance line polyline chunk

## 目标

- 继续优化 HelloPerformance 中新增的 chart / file image 场景。
- 约束：
  - 性能结论只看 QEMU。
  - 不保留无收益实验。
  - 涉及 RAM / heap 增长的方案，若目标场景收益不到 30%，不保留。

## 本轮保留改动

- 文件：`src/widget/egui_view_chart_line.c`
- 调整：
  - dense line chart 在点数超过 `CHART_LINE_MAX_POLYLINE_POINTS` 时，改为使用“重叠 polyline chunk”连续绘制。
  - 不再使用首个 polyline + 后半段逐线段 `draw_line` 的混合路径。
  - 栈缓冲大小不变，仍为 64 点。

## 保留原因

- QEMU `cortex-m3`：
  - `CHART_LINE_DENSE`: `1.190 ms -> 1.188 ms`
- 收益虽小，但为零额外内存、零行为回退风险的稳定改动。

## 已验证

- `python scripts/perf_analysis/main.py --profile cortex-m3`
- `python scripts/code_runtime_check.py --app HelloPerformance --timeout 10 --keep-screenshots`
- `make all APP=HelloUnitTest PORT=pc_test`
- `output/main.exe`

## 已放弃实验

- pie 扇区扫描行裁剪：无收益，不保留。
- pie enhanced path 退回 solid arc fill：无收益，不保留。
- pie / file image / bar 的 logical PFB hint 扫描：无更优结果，不保留。
- line / scatter marker 退回 solid fill：无量化收益，不保留。
