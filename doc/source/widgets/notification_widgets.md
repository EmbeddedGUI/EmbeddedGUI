# 通知控件

## 概述

通知控件用于向用户传达提示信息和弹出内容，适用于消息提醒、弹窗对话等场景。包括通知徽章和窗口两个控件。

## NotificationBadge

通知徽章控件，显示圆形红点和未读计数，常叠加在图标右上角使用。

### 效果展示

[在线演示](https://embeddedgui.github.io/EmbeddedGUI/basic.html?app=notification_badge)

### API

| 函数 | 说明 |
|------|------|
| `egui_view_notification_badge_init(self)` | 初始化徽章 |
| `egui_view_notification_badge_init_with_params(self, params)` | 使用参数初始化 |
| `egui_view_notification_badge_set_count(self, count)` | 设置计数 |
| `egui_view_notification_badge_get_count(self)` | 获取计数 |
| `egui_view_notification_badge_set_max_display(self, max)` | 设置最大显示数(超出显示 "N+") |
| `egui_view_notification_badge_set_badge_color(self, color)` | 设置徽章背景色 |
| `egui_view_notification_badge_set_text_color(self, color)` | 设置文字颜色 |
| `egui_view_notification_badge_set_font(self, font)` | 设置字体 |

### 参数宏

```c
EGUI_VIEW_NOTIFICATION_BADGE_PARAMS_INIT(name, x, y, w, h, count);
```

### 结构体字段

| 字段 | 类型 | 说明 |
|------|------|------|
| `count` | `uint16_t` | 通知计数 |
| `max_display` | `uint8_t` | 最大显示数 |
| `badge_color` | `egui_color_t` | 徽章背景色 |
| `text_color` | `egui_color_t` | 文字颜色 |

### 代码示例

```c
static egui_view_notification_badge_t badge;

EGUI_VIEW_NOTIFICATION_BADGE_PARAMS_INIT(badge_params, 50, 5, 20, 20, 3);

void init_ui(void)
{
    egui_view_notification_badge_init_with_params(
        EGUI_VIEW_OF(&badge), &badge_params);
    egui_view_notification_badge_set_max_display(
        EGUI_VIEW_OF(&badge), 99);
    egui_core_add_user_root_view(EGUI_VIEW_OF(&badge));
}

void add_notification(void)
{
    uint16_t cnt = egui_view_notification_badge_get_count(
        EGUI_VIEW_OF(&badge));
    egui_view_notification_badge_set_count(
        EGUI_VIEW_OF(&badge), cnt + 1);
}

void clear_notifications(void)
{
    egui_view_notification_badge_set_count(EGUI_VIEW_OF(&badge), 0);
}
```

### 使用说明

- 当 `count` 为 0 时，徽章自动隐藏
- 当 `count` 超过 `max_display` 时，显示为 "N+"（如 "99+"）
- 徽章通常叠加在其他控件上方，通过设置合适的坐标实现定位

---

## Window

窗口控件，继承自 Group，提供带标题栏和内容区域的弹窗容器，支持关闭回调。

### 效果展示

[在线演示](https://embeddedgui.github.io/EmbeddedGUI/basic.html?app=window)

### API

| 函数 | 说明 |
|------|------|
| `egui_view_window_init(self)` | 初始化窗口 |
| `egui_view_window_init_with_params(self, params)` | 使用参数初始化 |
| `egui_view_window_set_title(self, title)` | 设置标题文字 |
| `egui_view_window_set_header_height(self, height)` | 设置标题栏高度 |
| `egui_view_window_add_content(self, child)` | 向内容区域添加子控件 |
| `egui_view_window_set_on_close(self, callback)` | 设置关闭回调 |

### 参数宏

```c
EGUI_VIEW_WINDOW_PARAMS_INIT(name, x, y, w, h, header_height, title);
```

### 结构体字段

| 字段 | 类型 | 说明 |
|------|------|------|
| `title_label` | `egui_view_label_t` | 标题标签(内部) |
| `content` | `egui_view_group_t` | 内容容器(内部) |
| `header_height` | `egui_dim_t` | 标题栏高度 |
| `header_color` | `egui_color_t` | 标题栏颜色 |
| `content_bg_color` | `egui_color_t` | 内容区背景色 |

### 代码示例

```c
static egui_view_window_t win;
static egui_view_label_t content_label;

EGUI_VIEW_WINDOW_PARAMS_INIT(win_params, 20, 20, 200, 150, 30, "Settings");

static void on_close(egui_view_t *self)
{
    egui_view_set_visible(self, 0);
}

void init_ui(void)
{
    egui_view_window_init_with_params(EGUI_VIEW_OF(&win), &win_params);
    egui_view_window_set_on_close(EGUI_VIEW_OF(&win), on_close);

    egui_view_label_init(EGUI_VIEW_OF(&content_label));
    egui_view_set_position(EGUI_VIEW_OF(&content_label), 10, 10);
    egui_view_set_size(EGUI_VIEW_OF(&content_label), 180, 20);
    egui_view_label_set_text(EGUI_VIEW_OF(&content_label), "Window Content");
    egui_view_window_add_content(EGUI_VIEW_OF(&win),
        EGUI_VIEW_OF(&content_label));

    egui_core_add_user_root_view(EGUI_VIEW_OF(&win));
}
```

### 使用说明

- Window 内部自动创建标题栏和内容区域，无需手动管理布局
- 通过 `egui_view_window_add_content()` 添加的子控件会放入内容区域，坐标相对于内容区域左上角
- 关闭回调中通常调用 `egui_view_set_visible(self, 0)` 隐藏窗口
- 标题栏高度通过 `header_height` 参数控制，内容区域自动占据剩余空间
