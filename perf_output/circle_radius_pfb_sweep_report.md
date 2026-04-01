# Circle Radius Threshold PFB Sweep Report

## 测试信息

- 日期: 2026-04-01
- 应用: HelloPerformance
- Profile: cortex-m3 (QEMU `mps2-an385`, `-icount shift=0`)
- 数据文件: `perf_output/circle_radius_pfb_sweep.json`
- 场景说明:
  - `CIRCLE_FILL`: 半径 `119`，当前 helper 计算得到 `compact_tile_limit = 30`
  - `CIRCLE_FILL_DOUBLE`: 半径 `239`，当前 helper 计算得到 `compact_tile_limit = 60`
  - `CIRCLE_FILL_HQ`: 作为标准半径下的代表性参考对照
- 说明: Windows 下长 `APP_OBJ_SUFFIX` 会让 QEMU perf 构建在链接阶段触发 `collect2.exe CreateProcess` 长命令问题。本轮 sweep 仅缩短了构建产物目录别名，不影响参与 benchmark 的业务代码与编译宏。

## CIRCLE_FILL 定向 PFB 数据

| PFB | 预期路径 | 耗时 ms |
| --- | --- | ---: |
| 8x8 | legacy_compact_tile | 3.199 |
| 12x12 | legacy_compact_tile | 1.690 |
| 15x15 | legacy_compact_tile | 1.234 |
| 24x10 | legacy_compact_tile | 1.179 |
| 30x8 | legacy_compact_tile | 1.212 |
| 30x30 | legacy_compact_tile | 0.532 |
| 40x6 | row_wise_tile | 0.795 |
| 48x16 | row_wise_tile | 0.588 |
| 60x12 | row_wise_tile | 0.517 |
| 80x10 | row_wise_tile | 0.433 |
| 240x1 | row_wise_horizontal_strip | 0.414 |
| 1x240 | legacy_vertical_strip | 4.881 |
| 240x240 | row_wise_tile | 0.222 |

## CIRCLE_FILL_DOUBLE 定向 PFB 数据

| PFB | 预期路径 | 耗时 ms |
| --- | --- | ---: |
| 8x8 | legacy_compact_tile | 4.221 |
| 12x12 | legacy_compact_tile | 2.139 |
| 15x15 | legacy_compact_tile | 1.514 |
| 24x10 | legacy_compact_tile | 1.424 |
| 30x8 | legacy_compact_tile | 1.473 |
| 30x30 | legacy_compact_tile | 0.595 |
| 40x6 | legacy_compact_tile | 1.542 |
| 48x16 | legacy_compact_tile | 0.658 |
| 60x12 | legacy_compact_tile | 0.717 |
| 80x10 | row_wise_tile | 0.430 |
| 240x1 | row_wise_horizontal_strip | 0.398 |
| 1x240 | legacy_vertical_strip | 4.872 |
| 240x240 | row_wise_tile | 0.206 |

## CIRCLE_FILL_HQ 代表性参考数据

| PFB | 耗时 ms | 相对 basic |
| --- | ---: | ---: |
| 15x15 | 1.238 | basic -0.3% |
| 30x30 | 0.647 | basic -17.8% |
| 48x16 | 0.487 | basic +20.7% |
| 60x12 | 0.438 | basic +18.0% |
| 80x10 | 0.376 | basic +15.2% |
| 240x1 | 0.396 | basic +4.5% |
| 1x240 | 12.697 | basic -61.6% |
| 240x240 | 0.211 | basic +5.2% |

## 结论

1. 当前 helper 的“半径相关阈值”确实把二维 compact-tile 分界从标准半径场景的 `30` 扩展到了双倍半径场景的 `60`，并且横条/竖条的保留语义与预期一致：`240x1` 始终走 row-wise，`1x240` 始终保留 legacy。
2. 对标准半径 `CIRCLE_FILL`，`30x30` 是 compact legacy 区间内的最佳点之一，继续增大到更宽的 row-wise tile 后，性能进入更优平台，`80x10 = 0.433ms`，`240x240 = 0.222ms`。
3. 对双倍半径 `CIRCLE_FILL_DOUBLE`，`80x10` 作为第一个明显越过 `60` 阈值的 row-wise case，耗时降到 `0.430ms`，而 `40x6 / 48x16 / 60x12` 仍留在 legacy 区间，耗时分别为 `1.542 / 0.658 / 0.717ms`。这说明“按半径缩放阈值”比固定 `30` 更通用，但对 `40~60` 之间的扁长 tile 仍然偏保守。
4. HQ 对照说明当前 basic fill 在小 compact tile、`30x30`、竖向 strip 上仍有自己的性能价值；但在典型 row-wise tile (`48x16 / 60x12 / 80x10`) 上，HQ 仍快 `15%~21%`，因此这轮数据支持“generic rule 已经消除屏幕尺寸 magic number”，但不支持把当前 basic fill 宣称为所有 PFB 形状下都优于 HQ。

## 未证实点

1. 本轮数据没有对 `CIRCLE_FILL_DOUBLE` 的 `40x6 / 48x16 / 60x12` 做“同一 PFB 下强制 row-wise”A/B，因此目前只能确认半径相关阈值在这些形状上表现偏保守，不能直接证明 `60` 就是该场景的最优二维切点。
2. 如果下一轮要继续收敛规则，建议优先只对 `40x6 / 48x16 / 60x12` 增补强制 row-wise 对照，验证是否需要把二维 compact 判断从“只看 max(width,height)”进一步收紧到“同时考虑短边或面积”。