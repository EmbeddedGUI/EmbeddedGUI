# 低 RAM 配置宏说明文档

本文档梳理 `HelloPerformance` 等低 RAM 场景中使用的细粒度配置宏，说明每个宏的作用机制、默认值、及在 HelloPerformance 下的设定与 RAM 节省效果。

所有宏均可在应用的 `app_egui_config.h` 中覆盖，不影响其他应用的默认行为。

说明：下文 `HelloPerformance` 列同时包含“当前 shipped 默认”与“历史 low-RAM 实验值”。当前默认始终以 `example/HelloPerformance/app_egui_config.h` 为准；若某项已被回退为默认值，会在对应小节里单独说明。

---

## 一、图像解码相关宏

### 1. `EGUI_CONFIG_IMAGE_STD_ALPHA_OPAQUE_CACHE_SLOTS`

**位置：** [src/image/egui_image_std.c](../../../src/image/egui_image_std.c)

| 属性 | 值 |
|------|---|
| 默认值 | `4` |
| HelloPerformance | 当前默认 `4`（历史实验值 `0`） |
| RAM 类型 | BSS |
| HP 节省 | ~33 B |

**机制：**
缓存"某张 RGB565+alpha 图片的 alpha 通道是否全为不透明"的探测结果，跨帧复用以跳过逐行扫描，走更快的纯 RGB565 合成路径。SLOTS 槽数控制最多缓存多少张图片的结论，采用轮转淘汰。

设为 `0` 时整个 cache 数组不编译，每次绘制均重新扫描。

`2026-04-04` 基于当前 `HelloPerformance` 主线重新做完整 A/B 后，结论更新为：
- 关闭后 `HelloPerformance text 2055380 -> 2055224`，回收 `text -156B`、`bss -40B`
- 完整 `239` perf 场景里没有任何 `>=10%` 回退，只有 `EXTERN_IMAGE_RESIZE_TILED_565_8 -11.4%` 这一处 `>=10%` 改善
- 基础 `RECTANGLE / CIRCLE / ROUND_RECTANGLE / TEXT / IMAGE_565 / EXTERN_IMAGE_565 / IMAGE_RESIZE_565 / EXTERN_IMAGE_RESIZE_565` 全部保持在噪声内
- 但 runtime `4 vs 0` 有 `8` 帧真实像素差：`frame_0066~0069`、`frame_0095~0098`
- 额外控制组 `4 vs 4` repeat 为 `hash mismatch 0`、`pixel mismatch 0`，说明这不是录制噪声
- `HelloUnitTest` on / off 仍是 `688/688 passed`

因此这颗宏当前不能再作为 `HelloPerformance` 的 active low-RAM 推荐项。默认继续保持 `4`；历史实验值 `0` 只有在后续先解释并修掉这 `8` 帧渲染差异后，才值得重新评估。

**数据结构（默认 SLOTS=4 时）：**
```c
static egui_image_std_alpha_opaque_cache_t g_egui_image_std_alpha_opaque_cache[4]; // 4×8B
static uint8_t g_egui_image_std_alpha_opaque_cache_next;                          // 1B
```

---

### 2. `EGUI_CONFIG_IMAGE_DECODE_MAX_PIXEL_SIZE`

**位置：** [src/core/egui_config_default.h](../../../src/core/egui_config_default.h)

| 属性 | 值 |
|------|---|
| 默认值 | 当前默认 `2`（历史默认 `4`，对应 RGBA32 decode 上界） |
| HelloPerformance | 当前直接继承默认 `2`（历史对比值 `4`） |
| RAM 类型 | heap（动态峰值） |
| HP 节省 | 相比历史 `4` 约 ~7680 B 峰值堆 |

**机制：**
决定 codec 行缓存和单行 pixel scratch 的最大字节上界（PFB 行宽度 × 屏幕宽度 × 本宏）。当前框架默认已经收窄到 `2`，因为 HelloPerformance 当前压缩图像资产均为纯 RGB565，不含 RGBA32；`4` 仅保留为历史默认/对比值，用于需要 RGBA32 decode 上界的场景。

**公式：**
```
codec_row_cache_pixel_max = PFB_HEIGHT × SCREEN_WIDTH × EGUI_CONFIG_IMAGE_DECODE_MAX_PIXEL_SIZE
当前默认/HelloPerf(PFB=16×16): 16 × 240 × 2 = 7,680 B
历史默认 4 在相同 PFB 下: 16 × 240 × 4 = 15,360 B
差值: 7,680 B
```

`2026-04-05` 按当前主线完成 `2 -> 4` 全量 A/B 后，结果为：
- size：`text 2055380 -> 2055488`，即 `text +108B`，`bss` 不变
- perf：完整 `239` 场景没有任何 `>=10%` 回退或 `<=-10%` 改善，最大绝对波动仅 `IMAGE_TILED_QOI_565_8 -0.325%`
- 基础 `RECTANGLE / CIRCLE / ROUND_RECTANGLE / TEXT / IMAGE_565 / EXTERN_IMAGE_565 / IMAGE_RESIZE_565 / EXTERN_IMAGE_RESIZE_565` 全部 `+0.0%`
- runtime / unit：`HelloPerformance` 两侧都是 `241 frames` 且 `hash mismatch 0`、`pixel mismatch 0`；`HelloUnitTest` 两侧都是 `688/688 passed`

因此当前 shipped/default 应记为 `2`，`4` 只保留为历史默认/外部 A/B 对比值，不再视为 `HelloPerformance` 的 active app-side override。

---

### 3. `EGUI_CONFIG_IMAGE_DECODE_OPAQUE_ALPHA_ROW_USE_ROW_CACHE`

**位置：** [src/core/egui_config_default.h](../../../src/core/egui_config_default.h)，使用处 [src/image/egui_image_decode_utils.c](../../../src/image/egui_image_decode_utils.c)

| 属性 | 值 |
|------|---|
| 默认值 | `0` |
| HelloPerformance | 当前默认 `0`（历史实验值 `1`） |
| RAM 类型 | heap（节省独立分配） |
| HP 节省 | ~248 B（240B data + 8B 指针） |
| 依赖 | 需 `IMAGE_CODEC_ROW_CACHE_ENABLE=1` |

**机制：**
带 mask 的不透明 RGB565 图绘制时需要一行"全 255 alpha"参考缓冲用于合成回退。设为 `0` 时单独在 heap 上分配并跨帧持久；设为 `1` 时直接复用已存在的 codec row cache 的 alpha 缓冲头部，省去独立的 240 B 持久堆分配。

`2026-04-04` 基于当前 `HelloPerformance` 主线重新做完整 A/B 后，结论更新为：
- 历史实验值 `1` 相比当前默认 `0` 为 `text -132B, bss -8B`
- 完整 `239` perf 场景里没有任何 `>=10%` 回退或改善，最大绝对波动仅 `±0.4%`
- 基础主路径 `RECTANGLE / CIRCLE / ROUND_RECTANGLE / TEXT / IMAGE_565 / EXTERN_IMAGE_565 / IMAGE_RESIZE_565 / EXTERN_IMAGE_RESIZE_565` 全部 `+0.0%`
- runtime `241` 帧 `hash mismatch 0`、`pixel mismatch 0`
- `HelloUnitTest` on / off 都是 `688/688 passed`

因此这颗宏当前不应再写成 `HelloPerformance=1` 的 active override。对 `HelloPerformance` 而言，`1` 现阶段更适合作为行为等价、收益很小的条件实验值；当前 shipped/default 仍记为 `0`，是否改框架默认需要更广覆盖验证。

---

### 4. `EGUI_CONFIG_IMAGE_CODEC_TAIL_ROW_CACHE_ENABLE`

**位置：** [src/image/egui_image_qoi.c](../../../src/image/egui_image_qoi.c)，[src/image/egui_image_rle.c](../../../src/image/egui_image_rle.c)

| 属性 | 值 |
|------|---|
| 默认值 | `0`（全宽 row-band 模式） |
| HelloPerformance | 历史实验值 `1`（当前默认 `0`） |
| RAM 类型 | heap（峰值减小） |
| HP 节省 | ~2304 B 峰值堆（QOI/RLE α 场景） |

**机制：**
标准 row-band 模式：第一个 PFB tile 解码全图宽的当前行带并缓存，后续水平 tile 直接读缓存。Tail 模式：第一个 PFB tile 的像素直接从 scratch 流式解码并渲染，只把"后续 tile 尚未用到的尾列"写入 row cache。

在 240px 宽、48px PFB 步宽的场景下，第一 tile 占 48 列，尾部 192 列进入缓存，节省首 tile 的 `16×48×3 = 2304 B` 峰值堆分配。

`2026-04-04` 基于当前主线重新做完整 A/B 后，结论需要更新：

- 开启后 `HelloPerformance text +9976B`
- 当前默认 `0` 相比历史实验值 `1` 在完整 `239` perf 场景里有 `40` 个 `>=10%` 回退
- 回退全部集中在 `QOI/RLE` 压缩图主路径及其 masked 变体：
  - `IMAGE_QOI_565 +168.2%`
  - `EXTERN_IMAGE_QOI_565 +189.9%`
  - `IMAGE_RLE_565 +121.3%`
  - `EXTERN_IMAGE_RLE_565 +141.3%`
  - `MASK_IMAGE_QOI_NO_MASK +168.2%`
  - `EXTERN_MASK_IMAGE_QOI_NO_MASK +189.9%`
  - `MASK_IMAGE_RLE_NO_MASK +121.3%`
  - `EXTERN_MASK_IMAGE_RLE_NO_MASK +141.3%`
- 基础 `RECTANGLE / CIRCLE / ROUND_RECTANGLE / TEXT / IMAGE_565 / EXTERN_IMAGE_565 / IMAGE_RESIZE_565 / EXTERN_IMAGE_RESIZE_565` 都是 `+0.0%`
- runtime `241` 帧和 `HelloUnitTest 688/688` 仍保持等价

因此这颗宏现在不应再被写成 `HelloPerformance` 当前默认值；它是历史 low-RAM / codec policy 候选值 `1`。当前默认保持 `0`，但如果后续需要重新平衡 codec heap 与压缩图吞吐，这仍是优先级很高的一颗重评估宏。

---

### 5. `EGUI_CONFIG_IMAGE_QOI_INDEX_RGB565_CACHE_ENABLE`

**位置：** [src/image/egui_image_qoi.c](../../../src/image/egui_image_qoi.c)

| 属性 | 值 |
|------|---|
| 默认值 | `1`（预转换缓存） |
| HelloPerformance | 当前默认 `1`（历史实验值 `0`） |
| RAM 类型 | heap（QOI decode state 内部） |
| HP 节省 | ~128 B / 每个活跃解码实例 |

**机制：**
QOI 解码器维护 64 个历史色彩槽（RGBA index，256 B）。启用时额外维护对应的 RGB565 预转换版本（128 B），INDEX 操作命中时无需重新做 RGB888→RGB565 转换；禁用时节省 128 B heap，每次 INDEX 命中时实时计算颜色转换（代价极小）。

`2026-04-04` 基于当前 `HelloPerformance` 主线重新做完整 A/B 后，结论更新为：
- 关闭后 `HelloPerformance text 2055380 -> 2055316`，只回收 `text -64B`，最终 ELF `bss` 不变
- 完整 `239` perf 场景里没有任何 `>=10%` 回退或改善
- 基础 `RECTANGLE / CIRCLE / ROUND_RECTANGLE / TEXT / IMAGE_565 / EXTERN_IMAGE_565 / IMAGE_RESIZE_565 / EXTERN_IMAGE_RESIZE_565` 全部 `+0.0%`
- 最大绝对波动仍在噪声内，集中在 QOI 路径：
  - `IMAGE_QOI_565 +2.8%`
  - `MASK_IMAGE_QOI_NO_MASK +2.8%`
  - `MASK_IMAGE_QOI_ROUND_RECT +2.7%`
  - `MASK_IMAGE_QOI_CIRCLE +2.6%`
  - `EXTERN_MASK_IMAGE_QOI_IMAGE -1.7%`
  - `EXTERN_IMAGE_QOI_565 -1.2%`
  - `EXTERN_MASK_IMAGE_QOI_NO_MASK -1.2%`
- runtime `241` 帧和 `HelloUnitTest 688/688` 仍保持等价

因此这颗宏当前更适合作为条件实验宏：默认继续保持 `1`，若项目要进一步挤每个活动 QOI decode state 的 `128B` 瞬时 heap，再按场景评估是否关闭。

---

### 6. `EGUI_CONFIG_IMAGE_QOI_COMPACT_RGB565_INDEX_ENABLE`

**位置：** [src/image/egui_image_qoi.c](../../../src/image/egui_image_qoi.c)

| 属性 | 值 |
|------|---|
| 默认值 | `0` |
| HelloPerformance | 历史实验值 `1`（当前默认 `0`） |
| RAM 类型 | heap（QOI decode state） |
| HP 节省 | ~192 B / 每个活跃解码实例 |

**机制：**
将 QOI 的 64 项颜色索引表从标准 `uint32_t index_rgba[64]`（256 B）替换为紧凑版 `uint16_t index_rgb565[64]` + `uint8_t index_aux[64]`（共 192 B）。专为纯 RGB565 图源设计，辅助字节存放 alpha 或速度偏置信息，重建 RGB888 时从 RGB565 规范扩展推算，与 `RGB565_CACHE=0` 联用。

对比当前默认（compact=0, rgb565_cache=1）的 404 B，compact 配置只占 212 B，净省 192 B。

`2026-04-04` 基于当前 `HelloPerformance` 主线重新做完整 A/B 后，结论已经更新：

- 开启后 `HelloPerformance text +780B`
- 完整 `239` perf 场景里有 `10` 个 `>=10%` 回退，全部集中在 `QOI RGB565` 主路径
  - `IMAGE_QOI_565 +52.2%`
  - `EXTERN_IMAGE_QOI_565 +19.3%`
  - `MASK_IMAGE_QOI_NO_MASK +52.2%`
  - `EXTERN_MASK_IMAGE_QOI_NO_MASK +19.3%`
- runtime `241` 帧和 `HelloUnitTest 688/688` 仍保持等价

因此它现在只保留为“历史 low-RAM 实验思路”的参考项，不再建议作为当前 `HelloPerformance` 配置打开。

---

### 7. `EGUI_CONFIG_IMAGE_QOI_ROW_INDEX_8BIT_ENABLE`

**位置：** [src/image/egui_image_qoi.c](../../../src/image/egui_image_qoi.c)

| 属性 | 值 |
|------|---|
| 默认值 | 当前默认 `0`（uint16_t，支持 ≤65535 行） |
| HelloPerformance | 当前直接继承默认 `0`（历史实验值 `1`，uint8_t，限 ≤255 行） |
| RAM 类型 | heap（decode state 内字段） |
| HP 节省 | 历史 `1` 可回收 ~2~4 B / 每个解码实例 |

**机制：**
QOI 解码状态结构体中 `current_row`（行游标）的整数宽度。降至 uint8_t 要求图片高度不超过 255 行。HelloPerformance 的 QOI 资源最高 240 行，满足条件。

`2026-04-05` 按当前主线完成 `0 -> 1` 全量 A/B 后，结果为：
- size：`text 2055380 -> 2055392`，即 `text +12B`，`bss` 不变
- perf：完整 `239` 场景没有任何 `>=10%` 回退或 `<=-10%` 改善，最大绝对波动仅 `IMAGE_TILED_QOI_565_0 +0.551%`
- QOI 主路径也都在噪声内：`IMAGE_QOI_565 +0.016%`、`EXTERN_IMAGE_QOI_565 -0.006%`、`IMAGE_QOI_565_8 -0.047%`、`EXTERN_IMAGE_QOI_565_8 -0.074%`
- runtime / unit：`HelloPerformance` 两侧都是 `241 frames` 且 `hash mismatch 0`、`pixel mismatch 0`；`HelloUnitTest` 两侧都是 `688/688 passed`

因此当前 shipped/default 继续记为 `0`；`1` 只保留为历史低 RAM 对比值，不再作为 `HelloPerformance` 的 active app-side override。

---

## 二、外部资源行缓存共享宏

这组宏协同工作，通过逐步合并/复用缓冲区来减少 RAM 占用。

> **注意：** 原宏 `EGUI_CONFIG_IMAGE_EXTERNAL_ROW_CACHE_SHARE_BUFFERS`（宏8）和 `EGUI_CONFIG_IMAGE_EXTERNAL_SHARED_CACHE_USE_CODEC_ROW_CACHE`（宏9）已被移除。
> - 共享模式现在**强制启用**，std/transform/rle 三路共用一份后备存储，通过 generation 计数自动失效旧缓存。
> - 后备存储改为 **heap 动态分配**（首次使用时分配），未使用外部资源时 RAM 占用为 0。
> - 不再需要用户判断 std 和 codec 场景是否混用。

### 8. `EGUI_CONFIG_IMAGE_STD_EXTERNAL_ROW_PERSISTENT_CACHE_ENABLE`

**位置：** [src/image/egui_image_std.c](../../../src/image/egui_image_std.c)

| 属性 | 值 |
|------|---|
| 默认值 | 当前默认 `1` |
| HelloPerformance | 当前直接继承默认 `1`（历史实验值 `0`） |
| RAM 类型 | BSS |
| HP 节省 | 历史 `0` 可回收 ~40~48 B BSS |

**机制：**
外部 std 图像行块缓存的元数据（chunk 起始行、行数、行字节数、图片指针等）是否存放在全局 BSS。设为 `1` 跨 tile 保留行位置信息，减少 semihosting I/O 次数；设为 `0` 元数据转为调用栈局部变量，BSS 消去，代价是每次 draw 调用重建（HP 场景影响不大）。

`2026-04-05` 按当前主线完成 `1 -> 0` 全量 A/B 后，结果为：
- size：`text 2055380 -> 2055400`，即 `text +20B`，`bss 3832 -> 3792`
- perf：完整 `239` 场景没有任何 `>=10%` 回退或 `<=-10%` 改善，最大绝对波动仅 `EXTERN_IMAGE_RESIZE_TILED_565_0 +2.467%`
- 基础主路径保持在噪声内：`RECTANGLE / CIRCLE / ROUND_RECTANGLE / TEXT` 全部 `+0.0%`，`IMAGE_565 +1.674%`、`EXTERN_IMAGE_565 +0.959%`、`IMAGE_RESIZE_565 +0.0%`、`EXTERN_IMAGE_RESIZE_565 +0.789%`
- external codec 热点也保持稳定：`EXTERN_IMAGE_QOI_565 +0.0%`、`EXTERN_IMAGE_RLE_565 +0.0%`、`EXTERN_MASK_IMAGE_QOI_IMAGE -0.03%`、`EXTERN_MASK_IMAGE_RLE_IMAGE +0.0%`
- runtime / unit：`HelloPerformance` 两侧都是 `241 frames` 且 `hash mismatch 0`、`pixel mismatch 0`；`HelloUnitTest` 两侧都是 `688/688 passed`

因此当前 shipped/default 继续记为 `1`；`0` 只保留为低优先级 low-RAM 条件实验值，不再作为 `HelloPerformance` 的 active app-side override。

---

### 9. `EGUI_CONFIG_IMAGE_TRANSFORM_EXTERNAL_ROW_PERSISTENT_CACHE_ENABLE`

**位置：** [src/core/egui_canvas_transform.c](../../../src/core/egui_canvas_transform.c)

| 属性 | 值 |
|------|---|
| 默认值 | 当前默认 `1` |
| HelloPerformance | 当前直接继承默认 `1`（历史实验值 `0`） |
| RAM 类型 | BSS |
| HP 节省 | 历史 `0` 可回收 ~56~80 B BSS |

**机制：**
与宏 8 对称，负责 transform（rotate/scale）路径的外部图片行缓存元数据。默认两个全局 slot 各持行块状态；设为 `0` 后 slot 嵌入各 draw 调用的栈局部 `image_transform_external_source_t`，BSS 释放。

`2026-04-05` 按当前主线完成 `1 -> 0` 全量 A/B 后，结果为：
- size：`text 2055380 -> 2055356`，即 `text -24B`，`bss 3832 -> 3776`
- perf：完整 `239` 场景没有任何 `>=10%` 回退或 `<=-10%` 改善，最大绝对波动仅 `EXTERN_IMAGE_ROTATE_565_1 -1.316%`
- 基础主路径保持在噪声内：`RECTANGLE +0.0%`、`CIRCLE -0.123%`、`ROUND_RECTANGLE +0.0%`、`TEXT -0.097%`、`IMAGE_565 +0.0%`、`EXTERN_IMAGE_565 +0.0%`、`IMAGE_RESIZE_565 +0.0%`、`EXTERN_IMAGE_RESIZE_565 +0.0%`
- transform/rotate 侧也保持稳定：`IMAGE_ROTATE_565 +0.429%`、`IMAGE_ROTATE_565_8 +0.358%`、`IMAGE_ROTATE_TILED_565_0 +0.791%`、`EXTERN_IMAGE_ROTATE_TILED_565_0 +0.360%`
- runtime / unit：`HelloPerformance` 两侧都是 `241 frames` 且 `hash mismatch 0`、`pixel mismatch 0`；`HelloUnitTest` 两侧都是 `688/688 passed`

因此当前 shipped/default 继续记为 `1`；`0` 只保留为低优先级 low-RAM 条件实验值，不再作为 `HelloPerformance` 的 active app-side override。

---

## 三、RLE 外部资源 I/O 窗口宏

### 10. `EGUI_CONFIG_IMAGE_RLE_EXTERNAL_CACHE_WINDOW_SIZE`

**位置：** [src/image/egui_image_rle.c](../../../src/image/egui_image_rle.c)

| 属性 | 值 |
|------|---|
| 默认值 | `1024` |
| HelloPerformance | 当前默认 `1024`（历史实验值 `64`） |
| RAM 类型 | BSS 或 transient frame heap（取决于宏11） |
| HP 节省 | ~960 B（当前实测 `bss -1024B`） |

**机制：**
RLE 外部资源解码时，控制字节（操作码+长度字段）具有强局部性，缓存 `WINDOW_SIZE` 字节可大量减少 semihosting I/O 调用。像素字面量行（240px × 2B = 480 B）超过窗口大小时自动走直接 load，正确性不受影响。历史上 HelloPerformance 曾把它缩到 `64B`，用于 low-RAM 外部 RLE 实验。

`2026-04-04` 基于当前 `HelloPerformance` 主线重新做完整 A/B 后，结论更新为：
- `1024 -> 64` 后 `HelloPerformance text 2055380 -> 2055476`，同时 `bss 3832 -> 2808`
- 即缩小窗口会带来 `text +96B`，但能回收 `bss -1024B`
- 完整 `239` perf 场景里有 `5` 个 `>=10%` 回退，全部集中在 external RLE 主路径及其 masked 变体：
  - `EXTERN_IMAGE_RLE_565 +21.8%`
  - `EXTERN_MASK_IMAGE_RLE_NO_MASK +21.8%`
  - `EXTERN_MASK_IMAGE_RLE_ROUND_RECT +20.8%`
  - `EXTERN_MASK_IMAGE_RLE_CIRCLE +20.3%`
  - `EXTERN_MASK_IMAGE_RLE_IMAGE +17.5%`
- `EXTERN_IMAGE_RLE_565_8 / EXTERN_MASK_IMAGE_RLE_8_*` 也有 `+8.1% ~ +9.4%` 的稳定回退
- 基础 `RECTANGLE / CIRCLE / ROUND_RECTANGLE / TEXT / IMAGE_565 / EXTERN_IMAGE_565 / IMAGE_RESIZE_565 / EXTERN_IMAGE_RESIZE_565` 全部 `+0.0%`
- runtime `241` 帧 `hash mismatch 0`、`pixel mismatch 0`
- `HelloUnitTest` on / off 仍是 `688/688 passed`

因此这颗宏当前适合保留为 low-RAM 条件实验宏，但不适合作为 `HelloPerformance` shipped 默认值。若项目明确不关心 external RLE 热点，`64` 仍然是能稳定回收约 `1KB BSS` 的选项。

---

> 注：当前实现下，宏 10 与宏 11 的组合不再把 external RLE window 放到 draw 调用栈上；关闭 persistent cache 时，window 会改走 transient frame heap。

### 11. `EGUI_CONFIG_IMAGE_RLE_EXTERNAL_WINDOW_PERSISTENT_CACHE_ENABLE`

**位置：** [src/image/egui_image_rle.c](../../../src/image/egui_image_rle.c)

| 属性 | 值 |
|------|---|
| 默认值 | `1`（persistent BSS） |
| HelloPerformance | 默认不覆盖；低 RAM codec/decode 实验块里可设 `0` |
| RAM 类型 | BSS -> transient frame heap |
| HP 实测 | `bss -1040B`，`text +816B`（默认 `WINDOW_SIZE=1024`） |

**机制：**
控制 external RLE read window 是否常驻在全局 BSS。设为 `0` 后，当前实现会把 window 本体改成每帧复用的 transient heap buffer，并在 `egui_image_rle_release_frame_cache()` 里释放，不再把整块 window 放到 draw 调用栈上。

先纠正当前 shipped 关系：
- 框架默认值是 `1`
- `HelloPerformance` 当前 shipped 配置也直接继承这个默认值
- `example/HelloPerformance/app_egui_config.h` 里的 `0` 只保留在尾部 `#if 0` low-RAM codec/decode 块中，属于历史实验值

`2026-04-05` 基于当前 `HelloPerformance` 主线重新做完整 A/B 后，结论更新为：
- `1 -> 0` 后 `HelloPerformance text 2055380 -> 2056196`，`bss 3832 -> 2792`
- 即关闭 persistent cache 会带来 `text +816B`，但能回收 `bss -1040B`
- 完整 `239` perf 场景里没有任何 `>=10%` 回退或改善，最大绝对波动是 `IMAGE_TILED_RLE_565_0 +6.18%`
- 基础 `RECTANGLE / CIRCLE / ROUND_RECTANGLE / TEXT / IMAGE_565 / EXTERN_IMAGE_565 / IMAGE_RESIZE_565 / EXTERN_IMAGE_RESIZE_565` 都在噪声内
- internal `IMAGE_RLE_565 / IMAGE_RLE_565_8 / MASK_IMAGE_RLE_*` 基本都在 `+0.4%` 内
- `IMAGE_TILED_RLE_565_0 +6.2%`、`IMAGE_TILED_RLE_565_8 +2.8%`
- external `EXTERN_IMAGE_RLE_565 / EXTERN_MASK_IMAGE_RLE_NO_MASK +5.1%`
- `EXTERN_MASK_IMAGE_RLE_ROUND_RECT +4.8%`、`EXTERN_MASK_IMAGE_RLE_CIRCLE +4.7%`
- `EXTERN_MASK_IMAGE_RLE_IMAGE +3.7%`
- `EXTERN_IMAGE_RLE_565_8 / EXTERN_MASK_IMAGE_RLE_8_NO_MASK +3.1%`
- `EXTERN_MASK_IMAGE_RLE_8_ROUND_RECT +2.8%`、`EXTERN_MASK_IMAGE_RLE_8_CIRCLE +2.7%`、`EXTERN_MASK_IMAGE_RLE_8_IMAGE +2.7%`
- runtime `241` 帧 `hash mismatch 0`、`pixel mismatch 0`
- `HelloUnitTest` on / off 仍是 `688/688 passed`

结论：这颗宏当前仍适合作为 low-RAM 条件实验宏，不适合作为 code-size 宏。当前 shipped/default 保持 `1`；若项目要把 external RLE window 从 persistent BSS 切到 transient frame heap，`0` 是可接受的条件实验值，但要接受 tiled / external RLE 的小幅回退。它与宏 10 联动：宏 10 决定 window 大小，宏 11 决定这块 window 走 persistent BSS 还是 transient frame heap。

---

## 四、字体缓存宏

### 12. `EGUI_CONFIG_FONT_STD_CODE_LOOKUP_CACHE_ASCII_COMPACT`

**位置：** [src/font/egui_font_std.c](../../../src/font/egui_font_std.c)

| 属性 | 值 |
|------|---|
| 默认值 | 当前默认 `0`（uint16_t/uint32_t 字段） |
| HelloPerformance | 当前直接继承默认 `0`（历史实验值 `1`，uint8_t 紧凑） |
| RAM 类型 | data（静态缓存槽位） |
| HP 节省 | 历史 `1` 可回收约 ~20 B 静态缓存槽位 |

**机制：**
字体 code lookup 加速缓存（`g_font_std_code_lookup_cache`），记录上次命中的 ASCII 码及其字形表下标，加速有序 code 数组的二分查找。Compact 模式将内部字段压缩为 uint8_t，适用于纯 ASCII（code ≤ 255）字体，每 slot 从约 32 B 压缩到约 12 B。

`2026-04-05` 按当前主线完成 `0 -> 1` 全量 A/B 后，结果为：
- size：`text 2055380 -> 2055580`、`data 72 -> 52`、`bss` 不变
- 即历史 `1` 虽然回收了 `data -20B`，但同时带来 `text +200B`
- perf：完整 `239` 场景没有任何 `>=10%` 回退或 `<=-10%` 改善，最大绝对波动仅 `TEXT_GRADIENT +1.293%`
- 文本主路径也都在噪声内：`TEXT +0.0%`、`EXTERN_TEXT +0.0%`、`TEXT_ROTATE +0.0%`、`EXTERN_TEXT_ROTATE +0.0%`、`TEXT_RECT +0.570%`、`EXTERN_TEXT_RECT +0.524%`
- runtime / unit：`HelloPerformance` 两侧都是 `241 frames` 且 `hash mismatch 0`、`pixel mismatch 0`；`HelloUnitTest` 两侧都是 `688/688 passed`

因此当前 shipped/default 继续记为 `0`；`1` 只保留为历史低 RAM 对比值，不再作为 `HelloPerformance` 的 active app-side override。

---

### 13. `EGUI_CONFIG_FONT_STD_ASCII_LOOKUP_CACHE_ENABLE`

**位置：** [src/font/egui_font_std.c](../../../src/font/egui_font_std.c)

| 属性 | 值 |
|------|---|
| 默认值 | 当前默认 `0` |
| HelloPerformance | 当前直接继承默认 `0`（历史实验值 `1`） |
| RAM 类型 | heap + BSS（指针） |
| HP 节省 | 当前默认 `0` 相比历史 `1` 约节省 ~140 B heap + 8 B BSS |

**机制：**
为 ASCII(0~127) 字符预建直查表（`ascii_index[128]`），首次分配后跨帧复用，将 ASCII 字形查找从 O(log n) 降至 O(1)。当前默认 `0` 不保留这张表，从而省掉一小块运行期 heap 和 1 个指针位；历史实验值 `1` 则用更高的静态/运行期成本换取文本路径收益。可与宏 14 联用控制 index 宽度。

`2026-04-05` 按当前主线完成 `0 -> 1` 全量 A/B 后，结果为：
- size：`text 2055380 -> 2056772`，`data/bss` 不变
- 即历史 `1` 会带来 `text +1392B`
- perf：完整 `239` 场景没有任何 `>=10%` 回退，并有 `3` 个 `<=-10%` 改善
- 主要改善集中在文本路径：
  - `TEXT_GRADIENT -11.638%`
  - `TEXT_RECT -11.597%`
  - `TEXT_ROTATE_NONE -11.238%`
  - 其余文本主路径也稳定改善或持平：`EXTERN_TEXT_RECT -5.628%`、`TEXT -0.974%`、`EXTERN_TEXT -0.376%`、`TEXT_ROTATE -0.041%`、`EXTERN_TEXT_ROTATE -0.076%`
- runtime / unit：`HelloPerformance` 两侧都是 `241 frames` 且 `hash mismatch 0`、`pixel mismatch 0`；`HelloUnitTest` 两侧都是 `688/688 passed`

因此当前 shipped/default 应记为 `0`；`1` 不再是 `HelloPerformance` 的 active app-side override，而是一个“多花约 `text +1392B` 和一小块 ASCII cache heap/BSS，换部分文本路径 `11%` 级改善”的历史性能对比值。

---

### 14. `EGUI_CONFIG_FONT_STD_ASCII_LOOKUP_INDEX_8BIT`

**位置：** [src/font/egui_font_std.c](../../../src/font/egui_font_std.c)

| 属性 | 值 |
|------|---|
| 默认值 | 当前默认 `0`（uint16_t） |
| HelloPerformance | 当前直接继承默认 `0`（历史实验值 `1`，uint8_t） |
| RAM 类型 | heap（ASCII cache 内部） |
| HP 节省 | 历史 `1` 理论上可回收约 ~128 B heap（需同时开启 ASCII cache） |

**机制：**
ASCII 直查表 `ascii_index[128]` 内部元素类型。`0` 使用 `uint16_t`，`1` 使用 `uint8_t`。只有在 `EGUI_CONFIG_FONT_STD_ASCII_LOOKUP_CACHE_ENABLE=1` 时，这颗宏才会影响实际 heap 占用；它不改变查找逻辑，只影响缓存表元素宽度。

`2026-04-05` 按当前主线完成 `0 -> 1` 全量 A/B 后，结果为：
- size：`text/data/bss` 全部不变，仍为 `2055380 / 72 / 3832`
- perf：完整 `239` 场景没有任何 `>=10%` 回退或 `<=-10%` 改善；当前结果逐项完全重合，基础 `TEXT / EXTERN_TEXT / TEXT_ROTATE / EXTERN_TEXT_ROTATE / TEXT_RECT / EXTERN_TEXT_RECT` 全部 `+0.0%`
- runtime / unit：`HelloPerformance` 两侧都是 `241 frames` 且 `hash mismatch 0`、`pixel mismatch 0`；`HelloUnitTest` 两侧都是 `688/688 passed`

因此当前 shipped/default 继续记为 `0`；`1` 只保留为历史低 RAM 对比值。当前四项验证只能证明它对静态 size、perf、渲染结果和单测都无差异；若要重新评估它的现实收益，需单独补运行期 heap 量化。

---

### 15. `EGUI_CONFIG_FONT_STD_LINE_CACHE_ENABLE`

**位置：** [src/font/egui_font_std.c](../../../src/font/egui_font_std.c)

| 属性 | 值 |
|------|---|
| 默认值 | 当前默认 `0` |
| HelloPerformance | 当前直接继承默认 `0`（历史实验值 `1`） |
| RAM 类型 | heap + BSS |
| HP 节省 | 当前默认 `0` 相比历史 `1` 约节省 ~164 B heap |

**机制：**
多行文本行分割缓存，避免每次 `get_str_size` 或绘制时重新扫描 `\n`。结构体中含 `lines[MAX_LINES]` 指针数组，缓存最近 SLOTS 个字符串的分割结果。当前默认 `0` 不保留这块运行期 line cache；历史实验值 `1` 会多占一小块 heap，并换取文本矩形/分行路径的一点吞吐收益。

`2026-04-05` 按当前主线完成 `0 -> 1` 全量 A/B 后，结果为：
- size：`text 2055380 -> 2055868`，`data/bss` 不变
- 即历史 `1` 会带来 `text +488B`
- perf：完整 `239` 场景没有任何 `>=10%` 回退，也没有任何 `<=-10%` 改善
- 主要波动集中在文本矩形路径，仍低于 10% 阈值：
  - `TEXT_ROTATE_NONE +5.714%`
  - `TEXT_RECT +5.703%`
  - `EXTERN_TEXT_RECT +3.927%`
  - `TEXT_RECT_GRADIENT +3.811%`
  - 其余文本主路径基本持平：`TEXT -0.097%`、`EXTERN_TEXT +0.0%`、`TEXT_ROTATE +0.0%`、`EXTERN_TEXT_ROTATE +0.0%`
- runtime / unit：`HelloPerformance` 两侧都是 `241 frames` 且 `hash mismatch 0`、`pixel mismatch 0`；`HelloUnitTest` 两侧都是 `688/688 passed`

因此当前 shipped/default 应记为 `0`；历史 `1` 不是 active app-side override，而是一个“多付 `text +488B` 和约 `164B` line-cache heap，换文本矩形/换行路径少量收益”的对比值。

---

### 16 + 17. `EGUI_CONFIG_FONT_STD_DRAW_PREFIX_CACHE_MAX_GLYPHS` / `EGUI_CONFIG_FONT_STD_DRAW_PREFIX_CACHE_SLOTS`

**位置：** [src/font/egui_font_std.c](../../../src/font/egui_font_std.c)

| 属性 | 值 |
|------|---|
| 默认值 | 当前默认 MAX_GLYPHS=`0`，SLOTS=`0` |
| HelloPerformance | 当前直接继承默认 `0 / 0`（历史实验值 `64 / 2`） |
| RAM 类型 | BSS |
| HP 节省 | 当前默认 `0 / 0` 相比历史 `64 / 2` 约节省 ~2616 B BSS |

**机制：**
Draw-prefix cache 缓存每行文字的字形布局元数据（每个字符的 x 坐标、bitmap box、advance、字形下标等）。当同一字符串在多帧多处重复绘制时，跳过字符串扫描和字形 lookup，直接复用缓存位置信息渲染像素，是常规 UI 场景（label/title 等静态文字）的主要加速手段。

当前主线 shipped/default 已经把这组缓存整体下调到 `0 / 0`，不再保留 draw-prefix cache。历史实验值 `64 / 2` 对应的当前主线 A/B 结果为：
- size：`text 2055380 -> 2057196`，`data 72 -> 72`，`bss 3832 -> 6448`
- 即历史 `64 / 2` 会带来 `text +1816B, bss +2616B`
- perf：完整 `239` 场景里出现 `2` 个 `>=10%` 回退、`4` 个 `<=-10%` 改善
- 主要回退：
  - `TEXT +18.793%`
  - `EXTERN_TEXT +15.681%`
- 主要改善：
  - `EXTERN_TEXT_RECT -32.199%`
  - `TEXT_GRADIENT -14.224%`
  - `TEXT_RECT -12.357%`
  - `TEXT_ROTATE_NONE -11.429%`
- 基础图元仍在噪声内：`RECTANGLE / CIRCLE / ROUND_RECTANGLE` 都是 `+0.0%`
- runtime / unit：`HelloPerformance` 两侧都是 `241 frames` 且 `hash mismatch 0`、`pixel mismatch 0`；`HelloUnitTest` 两侧都是 `688/688 passed`

因此当前更准确的定位是：`64 / 2` 不是 active app-side override，而是一组“多付 `text +1816B, bss +2616B`，同时重排不同文本路径开销分布”的历史性能对比值；对当前 `HelloPerformance` 工作负载，它不适合作为默认回开项。

**任一为 0 即可禁用，两者联用确保完全关闭：**
```c
#define EGUI_CONFIG_FONT_STD_DRAW_PREFIX_CACHE_MAX_GLYPHS 0
#define EGUI_CONFIG_FONT_STD_DRAW_PREFIX_CACHE_SLOTS       0
```

---

## 五、文字旋转变换宏

### 18 + 19. `EGUI_CONFIG_TEXT_TRANSFORM_LAYOUT_PIXEL_INDEX_16BIT` / `LAYOUT_LINE_INDEX_16BIT`

**位置：** [src/core/egui_canvas_transform.c](../../../src/core/egui_canvas_transform.c)

| 属性 | 值 |
|------|---|
| 默认值 | `0 / 0`（关闭 16-bit layout 压缩） |
| HelloPerformance | 当前默认 `0 / 0`（历史实验值 `1 / 1`） |
| RAM 类型 | heap（transient，每帧分配释放） |
| HP 节省 | 每颗宏约 `~2N B`（N = 字形数，transient） |

**机制：**
Text transform 先将所有字形的像素偏移和行索引打包到 heap layout 数组，再按 PFB tile 顺序渲染。这两个宏将数组元素从 32-bit 降为 16-bit，分别限制最大像素偏移（≤ 65535 B）和最大字形索引（≤ 65535）。对短 benchmark 字符串（≤ 255 字形）和 26pt 以下字体完全够用。layout 数组是 transient heap（每帧分配、每帧释放），短字符串下节省约数十 B。

`2026-04-04` 基于当前 `HelloPerformance` 主线重新做完整 A/B 后，这组宏的 shipped/default 关系已明确更新为：
- 当前 shipped 不是历史上的 `1 / 1`，而是直接继承框架默认 `0 / 0`
- `1 / 1` 只保留在 `example/HelloPerformance/app_egui_config.h` 尾部 `#if 0` 的可选 low-RAM 块里

其中 `EGUI_CONFIG_TEXT_TRANSFORM_LAYOUT_PIXEL_INDEX_16BIT` 本轮已经完成定向复测，结论为：
- 开启后 `HelloPerformance text 2055380 -> 2055376`，仅回收 `text -4B`，`bss` 不变
- 完整 `239` perf 场景里没有任何 `>=10%` 回退或改善，最大绝对波动约 `0.75%`
- `TEXT_ROTATE_GRADIENT` / `EXTERN_TEXT_ROTATE` 与基础 `RECTANGLE / CIRCLE / ROUND_RECTANGLE / TEXT / IMAGE_565 / EXTERN_IMAGE_565 / IMAGE_RESIZE_565 / EXTERN_IMAGE_RESIZE_565` 都在噪声内
- runtime `241` 帧和 `HelloUnitTest 688/688` 仍保持等价

`EGUI_CONFIG_TEXT_TRANSFORM_LAYOUT_LINE_INDEX_16BIT` 本轮也已完成定向复测，结论为：
- 开启后 `HelloPerformance text 2055380 -> 2055480`，反而 `text +100B`，`bss` 不变
- 完整 `239` perf 场景里同样没有任何 `>=10%` 回退或改善，最大绝对波动约 `1.71%`，来自 `TEXT_ROTATE_QUARTER`
- 基础 `RECTANGLE / CIRCLE / ROUND_RECTANGLE / TEXT / IMAGE_565 / EXTERN_IMAGE_565 / IMAGE_RESIZE_565 / EXTERN_IMAGE_RESIZE_565` 仍是 `+0.0%`
- `TEXT_ROTATE -0.041%`、`TEXT_ROTATE_GRADIENT -0.039%`、`EXTERN_TEXT_ROTATE +0.0%`
- runtime `241` 帧和 `HelloUnitTest 688/688` 仍保持等价

因此当前更合理的定位是：这两颗宏继续保留为可选 low-RAM transient-heap 实验值，而不是 `HelloPerformance` 的默认 shipped 配置。

---

### 20. `EGUI_CONFIG_TEXT_TRANSFORM_PREPARE_CACHE_ENABLE`

**位置：** [src/core/egui_canvas_transform.c](../../../src/core/egui_canvas_transform.c)

| 属性 | 值 |
|------|---|
| 默认值 | `1` |
| HelloPerformance | `1`（同默认，**已保留**） |
| RAM 类型 | BSS |
| 大小 | ~60 B BSS |

**机制：**
预计算 text transform 的逆仿射矩阵（sin/cos × scale 混合）及旋转后包围盒，一帧内多个 PFB tile 渲染同一段文字只计算一次，其余 tile 直接复用。禁用后每 tile 均需重新执行三角函数和矩阵求逆，当前 rotated-text 场景会出现稳定回归，60 B BSS 换取的性价比仍然很高，HelloPerformance 选择保留。

---

### 21. `EGUI_CONFIG_TEXT_TRANSFORM_DIM_CACHE_ENABLE`

**位置：** [src/core/egui_canvas_transform.c](../../../src/core/egui_canvas_transform.c)

| 属性 | 值 |
|------|---|
| 默认值 | `1` |
| HelloPerformance | `1`（同默认，**已保留**） |
| RAM 类型 | BSS（static 局部变量，本质 BSS） |
| 大小 | 12 B BSS |

**机制：**
`egui_canvas_draw_string_transform()` 内记录上一次调用时的 `(font, string)` 对及对应的宽高结果（4 个 static 局部变量，共 12 B）。同一帧内多个 tile 渲染同一字符串时，跳过 `get_str_size` 遍历。12 B BSS，性价比极高，HelloPerformance 保留。

---

## 汇总参考表

| # | 宏名（省略通用前缀） | 默认值 | HP 设定 | 类型 | HP 节省估算 |
|---|---|---|---|---|---|
| 1 | `EGUI_CONFIG_IMAGE_STD_ALPHA_OPAQUE_CACHE_SLOTS` | 4 | 当前默认 **4**（历史实验 **0**） | BSS | ~33 B（当前关闭回收 `text -156B, bss -40B`，但有 `8` 帧像素差） |
| 2 | `EGUI_CONFIG_IMAGE_DECODE_MAX_PIXEL_SIZE` | 当前默认 **2**（历史默认 **4**） | 当前默认 **2**（历史对比 **4**） | heap峰值 | 相比历史 `4` 约 ~7680 B（当前 `2 -> 4` 仅 `text +108B`，perf/runtime/unit 等价） |
| 3 | `EGUI_CONFIG_IMAGE_DECODE_OPAQUE_ALPHA_ROW_USE_ROW_CACHE` | 0 | 当前默认 **0**（历史实验 **1**） | heap+BSS | ~248 B（当前 `1` 仅额外拿到 `text -132B, bss -8B`，perf/runtime/unit 等价） |
| 4 | `EGUI_CONFIG_IMAGE_CODEC_TAIL_ROW_CACHE_ENABLE` | 0 | 历史实验值 **1**（当前默认 0） | heap峰值 | ~2304 B（当前需同时权衡 `text +9976B`） |
| 5 | `EGUI_CONFIG_IMAGE_QOI_INDEX_RGB565_CACHE_ENABLE` | 1 | 当前默认 **1**（历史实验 **0**） | heap | ~128 B（当前关闭只回收 `text -64B`） |
| 6 | `EGUI_CONFIG_IMAGE_QOI_COMPACT_RGB565_INDEX_ENABLE` | 0 | 历史实验值 **1**（当前默认 0） | heap | ~192 B（当前不建议启用） |
| 7 | `EGUI_CONFIG_IMAGE_QOI_ROW_INDEX_8BIT_ENABLE` | 当前默认 **0**（历史实验 **1**） | 当前默认 **0**（历史对比 **1**） | heap | 历史 `1` 可回收 ~2~4 B / 解码实例（当前 `0 -> 1` 仅 `text +12B`，perf/runtime/unit 等价） |
| ~~8~~ | ~~`EGUI_CONFIG_IMAGE_EXTERNAL_ROW_CACHE_SHARE_BUFFERS`~~ | — | — | — | **已移除，强制共享+heap** |
| ~~9~~ | ~~`EGUI_CONFIG_IMAGE_EXTERNAL_SHARED_CACHE_USE_CODEC_ROW_CACHE`~~ | — | — | — | **已移除** |
| 8 | `EGUI_CONFIG_IMAGE_STD_EXTERNAL_ROW_PERSISTENT_CACHE_ENABLE` | 当前默认 **1**（历史实验 **0**） | 当前默认 **1**（历史对比 **0**） | BSS | 历史 `0` 可回收 `bss -40B`（代价 `text +20B`，当前 perf/runtime/unit 等价） |
| 9 | `EGUI_CONFIG_IMAGE_TRANSFORM_EXTERNAL_ROW_PERSISTENT_CACHE_ENABLE` | 当前默认 **1**（历史实验 **0**） | 当前默认 **1**（历史对比 **0**） | BSS | 历史 `0` 可回收 `text -24B, bss -56B`（当前 perf/runtime/unit 等价） |
| 10 | `EGUI_CONFIG_IMAGE_RLE_EXTERNAL_CACHE_WINDOW_SIZE` | 1024 | 当前默认 **1024**（历史实验 **64**） | BSS/frame heap | ~960 B（当前 `64` 可回收 `bss -1024B`，但 external RLE 场景回退 `+17.5% ~ +21.8%`） |
| 11 | `EGUI_CONFIG_IMAGE_RLE_EXTERNAL_WINDOW_PERSISTENT_CACHE_ENABLE` | 1 | 当前默认 **1**（历史实验 **0**） | BSS -> frame heap | ~1040 B（当前 `0` 回收 `bss -1040B`，但 `text +816B` 且 RLE 场景约 `+2.7% ~ +6.2%`） |
| 12 | `EGUI_CONFIG_FONT_STD_CODE_LOOKUP_CACHE_ASCII_COMPACT` | 当前默认 **0**（历史实验 **1**） | 当前默认 **0**（历史对比 **1**） | data | 历史 `1` 仅回收 `data -20B`，但会带来 `text +200B`；当前 perf/runtime/unit 等价 |
| 13 | `EGUI_CONFIG_FONT_STD_ASCII_LOOKUP_CACHE_ENABLE` | 当前默认 **0**（历史实验 **1**） | 当前默认 **0**（历史对比 **1**） | heap+BSS | 当前 `0` 可省约 ~140 B heap + 8 B BSS；历史 `1` 需额外付出 `text +1392B`，但文本场景可见 `11%` 级改善 |
| 14 | `EGUI_CONFIG_FONT_STD_ASCII_LOOKUP_INDEX_8BIT` | 当前默认 **0**（历史实验 **1**） | 当前默认 **0**（历史对比 **1**） | heap | 历史 `1` 理论上可回收约 ~128 B heap†；当前静态 size/perf/runtime/unit 全等 |
| 15 | `EGUI_CONFIG_FONT_STD_LINE_CACHE_ENABLE` | 当前默认 **0**（历史实验 **1**） | 当前默认 **0**（历史对比 **1**） | heap | 当前 `0` 可省约 ~164 B heap；历史 `1` 需额外付出 `text +488B`，当前 perf 仅见 `+3.8% ~ +5.7%` 级文本矩形波动 |
| 16 | `EGUI_CONFIG_FONT_STD_DRAW_PREFIX_CACHE_MAX_GLYPHS` | 当前默认 **0**（历史实验 **64**） | 当前默认 **0**（与宏17联动） | BSS | 当前 `0/0` 相比历史 `64/2` 可省 `text -1816B, bss -2616B`；但 `64/2` 同时带来 `TEXT +18.8%`、`EXTERN_TEXT +15.7%`，又改善 `TEXT_RECT / EXTERN_TEXT_RECT / TEXT_GRADIENT / TEXT_ROTATE_NONE` |
| 17 | `EGUI_CONFIG_FONT_STD_DRAW_PREFIX_CACHE_SLOTS` | 当前默认 **0**（历史实验 **2**） | 当前默认 **0**（与宏16联动） | BSS | 必须与宏16一起回开到 `64/2` 才会实际启用 draw-prefix cache |
| 18 | `EGUI_CONFIG_TEXT_TRANSFORM_LAYOUT_PIXEL_INDEX_16BIT` | 0 | 当前默认 **0**（历史实验 **1**） | heap transient | ~2N B（当前 `1` 仅 `text -4B`，perf/runtime/unit 等价） |
| 19 | `EGUI_CONFIG_TEXT_TRANSFORM_LAYOUT_LINE_INDEX_16BIT` | 0 | 当前默认 **0**（历史实验 **1**） | heap transient | ~2N B（当前 `1` 会 `text +100B`，perf/runtime/unit 等价） |
| 20 | `EGUI_CONFIG_TEXT_TRANSFORM_PREPARE_CACHE_ENABLE` | 1 | 1（保留）| BSS | 0（已开）|
| 21 | `EGUI_CONFIG_TEXT_TRANSFORM_DIM_CACHE_ENABLE` | 1 | 1（保留）| BSS | 0（已开）|

> **†** 宏 14 需搭配 `ASCII_LOOKUP_CACHE_ENABLE=1` 才有实际作用，HelloPerformance 禁用了 cache，故此宏在 HP 中无效，列入是为通用参考。

---

## 使用建议

### 激进低 RAM 场景（类 HelloPerformance）

建议优先启用那些已经验证为功能等价或可接受条件实验的项；其中宏1（alpha opaque cache）当前不再默认推荐关闭，因为 `4 -> 0` 虽能回收 `text -156B, bss -40B`，但会带来 `8` 帧 runtime 像素差。其余如 compress index（宏5/6）、禁用持久元数据全局 slot（宏8/9/11=0）、缩小 RLE window（宏10=64）、禁用 prefix 缓存（宏16/17=0）、禁用 ASCII/line 缓存（宏13/15=0）仍需结合对应小节单独评估。外部行缓存共享现已强制启用，heap 分配按需进行。

### 常规 UI 应用（label/button 等）

若是常规 UI 应用并且更看重静态文本吞吐，可单独评估重新开启 draw-prefix cache、ASCII lookup cache、line cache 这三类字体缓存；它们的历史配置分别是 `64/2`、`1`、`1`，但当前主线 shipped defaults 已统一下调到更省 RAM 的口径，需要按各自 current-mainline 复验结果决定是否回开。图像宏按实际 codec 支持情况配置。

### 注意事项

- 宏 3（OPAQUE_ALPHA_ROW_USE_ROW_CACHE）依赖 `IMAGE_CODEC_ROW_CACHE_ENABLE=1`。
- 宏 20/21（PREPARE_CACHE、DIM_CACHE）是文字旋转性能的关键，禁用会导致每 tile 重计算仿射/宽高，**不建议关闭**。
