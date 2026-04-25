# 第一个应用

本文以 `HelloSimple` 为基础，演示如何用 EmbeddedGUI 构建一个最小可运行界面。这个示例只包含两个标签，底部标签点击后会切换成固定提示 `"CLICKED"`。它刻意不使用 `LinearLayout`、`egui_api_sprintf`、`egui_view_button_t` 和默认字体资源，并在 app 配置里启用 `EGUI_CONFIG_FUNCTION_VIEW_LABEL_COMPACT_ONLY`、关闭 `EGUI_CONFIG_FUNCTION_CANVAS_COMPACT_NUMBER`、关闭 `EGUI_CONFIG_FUNCTION_VIEW_GROUP_TOUCH_CAPTURE_PATH`、关闭 `EGUI_CONFIG_FUNCTION_CORE_PRE_COMPUTE_SCROLL`，适合作为入门和 code size 基线。

## 运行 HelloSimple

先确保 [环境搭建](environment_setup.md) 已完成，然后在项目根目录执行：

```bash
make all APP=HelloSimple
make run
```

运行后会看到一个居中的 `"HELLO EGUI"` 标签和一个 `"CLICK"` 标签。点击底部标签后，它的文本会变为 `"CLICKED"`。

## 完整源代码

以下是 `example/HelloSimple/uicode_disp0.c` 的完整代码：

```c
#include "egui.h"
#include "uicode_disp0.h"

static egui_view_label_t label_1;
static egui_view_label_t action_1;

#define ACTION_WIDTH  150
#define ACTION_HEIGHT 50

#define LABEL_WIDTH  150
#define LABEL_HEIGHT 50

#define VIEW_GAP  12
#define LABEL_X   ((EGUI_CONFIG_SCEEN_WIDTH - LABEL_WIDTH) / 2)
#define LABEL_Y   ((EGUI_CONFIG_SCEEN_HEIGHT - LABEL_HEIGHT - ACTION_HEIGHT - VIEW_GAP) / 2)
#define ACTION_X  ((EGUI_CONFIG_SCEEN_WIDTH - ACTION_WIDTH) / 2)
#define ACTION_Y  (LABEL_Y + LABEL_HEIGHT + VIEW_GAP)

EGUI_VIEW_LABEL_PARAMS_INIT(label_1_params, LABEL_X, LABEL_Y, LABEL_WIDTH, LABEL_HEIGHT, "HELLO EGUI", NULL, EGUI_COLOR_WHITE, EGUI_ALPHA_100);
EGUI_VIEW_LABEL_PARAMS_INIT(action_1_params, ACTION_X, ACTION_Y, ACTION_WIDTH, ACTION_HEIGHT, "CLICK", NULL, EGUI_COLOR_WHITE, EGUI_ALPHA_100);

static void action_click_cb(egui_view_t *self)
{
    egui_view_label_set_text(self, "CLICKED");
}

void uicode_disp0_init(egui_core_t *core)
{
    egui_view_label_init_with_params(EGUI_VIEW_OF(&label_1), core, &label_1_params);
    egui_view_label_init_with_params(EGUI_VIEW_OF(&action_1), core, &action_1_params);
    egui_view_set_on_click_listener(EGUI_VIEW_OF(&action_1), action_click_cb);

    egui_core_add_user_root_view(EGUI_VIEW_OF(&label_1));
    egui_core_add_user_root_view(EGUI_VIEW_OF(&action_1));
}

#if EGUI_CONFIG_FUNCTION_RECORDING_TEST
bool egui_port_get_recording_action(int action_index, egui_sim_action_t *p_action)
{
    if (action_index >= 3)
    {
        return false;
    }

    EGUI_SIM_SET_CLICK_VIEW(p_action, &action_1, 1000);
    return true;
}
#endif
```

## 逐段解析

### 1. 静态控件声明

```c
static egui_view_label_t label_1;
static egui_view_label_t action_1;
```

EGUI 使用面向对象风格的 C 结构体来表示控件。这里声明了两个静态对象：

- `egui_view_label_t`：显示 `"HELLO EGUI"`
- 第二个 `egui_view_label_t`：接收点击事件并更新自己的文本

把控件声明成 `static` 全局变量，意味着整个应用生命周期内都能稳定访问它们，不需要动态分配内存。

### 2. 编译期布局参数

```c
#define VIEW_GAP  12
#define LABEL_X   ((EGUI_CONFIG_SCEEN_WIDTH - LABEL_WIDTH) / 2)
#define LABEL_Y   ((EGUI_CONFIG_SCEEN_HEIGHT - LABEL_HEIGHT - BUTTON_HEIGHT - VIEW_GAP) / 2)
#define ACTION_X  ((EGUI_CONFIG_SCEEN_WIDTH - ACTION_WIDTH) / 2)
#define ACTION_Y  (LABEL_Y + LABEL_HEIGHT + VIEW_GAP)
```

这个示例直接使用固定坐标来排版，而不是额外创建一个 `LinearLayout`。这样做有两个好处：

- 代码更短，适合入门
- 不会把 `linearlayout` 相关实现一起链接进最终二进制

位置仍然通过屏幕宽高宏计算，所以在同一分辨率配置下依然保持居中显示。

### 3. 参数宏初始化

```c
EGUI_VIEW_LABEL_PARAMS_INIT(label_1_params, LABEL_X, LABEL_Y, LABEL_WIDTH, LABEL_HEIGHT, "Hello World!", EGUI_CONFIG_FONT_DEFAULT, EGUI_COLOR_WHITE, EGUI_ALPHA_100);
EGUI_VIEW_LABEL_PARAMS_INIT(action_1_params, ACTION_X, ACTION_Y, ACTION_WIDTH, ACTION_HEIGHT, "Click", EGUI_CONFIG_FONT_DEFAULT, EGUI_COLOR_WHITE, EGUI_ALPHA_100);
```

这些 `PARAMS_INIT` 宏会在编译期生成参数结构体，常见字段包括：

- 位置：`x`、`y`
- 尺寸：宽度、高度
- 文本：显示字符串
- 字体与颜色：这里把字体设为 `NULL`，让 `Label` 走内置 compact ASCII 位图文本；颜色为白色，100% 不透明

把参数放在编译期常量里，通常比运行时逐项赋值更省 RAM。

### 4. 点击回调

```c
static void action_click_cb(egui_view_t *self)
{
    egui_view_label_set_text(self, "CLICKED");
}
```

这里直接把第二个标签设为 clickable，因此可以复用 `egui_view_label_set_text()` 修改它自己的文字，而不需要额外引入 `Button` 控件实现。

这里没有使用：

- `egui_api_sprintf`
- 文本缓冲区
- 点击计数变量
- 默认字体资源

这样可以避免把不必要的格式化逻辑带进 `HelloSimple`。

### 5. 构建视图树

```c
void uicode_disp0_init(egui_core_t *core)
{
    egui_view_label_init_with_params(EGUI_VIEW_OF(&label_1), core, &label_1_params);
    egui_view_label_init_with_params(EGUI_VIEW_OF(&action_1), core, &action_1_params);
    egui_view_set_on_click_listener(EGUI_VIEW_OF(&action_1), action_click_cb);

    egui_core_add_user_root_view(EGUI_VIEW_OF(&label_1));
    egui_core_add_user_root_view(EGUI_VIEW_OF(&action_1));
}
```

初始化流程非常直接：

1. 调用 `*_init_with_params()` 初始化控件
2. 给底部标签注册点击回调
3. 把两个控件直接挂到 user root

`EGUI_VIEW_OF()` 用来把具体控件指针向上转换成基类 `egui_view_t *`，这是 EGUI 面向对象 C 风格 API 的基础用法。

### 6. 录制动作

```c
#if EGUI_CONFIG_FUNCTION_RECORDING_TEST
bool egui_port_get_recording_action(int action_index, egui_sim_action_t *p_action)
{
    if (action_index >= 3)
    {
        return false;
    }

    EGUI_SIM_SET_CLICK_VIEW(p_action, &action_1, 1000);
    return true;
}
#endif
```

这段代码只在录制/运行时验证模式下启用，用来自动点击底部标签三次，方便 CI 和截图验证。

## 关键概念总结

### 面向对象的 C 风格

EGUI 的点击回调定义在基类 `egui_view_t` 上，因此像 `Label` 这样的基础控件也能直接注册点击事件，不一定要额外引入 `Button`。

### 为什么这个示例不用 LinearLayout

`HelloSimple` 的目标是“最小可运行示例”，不是“布局能力演示”。当界面只有两个固定控件时，直接坐标布局更适合：

- 更容易读懂
- 更容易做 code size 基线
- 不会额外引入 `linearlayout` 依赖链

如果你想学习容器布局，建议看 `HelloBasic/linearlayout`。

### 为什么这里不用 sprintf

把按钮文案改为固定字符串，能避免：

- 格式化函数
- 中间缓冲区
- 点击计数状态

这对资源受限平台更友好，也更符合 `HelloSimple` 作为最小示例的定位。

### 为什么这里关闭完整 touch capture path

`HelloSimple` 只有一个可点击标签，不需要 scroll / nested group / intercept 这类多层触摸捕获能力，所以可以在 app 配置里关闭 `EGUI_CONFIG_FUNCTION_VIEW_GROUP_TOUCH_CAPTURE_PATH`，退化成更轻量的单目标捕获路径。

### 为什么这里关闭 scroll prepass

`HelloSimple` 的界面里没有 `scroll`、`viewpage`、`canvas viewport` 这类会在每帧预计算滚动状态的控件，所以可以在 app 配置里关闭 `EGUI_CONFIG_FUNCTION_CORE_PRE_COMPUTE_SCROLL`。这样会跳过每帧绘制前的 group scroll 预处理，也避免把更重的 `egui_view_group_compute_scroll()` 路径保留在最终镜像里。

### PFB 工作方式

EGUI 使用 Partial Frame Buffer，而不是整屏帧缓冲。渲染时会把屏幕拆成多个小块，逐块绘制、逐块刷新，因此只需要几 KB RAM 就能驱动一整屏 UI。

## 练习建议

1. 修改标签文本，把 `"HELLO EGUI"` 换成你自己的内容。
2. 修改 `LABEL_X / LABEL_Y / BUTTON_X / BUTTON_Y`，观察控件位置变化。
3. 再加一个按钮，并继续用 `egui_core_add_user_root_view()` 直接挂到 root。
4. 如果你想体验自动布局，再去看 `HelloBasic/linearlayout`，对比两种写法的差别。

## 下一步

- [项目目录结构](project_structure.md)：了解源码组织方式
- [构建系统详解](build_system.md)：掌握更多构建参数
