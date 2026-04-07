# HelloPerformance 宏清理汇总

## 目标与规则

- 目标：梳理 `HelloPerformance` 当前 app 侧宏，降低为了 SRAM 小收益而引入的维护成本。
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
| `EGUI_CONFIG_FONT_STD_ASCII_LOOKUP_INDEX_8BIT` | 删除 override | `138a317` |
| `EGUI_CONFIG_TEXT_TRANSFORM_LAYOUT_HEAP_ENABLE` | 回退到默认 | `02d08af` |
| `EGUI_CONFIG_INPUT_MOTION_CACHE_COUNT` | 删除 override | `bbd2dbf` |
| `EGUI_CONFIG_INPUT_VELOCITY_TRACKER_ENABLE` | 删除 override | `9cb9bee` |
| `EGUI_CONFIG_TOUCH_CAPTURE_PATH_MAX` | 删除 override | `10330c2` |
| `EGUI_CONFIG_CORE_AUTO_REFRESH_TIMER_ENABLE` | 回退到默认 | `69c492e` |
| `EGUI_CONFIG_CANVAS_EXTRA_CLIP_ENABLE` | 回退到默认 | `a70be3d` |
| `EGUI_CONFIG_CANVAS_SPEC_CIRCLE_INFO_ENABLE` | 回退到默认 | `4187a09` |
| `EGUI_CONFIG_DIRTY_AREA_COUNT` | 回退到默认 | `34d1b02` |
| `EGUI_CONFIG_TEXT_TRANSFORM_LAYOUT_CACHE_ENABLE` | 回退到默认 | `4fb0a86` |
| `EGUI_CONFIG_TEXT_TRANSFORM_PREPARE_CACHE_ENABLE` | 删除 override | `18bed4f` |
| `EGUI_CONFIG_TEXT_TRANSFORM_DIM_CACHE_ENABLE` | 删除 override | `6fbab6c` |
| `EGUI_CONFIG_FONT_STD_LINE_CACHE_ENABLE` | 删除 override | `6b7cf77` |
| `EGUI_CONFIG_FONT_STD_DRAW_PREFIX_CACHE_MAX_GLYPHS` | 删除 override | `8f33b3f` |
| `EGUI_CONFIG_FONT_STD_DRAW_PREFIX_CACHE_SLOTS` | 删除 override | `a1d51d2` |
| `EGUI_CONFIG_FUNCTION_SUPPORT_TOAST` | 回退到默认 | `38637ba` |
| `EGUI_CONFIG_TEXT_TRANSFORM_VISIBLE_ALPHA8_STACK_MAX_BYTES` | 删除 override | `183aaca` |
| `EGUI_CONFIG_QEMU_PLATFORM_MALLOC_ENABLE` | 删除 override | `9c08111` |
| `EGUI_TEST_CONFIG_IMAGE_LARGE` | 删除 app 侧 override | `be80d98` |
| `EGUI_CONFIG_IMAGE_QOI_INDEX_RGB565_CACHE_ENABLE` | 回退到默认 | `ec28479` |
| `EGUI_CONFIG_IMAGE_CODEC_PERSISTENT_CACHE_MAX_BYTES` / `EGUI_CONFIG_IMAGE_EXTERNAL_PERSISTENT_CACHE_MAX_BYTES` | 删除 override | `ba71f3b` |
| `EGUI_CONFIG_IMAGE_EXTERNAL_DATA_CACHE_MAX_BYTES` | 删除 override | `7292758` |
| `EGUI_CONFIG_IMAGE_EXTERNAL_ALPHA_CACHE_MAX_BYTES` | 删除 override | `b9fc98a` / `af6e912` |
| `EGUI_TEST_CONFIG_IMAGE_SMALL` | 删除 app 侧 override | `996963c` |
| `EGUI_TEST_CONFIG_IMAGE_DOUBLE` | 默认打开，移除 app 侧 override | `96dae0b` |

- `2026-04-05` 当前主线重跑确认：`EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH` 继续保持 `HelloPerformance=0`。为避免误测，先把 app 侧 touch 宏改成可被 `USER_CFLAGS` 覆盖；真实生效后 `0 -> 1` 为 `text +2560B, bss +288B`，完整 `239` 场景 perf 全等，runtime `241/241` 全等，但 `HelloUnitTest` 在 `touch=0` 时会因 `test_combobox.c` 的 touch 专项用例未做宏保护而编译失败，因此当前仍保留 `0`。
- `2026-04-05` 当前主线重跑确认：`EGUI_CONFIG_CORE_SEPARATE_USER_ROOT_GROUP_ENABLE` 已不再是 HelloPerformance 的 app-side active override。代码搜索显示 `app_egui_config.h` 已不显式定义它；真实 `0 -> 1` 仅带来 `text +36B`、`bss` 不变，完整 `239` 场景 `ge10=1/le10=0`，runtime `241/241`、unit `688/688` 全等。`EGUI_CONFIG_FUNCTION_SUPPORT_ACTIVITY / DIALOG` 也同样不再由 app 侧显式定义，因此这 3 项已从当前 retained override 清单移出，仅在 `static_ram_override_retention.md` 保留历史纠偏记录。
- `2026-04-06` 当前主线复核后，`EGUI_CONFIG_IMAGE_QOI_ROW_INDEX_8BIT_ENABLE` 已从 public 配置面移除，并直接固定 shipped 路径 `0`；保留默认 `0` 的结论仍成立，因为历史 `0 -> 1` 仅 `text +12B`，完整 `239` 场景无 `>=10%` 波动，runtime `241/241`、unit `688/688` 全等。
- `2026-04-06` 当前主线复核后，`EGUI_CONFIG_IMAGE_DECODE_OPAQUE_ALPHA_ROW_USE_ROW_CACHE` 已从 public 配置面移除，并直接固定 shipped 路径 `0`；保留默认 `0` 的结论仍成立，因为历史 `1` 相比 `0` 也只额外拿到 `text -132B, bss -8B`，完整 `239` 场景 `ge10=0`、`le10=0`，最大绝对波动只有 `0.431%`，runtime `241/241`、unit `688/688` 全等。
- `2026-04-06` 当前主线复核后，`EGUI_CONFIG_IMAGE_QOI_INDEX_RGB565_CACHE_ENABLE` 已从 public 配置面移除，并直接固定 shipped 路径 `1`；保留默认 `1` 的结论仍成立，因为历史 `1 -> 0` 也只回收 `text -64B` 和约 `128B`/活动实例 heap，完整 `239` 场景 `ge10=0`、`le10=0`，runtime `241/241`、unit `688/688` 全等。
- `2026-04-06` 当前主线复核后，`EGUI_CONFIG_IMAGE_QOI_COMPACT_RGB565_INDEX_ENABLE` 已从 public 配置面移除，并直接固定 shipped 路径 `0`；保留默认 `0` 的结论仍成立，因为历史 `0 -> 1` 只换来约 `192B`/活动实例 heap，却会引入 `text +780B` 并让完整 `239` 场景出现 `10` 个 `>=10%` 的 `QOI RGB565` 主路径回退。
- `2026-04-06` 当前主线复核后，`EGUI_CONFIG_IMAGE_STD_ALPHA_OPAQUE_CACHE_SLOTS` 已从 public 配置面移除，并直接固定 shipped 路径 `4`；保留默认 `4` 的结论仍成立，因为历史 `4 -> 0` 只回收 `text -156B, bss -40B`，远低于 1KB 门槛，而且 runtime `4 vs 0` 仍有 `8` 帧真实像素差。
- `2026-04-06` 当前主线复核后，`EGUI_CONFIG_IMAGE_QOI_CHECKPOINT_COUNT` 已从 public 配置面移除，并直接固定 shipped 路径 `2`；保留默认 `2` 的结论仍成立，因为历史 `2 -> 0` 只回收 `text -276B, bss -8B`，却会让完整 `239` 场景出现 `20` 个 `>=10%` 的 QOI 主路径回退。
- `2026-04-06` 当前主线复核后，`EGUI_CONFIG_IMAGE_RLE_CHECKPOINT_ENABLE` 已从 public 配置面移除，并直接固定 shipped 路径 `1`；保留默认 `1` 的结论仍成立，因为历史 `1 -> 0` 只回收 `text -92B, bss -16B`，却会让完整 `239` 场景出现 `20` 个 `>=10%` 的 RLE 主路径回退。
- `2026-04-06` 当前主线复核后，`EGUI_CONFIG_IMAGE_RLE_EXTERNAL_WINDOW_PERSISTENT_CACHE_ENABLE` 已从 public 配置面移除，但实现侧仍暂留 shipped=`1` 的条件策略；保留默认 `1` 的结论仍成立，因为历史 `1 -> 0` 虽能回收 `bss -1040B`，却会带来 `text +816B`，并让 external / tiled RLE 路径出现 `+2.7% ~ +6.2%` 的局部回退。
- `2026-04-05` 当前主线重跑确认：`EGUI_CONFIG_IMAGE_QOI_COMPACT_RGB565_INDEX_ENABLE` 不应再被写成 `HelloPerformance=1`；`0 -> 1` 虽然能把单个活动 `QOI decode state` 从约 `404B` 压到 `212B`，额外节省 `192B heap`，但会引入 `text +780B`，并让完整 `239` 场景里的 `10` 个 `>=10%` 回退全部集中在 `QOI RGB565` 主路径：`IMAGE_QOI_565 / MASK_IMAGE_QOI_NO_MASK +52.2%`、`MASK_IMAGE_QOI_ROUND_RECT +50.3%`、`MASK_IMAGE_QOI_CIRCLE +49.2%`、`EXTERN_IMAGE_QOI_565 / EXTERN_MASK_IMAGE_QOI_NO_MASK +19.3%`、`EXTERN_MASK_IMAGE_QOI_IMAGE +10.4%`；基础 `RECTANGLE / CIRCLE / ROUND_RECTANGLE / TEXT / IMAGE_565 / EXTERN_IMAGE_565 / IMAGE_RESIZE_565 / EXTERN_IMAGE_RESIZE_565` 仍在噪声内，runtime `241/241`、unit `688/688` 全等，因此 shipped/default 继续保持 `0`，`1` 不再作为推荐 low-RAM 值。
- `2026-04-05` 当前主线重跑确认：`EGUI_CONFIG_IMAGE_CODEC_TAIL_ROW_CACHE_ENABLE` 不应再被写成当前 `HelloPerformance` 默认值 `1`；真实 shipped/default 仍是 `0`，`1` 只是历史实验值。`1 -> 0` 虽然能避免 `text +9976B`，且 `data / bss` 不变，runtime `241/241`、unit `688/688` 也全等，但完整 `239` 场景里会出现 `40` 个 `>=10%` 回退，集中在 `QOI/RLE` 压缩图主路径：`EXTERN_IMAGE_QOI_565 / EXTERN_MASK_IMAGE_QOI_NO_MASK +189.9%`、`IMAGE_QOI_565 / MASK_IMAGE_QOI_NO_MASK +168.2%`、`EXTERN_IMAGE_RLE_565 / EXTERN_MASK_IMAGE_RLE_NO_MASK +141.3%`、`IMAGE_RLE_565 / MASK_IMAGE_RLE_NO_MASK +121.3%`；基础 `RECTANGLE / CIRCLE / ROUND_RECTANGLE / TEXT / IMAGE_565 / EXTERN_IMAGE_565 / IMAGE_RESIZE_565 / EXTERN_IMAGE_RESIZE_565` 仍是 `+0.0%`，因此这颗宏现在应被记录成“历史实验值 `1`（当前默认 `0`）且仍是高优先级 codec policy 候选”，而不是简单弱化成失效历史注脚。
- `2026-04-05` 当前主线重跑确认：`EGUI_CONFIG_IMAGE_CODEC_ROW_CACHE_ENABLE` 继续保留 `HelloPerformance=1` 仍成立；`1 -> 0` 虽然能拿到 `text -5108B, data -12B, bss -8B`，runtime `241/241`、unit `688/688` 也全等，但完整 `239` 场景里仍有 `4` 个 `>=10%` 回退，集中在 internal tiled codec 热点：`IMAGE_TILED_QOI_565_0 +633.6%`、`IMAGE_TILED_RLE_565_0 +202.8%`、`IMAGE_TILED_QOI_565_8 +175.8%`、`IMAGE_TILED_RLE_565_8 +111.0%`；基础 `RECTANGLE / CIRCLE / ROUND_RECTANGLE / TEXT / IMAGE_565 / EXTERN_IMAGE_565 / IMAGE_RESIZE_565 / EXTERN_IMAGE_RESIZE_565` 保持在噪声内，因此这颗 override 仍不适合回退到框架默认 `0`。
- `2026-04-06` 当前主线复核后，`EGUI_CONFIG_IMAGE_CODEC_TAIL_ROW_CACHE_MAX_COLS` 已从 public 配置面移除，并直接固定 shipped 路径 `0`。这颗宏没有任何 code-size 收益可言，而历史更窄 cap `184/176/144/96` 会让当前 `240px` / `PFB_WIDTH=48` 横向 walk 下的 QOI/RLE alpha 热点出现 `+45% ~ +66%` 乃至数倍回退；当前仓内也没有真实 shipped override，因此不再继续保留 public 入口。
- `2026-04-06` 当前主线复核后，`EGUI_CONFIG_FONT_STD_CODE_LOOKUP_CACHE_ASCII_COMPACT` 已从 public 配置面移除，并直接固定 shipped 路径 `0`；保留默认 `0` 的结论仍成立，因为历史 `0 -> 1` 仅回收 `data -20B`，却会带来 `text +200B`，完整 `239` 场景 perf、runtime `241/241`、unit `688/688` 全等。
- `2026-04-05` 当前主线重跑确认：`EGUI_CONFIG_FONT_STD_ASCII_LOOKUP_CACHE_ENABLE` 保持默认 `0` 仍成立；`0 -> 1` 会带来 `text +1392B`，但无任何 `>=10%` 回退，并在 `TEXT_GRADIENT / TEXT_RECT / TEXT_ROTATE_NONE` 上带来 `-11.2% ~ -11.6%` 改善；runtime `241/241`、unit `688/688` 全等。
- `2026-04-06` 当前主线复核后，`EGUI_CONFIG_FONT_STD_ASCII_LOOKUP_INDEX_8BIT` 已从 public 配置面移除，并直接固定 shipped 路径 `0`；保留默认 `0` 的结论仍成立，因为历史 `0 -> 1` 的静态 `text/data/bss`、完整 `239` 场景 perf、runtime `241/241`、unit `688/688` 全部完全相同。若后续要重新证明它值得单独存在，仍需补运行期 heap 量化。
- `2026-04-06` 当前主线复核后，`EGUI_CONFIG_FONT_STD_LINE_CACHE_ENABLE` 已从 public 配置面移除，并直接固定 shipped 路径 `0`；保留默认 `0` 的结论仍成立，因为历史 `0 -> 1` 会带来 `text +488B`，完整 `239` 场景无任何 `>=10%` 回退或 `<=-10%` 改善，只有 `TEXT_ROTATE_NONE / TEXT_RECT / EXTERN_TEXT_RECT / TEXT_RECT_GRADIENT` 出现 `+3.8% ~ +5.7%` 级波动；runtime `241/241`、unit `688/688` 全等。
- `2026-04-05` 当前主线重跑确认：`EGUI_CONFIG_FONT_STD_DRAW_PREFIX_CACHE_MAX_GLYPHS / EGUI_CONFIG_FONT_STD_DRAW_PREFIX_CACHE_SLOTS` 保持默认 `0 / 0` 仍成立；`0 / 0 -> 64 / 2` 会带来 `text +1816B, bss +2616B`，并在完整 `239` 场景里同时出现 `TEXT +18.8%`、`EXTERN_TEXT +15.7%` 回退，以及 `EXTERN_TEXT_RECT -32.2%`、`TEXT_GRADIENT -14.2%`、`TEXT_RECT -12.4%`、`TEXT_ROTATE_NONE -11.4%` 改善；runtime `241/241`、unit `688/688` 全等。
- `2026-04-05` 当前主线重跑确认：`EGUI_CONFIG_IMAGE_CODEC_PERSISTENT_CACHE_MAX_BYTES` 保持默认 `0` 仍成立；`0 -> 5000` 会带来 `text +2948B, bss +24B`，完整 `239` 场景没有任何 `>=10%` 回退，但只有 `IMAGE_TILED_QOI_565_0 -13.1%` 一项达到 `<=-10%` 改善，其余 codec 场景都在噪声内；runtime `241/241`、unit `688/688` 全等。默认仍保持 `0`，`5000` 只保留为窄收益实验记录，不进入推荐入口。
- `2026-04-05` 当前主线重跑确认：`EGUI_CONFIG_IMAGE_EXTERNAL_PERSISTENT_CACHE_MAX_BYTES` 保持默认 `0` 仍成立；`0 -> 5000` 会带来 `text +948B, bss +40B`，完整 `239` 场景没有任何 `>=10%` 回退，但在 `EXTERN_IMAGE_TILED_565_0` 和 `EXTERN_IMAGE_RESIZE_TILED_565_1/2/4` 上带来 `-32.6% ~ -61.6%` 改善；runtime `241/241`、unit `688/688` 全等。默认仍保持 `0`，`5000` 只保留为可选 perf-first 实验值。
- `2026-04-06` 当前主线复核后，`EGUI_CONFIG_IMAGE_STD_EXTERNAL_ROW_PERSISTENT_CACHE_ENABLE` 已从 public 配置面移除，并直接固定 shipped 路径 `1`；保留默认 `1` 的结论仍成立，因为历史 `1 -> 0` 仅 `text +20B, bss -40B`，完整 `239` 场景无 `>=10%` 波动，runtime `241/241`、unit `688/688` 全等。
- `2026-04-06` 当前主线复核后，`EGUI_CONFIG_IMAGE_TRANSFORM_EXTERNAL_ROW_PERSISTENT_CACHE_ENABLE` 已从 public 配置面移除，并直接固定 shipped 路径 `1`；保留默认 `1` 的结论仍成立，因为历史 `1 -> 0` 直接拿到 `text -24B, bss -56B`，完整 `239` 场景无 `>=10%` 波动，runtime `241/241`、unit `688/688` 全等。
- 本轮继续移除了 `EGUI_CONFIG_SCEEN_WIDTH` app override，直接继承库默认 `240`；同时把 `code_perf_check.py` 调整为按维度回默认值，避免缺宽度定义时误判成 `240x320`。`2026-03-31` 已复跑 HelloPerformance clean perf，`read_screen_size()` 仍识别为 `(240, 240)`，报告继续输出 `222` 个 case。
- 本轮继续折叠了 `example/HelloPerformance/egui_view_test_performance.c` 中固定为 `1` 的 `EGUI_TEST_CONFIG_IMAGE_LARGE` 编译期开关。对应 HelloPerformance clean perf 仍保持 `222` 个 case，关键锚点维持 `IMAGE_565_DOUBLE 0.551ms`、`TEXT_ROTATE_BUFFERED 11.498ms`、`ANIMATION_SCALE 0.320ms`。
- 本轮继续折叠了 `example/HelloPerformance/egui_view_test_performance.c` 中固定为 `1` 的 `EGUI_TEST_CONFIG_IMAGE_SMALL` 编译期开关。对应 HelloPerformance clean perf 仍保持 `222` 个 case，关键锚点继续维持 `IMAGE_565_DOUBLE 0.551ms`、`TEXT_ROTATE_BUFFERED 11.498ms`、`ANIMATION_SCALE 0.320ms`。
- 本轮把 stack/transient-heap 低 RAM 策略宏移到了 `example/HelloPerformance/app_egui_config.h` 尾部，并收敛成默认关闭的 `#if 0` 可选块。随后又把 `EGUI_CONFIG_SHADOW_DSQ_LUT_MAX=64` 直接上收为库默认值，因此 `HelloPerformance` 不再重复定义它。当前关键锚点仍按 `64` 这个默认值统计：`TEXT_ROTATE_BUFFERED 6.411ms`、`EXTERN_TEXT_ROTATE_BUFFERED 6.792ms`、`SHADOW_ROUND 4.949ms`、`IMAGE_565_DOUBLE 0.551ms`、`ANIMATION_SCALE 0.320ms`；runtime check 仍是 `223/223`。

## 已确认保留项

| 项目 | 现值 | 保留原因 |
| --- | ---: | --- |
| `EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH` | `0` | `2026-04-05` 当前主线 recheck 后 `0 -> 1` 为 `text +2560B, bss +288B`，完整 `239` 场景 perf 全等，runtime `241/241` 全等；但 `HelloUnitTest` 在 `touch=0` 时编译失败（`test_combobox.c` 的 touch 专项用例未做宏保护） |
| `EGUI_CONFIG_IMAGE_STD_ALPHA_OPAQUE_CACHE_SLOTS` | 当前实现内固定 `4`（历史 public 实验值 `0`） | `2026-04-06` 已从 public 配置面移除并直接固定 shipped 路径；历史 `4 -> 0` 只回收 `text -156B, bss -40B`，perf `ge10=0/le10=1`，但 runtime `4 vs 0` 仍有 `8` 帧真实像素差，且 `4 vs 4 repeat` 控制组全等 |
| `EGUI_CONFIG_IMAGE_RLE_EXTERNAL_CACHE_WINDOW_SIZE` | 当前默认 `1024`（历史实验值 `64`） | `2026-04-05` 当前主线 recheck 后 `1024 -> 64` 仍是 `text +96B, bss -1024B`，runtime `241/241`、unit `688/688` 全等，但 external RLE 主路径仍有 `+17.5% ~ +21.8%` 回退 |

- 这组 retained static-RAM 项的现行依据见 `static_ram_override_retention.md`

## 剩余 override 分类

- benchmark 环境宏：
  - `EGUI_CONFIG_SCEEN_HEIGHT`
  - `EGUI_CONFIG_MAX_FPS`
  - `EGUI_CONFIG_PFB_WIDTH`
  - `EGUI_CONFIG_PFB_HEIGHT`
  - `EGUI_CONFIG_PFB_BUFFER_COUNT`
  - `EGUI_CONFIG_CIRCLE_SUPPORT_RADIUS_BASIC_RANGE`
  - `EGUI_CONFIG_DEBUG_LOG_LEVEL`
  - 其中 `SCEEN_WIDTH` 已删除 app override，直接继承库默认 `240`
  - 其中 `SCEEN_HEIGHT / MAX_FPS / CIRCLE_SUPPORT_RADIUS_BASIC_RANGE / DEBUG_LOG_LEVEL` 的现行依据见 `benchmark_environment_retention.md`
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
  - `EGUI_CONFIG_IMAGE_CODEC_TAIL_ROW_CACHE_ENABLE`
  - 这组 codec/decode override 的现行依据见 `codec_heap_override_retention.md`
- 可选 low-RAM stack/transient-heap 策略块：
  - `EGUI_CONFIG_TEXT_TRANSFORM_LAYOUT_PIXEL_INDEX_16BIT`
  - `EGUI_CONFIG_TEXT_TRANSFORM_LAYOUT_LINE_INDEX_16BIT`
  - `EGUI_CONFIG_TEXT_TRANSFORM_SCRATCH_HEAP_ENABLE`
  - `EGUI_CONFIG_TEXT_TRANSFORM_VISIBLE_ALPHA8_MAX_BYTES`
  - 现状：这组宏已经移到 `app_egui_config.h` 尾部的 `#if 0` 可选块，默认关闭
  - 含义：默认构建直接继承框架 stack/transient-heap 默认值；需要 tighter low-RAM profile 时，再按需打开这组 override
  - 其中 `SHADOW_DSQ_LUT_MAX` 与 `TEXT_TRANSFORM_LAYOUT_* / SCRATCH_HEAP_ENABLE` 的依据见 `stack_heap_policy_retention.md`
  - 其中 `TEXT_TRANSFORM_VISIBLE_ALPHA8_MAX_BYTES` 的依据见 `text_transform_heap_override_retention.md`
- 保留的 test/measurement 覆盖入口：
  - `EGUI_TEST_CONFIG_SINGLE_TEST`
  - 用途：单场景 heap/perf spot check
  - 结论：保留外部覆盖能力，不纳入这轮“默认固定开启”清理

## 结论

- 满足 `<100B static RAM` 且 `<5%` perf 的 app 侧小宏，已经清完。
- 本轮进一步移除了与库默认值相同的 `EGUI_CONFIG_SCEEN_WIDTH` 冗余 override。
- 当前剩余项要么已确认不能回收，要么超出 `<100B` 门槛，要么属于 heap/stack/codec 行为控制、benchmark 功能选择，或单场景测量入口，不再属于这轮 small static-RAM cleanup。
- 其中 stack/transient-heap low-RAM 策略宏已经从 active override 集合里收束到 `app_egui_config.h` 尾部的可选 `#if 0` 块，默认构建直接走框架默认 headroom。
