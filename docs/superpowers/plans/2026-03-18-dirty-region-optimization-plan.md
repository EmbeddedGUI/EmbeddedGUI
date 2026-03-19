# 脏区域优化实现计划

基于设计文档：`docs/superpowers/specs/2026-03-18-dirty-region-optimization-design.md`

## 实现顺序

按依赖关系排序：先核心 API，再控件优化，最后测试。

---

## Step 1: 核心 API — `egui_view_invalidate_region()`

### 修改文件

**`src/widget/egui_view.h`** (line 131 附近)
- 在 `egui_view_invalidate()` 声明之后添加：
```c
void egui_view_invalidate_region(egui_view_t *self, const egui_region_t *dirty_region);
```

**`src/widget/egui_view.c`** (line 40 附近，`egui_view_invalidate` 之后)
- 添加实现：
```c
void egui_view_invalidate_region(egui_view_t *self, const egui_region_t *dirty_region)
{
    if (!egui_view_is_visible(self))
    {
        return;
    }

    EGUI_REGION_DEFINE(screen_region,
        self->region_screen.location.x + dirty_region->location.x,
        self->region_screen.location.y + dirty_region->location.y,
        dirty_region->size.width,
        dirty_region->size.height);

    egui_region_intersect(&screen_region, &self->region_screen, &screen_region);

    if (!egui_region_is_empty(&screen_region))
    {
        egui_core_update_region_dirty(&screen_region);
    }
}
```

### 验证
- 编译通过
- 现有测试不受影响

---

## Step 2: 核心 API — `egui_canvas_is_region_active()`

### 修改文件

**`src/core/egui_canvas.h`** (line 497 附近，`egui_canvas_set_rect_color_intersect` 之后，`egui_canvas_get_canvas` 之前)
- 添加内联函数：
```c
__EGUI_STATIC_INLINE__ int egui_canvas_is_region_active(const egui_region_t *region)
{
    return egui_region_is_intersect((egui_region_t *)region, egui_canvas_get_pfb_region());
}
```
- 注意：`egui_region_is_intersect` 的第一个参数是 `egui_region_t *`（非 const），需要 cast

### 验证
- 编译通过

---

## Step 3: 编译开关

### 修改文件

**`src/core/egui_config_default.h`** (line 169 附近，`EGUI_CONFIG_FUNCTION_SUPPORT_SCROLLBAR` 之后)
- 添加：
```c
/**
 * Function options.
 * Enable sub-region dirty tracking for fine-grained widget invalidation. if 0, disable.
 */
#ifndef EGUI_CONFIG_FUNCTION_SUPPORT_SUB_DIRTY_REGION
#define EGUI_CONFIG_FUNCTION_SUPPORT_SUB_DIRTY_REGION 0
#endif

/**
 * Debug options.
 * Enable dirty region statistics logging for performance analysis. if 0, disable.
 */
#ifndef EGUI_CONFIG_DEBUG_DIRTY_REGION_STATS
#define EGUI_CONFIG_DEBUG_DIRTY_REGION_STATS 0
#endif
```

### 验证
- 编译通过

---

## Step 4: 高级 API — 子区域注册表（可选）

### 修改文件

**`src/widget/egui_view.h`**
- 在 `egui_view_invalidate_region` 声明之后，添加条件编译块：
```c
#if EGUI_CONFIG_FUNCTION_SUPPORT_SUB_DIRTY_REGION
typedef struct egui_sub_region
{
    egui_region_t region;
} egui_sub_region_t;

typedef struct egui_sub_region_table
{
    egui_sub_region_t *regions;
    uint16_t count;
} egui_sub_region_table_t;

void egui_view_invalidate_sub_region(egui_view_t *self,
    const egui_sub_region_table_t *table, uint16_t index);
#endif
```

**`src/widget/egui_view.c`**
- 添加实现：
```c
#if EGUI_CONFIG_FUNCTION_SUPPORT_SUB_DIRTY_REGION
void egui_view_invalidate_sub_region(egui_view_t *self,
    const egui_sub_region_table_t *table, uint16_t index)
{
    if (index >= table->count)
    {
        return;
    }
    egui_view_invalidate_region(self, &table->regions[index].region);
}
#endif
```

### 验证
- 编译通过（开关默认关闭，不影响现有代码）
- 开启开关后编译通过

---

## Step 5: textinput 控件优化

### 修改文件

**`src/widget/egui_view_textinput.c`**

#### 5a: 光标闪烁子区域 invalidate

修改 `egui_view_textinput_cursor_timer_callback` (line 69-79):
- 替换 `egui_view_invalidate(self)` 为计算光标区域后调用 `egui_view_invalidate_region()`
- 光标区域 = `{padding.left + cursor_x_in_work - scroll_offset_x, padding.top + cursor_y_offset, EGUI_TEXTINPUT_CURSOR_WIDTH, cursor_height}`
- 需要在 callback 中计算光标位置（复用 `egui_view_textinput_get_text_width_to_pos`）和字体高度

具体实现：
```c
static void egui_view_textinput_cursor_timer_callback(egui_timer_t *timer)
{
    egui_view_textinput_t *local = (egui_view_textinput_t *)timer->user_data;
    egui_view_t *self = (egui_view_t *)local;

    local->cursor_visible = !local->cursor_visible;

    // Only invalidate cursor region instead of entire widget
    if (local->font != NULL)
    {
        egui_dim_t cursor_x = egui_view_textinput_get_text_width_to_pos(local, local->cursor_pos) - local->scroll_offset_x;
        egui_dim_t dummy_w = 0;
        egui_dim_t cursor_h = 0;
        local->font->api->get_str_size(local->font, "A", 0, 0, &dummy_w, &cursor_h);
        egui_dim_t work_h = self->region.size.height - (self->padding.top + self->padding.bottom);
        egui_dim_t cursor_y = self->padding.top + (work_h - cursor_h) / 2;

        EGUI_REGION_DEFINE(cursor_region,
            self->padding.left + cursor_x,
            cursor_y,
            EGUI_TEXTINPUT_CURSOR_WIDTH,
            cursor_h);
        egui_view_invalidate_region(self, &cursor_region);
    }
    else
    {
        egui_view_invalidate(self);
    }

    egui_timer_start_timer(&local->cursor_timer, EGUI_CONFIG_TEXTINPUT_CURSOR_BLINK_MS, 0);
}
```

#### 5b: 光标移动时 invalidate 新旧位置

修改 `egui_view_textinput_set_cursor_pos` (line 238-260):
- 在更新 `cursor_pos` 之前，先计算旧光标区域并 invalidate
- 更新 `cursor_pos` 后，计算新光标区域并 invalidate
- 抽取一个 helper 函数 `egui_view_textinput_invalidate_cursor_region()` 来避免重复代码

```c
static void egui_view_textinput_invalidate_cursor_region(egui_view_t *self, egui_view_textinput_t *local)
{
    if (local->font == NULL)
    {
        egui_view_invalidate(self);
        return;
    }
    egui_dim_t cursor_x = egui_view_textinput_get_text_width_to_pos(local, local->cursor_pos) - local->scroll_offset_x;
    egui_dim_t dummy_w = 0;
    egui_dim_t cursor_h = 0;
    local->font->api->get_str_size(local->font, "A", 0, 0, &dummy_w, &cursor_h);
    egui_dim_t work_h = self->region.size.height - (self->padding.top + self->padding.bottom);
    egui_dim_t cursor_y = self->padding.top + (work_h - cursor_h) / 2;

    EGUI_REGION_DEFINE(cursor_region,
        self->padding.left + cursor_x,
        cursor_y,
        EGUI_TEXTINPUT_CURSOR_WIDTH,
        cursor_h);
    egui_view_invalidate_region(self, &cursor_region);
}
```

修改 `set_cursor_pos`:
```c
void egui_view_textinput_set_cursor_pos(egui_view_t *self, uint8_t pos)
{
    EGUI_LOCAL_INIT(egui_view_textinput_t);
    if (pos > local->text_len) pos = local->text_len;
    if (pos == local->cursor_pos) return;

    // Invalidate old cursor position
    egui_view_textinput_invalidate_cursor_region(self, local);

    local->cursor_pos = pos;
    local->cursor_visible = 1;
    egui_timer_stop_timer(&local->cursor_timer);
    egui_timer_start_timer(&local->cursor_timer, EGUI_CONFIG_TEXTINPUT_CURSOR_BLINK_MS, 0);

    egui_view_textinput_update_scroll(self);

    // Invalidate new cursor position
    egui_view_textinput_invalidate_cursor_region(self, local);
}
```

注意：`insert_char`、`delete_char`、`delete_forward`、`set_text` 等改变文字内容的方法保持 `egui_view_invalidate(self)` 不变（全量更新）。

#### 5c: on_draw PFB 检查

修改 `egui_view_textinput_on_draw` (line 368-447):
- 在绘制文字区域和光标区域之前，分别用 `egui_canvas_is_region_active()` 检查
- 背景容器绘制不加检查（通常覆盖整个控件）
- 文字和光标区域检查：

在 "Draw text or placeholder" 块之前加：
```c
// Check if text area is active in current PFB tile
egui_region_t text_screen_region = {{work_region.location.x, work_region.location.y},
                                     {work_region.size.width, work_region.size.height}};
int text_active = egui_canvas_is_region_active(&text_screen_region);
```

然后用 `if (text_active)` 包裹文字绘制块。

光标绘制块类似：计算 cursor 屏幕区域后检查。

### 验证
- 编译通过
- 运行 HelloBasic textinput 测试，截图对比正确

---

## Step 6: number_picker 控件优化

### 修改文件

**`src/widget/egui_view_number_picker.c`**

#### 6a: 按压/释放子区域 invalidate

添加 helper 函数：
```c
static void egui_view_number_picker_invalidate_zone(egui_view_t *self, int8_t zone)
{
    egui_dim_t third_h = self->region.size.height / 3;
    egui_dim_t w = self->region.size.width;
    egui_dim_t y_offset;

    if (zone == 1)
        y_offset = 0;
    else if (zone == -1)
        y_offset = self->region.size.height - third_h;
    else
        y_offset = third_h;

    EGUI_REGION_DEFINE(zone_region, 0, y_offset, w, third_h);
    egui_view_invalidate_region(self, &zone_region);
}
```

修改 `on_touch_event` (line 176-266):
- `ACTION_DOWN`: 替换 `egui_view_invalidate(self)` 为 `egui_view_number_picker_invalidate_zone(self, hit_zone)`
- `ACTION_MOVE`: 替换 `egui_view_invalidate(self)` 为 invalidate 旧 zone 和新 zone
- `ACTION_UP`: 替换 `egui_view_invalidate(self)` 为 `egui_view_number_picker_invalidate_zone(self, pressed_zone)`
- `ACTION_CANCEL`: 同 UP

修改 `set_value` (line 32-52):
- 替换 `egui_view_invalidate(self)` 为 `egui_view_number_picker_invalidate_zone(self, 0)` （中间数字区域）

#### 6b: on_draw PFB 检查

修改 `egui_view_number_picker_on_draw` (line 110-173):
- 对 top、mid、bottom 三个区域分别做 `egui_canvas_is_region_active()` 检查
- 分隔线绘制保持不变（开销极小）

在每个区域块之前加检查：
```c
// Top 1/3
{
    EGUI_REGION_DEFINE(top_screen, region.location.x, region.location.y, w, third_h);
    if (egui_canvas_is_region_active(&top_screen))
    {
        // ... existing top drawing code ...
    }
}
```

同理 mid 和 bottom。

### 验证
- 编译通过
- 运行 HelloBasic number_picker 测试，截图对比正确

---

## Step 7: mini_calendar 控件优化

### 修改文件

**`src/widget/egui_view_mini_calendar.c`**

#### 7a: 日期选择子区域 invalidate

添加 helper 函数计算单元格的本地坐标区域：
```c
static void egui_view_mini_calendar_get_day_cell_region(egui_view_t *self,
    egui_view_mini_calendar_t *local, uint8_t day, egui_region_t *out_region)
{
    egui_dim_t w = self->region.size.width;
    egui_dim_t h = self->region.size.height;
    egui_dim_t cell_w = w / 7;
    egui_dim_t header_h = h / 8;
    egui_dim_t cell_h = (h - header_h * 2) / 6;

    uint8_t first_dow = day_of_week(local->year, local->month, 1);
    uint8_t start_col = (first_dow - local->first_day_of_week + 7) % 7;
    uint8_t pos = start_col + day - 1;
    uint8_t col = pos % 7;
    uint8_t row = pos / 7;

    out_region->location.x = col * cell_w;
    out_region->location.y = header_h * 2 + row * cell_h;
    out_region->size.width = cell_w;
    out_region->size.height = cell_h;
}
```

修改 `on_touch_event` ACTION_UP 分支 (line 267-283):
- 在 `local->day = hit_day` 之前，先 invalidate 旧日期单元格
- 设置新日期后，invalidate 新日期单元格
- 替换 `egui_view_invalidate(self)` 为两次 `egui_view_invalidate_region()`

```c
if (was_pressed && pressed_day != 0 && pressed_day == hit_day && hit_day != local->day)
{
    // Invalidate old selected day cell
    egui_region_t old_cell;
    egui_view_mini_calendar_get_day_cell_region(self, local, local->day, &old_cell);
    egui_view_invalidate_region(self, &old_cell);

    local->day = hit_day;

    // Invalidate new selected day cell
    egui_region_t new_cell;
    egui_view_mini_calendar_get_day_cell_region(self, local, hit_day, &new_cell);
    egui_view_invalidate_region(self, &new_cell);

    if (local->on_date_selected)
    {
        local->on_date_selected(self, hit_day);
    }
}
```

注意：`set_date`（月份/年份切换）保持 `egui_view_invalidate(self)` 全量更新。

#### 7b: on_draw PFB 检查

修改 `egui_view_mini_calendar_on_draw` (line 122-243):
- 在日期网格循环 (line 189-242) 中，对每个单元格做 PFB 检查：

```c
for (d = 1; d <= total_days; d++)
{
    uint8_t pos = start_col + d - 1;
    col = pos % 7;
    uint8_t row = pos / 7;

    egui_region_t day_rect = {{x + col * cell_w, y + header_h * 2 + row * cell_h}, {cell_w, cell_h}};

    // Skip cells not in current PFB tile
    if (!egui_canvas_is_region_active(&day_rect))
    {
        continue;
    }

    // ... rest of existing drawing code ...
}
```

- 标题行和星期行的 PFB 检查可选（开销较小），但如果要做也很简单

### 验证
- 编译通过
- 运行 HelloBasic mini_calendar 测试，截图对比正确

---

## Step 8: 单元测试 — 子区域 invalidate

### 新增文件

**`example/HelloUnitTest/test/test_invalidate_region.h`**
```c
#ifndef _TEST_INVALIDATE_REGION_H_
#define _TEST_INVALIDATE_REGION_H_

#ifdef __cplusplus
extern "C" {
#endif

void test_invalidate_region_run(void);

#ifdef __cplusplus
}
#endif

#endif
```

**`example/HelloUnitTest/test/test_invalidate_region.c`**

测试用例：
1. `test_invalidate_region_basic` — 创建一个 view，设置 region_screen = {10,10,100,50}，调用 invalidate_region({5,5,20,10})，验证脏区域 = {15,15,20,10}
2. `test_invalidate_region_clamp` — 子区域超出控件边界，验证被裁剪
3. `test_invalidate_region_invisible` — 不可见 view，验证脏区域数组不变
4. `test_invalidate_region_multiple` — 多次调用，验证合并行为
5. `test_invalidate_region_vs_full` — 对比子区域和全量 invalidate 的结果

测试模式：
- 使用 `egui_core_clear_region_dirty()` 清空
- 手动设置 view 的 `region_screen`、`is_visible` 等字段
- 调用 `egui_view_invalidate_region()`
- 检查 `egui_core_get_region_dirty_arr()` 的结果

### 修改文件

**`example/HelloUnitTest/uicode.c`**
- 添加 `#include "test/test_invalidate_region.h"`
- 在 `uicode_create_ui()` 中添加 `test_invalidate_region_run();`

### 验证
- 编译通过
- 所有测试通过

---

## Step 9: 单元测试 — PFB 相关性检查

### 新增文件

**`example/HelloUnitTest/test/test_canvas_active.h`**
```c
#ifndef _TEST_CANVAS_ACTIVE_H_
#define _TEST_CANVAS_ACTIVE_H_

#ifdef __cplusplus
extern "C" {
#endif

void test_canvas_active_run(void);

#ifdef __cplusplus
}
#endif

#endif
```

**`example/HelloUnitTest/test/test_canvas_active.c`**

测试用例：
1. `test_canvas_is_region_active_inside` — 区域完全在 PFB tile 内，返回 1
2. `test_canvas_is_region_active_outside` — 区域完全在 PFB tile 外，返回 0
3. `test_canvas_is_region_active_partial` — 部分重叠，返回 1
4. `test_canvas_is_region_active_edge` — 相邻但不重叠（边界），返回 0

测试模式：
- 需要设置 canvas 的 pfb_region（通过直接访问 canvas_data 或通过 helper）
- 调用 `egui_canvas_is_region_active()` 检查返回值

### 修改文件

**`example/HelloUnitTest/uicode.c`**
- 添加 `#include "test/test_canvas_active.h"`
- 在 `uicode_create_ui()` 中添加 `test_canvas_active_run();`

### 验证
- 编译通过
- 所有测试通过

---

## Step 10: 运行时测试验证

### 修改文件

在 HelloBasic 的 thermostat 或其他测试场景中验证：
- textinput 光标闪烁渲染正确
- number_picker 按压/数值变化渲染正确
- mini_calendar 日期选择渲染正确

通过现有截图对比机制验证。

---

## 实现顺序总结

| Step | 内容 | 依赖 | 预估复杂度 |
|------|------|------|-----------|
| 1 | `egui_view_invalidate_region()` | 无 | 低 |
| 2 | `egui_canvas_is_region_active()` | 无 | 低 |
| 3 | 编译开关 | 无 | 低 |
| 4 | 子区域注册表 API | Step 1, 3 | 低 |
| 5 | textinput 优化 | Step 1, 2 | 中 |
| 6 | number_picker 优化 | Step 1, 2 | 中 |
| 7 | mini_calendar 优化 | Step 1, 2 | 中 |
| 8 | 单元测试 - invalidate_region | Step 1 | 中 |
| 9 | 单元测试 - canvas_active | Step 2 | 低 |
| 10 | 运行时测试验证 | Step 5-7 | 低 |

Step 1-4 可以在一个 commit 中完成（核心 API）。
Step 5-7 每个控件一个 commit。
Step 8-9 测试一个 commit。
Step 10 验证。
