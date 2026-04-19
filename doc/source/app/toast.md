# Toast 提示

## 当前实现更新

以下说明以当前代码实现为准：

- Toast 必须在 `egui_toast_init(self, core)` / `egui_toast_std_init(self, core)` 时直接带上目标 `core`
- `egui_toast_set_as_default(self)`、`egui_toast_show(self, text)`、`egui_toast_show_info(self, text)` 都只使用对象自身在 init 时保存的 `core`
- `egui_view_show_toast_info(...)`、`egui_page_base_show_toast_info(...)`、`egui_activity_show_toast_info(...)`、`egui_dialog_show_toast_info(...)` 也只依赖对象自己已经绑定好的 `core`

`bind_core` 相关旧接口已经删除，多屏场景下应从初始化开始就把对象构造在正确屏幕上。

Toast 是 EmbeddedGUI 中的轻量级提示组件，用于在屏幕底部短暂显示一条文本信息，一段时间后自动消失。

## 基本用法

最简单的使用方式，一行代码即可显示提示：

```c
egui_toast_show_info((egui_toast_t *)&toast, "Hello World");
```

前提是已经初始化并注册了 Toast 实例（通常在 `uicode_disp0_init` 中完成）。

## 初始化

EmbeddedGUI 提供了标准 Toast 实现 `egui_toast_std_t`，初始化流程：

```c
static egui_toast_std_t toast;

void uicode_disp0_init(egui_core_t *core)
{
    // 初始化标准 Toast
    egui_toast_std_init((egui_toast_t *)&toast, core);

    // 基于 Toast 初始化时绑定的 core 设为默认 Toast
    egui_toast_set_as_default((egui_toast_t *)&toast);
}
```

## 核心 API

```c
// 注册/清理默认 Toast
void egui_toast_set_as_default(egui_toast_t *self);
void egui_toast_clear_as_default(egui_toast_t *self);

// 直接操作 Toast 实例
void egui_toast_show(egui_toast_t *self, const char *text);
void egui_toast_show_info(egui_toast_t *self, const char *text);
void egui_toast_show_info_with_duration(egui_toast_t *self, const char *text, uint16_t duration);

// 通过已绑定 core 的对象快捷显示 Toast
void egui_view_show_toast_info(egui_view_t *self, const char *text);
void egui_page_base_show_toast_info(egui_page_base_t *self, const char *text);
void egui_activity_show_toast_info(egui_activity_t *self, const char *text);
void egui_dialog_show_toast_info(egui_dialog_t *self, const char *text);

// 设置显示持续时间（毫秒）
void egui_toast_set_duration(egui_toast_t *self, uint16_t duration);
char *egui_toast_get_str_buf(egui_toast_t *self);
```

## 持续时间配置

默认显示时间为 1000ms（1 秒），可通过两种方式修改：

```c
// 方式 1：运行时修改
egui_toast_set_duration((egui_toast_t *)&toast, 2000);  // 2 秒

// 方式 2：编译时修改默认值（在 app_egui_config.h 中）
#define EGUI_CONFIG_PARAM_TOAST_DEFAULT_SHOW_TIME 2000
```

Toast 内部使用 `egui_timer_t` 实现自动隐藏，到时间后调用 `on_hide` 回调。

## 格式化文本

如果需要显示动态内容，可以借助 Toast 内置的字符串缓冲区：

```c
char *p_str = egui_toast_get_str_buf((egui_toast_t *)&toast);
egui_api_sprintf(p_str, "Page %d loaded", page_index);
egui_toast_show_info((egui_toast_t *)&toast, p_str);
```

标准 Toast (`egui_toast_std_t`) 提供了 100 字节的内置缓冲区。

## 标准 Toast 样式

`egui_toast_std_t` 的默认样式：

- 位置：屏幕底部居中，距底边 20 像素
- 背景：黑色半透明圆角矩形（alpha=50%，圆角半径 30）
- 文字：白色，使用默认字体，居中对齐
- 内边距：左右 10px，上下 5px
- 宽度：根据文本内容自适应

## 自定义 Toast

如需自定义样式，可以继承 `egui_toast_t` 并重写虚函数：

```c
struct egui_toast_api
{
    void (*on_show)(egui_toast_t *self, const char *text);  // 显示时回调
    void (*on_hide)(egui_toast_t *self);                    // 隐藏时回调
    char *(*get_str_buf)(egui_toast_t *self);               // 获取字符串缓冲区
};
```

参考 `src/app/egui_toast_std.c` 中的实现。

## 相关文件

- `src/app/egui_toast.h` - Toast 基类定义
- `src/app/egui_toast.c` - Toast 基类实现
- `src/app/egui_toast_std.h` - 标准 Toast 定义
- `src/app/egui_toast_std.c` - 标准 Toast 实现

## Core 解析说明

在常规单屏流程下，`egui_toast_set_as_default(self)`、
`egui_toast_show(self, text)` 和 `egui_toast_show_info(self, text)` 等对象式入口，
当前实现中要求 Toast 在初始化时直接带上目标 `core`，这里不再自动绑定。

对于 `egui_view_show_toast_info(...)`、`egui_page_base_show_toast_info(...)`、
`egui_activity_show_toast_info(...)`、`egui_dialog_show_toast_info(...)`
这类对象式 helper，则会优先使用对象自己已经绑定的 `core`：

1. `view` 直接使用 `view->core`
2. `page/activity/dialog` 优先使用各自 `root_view` 已绑定的 `core`

因此在单屏常规流程下，只要对象在初始化时已经带上正确的 `core`，
这些 helper 就不需要再额外传入 `core` 参数，也不依赖当前 active core。

只有在多屏场景下、需要操作不同屏幕对象时，
才需要从一开始就用目标屏幕的 `core` 去初始化对应对象。
