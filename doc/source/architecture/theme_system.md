# 主题系统

## 概述

EmbeddedGUI 提供了一套基于样式描述符的主题系统，支持亮色和暗色两套内置主题，并允许运行时切换。主题通过 `egui_theme_t` 结构体为每种控件定义不同状态下的视觉样式，所有样式数据存储在 ROM 中，零 RAM 开销。

## 架构

```
egui_config_default.h          -- 主题色板宏 (EGUI_THEME_PRIMARY 等)
src/style/egui_style.h         -- 样式结构体和查询 API
src/style/egui_theme.h         -- 主题结构体定义
src/style/egui_theme_light.c   -- 亮色主题样式数据
src/style/egui_theme_dark.c    -- 暗色主题样式数据
src/style/egui_theme.c         -- 主题切换逻辑
```

### 核心数据结构

```c
// 单个样式 (~32 bytes, const ROM)
typedef struct egui_style
{
    uint16_t flags;                  // 哪些属性有效
    egui_color_t bg_color;           // 背景色
    egui_alpha_t bg_alpha;           // 背景透明度
    const egui_gradient_t *bg_gradient; // 渐变背景
    egui_color_t border_color;       // 边框色
    egui_dim_t border_width;         // 边框宽度
    egui_dim_t radius;               // 圆角半径
    egui_dim_t pad_top, pad_bottom, pad_left, pad_right; // 内边距
    egui_color_t text_color;         // 文字颜色
    const egui_font_t *text_font;    // 字体
    const egui_shadow_t *shadow;     // 阴影
} egui_style_t;

// 控件样式描述符: [part][state] 二维查找表
typedef struct egui_widget_style_desc
{
    uint8_t part_count;
    const egui_style_t *const *styles; // styles[part * STATE_MAX + state]
} egui_widget_style_desc_t;

// 主题: 为每种控件提供样式描述符
typedef struct egui_theme
{
    const char *name;
    const egui_widget_style_desc_t *button;
    const egui_widget_style_desc_t *label;
    const egui_widget_style_desc_t *switch_ctrl;
    const egui_widget_style_desc_t *slider;
    const egui_widget_style_desc_t *checkbox;
    const egui_widget_style_desc_t *card;
    const egui_widget_style_desc_t *progress_bar;
    const egui_widget_style_desc_t *circular_progress_bar;
} egui_theme_t;
```

### Part 和 State

样式通过 Part（控件部件）和 State（交互状态）两个维度索引：

| Part | 说明 | 示例 |
|------|------|------|
| `EGUI_PART_MAIN` | 主体 | 按钮背景、标签背景 |
| `EGUI_PART_INDICATOR` | 指示器 | 进度条填充、滑块轨道 |
| `EGUI_PART_KNOB` | 旋钮 | 滑块手柄、开关圆点 |
| `EGUI_PART_SELECTED` | 选中态 | 复选框选中背景 |

| State | 说明 |
|-------|------|
| `EGUI_STATE_NORMAL` | 默认状态 |
| `EGUI_STATE_PRESSED` | 按下状态 |
| `EGUI_STATE_DISABLED` | 禁用状态 |
| `EGUI_STATE_FOCUSED` | 焦点状态 |
| `EGUI_STATE_CHECKED` | 选中状态 |

## 内置主题

### 色板

主题色板通过宏定义在 `egui_config_default.h` 中，可在 `app_egui_config.h` 中覆盖：

| 宏 | 默认值 | 说明 |
|----|--------|------|
| `EGUI_THEME_PRIMARY` | #2563EB (Blue 600) | 主色调 |
| `EGUI_THEME_PRIMARY_DARK` | #1D4ED8 (Blue 700) | 主色调深色（按下态） |
| `EGUI_THEME_PRIMARY_LIGHT` | #3B82F6 (Blue 500) | 主色调浅色 |
| `EGUI_THEME_SECONDARY` | #14B8A6 (Teal 500) | 辅助色 |
| `EGUI_THEME_SUCCESS` | #16A34A (Green 600) | 成功色 |
| `EGUI_THEME_WARNING` | #F59E0B (Amber 500) | 警告色 |
| `EGUI_THEME_DANGER` | #DC2626 (Red 600) | 危险色 |
| `EGUI_THEME_SURFACE` | #FFFFFF | 表面色 |
| `EGUI_THEME_TEXT_PRIMARY` | #111827 (Gray 900) | 主文字色 |
| `EGUI_THEME_TEXT_SECONDARY` | #6B7280 (Gray 500) | 次要文字色 |

### 亮色主题 (egui_theme_light)

定义在 `src/style/egui_theme_light.c`，使用浅色背景、深色文字，带阴影效果。

### 暗色主题 (egui_theme_dark)

定义在 `src/style/egui_theme_dark.c`，使用深色背景、浅色文字，适合低光环境。

## 运行时切换

```c
// 切换到暗色主题
egui_theme_set(&egui_theme_dark);

// 切换到亮色主题
egui_theme_set(&egui_theme_light);

// 获取当前主题
const egui_theme_t *theme = egui_theme_get();
```

`egui_theme_set()` 内部调用 `egui_core_force_refresh()` 强制全屏重绘，确保所有控件立即应用新主题。

## 控件如何响应主题

控件在 `on_draw` 中查询当前主题获取样式，以 button 为例：

```c
void egui_view_button_on_draw(egui_view_t *self)
{
    // 从当前主题获取 button 的样式描述符
    const egui_widget_style_desc_t *desc =
        egui_current_theme ? egui_current_theme->button : NULL;

    // 根据当前状态（normal/pressed/disabled）获取对应样式
    const egui_style_t *style =
        desc ? egui_style_get_current(desc, EGUI_PART_MAIN, self) : NULL;

    if (style)
    {
        // 使用 style->bg_color, style->radius, style->text_color 等绘制
    }
}
```

`egui_style_get_current()` 是一个 O(1) 的内联查找，自动根据视图的 `is_pressed`、`is_enable` 等状态位映射到对应的 `egui_state_t`。

## 自定义主题

### 方法一：覆盖色板宏

在 `app_egui_config.h` 中重定义色板宏，所有内置主题自动使用新颜色：

```c
// app_egui_config.h
#define EGUI_THEME_PRIMARY       EGUI_COLOR_MAKE(0x10, 0xB9, 0x81) // Emerald
#define EGUI_THEME_PRIMARY_DARK  EGUI_COLOR_MAKE(0x05, 0x9D, 0x69)
```

### 方法二：创建全新主题

定义自己的样式数据和主题结构体：

```c
// 定义按钮正常态样式
static const egui_style_t my_btn_normal = {
    .flags = EGUI_STYLE_PROP_BG_COLOR | EGUI_STYLE_PROP_RADIUS | EGUI_STYLE_PROP_TEXT_COLOR,
    .bg_color = EGUI_COLOR_MAKE(0xFF, 0x69, 0x00),
    .bg_alpha = EGUI_ALPHA_COVER,
    .radius = 8,
    .text_color = EGUI_COLOR_WHITE,
    .text_alpha = EGUI_ALPHA_COVER,
};

// ... 定义其他状态和控件的样式 ...

// 组装主题
const egui_theme_t my_custom_theme = {
    .name = "custom",
    .button = &my_btn_desc,
    .label = &my_label_desc,
    // ... 其他控件 ...
};

// 使用
egui_theme_set(&my_custom_theme);
```

主题结构体中未赋值的控件指针为 NULL，对应控件会回退到硬编码的默认外观。
