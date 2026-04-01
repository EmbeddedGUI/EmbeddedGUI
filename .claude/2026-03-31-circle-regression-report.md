# CIRCLE Basic 与 HQ 回归分析报告

## 1. 结论摘要

- `basic` 仍然有保留必要，不能直接删除。
- 不建议把 `EGUI_CONFIG_CIRCLE_DEFAULT_ALGO_HQ` 改成全局默认值。
- `HQ` 不是全面更快，`basic` 也不是全面更小；两者的输赢强依赖 `PFB` 形状、图元类型，以及最终链接是否能把 LUT 和旧路径裁掉。
- 就当前 `HelloCanvas` 运行时验证看，`HQ` 没有表现出足以支撑“改默认”的显著可见画质优势。

一句话结论：`basic` 现在的价值不是“永远更优”，而是“在一部分关键场景下仍然明显更优，因此不能被 `HQ` 完全替代”。

补充结论：在不删除 `basic` 的前提下，对 `basic fill` 做保守优化后，`CIRCLE_FILL` 标准 perf 已从 `0.788ms` 降到 `0.587ms`；`small 15x15` 与 `middle 30x30` 下也已追平并反超 `HQ`，进一步说明更合理的方向是继续优化 `basic` 的场景化路径，而不是直接改默认或删除它。

## 2. 实验范围与方法

### 2.1 性能

- 标准性能回归：`cortex-m3`，`HelloPerformance`，比较 baseline 与 `-DEGUI_CONFIG_CIRCLE_DEFAULT_ALGO_HQ=1`
- `PFB matrix`：`small(15x15)`、`middle(30x30)`、`fullwidth(240x1)`、`fullheight(1x240)`、`fullscreen(240x240)`
- 性能数据全部来自 QEMU 回归，不使用 PC 模拟器计时结果。

### 2.2 体积

- `HelloSimple` + `qemu/cortex-m0plus`
- `HelloBasic/arc_slider` + `qemu/cortex-m0plus`
- `HelloSimple` + `stm32g0`
- 使用 `llvm-size` 看最终链接镜像，使用 `llvm-nm` 看 `basic/HQ` 相关符号和 `egui_res_circle_info*` LUT 是否仍在。

### 2.3 渲染

- `HelloCanvas` baseline 与 default-HQ 各跑一轮运行时验证
- 对截图做 hash 比较与像素差比较
- 对差异帧做人眼复核

### 2.4 方法学修正

首轮 default-HQ `PFB matrix` 结果与 baseline 完全一致，后排查出 perf 工具本身存在两处问题：

- `run_pfb_matrix()` 没有透传 CLI 传入的 `--extra-cflags`
- standalone `--pfb-matrix` / `--spi-matrix` 没有真正执行 `--clean`
- `APP_OBJ_SUFFIX` 没把 `extra_cflags` 编进目录名，A/B 宏实验容易复用旧对象

已修复 `scripts/code_perf_check.py` 后重新运行 matrix。本文所有 `PFB matrix` 结论，均以后一次修复后重跑的数据为准。

## 3. 标准性能回归结果

基线与 default-HQ 的关键默认 API 数据如下：

| 用例 | baseline(ms) | default-HQ(ms) | 变化 |
|---|---:|---:|---:|
| `CIRCLE` | 0.821 | 1.136 | `+38.4%` |
| `CIRCLE_FILL` | 0.788 | 0.487 | `-38.2%` |
| `ARC` | 1.149 | 1.084 | `-5.7%` |
| `ARC_FILL` | 2.115 | 1.686 | `-20.3%` |

这里可以直接看出两个事实：

- `HQ` 默认化后，填充类图元明显受益，尤其是 `CIRCLE_FILL`
- 但 `CIRCLE` 轮廓反而显著退化，退化幅度达到 `38.4%`

因此，只看标准 perf，就已经不能支持“把默认算法整体切到 HQ”。

## 4. PFB Matrix 回归结果

下表中的变化值，定义为“default-HQ 相对 baseline 的时间变化”；负值表示 `HQ` 更快，正值表示 `HQ` 更慢。

| PFB | `CIRCLE` | `CIRCLE_FILL` | `ARC` | `ARC_FILL` | 结论 |
|---|---:|---:|---:|---:|---|
| `small 15x15` | `-20.8%` | `-7.4%` | `-21.5%` | `-14.3%` | `HQ` 全面占优 |
| `middle 30x30` | `+27.4%` | `+1.4%` | `-6.1%` | `-15.8%` | 轮廓退化，弧与弧填充受益 |
| `fullwidth 240x1` | `+22.6%` | `-92.1%` | `-12.3%` | `-35.5%` | 水平条带下 `HQ fill` 极强 |
| `fullheight 1x240` | `-63.8%` | `+161.7%` | `-43.3%` | `+12.9%` | 垂直条带下结果反转 |
| `fullscreen 240x240` | `+181.7%` | `-32.6%` | `+22.9%` | `-20.7%` | 全屏缓冲下轮廓明显更适合 basic |

`PFB matrix` 给出的结论比标准 perf 更强：

- 不存在一个在所有 `PFB` 形状下都更优的默认算法
- `outline` 与 `fill` 的最优算法并不一致
- `PFB` 的长宽比会直接改变算法胜负，特别是 `240x1` 与 `1x240` 这种极端 strip 形态

如果把默认算法整体切到 `HQ`，会同时引入两类风险：

- 某些轮廓用例退化很大，例如 `fullscreen` 下 `CIRCLE +181.7%`
- 某些填充用例在特定 `PFB` 形状下也会大幅退化，例如 `fullheight` 下 `CIRCLE_FILL +161.7%`

这已经足够否定“一刀切 default-HQ”。

## 5. 代码体积结果

### 5.1 最终链接体积

| 场景 | baseline(dec) | default-HQ(dec) | 变化 | 观察 |
|---|---:|---:|---:|---|
| `HelloSimple qemu` | 305636 | 308876 | `+3240` (`+1.1%`) | minimal app 下切 `HQ` 反而变大 |
| `HelloBasic/arc_slider qemu` | 317960 | 307028 | `-10932` (`-3.4%`) | arc-heavy app 下切 `HQ` 明显变小 |
| `HelloSimple stm32g0` | 77360 | 82008 | `+4648` (`+6.0%`) | MCU 目标上放大更明显 |

### 5.2 符号级观察

`HelloSimple qemu` baseline：

- 有 `egui_canvas_draw_circle_fill_basic`
- 仍有 `egui_res_circle_info_arr` 与整组 `egui_res_circle_info_data/item_*`

`HelloSimple qemu` default-HQ：

- 基本路径切到 `egui_canvas_draw_circle_fill_hq`
- 但 `egui_res_circle_info*` LUT 仍然存在

这说明在 minimal app 场景里，切 `HQ` 只是新增了 `HQ` 绘制代码，但没有把 circle LUT 一起裁掉，所以镜像变大。

`HelloBasic/arc_slider qemu` baseline：

- 有 `egui_canvas_draw_arc_basic`
- 有 `egui_canvas_draw_arc_fill_basic`
- 有 `egui_canvas_draw_circle_fill_basic`
- 有大块 `egui_res_circle_info*` LUT

`HelloBasic/arc_slider qemu` default-HQ：

- 只剩 `egui_canvas_draw_arc_hq`
- 只剩 `egui_canvas_draw_arc_fill_hq`
- 只剩 `egui_canvas_draw_circle_fill_hq`
- `egui_res_circle_info*` LUT 已完全消失

这说明在 arc-heavy app 中，`HQ` 确实可能带来更小的最终镜像，因为它能连 LUT 和 basic 路径一起裁掉。

### 5.3 体积结论

体积上不能得出“basic 一定更有必要”或“HQ 一定能省代码”的结论。真正决定最终体积的，是：

- 当前 app 是否真的还在引用 `basic` 的 arc/circle 路径
- `egui_res_circle_info*` LUT 是否能在最终链接时被裁掉

所以，如果保留 `basic` 的理由是“它一定更小”，这个理由不成立；但如果要因此删除 `basic`，也同样站不住脚，因为 minimal app 和 MCU 目标下 default-HQ 反而更大。

## 6. 渲染效果结果

`HelloCanvas` baseline 与 default-HQ 对比结果：

- 两边都生成了 `11` 帧
- `9/11` 帧 hash 完全一致
- 差异帧只有 `frame_0001` 与 `frame_0004`
- 像素差数量：
  - `frame_0001`: `3699`
  - `frame_0004`: `997`

对这两帧的人眼复核结果：

- 画面布局、居中、裁切、颜色块位置未见明显回归
- 圆、弧、粗环等关键元素肉眼上基本一致
- 当前差异更像是边缘覆盖率 / AA 细节差异，不是用户层面明显可见的渲染错误

因此，基于当前回归用例，只能得出：

- `HQ` 与 `basic` 在像素级并非完全相同
- 但 `HQ` 没有表现出足以支撑“改默认”的明显画质领先

换句话说，画质角度也不足以支持删除 `basic` 或整体 default-HQ。

## 7. 为什么 basic 有时会比 HQ 慢

这是这次回归里最关键、也最容易误判的点。

### 7.1 basic 并不是“简单所以一定更快”

`basic` 的 fill 路径并不是一个“大 span 一次写完”的实现。

`egui_canvas_draw_circle_fill_basic()` 会调用四次 `egui_canvas_draw_circle_corner_fill()`；而 `egui_canvas_draw_circle_corner_fill()` 每一行会做三类事情：

- 根据 LUT 写边缘点
- 写一段 reserve `hline`
- 再写一段 mirror reserve `vline`
- 最后还可能补一个 reserve `fillrect`

同时，`hline/vline` 在实现上并不是专门的快路径，而是直接落到 `egui_canvas_draw_fillrect()`：

- `egui_canvas_draw_hline(x, y, len)` -> `egui_canvas_draw_fillrect(x, y, len, 1)`
- `egui_canvas_draw_vline(x, y, len)` -> `egui_canvas_draw_fillrect(x, y, 1, len)`

这意味着 `basic fill` 的真实成本是：

- LUT 访问
- 边缘点写入
- 大量小矩形调用
- 每次矩形调用都走一次裁剪 / alpha / PFB 路径

当这些“小块写入”的调度成本大于 `HQ` 的数学计算成本时，`basic` 就会输。

### 7.2 HQ 的优势来自“写入聚合”，不只是数学更高级

`egui_canvas_draw_circle_fill_hq()` 与 `egui_canvas_draw_circle_hq()` 的核心特征是：

- 先按 scanline 缩小有效 `x` 范围
- 内部区域直接做连续 span 填充
- 边缘区域才做 coverage/alpha 计算
- 在无 mask 时直接写 `PFB`

也就是说，`HQ` 的数学更重，但它常常能把写入组织成更连续、更贴合 `PFB` 的 span。

当 `PFB` 形态适合这种 scanline 访问时，例如：

- `small 15x15`
- `fullwidth 240x1`

`HQ` 的连续写优势就会压过它的数学成本，所以会出现 `basic` 反而更慢的现象，尤其是 fill 类图元。

`fullwidth 240x1` 下 `CIRCLE_FILL -92.1%`，就是最典型的例子。

### 7.3 为什么有些场景又反过来 basic 更快

`HQ` 并不总能赢，因为它的 scanline 方向与 `PFB` 方向并不总匹配。

典型反例有两个：

#### A. `fullscreen 240x240` 下的轮廓

在全屏或接近全屏的方形 `PFB` 下，`basic` 轮廓路径依靠 LUT 与对称性，能较便宜地完成轮廓绘制；而 `HQ` 轮廓仍要做一整套边缘 coverage 计算。

于是出现：

- 标准 perf：`CIRCLE +38.4%`
- `PFB fullscreen`：`CIRCLE +181.7%`

这说明对轮廓来说，`HQ` 的数学成本在这类场景下没有被更好的写入模式抵消。

#### B. `fullheight 1x240` 下的 fill

`HQ fill` 本质是按行扫描；但 `1x240` 的 `PFB` 是垂直 strip。此时 scanline 写入会不断跨列、反复触发细碎的 `PFB` 访问，局部性很差。

反过来看 `basic fill`：`egui_canvas_draw_circle_corner_fill()` 在 mirror 分支里会写 reserve `vline`，这一部分反而更贴近垂直 strip 的组织方式。

于是会出现强烈反转：

- `fullheight` 下 `CIRCLE_FILL +161.7%`
- `fullheight` 下 `ARC_FILL +12.9%`

这不是 `HQ` 算法本身错了，而是 `PFB` 方向把 `HQ` 的写入优势抵消甚至反杀了。

### 7.4 根因归纳

所以“为什么 basic 有时会比 HQ 慢”的根因不是一句“HQ 更先进”或“basic 更简单”能解释的，而是下面这组 trade-off：

- `basic` 用 LUT 降低了几何计算，但把绘制拆成很多点/线/小矩形
- `HQ` 增加了 coverage 数学，但常常能把写入聚合成更好的 scanline span
- 最终谁更快，取决于当前图元是 `outline` 还是 `fill`
- 也取决于当前 `PFB` 是 `small square`、`fullwidth strip`、`fullheight strip` 还是 `fullscreen`

这也是为什么单看一组 perf 数据会得出错误结论，必须跑真实 matrix 回归。

## 8. 最终建议

### 8.1 是否保留 basic

建议：**保留**。

原因不是它“全面更优”，而是它在以下场景仍有不可替代的价值：

- 某些轮廓场景明显更快
- 某些 `PFB` 方向下 fill 反而更稳
- 某些 minimal / MCU 目标下 default-HQ 会增大镜像

### 8.2 是否改全局默认为 HQ

建议：**不要一刀切改默认**。

因为真实回归已经证明：

- 改默认后 `CIRCLE` 标准 perf 会退化 `38.4%`
- `fullscreen` 下 `CIRCLE` 会退化 `181.7%`
- `fullheight` 下 `CIRCLE_FILL` 会退化 `161.7%`

这类回归已经超出“可接受波动”。

### 8.3 更合理的策略

更合理的方向是：

- 继续保留 `basic` 与 `HQ` 并存
- 默认 API 继续维持当前策略
- 对明确的 fill-heavy 场景或特定 `PFB` 布局，显式选择 `HQ`
- 如果未来要继续优化，优先考虑“按图元类型 / PFB 形态分流”，而不是“删除 basic”

### 8.4 后续实做验证：basic fill 第二轮优化结果

在完成上面的保留性评估后，又继续对 `basic fill` 做了两轮保守优化：

- 第一轮：把 `egui_canvas_draw_circle_fill_basic()` 改成按行 span 聚合写入，先解决 `fullwidth` / `fullscreen` 的碎片化写入问题
- 第二轮：保留 `fullheight` 这类 `1xN` 竖条的 legacy fallback，不再强行让小 tile 走 row-wise；小 tile 分流从固定像素阈值改成按圆直径缩放的 compact clip 判定，再给 `egui_canvas_draw_circle_corner_fill()` 增加单独的 no-mask direct-PFB helper，专门降低 legacy 小 tile 的点 / 线 / 小矩形调用开销

第二轮里还验证了一个错误方向：如果直接去掉 `small` fallback，让 `15x15/30x30` 全部走 row-wise，`CIRCLE_FILL` 会退化到 `small 1.588ms`、`middle 0.814ms`。这说明 `small` 差距的根因不在“fallback 阈值太保守”，而在 legacy 路径本身的调度开销。

最终 `CIRCLE_FILL` 数据如下：

| 场景 | 原始 basic(ms) | 当前优化后 basic(ms) | HQ(ms) | 结论 |
|---|---:|---:|---:|---|
| 标准 perf | `0.788` | `0.587` | `0.487` | 差距从 `+61.8%` 缩小到 `+20.5%` |
| `small 15x15` | `1.337` | `1.231` | `1.238` | `basic` 小幅反超 `HQ` |
| `middle 30x30` | `0.638` | `0.532` | `0.647` | `basic` 明显快于 `HQ` |
| `fullwidth 240x1` | `5.009` | `0.414` | `0.396` | 已非常接近 `HQ`，仅略慢 |
| `fullheight 1x240` | `4.851` | `4.880` | `12.697` | 基本持平原始值，仍显著优于 `HQ` |
| `fullscreen 240x240` | `0.313` | `0.222` | `0.211` | 已非常接近 `HQ`，仅略慢 |

这组结果说明两件事：

- `basic fill` 原先被 `HQ` 压制的主要问题，不是 LUT 几何本身，而是某些路径的写入组织方式不够贴近 `PFB`
- 只要把写入调度方式改对，`basic` 完全可以在保留现有语义的前提下，把优势区间扩展到更多 `PFB` 形态

因此，最新结论比本报告开头更强：

- `basic` 不仅“有保留必要”，而且“有继续优化的明确回报”
- 对 `fill` 场景，继续做 `basic` 的形态分流优化，比“一刀切切到 HQ”更稳、更可控
- 剩余真正还落后 `HQ` 的，只剩 `fullwidth/fullscreen` 这类已经非常接近的尾部差距，优先级已经明显下降

## 9. 半径相关阈值定向 PFB Sweep 补充

在第二轮 `basic fill` 优化完成后，又针对当前 helper

- `egui_canvas_should_use_circle_fill_basic_legacy_clip(radius, clip_width, clip_height)`

补做了一轮更稠密的定向 QEMU perf，用来回答一个更具体的问题：

- 旧的固定 `30x30` 已经被去掉后，“按半径缩放”的 compact-tile 阈值，是否真的会随圆尺寸扩张，并且是否能给出稳定的回归结论。

这轮专项数据见：

- `.claude/2026-04-01-circle-radius-threshold-pfb-perf-plan.md`
- `perf_output/circle_radius_pfb_sweep.json`
- `perf_output/circle_radius_pfb_sweep_report.md`

### 9.1 测试设计

- `CIRCLE_FILL`：标准半径 `119`，当前 helper 算出的 `compact_tile_limit = 30`
- `CIRCLE_FILL_DOUBLE`：双倍半径 `239`，当前 helper 算出的 `compact_tile_limit = 60`
- `CIRCLE_FILL_HQ`：作为标准半径下的代表性参考
- PFB 集合：`8x8`、`12x12`、`15x15`、`24x10`、`30x8`、`30x30`、`40x6`、`48x16`、`60x12`、`80x10`、`240x1`、`1x240`、`240x240`

补充说明：Windows 下长 `APP_OBJ_SUFFIX` 会让 QEMU perf 构建在链接阶段触发 `collect2.exe CreateProcess` 长命令问题，因此专项 sweep 使用了短构建别名；该处理只影响 obj 目录名，不影响业务代码与 benchmark 宏配置。

### 9.2 关键结果

| PFB | `CIRCLE_FILL` | 路径 | `CIRCLE_FILL_DOUBLE` | 路径 | `CIRCLE_FILL_HQ` |
|---|---:|---|---:|---|---:|
| `15x15` | `1.234` | legacy compact | `1.514` | legacy compact | `1.238` |
| `30x30` | `0.532` | legacy compact | `0.595` | legacy compact | `0.647` |
| `40x6` | `0.795` | row-wise | `1.542` | legacy compact | - |
| `48x16` | `0.588` | row-wise | `0.658` | legacy compact | `0.487` |
| `60x12` | `0.517` | row-wise | `0.717` | legacy compact | `0.438` |
| `80x10` | `0.433` | row-wise | `0.430` | row-wise | `0.376` |
| `240x1` | `0.414` | row-wise strip | `0.398` | row-wise strip | `0.396` |
| `1x240` | `4.881` | legacy strip | `4.872` | legacy strip | `12.697` |
| `240x240` | `0.222` | row-wise | `0.206` | row-wise | `0.211` |

### 9.3 补充结论

1. 当前 generic helper 确实把二维 compact-tile 分界从标准半径场景的 `30` 扩展到了双倍半径场景的 `60`，不再依赖固定像素 magic number。
2. strip 语义保持稳定：`240x1` 始终走 row-wise，`1x240` 始终保留 legacy，这一点与之前的设计目标一致。
3. 标准半径下，`30x30` 仍是 compact legacy 的强势点，而更宽的 row-wise tile 会继续进入更优平台，`80x10 = 0.433ms`，`240x240 = 0.222ms`。
4. 双倍半径下，`80x10` 作为第一个明显越过 `60` 阈值的 row-wise case，已经落到 `0.430ms`；但 `40x6 / 48x16 / 60x12` 仍停留在 legacy 区间，分别为 `1.542 / 0.658 / 0.717ms`。这说明“按半径缩放阈值”比固定 `30` 更通用，但对 `40~60` 区间的扁长 tile 仍偏保守。
5. HQ 对照进一步强化了前面的主结论：当前 `basic` 仍有保留必要，但也不应被表述成“所有 PFB 形状下都优于 HQ”。在典型 row-wise tile (`48x16 / 60x12 / 80x10`) 上，HQ 仍快 `15%~21%`；在 `30x30`、`15x15`、`1x240` 上，当前 `basic` 仍有明确价值。

这一轮补充数据把结论收得更严：

- 之前“不要改成全局 default-HQ”的判断仍成立
- 之前“basic 有继续优化价值”的判断也仍成立
- 但如果后续还要继续压尾部差距，下一轮应该只针对 `CIRCLE_FILL_DOUBLE` 的 `40x6 / 48x16 / 60x12` 做强制 row-wise A/B，而不是重跑整套大矩阵

## 10. 本次产出物

- 标准 perf：`.claude/circle_baseline_perf_report.md`、`.claude/circle_default_hq_perf_clean_report.md`
- baseline `PFB matrix`：`.claude/circle_baseline_pfb_matrix_report.md`
- 修复后 default-HQ `PFB matrix`：`.claude/circle_default_hq_pfb_matrix_fixed_report.md`
- 体积与符号：`.claude/circle_size_*`
- 渲染对比：`.claude/circle_runtime_hash_compare.txt`、`.claude/circle_runtime_pixel_compare.txt`
- 半径相关阈值定向 perf：`.claude/2026-04-01-circle-radius-threshold-pfb-perf-plan.md`、`perf_output/circle_radius_pfb_sweep.json`、`perf_output/circle_radius_pfb_sweep_report.md`

## 11. 最终判定

最终判定如下：

- **普通 CIRCLE 算法仍然有存在必要**
- **不建议因为 HQ 已经存在，就删除 basic 或改成默认 HQ**
- **HQ 更适合作为可选的、按场景启用的高质量 / 高聚合写入路径，而不是 basic 的完全替代品**