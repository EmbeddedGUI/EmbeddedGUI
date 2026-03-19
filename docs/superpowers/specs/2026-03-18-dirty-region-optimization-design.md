# 脏区域优化设计文档

## 概述

优化 EmbeddedGUI 的脏区域（dirty region）系统，使控件能够精确标记变化的子区域，并在 `on_draw` 内部利用 PFB tile 相关性检查跳过不需要绘制的子元素。目标是减少不必要的重绘面积和绘制计算开销。

## 背景与问题

### 现状

- 脏区域系统在屏幕级别维护最多 `EGUI_CONFIG_DIRTY_AREA_COUNT`（默认 5）个脏区域
- 控件通过 `egui_view_invalidate()` 标记整个 `region_screen` 为脏
- `egui_view_group_draw()` 对子控件做 PFB early culling（跳过不在 PFB tile 内的子控件）
- 控件内部的 `on_draw()` 不做 PFB 相关性检查，绘制所有子元素

### 问题

1. **脏区域过大**：控件任何局部变化都标记整个控件区域为脏。例如：
   - textinput 光标闪烁：1×16 像素的变化导致 200×30 的重绘
   - number_picker 按压：1/3 区域变化导致整个控件重绘
   - mini_calendar 日期选择：1 个单元格变化导致整个日历重绘

2. **on_draw 内部无 PFB 检查**：即使当前 PFB tile 只覆盖控件的一小部分，on_draw 仍然执行所有子元素的计算和绘制逻辑（虽然 canvas 会裁剪实际像素输出，但计算开销无法避免）

## 设计方案

采用两层 API 设计：基础层供所有控件使用，高级层供复杂网格型控件可选使用。

### 1. 基础层：子区域脏标记 + PFB 检查工具

#### 1.1 `egui_view_invalidate_region()`

```c
// egui_view.h 新增声明
void egui_view_invalidate_region(egui_view_t *self, const egui_region_t *dirty_region);
```

```c
// egui_view.c 实现
void egui_view_invalidate_region(egui_view_t *self, const egui_region_t *dirty_region)
{
    if (!egui_view_is_visible(self) || self->is_gone)
    {
        return;
    }

    // 转换为屏幕坐标
    EGUI_REGION_DEFINE(screen_region,
        self->region_screen.location.x + dirty_region->location.x,
        self->region_screen.location.y + dirty_region->location.y,
        dirty_region->size.width,
        dirty_region->size.height);

    // 裁剪到控件屏幕区域内
    egui_region_intersect(&screen_region, &self->region_screen, &screen_region);

    if (!egui_region_is_empty(&screen_region))
    {
        egui_core_update_region_dirty(&screen_region);
    }
}
```

语义：
- `dirty_region` 使用控件屏幕坐标系内的相对偏移（相对于 `region_screen.location`）
- 自动转换为屏幕绝对坐标并裁剪到控件屏幕区域内
- 不触发 `request_layout`（布局未变，仅内容变化）
- 不可见或 `is_gone` 的控件调用被忽略
- 注意：对于有 padding 的控件，调用者需要自行加上 padding 偏移（因为 work region 起点 = padding.left, padding.top）

#### 1.2 `egui_canvas_is_region_active()`

```c
// egui_canvas.h 新增
static inline int egui_canvas_is_region_active(const egui_region_t *region)
{
    return egui_region_is_intersect(region, egui_canvas_get_pfb_region());
}
```

语义：
- 检查给定屏幕坐标区域是否与当前 PFB tile 有交集
- 返回 1 表示需要绘制，0 表示可跳过
- O(1) 开销（4 次整数比较）
- 传入的 region 必须是屏幕坐标系

### 2. 高级层：子区域注册表（可选）

通过编译开关 `EGUI_CONFIG_FUNCTION_SUPPORT_SUB_DIRTY_REGION` 控制。

#### 2.1 数据结构

```c
typedef struct egui_sub_region
{
    egui_region_t region;  // 控件本地坐标
} egui_sub_region_t;

typedef struct egui_sub_region_table
{
    egui_sub_region_t *regions;
    uint16_t count;
} egui_sub_region_table_t;
```

#### 2.2 API

```c
void egui_view_invalidate_sub_region(egui_view_t *self,
    const egui_sub_region_table_t *table, uint16_t index);
```

语义：
- 通过索引标记子区域为脏
- 内部调用 `egui_view_invalidate_region()` 完成实际工作
- 索引越界时忽略（安全保护）
- 子区域表由控件在 `calculate_layout` 时填充

## 各控件优化策略

### textinput

**光标闪烁优化：**
- `cursor_timer_callback` 中：用 `egui_view_invalidate_region()` 只标记光标矩形区域（约 1×font_height）
- 光标 toggle（显示/隐藏）时只需 invalidate 当前光标位置（位置不变）
- 光标移动时（如 set_cursor_pos）需要 invalidate 旧位置和新位置两个区域
- 文字内容变化时仍使用 `egui_view_invalidate()` 全量更新

**on_draw PFB 检查：**
- 将绘制逻辑分为：背景容器、文字内容、光标三个区域
- 对文字区域和光标区域分别做 `egui_canvas_is_region_active()` 检查

预期收益：光标闪烁脏区域减少约 99%。

### number_picker

**按压区域优化：**
- `on_touch_event` 中：按压/释放时只 invalidate 变化的 1/3 区域
- 数值变化时只 invalidate 中间数字区域

**on_draw PFB 检查：**
- 对 top（上箭头）、mid（数字）、bottom（下箭头）三个区域分别做检查

预期收益：按压时脏区域缩小到 1/3。

### mini_calendar

**日期选择优化：**
- 选择日期时：只 invalidate 旧选中日期和新选中日期的两个单元格区域
- 月份/年份切换时：仍使用全量 invalidate

**on_draw PFB 检查：**
- 日期网格绘制循环中，对每个单元格做 `egui_canvas_is_region_active()` 检查
- 跳过不在 PFB tile 内的单元格（避免 sprintf、字体度量等计算）

预期收益：日期选择时脏区域减少约 95%。

### keyboard

**现状分析：**
- keyboard 是 `egui_view_group_t`，包含 31 个按钮子控件
- 按键按压已经是单按钮级别的 invalidate（通过 `egui_view_set_pressed`）
- `egui_view_group_draw` 已有 PFB early culling

**优化点：**
- 模式切换（大小写/符号）是合理的全量更新，保持现状
- 确保显示/隐藏时的脏区域标记精确
- keyboard 的优化空间相对较小

### group 控件通用优化

- 确保 `egui_view_group_request_layout` 的级联行为合理
- 在 `calculate_layout` 中只对 `is_request_layout == true` 的子控件重算布局

## 测试策略

### 单元测试（HelloUnitTest）

在现有 `test_dirty_region.c` 基础上新增测试文件。

**子区域 invalidate 测试：**
- `test_invalidate_region_basic` — 子区域正确转换为屏幕坐标并添加到脏区域数组
- `test_invalidate_region_clamp` — 子区域超出控件边界时被正确裁剪
- `test_invalidate_region_invisible` — 不可见控件的 invalidate_region 被忽略
- `test_invalidate_region_multiple` — 多次子区域 invalidate 的合并行为
- `test_invalidate_region_vs_full` — 子区域 vs 全量 invalidate 的脏区域面积差异

**PFB 相关性检查测试：**
- `test_canvas_is_region_active_inside` — 区域完全在 PFB tile 内
- `test_canvas_is_region_active_outside` — 区域完全在 PFB tile 外
- `test_canvas_is_region_active_partial` — 区域部分重叠
- `test_canvas_is_region_active_edge` — 边界情况

**子区域注册表测试（启用时）：**
- `test_sub_region_table_invalidate` — 通过索引 invalidate 子区域
- `test_sub_region_table_bounds` — 索引越界保护

### 运行时测试（HelloBasic）

对优化后的控件在现有 test.c 中增加验证：
- textinput：模拟光标闪烁后截图验证
- number_picker：模拟按压增减后截图验证
- mini_calendar：模拟日期选择后截图验证

### 调试工具（可选）

新增编译开关 `EGUI_CONFIG_DEBUG_DIRTY_REGION_STATS`，启用后统计：
- 每帧脏区域总面积
- 每帧 PFB tile 总数
- 累计统计信息

## 文件变更清单

### 新增文件
- `example/HelloUnitTest/test/test_invalidate_region.c` — 子区域 invalidate 单元测试
- `example/HelloUnitTest/test/test_canvas_active.c` — PFB 检查单元测试

### 修改文件
- `src/widget/egui_view.h` — 新增 `egui_view_invalidate_region()` 声明
- `src/widget/egui_view.c` — 新增 `egui_view_invalidate_region()` 实现
- `src/core/egui_canvas.h` — 新增 `egui_canvas_is_region_active()` 内联函数
- `src/widget/egui_view_textinput.c` — 光标闪烁子区域 invalidate + on_draw PFB 检查
- `src/widget/egui_view_number_picker.c` — 按压子区域 invalidate + on_draw PFB 检查
- `src/widget/egui_view_mini_calendar.c` — 日期选择子区域 invalidate + on_draw PFB 检查
- `src/core/egui_config_default.h` — 新增 `EGUI_CONFIG_FUNCTION_SUPPORT_SUB_DIRTY_REGION` 和 `EGUI_CONFIG_DEBUG_DIRTY_REGION_STATS` 开关
- `example/HelloUnitTest/uicode.c` — 注册新测试套件

## 风险与缓解

| 风险 | 缓解措施 |
|------|----------|
| 子区域计算错误导致渲染残留 | 单元测试覆盖边界情况；运行时截图对比验证 |
| 子区域 invalidate 遗漏导致不刷新 | 保留 `egui_view_invalidate()` 作为 fallback；控件可随时回退到全量 invalidate |
| PFB 检查引入的分支开销 | 检查是 O(1) 的 4 次比较，远小于跳过的绘制开销 |
| 注册表内存开销 | 通过编译开关控制，仅需要的控件启用 |
