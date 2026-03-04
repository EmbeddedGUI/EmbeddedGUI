---
name: resource-generation
description: Use when adding images, fonts, or icons to EmbeddedGUI apps, or when resource build fails with undefined reference errors
---

# Resource Generation Skill

EmbeddedGUI 资源生成管线：将 PNG 图片和 TTF 字体转换为 C 数组或外部 bin 文件。

## 资源配置文件

位置：`example/{APP}/resource/src/app_resource_config.json`

```json
{
    "img": [
        {
            "file": "icon_battery.png",
            "format": "alpha",
            "alpha": "4",
            "external": "0",
            "swap": "0"
        }
    ],
    "font": [
        {
            "file": "simhei.ttf",
            "pixelsize": "14",
            "fontbitsize": "4",
            "external": "0",
            "text": "supported_text.txt"
        }
    ]
}
```

### 图片配置字段

| 字段 | 说明 | 可选值 |
|------|------|--------|
| `file` | 源图片文件名（在 `resource/src/` 下） | PNG/BMP |
| `format` | 像素格式 | `rgb565`, `rgb32`, `gray8`, `alpha` |
| `alpha` | Alpha 位深 | `0`, `1`, `2`, `4`, `8` |
| `external` | 存储方式 | `0`=内部C数组, `1`=外部bin文件 |
| `swap` | RGB565 字节交换 | `0`, `1` |

### 格式选择指南

| 场景 | format | alpha | 说明 |
|------|--------|-------|------|
| 单色图标（Material Icons） | `alpha` | `4` | 仅存储透明度，运行时着色，体积最小 |
| 全彩照片/背景 | `rgb565` | `0` | 无透明度，16位色 |
| 全彩+半透明 | `rgb565` | `4` | 带4位alpha通道 |
| 高质量全彩 | `rgb32` | `8` | 32位ARGB，体积最大 |
| 灰度图 | `gray8` | `0` | 8位灰度 |

### 字体配置字段

| 字段 | 说明 | 可选值 |
|------|------|--------|
| `file` | TTF 字体文件名 | `.ttf` 文件 |
| `pixelsize` | 字号（像素高度） | 8-48 |
| `fontbitsize` | 字形位深 | `1`, `2`, `4`, `8` |
| `external` | 存储方式 | `0`=内部, `1`=外部 |
| `text` | 字符集文件 | 包含所有需要渲染的字符 |

字符集文件（`text`）为 UTF-8 纯文本，包含所有需要的字符。支持 HTML 实体如 `&#x2103;`（℃）。可指定多个文件用逗号分隔。

## 生成命令

```bash
# 自动生成（make all 时自动触发，有resource/目录即可）
make all APP={APP} PORT=pc BITS=64

# 仅生成资源
make resource

# 强制重新生成（忽略缓存的 bin 文件）
make resource_refresh

# 直接调用脚本
python scripts/tools/app_resource_generate.py -r example/{APP}/resource -o output
```

### 单独生成图片/字体

```bash
# 图片转C数组
python scripts/tools/img2c.py \
    -i icon.png -n battery -f alpha -a 4 -ext 0 -o output/

# 图片缩放
python scripts/tools/img2c.py \
    -i photo.png -n photo -f rgb565 -a 0 -d 64 64 -ext 0 -o output/

# 字体转C数组
python scripts/tools/ttf2c.py \
    -i simhei.ttf -n title -t supported_text.txt -p 16 -s 4 -ext 0
```

## 输出文件结构

```
example/{APP}/resource/
├── src/                              ← 源文件（手动管理）
│   ├── app_resource_config.json
│   ├── icon_battery.png
│   ├── simhei.ttf
│   └── supported_text.txt
├── img/                              ← 生成的图片C文件
│   └── egui_res_image_{name}_{format}_{alpha}.c
├── font/                             ← 生成的字体C文件
│   └── egui_res_font_{name}_{size}_{bits}.c
├── app_egui_resource_generate.h      ← 资源声明头文件
├── app_egui_resource_generate.c      ← 资源ID偏移表
├── app_egui_resource_merge.bin       ← 外部资源合并文件
└── app_egui_resource_generate_report.md  ← 资源大小报告
```

## 代码中引用资源

```c
// 图片资源（extern 声明在 app_egui_resource_generate.h 中）
extern const egui_image_std_t egui_res_image_battery_alpha_4;
egui_view_image_set_image(view, &egui_res_image_battery_alpha_4);

// 字体资源
extern const egui_font_t egui_res_font_simhei_14_4;
egui_view_label_set_font(view, &egui_res_font_simhei_14_4);
```

## 内置共享字体库

常用开源字体已预置于 `scripts/tools/build_in/`（均为 Apache 2.0 授权），直接复制使用，无需自行查找：

| 文件 | 用途 |
|------|------|
| `NotoSansSC-VF.ttf` | 中文（简体）+ 英文，可变字重 |
| `MaterialSymbolsOutlined-Regular.ttf` | Material Design 图标符号 |
| `Montserrat-Medium.ttf` / `Montserrat-Bold.ttf` | 英文西文正文/标题 |
| `DejaVuSans.ttf` | 英文通用，字符集全 |
| `unscii-8.ttf` | 等宽像素字体 |

新建应用时，从此目录复制所需字体到 `example/{APP}/resource/src/`，无需从系统字体目录或其他项目中搜索。

## 自动提取字符集（中文 / 图标）

代码中的文本字符串需要写入字体的字符集文件（如 `cn_text.txt`、`icon_text.txt`），才能被编译进字体资源。手动维护容易遗漏——应使用自动提取脚本。

```bash
# 提取 HelloShowcase 中所有中文字符串和图标字符，写入对应 text 文件
python scripts/tools/extract_font_text.py --app HelloShowcase

# 先预览提取结果（不写入文件）
python scripts/tools/extract_font_text.py --app HelloShowcase --dry-run

# 覆盖模式（重建 text 文件，丢弃旧内容，适合首次生成）
python scripts/tools/extract_font_text.py --app HelloShowcase --overwrite
```

脚本自动执行的操作：
1. 读取 `example/{APP}/resource/src/app_resource_config.json`，根据字体文件名判断类型（CN / 图标 / 拉丁）
2. 扫描 `example/{APP}/*.c *.h`，提取：
   - `S("EN", "中文")` 双语宏的中文部分
   - `flag ? "EN" : "中文"` / `flag ? "中文" : "EN"` 三元表达式的**两侧**（因为同一 widget 用 CN 字体渲染两种状态）
   - `xxx_cn[]` 静态数组中的中文字符串
   - 代码中直接出现的 CJK 字符串
   - `\xNN\xNN` 等 hex 转义解码后属于 PUA 范围的图标字符
3. 每个字符串单独写一行（便于维护，`app_resource_generate.py` 会去重）
4. icon 字符输出为 `&#xNNNN;` HTML 实体格式（`ttf2c.py` 原生支持）
5. 默认**追加**模式：不删除已有行，只补充新字符串

**多字体场景**：如果同一应用有多个 CN 字体（不同字号），脚本会为每个字体的 text 文件都补充完整字符集（安全做法）。如需按字号拆分，手动从生成的 text 文件中删除不需要的行即可。

**字体类型判断规则**（基于文件名关键词）：
- CN 字体：`noto`、`sc`、`hans`、`simhei`、`cjk` 等
- 图标字体：`material`、`symbol`、`icon`、`fontawesome` 等
- 拉丁字体：其余（跳过，ASCII 使用默认字体）

每次在代码中新增中文文本或图标字符后，都应重新运行脚本，然后执行 `make resource_refresh`。

## 添加新资源的流程

1. 将源文件（PNG/TTF）放入 `example/{APP}/resource/src/`
   - 字体优先从 `scripts/tools/build_in/` 复制
2. 编辑 `app_resource_config.json` 添加配置条目
3. 运行字符集提取脚本填充 text 文件：
   ```bash
   python scripts/tools/extract_font_text.py --app {APP}
   ```
4. 运行 `make resource_refresh APP={APP}` 重新生成 C 源文件
5. 在代码中 `#include "app_egui_resource_generate.h"` 并引用资源

## 常见问题排查

| 问题 | 原因 | 修复 |
|------|------|------|
| `undefined reference to 'egui_res_image_xxx'` | 资源未生成或名称不匹配 | 检查 config.json 中 file 字段，运行 `make resource_refresh` |
| 图标显示为空白 | alpha格式缺少 `image_color` 设置 | 代码中调用 `egui_view_image_set_image_color()` |
| 字符显示为方框 | 字符不在字符集文件中 | 将缺失字符添加到 `supported_text.txt` |
| 字体模糊 | `fontbitsize` 过低 | 提高到 `4` 或 `8` |
| 图片颜色偏差 | RGB565 字节序错误 | 尝试设置 `"swap": "1"` |
| 资源文件过大 | 图片尺寸过大或位深过高 | 缩小图片、降低 alpha 位深、使用 external 存储 |
| 生成跳过（使用缓存） | `app_egui_resource_merge.bin` 已存在 | 删除 bin 文件或用 `make resource_refresh` |

## build.mk 资源目录配置

应用的 `build.mk` 需要包含资源目录：

```makefile
EGUI_CODE_SRC += $(wildcard example/{APP}/resource/img/*.c)
EGUI_CODE_SRC += $(wildcard example/{APP}/resource/font/*.c)
EGUI_CODE_SRC += example/{APP}/resource/app_egui_resource_generate.c
EGUI_CODE_INCLUDE += example/{APP}/resource
```

## 文件参考

| 文件 | 说明 |
|------|------|
| `scripts/tools/extract_font_text.py` | 自动提取 C 源码中文/图标字符串到字体 text 文件 |
| `scripts/tools/app_resource_generate.py` | 资源生成主脚本 |
| `scripts/tools/img2c.py` | 图片转C数组 |
| `scripts/tools/ttf2c.py` | 字体转C数组 |
| `porting/pc/Makefile.base` | 资源生成 make 规则 |
