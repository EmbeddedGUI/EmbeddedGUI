# HelloPerformance 宏决策矩阵

## 说明

- 范围：仅统计 `example/HelloPerformance/app_egui_config.h` 当前仍保留的 app 侧 override。
- 目标：给出每个宏的当前值、默认值、分类和当前处理结论，作为本轮小宏清理的最终索引。
- 判定规则沿用本轮标准：只回收 `static RAM <100B` 且 `HelloPerformance` QEMU perf 最大回退 `<5%` 的项。

| 宏 | 当前值 | 默认值 | 分类 | 当前结论 | 依据 |
| --- | --- | --- | --- | --- | --- |
| `EGUI_CONFIG_SCEEN_HEIGHT` | `240` | `320` | benchmark 环境 | 保留 | 当前 clean perf 锚定在 `240x240`；详见 `benchmark_environment_retention.md` |
| `EGUI_CONFIG_MAX_FPS` | `1` | `60` | benchmark 环境 | 保留 | benchmark 节奏锚点；详见 `benchmark_environment_retention.md` |
| `EGUI_CONFIG_PFB_WIDTH` | `48` | `(EGUI_CONFIG_SCEEN_WIDTH / 8)` | benchmark 环境 | 保留 | `48x16x1=1536B`；仅回退到 `30x30x1` 就是 `+264B`，且 `--pfb-matrix` 依赖此入口 |
| `EGUI_CONFIG_PFB_HEIGHT` | `16` | `(EGUI_CONFIG_SCEEN_HEIGHT / 8)` | benchmark 环境 | 保留 | 与 `PFB_WIDTH` 一起定义当前低 SRAM 基准几何；详见 `benchmark_override_retention.md` |
| `EGUI_CONFIG_PFB_BUFFER_COUNT` | `1` | `2` | benchmark 环境 | 保留 | 当前 `1 -> 2` 直接 `+1536B`，且 `--spi-matrix` 依赖此入口 |
| `EGUI_CONFIG_CIRCLE_SUPPORT_RADIUS_BASIC_RANGE` | `241` | `50` | benchmark 环境 | 保留 | 保持 `radius=240` 场景 coverage 的最小值；详见 `benchmark_environment_retention.md` |
| `EGUI_CONFIG_DEBUG_LOG_LEVEL` | `EGUI_LOG_IMPL_LEVEL_INF` | `EGUI_LOG_IMPL_LEVEL_NONE` | benchmark 环境 | 保留 | perf 报告依赖 `PERF_RESULT/PERF_COMPLETE`；详见 `benchmark_environment_retention.md` |
| `EGUI_CONFIG_FUNCTION_SUPPORT_SHADOW` | `1` | `0` | benchmark 能力 | 保留 | clean perf 仍直接覆盖 `SHADOW/SHADOW_ROUND`；详见 `benchmark_capability_retention.md` |
| `EGUI_CONFIG_FUNCTION_SUPPORT_MASK` | `1` | `0` | benchmark 能力 | 保留 | clean perf 仍有 `49` 个 `MASK_*` 场景；详见 `benchmark_capability_retention.md` |
| `EGUI_CONFIG_FUNCTION_SUPPORT_ACTIVITY` | `0` | `1` | static RAM / 依赖链 | 保留 | 被 rejected `user_root=1` 路径阻塞，不能独立回收；详见 `static_ram_override_retention.md` |
| `EGUI_CONFIG_FUNCTION_SUPPORT_DIALOG` | `0` | `1` | static RAM / 依赖链 | 保留 | `dialog=1` 会强制 `activity=1`，同样被 rejected `user_root=1` 路径阻塞 |
| `EGUI_CONFIG_CORE_SEPARATE_USER_ROOT_GROUP_ENABLE` | `0` | `1` | static RAM | 保留 | 仅约 `+56B`，但最坏 `MASK_RECT_FILL_NO_MASK_QUARTER +19.92%`；详见 `static_ram_override_retention.md` |
| `EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_RGB565_1` | `1` | `0` | benchmark 能力 | 保留 | clean perf 仍覆盖 `IMAGE/RESIZE/ROTATE_565_1` 与 `EXTERN_*_565_1` |
| `EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_RGB565_2` | `1` | `0` | benchmark 能力 | 保留 | clean perf 仍覆盖 `IMAGE/RESIZE/ROTATE_565_2` 与 `EXTERN_*_565_2` |
| `EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_RGB565_8` | `1` | `0` | benchmark 能力 | 保留 | clean perf 仍覆盖 `IMAGE/RESIZE/ROTATE_565_8` 与 `EXTERN_*_565_8` |
| `EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_ALPHA_8` | `1` | `0` | benchmark 能力 | 保留 | clean perf 仍覆盖 `MASK_IMAGE_QOI_8_*` / `MASK_IMAGE_RLE_8_*` |
| `EGUI_CONFIG_FUNCTION_EXTERNAL_RESOURCE` | `1` | `0` | benchmark 能力 | 保留 | clean perf 仍有 `62` 个 `EXTERN_*` 场景；详见 `benchmark_capability_retention.md` |
| `EGUI_CONFIG_IMAGE_CODEC_QOI_ENABLE` | `1` | `0` | benchmark 能力 | 保留 | clean perf 仍有 `22` 个 `*QOI*` 场景；详见 `benchmark_capability_retention.md` |
| `EGUI_CONFIG_IMAGE_CODEC_RLE_ENABLE` | `1` | `0` | benchmark 能力 | 保留 | clean perf 仍有 `22` 个 `*RLE*` 场景；详见 `benchmark_capability_retention.md` |
| `EGUI_CONFIG_IMAGE_CODEC_ROW_CACHE_ENABLE` | `1` | `0` | decode/heap 行为 | 保留 | `row-cache off` 会让 QOI/RLE 热点出现 `+1800% ~ +3600%` 级回退；详见 `codec_heap_override_retention.md` |
| `EGUI_CONFIG_IMAGE_DECODE_MAX_PIXEL_SIZE` | `2` | `4` | decode/heap 行为 | 保留 | 绑定 RGB565-only decode 路径，主要影响约 `7680B` heap 上界，不是 small static-RAM 宏 |
| `EGUI_CONFIG_IMAGE_DECODE_OPAQUE_ALPHA_ROW_USE_ROW_CACHE` | `1` | `0` | decode/heap 行为 | 保留 | 复用 row-cache alpha 头部，影响约 `248B` decode heap 形态；详见 `codec_heap_override_retention.md` |
| `EGUI_CONFIG_IMAGE_QOI_CHECKPOINT_COUNT` | `0` | `2` | decode/heap 行为 | 保留 | `1/2` checkpoint 无法挽回窄 tail 方案的 `+45% ~ +66%` 回退；详见 `codec_heap_override_retention.md` |
| `EGUI_CONFIG_IMAGE_CODEC_TAIL_ROW_CACHE_ENABLE` | `1` | `0` | decode/heap 行为 | 保留 | 当前默认是 `-1584B heap / +8B static RAM` 的 codec 模式，主要不是 small static-RAM 取舍 |
| `EGUI_CONFIG_IMAGE_RLE_EXTERNAL_CACHE_WINDOW_SIZE` | `64` | `1024` | static RAM | 保留 | 恢复默认 `+1016B static RAM`，远超本轮阈值；详见 `static_ram_override_retention.md` |
| `EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH` | `0` | `1` | static RAM | 保留 | 恢复默认后 static RAM `+328B`，`ANIMATION_SCALE +16.56%`；详见 `static_ram_override_retention.md` |
| `EGUI_CONFIG_IMAGE_STD_ALPHA_OPAQUE_CACHE_SLOTS` | `0` | `4` | static RAM | 保留 | 仅约 `~33B` BSS，但最坏 `EXTERN_IMAGE_RESIZE_TILED_565_8 +17.78%`；详见 `static_ram_override_retention.md` |

## 可选 Low-RAM Tail Block

- `EGUI_CONFIG_SHADOW_DSQ_LUT_MAX`
- `EGUI_CONFIG_TEXT_TRANSFORM_LAYOUT_PIXEL_INDEX_16BIT`
- `EGUI_CONFIG_TEXT_TRANSFORM_LAYOUT_LINE_INDEX_16BIT`
- `EGUI_CONFIG_TEXT_TRANSFORM_SCRATCH_HEAP_ENABLE`
- `EGUI_CONFIG_TEXT_TRANSFORM_VISIBLE_ALPHA8_MAX_BYTES`

以上 5 项已移动到 `example/HelloPerformance/app_egui_config.h` 尾部的 `#if 0` 可选块，默认关闭，不再属于当前 active app-side override 集合。

- 默认关闭后的当前 clean perf：
  - `222` 个 case
  - `TEXT_ROTATE_BUFFERED 6.411ms`
  - `EXTERN_TEXT_ROTATE_BUFFERED 6.792ms`
  - `SHADOW_ROUND 6.306ms`
  - `IMAGE_565_DOUBLE 0.551ms`
  - `ANIMATION_SCALE 0.321ms`
- 对应 runtime check：`223/223`
- 低 RAM 取值、历史 A/B 和重新打开这些宏时的取舍，分别见 `stack_heap_policy_retention.md` 与 `text_transform_heap_override_retention.md`

## 结论

- 到当前 `HEAD`，`HelloPerformance/app_egui_config.h` 中所有剩余 override 都已经有明确分类和处理结论。
- `EGUI_CONFIG_SCEEN_WIDTH` 已回退到库默认 `240`，不再占用 app 侧宏管理。
- stack/transient-heap low-RAM 策略宏已下沉到尾部可选块，默认构建直接继承框架默认值。
- 本轮可回收的小 static-RAM 宏已经清完；剩余项均不再满足当前回收规则，或者本身就不属于这轮 `<100B static RAM` 清理范围。
