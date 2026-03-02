# 渲染管线

## 概述

EmbeddedGUI 的渲染管线基于 PFB（Partial Frame Buffer）分块机制，将屏幕划分为多个小块逐块渲染。整个流程从视图调用 `egui_view_invalidate()` 标记脏区域开始，经过布局计算、PFB 分块遍历、Canvas 绘制，最终输出到显示设备。

## 从 invalidate 到屏幕的完整流程

```
1. egui_view_invalidate()
   |-- 标记 is_request_layout = true
   |-- 向上传播到父视图
   v
2. egui_core_update_region_dirty()
   |-- 将变化区域合并到 region_dirty_arr[]
   v
3. egui_check_need_refresh()
   |-- 检查 dirty 数组是否有非空区域
   v
4. egui_core_draw_view_group_pre_work()
   |-- compute_scroll(): 计算滚动偏移
   |-- calculate_layout(): 递归计算所有视图的 region_screen
   v
5. egui_polling_refresh_display()
   |-- 遍历每个脏区域
   |   +-- egui_core_draw_view_group(p_region_dirty)
   |       |-- 将脏区域划分为 PFB 大小的块
   |       |-- 对每个 PFB 块:
   |       |   +-- egui_canvas_init(): 初始化画布
   |       |   +-- egui_api_pfb_clear(): 清空 PFB 缓冲
   |       |   +-- view_group.draw(): 递归绘制视图树
   |       |   +-- egui_core_draw_data(): 输出到显示设备
   v
6. egui_core_clear_region_dirty()
   |-- 清空脏区域数组
```

## PFB 分块遍历机制

脏区域可能大于 PFB 缓冲区，因此需要分块遍历。核心逻辑在 `egui_core_draw_view_group()` 中：

```c
// 计算需要多少个 PFB 块覆盖脏区域
pfb_width_count  = (width_dirty + pfb_width - 1) / pfb_width;
pfb_height_count = (height_dirty + pfb_height - 1) / pfb_height;

// 逐块遍历
for (y = 0; y < pfb_height_count; y++)
{
    for (x = 0; x < pfb_width_count; x++)
    {
        // 1. 计算当前块的实际尺寸（边缘块可能小于标准 PFB）
        tmp_pfb_width  = MIN(pfb_width,  x_pos_base + width_dirty - x_pos);
        tmp_pfb_height = MIN(pfb_height, y_pos_base + height_dirty - y_pos);

        // 2. 初始化 Canvas，绑定 PFB 缓冲区
        egui_canvas_init(pfb, &region);

        // 3. 清空 PFB
        egui_api_pfb_clear(pfb, buffer_size);

        // 4. 递归绘制视图树（只绘制与当前 PFB 块相交的视图）
        view_group->base.api->draw((egui_view_t *)view_group);

        // 5. 将 PFB 数据刷新到显示设备
        egui_core_draw_data(&region);
    }
}
```

自适应优化：当脏区域小于 PFB 时，系统会动态调整 PFB 尺寸以充分利用缓冲区内存。

## Canvas API

Canvas 是所有绘制操作的统一接口，定义在 `src/core/egui_canvas.h`。所有坐标均为屏幕绝对坐标，Canvas 内部自动处理与 PFB 块的裁剪和坐标转换。

### Canvas 结构体

```c
struct egui_canvas
{
    egui_color_int_t *pfb;                    // PFB 缓冲区指针
    egui_region_t pfb_region;                 // PFB 在屏幕上的区域
    egui_region_t base_view_work_region;      // 有效绘制区域
    egui_location_t pfb_location_in_base_view;// PFB 在基础视图中的偏移
    egui_alpha_t alpha;                       // 全局 alpha
    egui_mask_t *mask;                        // 当前遮罩
};
```

### 基本图形

| API | 说明 |
|-----|------|
| `egui_canvas_draw_point(x, y, color, alpha)` | 画点 |
| `egui_canvas_draw_line(x1, y1, x2, y2, width, color, alpha)` | 画线 |
| `egui_canvas_draw_hline(x, y, len, color, alpha)` | 水平线 |
| `egui_canvas_draw_vline(x, y, len, color, alpha)` | 垂直线 |
| `egui_canvas_draw_rectangle(x, y, w, h, stroke, color, alpha)` | 矩形边框 |
| `egui_canvas_draw_rectangle_fill(x, y, w, h, color, alpha)` | 填充矩形 |
| `egui_canvas_draw_round_rectangle(x, y, w, h, r, stroke, color, alpha)` | 圆角矩形边框 |
| `egui_canvas_draw_round_rectangle_fill(x, y, w, h, r, color, alpha)` | 填充圆角矩形 |
| `egui_canvas_draw_circle(cx, cy, r, stroke, color, alpha)` | 圆形边框 |
| `egui_canvas_draw_circle_fill(cx, cy, r, color, alpha)` | 填充圆形 |
| `egui_canvas_draw_arc(cx, cy, r, start, end, stroke, color, alpha)` | 圆弧 |
| `egui_canvas_draw_arc_fill(cx, cy, r, start, end, color, alpha)` | 填充扇形 |
| `egui_canvas_draw_triangle(x1,y1, x2,y2, x3,y3, color, alpha)` | 三角形边框 |
| `egui_canvas_draw_triangle_fill(x1,y1, x2,y2, x3,y3, color, alpha)` | 填充三角形 |

### 文本绘制

| API | 说明 |
|-----|------|
| `egui_canvas_draw_text(font, str, x, y, color, alpha)` | 在指定位置绘制文本 |
| `egui_canvas_draw_text_in_rect(font, str, rect, align, color, alpha)` | 在矩形区域内绘制文本（支持对齐） |
| `egui_canvas_draw_text_in_rect_with_line_space(...)` | 带行间距的文本绘制 |

### 图片绘制

| API | 说明 |
|-----|------|
| `egui_canvas_draw_image(img, x, y)` | 绘制图片 |
| `egui_canvas_draw_image_resize(img, x, y, w, h)` | 缩放绘制图片 |
| `egui_canvas_draw_image_color(img, x, y, color, alpha)` | 带颜色混合的图片绘制 |

### 高级图形（需配置开启）

| API | 配置宏 |
|-----|--------|
| `egui_canvas_draw_ellipse()` / `_fill()` | `EGUI_CONFIG_FUNCTION_CANVAS_DRAW_ELLIPSE` |
| `egui_canvas_draw_polygon()` / `_fill()` | `EGUI_CONFIG_FUNCTION_CANVAS_DRAW_POLYGON` |
| `egui_canvas_draw_bezier_quad()` / `_cubic()` | `EGUI_CONFIG_FUNCTION_CANVAS_DRAW_BEZIER` |

## 裁剪 (Clip) 机制

Canvas 的裁剪通过 `base_view_work_region` 实现。每个视图在绘制前调用 `egui_canvas_calc_work_region()` 计算当前视图区域与 PFB 块的交集：

```c
// egui_view_draw() 中
egui_canvas_calc_work_region(&self->region_screen);

if (!egui_region_is_empty(egui_canvas_get_base_view_work_region()))
{
    // 只有交集非空才执行绘制
    egui_view_draw_background(self);
    self->api->on_draw(self);
}
```

所有 Canvas 绘制函数内部都会检查像素是否在 `base_view_work_region` 内，超出区域的像素自动跳过。这实现了自然的父视图裁剪效果 -- 子视图超出父视图边界的部分不会被绘制。

## Alpha 混合

### 全局 Alpha

Canvas 维护一个全局 alpha 值，所有绘制操作的 alpha 会与之混合：

```c
// 设置全局 alpha
egui_canvas_set_alpha(alpha);

// 混合 alpha（用于视图嵌套时的 alpha 叠加）
egui_canvas_mix_alpha(view_alpha);

// alpha 混合公式
result_alpha = (alpha_0 * alpha_1 + 128) >> 8;
```

视图绘制时，`egui_view_draw()` 会将视图自身的 alpha 混合到 Canvas 全局 alpha，绘制完成后恢复。这样嵌套视图的透明度会自动叠加。

### 像素级 Alpha 混合

当 alpha 不为 255（完全不透明）时，使用以下公式混合前景色和背景色：

```c
result.r = (fore_alpha * fore.r + (255 - fore_alpha) * back.r + 128) >> 8;
result.g = (fore_alpha * fore.g + (255 - fore_alpha) * back.g + 128) >> 8;
result.b = (fore_alpha * fore.b + (255 - fore_alpha) * back.b + 128) >> 8;
```

## Mask 遮罩支持

Mask 提供像素级的 alpha 调制，用于实现圆角裁剪、不规则形状等效果。

### Mask 接口

```c
struct egui_mask_api
{
    // 逐像素遮罩：修改该点的颜色和 alpha
    void (*mask_point)(egui_mask_t *self, egui_dim_t x, egui_dim_t y,
                       egui_color_t *color, egui_alpha_t *alpha);

    // 行范围查询（可选优化）：返回该行的不透明区间
    int (*mask_get_row_range)(egui_mask_t *self, egui_dim_t y,
                              egui_dim_t x_min, egui_dim_t x_max,
                              egui_dim_t *x_start, egui_dim_t *x_end);
};
```

使用方式：

```c
// 设置遮罩
egui_canvas_set_mask(&my_mask);

// 绘制操作会自动应用遮罩
egui_canvas_draw_rectangle_fill(x, y, w, h, color, alpha);

// 清除遮罩
egui_canvas_clear_mask();
```

Canvas 在绘制每个像素时检查 mask 是否存在，若存在则调用 `mask_point()` 调制 alpha 值。支持 `mask_get_row_range` 的 mask 实现可以跳过整行或批量处理不透明区间，显著提升性能。

## 抗锯齿 (HQ 后缀函数)

圆形和弧线绘制提供两套实现：

| 类型 | 后缀 | 特点 |
|------|------|------|
| Basic | `_basic` | 使用预计算查找表，速度快，半径受限 |
| HQ | `_hq` | 运行时子像素采样，抗锯齿效果好，无半径限制 |

通过配置宏选择默认实现：

```c
// egui_canvas.h 中
#if EGUI_CONFIG_CIRCLE_DEFAULT_ALGO_HQ
#define egui_canvas_draw_circle      egui_canvas_draw_circle_hq
#define egui_canvas_draw_circle_fill egui_canvas_draw_circle_fill_hq
#define egui_canvas_draw_arc         egui_canvas_draw_arc_hq
#define egui_canvas_draw_arc_fill    egui_canvas_draw_arc_fill_hq
#else
#define egui_canvas_draw_circle      egui_canvas_draw_circle_basic
// ...
#endif
```

HQ 系列还包括线段和折线的抗锯齿版本（需启用 `EGUI_CONFIG_FUNCTION_CANVAS_DRAW_LINE_HQ`）：

| API | 说明 |
|-----|------|
| `egui_canvas_draw_line_hq()` | 抗锯齿线段 |
| `egui_canvas_draw_line_round_cap_hq()` | 圆头线段 |
| `egui_canvas_draw_polyline_hq()` | 抗锯齿折线 |
| `egui_canvas_draw_arc_round_cap_hq()` | 圆头弧线 |

## 渲染相关配置

| 配置宏 | 说明 |
|--------|------|
| `EGUI_CONFIG_PFB_WIDTH` | PFB 块宽度 |
| `EGUI_CONFIG_PFB_HEIGHT` | PFB 块高度 |
| `EGUI_CONFIG_DIRTY_AREA_COUNT` | 脏区域数组大小 |
| `EGUI_CONFIG_MAX_FPS` | 最大帧率 |
| `EGUI_CONFIG_COLOR_DEPTH` | 色深 (8/16/32) |
| `EGUI_CONFIG_CIRCLE_DEFAULT_ALGO_HQ` | 默认使用 HQ 圆形算法 |
| `EGUI_CONFIG_DEBUG_PFB_REFRESH` | 调试模式：显示 PFB 块边界 |
| `EGUI_CONFIG_DEBUG_DIRTY_REGION_REFRESH` | 调试模式：显示脏区域边界 |
