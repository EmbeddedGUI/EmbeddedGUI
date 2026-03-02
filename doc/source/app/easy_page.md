# Page 开发模式

Page 是 EmbeddedGUI 提供的轻量级多页面管理方案。相比 Activity 的完整生命周期和栈管理，Page 模式更简单直接，特别适合 RAM 资源紧张的嵌入式场景。

## Page 生命周期

Page 的生命周期只有两个状态：

```
Open  ->  Close
```

| 状态 | 说明 |
|------|------|
| `on_open` | 页面打开，初始化所有控件和资源，将视图添加到 page |
| `on_close` | 页面关闭，销毁所有控件和资源（如停止定时器） |

核心 API：

```c
// Page 基类 API 虚函数表
struct egui_page_base_api
{
    void (*on_open)(egui_page_base_t *self);
    void (*on_close)(egui_page_base_t *self);
    // ...
};

// 打开/关闭 Page
void egui_page_base_open(egui_page_base_t *self);
void egui_page_base_close(egui_page_base_t *self);
```

设计原则：同一时刻只有一个 Page 处于 Open 状态。切换页面时，先 close 当前 Page，再 open 新 Page。

## Page 结构体定义

每个 Page 是一个独立的结构体，包含 `egui_page_base_t` 基类和页面专属的控件与数据：

```c
struct egui_page_0
{
    egui_page_base_t base;      // 继承基类

    int index;
    char label_str[20];

    egui_timer_t timer;         // 页面专属定时器

    egui_view_linearlayout_t layout_1;
    egui_view_label_t label_1;
    egui_view_button_t button_1;
    egui_view_button_t button_2;
    egui_view_image_t image_1;
};
```

在 `on_open` 中初始化所有控件，在 `on_close` 中清理资源（如停止定时器）。

## Union 内存优化

这是 Page 模式最大的优势。由于同一时刻只有一个 Page 处于活跃状态，可以用 `union` 让所有 Page 共享同一块内存：

```c
// 定义 Page union，所有 Page 共享内存
union page_array
{
    egui_page_0_t page_0;
    egui_page_1_t page_1;
    egui_page_2_t page_2;
};

static union page_array g_page_array;
```

实际 RAM 占用 = `max(sizeof(page_0), sizeof(page_1), sizeof(page_2))`，而非三者之和。

## 页面切换

页面切换的核心逻辑：

```c
static egui_page_base_t *current_page = NULL;

void uicode_switch_page(int page_index)
{
    // 1. 关闭当前 Page
    if (current_page)
    {
        egui_page_base_close(current_page);
    }

    // 2. 初始化新 Page（在 union 内存上重新构造）
    switch (page_index)
    {
    case 0:
        egui_page_0_init((egui_page_base_t *)&g_page_array.page_0);
        current_page = (egui_page_base_t *)&g_page_array.page_0;
        break;
    case 1:
        egui_page_1_init((egui_page_base_t *)&g_page_array.page_1);
        current_page = (egui_page_base_t *)&g_page_array.page_1;
        break;
    // ...
    }

    // 3. 打开新 Page
    egui_page_base_open(current_page);
}
```

前进和后退封装：

```c
int uicode_start_next_page(void)
{
    int page_index = index + 1;
    if (page_index >= UICODE_MAX_PAGE_NUM)
    {
        egui_core_toast_show_info("No more next page");
        return -1;
    }
    uicode_switch_page(page_index);
    return 0;
}

int uicode_start_prev_page(void)
{
    int page_index = index - 1;
    if (page_index < 0)
    {
        egui_core_toast_show_info("No more previous page");
        return -1;
    }
    uicode_switch_page(page_index);
    return 0;
}
```

## 键盘事件处理

Page 模式支持按键控制，通过实现 `egui_port_hanlde_key_event` 接口将按键事件分发给当前 Page：

```c
void egui_port_hanlde_key_event(int key, int event)
{
    if (event == 0)
        return;

    if (current_page)
    {
        egui_page_base_key_pressed(current_page, key);
    }
}
```

`egui_page_base_t` 内置了 `key_pressed` 回调支持，各 Page 可自行处理按键逻辑。

## 与 Activity 模式对比

| 特性 | Activity 模式 | Page 模式 |
|------|--------------|-----------|
| 生命周期 | 6 个状态（完整） | 2 个状态（简化） |
| 页面栈 | 支持多层栈管理 | 无栈，手动管理 |
| 切换动画 | 内置滑动动画支持 | 无内置动画 |
| 内存占用 | 每个 Activity 独立内存 | union 共享内存 |
| RAM 开销 | 所有 Activity 之和 | 最大 Page 的大小 |
| Dialog 支持 | 内置 | 无 |
| 适用场景 | 复杂交互、多层导航 | 简单切换、资源受限 |

选型建议：

- RAM 充裕（>4KB 可用于 UI）且需要页面栈、切换动画 -> Activity
- RAM 紧张（<2KB 可用于 UI）或只需简单的页面切换 -> Page

## HelloEasyPage 完整流程

`example/HelloEasyPage/uicode.c` 展示了 Page 模式的完整用法：

```c
void uicode_init_ui(void)
{
    // 启动第一个 Page
    index = 0;
    uicode_switch_page(0);

    // 初始化 Toast
    egui_toast_std_init((egui_toast_t *)&toast);
    egui_core_toast_set((egui_toast_t *)&toast);
}

void uicode_create_ui(void)
{
    uicode_init_ui();
}
```

整个应用只需要一个 `union page_array` 的内存，加上一个 `current_page` 指针和一个 `index` 变量，即可管理任意数量的页面。

## 相关文件

- `src/app/egui_page_base.h` - Page 基类定义
- `example/HelloEasyPage/` - 完整示例
- `example/HelloEasyPage/egui_page_0.h/c` - Page 0 实现
- `example/HelloEasyPage/egui_page_1.h/c` - Page 1 实现
- `example/HelloEasyPage/egui_page_2.h/c` - Page 2 实现
