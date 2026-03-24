# 图片压缩 Codec 系统设计

**日期**：2026-03-23  
**状态**：设计评审  

## 1. 背景与目标

### 1.1 现状

EmbeddedGUI 当前图片存储方式为原始像素数据（RGB565/RGB32/GRAY8 + 可选 alpha 通道），无任何压缩。MP4 帧序列动画也是将每帧预转成原始图片数组。这在以下场景造成痛点：

- **ROM/Flash 占用大**：一张 240×240 RGB565 图片即需 115KB，帧序列动画轻松 MB 级
- **外部存储读取慢**：从外部 Flash/SD 卡加载原始数据，传输带宽成为瓶颈

### 1.2 目标

- 引入图片压缩机制，减小 ROM/Flash 占用并优化外部资源读取速度
- 支持从极低端（RAM <8KB, CPU ~48MHz）到中端（RAM >64KB, CPU >200MHz）的可伸缩配置
- 可插拔 codec 架构，内置 QOI + RLE，用户可扩展（如 LZ4）
- MP4 帧序列第一期支持逐帧独立压缩，后续可扩展帧间差分
- 构建时自动压缩，通过 `app_resource_config.json` 配置
- 完全向后兼容：新增图片类型，不改动现有 `egui_image_std_t`

### 1.3 算法授权

| 算法 | 授权 | 风险 |
|------|------|------|
| QOI | MIT | 无风险，允许商用、修改、闭源 |
| RLE | 基础算法，无专利 | 无风险 |
| LZ4 | BSD 2-Clause | 无风险，允许商用、修改、闭源 |

三者均与项目 Apache-2.0 许可完全兼容。

## 2. 整体架构与类层次

### 2.1 类型继承关系

```
egui_image_t                          ← 基类（不改动）
  ├── egui_image_std_t                ← 现有原始像素（不改动）
  ├── egui_image_qoi_t                ← 新增：QOI 压缩图片
  └── egui_image_rle_t                ← 新增：RLE 压缩图片
      (用户可扩展 egui_image_lz4_t 等)
```

每种压缩类型是独立的图片子类，各自实现 `egui_image_api_t` 虚函数表。与现有 OOP 模式完全一致。

### 2.2 压缩图片类型结构

以 QOI 为例：

```c
typedef struct egui_image_qoi_info
{
    const uint8_t *data_buf;       // QOI 压缩数据流（INTERNAL 时指向 ROM 数组）
                                   // EXTERNAL 时为外部资源 ID
    uint32_t data_size;            // 压缩数据长度（字节）
    uint32_t decompressed_size;    // 解压后原始数据大小（用于校验）
    uint8_t data_type;             // 解压后像素格式：RGB565 / RGB32
    uint8_t alpha_type;            // 解压后 alpha 位深
    uint8_t res_type;              // INTERNAL / EXTERNAL（针对压缩数据）
    uint16_t width;
    uint16_t height;
} egui_image_qoi_info_t;

typedef struct egui_image_qoi
{
    egui_image_t base;             // 继承基类
} egui_image_qoi_t;
```

RLE 同理，`egui_image_rle_info_t` + `egui_image_rle_t`。

### 2.3 虚函数表

```c
const egui_image_api_t egui_image_qoi_t_api_table = {
    .get_point        = egui_image_qoi_get_point,
    .get_point_resize = egui_image_qoi_get_point_resize,
    .draw_image       = egui_image_qoi_draw_image,
    .draw_image_resize = egui_image_qoi_draw_image_resize,
};
```

### 2.4 资源声明（生成代码）

```c
static const uint8_t g_image_logo_qoi_data[] = { /* QOI 压缩字节流 */ };

static const egui_image_qoi_info_t g_image_logo_qoi_info = {
    .data_buf   = g_image_logo_qoi_data,
    .data_size  = sizeof(g_image_logo_qoi_data),
    .data_type  = EGUI_IMAGE_DATA_TYPE_RGB565,
    .alpha_type = EGUI_IMAGE_ALPHA_TYPE_8,
    .res_type   = EGUI_RESOURCE_TYPE_INTERNAL,
    .width      = 128,
    .height     = 96,
};

EGUI_IMAGE_SUB_DEFINE_CONST(egui_image_qoi_t, egui_res_image_logo_rgb565_8,
                            (const void *)&g_image_logo_qoi_info);
```

### 2.5 使用方式（对用户透明）

```c
// 用户代码完全相同，不需要关心是否压缩
egui_canvas_draw_image((const egui_image_t *)&egui_res_image_logo_rgb565_8, x, y);
```

### 2.6 编译裁剪宏

```c
#define EGUI_CONFIG_IMAGE_CODEC_QOI_ENABLE  1  // 按需开启
#define EGUI_CONFIG_IMAGE_CODEC_RLE_ENABLE  1
```

不开启时，对应 `.c` 不参与编译，零额外 ROM。

## 3. 解码管线与渲染流程

### 3.1 核心约束

嵌入式不能全图解码。一张 240×240 RGB565 图片解压后需要 115KB RAM。必须做**逐行解码**。

### 3.2 PFB 瓦片与顺序解码的协作机制

PFB 渲染系统对每个脏区域按瓦片遍历，同一张图片的 `draw_image()` 会被多次调用（每个重叠瓦片一次）。对于 QOI 这类顺序解码算法，不能每次从头解码。

**解决方案：持久化解码上下文 + 行缓存**

每种压缩图片类型维护一个静态解码上下文，在同一帧的多个 PFB 瓦片调用间持久化：

```
帧开始:
  重置解码上下文（data_pos=0, current_row=0）

对每个 PFB 瓦片:
  1. 计算该瓦片与图片的重叠行范围 [row_start, row_end]
  2. 如果 row_start < current_row → 需要重置解码器（回退场景，极少见）
  3. 如果 row_start == cached_row → 直接从行缓存取像素
  4. 否则 → 从 current_row 继续解码到 row_start（跳过中间行）
  5. 对重叠范围内每行：
     a. 解码一整行像素到行缓冲区（全宽解码）
     b. 从行缓冲区中截取 PFB 瓦片可见列，混合写入 PFB
     c. 更新 cached_row = current_row
  6. 保存解码状态供下一个瓦片使用
```

PFB 瓦片默认按光栅序（左→右，上→下）遍历，因此解码器只需单向前进，不会回退。行缓存确保同一行的多个水平瓦片不需要重复解码。

### 3.3 行缓冲区分配

```c
// 静态行缓冲区（编译时确定大小，零动态分配）
#define EGUI_IMAGE_DECODE_ROW_BUF_SIZE  (EGUI_CONFIG_IMAGE_DECODE_ROW_BUF_WIDTH * 4)

static uint8_t s_decode_row_buf[EGUI_IMAGE_DECODE_ROW_BUF_SIZE];
static uint8_t s_decode_alpha_buf[EGUI_CONFIG_IMAGE_DECODE_ROW_BUF_WIDTH];
```

- 全局共享，不同 codec 复用同一个缓冲区
- 最差情况 ~1.2KB（240 宽 RGB32 + alpha8），适合 <8KB RAM

### 3.4 QOI 解码器适配

QOI 是流式编解码，像素按从左到右、从上到下顺序线性排列：

- **顺序解码**：必须从头开始，不能随机跳行
- **PFB 友好**：PFB 按瓦片从左上到右下遍历，天然匹配 QOI 的顺序
- **行缓存优化**：解码到目标行时暂停，输出一行像素，继续下一行

```c
typedef struct egui_image_qoi_decode_state
{
    const egui_image_qoi_info_t *info; // 当前解码的图片
    uint32_t data_pos;              // 压缩数据读取位置
    uint16_t current_row;           // 当前已解码到的行号
    uint16_t cached_row;            // 行缓存中的行号（0xFFFF 表示无效）
    egui_color_int_t prev;          // QOI 上一像素值
    egui_color_int_t index[64];     // QOI 哈希表（128B for RGB565, 256B for RGB32）
    uint8_t run;                    // QOI RLE run 计数
    uint8_t frame_id;               // 帧序号，用于检测新帧重置
} egui_image_qoi_decode_state_t;
```

解码上下文为静态全局变量（同一时刻只有一张压缩图片在解码），在帧切换时通过 `frame_id` 检测并重置。

### 3.5 RLE 解码器适配

RLE 更简单，控制字节格式参考 LVGL：

```
bit7=1: literal, count = ctrl & 0x7F, 复制 count × blk_size 字节
bit7=0: repeat,  count = ctrl,       重复 blk_size 字节 count 次
```

### 3.6 渲染调用链

```
egui_canvas_draw_image(img, x, y)
  → img->api->draw_image(img, x, y)         // 虚函数分发
    → egui_image_qoi_draw_image(img, x, y)   // QOI 实现
      → 初始化 QOI 解码状态
      → 逐行解码到 s_decode_row_buf
      → 调用现有像素混合函数写入 PFB
```

解码后调用的像素混合函数与 `egui_image_std` 使用的完全相同，不重写渲染逻辑。

### 3.7 get_point() 与 resize 处理

**限制说明**：

- `get_point()` 对压缩图片可用但性能低——需从头顺序解码到目标行再取像素
- `get_point_resize()` 对压缩图片**不支持**，调用时返回错误码。需要 resize 的图片必须使用原始格式（`egui_image_std_t`）
- `draw_image_resize()` 同样不支持压缩图片，返回后不绘制任何内容

这是压缩图片的已知限制。压缩优化的目标是 `draw_image()`（原始尺寸绘制）路径。

### 3.8 GRAY8 格式说明

GRAY8 图片不支持 QOI 压缩（QOI 设计用于 RGB/RGBA）。GRAY8 图片可使用 RLE 压缩（blk_size=1，效果好）。用户在 `app_resource_config.json` 中为 GRAY8 图片指定 `"compress": "qoi"` 时，构建脚本将报错并提示改用 RLE 或 none。

## 4. 资源生成管线

### 4.1 用户配置

在 `app_resource_config.json` 中新增可选字段 `compress`：

```json
{
  "img": [
    {
      "file": "background.png",
      "format": "rgb565",
      "alpha": "0",
      "compress": "rle"
    },
    {
      "file": "icon.png",
      "format": "rgb565",
      "alpha": "4",
      "compress": "qoi"
    }
  ]
}
```

- `"compress"`: `"none"`（默认，等同省略）| `"qoi"` | `"rle"`
- 省略时走现有 `egui_image_std_t` 原始路径，完全向后兼容

### 4.2 img2c.py 改造

现有流程新增压缩分支：

```
PNG → 解析像素
  ├─ compress=none  → 现有路径，生成 egui_image_std_info_t
  ├─ compress=qoi   → 像素数据 → QOI 编码 → 生成 egui_image_qoi_info_t
  └─ compress=rle   → 像素数据 → RLE 编码 → 生成 egui_image_rle_info_t
```

QOI/RLE 编码器在 Python 脚本中实现（构建时运行），C 端只需要解码器。

### 4.3 MP4 帧序列压缩

MP4 帧提取和压缩已集成到 `app_resource_generate.py` 的 `generate_mp4_resource()` 中，通过 `app_resource_config.json` 的 `compress` 选项控制。每帧独立压缩，生成的帧数组结构不变：

```c
const egui_image_t *mp4_arr_anim[50] = {
    (const egui_image_t *)&egui_res_image_frame_0001,  // egui_image_qoi_t
    (const egui_image_t *)&egui_res_image_frame_0002,
    ...
};
```

MP4 widget 代码零修改。

### 4.4 压缩率报告

`img2c.py` 生成结束后输出压缩统计：

```
[IMG] background.png: 115200B → 38400B (QOI, -66.7%)
[IMG] icon.png:        4800B →  2100B (RLE, -56.3%)
Total: 120000B → 40500B (saved 79500B, -66.3%)
```

## 5. 文件组织与构建集成

### 5.1 新增源文件

```
src/image/
  ├── egui_image.h / .c              (不改动)
  ├── egui_image_std.h / .c          (不改动)
  ├── egui_image_qoi.h / .c          (新增：QOI 类型 + 解码器)
  ├── egui_image_rle.h / .c          (新增：RLE 类型 + 解码器)
  └── egui_image_decode_utils.h / .c (新增：共享行缓冲、像素混合辅助)

scripts/tools/
  ├── img2c.py                       (修改：增加 compress 分支)
  ├── img_codec_qoi.py               (新增：Python QOI 编码器)
  ├── img_codec_rle.py               (新增：Python RLE 编码器)
  └── app_resource_generate.py       (修改：MP4 帧提取已集成，增加 compress 选项)
```

### 5.2 构建模块（build.mk）

```makefile
EGUI_CODE_SRC += src/image/egui_image.c
EGUI_CODE_SRC += src/image/egui_image_std.c

ifeq ($(EGUI_CONFIG_IMAGE_CODEC_QOI_ENABLE), 1)
EGUI_CODE_SRC += src/image/egui_image_qoi.c
endif

ifeq ($(EGUI_CONFIG_IMAGE_CODEC_RLE_ENABLE), 1)
EGUI_CODE_SRC += src/image/egui_image_rle.c
endif
```

### 5.3 配置宏（egui_config_default.h）

```c
#ifndef EGUI_CONFIG_IMAGE_CODEC_QOI_ENABLE
#define EGUI_CONFIG_IMAGE_CODEC_QOI_ENABLE  0
#endif

#ifndef EGUI_CONFIG_IMAGE_CODEC_RLE_ENABLE
#define EGUI_CONFIG_IMAGE_CODEC_RLE_ENABLE  0
#endif

#ifndef EGUI_CONFIG_IMAGE_DECODE_ROW_BUF_WIDTH
#define EGUI_CONFIG_IMAGE_DECODE_ROW_BUF_WIDTH  EGUI_CONFIG_SCEEN_WIDTH
#endif
```

### 5.4 示例应用

新增 `example/HelloBasic/image_compressed/test.c`，展示 QOI/RLE 图片加载与渲染。

## 6. ROM/RAM 开销估算

### 6.1 RAM 开销

| 组件 | 大小（RGB565） | 大小（RGB32） | 说明 |
|------|---------------|--------------|------|
| 行缓冲区（像素） | 480B | 960B | 240 宽 × bpp |
| 行缓冲区（alpha） | 240B | 240B | 240 宽 × 8bit |
| QOI 解码状态 | ~150B | ~280B | 哈希表 128/256B + 上下文 |
| RLE 解码状态 | ~8B | ~8B | 位置指针 + 计数器 |
| **合计** | **~880B** | **~1490B** | 远低于 8KB 上限 |

### 6.2 ROM 开销（代码体积）

| 模块 | 估算 | 说明 |
|------|------|------|
| egui_image_qoi.c | ~2-3KB | QOI 解码器 + draw 实现 |
| egui_image_rle.c | ~1-2KB | RLE 解码器 + draw 实现 |
| egui_image_decode_utils.c | ~0.5-1KB | 共享行混合辅助 |
| **合计** | **~4-6KB** | 不开启时为 0 |

### 6.3 存储节省（典型场景）

| 资源类型 | 原始大小 | QOI 压缩后 | RLE 压缩后 |
|----------|----------|-----------|-----------|
| 全屏背景 240×240 RGB565（渐变） | 115KB | ~35-50KB | ~25-40KB |
| 图标 48×48 RGB565+A8（大面积透明） | 4.6KB | ~2-3KB | ~1.5-2.5KB |
| MP4 50帧 320×240（UI 动画） | 7.5MB | ~2.5-3.5MB | ~2-3MB |

### Codec 选择建议

| 场景 | 推荐 Codec | 原因 |
|------|-----------|------|
| 大面积纯色/渐变 UI 背景 | RLE | 纯色 run 压缩率极高 |
| 混合内容图片（图标、照片式素材） | QOI | 通用无损，比 RLE 稳定 |
| GRAY8 图片 | RLE | QOI 不支持 GRAY8 |
| 照片/噪声多的图片 | none | 压缩率低，不值得解码开销 |
| 帧序列动画 | QOI | 帧间内容变化大，QOI 更稳定 |

## 7. 已知限制

- `draw_image_resize()` 和 `get_point_resize()` 不支持压缩图片，需要 resize 的图片应使用原始格式
- GRAY8 像素格式仅支持 RLE 压缩，不支持 QOI
- 同一时刻只有一张压缩图片在解码（静态解码上下文），不支持嵌套/并发解码
- MP4 帧序列建议所有帧使用相同 codec，混用不同 codec 会增加每帧的状态重置开销

## 8. 后续扩展（不在本期范围）

- **帧间差分压缩**：MP4 只存首帧 + delta，进一步压缩帧序列
- **LZ4 codec**：用户参照 QOI/RLE 实现自行扩展
- **外部资源流式解压**：结合 external resource loader 实现边读边解
- **硬件加速解码**：为特定 MCU 的 DMA2D 提供加速路径

## 9. Python 编码器参考

- **QOI 编码器**：参考 [phoboslab/qoi](https://github.com/phoboslab/qoi) 的 `qoi.h` 规范实现 Python 编码器，格式已冻结无后续变更
- **RLE 编码器**：参考 LVGL `ref/lvgl/src/libs/rle/lv_rle.c` 的控制字节格式
- 两者均为纯 Python 实现，仅在构建时运行，不依赖外部二进制
