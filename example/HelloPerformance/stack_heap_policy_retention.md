# HelloPerformance Stack/Heap Policy 记录

## 范围

- 本文记录 HelloPerformance 的 stack/transient-heap policy 宏：
  - `EGUI_CONFIG_SHADOW_DSQ_LUT_MAX`
  - `EGUI_CONFIG_TEXT_TRANSFORM_LAYOUT_PIXEL_INDEX_16BIT`
  - `EGUI_CONFIG_TEXT_TRANSFORM_LAYOUT_LINE_INDEX_16BIT`
  - `EGUI_CONFIG_TEXT_TRANSFORM_SCRATCH_HEAP_ENABLE`
  - `EGUI_CONFIG_TEXT_TRANSFORM_VISIBLE_ALPHA8_MAX_BYTES`
- 这些项控制的是 stack 上限或 transient heap 形态，不是 `<100B static RAM` cleanup 候选。
- `2026-03-31` 起，这组宏已移动到 `example/HelloPerformance/app_egui_config.h` 尾部的 `#if 0` 可选 low-RAM 块，默认关闭。

## 依据来源

- `example/HelloPerformance/ram_tracking.md`
- `../../doc/source/performance/low_ram_config_macros.md`
- `perf_output/perf_report.md`

## 宏结论

### `EGUI_CONFIG_SHADOW_DSQ_LUT_MAX`

- 这项是 shadow corner path 的 stack/LUT 上限。
- 历史 A/B 已验证：
  - `128 -> 96` 时，`SHADOW_ROUND 6.113 -> 5.584 (-8.7%)`
  - `96 -> 64` 时，`SHADOW_ROUND 5.584 -> 5.584`
  - 对应 `egui_shadow_draw_corner` 栈帧 `792 -> 728 -> 664`
- `2026-03-31` 又补做了一次“当时的框架默认 `256` vs 强制 `64`”的定向 HelloPerformance QEMU perf A/B。
  - 测试方法：沿用 `scripts/perf_analysis/code_perf_check.py` 的同一套 `cortex-m3` QEMU perf harness，只额外用 `USER_CFLAGS` 覆盖 `-DEGUI_CONFIG_SHADOW_DSQ_LUT_MAX=64`
  - 两组构建都保持 `222` 个 case、`24` 个 skip
  - `SHADOW 4.039 -> 4.039 (0.00%)`
  - `SHADOW_ROUND 6.306 -> 4.949 (-21.52%)`
  - 其它锚点基本不动：`TEXT_ROTATE_BUFFERED 6.411 -> 6.411`、`EXTERN_TEXT_ROTATE_BUFFERED 6.792 -> 6.792`、`ANIMATION_SCALE 0.321 -> 0.320`
- 这次定向 A/B 说明：之前默认关闭 low-RAM tail block 之后，`SHADOW_ROUND` 变慢的根因就是这里回到了框架默认 `256`，不是 text-transform 那几项影响了 shadow。
- 原因分析：
  - `SHADOW_ROUND` 场景在 `example/HelloPerformance/egui_view_test_performance.c` 里固定使用 `width=20`、`corner_radius=30`
  - round shadow 会走 `src/shadow/egui_shadow.c` 里的 `egui_shadow_draw_corner()`，并且每个角都会先构建一张 `d_sq -> alpha` LUT
  - 对当前场景，`64` 上限时只需要 `51` 个 LUT bucket；回到默认 `256` 后会扩成 `201` 个 bucket
  - 这会让每个角额外做约 `150` 次 `egui_shadow_isqrt()` + LUT 填充，四个角合计多约 `600` 次预计算，所以 `SHADOW_ROUND` 会明显变慢
  - 同一轮 `-fstack-usage` 复测也印证了这一点：`egui_shadow_draw_corner` 当前栈帧在默认 `256` 时是 `368B`，强制 `64` 时是 `176B`
- 它不是 fixed static RAM 宏，而是 shadow corner path 的 stack/LUT policy 点。
- 处理结论：
  - `64` 已直接上收为框架默认值，不再作为 `HelloPerformance` 的 app-side override 保留
  - `2026-03-31` 起，`64` 已直接上收为框架默认值，`HelloPerformance` 不再重复定义这个宏
  - 当前可选 `#if 0` low-RAM tail block 只保留 text-transform 相关 tighter transient-heap 取值
  - 以 `64` 这个当前默认值重新看 clean perf，`SHADOW_ROUND` 为 `4.949ms`

### `EGUI_CONFIG_TEXT_TRANSFORM_LAYOUT_PIXEL_INDEX_16BIT`

- 这项会把 text-transform layout 里的像素偏移数组从 32-bit 压到 16-bit。
- 当前 shipped/default 关系已经重新确认：
  - 框架默认 `0`
  - `HelloPerformance` 当前 active 配置也是 `0`
  - `1` 只保留在 `app_egui_config.h` 尾部 `#if 0` 的 low-RAM 可选块里
- `2026-04-04` 基于当前主线补做完整 A/B 后：
  - size：`1` 相比 `0` 仅 `text -4B`，`bss` 不变
  - perf：完整 `239` 场景无任何 `>=10%` 回退或改善，最大绝对波动约 `0.75%`
  - `TEXT_ROTATE_GRADIENT +0.039%`、`EXTERN_TEXT_ROTATE +0.038%`
  - 基础 `RECTANGLE / CIRCLE / ROUND_RECTANGLE / TEXT / IMAGE_565 / EXTERN_IMAGE_565 / IMAGE_RESIZE_565 / EXTERN_IMAGE_RESIZE_565` 都在噪声内
  - runtime：on / off 都是 `241 frames`，`hash mismatch 0`、`pixel mismatch 0`
  - unit：on / off 都是 `688/688 passed`
- 处理结论：
  - 这颗宏不再适合写成 `HelloPerformance` 当前默认 `1`
  - 当前默认继续保持 `0`
  - `1` 继续只作为可选 low-RAM transient-heap 实验值保留

### `EGUI_CONFIG_TEXT_TRANSFORM_LAYOUT_LINE_INDEX_16BIT`

- `../../doc/source/performance/low_ram_config_macros.md` 已说明这两项都会把 text-transform layout 数组元素从 32-bit 压到 16-bit。
- 它们影响的是 transient heap：
  - 对短 benchmark 字符串，按字形数 `N` 约节省 `4N B`
  - 每帧分配、每帧释放，不是 fixed static RAM
- HelloPerformance 当前字体大小、字符串长度和位图偏移都在 16-bit 安全范围内。
- `2026-04-05` 基于当前主线补做完整 A/B 后：
  - size：`1` 相比 `0` 反而 `text +100B`，`bss` 不变
  - perf：完整 `239` 场景无任何 `>=10%` 回退或改善，最大绝对波动约 `1.71%`
  - `TEXT_ROTATE -0.041%`、`TEXT_ROTATE_GRADIENT -0.039%`、`EXTERN_TEXT_ROTATE +0.0%`
  - 基础 `RECTANGLE / CIRCLE / ROUND_RECTANGLE / TEXT / IMAGE_565 / EXTERN_IMAGE_565 / IMAGE_RESIZE_565 / EXTERN_IMAGE_RESIZE_565` 都是 `+0.0%`
  - runtime：on / off 都是 `241 frames`，`hash mismatch 0`、`pixel mismatch 0`
  - unit：on / off 都是 `688/688 passed`
- 处理结论：
  - 当前 shipped/default 关系也已经确认回到框架默认 `0`
  - 这颗宏当前也不再适合写成 `HelloPerformance` 默认 `1`
  - `1 / 1` 继续保留在尾部可选 low-RAM 块中，作为压缩 transient heap 的取值
  - 默认关闭该块后，当前构建回到框架默认 `0 / 0`

### `EGUI_CONFIG_TEXT_TRANSFORM_SCRATCH_HEAP_ENABLE`

- 这项是 rotated-text scratch 的放置策略开关。
- 历史 low-RAM 路径用 `1` 把 layout/tile scratch 放到 transient heap，避免固定 stack 数组。
- 当前 shipped/default 关系已经重新确认：
  - 框架默认 `0`
  - `HelloPerformance` 当前 active 配置也是 `0`
  - `1` 只保留在 `app_egui_config.h` 尾部 `#if 0` 的 low-RAM 可选块里
- `2026-04-05` 基于当前主线补做完整 A/B 后：
  - size：`1` 相比 `0` 回收 `text -264B, bss -8B`
  - perf：完整 `239` 场景里有 `6` 个 `>=10%` 回退，全部集中在 rotated-text 主路径
  - `EXTERN_TEXT_ROTATE +52.4%`
  - `TEXT_ROTATE_RESIZE +39.6%`
  - `TEXT_ROTATE +39.5%`
  - `TEXT_ROTATE_GRADIENT +37.5%`
  - `TEXT_ROTATE_QUARTER +36.6%`
  - `TEXT_ROTATE_DOUBLE +33.7%`
  - 基础 `RECTANGLE / CIRCLE / ROUND_RECTANGLE / TEXT / IMAGE_565 / EXTERN_IMAGE_565 / IMAGE_RESIZE_565 / EXTERN_IMAGE_RESIZE_565` 都在噪声内
  - runtime：on / off 都是 `241 frames`，`hash mismatch 0`、`pixel mismatch 0`
  - unit：on / off 都是 `688/688 passed`
- 处理结论：
  - 默认关闭该块后，当前构建回到框架默认 `0`
  - `1` 继续保留在尾部可选 low-RAM 块中
  - 但它当前更适合作为“栈/瞬时堆交换”的条件实验值，不适合作为 `HelloPerformance` 默认配置

### `EGUI_CONFIG_TEXT_TRANSFORM_VISIBLE_ALPHA8_MAX_BYTES`

- 这项是 rotated-text visible alpha8 fast-path 的 transient heap 上限。
- 当前 shipped/default 关系已经重新确认：
  - 框架默认 `4096`
  - `HelloPerformance` 当前 active 配置也是 `4096`
  - `2560` 只保留在 `app_egui_config.h` 尾部 `#if 0` 的 low-RAM 可选块里
- `2026-04-05` 基于当前主线补做完整 A/B 后：
  - size：`2560` 相比 `4096`，`text / data / bss` 完全相同
  - perf：完整 `239` 场景无任何 `>=10%` 回退或改善，最大绝对波动约 `5.62%`
  - `TEXT_ROTATE -2.936%`、`TEXT_ROTATE_RESIZE -2.936%`
  - `TEXT_ROTATE_GRADIENT -3.096%`、`TEXT_ROTATE_DOUBLE -2.149%`
  - `EXTERN_TEXT_ROTATE -5.623%`、`TEXT_ROTATE_QUARTER +0.0%`
  - 基础 `RECTANGLE / CIRCLE / ROUND_RECTANGLE / TEXT / IMAGE_565 / EXTERN_IMAGE_565 / IMAGE_RESIZE_565 / EXTERN_IMAGE_RESIZE_565` 都在噪声内
  - runtime：`2560 / 4096` 都是 `241 frames`，`hash mismatch 0`、`pixel mismatch 0`
  - unit：`2560 / 4096` 都是 `688/688 passed`
- 它不是 fixed static RAM 宏，而是 rotated-text visible alpha8 fast-path 的 transient heap cap。
- 处理结论：
  - 当前 shipped/default 继续保持 `4096`
  - `2560` 继续保留在尾部可选 low-RAM 块中
  - 但它当前已经是很适合优先考虑的 tighter low-RAM 数值宏：size 不退化、perf 无回退，且直接收紧这条 text-transform transient-heap 上限

## 最终结论

- `EGUI_CONFIG_SHADOW_DSQ_LUT_MAX`
- `EGUI_CONFIG_TEXT_TRANSFORM_LAYOUT_PIXEL_INDEX_16BIT`
- `EGUI_CONFIG_TEXT_TRANSFORM_LAYOUT_LINE_INDEX_16BIT`
- `EGUI_CONFIG_TEXT_TRANSFORM_SCRATCH_HEAP_ENABLE`
- `EGUI_CONFIG_TEXT_TRANSFORM_VISIBLE_ALPHA8_MAX_BYTES`

以上 5 项都应归类为 HelloPerformance 的 stack/transient-heap policy 宏，不纳入本轮 small static-RAM cleanup。
其中 `EGUI_CONFIG_SHADOW_DSQ_LUT_MAX` 已经上收为框架默认 `64`；其余 4 项 text-transform 宏则继续下沉到 `app_egui_config.h` 尾部的可选 low-RAM 块，默认构建直接继承框架 stack/transient-heap 默认值；需要 tighter low-RAM profile 时，再按需打开这组 override。
