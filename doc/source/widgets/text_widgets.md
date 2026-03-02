# 文本控件

## 概述

文本控件用于展示各种形式的文本内容。Textblock 支持多行文本、滚动和编辑；DynamicLabel 用于运行时频繁更新的短文本；Spangroup 支持在同一区域内混排不同字体和颜色的文本片段。

## Textblock

多行文本块控件，支持自动换行、滚动条、边框、对齐方式，以及可选的编辑模式。

### 效果展示

[在线演示](https://embeddedgui.github.io/EmbeddedGUI/basic.html?app=textblock)

### API

| 函数 | 说明 |
|------|------|
| `egui_view_textblock_init(self)` | 初始化 Textblock |
| `egui_view_textblock_init_with_params(self, params)` | 使用参数初始化 |
| `egui_view_textblock_set_text(self, text)` | 设置文本内容 |
| `egui_view_textblock_set_font(self, font)` | 设置字体 |
| `egui_view_textblock_set_font_color(self, color, alpha)` | 设置字体颜色和透明度 |
| `egui_view_textblock_set_align_type(self, align_type)` | 设置对齐方式 |
| `egui_view_textblock_set_line_space(self, line_space)` | 设置行间距 |
| `egui_view_textblock_set_max_lines(self, max_lines)` | 设置最大行数(0=不限) |
| `egui_view_textblock_set_auto_height(self, is_auto_height)` | 启用自动高度 |
| `egui_view_textblock_get_text_size(self, text, max_width, width, height)` | 计算文本渲染尺寸 |
| `egui_view_textblock_set_border_enabled(self, enabled)` | 启用/禁用边框 |
| `egui_view_textblock_set_border_radius(self, radius)` | 设置边框圆角半径 |
| `egui_view_textblock_set_border_color(self, color)` | 设置边框颜色 |
| `egui_view_textblock_set_scrollbar_enabled(self, enabled)` | 启用/禁用滚动条 |
| `egui_view_textblock_set_editable(self, is_editable)` | 启用/禁用编辑模式 |
| `egui_view_textblock_insert_char(self, c)` | 在光标处插入字符 |
| `egui_view_textblock_delete_char(self)` | 删除光标前字符 |
| `egui_view_textblock_set_cursor_pos(self, pos)` | 设置光标位置 |
| `egui_view_textblock_set_cursor_color(self, color)` | 设置光标颜色 |
| `egui_view_textblock_get_edit_text(self)` | 获取编辑缓冲区文本 |

### 参数宏

```c
// 完整参数初始化
EGUI_VIEW_TEXTBLOCK_PARAMS_INIT(name, x, y, w, h, text, font, color, alpha);

// 简化参数初始化(使用默认字体和白色)
EGUI_VIEW_TEXTBLOCK_PARAMS_INIT_SIMPLE(name, x, y, w, h, text);
```

### 代码示例

```c
static egui_view_textblock_t textblock;

EGUI_VIEW_TEXTBLOCK_PARAMS_INIT(tb_params, 10, 10, 220, 80,
    "Multi-line\ntext block\nexample",
    EGUI_CONFIG_FONT_DEFAULT, EGUI_THEME_TEXT, EGUI_ALPHA_100);

void init_ui(void)
{
    egui_view_textblock_init_with_params(EGUI_VIEW_OF(&textblock), &tb_params);
    egui_view_textblock_set_align_type(EGUI_VIEW_OF(&textblock), EGUI_ALIGN_LEFT | EGUI_ALIGN_TOP);
    egui_view_textblock_set_line_space(EGUI_VIEW_OF(&textblock), 4);
    egui_view_set_padding(EGUI_VIEW_OF(&textblock), 4, 4, 4, 4);

    // 启用圆角边框
    egui_view_textblock_set_border_enabled(EGUI_VIEW_OF(&textblock), 1);
    egui_view_textblock_set_border_radius(EGUI_VIEW_OF(&textblock), 6);
    egui_view_textblock_set_border_color(EGUI_VIEW_OF(&textblock), EGUI_THEME_PRIMARY_DARK);

    egui_core_add_user_root_view(EGUI_VIEW_OF(&textblock));
}
```

### 配置选项

| 宏 | 说明 |
|----|------|
| `EGUI_CONFIG_FUNCTION_SUPPORT_SCROLLBAR` | 启用滚动条功能 |
| `EGUI_CONFIG_FUNCTION_SUPPORT_KEY` | 启用按键输入(编辑模式依赖) |
| `EGUI_CONFIG_FUNCTION_SUPPORT_FOCUS` | 启用焦点(编辑模式依赖) |
| `EGUI_CONFIG_TEXTBLOCK_EDIT_MAX_LENGTH` | 编辑缓冲区最大长度 |

---

## DynamicLabel

动态标签控件，继承自 Label，内置固定大小的文本缓冲区，适合运行时频繁更新文本内容的场景(如实时数据显示)。

### 效果展示

DynamicLabel 的外观与 Label 一致，区别在于内部维护了独立的文本缓冲区。

### API

| 函数 | 说明 |
|------|------|
| `egui_view_dynamic_label_init(self)` | 初始化 DynamicLabel |
| `egui_view_dynamic_label_init_with_params(self, params)` | 使用参数初始化 |
| `egui_view_dynamic_label_set_text(self, text)` | 设置文本(拷贝到内部缓冲区) |

DynamicLabel 继承 Label 的全部 API，可直接使用 `egui_view_label_set_font()`、`egui_view_label_set_font_color()` 等函数。

### 参数宏

```c
// 复用 Label 参数宏
EGUI_VIEW_DYNAMIC_LABEL_PARAMS_INIT(name, x, y, w, h, text, font, color, alpha);
EGUI_VIEW_DYNAMIC_LABEL_PARAMS_INIT_SIMPLE(name, x, y, w, h, text);
```

### 代码示例

```c
static egui_view_dynamic_label_t dyn_label;

EGUI_VIEW_DYNAMIC_LABEL_PARAMS_INIT_SIMPLE(dyn_params, 10, 10, 100, 24, "0");

void init_ui(void)
{
    egui_view_dynamic_label_init_with_params(EGUI_VIEW_OF(&dyn_label), &dyn_params);
    egui_core_add_user_root_view(EGUI_VIEW_OF(&dyn_label));
}

// 运行时更新文本
void update_value(int val)
{
    char buf[20];
    snprintf(buf, sizeof(buf), "%d", val);
    egui_view_dynamic_label_set_text(EGUI_VIEW_OF(&dyn_label), buf);
}
```

### 配置选项

| 宏 | 说明 |
|----|------|
| `EGUI_VIEW_DYNAMIC_LABEL_MAX_SIZE` | 内部文本缓冲区大小，默认 20 字节 |

---

## Spangroup

富文本控件，支持在同一区域内添加多个文本片段(Span)，每个片段可独立设置字体和颜色。

### 效果展示

[在线演示](https://embeddedgui.github.io/EmbeddedGUI/basic.html?app=spangroup)

### API

| 函数 | 说明 |
|------|------|
| `egui_view_spangroup_init(self)` | 初始化 Spangroup |
| `egui_view_spangroup_init_with_params(self, params)` | 使用参数初始化 |
| `egui_view_spangroup_add_span(self, text, font, color)` | 添加一个文本片段 |
| `egui_view_spangroup_clear(self)` | 清除所有片段 |
| `egui_view_spangroup_set_align(self, align)` | 设置对齐方式 |
| `egui_view_spangroup_set_line_spacing(self, spacing)` | 设置行间距 |

### 参数宏

```c
EGUI_VIEW_SPANGROUP_PARAMS_INIT(name, x, y, w, h);
```

### 代码示例

```c
static egui_view_spangroup_t spangroup;

EGUI_VIEW_SPANGROUP_PARAMS_INIT(sg_params, 10, 10, 220, 140);

void init_ui(void)
{
    egui_view_spangroup_init_with_params(EGUI_VIEW_OF(&spangroup), &sg_params);
    egui_view_spangroup_set_line_spacing(EGUI_VIEW_OF(&spangroup), 6);

    egui_view_spangroup_add_span(EGUI_VIEW_OF(&spangroup),
        "EmbeddedGUI ", (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT, EGUI_THEME_TEXT);
    egui_view_spangroup_add_span(EGUI_VIEW_OF(&spangroup),
        "SpanGroup", (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT, EGUI_THEME_PRIMARY);
    egui_view_spangroup_add_span(EGUI_VIEW_OF(&spangroup),
        "  Demo", (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT, EGUI_THEME_TEXT_SECONDARY);

    egui_core_add_user_root_view(EGUI_VIEW_OF(&spangroup));
}
```

### 配置选项

| 宏 | 说明 |
|----|------|
| `EGUI_VIEW_SPANGROUP_MAX_SPANS` | 最大 Span 数量，默认 8 |
