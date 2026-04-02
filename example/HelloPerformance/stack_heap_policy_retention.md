# HelloPerformance Stack/Heap Policy 记录

## 范围

- 本文记录 HelloPerformance 的 stack/transient-heap policy 宏：
  - `EGUI_CONFIG_SHADOW_DSQ_LUT_MAX`
  - `EGUI_CONFIG_TEXT_TRANSFORM_LAYOUT_PIXEL_INDEX_16BIT`
  - `EGUI_CONFIG_TEXT_TRANSFORM_LAYOUT_LINE_INDEX_16BIT`
  - `EGUI_CONFIG_TEXT_TRANSFORM_SCRATCH_HEAP_ENABLE`
- 这些项控制的是 stack 上限或 transient heap 形态，不是 `<100B static RAM` cleanup 候选。
- `2026-03-31` 起，这组宏已移动到 `example/HelloPerformance/app_egui_config.h` 尾部的 `#if 0` 可选 low-RAM 块，默认关闭。

## 依据来源

- `example/HelloPerformance/ram_tracking.md`
- `docs/low_ram_config_macros.md`
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

### `EGUI_CONFIG_TEXT_TRANSFORM_LAYOUT_LINE_INDEX_16BIT`

- `docs/low_ram_config_macros.md` 已说明这两项都会把 text-transform layout 数组元素从 32-bit 压到 16-bit。
- 它们影响的是 transient heap：
  - 对短 benchmark 字符串，按字形数 `N` 约节省 `4N B`
  - 每帧分配、每帧释放，不是 fixed static RAM
- HelloPerformance 当前字体大小、字符串长度和位图偏移都在 16-bit 安全范围内。
- 处理结论：
  - `1 / 1` 继续保留在尾部可选 low-RAM 块中，作为压缩 transient heap 的取值
  - 默认关闭该块后，当前构建回到框架默认 `0 / 0`

### `EGUI_CONFIG_TEXT_TRANSFORM_SCRATCH_HEAP_ENABLE`

- 这项是 rotated-text scratch 的放置策略开关。
- 历史 low-RAM 路径用 `1` 把 layout/tile scratch 放到 transient heap，避免固定 stack 数组。
- 处理结论：
  - `1` 继续保留在尾部可选 low-RAM 块中
  - 默认关闭该块后，当前构建回到框架默认 `0`
  - `2026-03-31` 默认关闭后的 clean perf 中，`TEXT_ROTATE_BUFFERED` / `EXTERN_TEXT_ROTATE_BUFFERED` 分别为 `6.411ms` / `6.792ms`

## 最终结论

- `EGUI_CONFIG_TEXT_TRANSFORM_LAYOUT_PIXEL_INDEX_16BIT`
- `EGUI_CONFIG_TEXT_TRANSFORM_LAYOUT_LINE_INDEX_16BIT`
- `EGUI_CONFIG_TEXT_TRANSFORM_SCRATCH_HEAP_ENABLE`

以上 4 项都应归类为 HelloPerformance 的 stack/transient-heap policy 宏，不纳入本轮 small static-RAM cleanup。
当前代码已经把它们下沉到 `app_egui_config.h` 尾部的可选 low-RAM 块，默认构建直接继承框架 stack/transient-heap 默认值；需要 tighter low-RAM profile 时，再按需打开这组 override。
