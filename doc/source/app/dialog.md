# Dialog 对话框

## 当前实现更新

以下说明以当前代码实现为准：

- Dialog 必须在 `egui_dialog_init(self, core)` 时直接绑定目标 `core`
- `dialog.root_view` 与 `dialog.user_root_view` 会在 init 阶段一起带上同一个 `core`
- `egui_dialog_start(self, activity)` 要求 `dialog`、`dialog.root_view`、`dialog.user_root_view` 与目标 `activity` 已经属于同一个 `core`
- `egui_dialog_start_with_current(self)` 也只是使用 Dialog 自身的 `core` 去查找当前 Activity，不再自动补绑

`bind_core` 相关旧接口已经删除，多屏场景下应在构造 Dialog 时就传入目标屏幕的 `core`。

Dialog 是 EmbeddedGUI 中的模态对话框组件，依附于 Activity 存在。Dialog 拥有与 Activity 类似的完整生命周期，支持自定义内容和过渡动画。

## Dialog 与 Activity 的关系

- Dialog 必须绑定到一个 Activity（通过 `bind_activity` 字段）
- Dialog 显示时，宿主 Activity 仍然可见，但 Dialog 覆盖在其上方
- Dialog 拥有独立的 `root_view`（半透明遮罩层）和 `user_root_view`（内容区域）
- 关闭 Dialog 后，焦点回到宿主 Activity

## Dialog 生命周期

Dialog 的生命周期与 Activity 完全一致：

```
onCreate -> onStart -> onResume -> onPause -> onStop -> onDestroy
```

```c
struct egui_dialog_api
{
    void (*on_create)(egui_dialog_t *self);
    void (*on_start)(egui_dialog_t *self);
    void (*on_resume)(egui_dialog_t *self);
    void (*on_pause)(egui_dialog_t *self);
    void (*on_stop)(egui_dialog_t *self);
    void (*on_destroy)(egui_dialog_t *self);
};
```

## Dialog 结构体

```c
struct egui_dialog
{
    uint8_t state;                      // 当前状态
    uint8_t is_need_finish;             // 是否需要关闭
    uint8_t is_cancel_on_touch_outside; // 点击外部是否关闭

    egui_view_group_t root_view;        // 遮罩层（全屏）
    egui_view_group_t user_root_view;   // 内容区域

    egui_activity_t *bind_activity;     // 绑定的 Activity

    const egui_dialog_api_t *api;       // 虚函数表
};
```

## 核心 API

```c
// 启动 Dialog（绑定到指定 Activity）
void egui_dialog_start(egui_dialog_t *self, egui_activity_t *activity);

// 启动 Dialog（绑定到当前栈顶 Activity）
void egui_dialog_start_with_current(egui_dialog_t *self);

// 关闭 Dialog
void egui_dialog_finish(egui_dialog_t *self);

// 检查 Dialog 是否正处于切换流程中
int egui_dialog_check_in_process(egui_dialog_t *self);

// 通过视图反查当前 Dialog / Activity
egui_dialog_t *egui_view_get_dialog(egui_view_t *self);
egui_activity_t *egui_view_get_activity(egui_view_t *self);

// 设置 Dialog 的打开/关闭动画
void egui_dialog_set_anim(
    egui_dialog_t *self,
    egui_animation_t *open_anim,
    egui_animation_t *close_anim
);

// 设置 Dialog 内容区域的布局
void egui_dialog_set_layout(egui_dialog_t *self, egui_region_t *layout);

// 向 Dialog 内容区域添加子视图
void egui_dialog_add_view(egui_dialog_t *self, egui_view_t *view);
```

## Dialog 动画配置

典型的底部上滑动画：

```c
// 打开动画：从屏幕底部滑入
EGUI_ANIMATION_TRANSLATE_PARAMS_INIT(anim_dialog_start_param,
    0, 0, EGUI_CONFIG_SCEEN_HEIGHT, 0);
egui_animation_translate_t anim_dialog_start;

// 关闭动画：向屏幕底部滑出
EGUI_ANIMATION_TRANSLATE_PARAMS_INIT(anim_dialog_finish_param,
    0, 0, 0, -EGUI_CONFIG_SCEEN_HEIGHT);
egui_animation_translate_t anim_dialog_finish;

// 初始化并注册
egui_animation_translate_init(EGUI_ANIM_OF(&anim_dialog_start));
egui_animation_translate_params_set(&anim_dialog_start, &anim_dialog_start_param);
egui_animation_duration_set(EGUI_ANIM_OF(&anim_dialog_start), 500);

egui_animation_translate_init(EGUI_ANIM_OF(&anim_dialog_finish));
egui_animation_translate_params_set(&anim_dialog_finish, &anim_dialog_finish_param);
egui_animation_duration_set(EGUI_ANIM_OF(&anim_dialog_finish), 500);
egui_animation_is_fill_before_set(EGUI_ANIM_OF(&anim_dialog_finish), true);

egui_dialog_test_init((egui_dialog_t *)&dialog, core);
egui_dialog_set_anim(
    (egui_dialog_t *)&dialog,
    EGUI_ANIM_OF(&anim_dialog_start),
    EGUI_ANIM_OF(&anim_dialog_finish)
);
```

## 自定义 Dialog

以 HelloActivity 中的 `egui_dialog_test_t` 为例：

```c
// 1. 定义结构体
struct egui_dialog_test
{
    egui_dialog_t base;

    egui_view_linearlayout_t layout_1;
    egui_view_label_t label_1;
    egui_view_button_t button_1;    // "Close" 按钮
};

// 2. 重写 on_create
void egui_dialog_test_on_create(egui_dialog_t *self)
{
    egui_dialog_test_t *local = (egui_dialog_test_t *)self;
    egui_dialog_on_create(self);    // 调用父类

    // 初始化控件
    egui_view_linearlayout_init((egui_view_t *)&local->layout_1, egui_dialog_get_core(self));
    // ... 设置按钮回调 ...

    // 添加到 Dialog 内容区域
    egui_dialog_add_view(self, (egui_view_t *)&local->layout_1);

    // 设置背景（圆角矩形）和布局位置（居中）
    egui_view_set_background(
        (egui_view_t *)&self->user_root_view,
        (egui_background_t *)&bg_dialog
    );
    egui_region_t region;
    egui_region_init(&region,
        (EGUI_CONFIG_SCEEN_WIDTH - DIALOG_WIDTH) / 2,
        (EGUI_CONFIG_SCEEN_HEIGHT - DIALOG_HEIGHT) / 2,
        DIALOG_WIDTH, DIALOG_HEIGHT);
    egui_dialog_set_layout(self, &region);
}

// 3. 关闭按钮回调
static void button_1_click_cb(egui_view_t *self)
{
    egui_dialog_t *p_dialog = egui_view_get_dialog(self);
    if (p_dialog)
    {
        egui_dialog_finish(p_dialog);
    }
}
```

## 使用流程

```c
// 在 Activity 的按钮回调中启动 Dialog
static void open_dialog_cb(egui_view_t *self)
{
    egui_activity_t *p_activity = egui_view_get_activity(self);
    if (p_activity)
    {
        egui_dialog_start((egui_dialog_t *)&dialog, p_activity);
    }
}
```

## 相关文件

- `src/app/egui_dialog.h` - Dialog 基类定义
- `src/app/egui_dialog.c` - Dialog 基类实现
- `example/HelloActivity/egui_dialog_test.h/c` - Dialog 示例实现

## Toast Helper

Dialog 同样提供了基于对象的 Toast 快捷接口：

```c
void egui_dialog_show_toast_info(egui_dialog_t *self, const char *text);
void egui_dialog_show_toast_info_with_duration(egui_dialog_t *self, const char *text, uint16_t duration);
```

典型用法：

```c
static void button_1_click_cb(egui_view_t *self)
{
    egui_dialog_t *dialog = egui_view_get_dialog(self);

    if (dialog != NULL)
    {
        egui_dialog_show_toast_info(dialog, "Dialog closing");
        egui_dialog_finish(dialog);
    }
}
```

## Core 解析说明

在常规单屏流程下，`egui_dialog_start_with_current(self)` 会优先使用当前 activity / active core，
`egui_dialog_start(self, activity)` 也会优先复用 `activity` 已绑定的 core，
并在需要时自动把 `self` 绑定到该 core。

`egui_dialog_show_toast_info(...)` /
`egui_dialog_show_toast_info_with_duration(...)` 这类对象式 helper，
则会优先使用 `dialog.root_view` 已绑定的 `core` 去查找默认 Toast。
因此只要 Dialog 在初始化时已经带上目标屏幕的 `core`，
后续显示 Toast 时就不需要再额外传 `core`。

只有在多屏场景下、目标屏幕无法通过当前 activity 唯一确定时，
才需要在构造 Dialog 时直接传入目标屏幕的 `core`。
