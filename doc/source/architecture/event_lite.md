# egui_event 轻量事件系统

## 概述

`egui_event` 是 EmbeddedGUI 的轻量 view 事件监听机制。它允许调用方在某个 `egui_view_t` 上注册回调，监听 pressed、released、clicked、value changed、focused、size changed、layout changed 等高层 UI 状态变化。

它的定位是“控件状态变化通知”，不是原始输入事件队列，也不是 key/touch 分发链路的替代品。

典型使用场景：

- 给一个 view 添加多个轻量监听器，而不是只依赖单个 `on_click_listener`。
- 用统一方式监听 pressed、clicked、focus、value changed 等状态变化。
- 对整棵 view tree 广播轻量通知，例如语言变化、主题相关刷新、自定义业务事件。
- 为自动化测试、调试工具或生成器提供统一事件入口。

## 启用方式

`egui_event` 受 `EGUI_CONFIG_FUNCTION_EVENT_LITE` 控制，默认关闭。

```c
// app_egui_config.h
#define EGUI_CONFIG_FUNCTION_EVENT_LITE 1
```

每个 view 可注册的 listener 数量由 `EGUI_CONFIG_EVENT_MAX_LISTENERS_PER_VIEW` 控制，默认值为 `4`。

```c
// app_egui_config.h
#define EGUI_CONFIG_EVENT_MAX_LISTENERS_PER_VIEW 4
```

启用后，每个 `egui_view_t` 会增加一个固定长度 listener 数组和一个计数值。因此它是按 view 实例增加 RAM 的功能，只有确实需要通用事件监听时才建议打开。

## 核心对象

### 事件码

当前支持的事件码定义在 `src/core/egui_event.h`：

| 事件码 | 说明 |
| --- | --- |
| `EGUI_EVENT_ALL` | 监听该 view 上的所有 lite event |
| `EGUI_EVENT_PRESSED` | view 进入 pressed 状态 |
| `EGUI_EVENT_RELEASED` | view 退出 pressed 状态 |
| `EGUI_EVENT_CLICKED` | view 执行 click |
| `EGUI_EVENT_VALUE_CHANGED` | 控件值发生变化 |
| `EGUI_EVENT_FOCUSED` | view 获得焦点 |
| `EGUI_EVENT_DEFOCUSED` | view 失去焦点 |
| `EGUI_EVENT_SIZE_CHANGED` | view 尺寸变化 |
| `EGUI_EVENT_LAYOUT_CHANGED` | view 布局区域变化 |
| `EGUI_EVENT_LANGUAGE_CHANGED` | 语言变化广播 |
| `EGUI_EVENT_CUSTOM_BASE` | 自定义事件起始值 |

应用或控件可以从 `EGUI_EVENT_CUSTOM_BASE` 开始定义自己的事件码，避免和框架内置事件冲突。

### 事件对象

回调收到的是 `egui_event_t`：

```c
struct egui_event
{
    egui_event_code_t code;
    egui_view_t *target;
    egui_view_t *current_target;
    void *param;
    void *user_data;
};
```

字段含义：

| 字段 | 说明 |
| --- | --- |
| `code` | 本次事件码 |
| `target` | 发送事件的 view |
| `current_target` | 当前回调所在 view；当前实现中和 `target` 相同 |
| `param` | 事件参数，由发送方约定类型 |
| `user_data` | 注册 listener 时保存的调用方上下文 |

## API 说明

### 添加监听器

```c
int egui_view_add_event_listener(egui_view_t *self,
                                 egui_event_code_t code,
                                 egui_event_cb_t cb,
                                 void *user_data);
```

返回值：

- `0`：添加成功，或同一个 `(code, cb, user_data)` 已存在。
- `-1`：参数为空，或该 view 的 listener 数组已满。

### 移除监听器

```c
int egui_view_remove_event_listener(egui_view_t *self,
                                    egui_event_code_t code,
                                    egui_event_cb_t cb,
                                    void *user_data);
```

移除成功返回 `0`。找不到匹配项或参数为空时返回 `-1`。移除后，后续 listener 会向前压缩。

### 清空监听器

```c
void egui_view_clear_event_listeners(egui_view_t *self);
```

清空该 view 上的全部 lite event listener，不触发回调。

### 发送到单个 view

```c
int egui_view_send_event(egui_view_t *self, egui_event_code_t code, void *param);
```

只发送给 `self` 自己，不会自动冒泡到父 view，也不会自动下发到子 view。

返回值表示是否有 listener 被调用：

- `1`：至少一个 listener 匹配并被调用。
- `0`：没有 listener 被调用，或 `self == NULL`。

匹配规则：

- listener 的 code 等于本次 code。
- 或 listener 注册的是 `EGUI_EVENT_ALL`。

### 广播到 view tree

```c
void egui_view_send_event_to_tree(egui_view_t *root, egui_event_code_t code, void *param);
void egui_core_send_event_to_tree(egui_core_t *core, egui_event_code_t code, void *param);
```

这两个 API 用于树广播。发送顺序是先 root，再递归发送给子 view。

它和输入分发不同：

- 不做命中测试。
- 不做 focus 路由。
- 不支持拦截或消费后停止传播。
- 每个节点是否有 listener，只影响该节点是否执行回调，不影响其它节点。

## 自动触发的内置事件

启用 `EGUI_CONFIG_FUNCTION_EVENT_LITE` 后，部分框架 API 会自动发送 lite event。

| 触发来源 | 事件 |
| --- | --- |
| `egui_view_set_pressed(view, 1)` | `EGUI_EVENT_PRESSED` |
| `egui_view_set_pressed(view, 0)` | `EGUI_EVENT_RELEASED` |
| `egui_view_perform_click()` | `EGUI_EVENT_CLICKED` |
| `egui_view_set_size()` | `EGUI_EVENT_SIZE_CHANGED`，`param = &view->region.size` |
| `egui_view_set_position()` | `EGUI_EVENT_LAYOUT_CHANGED`，`param = &view->region` |
| `egui_view_layout()` | `EGUI_EVENT_LAYOUT_CHANGED`，`param = &view->region` |
| focus manager 设置焦点 | `EGUI_EVENT_FOCUSED` / `EGUI_EVENT_DEFOCUSED` |
| checkbox、switch、slider、progress bar 等控件值变化 | `EGUI_EVENT_VALUE_CHANGED` |
| 调用树广播 API | 调用方指定的事件，例如 `EGUI_EVENT_LANGUAGE_CHANGED` |

并不是所有 setter 都会自动发送 lite event。新增控件或新增状态时，如果希望暴露统一监听能力，应在状态真正变化后显式调用 `egui_view_send_event()`。

## 和 key 事件分发的关系

`egui_event` 和 key 事件分发是两层机制。

key 事件分发处理的是输入：

```text
port / driver
    -> egui_input_add_key()
    -> key FIFO
    -> egui_input_key_dispatch_work()
    -> egui_core_process_input_key()
    -> focus manager 或 root view tree
    -> view->api->dispatch_key_event()
    -> view->api->on_key / on_key_event
```

`egui_event` 处理的是 view 状态变化通知：

```text
view 状态变化
    -> egui_view_send_event()
    -> 匹配该 view 上注册的 listener
```

两者的关联发生在 key 分发改变 view 状态的时候。

### ENTER / SPACE 触发点击

默认 `egui_view_on_key_event()` 中，clickable view 会处理 `ENTER` 和 `SPACE`：

```text
KEY DOWN
    -> egui_view_set_pressed(view, 1)
    -> EGUI_EVENT_PRESSED

KEY UP
    -> egui_view_set_pressed(view, 0)
    -> EGUI_EVENT_RELEASED
    -> egui_view_perform_click()
    -> EGUI_EVENT_CLICKED
```

因此，按键本身不会变成 `EGUI_EVENT_KEY_DOWN` 之类的 lite event；但按键导致的 pressed/clicked 状态变化会触发对应 lite event。

### TAB 和方向键触发焦点变化

启用 `EGUI_CONFIG_FUNCTION_SUPPORT_FOCUS` 后，`egui_core_process_input_key()` 会优先处理焦点导航：

- `TAB` 的 `ACTION_UP` 会移动到下一个焦点 view。
- `Shift + TAB` 会移动到上一个焦点 view。
- 方向键可以按空间位置移动焦点。
- `ESCAPE` 可以清除当前焦点。

当焦点变化时，focus manager 会调用 `egui_view_send_event()`：

```text
旧焦点 view -> EGUI_EVENT_DEFOCUSED
新焦点 view -> EGUI_EVENT_FOCUSED
```

因此 key 分发不会直接广播 lite event，但焦点变化会产生 `FOCUSED` / `DEFOCUSED`。

### 原始 key 事件仍然走 key API

如果控件需要处理原始按键，例如字符输入、方向键编辑、游戏控制，应继续使用 key 分发 API：

- 重写 view API 中的 `on_key`。
- 或实现控件自己的 `dispatch_key_event` / `on_key_event`。
- 或使用 `egui_view_override_api_on_key()` 安装自定义 key 回调。

不要用 `egui_event` 替代原始 key 事件处理，因为 `egui_event_t` 当前没有 `key_code`、`type`、`is_shift`、`is_ctrl` 等 key 字段。

## 和 touch/click 的关系

touch 输入分发处理的是命中测试、拦截、capture path 和默认点击行为：

```text
egui_input_add_motion()
    -> motion FIFO
    -> egui_input_polling_work()
    -> egui_core_process_input_motion()
    -> root view group dispatch touch
    -> child view dispatch touch
```

当 touch 分发最终让 view 进入 pressed 或执行 click 时，才会触发 lite event：

```text
TOUCH DOWN inside clickable view
    -> egui_view_set_pressed(view, 1)
    -> EGUI_EVENT_PRESSED

TOUCH UP inside clickable view
    -> egui_view_set_pressed(view, 0)
    -> EGUI_EVENT_RELEASED
    -> egui_view_perform_click()
    -> EGUI_EVENT_CLICKED
```

如果需要处理原始坐标、MOVE、CANCEL、滑动距离等细节，应继续使用 touch 分发和 `on_touch` 回调，而不是 lite event。

## 使用示例

监听按钮点击：

```c
static void on_button_event(egui_event_t *event)
{
    if (event->code == EGUI_EVENT_CLICKED)
    {
        egui_view_t *button = event->target;
        EGUI_UNUSED(button);
    }
}

egui_view_add_event_listener(EGUI_VIEW_OF(&button), EGUI_EVENT_CLICKED, on_button_event, NULL);
```

监听所有 lite event：

```c
static void on_any_event(egui_event_t *event)
{
    EGUI_LOG_INF("event code: %d\n", event->code);
}

egui_view_add_event_listener(EGUI_VIEW_OF(&view), EGUI_EVENT_ALL, on_any_event, NULL);
```

广播语言变化：

```c
egui_core_send_event_to_tree(core, EGUI_EVENT_LANGUAGE_CHANGED, NULL);
```

自定义事件：

```c
#define APP_EVENT_DATA_READY ((egui_event_code_t)(EGUI_EVENT_CUSTOM_BASE + 1))

egui_view_send_event(EGUI_VIEW_OF(&view), APP_EVENT_DATA_READY, data_ptr);
```

## 设计边界

`egui_event` 是轻量通知机制，使用时需要注意：

- listener 存在每个 view 的固定数组中，不使用 heap，也不会动态扩容。
- `param` 只透传指针，框架不复制、不释放、不做类型检查。
- 单 view 发送不会冒泡，树发送也不会因为某个 listener 处理了事件而停止。
- 原始 key/touch 输入仍由现有 input pipeline 和 view API 处理。
- callback 中应避免修改同一个 view 的 listener 数组；如需增删 listener，建议放到事件回调结束后的业务流程中处理。
- 对于数据模型变化通知，优先考虑 `egui_subject`；对于 view 状态变化监听，使用 `egui_event`。

相关单测位于 `example/HelloUnitTest/test/test_event_lite.c`，覆盖 pressed、clicked、size changed、树广播和 focus 事件。
