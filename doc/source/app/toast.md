# Toast 提示

Toast 是 EmbeddedGUI 中的轻量级提示组件，用于在屏幕底部短暂显示一条文本信息，一段时间后自动消失。

## 基本用法

最简单的使用方式，一行代码即可显示提示：

```c
egui_core_toast_show_info("Hello World");
```

前提是已经初始化并注册了 Toast 实例（通常在 `uicode_init_ui` 中完成）。

## 初始化

EmbeddedGUI 提供了标准 Toast 实现 `egui_toast_std_t`，初始化流程：

```c
static egui_toast_std_t toast;

void uicode_init_ui(void)
{
    // 初始化标准 Toast
    egui_toast_std_init((egui_toast_t *)&toast);

    // 注册为全局 Toast
    egui_core_toast_set((egui_toast_t *)&toast);
}
```

## 核心 API

```c
// 注册全局 Toast 实例
void egui_core_toast_set(egui_toast_t *toast);

// 获取全局 Toast 实例
egui_toast_t *egui_core_toast_get(void);

// 显示提示信息（使用全局 Toast）
void egui_core_toast_show_info(const char *text);

// 直接操作 Toast 实例
void egui_toast_show(egui_toast_t *self, const char *text);

// 设置显示持续时间（毫秒）
void egui_toast_set_duration(egui_toast_t *self, uint16_t duration);
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
egui_toast_t *p_toast = egui_core_toast_get();
if (p_toast != NULL)
{
    char *p_str = p_toast->api->get_str_buf(p_toast);
    egui_api_sprintf(p_str, "Page %d loaded", page_index);
    egui_core_toast_show_info(p_str);
}
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
