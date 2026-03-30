# HelloPerformance Stack/Heap Policy 保留记录

## 范围

- 本文记录当前仍保留的 stack/transient-heap policy 宏：
  - `EGUI_CONFIG_SHADOW_DSQ_LUT_MAX`
  - `EGUI_CONFIG_TEXT_TRANSFORM_LAYOUT_PIXEL_INDEX_16BIT`
  - `EGUI_CONFIG_TEXT_TRANSFORM_LAYOUT_LINE_INDEX_16BIT`
  - `EGUI_CONFIG_TEXT_TRANSFORM_SCRATCH_HEAP_ENABLE`
- 这些项控制的是 stack 上限或 transient heap 形态，不是 `<100B static RAM` cleanup 候选。

## 依据来源

- `example/HelloPerformance/ram_tracking.md`
- `docs/low_ram_config_macros.md`

## 宏结论

### `EGUI_CONFIG_SHADOW_DSQ_LUT_MAX`

- 这项是 shadow corner path 的 stack/LUT 上限。
- 历史 A/B 已验证：
  - `128 -> 96` 时，`SHADOW_ROUND 6.113 -> 5.584 (-8.7%)`
  - `96 -> 64` 时，`SHADOW_ROUND 5.584 -> 5.584`
  - 对应 `egui_shadow_draw_corner` 栈帧 `792 -> 728 -> 664`
- 它不是 fixed static RAM 宏，而是当前 shadow 栈预算的 policy 点。
- 处理结论：保持 `64`。

### `EGUI_CONFIG_TEXT_TRANSFORM_LAYOUT_PIXEL_INDEX_16BIT`

### `EGUI_CONFIG_TEXT_TRANSFORM_LAYOUT_LINE_INDEX_16BIT`

- `docs/low_ram_config_macros.md` 已说明这两项都会把 text-transform layout 数组元素从 32-bit 压到 16-bit。
- 它们影响的是 transient heap：
  - 对短 benchmark 字符串，按字形数 `N` 约节省 `4N B`
  - 每帧分配、每帧释放，不是 fixed static RAM
- HelloPerformance 当前字体大小、字符串长度和位图偏移都在 16-bit 安全范围内。
- 处理结论：保持 `1 / 1`。

### `EGUI_CONFIG_TEXT_TRANSFORM_SCRATCH_HEAP_ENABLE`

- 这项是 rotated-text scratch 的放置策略开关。
- 当前 shipped 路径要求 layout/tile scratch 跟随实际 glyph/line 数，走 transient heap，而不是固定 stack 数组。
- `ram_tracking.md` 已明确当前状态：
  - active rotated-text layout/tile scratch 现在按实际 glyph/line 计数走 transient heap
  - `egui_canvas_draw_text_transform` 栈帧已从早期大栈热点降到后续可接受范围
- 因此这项属于 stack/heap placement policy，不是 small static-RAM 回收项。
- 处理结论：保持 `1`。

## 最终结论

- `EGUI_CONFIG_SHADOW_DSQ_LUT_MAX`
- `EGUI_CONFIG_TEXT_TRANSFORM_LAYOUT_PIXEL_INDEX_16BIT`
- `EGUI_CONFIG_TEXT_TRANSFORM_LAYOUT_LINE_INDEX_16BIT`
- `EGUI_CONFIG_TEXT_TRANSFORM_SCRATCH_HEAP_ENABLE`

以上 4 项都应归类为 HelloPerformance 的 stack/transient-heap policy 宏，不纳入本轮 small static-RAM cleanup。
