# 资源管理概述

对于嵌入式 GUI 项目而言，除了完成对控件的管理外，还需要对 GUI 所需的资源进行管理。一般来讲，GUI 项目的管理代码基本不大，主要占用 Code Size 的是图片和文字资源，如何合理管理这些资源对于整个 GUI 项目至关重要。

## 资源分类

EmbeddedGUI 将资源分为两大类：

### 内部资源（Internal Resource）

内部资源直接编译到固件中，存储在芯片的 Flash（ROM）里。

- 访问速度快，无需额外的加载逻辑
- 占用芯片内部 Code Size
- 适合高频访问的小尺寸资源（常用图标、主界面字体等）

生成的文件格式：
- 图片：`egui_res_image_xxxxx.c`，包含像素数组和配置信息
- 字体：`egui_res_font_xxxxx.c`，包含字形像素矩阵和编码映射

### 外部资源（External Resource）

外部资源存储在外部存储介质（SPI Flash、SD 卡等），运行时按需加载。

- 不占用芯片内部 Code Size
- 需要实现 `load_external_resource` 平台接口
- 适合大尺寸资源（高分辨率图片、多语言字体等）

生成的文件格式：
- 图片：`egui_res_image_xxxxx_bin.c`（配置信息）+ `.bin`（二进制数据）
- 字体：`egui_res_font_xxxxx_bin.c`（配置信息）+ `.bin`（二进制数据）
- 合并文件：`app_egui_resource_merge.bin`（所有外部资源打包）

## 资源目录结构

每个示例应用的资源目录结构如下：

```
example/HelloXXX/
├── resource/
│   ├── src/
│   │   ├── app_resource_config.json    # 资源配置文件
│   │   ├── star.png                     # 原始图片文件
│   │   ├── optional_local_font.ttf      # 非 build-in 字体时放这里
│   │   └── supported_text.txt           # 所需文本列表
│   ├── font/                            # 生成的字体资源
│   ├── img/                             # 生成的图片资源
│   ├── app_egui_resource_generate.h     # 资源索引头文件
│   ├── app_egui_resource_generate.c     # 外部资源偏移表
│   ├── app_egui_resource_merge.bin      # 外部资源合并文件
│   └── app_egui_resource_generate_report.md  # 资源统计报告
```

## app_resource_config.json 配置

资源配置使用 JSON5 格式（支持注释），包含 `img` 和 `font` 两个数组。

### 图片资源配置

```json
{
    "img": [
        {
            "file": "star.png",
            "name": "star",
            "external": "0",
            "format": "rgb565",
            "alpha": "4",
            "dim": "-1,-1",
            "rot": "-1",
            "swap": "0"
        }
    ]
}
```

| 参数 | 说明 | 可选值 |
|------|------|--------|
| `file` | 原始图片文件名 | `*.png`, `*.jpg`, `*.bmp` |
| `name` | 资源名称（可选，默认用文件名） | 自定义字符串 |
| `external` | 存储位置 | `0`=内部, `1`=外部, `all`=两者都生成 |
| `format` | 像素格式 | `rgb565`, `rgb32`, `gray8`, `all` |
| `alpha` | Alpha 位深 | `0`, `1`, `2`, `4`, `8`, `all` |
| `dim` | 缩放尺寸 | `"宽,高"`, `"-1,-1"`=不缩放 |
| `rot` | 旋转角度 | 角度值, `"-1"`=不旋转 |
| `swap` | RGB565 字节交换 | `0`=不交换, `1`=交换 |

### 字体资源配置

```json
{
    "font": [
        {
            "file": "build_in/Montserrat-Medium.ttf",
            "name": "demo_font",
            "text": "supported_text.txt",
            "external": "0",
            "pixelsize": "16",
            "fontbitsize": "4"
        }
    ]
}
```

| 参数 | 说明 | 可选值 |
|------|------|--------|
| `file` | TTF 字体文件名 | `resource/src/*.ttf` 或 `build_in/xxx.ttf` |
| `text` | 所需文本列表文件 | `*.txt`（UTF-8 编码） |
| `external` | 存储位置 | `0`=内部, `1`=外部, `all`=两者都生成 |
| `pixelsize` | 字体像素大小 | `4`-`32`, `all` |
| `fontbitsize` | 字形位深 | `1`, `2`, `4`, `8`, `all` |

相同字体文件、相同参数的多个配置项会自动合并文本列表，重复文字会去重。
优先直接引用 `scripts/tools/build_in/` 中的内置字体；只有 build-in 不满足时才把额外 TTF 放到 `resource/src/`。

## 资源生成命令

```bash
# 生成资源（检查 merge.bin 是否存在，不存在才生成）
make resource APP=HelloResourceManager

# 强制重新生成资源
make resource_refresh APP=HelloResourceManager
```

资源生成脚本 `app_resource_generate.py` 会：

1. 解析 `app_resource_config.json` 配置
2. 调用 `img2c.py` 转换图片资源
3. 调用 `ttf2c.py` 转换字体资源
4. 生成 `app_egui_resource_generate.h`（资源声明和外部资源 ID 枚举）
5. 生成 `app_egui_resource_generate.c`（外部资源偏移地址表）
6. 将所有外部资源打包为 `app_egui_resource_merge.bin`
7. 生成 `app_egui_resource_generate_report.md`（资源统计报告）

## 生成产物说明

### app_egui_resource_generate.h

包含所有资源的 `extern` 声明和外部资源 ID 枚举：

```c
// 资源声明
extern const egui_image_std_t egui_res_image_star_rgb565_4;
extern const egui_image_std_t egui_res_image_star_rgb565_4_bin;
extern const egui_font_std_t egui_res_font_test_16_4;

// 外部资源 ID 枚举
enum {
    EGUI_EXT_RES_ID_BASE = 0x00,
    EGUI_EXT_RES_ID_EGUI_RES_IMAGE_STAR_RGB565_4_DATA,
    EGUI_EXT_RES_ID_EGUI_RES_IMAGE_STAR_RGB565_4_ALPHA,
    // ...
    EGUI_EXT_RES_ID_MAX,
};

extern const uint32_t egui_ext_res_id_map[EGUI_EXT_RES_ID_MAX];
```

### app_egui_resource_merge.bin

所有外部资源的二进制打包文件，可直接烧录到外部 Flash。`egui_ext_res_id_map` 数组记录了每个资源在 bin 文件中的偏移地址。

### 资源统计报告

`app_egui_resource_generate_report.md` 以 Markdown 表格形式展示每个资源的 Code Size 占用，方便开发者评估资源使用情况。

## 在代码中使用资源

```c
#include "app_egui_resource_generate.h"

// 使用内部图片资源
egui_view_image_set_image(EGUI_VIEW_OF(&image), (egui_image_t *)&egui_res_image_star_rgb565_4);

// 使用外部图片资源（名称带 _bin 后缀）
egui_view_image_set_image(EGUI_VIEW_OF(&image), (egui_image_t *)&egui_res_image_star_rgb565_4_bin);

// 使用字体资源
egui_view_label_set_font(EGUI_VIEW_OF(&label), (egui_font_t *)&egui_res_font_test_16_4);
```

使用外部资源时，需要在 `app_egui_config.h` 中启用：

```c
#define EGUI_CONFIG_FUNCTION_RESOURCE_MANAGER  1
#define EGUI_CONFIG_FUNCTION_EXTERNAL_RESOURCE 1
```

并在平台驱动中实现 `load_external_resource` 接口。
