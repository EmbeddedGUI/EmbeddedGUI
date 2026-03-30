# HelloPerformance 宏清理汇总

## 目标与规则

- 目标：梳理 `HelloPerformance` 当前 app 侧宏，降低为 SRAM 小收益而引入的维护成本。
- 规则：
  - 只看 app 侧 override
  - 只处理 `static RAM <100B` 的项
  - 需要 `HelloPerformance` QEMU perf 结果支撑
  - 最大性能回退必须 `<5%`
  - 每项独立处理、独立提交

## 当前基线

- 当前代码侧最后一笔有效清理提交：`96dae0b`
- 干净重编 `llvm-size -A output/main.elf`：
  - `.data = 72B`
  - `.bss = 2164B`
  - `._user_heap_stack = 436B`
  - `static RAM = 2672B`

## 已完成回收项

| 项目 | 结果 | 提交 |
| --- | --- | --- |
| `EGUI_CONFIG_IMAGE_QOI_ROW_INDEX_8BIT_ENABLE` | 回退到默认 | `052de02` |
| `EGUI_CONFIG_FONT_STD_CODE_LOOKUP_CACHE_ASCII_COMPACT` | 回退到默认 | `8ee6a96` |
| `EGUI_CONFIG_IMAGE_RLE_CHECKPOINT_ENABLE` | 回退到默认 | `a4c55a5` |
| `EGUI_CONFIG_IMAGE_STD_EXTERNAL_ROW_PERSISTENT_CACHE_ENABLE` | 回退到默认 | `d00e173` |
| `EGUI_CONFIG_IMAGE_RLE_EXTERNAL_WINDOW_PERSISTENT_CACHE_ENABLE` | 回退到默认 | `34e2297` |
| `EGUI_CONFIG_IMAGE_TRANSFORM_EXTERNAL_ROW_PERSISTENT_CACHE_ENABLE` | 回退到默认 | `c995d40` |
| `EGUI_CONFIG_FONT_STD_ASCII_LOOKUP_INDEX_8BIT` | 删除死 override | `138a317` |
| `EGUI_CONFIG_TEXT_TRANSFORM_LAYOUT_HEAP_ENABLE` | 回退到默认 | `02d08af` |
| `EGUI_CONFIG_INPUT_MOTION_CACHE_COUNT` | 删除死 override | `bbd2dbf` |
| `EGUI_CONFIG_INPUT_VELOCITY_TRACKER_ENABLE` | 删除死 override | `9cb9bee` |
| `EGUI_CONFIG_TOUCH_CAPTURE_PATH_MAX` | 删除死 override | `10330c2` |
| `EGUI_CONFIG_CORE_AUTO_REFRESH_TIMER_ENABLE` | 回退到默认 | `69c492e` |
| `EGUI_CONFIG_CANVAS_EXTRA_CLIP_ENABLE` | 回退到默认 | `a70be3d` |
| `EGUI_CONFIG_CANVAS_SPEC_CIRCLE_INFO_ENABLE` | 回退到默认 | `4187a09` |
| `EGUI_CONFIG_DIRTY_AREA_COUNT` | 回退到默认 | `34d1b02` |
| `EGUI_CONFIG_TEXT_TRANSFORM_LAYOUT_CACHE_ENABLE` | 回退到默认 | `4fb0a86` |
| `EGUI_CONFIG_TEXT_TRANSFORM_PREPARE_CACHE_ENABLE` | 删除死 override | `18bed4f` |
| `EGUI_CONFIG_TEXT_TRANSFORM_DIM_CACHE_ENABLE` | 删除死 override | `6fbab6c` |
| `EGUI_CONFIG_FONT_STD_LINE_CACHE_ENABLE` | 删除死 override | `6b7cf77` |
| `EGUI_CONFIG_FONT_STD_DRAW_PREFIX_CACHE_MAX_GLYPHS` | 删除死 override | `8f33b3f` |
| `EGUI_CONFIG_FONT_STD_DRAW_PREFIX_CACHE_SLOTS` | 删除死 override | `a1d51d2` |
| `EGUI_CONFIG_FUNCTION_SUPPORT_TOAST` | 回退到默认 | `38637ba` |
| `EGUI_CONFIG_TEXT_TRANSFORM_VISIBLE_ALPHA8_STACK_MAX_BYTES` | 删除死 override | `183aaca` |
| `EGUI_CONFIG_QEMU_PLATFORM_MALLOC_ENABLE` | 删除死 override | `9c08111` |
| `EGUI_TEST_CONFIG_IMAGE_LARGE` | 删除死 override | `be80d98` |
| `EGUI_CONFIG_IMAGE_QOI_INDEX_RGB565_CACHE_ENABLE` | 回退到默认 | `ec28479` |
| `EGUI_CONFIG_IMAGE_EXTERNAL_DATA_CACHE_MAX_BYTES` | 删除死 override | `7292758` |
| `EGUI_CONFIG_IMAGE_EXTERNAL_ALPHA_CACHE_MAX_BYTES` | 删除死 override | `b9fc98a` / `af6e912` |
| `EGUI_TEST_CONFIG_IMAGE_DOUBLE` | 默认打开，移除 app 侧 override | `96dae0b` |

## 已确认保留项

| 项目 | 现值 | 保留原因 |
| --- | ---: | --- |
| `EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH` | `0` | 恢复默认后 static RAM `+328B`，最坏 perf 回退 `ANIMATION_SCALE +16.56%` |
| `EGUI_CONFIG_CORE_SEPARATE_USER_ROOT_GROUP_ENABLE` | `0` | 恢复默认后最坏 perf 回退约 `+19.92%` |
| `EGUI_CONFIG_FUNCTION_SUPPORT_ACTIVITY` / `EGUI_CONFIG_FUNCTION_SUPPORT_DIALOG` | `0 / 0` | 受 `user_root=1` 依赖链约束，不能独立回收 |
| `EGUI_CONFIG_IMAGE_STD_ALPHA_OPAQUE_CACHE_SLOTS` | `0` | 恢复默认后最坏 perf 回退约 `+10% ~ +18%` |
| `EGUI_CONFIG_IMAGE_RLE_EXTERNAL_CACHE_WINDOW_SIZE` | `64` | 恢复默认 `1024` 会带来 `+1016B` static RAM，超出本轮 `<100B` 范围 |

- 这组 retained static-RAM 项的现行依据见 `static_ram_override_retention.md`

## 剩余 override 分类

- benchmark 环境宏：
  - `EGUI_CONFIG_SCEEN_WIDTH`
  - `EGUI_CONFIG_SCEEN_HEIGHT`
  - `EGUI_CONFIG_MAX_FPS`
  - `EGUI_CONFIG_PFB_WIDTH`
  - `EGUI_CONFIG_PFB_HEIGHT`
  - `EGUI_CONFIG_PFB_BUFFER_COUNT`
  - `EGUI_CONFIG_CIRCLE_SUPPORT_RADIUS_BASIC_RANGE`
  - `EGUI_CONFIG_DEBUG_LOG_LEVEL`
  - 其中 `PFB_WIDTH / PFB_HEIGHT / PFB_BUFFER_COUNT` 的现行 perf 支撑见 `benchmark_override_retention.md`
- benchmark 能力选择宏：
  - `EGUI_CONFIG_FUNCTION_SUPPORT_SHADOW`
  - `EGUI_CONFIG_FUNCTION_SUPPORT_MASK`
  - `EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_RGB565_1`
  - `EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_RGB565_2`
  - `EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_RGB565_8`
  - `EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_ALPHA_8`
  - `EGUI_CONFIG_FUNCTION_EXTERNAL_RESOURCE`
  - `EGUI_CONFIG_IMAGE_CODEC_QOI_ENABLE`
  - `EGUI_CONFIG_IMAGE_CODEC_RLE_ENABLE`
  - 这组 benchmark workload 开关的现行依据见 `benchmark_capability_retention.md`
- heap/decode 行为宏：
  - `EGUI_CONFIG_IMAGE_CODEC_ROW_CACHE_ENABLE`
  - `EGUI_CONFIG_IMAGE_DECODE_MAX_PIXEL_SIZE`
  - `EGUI_CONFIG_IMAGE_DECODE_OPAQUE_ALPHA_ROW_USE_ROW_CACHE`
  - `EGUI_CONFIG_IMAGE_QOI_CHECKPOINT_COUNT`
  - `EGUI_CONFIG_IMAGE_CODEC_TAIL_ROW_CACHE_ENABLE`
  - 这组 codec/decode override 的现行依据见 `codec_heap_override_retention.md`
- stack/transient-heap 上限宏：
  - `EGUI_CONFIG_SHADOW_DSQ_LUT_MAX`
  - `EGUI_CONFIG_TEXT_TRANSFORM_LAYOUT_PIXEL_INDEX_16BIT`
  - `EGUI_CONFIG_TEXT_TRANSFORM_LAYOUT_LINE_INDEX_16BIT`
  - `EGUI_CONFIG_TEXT_TRANSFORM_SCRATCH_HEAP_ENABLE`
  - `EGUI_CONFIG_TEXT_TRANSFORM_VISIBLE_ALPHA8_MAX_BYTES`
  - 其中 `SHADOW_DSQ_LUT_MAX` 与 `TEXT_TRANSFORM_LAYOUT_* / SCRATCH_HEAP_ENABLE` 的依据见 `stack_heap_policy_retention.md`
  - 其中 `TEXT_TRANSFORM_VISIBLE_ALPHA8_MAX_BYTES` 的现行依据见 `text_transform_heap_override_retention.md`
- 保留的 test/measurement 覆盖入口：
  - `EGUI_TEST_CONFIG_SINGLE_TEST`
  - 用途：单场景 heap/perf spot check
  - 结论：保留外部覆盖能力，不纳入这轮“默认固定开启”清理

## 结论

- 满足 `<100B static RAM` 且 `<5%` perf 的 app 侧小宏，已经清完。
- 当前剩余项要么已确认不能回收，要么超出 `<100B` 门槛，要么属于 heap/stack/codec 行为控制、benchmark 功能选择，或单场景测量入口，不再属于这轮 small static-RAM cleanup。
