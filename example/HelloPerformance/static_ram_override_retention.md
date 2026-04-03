# HelloPerformance Static-RAM Override 保留记录

## 范围

- 本文记录当前仍保留、但已经明确不能回收的 static-RAM 相关项：
  - `EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH`
  - `EGUI_CONFIG_CORE_SEPARATE_USER_ROOT_GROUP_ENABLE`
  - `EGUI_CONFIG_FUNCTION_SUPPORT_ACTIVITY`
  - `EGUI_CONFIG_FUNCTION_SUPPORT_DIALOG`
  - `EGUI_CONFIG_IMAGE_STD_ALPHA_OPAQUE_CACHE_SLOTS`
  - `EGUI_CONFIG_IMAGE_RLE_EXTERNAL_CACHE_WINDOW_SIZE`

## 依据来源

- `example/HelloPerformance/ram_tracking.md`
- `../../doc/source/performance/low_ram_config_macros.md`
- 已保留在本地 `.claude/` 的历史 QEMU perf A/B 结果：
  - `perf_macro_function_support_touch_default_on_results.json`
  - `perf_macro_core_separate_user_root_default_on_results.json`
  - `perf_macro_alpha_opaque_slots_on_results.json`
  - `perf_macro_rle_external_cache_window_default_1024_results.json`

## 宏结论

### `EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH`

- 已在 `2026-03-31` 基于 `96dae0b` 基线复核。
- 结果：
  - `static RAM 2672B -> 3000B`，即 `+328B`
  - 最坏 perf：`ANIMATION_SCALE 0.320 -> 0.373 ms (+16.56%)`
- 处理结论：拒绝回收，继续保持 `0`。

### `EGUI_CONFIG_CORE_SEPARATE_USER_ROOT_GROUP_ENABLE`

- 当前 `0` 会让 HelloPerformance 直接复用主 root group，不再额外保留 user-root wrapper。
- 恢复默认 `1` 后：
  - static RAM 约 `+56B`
  - 最坏 perf：`MASK_RECT_FILL_NO_MASK_QUARTER 0.241 -> 0.289 ms (+19.92%)`
- 同时 `user_root=1` 还会把 `activity/dialog` 依赖链重新带回来。
- 处理结论：拒绝回收，继续保持 `0`。

### `EGUI_CONFIG_FUNCTION_SUPPORT_ACTIVITY` / `EGUI_CONFIG_FUNCTION_SUPPORT_DIALOG`

- 这两个项当前不是独立开关：
  - `dialog=1` 会强制 `activity=1`
  - 当前 HelloPerformance 若恢复 `activity=1`，又需要先接受 `user_root=1`
- 但 `user_root=1` 已被上一项 perf 结论拒绝。
- 处理结论：当前路径下无法独立回收，继续保持 `0 / 0`。

### `EGUI_CONFIG_IMAGE_STD_ALPHA_OPAQUE_CACHE_SLOTS`

- `../../doc/source/performance/low_ram_config_macros.md` 对当前量级给出的估算是：
  - 默认 `4`
  - HelloPerformance `0`
  - 约 `~33B` BSS
- 虽然字节量不大，但恢复默认后 active image path 会明显变慢。
- 基于 `c995d40` 基线的本地 QEMU A/B，最坏 perf 为：
  - `EXTERN_IMAGE_RESIZE_TILED_565_8 2.750 -> 3.239 ms (+17.78%)`
- 处理结论：拒绝回收，继续保持 `0`。

### `EGUI_CONFIG_IMAGE_RLE_EXTERNAL_CACHE_WINDOW_SIZE`

- 当前 `64` 不是 small static-RAM 宏。
- 恢复默认 `1024` 时：
  - static RAM `+1016B`
  - perf 变化基本在噪声内，最坏测得仅 `+0.11%`
- 这里真正的问题不是 perf，而是已经远远超过本轮 `<100B static RAM` 门槛。
- 处理结论：继续保持 `64`。

## 最终结论

- 这组项已经全部有结论：
  - `touch`、`user_root`、`alpha opaque slots` 都被 perf 明确拒绝回收
  - `activity/dialog` 被 rejected `user_root=1` 依赖链阻塞
  - `RLE external cache window` 虽 perf 无问题，但 SRAM 增量 `+1016B`，不属于本轮 small-macro cleanup
