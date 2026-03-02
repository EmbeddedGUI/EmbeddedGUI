# View 体系与视图树

## 概述

EmbeddedGUI 的 UI 构建基于视图树（View Tree）模型。所有可见元素都是 `egui_view_t` 的派生类型，通过 `egui_view_group_t` 容器组织成树形结构。这一设计借鉴了 Android 的 View 体系，在 C 语言中通过结构体组合和函数指针实现继承与多态。

## egui_view_t 基类

`egui_view_t` 是所有视图的基类，定义在 `src/widget/egui_view.h`。每个视图实例包含以下核心属性：

```c
struct egui_view
{
    uint16_t id;                    // 视图唯一标识（调试用）
    uint8_t is_enable : 1;          // 是否启用
    uint8_t is_visible : 1;         // 是否可见
    uint8_t is_gone : 1;            // 是否隐藏（不占布局空间）
    uint8_t is_pressed : 1;         // 是否按下
    uint8_t is_clickable : 1;       // 是否可点击
    uint8_t is_request_layout : 1;  // 是否需要重新布局

    egui_alpha_t alpha;             // 透明度 (0-255)
    egui_view_padding_t padding;    // 内边距
    egui_view_margin_t margin;      // 外边距
    egui_region_t region;           // 相对父视图的位置和大小
    egui_region_t region_screen;    // 屏幕绝对坐标（自动计算）
    egui_background_t *background;  // 背景对象
    egui_view_group_t *parent;      // 父视图指针
    const egui_view_api_t *api;     // 虚函数表
};
```

### 可见性控制

视图有两种隐藏方式：

| 属性 | 效果 | 布局影响 |
|------|------|----------|
| `is_visible = 0` | 不绘制，但仍占据布局空间 | 无 |
| `is_gone = 1` | 不绘制，不占据布局空间 | 触发父容器重新布局 |

### 虚函数表 (API Table)

每个视图类型通过 `egui_view_api_t` 提供多态行为：

```c
struct egui_view_api
{
    int  (*dispatch_touch_event)(egui_view_t *self, egui_motion_event_t *event);
    int  (*on_touch_event)(egui_view_t *self, egui_motion_event_t *event);
    int  (*on_intercept_touch_event)(egui_view_t *self, egui_motion_event_t *event);
    void (*compute_scroll)(egui_view_t *self);
    void (*calculate_layout)(egui_view_t *self);
    void (*request_layout)(egui_view_t *self);
    void (*draw)(egui_view_t *self);
    void (*on_attach_to_window)(egui_view_t *self);
    void (*on_draw)(egui_view_t *self);
    void (*on_detach_from_window)(egui_view_t *self);
};
```

子类通过定义自己的 API 表来覆盖父类行为，实现多态。

## egui_view_group_t 容器

`egui_view_group_t` 继承自 `egui_view_t`，是所有容器类视图的基类。它通过双向链表管理子视图：

```c
struct egui_view_group
{
    egui_view_t base;                       // 基类
    uint8_t is_disallow_intercept;          // 禁止拦截触摸事件
    uint8_t is_disallow_process_touch_event;// 禁止处理触摸事件
    egui_view_t *first_touch_target;        // 触摸事件目标
    egui_dlist_t childs;                    // 子视图双向链表
};
```

### 子视图管理 API

| API | 说明 |
|-----|------|
| `egui_view_group_add_child(self, child)` | 添加子视图 |
| `egui_view_group_remove_child(self, child)` | 移除子视图 |
| `egui_view_group_clear_childs(self)` | 清空所有子视图 |
| `egui_view_group_get_child_count(self)` | 获取子视图数量 |
| `egui_view_group_get_first_child(self)` | 获取第一个子视图 |

启用 `EGUI_CONFIG_FUNCTION_SUPPORT_LAYER` 后，还支持 Z 轴排序：

| API | 说明 |
|-----|------|
| `egui_view_group_bring_child_to_front(self, child)` | 将子视图移到最前 |
| `egui_view_group_send_child_to_back(self, child)` | 将子视图移到最后 |
| `egui_view_set_layer(self, layer)` | 设置图层值 (0-255) |

## 视图树结构

系统启动后，`egui_core` 维护一棵全局视图树：

```
root_view_group (egui_core 内部)
  +-- user_root_view_group
        +-- Activity root_view (由 Activity 管理)
              +-- group_1 (egui_view_group_t)
              |     +-- label_1 (egui_view_label_t)
              |     +-- button_1 (egui_view_button_t)
              +-- group_2 (egui_view_linearlayout_t)
                    +-- image_1 (egui_view_image_t)
                    +-- switch_1 (egui_view_switch_t)
```

- `root_view_group`：框架内部根节点，包含系统级视图（如调试信息）
- `user_root_view_group`：用户视图根节点，Activity 的视图挂载于此
- 叶子节点：label、button、image 等不包含子视图的控件

## 视图生命周期

一个视图从创建到销毁经历以下阶段：

```
init --> add_child (attach) --> calculate_layout --> draw --> remove_child (detach)
  |                                  |              |
  |                           request_layout   on_draw
  |                           compute_scroll
  v
on_attach_to_window                              on_detach_from_window
```

1. `init`：初始化视图结构体，设置默认值，绑定 API 表
2. `on_attach_to_window`：视图被添加到视图树时调用
3. `calculate_layout`：计算屏幕绝对坐标 `region_screen`
4. `draw`：绘制流程 -- 检查可见性 -> 设置 alpha -> 绘制背景 -> 调用 `on_draw`
5. `on_detach_from_window`：视图从视图树移除时调用

### draw 流程详解

`egui_view_draw()` 的执行步骤：

```c
void egui_view_draw(egui_view_t *self)
{
    // 1. 保存当前 canvas alpha
    // 2. 检查 is_visible 和 is_gone
    // 3. 清除 canvas mask
    // 4. 混合视图自身 alpha 到 canvas
    // 5. 绘制阴影（如果有）
    // 6. 计算工作区域（与 PFB 的交集）
    // 7. 绘制背景
    // 8. 调用 on_draw（子类实现具体绘制）
    // 9. 恢复 canvas alpha
}
```

## 常用 API 速查表

### 位置与大小

| API | 说明 |
|-----|------|
| `egui_view_set_position(self, x, y)` | 设置相对父视图的位置 |
| `egui_view_set_size(self, w, h)` | 设置视图宽高 |
| `egui_view_scroll_to(self, x, y)` | 滚动到指定位置 |
| `egui_view_scroll_by(self, dx, dy)` | 相对滚动 |
| `egui_view_layout(self, region)` | 直接设置 region |

### 外观属性

| API | 说明 |
|-----|------|
| `egui_view_set_alpha(self, alpha)` | 设置透明度 (0-255) |
| `egui_view_set_background(self, bg)` | 设置背景 |
| `egui_view_set_visible(self, v)` | 设置可见性 |
| `egui_view_set_gone(self, g)` | 设置是否隐藏 |
| `egui_view_set_shadow(self, shadow)` | 设置阴影效果 |

### 边距

| API | 说明 |
|-----|------|
| `egui_view_set_padding(self, l, r, t, b)` | 设置内边距 |
| `egui_view_set_padding_all(self, p)` | 四边统一内边距 |
| `egui_view_set_margin(self, l, r, t, b)` | 设置外边距 |
| `egui_view_set_margin_all(self, m)` | 四边统一外边距 |

### 交互

| API | 说明 |
|-----|------|
| `egui_view_set_on_click_listener(self, cb)` | 设置点击回调 |
| `egui_view_set_on_touch_listener(self, cb)` | 设置触摸回调 |
| `egui_view_set_clickable(self, v)` | 设置是否可点击 |
| `egui_view_set_enable(self, v)` | 设置是否启用 |
| `egui_view_invalidate(self)` | 标记需要重绘 |

## 代码示例

创建一个包含 label 和 button 的 group：

```c
#include "egui.h"

// 声明视图
static egui_view_group_t my_group;
static egui_view_label_t my_label;
static egui_view_button_t my_button;

// 点击回调
static void on_button_click(egui_view_t *self)
{
    egui_view_label_set_text(EGUI_VIEW_OF(&my_label), "Clicked!");
}

void my_page_init(void)
{
    // 初始化 group
    egui_view_group_init(EGUI_VIEW_OF(&my_group));
    egui_view_set_position(EGUI_VIEW_OF(&my_group), 10, 10);
    egui_view_set_size(EGUI_VIEW_OF(&my_group), 200, 100);

    // 初始化 label
    egui_view_label_init(EGUI_VIEW_OF(&my_label));
    egui_view_set_position(EGUI_VIEW_OF(&my_label), 0, 0);
    egui_view_set_size(EGUI_VIEW_OF(&my_label), 200, 30);
    egui_view_label_set_text(EGUI_VIEW_OF(&my_label), "Hello EGUI");

    // 初始化 button
    egui_view_button_init(EGUI_VIEW_OF(&my_button));
    egui_view_set_position(EGUI_VIEW_OF(&my_button), 0, 40);
    egui_view_set_size(EGUI_VIEW_OF(&my_button), 100, 40);
    egui_view_set_on_click_listener(EGUI_VIEW_OF(&my_button), on_button_click);

    // 构建视图树
    egui_view_group_add_child(EGUI_VIEW_OF(&my_group), EGUI_VIEW_OF(&my_label));
    egui_view_group_add_child(EGUI_VIEW_OF(&my_group), EGUI_VIEW_OF(&my_button));

    // 添加到用户根视图
    egui_core_add_user_root_view(EGUI_VIEW_OF(&my_group));
}
```

## 继承关系图

```
egui_view_t (基类)
  +-- egui_view_label_t
  +-- egui_view_image_t
  +-- egui_view_group_t (容器基类)
        +-- egui_view_linearlayout_t
        +-- egui_view_gridlayout_t
        +-- egui_view_scroll_t
        +-- egui_view_viewpage_t
        +-- egui_view_button_t
        +-- egui_view_switch_t
        +-- ...
```

所有容器类控件（button、switch 等）都继承自 `egui_view_group_t`，因为它们内部可能包含子视图（如 button 内部的 label）。纯显示类控件（label、image）直接继承 `egui_view_t`。
