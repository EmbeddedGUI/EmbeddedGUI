# HTML/JSX -> XML 转换流程

## 概述

HTML/JSX -> XML 转换是 UI Designer 设计稿转换管道的第一阶段。它将 Stitch HTML（Tailwind CSS + Material Symbols）或 Figma Make JSX/TSX（React + Tailwind + Lucide）设计稿解析为结构化 JSON，再由 AI 或工具转换为 EGUI XML 布局文件。

转换管道的核心工具是 `scripts/html2egui_helper.py`，提供以下子命令：

| 子命令 | 说明 |
|--------|------|
| `extract-layout` | 解析 HTML，输出结构化 JSON |
| `figmamake-extract` | 解析 Figma Make JSX/TSX 项目 |
| `scaffold` | 创建 .egui 项目结构 |
| `export-icons` | 导出图标 PNG |
| `generate-code` | XML -> C 代码 |
| `gen-resource` | 生成 C 资源数组 |
| `verify` | 构建 + 运行时验证 |

## 输入格式

### Stitch HTML

Stitch 生成的 HTML 使用 Tailwind CSS 类名描述布局和样式：

```html
<div class="w-[320px] h-[480px] bg-[#0F172A] flex flex-col">
    <div class="flex justify-between items-center px-4 py-2">
        <span class="text-white text-sm">12:30</span>
        <span class="material-symbols-outlined text-white">wifi</span>
    </div>
    <div class="grid grid-cols-2 gap-4 p-4">
        <div class="bg-[#1A2130] rounded-xl p-4">
            <span class="text-white text-sm">Lights</span>
        </div>
    </div>
</div>
```

### Figma Make JSX/TSX

Figma Make 生成的 React 项目使用 `className` 和 Lucide 图标：

```tsx
<div className="w-[320px] h-[240px] bg-[#060608] flex flex-col">
    <div className="flex items-center gap-2 p-4">
        <Battery className="w-5 h-5 text-cyan-400" />
        <span className="text-white text-lg">{batteryLevel}%</span>
    </div>
</div>
```

## HTML 解析器

### _LayoutHTMLParser

`_LayoutHTMLParser` 继承自 Python 标准库的 `html.parser.HTMLParser`，负责将 HTML 解析为 DOM 树：

```python
class _LayoutHTMLParser(HTMLParser):
    def __init__(self):
        self._stack = []   # 节点栈
        self._root = None  # 根节点

    def handle_starttag(self, tag, attrs):
        node = {
            "tag": tag,
            "attrs": dict(attrs),
            "classes": attrs_dict.get("class", ""),
            "children": [],
            "text": "",
        }
        # 构建父子关系
        if self._stack:
            self._stack[-1]["children"].append(node)
        self._stack.append(node)

    def handle_data(self, data):
        # 收集文本内容
        if text and self._stack:
            self._stack[-1]["text"] += text
```

解析器自动检测主容器：查找同时具有 `w-[Npx]` 和 `h-[Npx]` 类名的 div 元素作为屏幕根容器。

### 布局分类

`_classify_layout()` 函数将 Tailwind CSS 类名分类为 EGUI 布局类型：

| Tailwind 类名 | 分类结果 | EGUI 映射 |
|---------------|---------|-----------|
| `grid grid-cols-2` | `grid-2col` | `GridLayout` (col_count=2) |
| `grid grid-cols-3` | `grid-3col` | `GridLayout` (col_count=3) |
| `flex flex-col` | `flex-col-start` | `LinearLayout` (vertical) |
| `flex` (默认 row) | `flex-row-start` | `LinearLayout` (horizontal) |
| `flex justify-between` | `flex-row-between` | `Group` (手动定位) |
| `flex justify-center` | `flex-row-center` | `LinearLayout` (居中) |
| 无 flex/grid | `block` | `Group` |

注意：`justify-between` 布局在 EGUI 中没有直接对应，需要使用 `Group` + 手动 x/y 定位。

### 子元素识别

`_extract_child_info()` 函数识别子元素类型：

| HTML 特征 | 识别为 | EGUI 控件 |
|-----------|--------|-----------|
| `class="material-symbols-outlined"` | `icon` | `Image` (alpha 格式) |
| `<span>`, `<p>`, `<h1>` 等有文本 | `text` | `Label` |
| `<svg>` 含 `stroke-dasharray` | `svg_gauge` | `CircularProgressBar` |
| 有子元素的 div | `container` | `Group`/`Card`/`LinearLayout` |

## 样式属性映射

### 颜色映射

解析器从 Tailwind 配置和类名中提取颜色：

```python
# 从 tailwind.config 提取自定义颜色
_extract_colors_from_tailwind_config(html)
# 输出: {"cyan-400": "0x22D3EE", "gray-400": "0x9CA3AF"}
```

| Tailwind 颜色 | EGUI 颜色 |
|---------------|-----------|
| `text-white` | `EGUI_COLOR_WHITE` |
| `text-black` | `EGUI_COLOR_BLACK` |
| `text-red-500` | `EGUI_COLOR_HEX(0xEF4444)` |
| `bg-[#1A2130]` | `EGUI_COLOR_HEX(0x1A2130)` |
| 自定义 `text-cyan-400` | `EGUI_COLOR_HEX(0x22D3EE)` |

### 字体映射

| Tailwind 字体 | 像素大小 | EGUI 字体 | 最小标签高度 |
|---------------|---------|-----------|------------|
| `text-xs` | 12px | `&egui_res_font_montserrat_12_4` | 16 |
| `text-sm` | 14px | `&egui_res_font_montserrat_14_4` | 18 |
| `text-base` | 16px | `&egui_res_font_montserrat_16_4` | 21 |
| `text-lg` | 18px | `&egui_res_font_montserrat_18_4` | 23 |
| `text-xl` | 20px | `&egui_res_font_montserrat_20_4` | 26 |
| `text-2xl` | 24px | `&egui_res_font_montserrat_24_4` | 31 |
| `text-[Npx]` | N | 最接近的 montserrat 字体 | 查表 |

### 间距映射

| Tailwind 间距 | 像素值 |
|--------------|--------|
| `p-1` / `gap-1` | 4px |
| `p-2` / `gap-2` | 8px |
| `p-3` / `gap-3` | 12px |
| `p-4` / `gap-4` | 16px |
| `p-6` / `gap-6` | 24px |
| `p-8` / `gap-8` | 32px |
| `p-[Npx]` | N |

### 圆角映射

| Tailwind 圆角 | 像素值 |
|--------------|--------|
| `rounded` | 4px |
| `rounded-lg` | 8px |
| `rounded-xl` | 12px |
| `rounded-2xl` | 16px |
| `rounded-full` | circle |

## extract-layout 输出格式

`extract-layout` 命令输出结构化 JSON：

```none
{
  "screen": {"width": 320, "height": 480},
  "colors": {"cyan-400": "0x22D3EE", "gray-400": "0x9CA3AF"},
  "icons": [
    {"name": "wifi", "color": "white", "size_px": 24}
  ],
  "sections": [
    {
      "id": "section_0",
      "classes": "flex justify-between items-center px-4 py-2",
      "layout_type": "flex-row-between",
      "padding": {"x": 16, "y": 8},
      "children": [
        {"type": "text", "content": "12:30", "font": {"size": "sm"}, "color": "white"},
        {"type": "icon", "name": "wifi", "color": "white"}
      ]
    },
    {
      "id": "section_1",
      "classes": "grid grid-cols-2 gap-4 p-4",
      "layout_type": "grid-2col",
      "gap": 16,
      "padding": {"all": 16},
      "children": [
        {"type": "container", "classes": "bg-[#1A2130] rounded-xl p-4", ...}
      ]
    }
  ]
}
```

## 布局推断规则

从 JSON 分析结果到 XML 布局的推断规则：

### flex-col -> LinearLayout (vertical)

```json
{"layout_type": "flex-col-start", "gap": 8}
```

```none
<LinearLayout id="vlist" orientation="vertical" align_type="EGUI_ALIGN_CENTER">
    <Label id="item1" margin="4" ... />
    <Label id="item2" margin="4" ... />
</LinearLayout>
```

`gap` 值转换为子元素的 `margin` 属性（margin = gap / 2）。

### flex-row -> LinearLayout (horizontal)

```json
{"layout_type": "flex-row-center"}
```

```none
<LinearLayout id="hlist" orientation="horizontal" align_type="EGUI_ALIGN_VCENTER">
    <Image id="icon" ... />
    <Label id="name" margin="4" ... />
</LinearLayout>
```

### flex-row-between -> Group (手动定位)

`justify-between` 无法用 LinearLayout 表达，需要使用 Group + 手动计算 x/y：

```json
{"layout_type": "flex-row-between"}
```

```none
<Group id="header" x="0" y="0" width="320" height="32">
    <Label id="time" x="16" y="4" ... />
    <Image id="wifi" x="280" y="4" ... />
</Group>
```

### grid-Ncol -> GridLayout

```json
{"layout_type": "grid-2col", "gap": 16}
```

```none
<GridLayout id="grid" col_count="2" align_type="EGUI_ALIGN_CENTER">
    <Card id="cell1" margin="4" ... />
    <Card id="cell2" margin="4" ... />
</GridLayout>
```

### 圆角容器 -> Card

带 `rounded-*` 和背景色的容器映射为 Card：

```json
{"type": "container", "classes": "bg-[#1A2130] rounded-xl p-4"}
```

```xml
<Card id="device_card" corner_radius="12">
    <Background type="solid" color="EGUI_COLOR_HEX(0x1A2130)" alpha="EGUI_ALPHA_100" />
    <!-- children -->
</Card>
```

## 嵌套结构保持

HTML 的嵌套结构在 XML 中保持一致。解析器递归处理子元素，生成对应的 XML 控件树：

```
HTML:                              XML:
<div class="flex flex-col">        <LinearLayout orientation="vertical">
  <div class="flex gap-2">           <LinearLayout orientation="horizontal">
    <span>icon</span>                   <Image />
    <span>WiFi</span>                   <Label text="WiFi" />
  </div>                              </LinearLayout>
  <div class="grid grid-cols-2">      <GridLayout col_count="2">
    <div class="rounded-xl">            <Card corner_radius="12">
      <span>Cell 1</span>                  <Label text="Cell 1" />
    </div>                               </Card>
  </div>                              </GridLayout>
</div>                             </LinearLayout>
```

## Figma Make 项目的差异

Figma Make 项目使用 `figmamake-extract` 子命令解析，与 Stitch HTML 的主要差异：

| 方面 | Stitch HTML | Figma Make JSX |
|------|-------------|----------------|
| 属性名 | `class` | `className` |
| 图标 | Material Symbols `<span>` | Lucide React `<Battery />` |
| 动态内容 | 静态文本 | `{variable}`, `.map()` |
| 文件结构 | 单 HTML 文件 | 多文件组件 + 路由 |
| 透明度 | 无 | `bg-gray-800/50` |

### Lucide -> Material Symbols 映射

Figma Make 使用 Lucide 图标，需要映射为 Material Symbols 以便导出 PNG：

| Lucide | Material Symbols |
|--------|-----------------|
| `Battery` | `battery_full` |
| `Zap` | `bolt` |
| `ThermometerSun` | `thermostat` |
| `Settings` | `settings` |
| `Wifi` | `wifi` |
| `AlertTriangle` | `warning` |

### 动态内容处理

- `{variable}` 表达式 -> 占位文本
- `.map()` 迭代 -> 提取模板元素（显示一次）
- 条件渲染 `{cond && <Elem/>}` -> 保留元素
- 模板字面量 className -> 取第一个分支

## 完整转换示例

### 输入 HTML

```html
<div class="w-[320px] h-[480px] bg-[#0F172A] flex flex-col">
    <!-- Status Bar -->
    <div class="flex justify-between items-center px-4 py-1">
        <span class="text-white text-sm">12:30</span>
        <span class="material-symbols-outlined text-white text-[20px]">wifi</span>
    </div>
    <!-- Title -->
    <span class="text-white text-2xl font-bold px-4 py-2">Smart Home</span>
    <!-- Device Grid -->
    <div class="grid grid-cols-2 gap-4 px-4">
        <div class="bg-[#1A2130] rounded-xl p-4">
            <span class="material-symbols-outlined text-yellow-400 text-[24px]">lightbulb</span>
            <span class="text-white text-sm mt-2">Lights</span>
        </div>
    </div>
</div>
```

### 中间 JSON（extract-layout 输出）

```json
{
  "screen": {"width": 320, "height": 480},
  "sections": [
    {"id": "section_0", "layout_type": "flex-row-between", "children": [
      {"type": "text", "content": "12:30", "font": {"size": "sm"}, "color": "white"},
      {"type": "icon", "name": "wifi", "size_px": 20, "color": "white"}
    ]},
    {"id": "section_1", "layout_type": "block", "children": [
      {"type": "text", "content": "Smart Home", "font": {"size": "2xl"}, "color": "white"}
    ]},
    {"id": "section_2", "layout_type": "grid-2col", "gap": 16, "children": [
      {"type": "container", "children": [
        {"type": "icon", "name": "lightbulb", "size_px": 24, "color": "yellow-400"},
        {"type": "text", "content": "Lights", "font": {"size": "sm"}, "color": "white"}
      ]}
    ]}
  ]
}
```

### 输出 XML

```xml
<?xml version="1.0" encoding="utf-8"?>
<Page>
    <Group id="root" x="0" y="0" width="320" height="480">
        <Background type="solid" color="EGUI_COLOR_HEX(0x0F172A)" alpha="EGUI_ALPHA_100" />

        <!-- Status Bar -->
        <Group id="status_bar" x="0" y="0" width="320" height="28">
            <Label id="time_label" x="16" y="4" width="80" height="18"
                   text="12:30" font_builtin="&amp;egui_res_font_montserrat_14_4"
                   color="EGUI_COLOR_WHITE" alpha="EGUI_ALPHA_100"
                   align_type="EGUI_ALIGN_LEFT | EGUI_ALIGN_VCENTER" />
            <Image id="wifi_icon" x="284" y="4" width="20" height="20"
                   image_file="icon_wifi.png" image_format="alpha" image_alpha="4"
                   image_color="EGUI_COLOR_WHITE" image_color_alpha="EGUI_ALPHA_100"
                   image_resize="true" />
        </Group>

        <!-- Title -->
        <Label id="title" x="16" y="32" width="200" height="31"
               text="Smart Home" font_builtin="&amp;egui_res_font_montserrat_24_4"
               color="EGUI_COLOR_WHITE" alpha="EGUI_ALPHA_100" />

        <!-- Device Grid -->
        <GridLayout id="devices" x="16" y="72" width="288" height="200"
                    col_count="2" align_type="EGUI_ALIGN_CENTER">
            <Card id="light_card" x="0" y="0" width="136" height="90"
                  corner_radius="12" margin="4">
                <Background type="solid" color="EGUI_COLOR_HEX(0x1A2130)"
                            alpha="EGUI_ALPHA_100" />
                <Image id="light_icon" x="16" y="12" width="24" height="24"
                       image_file="icon_lightbulb.png" image_format="alpha" image_alpha="4"
                       image_color="EGUI_COLOR_HEX(0xFBBF24)"
                       image_color_alpha="EGUI_ALPHA_100" />
                <Label id="light_name" x="16" y="44" width="100" height="18"
                       text="Lights" font_builtin="&amp;egui_res_font_montserrat_14_4"
                       color="EGUI_COLOR_WHITE" alpha="EGUI_ALPHA_100" />
            </Card>
        </GridLayout>
    </Group>
</Page>
```

## XML 规则

编写 XML 布局时必须遵守以下规则：

1. 根元素为 `<Page>`，直接子元素为 `<Group id="root">` 覆盖全屏
2. 所有控件必须有 `id`、`x`、`y`、`width`、`height` 属性
3. `id` 值在页面内唯一，且为合法 C 标识符（小写字母 + 下划线）
4. 字体引用使用 `&amp;` 转义：`font_builtin="&amp;egui_res_font_montserrat_18_4"`
5. Alpha-only 图片必须设置 `image_color` 和 `image_color_alpha`
6. 图片视图尺寸与资源 PNG 尺寸不同时，设置 `image_resize="true"`
7. Label 高度必须 >= 字体行高
8. `<Background>` 是容器的可选子元素，不是属性
9. 颜色使用 EGUI 常量：`EGUI_COLOR_WHITE`、`EGUI_COLOR_HEX(0xRRGGBB)`
10. Group 子元素使用显式 x/y 定位；GridLayout/LinearLayout 子元素自动定位
