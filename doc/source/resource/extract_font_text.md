# 字体字符集自动提取

## 背景

EmbeddedGUI 的字体资源在编译期由 `ttf2c.py` 将 TTF 转换为 C 数组，只编译**实际用到的字符**，以节省 ROM 空间。这意味着：每次在代码中新增中文文本或图标字符，都必须把对应的字符写入字体的字符集文件（`text` 字段指定的 `.txt` 文件），否则运行时会显示方框或空白。

手动维护字符集文件存在以下问题：

- **容易遗漏**：代码中的字符串分散在多个文件、多个静态数组、双语宏等位置。
- **格式不统一**：有些项目把所有字符合并成一行，维护时难以分辨哪些字符来自哪段代码。
- **多字体时更繁琐**：一个应用可能同时用 NotoSansSC（中文）和 MaterialSymbolsOutlined（图标），两套字符集需要分开管理。

为解决上述问题，提供了自动提取脚本 `scripts/tools/extract_font_text.py`。

---

## 工作流程

```
C 源文件 (*.c / *.h)
      │
      ▼
extract_font_text.py            ← 扫描提取所有文本字符串
      │
      ├─ cn_text.txt            ← 中文字体字符集（每行一条字符串）
      └─ icon_text.txt          ← 图标字体字符集（每行一个 &#xNNNN; 实体）
                │
                ▼
          make resource_refresh  ← 重新生成 C 字体数组
```

脚本会自动：

1. 读取 `example/{APP}/resource/src/app_resource_config.json`，根据字体文件名推断字体类型（中文 / 图标 / 拉丁）。
2. 扫描应用目录下的所有 `.c` 和 `.h` 文件，通过以下多种规则提取字符串：
   - `S("EN", "中文")` 双语宏——提取第二参数中文部分（最可靠）
   - `flag ? "EN" : "中文"` 或 `flag ? "中文" : "EN"` 三元表达式——**两侧都提取**，因为同一 widget 使用 CN 字体渲染两种状态
   - `xxx_cn[]` 静态数组——提取数组中的所有字符串
   - 代码中直接出现的 CJK 字符串（catch-all 保底）
   - `\xNN\xNN` 等 hex 转义字节，解码后属于 PUA 范围的图标字符
3. 每条字符串单独写一行，便于后续维护。
4. 图标字符以 `&#xNNNN;` HTML 实体格式输出（`ttf2c.py` 原生支持该格式）。
5. 默认使用**追加模式**：不删除已有行，只补充新字符串，不会破坏手动维护的内容。
6. `app_resource_generate.py` 会在生成字体时对所有字符去重，因此文本文件中的重复字符串无害。

---

## 快速上手

### 基本用法

```bash
# 追加模式：提取并填充新字符（不覆盖已有内容）
python scripts/tools/extract_font_text.py --app HelloShowcase

# 预览模式：只打印提取结果，不写入任何文件
python scripts/tools/extract_font_text.py --app HelloShowcase --dry-run

# 覆盖模式：完全重建 text 文件（首次迁移旧项目时使用）
python scripts/tools/extract_font_text.py --app HelloShowcase --overwrite
```

### 完整开发循环

```bash
# 1. 在 C 代码中新增中文文本或图标字符后，重新提取
python scripts/tools/extract_font_text.py --app HelloShowcase

# 2. 重新生成字体资源
make resource_refresh APP=HelloShowcase

# 3. 编译验证
make all APP=HelloShowcase
```

---

## 字体类型判断规则

脚本根据字体文件名中的关键词自动判断字体类型，无需手动配置：

| 类型 | 触发关键词示例 | 提取内容 |
|------|-------------|--------|
| **中文（CN）** | `noto`、`sc`、`hans`、`simhei`、`cjk`、`chinese` | 代码中的 CJK 字符串 |
| **图标（Icon）** | `material`、`symbol`、`icon`、`fontawesome`、`fa-` | 解码后为 PUA（U+E000~F8FF）的字符 |
| **拉丁（Latin）** | 其余 | 跳过（ASCII 字符由默认字体提供） |

---

## 文本文件格式

### 中文字体（cn_text.txt）

每行写一条完整的字符串，即代码中实际出现的文本，包括标点和空格：

```
基础
切换
滑块/选择
动态 42
多行文本块
你好
请输入...
选项 1
选项 2
```

这种格式的优点：

- **可读性强**：一眼可知哪些字符来自哪段界面文本。
- **方便 diff**：每次新增字符串只会追加新行，不会修改已有行，Git 历史清晰。
- **允许重复**：`app_resource_generate.py` 内部自动去重，同一汉字出现多次不影响生成结果。

### 图标字体（icon_text.txt）

图标字符位于 Unicode PUA 区域，无法直接输入，以 HTML 实体表示：

```
&#xE518;
&#xE51C;
```

`ttf2c.py` 在读取文本文件时会自动将 `&#xNNNN;` 解析为对应的 Unicode 字符。

> **如何获取图标的 Unicode 码点？**  
> 在代码中，图标通常以 `\xEE\x94\x98` 这样的 UTF-8 hex 转义出现。  
> 将其字节序列按 UTF-8 解码即得到码点，例如：  
> `\xEE\x94\x98` → UTF-8 字节 `EE 94 98` → U+E518 → `&#xE518;`

---

## 多字体场景

若同一应用使用多个中文字体（如不同字号），`app_resource_config.json` 中会有多个 CN 类型的字体条目，各自指向不同的 text 文件。

```json
{
    "font": [
        {
            "file": "build_in/NotoSansSC-VF.ttf",
            "name": "notosanssc_14",
            "pixelsize": "14",
            "text": "cn_text_14.txt"
        },
        {
            "file": "build_in/NotoSansSC-VF.ttf",
            "name": "notosanssc_20",
            "pixelsize": "20",
            "text": "cn_text_20.txt"
        }
    ]
}
```

脚本会为每个字体的 text 文件都填充完整字符集（安全做法）。如需按字号精确拆分，只保留各自需要的字符串行即可。

---

## 命令行参数

| 参数 | 说明 | 默认值 |
|------|------|--------|
| `--app APP` | 应用名称（必填） | — |
| `--src-dir DIR` | 源代码目录 | `example/{APP}` |
| `--resource-dir DIR` | 资源 src 目录 | `example/{APP}/resource/src` |
| `--dry-run` | 预览提取结果，不写入文件 | 关闭 |
| `--overwrite` | 覆盖模式，重建 text 文件 | 关闭（默认追加） |

---

## 常见问题

### 某个 ASCII 字母用 CN 字体渲染时显示方框

**典型场景**：`btn_lang` 始终使用 CN 字体，其文本在中英切换之间交替：
```c
egui_view_label_set_text(..., is_chinese ? "EN" : "中文");
```
"EN" 中没有 CJK 字符，之前版本的提取脚本会忽略它。现在脚本通过三元表达式规则（`? "latin" : "cjk"` 两侧都提取）自动处理此类情况。如果使用旧版本脚本生成了字符集，应重新运行 `extract_font_text.py --overwrite`。

### 某个汉字仍然显示方框

**原因**：该字符未被脚本提取到（例如通过运行时拼接生成的字符串）。

**修复**：在对应的 text 文件中手动添加包含该字符的行，然后重新执行 `make resource_refresh`。

### 运行脚本后字符集文件没有变化

可能的原因：

1. 追加模式下，所有字符已存在于 text 文件中——说明字符集是最新的，无需操作。
2. 配置文件中该字体没有指定 `text` 字段——需要在 `app_resource_config.json` 的字体条目中添加 `"text"` 字段。

### 图标字符显示为空白（不是方框）

图标字符进入了 text 文件并生成了字体资源，但渲染时没有设置颜色。`alpha` 格式的图标不带颜色信息，需要在代码中调用：

```c
egui_view_image_set_image_color(view, EGUI_COLOR_MAKE(255, 255, 255), EGUI_ALPHA_100);
```
