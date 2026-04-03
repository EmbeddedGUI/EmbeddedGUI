# 低 RAM 配置宏说明文档

本文档梳理 `HelloPerformance` 等低 RAM 场景中使用的细粒度配置宏，说明每个宏的作用机制、默认值、及在 HelloPerformance 下的设定与 RAM 节省效果。

所有宏均可在应用的 `app_egui_config.h` 中覆盖，不影响其他应用的默认行为。

---

## 一、图像解码相关宏

### 1. `EGUI_CONFIG_IMAGE_STD_ALPHA_OPAQUE_CACHE_SLOTS`

**位置：** [src/image/egui_image_std.c](../../../src/image/egui_image_std.c)

| 属性 | 值 |
|------|---|
| 默认值 | `4` |
| HelloPerformance | `0` |
| RAM 类型 | BSS |
| HP 节省 | ~33 B |

**机制：**
缓存"某张 RGB565+alpha 图片的 alpha 通道是否全为不透明"的探测结果，跨帧复用以跳过逐行扫描，走更快的纯 RGB565 合成路径。SLOTS 槽数控制最多缓存多少张图片的结论，采用轮转淘汰。

设为 `0` 时整个 cache 数组不编译，每次绘制均重新扫描。HelloPerformance 的 benchmark 场景会大量切换图片，缓存命中率不高，故直接禁用。

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
| 默认值 | `4`（支持 RGBA32） |
| HelloPerformance | `2`（仅 RGB565） |
| RAM 类型 | heap（动态峰值） |
| HP 节省 | ~7680 B 峰值堆 |

**机制：**
决定 codec 行缓存和单行 pixel scratch 的最大字节上界（PFB 行宽度 × 屏幕宽度 × 本宏）。HelloPerformance 的压缩图像资产均为纯 RGB565，不含 RGBA32，因此将上界从 4 收窄到 2，峰值堆占用减半。

**公式：**
```
codec_row_cache_pixel_max = PFB_HEIGHT × SCREEN_WIDTH × EGUI_CONFIG_IMAGE_DECODE_MAX_PIXEL_SIZE
默认(PFB=30×30): 30 × 240 × 4 = 28,800 B
HelloPerf(PFB=16×16): 16 × 240 × 2 = 7,680 B
```

---

### 3. `EGUI_CONFIG_IMAGE_DECODE_OPAQUE_ALPHA_ROW_USE_ROW_CACHE`

**位置：** [src/core/egui_config_default.h](../../../src/core/egui_config_default.h)，使用处 [src/image/egui_image_decode_utils.c](../../../src/image/egui_image_decode_utils.c)

| 属性 | 值 |
|------|---|
| 默认值 | `0` |
| HelloPerformance | `1` |
| RAM 类型 | heap（节省独立分配） |
| HP 节省 | ~248 B（240B data + 8B 指针） |
| 依赖 | 需 `IMAGE_CODEC_ROW_CACHE_ENABLE=1` |

**机制：**
带 mask 的不透明 RGB565 图绘制时需要一行"全 255 alpha"参考缓冲用于合成回退。设为 `0` 时单独在 heap 上分配并跨帧持久；设为 `1` 时直接复用已存在的 codec row cache 的 alpha 缓冲头部，省去独立的 240 B 持久堆分配。

---

### 4. `EGUI_CONFIG_IMAGE_CODEC_TAIL_ROW_CACHE_ENABLE`

**位置：** [src/image/egui_image_qoi.c](../../../src/image/egui_image_qoi.c)，[src/image/egui_image_rle.c](../../../src/image/egui_image_rle.c)

| 属性 | 值 |
|------|---|
| 默认值 | `0`（全宽 row-band 模式） |
| HelloPerformance | `1` |
| RAM 类型 | heap（峰值减小） |
| HP 节省 | ~2304 B 峰值堆（α 场景） |

**机制：**
标准 row-band 模式：第一个 PFB tile 解码全图宽的当前行带并缓存，后续水平 tile 直接读缓存。Tail 模式：第一个 PFB tile 的像素直接从 scratch 流式解码并渲染，只把"后续 tile 尚未用到的尾列"写入 row cache。

在 240px 宽、48px PFB 步宽的场景下，第一 tile 占 48 列，尾部 192 列进入缓存，节省首 tile 的 `16×48×3 = 2304 B` 峰值堆分配。

---

### 5. `EGUI_CONFIG_IMAGE_QOI_INDEX_RGB565_CACHE_ENABLE`

**位置：** [src/image/egui_image_qoi.c](../../../src/image/egui_image_qoi.c)

| 属性 | 值 |
|------|---|
| 默认值 | `1`（预转换缓存） |
| HelloPerformance | `0` |
| RAM 类型 | heap（QOI decode state 内部） |
| HP 节省 | ~128 B / 每个活跃解码实例 |

**机制：**
QOI 解码器维护 64 个历史色彩槽（RGBA index，256 B）。启用时额外维护对应的 RGB565 预转换版本（128 B），INDEX 操作命中时无需重新做 RGB888→RGB565 转换；禁用时节省 128 B heap，每次 INDEX 命中时实时计算颜色转换（代价极小）。

---

### 6. `EGUI_CONFIG_IMAGE_QOI_COMPACT_RGB565_INDEX_ENABLE`

**位置：** [src/image/egui_image_qoi.c](../../../src/image/egui_image_qoi.c)

| 属性 | 值 |
|------|---|
| 默认值 | `0` |
| HelloPerformance | `1` |
| RAM 类型 | heap（QOI decode state） |
| HP 节省 | ~192 B / 每个活跃解码实例 |

**机制：**
将 QOI 的 64 项颜色索引表从标准 `uint32_t index_rgba[64]`（256 B）替换为紧凑版 `uint16_t index_rgb565[64]` + `uint8_t index_aux[64]`（共 192 B）。专为纯 RGB565 图源设计，辅助字节存放 alpha 或速度偏置信息，重建 RGB888 时从 RGB565 规范扩展推算，与 `RGB565_CACHE=0` 联用。

对比默认（compact=0, rgb565_cache=1）的 384 B，HelloPerformance 的 compact 配置仅占 192 B，净省 192 B。

---

### 7. `EGUI_CONFIG_IMAGE_QOI_ROW_INDEX_8BIT_ENABLE`

**位置：** [src/image/egui_image_qoi.c](../../../src/image/egui_image_qoi.c)

| 属性 | 值 |
|------|---|
| 默认值 | `0`（uint16_t，支持 ≤65535 行） |
| HelloPerformance | `1`（uint8_t，限 ≤255 行） |
| RAM 类型 | heap（decode state 内字段） |
| HP 节省 | ~2~4 B / 每个解码实例 |

**机制：**
QOI 解码状态结构体中 `current_row`（行游标）的整数宽度。降至 uint8_t 要求图片高度不超过 255 行。HelloPerformance 的 QOI 资源最高 240 行，满足条件。

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
| 默认值 | `1` |
| HelloPerformance | `0` |
| RAM 类型 | BSS |
| HP 节省 | ~48 B BSS |

**机制：**
外部 std 图像行块缓存的元数据（chunk 起始行、行数、行字节数、图片指针等）是否存放在全局 BSS。设为 `1` 跨 tile 保留行位置信息，减少 semihosting I/O 次数；设为 `0` 元数据转为调用栈局部变量，BSS 消去，代价是每次 draw 调用重建（HP 场景影响不大）。

---

### 9. `EGUI_CONFIG_IMAGE_TRANSFORM_EXTERNAL_ROW_PERSISTENT_CACHE_ENABLE`

**位置：** [src/core/egui_canvas_transform.c](../../../src/core/egui_canvas_transform.c)

| 属性 | 值 |
|------|---|
| 默认值 | `1` |
| HelloPerformance | `0` |
| RAM 类型 | BSS |
| HP 节省 | ~80~100 B BSS |

**机制：**
与宏 8 对称，负责 transform（rotate/scale）路径的外部图片行缓存元数据。默认两个全局 slot 各持行块状态；设为 `0` 后 slot 嵌入各 draw 调用的栈局部 `image_transform_external_source_t`，BSS 释放。

---

## 三、RLE 外部资源 I/O 窗口宏

### 10. `EGUI_CONFIG_IMAGE_RLE_EXTERNAL_CACHE_WINDOW_SIZE`

**位置：** [src/image/egui_image_rle.c](../../../src/image/egui_image_rle.c)

| 属性 | 值 |
|------|---|
| 默认值 | `1024` |
| HelloPerformance | `64` |
| RAM 类型 | BSS 或 stack（取决于宏13） |
| HP 节省 | ~960 B |

**机制：**
RLE 外部资源解码时，控制字节（操作码+长度字段）具有强局部性，缓存 `WINDOW_SIZE` 字节可大量减少 semihosting I/O 调用。像素字面量行（240px × 2B = 480 B）超过窗口大小时自动走直接 load，正确性不受影响。HelloPerformance 设为 64 B 满足控制流缓存需求同时节省 960 B。

---

### 11. `EGUI_CONFIG_IMAGE_RLE_EXTERNAL_WINDOW_PERSISTENT_CACHE_ENABLE`

**位置：** [src/image/egui_image_rle.c](../../../src/image/egui_image_rle.c)

| 属性 | 值 |
|------|---|
| 默认值 | `1`（全局 BSS） |
| HelloPerformance | `0`（栈上） |
| RAM 类型 | BSS |
| HP 节省 | ~68 B BSS（HP WINDOW_SIZE=64 时）；默认参数下 ~1034 B |

**机制：**
控制 `egui_image_rle_external_window_cache_t`（含 window 数组）是否放在全局 BSS 跨调用保留。设为 `0` 后，结构体移至 draw 调用栈上，每次调用初始化，但 window 命中率不跨 tile，I/O 次数略升。与宏 10 联动：宏10 决定 window 数组大小，宏11 决定其放在哪里。

---

## 四、字体缓存宏

### 12. `EGUI_CONFIG_FONT_STD_CODE_LOOKUP_CACHE_ASCII_COMPACT`

**位置：** [src/font/egui_font_std.c](../../../src/font/egui_font_std.c)

| 属性 | 值 |
|------|---|
| 默认值 | `0`（uint16_t/uint32_t 字段） |
| HelloPerformance | `1`（uint8_t 紧凑） |
| RAM 类型 | BSS |
| HP 节省 | ~20 B |

**机制：**
字体 code lookup 加速缓存（`g_font_std_code_lookup_cache`），记录上次命中的 ASCII 码及其字形表下标，加速有序 code 数组的二分查找。Compact 模式将内部字段压缩为 uint8_t，适用于纯 ASCII（code ≤ 255）字体，每 slot 从约 32 B 压缩到约 12 B。

---

### 13. `EGUI_CONFIG_FONT_STD_ASCII_LOOKUP_CACHE_ENABLE`

**位置：** [src/font/egui_font_std.c](../../../src/font/egui_font_std.c)

| 属性 | 值 |
|------|---|
| 默认值 | `1` |
| HelloPerformance | `0` |
| RAM 类型 | heap + BSS（指针） |
| HP 节省 | ~140 B heap + 8 B BSS |

**机制：**
为 ASCII(0~127) 字符预建直查表（`ascii_index[128]`），首次分配后跨帧复用，将 ASCII 字形查找从 O(log n) 降至 O(1)。设为 `0` 后所有 ASCII 字符走二分查找，对 HelloPerformance 只有简单 benchmark 字符串的场景可接受。可与宏 14 联用控制 index 宽度。

---

### 14. `EGUI_CONFIG_FONT_STD_ASCII_LOOKUP_INDEX_8BIT`

**位置：** [src/font/egui_font_std.c](../../../src/font/egui_font_std.c)

| 属性 | 值 |
|------|---|
| 默认值 | `0`（uint16_t） |
| HelloPerformance | `1`（uint8_t） |
| RAM 类型 | heap（ASCII cache 内部） |
| HP 节省 | ~128 B（需同时开启 ASCII cache） |

**机制：**
`ascii_index[128]` 数组的元素宽度。uint8_t 上限 255 个字形（0xFF 为无效标记），对小字体子集安全。仅当 `ASCII_LOOKUP_CACHE_ENABLE=1` 时有效；HelloPerformance 已设 cache=0，此宏对 HP 无实际作用，但在一般应用中需与 cache 联用。

---

### 15. `EGUI_CONFIG_FONT_STD_LINE_CACHE_ENABLE`

**位置：** [src/font/egui_font_std.c](../../../src/font/egui_font_std.c)

| 属性 | 值 |
|------|---|
| 默认值 | `1` |
| HelloPerformance | `0` |
| RAM 类型 | heap + BSS |
| HP 节省 | ~164 B heap |

**机制：**
多行文本行分割缓存，避免每次 `get_str_size` 或绘制时重新扫描 `\n`。结构体中含 `lines[MAX_LINES]` 指针数组，缓存最近 SLOTS 个字符串的分割结果。HelloPerformance 的 benchmark 字符串固定，但每帧只绘制一次，分割复用价值有限，设为 `0` 节省堆。

---

### 16 + 17. `EGUI_CONFIG_FONT_STD_DRAW_PREFIX_CACHE_MAX_GLYPHS` / `EGUI_CONFIG_FONT_STD_DRAW_PREFIX_CACHE_SLOTS`

**位置：** [src/font/egui_font_std.c](../../../src/font/egui_font_std.c)

| 属性 | 值 |
|------|---|
| 默认值 | MAX_GLYPHS=`64`，SLOTS=`2` |
| HelloPerformance | MAX_GLYPHS=`0`，SLOTS=`0`（均为 0 → 禁用） |
| RAM 类型 | BSS |
| HP 节省 | ~2612 B BSS |

**机制：**
Draw-prefix cache 缓存每行文字的字形布局元数据（每个字符的 x 坐标、bitmap box、advance、字形下标等）。当同一字符串在多帧多处重复绘制时，跳过字符串扫描和字形 lookup，直接复用缓存位置信息渲染像素，是常规 UI 场景（label/title 等静态文字）的主要加速手段。

每个字形描述约 20 B，默认 2 slots × 64 glyphs × ~20 B ≈ **2612 B 静态 BSS**。HelloPerformance 的场景每帧全量刷新，cache 命中率为 0，禁用后节省全部 BSS。

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
| 默认值 | `0`（uint32_t / int，4 B/字形） |
| HelloPerformance | `1`（uint16_t，2 B/字形） |
| RAM 类型 | heap（transient，每帧分配释放） |
| HP 节省 | 4N B（N = 字形数，transient） |

**机制：**
Text transform 先将所有字形的像素偏移和行索引打包到 heap layout 数组，再按 PFB tile 顺序渲染。这两个宏将数组元素从 32-bit 降为 16-bit，分别限制最大像素偏移（≤ 65535 B）和最大字形索引（≤ 65535）。对短 benchmark 字符串（≤ 255 字形）和 26pt 以下字体完全够用。layout 数组是 transient heap（每帧分配、每帧释放），短字符串下节省约数十 B。

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
| 1 | `EGUI_CONFIG_IMAGE_STD_ALPHA_OPAQUE_CACHE_SLOTS` | 4 | **0** | BSS | ~33 B |
| 2 | `EGUI_CONFIG_IMAGE_DECODE_MAX_PIXEL_SIZE` | 4 | **2** | heap峰值 | ~7680 B |
| 3 | `EGUI_CONFIG_IMAGE_DECODE_OPAQUE_ALPHA_ROW_USE_ROW_CACHE` | 0 | **1** | heap+BSS | ~248 B |
| 4 | `EGUI_CONFIG_IMAGE_CODEC_TAIL_ROW_CACHE_ENABLE` | 0 | **1** | heap峰值 | ~2304 B |
| 5 | `EGUI_CONFIG_IMAGE_QOI_INDEX_RGB565_CACHE_ENABLE` | 1 | **0** | heap | ~128 B |
| 6 | `EGUI_CONFIG_IMAGE_QOI_COMPACT_RGB565_INDEX_ENABLE` | 0 | **1** | heap | ~192 B |
| 7 | `EGUI_CONFIG_IMAGE_QOI_ROW_INDEX_8BIT_ENABLE` | 0 | **1** | heap | ~2 B |
| ~~8~~ | ~~`EGUI_CONFIG_IMAGE_EXTERNAL_ROW_CACHE_SHARE_BUFFERS`~~ | — | — | — | **已移除，强制共享+heap** |
| ~~9~~ | ~~`EGUI_CONFIG_IMAGE_EXTERNAL_SHARED_CACHE_USE_CODEC_ROW_CACHE`~~ | — | — | — | **已移除** |
| 8 | `EGUI_CONFIG_IMAGE_STD_EXTERNAL_ROW_PERSISTENT_CACHE_ENABLE` | 1 | **0** | BSS | ~48 B |
| 9 | `EGUI_CONFIG_IMAGE_TRANSFORM_EXTERNAL_ROW_PERSISTENT_CACHE_ENABLE` | 1 | **0** | BSS | ~80 B |
| 10 | `EGUI_CONFIG_IMAGE_RLE_EXTERNAL_CACHE_WINDOW_SIZE` | 1024 | **64** | BSS/stack | ~960 B |
| 11 | `EGUI_CONFIG_IMAGE_RLE_EXTERNAL_WINDOW_PERSISTENT_CACHE_ENABLE` | 1 | **0** | BSS | ~68 B（HP）|
| 12 | `EGUI_CONFIG_FONT_STD_CODE_LOOKUP_CACHE_ASCII_COMPACT` | 0 | **1** | BSS | ~20 B |
| 13 | `EGUI_CONFIG_FONT_STD_ASCII_LOOKUP_CACHE_ENABLE` | 1 | **0** | heap+BSS | ~140 B |
| 14 | `EGUI_CONFIG_FONT_STD_ASCII_LOOKUP_INDEX_8BIT` | 0 | **1** | heap | ~128 B† |
| 15 | `EGUI_CONFIG_FONT_STD_LINE_CACHE_ENABLE` | 1 | **0** | heap | ~164 B |
| 16 | `EGUI_CONFIG_FONT_STD_DRAW_PREFIX_CACHE_MAX_GLYPHS` | 64 | **0** | BSS | ~2612 B |
| 17 | `EGUI_CONFIG_FONT_STD_DRAW_PREFIX_CACHE_SLOTS` | 2 | **0** | BSS | 与宏16联动 |
| 18 | `EGUI_CONFIG_TEXT_TRANSFORM_LAYOUT_PIXEL_INDEX_16BIT` | 0 | **1** | heap transient | ~2N B |
| 19 | `EGUI_CONFIG_TEXT_TRANSFORM_LAYOUT_LINE_INDEX_16BIT` | 0 | **1** | heap transient | ~2N B |
| 20 | `EGUI_CONFIG_TEXT_TRANSFORM_PREPARE_CACHE_ENABLE` | 1 | 1（保留）| BSS | 0（已开）|
| 21 | `EGUI_CONFIG_TEXT_TRANSFORM_DIM_CACHE_ENABLE` | 1 | 1（保留）| BSS | 0（已开）|

> **†** 宏 14 需搭配 `ASCII_LOOKUP_CACHE_ENABLE=1` 才有实际作用，HelloPerformance 禁用了 cache，故此宏在 HP 中无效，列入是为通用参考。

---

## 使用建议

### 激进低 RAM 场景（类 HelloPerformance）

建议全套启用：禁用 alpha opaque cache（宏1=0）、compress index（宏5/6）、禁用所有持久元数据全局 slot（宏8/9/11=0）、缩小 RLE window（宏10=64）、禁用 prefix 缓存（宏16/17=0）、禁用 ASCII/line 缓存（宏13/15=0）。外部行缓存共享现已强制启用，heap 分配按需进行。

### 常规 UI 应用（label/button 等）

建议保留 draw-prefix cache（宏16/17 保持默认 64/2）、保留 ASCII lookup cache（宏13=1）、保留 line cache（宏15=1），这三项对静态文字 UI 有明显渲染加速。图像宏按实际 codec 支持情况配置。

### 注意事项

- 宏 3（OPAQUE_ALPHA_ROW_USE_ROW_CACHE）依赖 `IMAGE_CODEC_ROW_CACHE_ENABLE=1`。
- 宏 20/21（PREPARE_CACHE、DIM_CACHE）是文字旋转性能的关键，禁用会导致每 tile 重计算仿射/宽高，**不建议关闭**。
