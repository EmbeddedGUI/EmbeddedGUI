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
- 它不是 fixed static RAM 宏，而是 shadow corner path 的 stack/LUT policy 点。
- 处理结论：
  - `64` 继续保留在尾部可选 low-RAM 块中，作为 tighter stack profile 取值
  - 默认关闭该块后，当前构建回到框架默认 `256`
  - `2026-03-31` 默认关闭后的 clean perf 中，`SHADOW_ROUND` 为 `6.306ms`

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

- `EGUI_CONFIG_SHADOW_DSQ_LUT_MAX`
- `EGUI_CONFIG_TEXT_TRANSFORM_LAYOUT_PIXEL_INDEX_16BIT`
- `EGUI_CONFIG_TEXT_TRANSFORM_LAYOUT_LINE_INDEX_16BIT`
- `EGUI_CONFIG_TEXT_TRANSFORM_SCRATCH_HEAP_ENABLE`

以上 4 项都应归类为 HelloPerformance 的 stack/transient-heap policy 宏，不纳入本轮 small static-RAM cleanup。
当前代码已经把它们下沉到 `app_egui_config.h` 尾部的可选 low-RAM 块，默认构建直接继承框架 stack/transient-heap 默认值；需要 tighter low-RAM profile 时，再按需打开这组 override。
