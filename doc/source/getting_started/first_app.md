# 第一个应用

本文以 HelloSimple 示例为基础，逐步讲解如何使用 EmbeddedGUI 构建一个包含标签和按钮的简单界面。通过这个例子，你将理解 EGUI 的核心编程模式。

## 运行 HelloSimple

首先确保 [环境搭建](environment_setup.md) 已完成，然后在项目根目录执行:

```bash
make all APP=HelloSimple
make run
```

运行后将弹出一个窗口，显示 "Hello World!" 文本和一个 "Click me!" 按钮。点击按钮后，按钮文本会变为 "Clicked 1s"、"Clicked 2s"...依次递增。

## 完整源代码

以下是 `example/HelloSimple/uicode.c` 的完整代码:

```c
#include "egui.h"
#include <stdlib.h>
#include <math.h>
#include "uicode.h"

// views in root
static egui_view_label_t label_1;
static egui_view_button_t button_1;
static egui_view_linearlayout_t layout_1;

#define BUTTON_WIDTH  150
#define BUTTON_HEIGHT 50

#define LABEL_WIDTH  150
#define LABEL_HEIGHT 50

// View params
EGUI_VIEW_LINEARLAYOUT_PARAMS_INIT(layout_1_params,
    0, 0, EGUI_CONFIG_SCEEN_WIDTH, EGUI_CONFIG_SCEEN_HEIGHT,
    EGUI_ALIGN_CENTER);
EGUI_VIEW_LABEL_PARAMS_INIT(label_1_params,
    0, 0, LABEL_WIDTH, LABEL_HEIGHT,
    "Hello World!", EGUI_CONFIG_FONT_DEFAULT, EGUI_COLOR_WHITE, EGUI_ALPHA_100);
EGUI_VIEW_LABEL_PARAMS_INIT(button_1_params,
    0, 0, BUTTON_WIDTH, BUTTON_HEIGHT,
    NULL, EGUI_CONFIG_FONT_DEFAULT, EGUI_COLOR_WHITE, EGUI_ALPHA_100);

static char button_str[20] = "Click me!";
static void button_click_cb(egui_view_t *self)
{
    EGUI_LOG_INF("Clicked\n");

    static uint32_t cnt = 1;
    egui_api_sprintf(button_str, "Clicked %ds", cnt);
    EGUI_LOG_INF("button_str: %s\n", button_str);

    egui_view_label_set_text((egui_view_t *)self, button_str);
    cnt++;
}

void uicode_init_ui(void)
{
    // Init all views
    egui_view_linearlayout_init_with_params(EGUI_VIEW_OF(&layout_1), &layout_1_params);
    egui_view_label_init_with_params(EGUI_VIEW_OF(&label_1), &label_1_params);
    egui_view_button_init_with_params(EGUI_VIEW_OF(&button_1), &button_1_params);

    egui_view_label_set_text(EGUI_VIEW_OF(&button_1), button_str);
    egui_view_set_on_click_listener(EGUI_VIEW_OF(&button_1), button_click_cb);

    // Add childs to layout_1
    egui_view_group_add_child(EGUI_VIEW_OF(&layout_1), EGUI_VIEW_OF(&label_1));
    egui_view_group_add_child(EGUI_VIEW_OF(&layout_1), EGUI_VIEW_OF(&button_1));

    // Re-layout childs
    egui_view_linearlayout_layout_childs(EGUI_VIEW_OF(&layout_1));

    // Add To Root
    egui_core_add_user_root_view(EGUI_VIEW_OF(&layout_1));
}

void uicode_create_ui(void)
{
    uicode_init_ui();
}
```

## 逐段解析

### 1. 头文件与静态控件声明

```c
#include "egui.h"
#include "uicode.h"

static egui_view_label_t label_1;
static egui_view_button_t button_1;
static egui_view_linearlayout_t layout_1;
```

EGUI 采用面向对象的 C 语言模式。每个 UI 控件都是一个 C 结构体实例。这里声明了三个控件:

- `egui_view_label_t`: 文本标签，用于显示 "Hello World!"
- `egui_view_button_t`: 按钮控件，可响应点击事件
- `egui_view_linearlayout_t`: 线性布局容器，将子控件按垂直或水平方向排列

这些控件声明为 `static` 全局变量，因为它们的生命周期贯穿整个应用运行期间。在嵌入式环境中，避免动态内存分配是一个重要的实践。

### 2. 参数宏初始化

```c
EGUI_VIEW_LINEARLAYOUT_PARAMS_INIT(layout_1_params,
    0, 0, EGUI_CONFIG_SCEEN_WIDTH, EGUI_CONFIG_SCEEN_HEIGHT,
    EGUI_ALIGN_CENTER);

EGUI_VIEW_LABEL_PARAMS_INIT(label_1_params,
    0, 0, LABEL_WIDTH, LABEL_HEIGHT,
    "Hello World!", EGUI_CONFIG_FONT_DEFAULT, EGUI_COLOR_WHITE, EGUI_ALPHA_100);

EGUI_VIEW_LABEL_PARAMS_INIT(button_1_params,
    0, 0, BUTTON_WIDTH, BUTTON_HEIGHT,
    NULL, EGUI_CONFIG_FONT_DEFAULT, EGUI_COLOR_WHITE, EGUI_ALPHA_100);
```

每种控件都有对应的 `PARAMS_INIT` 宏，用于在编译期初始化参数结构体。这些参数包括:

- **位置**: x, y 坐标 (相对于父容器)
- **尺寸**: 宽度和高度 (像素)
- **对齐方式**: `EGUI_ALIGN_CENTER` 表示居中对齐
- **文本属性** (Label/Button): 显示文本、字体、颜色、透明度

使用参数宏而非运行时赋值，可以将参数存储在 ROM 中，节省宝贵的 RAM 空间。

### 3. 事件处理: 按钮点击回调

```c
static char button_str[20] = "Click me!";

static void button_click_cb(egui_view_t *self)
{
    static uint32_t cnt = 1;
    egui_api_sprintf(button_str, "Clicked %ds", cnt);
    egui_view_label_set_text((egui_view_t *)self, button_str);
    cnt++;
}
```

回调函数接收一个 `egui_view_t *` 指针，这是所有控件的基类指针。在回调中:

- `egui_api_sprintf` 是 EGUI 提供的格式化字符串函数
- `egui_view_label_set_text` 更新控件显示的文本 (Button 继承自 Label，因此也可以使用此函数)
- 文本缓冲区 `button_str` 必须在回调返回后仍然有效 (这里使用了 static 全局数组)

### 4. 视图树构建

```c
void uicode_init_ui(void)
{
    // 初始化控件
    egui_view_linearlayout_init_with_params(EGUI_VIEW_OF(&layout_1), &layout_1_params);
    egui_view_label_init_with_params(EGUI_VIEW_OF(&label_1), &label_1_params);
    egui_view_button_init_with_params(EGUI_VIEW_OF(&button_1), &button_1_params);

    // 设置按钮文本和点击回调
    egui_view_label_set_text(EGUI_VIEW_OF(&button_1), button_str);
    egui_view_set_on_click_listener(EGUI_VIEW_OF(&button_1), button_click_cb);

    // 将子控件添加到布局容器
    egui_view_group_add_child(EGUI_VIEW_OF(&layout_1), EGUI_VIEW_OF(&label_1));
    egui_view_group_add_child(EGUI_VIEW_OF(&layout_1), EGUI_VIEW_OF(&button_1));

    // 触发布局计算
    egui_view_linearlayout_layout_childs(EGUI_VIEW_OF(&layout_1));

    // 将根布局添加到系统
    egui_core_add_user_root_view(EGUI_VIEW_OF(&layout_1));
}
```

视图树的构建遵循以下流程:

1. **初始化**: 调用 `xxx_init_with_params()` 初始化每个控件
2. **配置**: 设置文本、回调等运行时属性
3. **组装**: 用 `egui_view_group_add_child()` 将子控件添加到容器
4. **布局**: 调用 `egui_view_linearlayout_layout_childs()` 让容器自动计算子控件的排列位置
5. **注册**: 用 `egui_core_add_user_root_view()` 将根视图注册到 EGUI 核心

`EGUI_VIEW_OF()` 宏将具体控件指针向上转换为基类 `egui_view_t *` 指针，类似于面向对象语言中的向上转型。

### 5. 入口函数

```c
void uicode_create_ui(void)
{
    uicode_init_ui();
}
```

`uicode_create_ui()` 是 EGUI 框架约定的 UI 入口函数，由框架在初始化完成后自动调用。所有的 UI 构建工作都在这个函数 (或其调用的子函数) 中完成。

## 关键概念总结

### 面向对象的 C 语言模式

EGUI 使用 C 结构体模拟类继承关系:

```
egui_view_t (基类)
  +-- egui_view_group_t (容器基类)
  |     +-- egui_view_linearlayout_t
  |     +-- egui_view_gridlayout_t
  +-- egui_view_label_t
  |     +-- egui_view_button_t
  +-- egui_view_image_t
  ...
```

通过 `EGUI_VIEW_OF()` 宏实现向上转换，所有控件 API 接受基类指针 `egui_view_t *` 作为第一个参数。

### PFB 工作原理

EGUI 不会一次性将整个屏幕绘制到帧缓冲区。相反，它将屏幕划分为多个小块 (Partial Frame Buffer)，逐块渲染并刷新到显示屏。每个控件在渲染时只需要处理与当前 PFB 块相交的区域。这使得 EGUI 仅需数 KB 的 RAM 即可驱动整个屏幕。

## 练习建议

1. **修改文本**: 将 "Hello World!" 改为你自己的文字，重新编译运行查看效果。

2. **添加第二个按钮**: 声明一个新的 `egui_view_button_t`，初始化并添加到 `layout_1` 中。记得在添加所有子控件后再调用 `egui_view_linearlayout_layout_childs()`。

3. **修改颜色**: 在 `EGUI_VIEW_LABEL_PARAMS_INIT` 中将 `EGUI_COLOR_WHITE` 替换为其他颜色常量，如 `EGUI_COLOR_RED`、`EGUI_COLOR_GREEN`。

4. **探索其他示例**: 尝试编译运行 HelloBasic 的各种子应用:

   ```bash
   make all APP=HelloBasic APP_SUB=slider
   make run
   ```

## 下一步

- [项目目录结构](project_structure.md): 了解源代码的组织方式
- [构建系统详解](build_system.md): 掌握更多构建选项
