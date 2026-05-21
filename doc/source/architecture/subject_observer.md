# egui_subject 观察者系统

## 概述

`egui_subject` 是 EmbeddedGUI 提供的轻量 Subject-Observer 机制，用于在嵌入式 UI 中做简单的数据绑定和发布订阅。

它把“数据发生变化”和“UI 如何响应变化”解耦：业务代码更新模型数据后，只需要调用 `egui_subject_notify()`；所有订阅该数据源的观察者会按订阅顺序收到通知，并在回调中更新 label、按钮状态、进度条等 UI。

典型数据流如下：

```text
用户操作 / 业务逻辑
    -> 修改模型数据
    -> egui_subject_notify()
    -> observer callback
    -> 更新 UI 控件
```

这个机制主要用于以下场景：

- 一个数据源需要驱动多个 UI 控件，例如计数值同时更新文本和进度条。
- 希望业务逻辑只维护模型，不直接散落大量控件更新代码。
- 示例、测试、调试面板或小型动态 UI 需要一个低成本的数据变化通知机制。
- 资源受限平台上需要避免 heap、链表节点动态分配和大型事件总线。

## 启用方式

`egui_subject` 受配置宏 `EGUI_CONFIG_FUNCTION_SUBJECT_OBSERVER` 控制，默认关闭。

```c
// app_egui_config.h
#define EGUI_CONFIG_FUNCTION_SUBJECT_OBSERVER 1
```

每个 subject 可订阅的观察者数量由 `EGUI_CONFIG_SUBJECT_MAX_OBSERVERS` 控制，默认值为 `4`。

```c
// app_egui_config.h
#define EGUI_CONFIG_SUBJECT_MAX_OBSERVERS 4
```

启用后可以使用 `src/core/egui_subject.h` 中的 API。

## 核心对象

### egui_subject_t

`egui_subject_t` 表示一个可被观察的数据源。它内部保存一个固定长度的 `egui_observer_t *` 指针数组和当前观察者数量。

```c
struct egui_subject
{
    egui_observer_t *observers[EGUI_CONFIG_SUBJECT_MAX_OBSERVERS];
    uint8_t          count;
};
```

它不持有具体数据，只负责通知。通知时传入的 `data` 指针会原样转发给观察者回调。

### egui_observer_t

`egui_observer_t` 表示一个订阅者节点，由调用方分配，通常放在静态变量或业务对象结构体中。

```c
struct egui_observer
{
    egui_observer_callback_t callback;
    void                    *user_data;
};
```

一个订阅关系应对应一个 `egui_observer_t`。订阅后不要复制或移动该 observer；如果 UI 对象销毁或不再需要接收通知，应先调用 `egui_subject_unsubscribe()`。

## API 说明

### 初始化

```c
void egui_subject_init(egui_subject_t *subject);
```

初始化 subject，清空观察者列表并把计数置为 0。使用其它 subject API 前必须先初始化。

### 订阅

```c
int egui_subject_subscribe(egui_subject_t *subject,
                           egui_observer_t *observer,
                           egui_observer_callback_t callback,
                           void *user_data);
```

订阅成功返回 `0`。以下情况返回 `-1`：

- `subject`、`observer` 或 `callback` 为空。
- 同一个 observer 已经订阅到该 subject。
- subject 的观察者数组已满。

`user_data` 会保存到 observer 中，并在每次通知时传回 callback，适合传入控件指针或业务上下文。

### 取消订阅

```c
int egui_subject_unsubscribe(egui_subject_t *subject, egui_observer_t *observer);
```

取消订阅成功返回 `0`。如果参数为空或没有找到该 observer，返回 `-1`。

取消订阅后，后续观察者会向前压缩，剩余观察者的相对顺序保持不变。

### 通知

```c
void egui_subject_notify(egui_subject_t *subject, const void *data);
```

通知所有已订阅 observer。`data` 可以是新值指针，也可以为 `NULL`，由具体回调约定解释。

通知行为：

- observer 按订阅顺序调用。
- `data` 指针不被复制、不被释放，只原样传递。
- 通知开始时会记录当前 observer 数量，本轮通知中新追加的 observer 不会在本轮被调用。
- `subject == NULL` 时直接返回，不会崩溃。

建议不要在 callback 中修改同一个 subject 的订阅关系；如果确实需要订阅或取消订阅，放到本轮通知结束后的流程中处理。

### 清空

```c
void egui_subject_clear(egui_subject_t *subject);
```

移除全部 observer，不调用任何 observer callback。

### 查询数量

```c
uint8_t egui_subject_observer_count(const egui_subject_t *subject);
```

返回当前 observer 数量。`subject == NULL` 时返回 `0`。

## 使用示例

下面示例展示一个计数器模型如何通知 label 更新文本。

```c
#include "core/egui_subject.h"

static int32_t        s_counter;
static egui_subject_t s_counter_subject;
static egui_observer_t s_label_observer;
static char           s_count_buf[24];

static void on_counter_changed(egui_subject_t *subject, const void *data, void *user_data)
{
    egui_view_t *label = (egui_view_t *)user_data;
    const int32_t *value = (const int32_t *)data;

    EGUI_UNUSED(subject);
    snprintf(s_count_buf, sizeof(s_count_buf), "Count: %d", (int)*value);
    egui_view_label_set_text(label, s_count_buf);
}

void counter_page_init(egui_view_t *label)
{
    s_counter = 0;
    egui_subject_init(&s_counter_subject);
    egui_subject_subscribe(&s_counter_subject, &s_label_observer, on_counter_changed, label);
}

void counter_inc(void)
{
    s_counter++;
    egui_subject_notify(&s_counter_subject, &s_counter);
}
```

完整示例可参考 `example/HelloBasic/subject_observer/test.c`。

## 和事件系统的区别

`egui_subject` 和事件系统关注点不同：

| 模块 | 主要用途 |
| --- | --- |
| 事件系统 | 处理触摸、点击、按键、滚动等 UI 输入或控件事件 |
| `egui_subject` | 处理数据变化后的通知和 UI 同步 |

例如按钮点击仍然使用点击回调处理；点击回调中修改业务数据后，再通过 `egui_subject_notify()` 通知相关 UI 刷新。

## 内存和体积特点

`egui_subject` 的设计目标是小而确定：

- 默认关闭，关闭时不引入运行时代码，也不会给 view 结构体增加字段。
- 不使用 heap，observer 和 subject 都由调用方分配。
- 每个 subject 的 RAM 开销约为 `EGUI_CONFIG_SUBJECT_MAX_OBSERVERS * sizeof(pointer) + 1` 字节，再加上结构体对齐。
- 每个 observer 节点保存一个 callback 指针和一个 `user_data` 指针。
- 最大订阅数量固定，满了以后 `egui_subject_subscribe()` 返回 `-1`，不会动态扩容。

因此它适合资源受限场景下的小型数据绑定，不适合替代完整的全局消息总线。

## 设计边界

`egui_subject` 只负责通知，不负责数据生命周期。

需要注意：

- `data` 指针由调用方拥有，subject 不复制、不释放、不缓存。
- observer 由调用方拥有，必须保证订阅期间一直有效。
- 没有线程安全保护，应在 EmbeddedGUI 主循环线程中使用。
- 不做类型检查，callback 需要按约定把 `data` 和 `user_data` 转回正确类型。
- 订阅数量是编译期固定值，不适合观察者数量不确定的大型场景。

相关单测位于 `example/HelloUnitTest/test/test_subject.c`，覆盖初始化、订阅、通知、取消订阅、清空、重复订阅、容量上限和空参数行为。
