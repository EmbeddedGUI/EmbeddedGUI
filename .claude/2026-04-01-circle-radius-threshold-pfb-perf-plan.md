# 2026-04-01 circle-radius-threshold-pfb-perf plan

## 目标

补一轮面向不同 PFB 尺寸的定向 QEMU perf，验证当前 `egui_canvas_should_use_circle_fill_basic_legacy_clip()` 的“半径相关阈值”在标准半径与双倍半径场景下的实际表现，并给出系统化回归数据。

## 执行方法

1. 使用 `HelloPerformance` 的 `EGUI_TEST_CONFIG_SINGLE_TEST` 单场景模式，分别运行 `CIRCLE_FILL`、`CIRCLE_FILL_DOUBLE`、`CIRCLE_FILL_HQ`。
2. 选取稠密 PFB 集合：`8x8`、`12x12`、`15x15`、`24x10`、`30x8`、`30x30`、`40x6`、`48x16`、`60x12`、`80x10`、`240x1`、`1x240`、`240x240`。
3. 运行时根据当前 helper 规则给每个 PFB 打标签：`legacy_compact_tile`、`row_wise_tile`、`row_wise_horizontal_strip`、`legacy_vertical_strip`。
4. Windows 上长 `APP_OBJ_SUFFIX` 会在链接阶段触发 `collect2.exe CreateProcess` 长命令问题，因此本轮 sweep 改用短别名 suffix，仅作为构建目录名，不改业务逻辑。

## 结果摘要

1. `CIRCLE_FILL` 半径 `119`，阈值 `30`。`30x30 = 0.532ms`，`80x10 = 0.433ms`，`240x240 = 0.222ms`。
2. `CIRCLE_FILL_DOUBLE` 半径 `239`，阈值 `60`。`60x12 = 0.717ms` 仍在 legacy 区间，而 `80x10 = 0.430ms` 已进入 row-wise 平台。
3. `240x1` 与 `1x240` 的 strip 语义保持稳定：横条始终 row-wise，竖条始终 legacy。
4. HQ 参考表明当前 basic fill 在 `15x15`、`30x30`、`1x240` 上仍有竞争力，但在 `48x16 / 60x12 / 80x10` 等典型 row-wise tile 上仍慢于 HQ。

## 结论

1. 这轮数据支持“当前规则已经去掉固定 `30x30` magic number，并且阈值会随半径扩张”。
2. 这轮数据同时暴露出一个更细的问题：双倍半径下 `40x6 / 48x16 / 60x12` 仍停留在 legacy 区间，说明按 `max(width,height)` 缩放出的二维 compact 判断对扁长 tile 偏保守。
3. 如果继续优化，下一轮应当只围绕 `40~60` 这段扁长 tile 做强制 row-wise A/B，而不是重跑整套 sweep。

## 产物

1. `perf_output/circle_radius_pfb_sweep.json`
2. `perf_output/circle_radius_pfb_sweep_report.md`