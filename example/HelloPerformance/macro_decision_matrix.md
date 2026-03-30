# HelloPerformance 宏决策矩阵

## 说明

- 范围：仅统计 `example/HelloPerformance/app_egui_config.h` 当前仍保留的 app 侧 override。
- 目标：给出每个宏的当前值、默认值、分类和当前处理结论，作为本轮小宏清理的最终索引。
- 判定规则沿用本轮标准：只回收 `static RAM <100B` 且 `HelloPerformance` QEMU perf 最大回退 `<5%` 的项。

| 宏 | 当前值 | 默认值 | 分类 | 当前结论 | 依据 |
| --- | --- | --- | --- | --- | --- |
| `EGUI_CONFIG_SCEEN_WIDTH` | `240` | `240` | benchmark 环境 | 保留 | HelloPerformance 固定测试分辨率 |
| `EGUI_CONFIG_SCEEN_HEIGHT` | `240` | `320` | benchmark 环境 | 保留 | HelloPerformance 固定为 `240x240` |
| `EGUI_CONFIG_MAX_FPS` | `1` | `60` | benchmark 环境 | 保留 | 限制录制/基准刷新节奏 |
| `EGUI_CONFIG_PFB_WIDTH` | `48` | `(EGUI_CONFIG_SCEEN_WIDTH / 8)` | benchmark 环境 | 保留 | 当前基准 tile 形状 |
| `EGUI_CONFIG_PFB_HEIGHT` | `16` | `(EGUI_CONFIG_SCEEN_HEIGHT / 8)` | benchmark 环境 | 保留 | 当前基准 tile 形状 |
| `EGUI_CONFIG_PFB_BUFFER_COUNT` | `1` | `2` | benchmark 环境 | 保留 | QEMU/PC 同步 flush，不做小宏回收 |
| `EGUI_CONFIG_CIRCLE_SUPPORT_RADIUS_BASIC_RANGE` | `241` | `50` | benchmark 环境 | 保留 | HelloPerformance 覆盖大圆场景 |
| `EGUI_CONFIG_DEBUG_LOG_LEVEL` | `EGUI_LOG_IMPL_LEVEL_INF` | `EGUI_LOG_IMPL_LEVEL_NONE` | benchmark 环境 | 保留 | 便于当前 benchmark 运行观察 |
| `EGUI_CONFIG_FUNCTION_SUPPORT_SHADOW` | `1` | `0` | benchmark 能力 | 保留 | 当前场景需要阴影 benchmark |
| `EGUI_CONFIG_FUNCTION_SUPPORT_MASK` | `1` | `0` | benchmark 能力 | 保留 | 当前场景需要 mask benchmark |
| `EGUI_CONFIG_FUNCTION_SUPPORT_ACTIVITY` | `0` | `1` | static RAM / 依赖链 | 保留 | 与 `user_root=0` 绑定，不能独立回收 |
| `EGUI_CONFIG_FUNCTION_SUPPORT_DIALOG` | `0` | `1` | static RAM / 依赖链 | 保留 | `dialog=1` 会强制 `activity=1` |
| `EGUI_CONFIG_CORE_SEPARATE_USER_ROOT_GROUP_ENABLE` | `0` | `1` | static RAM | 保留 | 恢复默认虽仅约 `+56B`，但最坏 perf 回退约 `+19.92%` |
| `EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_RGB565_1` | `1` | `0` | benchmark 能力 | 保留 | 当前场景需要该图片格式 |
| `EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_RGB565_2` | `1` | `0` | benchmark 能力 | 保留 | 当前场景需要该图片格式 |
| `EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_RGB565_8` | `1` | `0` | benchmark 能力 | 保留 | 当前场景需要该图片格式 |
| `EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_ALPHA_8` | `1` | `0` | benchmark 能力 | 保留 | 当前场景需要 alpha8 图片 |
| `EGUI_CONFIG_FUNCTION_EXTERNAL_RESOURCE` | `1` | `0` | benchmark 能力 | 保留 | 当前场景包含外部资源基准 |
| `EGUI_CONFIG_IMAGE_CODEC_QOI_ENABLE` | `1` | `0` | benchmark 能力 | 保留 | 当前场景包含 QOI benchmark |
| `EGUI_CONFIG_IMAGE_CODEC_RLE_ENABLE` | `1` | `0` | benchmark 能力 | 保留 | 当前场景包含 RLE benchmark |
| `EGUI_CONFIG_IMAGE_CODEC_ROW_CACHE_ENABLE` | `1` | `0` | decode/heap 行为 | 保留 | `row-cache off` 已被历史 A/B 明确拒绝 |
| `EGUI_CONFIG_IMAGE_DECODE_MAX_PIXEL_SIZE` | `2` | `4` | decode/heap 行为 | 保留 | 绑定 HelloPerformance 当前 RGB565-only decode 路径 |
| `EGUI_CONFIG_IMAGE_DECODE_OPAQUE_ALPHA_ROW_USE_ROW_CACHE` | `1` | `0` | decode/heap 行为 | 保留 | 属于 decode heap 形态，不是小 static-RAM 宏 |
| `EGUI_CONFIG_IMAGE_QOI_CHECKPOINT_COUNT` | `0` | `2` | decode/heap 行为 | 保留 | 属于 codec heap/perf 路径，不在本轮 `<100B static RAM` 规则内 |
| `EGUI_CONFIG_IMAGE_CODEC_TAIL_ROW_CACHE_ENABLE` | `1` | `0` | decode/heap 行为 | 保留 | 属于 codec heap/perf 路径，不在本轮 `<100B static RAM` 规则内 |
| `EGUI_CONFIG_IMAGE_RLE_EXTERNAL_CACHE_WINDOW_SIZE` | `64` | `1024` | static RAM | 保留 | 恢复默认会带来 `+1016B` static RAM，超出本轮阈值 |
| `EGUI_CONFIG_SHADOW_DSQ_LUT_MAX` | `64` | `256` | stack 行为 | 保留 | 这是 stack/LUT 上限，不是 fixed static RAM 项 |
| `EGUI_CONFIG_TEXT_TRANSFORM_LAYOUT_PIXEL_INDEX_16BIT` | `1` | `0` | heap 行为 | 保留 | 属于 text-transform heap/layout 策略 |
| `EGUI_CONFIG_TEXT_TRANSFORM_LAYOUT_LINE_INDEX_16BIT` | `1` | `0` | heap 行为 | 保留 | 属于 text-transform heap/layout 策略 |
| `EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH` | `0` | `1` | static RAM | 保留 | 恢复默认后 static RAM `+328B`，`ANIMATION_SCALE +16.56%` |
| `EGUI_CONFIG_TEXT_TRANSFORM_SCRATCH_HEAP_ENABLE` | `1` | `0` | heap 行为 | 保留 | 属于 stack/heap 放置策略，不在本轮范围内 |
| `EGUI_CONFIG_TEXT_TRANSFORM_VISIBLE_ALPHA8_MAX_BYTES` | `2560` | `4096` | heap 行为 | 保留 | 属于 transient heap ceiling，不是 fixed static RAM 项 |
| `EGUI_CONFIG_IMAGE_STD_ALPHA_OPAQUE_CACHE_SLOTS` | `0` | `4` | static RAM | 保留 | 恢复默认后最坏 perf 回退约 `+10% ~ +18%` |

## 结论

- 到当前 `HEAD`，`HelloPerformance/app_egui_config.h` 中所有剩余 override 都已经有明确分类和处理结论。
- 本轮可回收的小 static-RAM 宏已经清完；剩余项均不再满足当前回收规则，或者本身就不属于这轮 `<100B static RAM` 清理范围。
