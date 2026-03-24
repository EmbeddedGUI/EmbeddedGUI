# 图片压缩 Codec 系统 实现计划

> **For agentic workers:** REQUIRED: Use superpowers:subagent-driven-development (if subagents available) or superpowers:executing-plans to implement this plan. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** 为 EmbeddedGUI 新增可插拔图片压缩解码系统，内置 QOI + RLE codec，支持构建时自动压缩，运行时逐行解码渲染。

**Architecture:** 新增 `egui_image_qoi_t` 和 `egui_image_rle_t` 图片子类型，各自实现 `egui_image_api_t` 虚函数表。共享行缓冲区和像素混合工具函数。Python 构建脚本增加编码器，`app_resource_config.json` 新增 `compress` 字段控制。

**Tech Stack:** C99（嵌入式端解码器），Python 3（构建端编码器），Make/CMake（条件编译）

**Spec:** `docs/superpowers/specs/2026-03-23-image-compression-codec-design.md`

---

## Chunk 1: 核心 C 层 — 解码工具与 RLE Codec

### Task 1: 配置宏定义

**Files:**
- Modify: `src/core/egui_config_default.h`

- [ ] **Step 1: 在 egui_config_default.h 中添加 codec 配置宏**

在现有 `EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_*` 宏块之后，添加：

```c
/* Image codec (compression) enable/disable */
#ifndef EGUI_CONFIG_IMAGE_CODEC_QOI_ENABLE
#define EGUI_CONFIG_IMAGE_CODEC_QOI_ENABLE 0
#endif

#ifndef EGUI_CONFIG_IMAGE_CODEC_RLE_ENABLE
#define EGUI_CONFIG_IMAGE_CODEC_RLE_ENABLE 0
#endif

/* Decode row buffer width (pixels), default = screen width */
#ifndef EGUI_CONFIG_IMAGE_DECODE_ROW_BUF_WIDTH
#define EGUI_CONFIG_IMAGE_DECODE_ROW_BUF_WIDTH EGUI_CONFIG_SCEEN_WIDTH
#endif
```

- [ ] **Step 2: 构建验证无回归**

Run: `make clean; make all APP=HelloSimple PORT=pc`
Expected: 编译成功，无 warning

- [ ] **Step 3: Commit**

```bash
git add src/core/egui_config_default.h
git commit -m "feat(image): add image codec config macros (QOI/RLE)"
```

---

### Task 2: 共享解码工具模块

**Files:**
- Create: `src/image/egui_image_decode_utils.h`
- Create: `src/image/egui_image_decode_utils.c`

- [ ] **Step 1: 创建 egui_image_decode_utils.h**

```c
#ifndef _EGUI_IMAGE_DECODE_UTILS_H_
#define _EGUI_IMAGE_DECODE_UTILS_H_

#include "egui_image.h"
#include "egui_image_std.h"

#ifdef __cplusplus
extern "C" {
#endif

// Shared row decode buffer (static, zero dynamic allocation)
// Pixel buffer: EGUI_CONFIG_IMAGE_DECODE_ROW_BUF_WIDTH * 4 bytes (worst case RGB32)
// Alpha buffer: EGUI_CONFIG_IMAGE_DECODE_ROW_BUF_WIDTH bytes
extern uint8_t egui_image_decode_row_pixel_buf[];
extern uint8_t egui_image_decode_row_alpha_buf[];

// Blend one decoded row into current PFB region
// data_type: EGUI_IMAGE_DATA_TYPE_RGB565 / RGB32
// alpha_type: EGUI_IMAGE_ALPHA_TYPE_1/2/4/8, or 0xFF if no alpha
// img_x, img_y: image top-left position on screen
// row: the image row number being blended
// pixel_buf: decoded pixel data for this full row
// alpha_buf: decoded alpha data for this full row (NULL if no alpha)
void egui_image_decode_blend_row(egui_dim_t img_x, egui_dim_t img_y, uint16_t row,
                                 uint16_t img_width, uint8_t data_type, uint8_t alpha_type,
                                 const uint8_t *pixel_buf, const uint8_t *alpha_buf);

#ifdef __cplusplus
}
#endif

#endif /* _EGUI_IMAGE_DECODE_UTILS_H_ */
```

- [ ] **Step 2: 创建 egui_image_decode_utils.c**

实现行缓冲区定义和行混合函数。行混合函数复用 `egui_canvas_draw_point_color()` 或直接写入 PFB（参考 `egui_image_std.c` 中 `egui_image_std_set_image_rgb565` 等的像素混合逻辑）。

关键实现点：
- 静态定义 `egui_image_decode_row_pixel_buf[EGUI_CONFIG_IMAGE_DECODE_ROW_BUF_WIDTH * 4]`
- 静态定义 `egui_image_decode_row_alpha_buf[EGUI_CONFIG_IMAGE_DECODE_ROW_BUF_WIDTH]`
- `egui_image_decode_blend_row()` 根据 `data_type` 分支，对 PFB 可见列范围逐像素混合
- 使用 `egui_canvas_get_pfb_region()` 获取当前 PFB 区域，计算重叠列范围
- 使用现有 `egui_color_alpha_mix()` 做 alpha 混合

- [ ] **Step 3: 构建验证**

Run: `make clean; make all APP=HelloSimple PORT=pc`
Expected: 编译成功（decode_utils 代码此时还未被引用，但文件可编译）

- [ ] **Step 4: Commit**

```bash
git add src/image/egui_image_decode_utils.h src/image/egui_image_decode_utils.c
git commit -m "feat(image): add shared decode utils (row buffer + blend)"
```

---

### Task 3: RLE Codec C 层实现

**Files:**
- Create: `src/image/egui_image_rle.h`
- Create: `src/image/egui_image_rle.c`

- [ ] **Step 1: 创建 egui_image_rle.h**

```c
#ifndef _EGUI_IMAGE_RLE_H_
#define _EGUI_IMAGE_RLE_H_

#include "egui_image.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
    const uint8_t *data_buf;       // RLE compressed data stream
    const uint8_t *alpha_buf;      // RLE compressed alpha stream (NULL if no alpha)
    uint32_t data_size;            // compressed data length in bytes
    uint32_t alpha_size;           // compressed alpha length in bytes
    uint32_t decompressed_size;    // decompressed data size for validation
    uint8_t data_type;             // EGUI_IMAGE_DATA_TYPE_RGB565 / RGB32 / GRAY8
    uint8_t alpha_type;            // EGUI_IMAGE_ALPHA_TYPE_1/2/4/8
    uint8_t res_type;              // EGUI_RESOURCE_TYPE_INTERNAL / EXTERNAL
    uint16_t width;
    uint16_t height;
} egui_image_rle_info_t;

typedef struct egui_image_rle egui_image_rle_t;
struct egui_image_rle
{
    egui_image_t base;
};

void egui_image_rle_init(egui_image_t *self, const void *res);

#ifdef __cplusplus
}
#endif

#endif /* _EGUI_IMAGE_RLE_H_ */
```

- [ ] **Step 2: 创建 egui_image_rle.c 核心解码函数**

实现 RLE 解码算法（参考 LVGL `ref/lvgl/src/libs/rle/lv_rle.c`）：

```c
// RLE control byte format:
// bit7=1: literal mode, count = ctrl & 0x7F, copy count*blk_size bytes
// bit7=0: repeat mode, count = ctrl, repeat next blk_size bytes count times
static uint32_t egui_image_rle_decompress_row(const uint8_t *src, uint32_t src_offset,
                                               uint8_t *dst, uint16_t pixels,
                                               uint8_t blk_size);
```

关键实现点：
- `egui_image_rle_decode_state_t`：静态全局，含 `data_pos`、`alpha_pos`、`current_row`、`cached_row`、指向当前 info 的指针
- `egui_image_rle_draw_image()`：逐行解码 + 调用 `egui_image_decode_blend_row()`
- `egui_image_rle_get_point()`：顺序解码到目标行，读取单像素
- `egui_image_rle_get_point_resize()`：返回错误（不支持）
- `egui_image_rle_draw_image_resize()`：返回（不绘制）
- `egui_image_rle_t_api_table`：绑定 4 个虚函数

RLE 的特点：像素数据和 alpha 数据分别独立压缩，各自有自己的 RLE 流。blk_size 按数据类型：RGB565=2, RGB32=4, GRAY8=1。Alpha 的 blk_size 根据打包方式而定（alpha_type=8 时 blk_size=1）。

- [ ] **Step 3: 构建验证**

Run: `make clean; make all APP=HelloSimple PORT=pc`
Expected: 编译成功

- [ ] **Step 4: Commit**

```bash
git add src/image/egui_image_rle.h src/image/egui_image_rle.c
git commit -m "feat(image): add RLE image codec (decoder + vtable)"
```

---

### Task 4: QOI Codec C 层实现

**Files:**
- Create: `src/image/egui_image_qoi.h`
- Create: `src/image/egui_image_qoi.c`

- [ ] **Step 1: 创建 egui_image_qoi.h**

```c
#ifndef _EGUI_IMAGE_QOI_H_
#define _EGUI_IMAGE_QOI_H_

#include "egui_image.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
    const uint8_t *data_buf;       // QOI compressed data stream
    uint32_t data_size;            // compressed data length in bytes
    uint32_t decompressed_size;    // decompressed size for validation
    uint8_t data_type;             // EGUI_IMAGE_DATA_TYPE_RGB565 / RGB32
    uint8_t alpha_type;            // EGUI_IMAGE_ALPHA_TYPE_8 or no alpha
    uint8_t res_type;              // EGUI_RESOURCE_TYPE_INTERNAL / EXTERNAL
    uint16_t width;
    uint16_t height;
} egui_image_qoi_info_t;

typedef struct egui_image_qoi egui_image_qoi_t;
struct egui_image_qoi
{
    egui_image_t base;
};

void egui_image_qoi_init(egui_image_t *self, const void *res);

#ifdef __cplusplus
}
#endif

#endif /* _EGUI_IMAGE_QOI_H_ */
```

- [ ] **Step 2: 创建 egui_image_qoi.c 核心解码器**

QOI 解码器参考 `qoi.h` 规范实现。关键数据结构：

```c
typedef struct
{
    const egui_image_qoi_info_t *info;
    uint32_t data_pos;
    uint16_t current_row;
    uint16_t current_col;
    uint16_t cached_row;
    uint8_t prev_r, prev_g, prev_b, prev_a;
    uint8_t index_r[64], index_g[64], index_b[64], index_a[64];
    uint8_t run;
} egui_image_qoi_decode_state_t;
```

QOI 操作码：
- `QOI_OP_RGB`  (0xFE): 3 字节 r,g,b
- `QOI_OP_RGBA` (0xFF): 4 字节 r,g,b,a
- `QOI_OP_INDEX` (0b00xxxxxx): 6 位索引
- `QOI_OP_DIFF`  (0b01xxxxxx): 2 位差分 dr,dg,db
- `QOI_OP_LUMA`  (0b10xxxxxx): 6+4+4 位 luma 差分
- `QOI_OP_RUN`   (0b11xxxxxx): 6 位 run length

QOI 原生输出 RGB/RGBA，需要在逐像素解码后转换为目标格式（RGB565 等），写入行缓冲区。

关键实现点：
- `egui_image_qoi_decode_one_pixel(state, src_data)`：解码一个像素，更新状态
- `egui_image_qoi_decode_row(state, info, pixel_buf, alpha_buf)`：解码一整行
- `egui_image_qoi_draw_image()`：帧开始时检查/重置状态，逐行解码 + blend
- `egui_image_qoi_get_point()`：顺序解码到目标位置
- `egui_image_qoi_get_point_resize()`：返回错误
- `egui_image_qoi_draw_image_resize()`：返回（不绘制）
- `egui_image_qoi_t_api_table`

- [ ] **Step 3: 构建验证**

Run: `make clean; make all APP=HelloSimple PORT=pc`
Expected: 编译成功

- [ ] **Step 4: Commit**

```bash
git add src/image/egui_image_qoi.h src/image/egui_image_qoi.c
git commit -m "feat(image): add QOI image codec (decoder + vtable)"
```

---

### Task 5: 构建系统集成

**Files:**
- Modify: `src/build.mk`
- Modify: `CMakeLists.txt`（或 `src/CMakeLists.txt`）

- [ ] **Step 1: 修改 src/build.mk 添加条件编译**

在现有 `EGUI_CODE_SRC += $(EGUI_PATH)/image` 行之后添加：

```makefile
# Image codec conditional sources
ifeq ($(EGUI_CONFIG_IMAGE_CODEC_QOI_ENABLE), 1)
EGUI_CODE_SRC_FILES += $(EGUI_PATH)/image/egui_image_qoi.c
EGUI_CODE_SRC_FILES += $(EGUI_PATH)/image/egui_image_decode_utils.c
endif

ifeq ($(EGUI_CONFIG_IMAGE_CODEC_RLE_ENABLE), 1)
EGUI_CODE_SRC_FILES += $(EGUI_PATH)/image/egui_image_rle.c
EGUI_CODE_SRC_FILES += $(EGUI_PATH)/image/egui_image_decode_utils.c
endif
```

注意：需要确认 build.mk 使用的是目录模式（自动编译目录下所有 .c）还是文件模式。如果是目录模式，需要改为在 codec 文件内部用 `#if EGUI_CONFIG_IMAGE_CODEC_QOI_ENABLE` 包裹整个文件内容，确保未启用时编译为空。

- [ ] **Step 2: 修改 CMake 集成**

在 CMakeLists.txt 中添加 codec 源文件的条件编译支持。

- [ ] **Step 3: 构建验证（默认关闭 codec）**

Run: `make clean; make all APP=HelloSimple PORT=pc`
Expected: 编译成功，codec 文件不参与编译或编译为空

- [ ] **Step 4: Commit**

```bash
git add src/build.mk CMakeLists.txt
git commit -m "feat(image): integrate codec sources into build system"
```

---

## Chunk 2: Python 构建端 — 编码器与资源管线

### Task 6: Python RLE 编码器

**Files:**
- Create: `scripts/tools/img_codec_rle.py`

- [ ] **Step 1: 创建 RLE 编码器模块**

实现与 C 端解码器完全对称的 RLE 编码算法：

```python
def rle_encode(data: bytes, blk_size: int) -> bytes:
    """
    RLE encode byte stream.
    Control byte format:
    - bit7=1: literal, count = ctrl & 0x7F, followed by count*blk_size raw bytes
    - bit7=0: repeat, count = ctrl, followed by blk_size bytes to repeat
    """
```

关键函数：
- `rle_encode(data, blk_size)` → `bytes`：编码
- `rle_decode(data, blk_size, output_size)` → `bytes`：解码（用于验证）
- `rle_encode_image(pixel_data, alpha_data, width, height, data_type, alpha_type)` → `(compressed_pixels, compressed_alpha)`

- [ ] **Step 2: 添加自测**

在文件末尾添加 `if __name__ == "__main__":` 自测：编码一段测试数据 → 解码 → 验证与原始一致。

Run: `python scripts/tools/img_codec_rle.py`
Expected: 自测通过

- [ ] **Step 3: Commit**

```bash
git add scripts/tools/img_codec_rle.py
git commit -m "feat(tools): add Python RLE encoder for image compression"
```

---

### Task 7: Python QOI 编码器

**Files:**
- Create: `scripts/tools/img_codec_qoi.py`

- [ ] **Step 1: 创建 QOI 编码器模块**

参考 [phoboslab/qoi](https://github.com/phoboslab/qoi) 的 `qoi.h` 规范实现 Python 编码器：

```python
def qoi_encode(pixel_data: bytes, width: int, height: int, 
               channels: int = 3) -> bytes:
    """
    QOI encode pixel data (RGB or RGBA, 8-bit per channel).
    Returns QOI binary stream (without file header, raw compressed data only).
    """
```

关键函数：
- `qoi_encode(pixel_data, width, height, channels)` → `bytes`
- `qoi_decode(data, width, height, channels)` → `bytes`（验证用）
- `qoi_encode_image(pixel_data, alpha_data, width, height, data_type, alpha_type)` → `bytes`

注意：QOI 原生工作在 RGB888/RGBA8888 空间。对于 RGB565 输入，编码前需要先扩展为 RGB888，C 端解码后再转回 RGB565。这在编码器中处理。

- [ ] **Step 2: 添加自测**

Run: `python scripts/tools/img_codec_qoi.py`
Expected: 自测通过，编码解码结果一致

- [ ] **Step 3: Commit**

```bash
git add scripts/tools/img_codec_qoi.py
git commit -m "feat(tools): add Python QOI encoder for image compression"
```

---

### Task 8: 修改 img2c.py 集成压缩

**Files:**
- Modify: `scripts/tools/img2c.py`

- [ ] **Step 1: 在 img2c_tool.__init__() 添加 compress 参数**

新增参数 `compress="none"`，存储为 `self.compress`。

- [ ] **Step 2: 在 write_c_file() 中添加压缩分支**

在原始像素/alpha 数据生成之后、写入 C 文件之前，根据 `self.compress` 分支：

```python
if self.compress == "qoi":
    from img_codec_qoi import qoi_encode_image
    compressed = qoi_encode_image(pixel_data, alpha_data, w, h, data_type, alpha_type)
    # 输出 egui_image_qoi_info_t 结构
elif self.compress == "rle":
    from img_codec_rle import rle_encode_image
    compressed_pixels, compressed_alpha = rle_encode_image(...)
    # 输出 egui_image_rle_info_t 结构
else:
    # 现有路径，输出 egui_image_std_info_t
```

- [ ] **Step 3: 修改 C 模板输出**

对于压缩图片，生成代码格式：
- 数据数组：`static const uint8_t g_image_{name}_{codec}_data[] = { ... };`
- Info 结构：`static const egui_image_{codec}_info_t g_image_{name}_{codec}_info = { ... };`
- 声明宏：`EGUI_IMAGE_SUB_DEFINE_CONST(egui_image_{codec}_t, egui_res_image_{name}_{format}_{alpha}, ...);`
- Include 头文件：`#include "egui_image_{codec}.h"`

- [ ] **Step 4: 添加压缩率日志**

压缩完成后打印：`[IMG] {file}: {original}B → {compressed}B ({codec}, -{ratio}%)`

- [ ] **Step 5: 测试无压缩路径不受影响**

Run: `cd example/HelloBasic/image/resource/src && python ../../../../../scripts/tools/img2c.py ...`（使用现有图片，compress=none）
Expected: 输出与修改前一致

- [ ] **Step 6: Commit**

```bash
git add scripts/tools/img2c.py
git commit -m "feat(tools): add compression support to img2c.py (QOI/RLE)"
```

---

### Task 9: 修改 app_resource_generate.py 传递 compress

**Files:**
- Modify: `scripts/tools/app_resource_generate.py`

- [ ] **Step 1: 在 ImageResourceInfo 解析 compress 字段**

```python
self.compress = config_dict.get("compress", "none")
```

- [ ] **Step 2: 在 generate_img_resource() 传递 compress 到 img2c_tool**

在 `img2c_tool()` 实例化时增加 `compress=img_info.compress` 参数。

- [ ] **Step 3: 添加 GRAY8 + QOI 校验**

如果 `format == "gray8"` 且 `compress == "qoi"`，报错提示改用 RLE 或 none。

- [ ] **Step 4: Commit**

```bash
git add scripts/tools/app_resource_generate.py
git commit -m "feat(tools): pass compress config through resource pipeline"
```

---

### Task 10: MP4 帧压缩支持（已集成）

`app_mp4_image_generate.py` 已删除，其帧提取功能已内联到 `app_resource_generate.py` 的 `_mp4_extract_frames()` 中。MP4 帧的压缩通过 `app_resource_config.json` 中的 `compress` 字段统一配置，由 `generate_mp4_resource()` 处理。

- [x] **已完成：MP4 帧提取集成到 app_resource_generate.py**

---

## Chunk 3: 示例应用与端到端验证

### Task 11: 创建示例子应用 image_compressed

**Files:**
- Create: `example/HelloBasic/image_compressed/test.c`
- Create: `example/HelloBasic/image_compressed/test.h`
- Create: `example/HelloBasic/image_compressed/build.mk`
- Create: `example/HelloBasic/image_compressed/resource/src/app_resource_config.json`
- Copy: 一张测试图片到 `resource/src/`

- [ ] **Step 1: 准备测试图片和资源配置**

从现有示例复制一张图片（如 `star.png`），放入 `resource/src/`。

`app_resource_config.json`：
```json
{
    "img": [
        {
            "file": "star.png",
            "format": "rgb565",
            "alpha": "8",
            "compress": "none"
        },
        {
            "file": "star.png",
            "format": "rgb565",
            "alpha": "8",
            "compress": "qoi"
        },
        {
            "file": "star.png",
            "format": "rgb565",
            "alpha": "8",
            "compress": "rle"
        }
    ]
}
```

同一张图片生成三种版本：原始、QOI、RLE，方便对比。

- [ ] **Step 2: 创建 test.c**

显示三张图片（原始、QOI、RLE）并排排列：

```c
#include "egui.h"
#include "uicode.h"

// 三种格式的同一张图片
extern const egui_image_std_t egui_res_image_star_rgb565_8;
extern const egui_image_qoi_t egui_res_image_star_qoi_rgb565_8;
extern const egui_image_rle_t egui_res_image_star_rle_rgb565_8;

static egui_view_image_t image_std;
static egui_view_image_t image_qoi;
static egui_view_image_t image_rle;
// ... init 并排显示
```

- [ ] **Step 3: 创建 build.mk 和 test.h**

参考现有 HelloBasic 子应用模式。

- [ ] **Step 4: 在 app_egui_config.h 中开启 codec**

```c
#define EGUI_CONFIG_IMAGE_CODEC_QOI_ENABLE 1
#define EGUI_CONFIG_IMAGE_CODEC_RLE_ENABLE 1
```

- [ ] **Step 5: 生成资源**

Run: `make resource APP=HelloBasic APP_SUB=image_compressed`
Expected: 资源生成成功，输出压缩率日志

- [ ] **Step 6: 构建**

Run: `make clean; make all APP=HelloBasic APP_SUB=image_compressed PORT=pc`
Expected: 编译链接成功

- [ ] **Step 7: 运行时验证**

Run: `python scripts/code_runtime_check.py --app HelloBasic --app-sub image_compressed --timeout 10`
Expected: 程序不崩溃，截图中三张图片都正确渲染

- [ ] **Step 8: 检查截图**

对比 `runtime_check_output/HelloBasic_image_compressed/` 目录下的截图：
- 三张图片应该视觉一致（无损压缩，解码结果应与原始像素完全相同）
- 任何差异都说明 codec 实现有 bug

- [ ] **Step 9: Commit**

```bash
git add example/HelloBasic/image_compressed/
git commit -m "feat(example): add image_compressed demo (QOI/RLE comparison)"
```

---

### Task 12: MP4 压缩帧验证

**Files:**
- Modify: `example/HelloBasic/mp4/resource/src/app_resource_config.json`（临时测试）

- [ ] **Step 1: 修改 mp4 示例的资源配置添加 compress**

为 mp4 帧添加 `"compress": "qoi"` 或 `"compress": "rle"`。

- [ ] **Step 2: 重新生成资源并构建**

Run: `make resource APP=HelloBasic APP_SUB=mp4; make all APP=HelloBasic APP_SUB=mp4 PORT=pc`

- [ ] **Step 3: 运行时验证 MP4 播放**

Run: `python scripts/code_runtime_check.py --app HelloBasic --app-sub mp4 --timeout 15`
Expected: MP4 帧动画正常播放，无花屏、无卡顿

- [ ] **Step 4: 恢复 mp4 示例配置（可选，如不需要默认压缩）**

- [ ] **Step 5: Commit**

```bash
git add example/HelloBasic/mp4/
git commit -m "test(mp4): verify compressed frame playback"
```

---

### Task 13: 现有示例回归测试

- [ ] **Step 1: 验证 HelloSimple 不受影响**

Run: `make clean; make all APP=HelloSimple PORT=pc; python scripts/code_runtime_check.py --app HelloSimple --timeout 10`
Expected: 正常运行

- [ ] **Step 2: 验证 HelloBasic image 不受影响**

Run: `make clean; make all APP=HelloBasic APP_SUB=image PORT=pc; python scripts/code_runtime_check.py --app HelloBasic --app-sub image --timeout 10`
Expected: 正常运行

- [ ] **Step 3: 如果时间允许，跑完整编译检查**

Run: `python scripts/code_compile_check.py --full-check`
Expected: 所有示例编译通过

- [ ] **Step 4: Commit（如有修复）**

---

### Task 14: 更新 HelloBasic 子应用列表

**Files:**
- Modify: `CLAUDE.md`（如需添加 image_compressed 到子应用列表）

- [ ] **Step 1: 在 CLAUDE.md 的 HelloBasic 子应用列表中添加 image_compressed**

- [ ] **Step 2: Commit**

```bash
git add CLAUDE.md
git commit -m "docs: add image_compressed to HelloBasic sub-app list"
```
