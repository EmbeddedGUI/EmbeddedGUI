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
| `EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_RGB565_1` | `1` | `0` | benchmark 能力 | 保留 | clean perf 仍覆盖 `IMAGE/RESIZE/ROTATE_565_1` 与 `EXTERN_*_565_1` |
| `EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_RGB565_2` | `1` | `0` | benchmark 能力 | 保留 | clean perf 仍覆盖 `IMAGE/RESIZE/ROTATE_565_2` 与 `EXTERN_*_565_2` |
| `EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_RGB565_8` | `1` | `0` | benchmark 能力 | 保留 | clean perf 仍覆盖 `IMAGE/RESIZE/ROTATE_565_8` 与 `EXTERN_*_565_8` |
| `EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_ALPHA_8` | `1` | `0` | benchmark 能力 | 保留 | clean perf 仍覆盖 `MASK_IMAGE_QOI_8_*` / `MASK_IMAGE_RLE_8_*` |
| `EGUI_CONFIG_FUNCTION_EXTERNAL_RESOURCE` | `1` | `0` | benchmark 能力 | 保留 | clean perf 仍有 `62` 个 `EXTERN_*` 场景；详见 `benchmark_capability_retention.md` |
| `EGUI_CONFIG_IMAGE_CODEC_QOI_ENABLE` | `1` | `0` | benchmark 能力 | 保留 | clean perf 仍有 `22` 个 `*QOI*` 场景；详见 `benchmark_capability_retention.md` |
| `EGUI_CONFIG_IMAGE_CODEC_RLE_ENABLE` | `1` | `0` | benchmark 能力 | 保留 | clean perf 仍有 `22` 个 `*RLE*` 场景；详见 `benchmark_capability_retention.md` |
| `EGUI_CONFIG_IMAGE_CODEC_ROW_CACHE_ENABLE` | `1` | `0` | decode/heap 行为 | 保留 | `row-cache off` 会让 QOI/RLE 热点出现 `+1800% ~ +3600%` 级回退；详见 `codec_heap_override_retention.md` |
| `EGUI_CONFIG_IMAGE_DECODE_MAX_PIXEL_SIZE` | 当前默认 `2`（历史对比 `4`） | `2` | decode/heap 行为 | 保留默认 | 当前完整 A/B 显示 `2 -> 4` 仅 `text +108B`，`239` perf 场景无 `>=10%` 波动，runtime/unit 等价；它已上收为框架默认值，主要约束约 `7680B` decode heap 上界 |
| `EGUI_CONFIG_IMAGE_DECODE_OPAQUE_ALPHA_ROW_USE_ROW_CACHE` | 当前 shipped 固定 `0`（历史 public 实验值 `1`） | `0` | decode/heap 行为 | 已删分叉 | 当前完整 A/B 无 perf/runtime/unit 差异，`1` 仅额外回收 `text -132B, bss -8B`；收益低于 1KB 门槛，已从 public 配置面移除并直接固定 shipped 路径，详见 `codec_heap_override_retention.md` |
| `EGUI_CONFIG_IMAGE_QOI_CHECKPOINT_COUNT` | 当前 shipped 固定 `2`（历史 public 实验值 `0`） | `2` | decode/heap 行为 | 已删分叉 | 当前完整 A/B 显示 `2 -> 0` 只回收 `text -276B, bss -8B`，但 `20` 个 QOI 场景回退 `+110% ~ +911%`；收益低于 1KB 门槛，已从 public 配置面移除并直接固定 shipped 路径，详见 `codec_heap_override_retention.md` |
| `EGUI_CONFIG_IMAGE_CODEC_TAIL_ROW_CACHE_ENABLE` | 历史实验值 `1`（当前默认 `0`） | `0` | decode/heap 行为 | 保留 | `2026-04-05` 当前 recheck 显示：`1` 侧 `text +9976B`，但 `0` 侧会让 `40` 个 `QOI/RLE` 压缩图场景出现 `+16.2% ~ +189.9%` 回退；它已不是当前 active override，但仍是高优先级 codec policy 候选 |
| `EGUI_CONFIG_IMAGE_CODEC_TAIL_ROW_CACHE_MAX_COLS` | 当前 shipped 固定 `0`（历史 public 实验值 `184/176/144/96`） | `0` | decode/heap 行为 | 已删分叉 | 更窄 tail cap 没有任何 code-size 收益，且历史 A/B 显示 `184/176` 仍回退 `+45% ~ +66%`，`144/96` 直接退化到数倍；当前仓内也无 shipped 依赖，因此已从 public 配置面移除并直接固定 shipped 路径 |
| `EGUI_CONFIG_IMAGE_QOI_INDEX_RGB565_CACHE_ENABLE` | 当前 shipped 固定 `1`（历史 public 实验值 `0`） | `1` | decode/heap 行为 | 已删分叉 | 当前完整 A/B 显示 `1 -> 0` 只回收 `text -64B`，但完整 `239` 场景无 `>=10%` 波动，runtime/unit 等价；收益低于 1KB 门槛，已从 public 配置面移除并直接固定 shipped 路径 |
| `EGUI_CONFIG_IMAGE_QOI_COMPACT_RGB565_INDEX_ENABLE` | 当前 shipped 固定 `0`（历史 public 实验值 `1`） | `0` | decode/heap 行为 | 已删分叉 | 当前完整 A/B 显示 `0 -> 1` 会带来 `text +780B`，并让 `10` 个 `QOI RGB565` 场景回退 `+10.4% ~ +52.2%`；收益低于 1KB 门槛，已从 public 配置面移除并直接固定 shipped 路径 |
| `EGUI_CONFIG_IMAGE_RLE_EXTERNAL_WINDOW_PERSISTENT_CACHE_ENABLE` | 当前实现侧 shipped=`1`（历史 public 实验值 `0`） | `1` | decode/heap 行为 | 实现侧暂留 | `2026-04-05` 当前 A/B：`1 -> 0` 虽可回收 `bss -1040B`，但会带来 `text +816B`，且 `IMAGE_TILED_RLE_565_0 +6.2%`、external RLE 主路径约 `+2.7% ~ +5.1%`；这颗宏已经从 public 配置面移除，但因 SRAM 收益超过当前 256B 门槛，仍暂留为实现侧条件策略，详见 `codec_heap_override_retention.md` |
| `EGUI_CONFIG_IMAGE_RLE_EXTERNAL_CACHE_WINDOW_SIZE` | 历史实验值 `64`（当前默认 `1024`） | `1024` | static RAM | 保留默认 | `2026-04-05` 当前 recheck：`1024 -> 64` 仍会回收 `bss -1024B` 并带来 `text +96B`，runtime/unit 等价，但 external RLE 主路径仍回退 `+17.5% ~ +21.8%`；详见 `static_ram_override_retention.md` |
| `EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH` | `0` | `1` | static RAM | 保留 | `2026-04-05` recheck：`0 -> 1` 为 `text +2560B, bss +288B`，完整 `239` 场景 perf 全等，runtime `241/241` 全等；`HelloUnitTest` 在 `touch=0` 时编译失败（touch 专项用例未做宏保护）；详见 `static_ram_override_retention.md` |
| `EGUI_CONFIG_IMAGE_STD_ALPHA_OPAQUE_CACHE_SLOTS` | 当前 shipped 固定 `4`（历史 public 实验值 `0`） | `4` | static RAM | 已删分叉 | `2026-04-06` 当前复核：`4 -> 0` 只回收 `text -156B, bss -40B`，perf `ge10=0/le10=1`，但 runtime `4 vs 0` 仍有 `8` 帧真实像素差，且 `4 vs 4 repeat` 控制组全等；收益低于 1KB 门槛，已从 public 配置面移除并直接固定 shipped 路径；详见 `static_ram_override_retention.md` |

- `2026-04-05` 代码搜索与 recheck 还确认：`EGUI_CONFIG_CORE_SEPARATE_USER_ROOT_GROUP_ENABLE` / `EGUI_CONFIG_FUNCTION_SUPPORT_ACTIVITY` / `EGUI_CONFIG_FUNCTION_SUPPORT_DIALOG` 已不再由 `HelloPerformance` app-side 明确定义，因此从当前 active override 矩阵移出。其历史 low-RAM 口径与最新 `user_root=1` A/B 结果见 `static_ram_override_retention.md`。

## 可选 Low-RAM Tail Block

- `EGUI_CONFIG_TEXT_TRANSFORM_LAYOUT_PIXEL_INDEX_16BIT`
- `EGUI_CONFIG_TEXT_TRANSFORM_LAYOUT_LINE_INDEX_16BIT`
- `EGUI_CONFIG_TEXT_TRANSFORM_SCRATCH_HEAP_ENABLE`
- `EGUI_CONFIG_TEXT_TRANSFORM_VISIBLE_ALPHA8_MAX_BYTES`

- `EGUI_CONFIG_SHADOW_DSQ_LUT_MAX=64` 已上收为库默认值，不再属于 `HelloPerformance` 的可选 app-side tail block

以上 4 项已移动到 `example/HelloPerformance/app_egui_config.h` 尾部的 `#if 0` 可选块，默认关闭，不再属于当前 active app-side override 集合。

- 默认关闭后的当前 clean perf：
  - `222` 个 case
  - `TEXT_ROTATE_BUFFERED 6.411ms`
  - `EXTERN_TEXT_ROTATE_BUFFERED 6.792ms`
  - `SHADOW_ROUND 4.949ms`
  - `IMAGE_565_DOUBLE 0.551ms`
  - `ANIMATION_SCALE 0.320ms`
- 对应 runtime check：`223/223`
- 低 RAM 取值、历史 A/B 和重新打开这些宏时的取舍，分别见 `stack_heap_policy_retention.md` 与 `text_transform_heap_override_retention.md`

## 结论

- 到当前 `HEAD`，`HelloPerformance/app_egui_config.h` 中所有剩余 override 都已经有明确分类和处理结论。
- `EGUI_CONFIG_SCEEN_WIDTH` 已回退到库默认 `240`，不再占用 app 侧宏管理。
- stack/transient-heap low-RAM 策略宏已下沉到尾部可选块，默认构建直接继承框架默认值。
- 本轮可回收的小 static-RAM 宏已经清完；剩余项均不再满足当前回收规则，或者本身就不属于这轮 `<100B static RAM` 清理范围。
