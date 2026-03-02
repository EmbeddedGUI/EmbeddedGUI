# 图表控件

## 概述

图表控件用于数据可视化展示。EmbeddedGUI 提供了四种图表：ChartLine 折线图、ChartBar 柱状图、ChartScatter 散点图和 ChartPie 饼图。前三种基于坐标轴(Axis)体系，共享数据系列、坐标轴配置、图例和缩放等能力；ChartPie 使用独立的饼图切片数据模型。

### 公共数据结构

所有坐标轴类图表共享以下数据结构：

```c
// 数据点
typedef struct egui_chart_point
{
    int16_t x;
    int16_t y;
} egui_chart_point_t;

// 数据系列(折线/散点/柱状)
typedef struct egui_chart_series
{
    const egui_chart_point_t *points;
    uint8_t point_count;
    egui_color_t color;
    const char *name; // 图例名称, NULL = 不显示
} egui_chart_series_t;

// 饼图切片
typedef struct egui_chart_pie_slice
{
    uint16_t value;
    egui_color_t color;
    const char *name; // 图例名称, NULL = 不显示
} egui_chart_pie_slice_t;
```

### 坐标轴配置

```c
typedef struct egui_chart_axis_config
{
    int16_t min_value;
    int16_t max_value;
    int16_t tick_step;  // 0 = 自动计算
    uint8_t tick_count; // tick_step==0 时使用, 默认 5
    uint8_t show_grid : 1;
    uint8_t show_labels : 1;
    uint8_t show_axis : 1;
    uint8_t is_categorical : 1; // 1 = 分类轴(柱状图)
    const char *title;          // NULL = 不显示
} egui_chart_axis_config_t;
```

### 图例位置

| 常量 | 说明 |
|------|------|
| `EGUI_CHART_LEGEND_NONE` | 不显示图例 |
| `EGUI_CHART_LEGEND_TOP` | 图例在顶部 |
| `EGUI_CHART_LEGEND_BOTTOM` | 图例在底部 |
| `EGUI_CHART_LEGEND_RIGHT` | 图例在右侧 |

### 坐标轴类图表公共 API

以下 API 通过宏映射，ChartLine/ChartBar/ChartScatter 均可使用(将前缀替换为对应控件名即可)：

| 函数 | 说明 |
|------|------|
| `egui_view_chart_axis_set_series(self, series, count)` | 设置数据系列 |
| `egui_view_chart_axis_set_axis_x(self, min, max, tick_step)` | 设置 X 轴范围 |
| `egui_view_chart_axis_set_axis_y(self, min, max, tick_step)` | 设置 Y 轴范围 |
| `egui_view_chart_axis_set_axis_x_config(self, config)` | 设置 X 轴完整配置 |
| `egui_view_chart_axis_set_axis_y_config(self, config)` | 设置 Y 轴完整配置 |
| `egui_view_chart_axis_set_legend_pos(self, pos)` | 设置图例位置 |
| `egui_view_chart_axis_set_colors(self, bg, axis, grid, text)` | 设置颜色方案 |
| `egui_view_chart_axis_set_font(self, font)` | 设置字体 |

### 缩放 API (需开启 MULTI_TOUCH)

| 函数 | 说明 |
|------|------|
| `egui_view_chart_axis_set_zoom_enabled(self, enabled)` | 启用/禁用缩放 |
| `egui_view_chart_axis_zoom_in(self)` | 放大(双轴) |
| `egui_view_chart_axis_zoom_out(self)` | 缩小(双轴) |
| `egui_view_chart_axis_zoom_reset(self)` | 重置缩放(双轴) |
| `egui_view_chart_axis_zoom_in_x(self)` | 仅 X 轴放大 |
| `egui_view_chart_axis_zoom_out_x(self)` | 仅 X 轴缩小 |
| `egui_view_chart_axis_zoom_in_y(self)` | 仅 Y 轴放大 |
| `egui_view_chart_axis_zoom_out_y(self)` | 仅 Y 轴缩小 |

---

## ChartLine

折线图控件，支持多条数据系列、数据点标记、线宽设置。

### 效果展示

[在线演示](https://embeddedgui.github.io/EmbeddedGUI/basic.html?app=chart_line)

### 专有 API

| 函数 | 说明 |
|------|------|
| `egui_view_chart_line_init(self)` | 初始化 ChartLine |
| `egui_view_chart_line_init_with_params(self, params)` | 使用参数初始化 |
| `egui_view_chart_line_set_line_width(self, width)` | 设置线宽(默认 2) |
| `egui_view_chart_line_set_point_radius(self, radius)` | 设置数据点半径(默认 3) |

### 参数宏

```c
EGUI_VIEW_CHART_LINE_PARAMS_INIT(name, x, y, w, h);
```

### 代码示例

```c
static egui_view_chart_line_t line_chart;

static const egui_chart_point_t temp_data[] = {
    {0, 22}, {1, 24}, {2, 23}, {3, 26},
    {4, 28}, {5, 25}, {6, 23},
};

static const egui_chart_series_t series[] = {
    {temp_data, 7, EGUI_COLOR_MAKE(0x00, 0xAA, 0xFF), "Temp"},
};

EGUI_VIEW_CHART_LINE_PARAMS_INIT(lc_params, 0, 0, 220, 160);

void init_ui(void)
{
    egui_view_chart_line_init_with_params(
        EGUI_VIEW_OF(&line_chart), &lc_params);
    egui_view_chart_line_set_series(
        EGUI_VIEW_OF(&line_chart), series, 1);
    egui_view_chart_line_set_axis_x(
        EGUI_VIEW_OF(&line_chart), 0, 6, 1);
    egui_view_chart_line_set_axis_y(
        EGUI_VIEW_OF(&line_chart), 20, 30, 2);
    egui_view_chart_line_set_legend_pos(
        EGUI_VIEW_OF(&line_chart), EGUI_CHART_LEGEND_TOP);
    egui_view_chart_line_set_line_width(
        EGUI_VIEW_OF(&line_chart), 2);
    egui_view_chart_line_set_point_radius(
        EGUI_VIEW_OF(&line_chart), 3);

    egui_core_add_user_root_view(EGUI_VIEW_OF(&line_chart));
}
```

---

## ChartBar

柱状图控件，支持多组数据系列并排显示，可设置柱间距。

### 效果展示

[在线演示](https://embeddedgui.github.io/EmbeddedGUI/basic.html?app=chart_bar)

### 专有 API

| 函数 | 说明 |
|------|------|
| `egui_view_chart_bar_init(self)` | 初始化 ChartBar |
| `egui_view_chart_bar_init_with_params(self, params)` | 使用参数初始化 |
| `egui_view_chart_bar_set_bar_gap(self, gap)` | 设置柱间距(默认 2) |

### 参数宏

```c
EGUI_VIEW_CHART_BAR_PARAMS_INIT(name, x, y, w, h);
```

### 代码示例

```c
static egui_view_chart_bar_t bar_chart;

static const egui_chart_point_t sales_data[] = {
    {0, 120}, {1, 200}, {2, 150}, {3, 180},
};

static const egui_chart_series_t bar_series[] = {
    {sales_data, 4, EGUI_COLOR_MAKE(0x44, 0xBB, 0x55), "Sales"},
};

EGUI_VIEW_CHART_BAR_PARAMS_INIT(bc_params, 0, 0, 220, 160);

void init_ui(void)
{
    egui_view_chart_bar_init_with_params(
        EGUI_VIEW_OF(&bar_chart), &bc_params);
    egui_view_chart_bar_set_series(
        EGUI_VIEW_OF(&bar_chart), bar_series, 1);
    egui_view_chart_bar_set_axis_x(
        EGUI_VIEW_OF(&bar_chart), 0, 3, 1);
    egui_view_chart_bar_set_axis_y(
        EGUI_VIEW_OF(&bar_chart), 0, 250, 50);
    egui_view_chart_bar_set_bar_gap(
        EGUI_VIEW_OF(&bar_chart), 4);
    egui_view_chart_bar_set_legend_pos(
        EGUI_VIEW_OF(&bar_chart), EGUI_CHART_LEGEND_BOTTOM);

    egui_core_add_user_root_view(EGUI_VIEW_OF(&bar_chart));
}
```

---

## ChartScatter

散点图控件，以圆点形式展示数据分布，支持设置点半径。

### 效果展示

[在线演示](https://embeddedgui.github.io/EmbeddedGUI/basic.html?app=chart_scatter)

### 专有 API

| 函数 | 说明 |
|------|------|
| `egui_view_chart_scatter_init(self)` | 初始化 ChartScatter |
| `egui_view_chart_scatter_init_with_params(self, params)` | 使用参数初始化 |
| `egui_view_chart_scatter_set_point_radius(self, radius)` | 设置点半径(默认 3) |

### 参数宏

```c
EGUI_VIEW_CHART_SCATTER_PARAMS_INIT(name, x, y, w, h);
```

### 代码示例

```c
static egui_view_chart_scatter_t scatter_chart;

static const egui_chart_point_t scatter_data[] = {
    {10, 50}, {20, 80}, {35, 60}, {45, 90},
    {55, 40}, {70, 75}, {85, 55}, {95, 85},
};

static const egui_chart_series_t scatter_series[] = {
    {scatter_data, 8, EGUI_COLOR_MAKE(0xFF, 0x66, 0x00), "Samples"},
};

EGUI_VIEW_CHART_SCATTER_PARAMS_INIT(sc_params, 0, 0, 220, 160);

void init_ui(void)
{
    egui_view_chart_scatter_init_with_params(
        EGUI_VIEW_OF(&scatter_chart), &sc_params);
    egui_view_chart_scatter_set_series(
        EGUI_VIEW_OF(&scatter_chart), scatter_series, 1);
    egui_view_chart_scatter_set_axis_x(
        EGUI_VIEW_OF(&scatter_chart), 0, 100, 20);
    egui_view_chart_scatter_set_axis_y(
        EGUI_VIEW_OF(&scatter_chart), 0, 100, 20);
    egui_view_chart_scatter_set_point_radius(
        EGUI_VIEW_OF(&scatter_chart), 4);

    egui_core_add_user_root_view(EGUI_VIEW_OF(&scatter_chart));
}
```

---

## ChartPie

饼图控件，以扇形展示各部分占比。使用独立的切片数据模型，不依赖坐标轴体系。

### 效果展示

[在线演示](https://embeddedgui.github.io/EmbeddedGUI/basic.html?app=chart_pie)

### API

| 函数 | 说明 |
|------|------|
| `egui_view_chart_pie_init(self)` | 初始化 ChartPie |
| `egui_view_chart_pie_init_with_params(self, params)` | 使用参数初始化 |
| `egui_view_chart_pie_set_slices(self, slices, count)` | 设置饼图切片数据 |
| `egui_view_chart_pie_set_legend_pos(self, pos)` | 设置图例位置 |
| `egui_view_chart_pie_set_colors(self, bg, text)` | 设置背景色和文本色 |
| `egui_view_chart_pie_set_font(self, font)` | 设置字体 |

### 参数宏

```c
EGUI_VIEW_CHART_PIE_PARAMS_INIT(name, x, y, w, h);
```

### 代码示例

```c
static egui_view_chart_pie_t pie_chart;

static const egui_chart_pie_slice_t slices[] = {
    {40, EGUI_COLOR_MAKE(0xFF, 0x44, 0x44), "Code"},
    {25, EGUI_COLOR_MAKE(0x44, 0xBB, 0x55), "Test"},
    {20, EGUI_COLOR_MAKE(0x33, 0x99, 0xFF), "Docs"},
    {15, EGUI_COLOR_MAKE(0xFF, 0xAA, 0x00), "Other"},
};

EGUI_VIEW_CHART_PIE_PARAMS_INIT(pc_params, 0, 0, 220, 180);

void init_ui(void)
{
    egui_view_chart_pie_init_with_params(
        EGUI_VIEW_OF(&pie_chart), &pc_params);
    egui_view_chart_pie_set_slices(
        EGUI_VIEW_OF(&pie_chart), slices, 4);
    egui_view_chart_pie_set_legend_pos(
        EGUI_VIEW_OF(&pie_chart), EGUI_CHART_LEGEND_RIGHT);

    egui_core_add_user_root_view(EGUI_VIEW_OF(&pie_chart));
}
```

### 配置选项

| 宏 | 说明 |
|----|------|
| `EGUI_CONFIG_FUNCTION_SUPPORT_MULTI_TOUCH` | 启用坐标轴类图表的缩放和平移手势 |
