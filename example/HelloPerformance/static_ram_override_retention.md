# HelloPerformance Static-RAM Override 保留记录

## 范围

- 本文记录当前仍需要跟踪的 static-RAM / 历史 low-RAM 相关项：
  - `EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH`
  - `EGUI_CONFIG_CORE_SEPARATE_USER_ROOT_GROUP_ENABLE`
  - `EGUI_CONFIG_FUNCTION_SUPPORT_ACTIVITY`
  - `EGUI_CONFIG_FUNCTION_SUPPORT_DIALOG`
  - `EGUI_CONFIG_IMAGE_STD_ALPHA_OPAQUE_CACHE_SLOTS`
  - `EGUI_CONFIG_IMAGE_RLE_EXTERNAL_CACHE_WINDOW_SIZE`
- 其中：
  - `EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH` 仍是当前 HelloPerformance app-side active 设置
  - `EGUI_CONFIG_CORE_SEPARATE_USER_ROOT_GROUP_ENABLE` / `EGUI_CONFIG_FUNCTION_SUPPORT_ACTIVITY` / `EGUI_CONFIG_FUNCTION_SUPPORT_DIALOG` 已不再由 `HelloPerformance/app_egui_config.h` 明确定义，但旧文档长期把它们当 active override，需要用当前主线结论纠偏
  - `EGUI_CONFIG_IMAGE_STD_ALPHA_OPAQUE_CACHE_SLOTS` / `EGUI_CONFIG_IMAGE_RLE_EXTERNAL_CACHE_WINDOW_SIZE` 已回到框架默认，只保留历史实验值记录

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

- `2026-04-05` 基于当前主线重新做完整 recheck A/B：
  - off：`EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH=0`（当前 shipped 值）
  - on：`EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH=1`（框架默认）
- 先修正了实验入口：`example/HelloPerformance/app_egui_config.h` 现在对这颗宏改成 `#ifndef / #define 0`，允许 `USER_CFLAGS` 真正覆盖；此前命令行 `-DEGUI_CONFIG_FUNCTION_SUPPORT_TOUCH=1` 会被 app 侧硬编码 `0` 覆盖，导致 on / off 误测成同一配置。
- size
  - `HelloPerformance text 2055380 -> 2057940`
  - `HelloPerformance bss 3832 -> 4120`
  - 即 `0 -> 1` 是 `text +2560B, bss +288B`
- perf
  - 完整 `239` 场景 `ge10=0`、`le10=0`
  - 基础 `RECTANGLE / CIRCLE / ROUND_RECTANGLE / TEXT / IMAGE_565 / EXTERN_IMAGE_565 / IMAGE_RESIZE_565 / EXTERN_IMAGE_RESIZE_565` 全部 `+0.0%`
  - 旧口径里的 `ANIMATION_SCALE +16.56%` 当前已不再复现；`ANIMATION_SCALE / ANIMATION_ALPHA / ANIMATION_TRANSLATE` 都是 `+0.0%`
- runtime
  - `HelloPerformance` on / off 都是 `241 frames`
  - 全量 `241` 帧：`hash mismatch 0`、`pixel mismatch 0`
  - sample hash 一致：
    - `frame_0000.png A4337E1408AB6C17F77FC5F1D5BB6815E8EEAB83B934854E1EEBDAF37340D7A5`
    - `frame_0120.png 7CBEC8DCC7375B8F37C13019A017FC460CFC276A9764993516D0AC5EEF94F569`
    - `frame_0240.png 619EA7069DAC066ABA1A27113EAC475C9B13391BCCD078D03CBFBB113F576EA2`
- unit
  - `HelloUnitTest` `touch=1` 为 `688/688 passed`
  - `HelloUnitTest` `touch=0` 当前编译失败，第一处失败在 `example/HelloUnitTest/test/test_combobox.c`：touch 专项用例直接依赖 `egui_touch_driver_t` / `egui_touch_driver_register()`，未按 `EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH` 做宏保护
- 处理结论：
  - `HelloPerformance` 继续保持 `EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH=0`
  - 当前主线下恢复默认 `1` 已不再带来任何 perf / runtime 收益，但会稳定引入 `text +2560B, bss +288B`
  - 对这个定时驱动、非交互的 benchmark app 来说，touch 管线仍然是纯成本，不适合回收到默认
  - `touch=0` 在 `HelloUnitTest` 上的编译失败应记录为当前 test harness 限制，不改变 HelloPerformance 侧保留 `0` 的结论

### `EGUI_CONFIG_CORE_SEPARATE_USER_ROOT_GROUP_ENABLE`

- 代码搜索确认：`example/HelloPerformance/app_egui_config.h` 当前已不再显式定义这颗宏，HelloPerformance shipped 配置直接继承框架当前默认 `0`。
- `2026-04-05` 基于当前主线重新做完整 recheck A/B：
  - off：`EGUI_CONFIG_CORE_SEPARATE_USER_ROOT_GROUP_ENABLE=0`（当前 shipped / default）
  - on：`EGUI_CONFIG_CORE_SEPARATE_USER_ROOT_GROUP_ENABLE=1`
- size
  - `HelloPerformance text 2055380 -> 2055416`
  - `HelloPerformance bss 3832 -> 3832`
  - 即 `0 -> 1` 仅 `text +36B`，`bss` 不变
- perf
  - 完整 `239` 场景：`ge10=1`、`le10=0`
  - 旧口径里的 `MASK_RECT_FILL_NO_MASK_QUARTER +19.92%` 当前已缩小到 `+9.8%`
  - 基础主路径存在普遍性小幅抬升，但都低于 `10%`
    - `RECTANGLE +6.1%`
    - `IMAGE_565 +4.6%`
    - `EXTERN_IMAGE_565 +2.6%`
    - `TEXT +1.1%`
  - 当前唯一 `>=10%` 回退场景是 `IMAGE_565_8_DOUBLE +12.6%`
- runtime / unit
  - `HelloPerformance` on / off 都是 `241 frames`，全量 `hash mismatch 0 / pixel mismatch 0`
  - `HelloUnitTest` on / off 都是 `688/688 passed`
- 处理结论：
  - 这颗宏已经不应再被记录为 HelloPerformance 的当前 active static-RAM override
  - 当前主线下它不再改变 `bss`，只带来 `text +36B` 和少量普遍性 perf 抬升
  - 若后续还要评估 `user_root=1`，语义应是“行为实验 / feature path recheck”，而不是 small static-RAM cleanup 项

### `EGUI_CONFIG_FUNCTION_SUPPORT_ACTIVITY` / `EGUI_CONFIG_FUNCTION_SUPPORT_DIALOG`

- 代码搜索确认：`example/HelloPerformance/app_egui_config.h` 当前也不再显式定义这两颗宏，HelloPerformance shipped 配置直接继承“未定义即 `0`”的当前框架行为。
- 这两项也不是独立开关：
  - `dialog=1` 会强制 `activity=1`
  - `activity=1` / `dialog=1` 都会自动把 `EGUI_CONFIG_CORE_SEPARATE_USER_ROOT_GROUP_ENABLE` 置为 `1`
- 处理结论：
  - 这两颗宏同样应从“当前 active static-RAM override”清单里移除
  - 后续如果要重新评估，语义应是“功能开启实验”，不是 small static-RAM cleanup 项

### `EGUI_CONFIG_IMAGE_STD_ALPHA_OPAQUE_CACHE_SLOTS`

- `../../doc/source/performance/low_ram_config_macros.md` 对当前量级给出的估算是：
  - 默认 `4`
  - HelloPerformance 当前默认 `4`（历史实验值 `0`）
  - 约 `~33B` BSS
- `2026-04-05` 基于当前主线重新做完整 recheck A/B：
  - on：`EGUI_CONFIG_IMAGE_STD_ALPHA_OPAQUE_CACHE_SLOTS=4`（当前默认）
  - off：`EGUI_CONFIG_IMAGE_STD_ALPHA_OPAQUE_CACHE_SLOTS=0`
- size
  - `HelloPerformance text 2055380 -> 2055224`
  - `HelloPerformance bss 3832 -> 3792`
  - 即关闭后回收 `text -156B, bss -40B`
- perf
  - 完整 `239` 场景里没有任何 `>=10%` 回退
  - 只有 `EXTERN_IMAGE_RESIZE_TILED_565_8 -11.4%` 这一处 `>=10%` 改善
  - 基础 `RECTANGLE / CIRCLE / ROUND_RECTANGLE / TEXT / IMAGE_565 / EXTERN_IMAGE_565 / IMAGE_RESIZE_565 / EXTERN_IMAGE_RESIZE_565` 都维持在噪声内
- runtime / unit
  - `HelloPerformance` on / off 都是 `241 frames`
  - 但 `4 vs 0` 全量对比仍有 `8` 帧真实像素差：`frame_0066~0069`、`frame_0095~0098`
  - 抽查 sample hash 仍一致：
    - `frame_0000.png A4337E1408AB6C17F77FC5F1D5BB6815E8EEAB83B934854E1EEBDAF37340D7A5`
    - `frame_0120.png 7CBEC8DCC7375B8F37C13019A017FC460CFC276A9764993516D0AC5EEF94F569`
    - `frame_0240.png 619EA7069DAC066ABA1A27113EAC475C9B13391BCCD078D03CBFBB113F576EA2`
  - 为排除录制噪声，额外补的 `4 vs 4` repeat 对比仍是 `hash mismatch 0`、`pixel mismatch 0`
  - `HelloUnitTest` on / off 都是 `688/688 passed`
- 处理结论：
  - 这颗宏当前不应再被写成 `HelloPerformance=0`
  - 虽然关闭后当前 perf 没有变慢，且还能回收 `text -156B, bss -40B`
  - 但它会带来 `8` 帧真实 render diff，因此当前不能把 `0` 视为安全等价的 low-RAM 档位
  - 结论是保持当前默认 `4`，历史实验值 `0` 仅保留为后续 render-diff 排查入口

### `EGUI_CONFIG_IMAGE_RLE_EXTERNAL_CACHE_WINDOW_SIZE`

- `docs/low_ram_config_macros.md` 对当前量级给出的估算是：
  - 默认 `1024`
  - HelloPerformance 当前默认 `1024`（历史实验值 `64`）
  - 约 `~960B` 窗口 BSS
- `2026-04-05` 基于当前主线重新做完整 recheck A/B：
  - on：`EGUI_CONFIG_IMAGE_RLE_EXTERNAL_CACHE_WINDOW_SIZE=1024`（当前默认）
  - off：`EGUI_CONFIG_IMAGE_RLE_EXTERNAL_CACHE_WINDOW_SIZE=64`
- size
  - `HelloPerformance text 2055380 -> 2055476`
  - `HelloPerformance bss 3832 -> 2808`
  - 即 `1024 -> 64` 后是 `text +96B, bss -1024B`
- perf
  - 完整 `239` 场景里有 `5` 个 `>=10%` 回退
  - 基础 `RECTANGLE / CIRCLE / ROUND_RECTANGLE / TEXT / IMAGE_565 / EXTERN_IMAGE_565 / IMAGE_RESIZE_565 / EXTERN_IMAGE_RESIZE_565` 都是 `+0.0%`
  - 回退全部集中在 external RLE 主路径及其 masked 变体：
    - `EXTERN_IMAGE_RLE_565 +21.8%`
    - `EXTERN_MASK_IMAGE_RLE_NO_MASK +21.8%`
    - `EXTERN_MASK_IMAGE_RLE_ROUND_RECT +20.8%`
    - `EXTERN_MASK_IMAGE_RLE_CIRCLE +20.3%`
    - `EXTERN_MASK_IMAGE_RLE_IMAGE +17.5%`
  - `EXTERN_IMAGE_RLE_565_8 / EXTERN_MASK_IMAGE_RLE_8_*` 也有 `+8.1% ~ +9.4%` 的稳定回退
- runtime / unit
  - `HelloPerformance` on / off 都是 `241 frames`
  - 全量 `241` 帧 `hash mismatch 0`、`pixel mismatch 0`
  - sample hash 仍一致：
    - `frame_0000.png A4337E1408AB6C17F77FC5F1D5BB6815E8EEAB83B934854E1EEBDAF37340D7A5`
    - `frame_0120.png 7CBEC8DCC7375B8F37C13019A017FC460CFC276A9764993516D0AC5EEF94F569`
    - `frame_0240.png 619EA7069DAC066ABA1A27113EAC475C9B13391BCCD078D03CBFBB113F576EA2`
  - `HelloUnitTest` on / off 都是 `688/688 passed`
- 处理结论：
  - 这颗宏当前不应再被写成 `HelloPerformance=64`
  - `64` 仍然是一个有效的 low-RAM 条件实验值，因为它能稳定回收 `bss -1024B`
  - 但它不再适合作为当前 shipped 默认：external RLE 主路径会出现 `+17.5% ~ +21.8%` 的明显回退
  - 结论是保持当前默认 `1024`，`64` 只在项目明确不关心 external RLE 热点时再考虑启用

## 最终结论

- 这组项已经全部有结论：
- `touch` 仍是当前唯一 active static-RAM override，且已被完整 `size / perf / runtime / unit` 重新确认需要继续保留
- `user_root / activity / dialog` 已从当前 active override 清单移出；若后续继续评估，语义应是行为/功能实验，而不是 small static-RAM cleanup
  - `alpha opaque slots` 已回到默认 `4`，`2026-04-05` recheck 继续确认历史实验值 `0` 会带来 `8` 帧真实 render diff，而 `4 vs 4 repeat` 控制组全量等价
  - `RLE external cache window` 已回到默认 `1024`，`2026-04-05` recheck 继续确认历史实验值 `64` 虽能回收 `bss -1024B`，但 external RLE 主路径仍会回退 `+17.5% ~ +21.8%`
