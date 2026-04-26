# Activity 生命周期管理

## 当前实现更新

以下说明以当前代码实现为准：

- Activity 必须在 `egui_activity_init(self, core)` 时直接绑定目标 `core`
- `activity.root_view` 会在 init 阶段一起绑定到同一个 `core`
- `egui_activity_start(self, prev_activity)` 要求 `self` 与 `self.root_view` 已经属于目标 `core`
- 当前实现不会在启动时自动改写 `self->core` 或 `root_view` 的归属关系

`bind_core` 相关旧接口已经删除，多屏场景下应在构造 Activity 时就传入目标屏幕的 `core`。

Activity 是 EmbeddedGUI 中管理全屏页面的核心组件，借鉴了 Android Activity 的设计理念。每个 Activity 拥有独立的视图树和完整的生命周期，通过栈式管理实现页面间的前进与回退。

## 生命周期

Activity 的生命周期由 6 个回调函数组成，按顺序依次触发：

```
onCreate -> onStart -> onResume -> onPause -> onStop -> onDestroy
```

各状态含义：

| 状态 | 说明 |
|------|------|
| `onCreate` | Activity 创建，初始化控件和布局，将视图添加到根视图 |
| `onStart` | Activity 可见，root_view 设为 visible |
| `onResume` | Activity 获得焦点，允许处理触摸事件 |
| `onPause` | Activity 失去焦点，禁止处理触摸事件 |
| `onStop` | Activity 不可见，root_view 设为 hidden |
| `onDestroy` | Activity 销毁，从视图树和 Activity 栈中移除 |

状态枚举定义在 `egui_activity.h` 中：

```c
enum
{
    EGUI_ACTIVITY_STATE_NONE = 0,
    EGUI_ACTIVITY_STATE_CREATE,
    EGUI_ACTIVITY_STATE_START,
    EGUI_ACTIVITY_STATE_RESUME,
    EGUI_ACTIVITY_STATE_PAUSE,
    EGUI_ACTIVITY_STATE_STOP,
    EGUI_ACTIVITY_STATE_DESTROY
};
```

## Activity 结构体

```c
struct egui_activity
{
    egui_dnode_t node;              // Activity 栈中的链表节点
    uint8_t state;                  // 当前状态
    uint8_t is_need_finish;         // 是否需要销毁

    egui_core_t *core;              // Activity 所属的 core

    egui_view_root_group_t root_view; // Activity 的根视图容器

    const egui_activity_api_t *api; // 虚函数表
};
```

虚函数表 `egui_activity_api_t` 定义了 6 个生命周期回调：

```c
struct egui_activity_api
{
    void (*on_create)(egui_activity_t *self);
    void (*on_start)(egui_activity_t *self);
    void (*on_resume)(egui_activity_t *self);
    void (*on_pause)(egui_activity_t *self);
    void (*on_stop)(egui_activity_t *self);
    void (*on_destroy)(egui_activity_t *self);
};
```

## Activity 栈管理

EmbeddedGUI 使用双向链表维护 Activity 栈。核心 API 如下：

```c
// 启动新 Activity（prev_activity 为前一个 Activity，可为 NULL）
void egui_activity_start(egui_activity_t *self, egui_activity_t *prev_activity);

// 结束 Activity（触发 onPause -> onStop -> onDestroy）
void egui_activity_finish(egui_activity_t *self);

// 检查 Activity 是否正处于切换流程中
int egui_activity_check_in_process(egui_activity_t *self);

// 通过子视图查找所属 Activity
egui_activity_t *egui_view_get_activity(egui_view_t *view);
```

启动流程：调用 `egui_activity_start()` 时，框架会先 pause 当前 Activity，然后对新 Activity 依次调用 `onCreate -> onStart -> onResume`。

结束流程：调用 `egui_activity_finish()` 时，框架标记 `is_need_finish`，在 `onStop` 中自动触发 `onDestroy`，同时恢复前一个 Activity。

## 切换动画配置

Activity 切换支持 4 个动画槽位，分别控制启动和结束时新旧 Activity 的过渡效果：

```c
// 设置启动动画：当前 Activity 的进入动画 + 前一个 Activity 的退出动画
void egui_activity_set_start_anim(
    egui_activity_t *self,
    egui_animation_t *open_anim,   // 当前 Activity 从右侧滑入
    egui_animation_t *close_anim   // 前一个 Activity 向左侧滑出
);

// 设置结束动画：前一个 Activity 的恢复动画 + 当前 Activity 的退出动画
void egui_activity_set_finish_anim(
    egui_activity_t *self,
    egui_animation_t *open_anim,   // 前一个 Activity 从左侧滑入
    egui_animation_t *close_anim   // 当前 Activity 向右侧滑出
);
```

典型的水平滑动切换配置：

```c
// 启动时：新页面从右侧滑入
EGUI_ANIMATION_TRANSLATE_PARAMS_INIT(anim_start_open_param,
    EGUI_CONFIG_SCREEN_WIDTH, 0, 0, 0);  // from_x, to_x, from_y, to_y
egui_animation_translate_t anim_start_open;

// 启动时：旧页面向左侧滑出
EGUI_ANIMATION_TRANSLATE_PARAMS_INIT(anim_start_close_param,
    0, -EGUI_CONFIG_SCREEN_WIDTH, 0, 0);
egui_animation_translate_t anim_start_close;

// 结束时：前一个页面从左侧滑入
EGUI_ANIMATION_TRANSLATE_PARAMS_INIT(anim_finish_open_param,
    -EGUI_CONFIG_SCREEN_WIDTH, 0, 0, 0);
egui_animation_translate_t anim_finish_open;

// 结束时：当前页面向右侧滑出
EGUI_ANIMATION_TRANSLATE_PARAMS_INIT(anim_finish_close_param,
    0, EGUI_CONFIG_SCREEN_WIDTH, 0, 0);
egui_animation_translate_t anim_finish_close;
```

如果需要垂直滑动切换，将 x 参数改为 0，y 参数改为 `EGUI_CONFIG_SCREEN_HEIGHT` 即可。

## 自定义 Activity

自定义 Activity 需要：

1. 定义继承结构体（包含 `egui_activity_t base` 和自定义字段）
2. 重写需要的生命周期回调
3. 构建自定义的虚函数表
4. 在 init 函数中调用父类 init 并替换虚函数表

以 HelloActivity 中的 `egui_activity_test_t` 为例：

```c
// 1. 定义结构体
struct egui_activity_test
{
    egui_activity_t base;           // 继承基类

    int index;
    char label_str[20];

    egui_view_linearlayout_t layout_1;
    egui_view_label_t label_1;
    egui_view_button_t button_1;    // "Next" 按钮
    egui_view_button_t button_2;    // "Finish" 按钮
    egui_view_button_t button_3;    // "Dialog" 按钮
};

// 2. 重写 on_create：初始化控件并添加到 Activity
void egui_activity_test_on_create(egui_activity_t *self)
{
    egui_activity_test_t *local = (egui_activity_test_t *)self;
    egui_activity_on_create(self);  // 调用父类

    // 初始化布局和控件...
    egui_view_linearlayout_init((egui_view_t *)&local->layout_1, egui_activity_get_core(self));
    // ... 设置按钮回调 ...
    egui_activity_add_view(self, (egui_view_t *)&local->layout_1);
}

// 3. 构建虚函数表（只替换需要修改的回调）
static const egui_activity_api_t egui_activity_test_t_api_table = {
    .on_create  = egui_activity_test_on_create,  // 自定义
    .on_start   = egui_activity_on_start,        // 使用默认
    .on_resume  = egui_activity_on_resume,
    .on_pause   = egui_activity_on_pause,
    .on_stop    = egui_activity_on_stop,
    .on_destroy = egui_activity_test_on_destroy,  // 自定义
};

// 4. 初始化函数
void egui_activity_test_init(egui_activity_t *self, egui_core_t *core)
{
    egui_activity_init(self, core);       // 调用父类 init
    self->api = &egui_activity_test_t_api_table;  // 替换虚函数表
}
```

## 资源管理

Activity 应用的资源通过 `app_resource_config.json` 配置，包括字体和图片。详细说明参见资源管理章节。

## HelloActivity 完整流程

`example/HelloActivity/uicode_disp0.c` 展示了完整的 Activity 管理流程：

```c
void uicode_disp0_init(egui_core_t *core)
{
    EGUI_UNUSED(core);

    // 1. 初始化 Dialog 动画（底部上滑）
    egui_animation_translate_init(EGUI_ANIM_OF(&anim_dialog_start));
    egui_animation_translate_params_set(&anim_dialog_start, &anim_dialog_start_param);
    egui_animation_duration_set(EGUI_ANIM_OF(&anim_dialog_start), 500);
    // ... 初始化其他动画 ...

    // 2. 初始化 Dialog，并把动画绑定到 Dialog 对象本身
    egui_dialog_test_init((egui_dialog_t *)&dialog, core);
    egui_dialog_set_anim(
        (egui_dialog_t *)&dialog,
        EGUI_ANIM_OF(&anim_dialog_start),
        EGUI_ANIM_OF(&anim_dialog_finish)
    );

    // 3. 启动第一个 Activity
    // uicode_start_next_activity() 内部会对待启动 Activity 调用：
    // egui_activity_set_start_anim(...)
    // egui_activity_set_finish_anim(...)
    // egui_activity_start(...)
    uicode_start_next_activity(NULL);

    // 4. 初始化 Toast
    egui_toast_std_init((egui_toast_t *)&toast, core);
    egui_toast_set_as_default((egui_toast_t *)&toast);
}
```

Activity 的内存分配支持两种模式：

- 静态分配：预分配固定数量的 Activity 数组，适合资源受限场景
- 动态分配：通过 `egui_api_malloc` 动态创建，需在 `onDestroy` 中释放

```c
// 静态分配模式
#define ACTIVITY_ARRAY_SIZE 3
static egui_activity_test_t activity_test_array[ACTIVITY_ARRAY_SIZE];

egui_activity_test_t *uicode_get_empty_activity(void)
{
    if (activity_index >= ACTIVITY_ARRAY_SIZE)
        return NULL;
    return &activity_test_array[activity_index++];
}
```

## 相关文件

- `src/app/egui_activity.h` - Activity 基类定义
- `src/app/egui_activity.c` - Activity 基类实现
- `src/core/egui_core.h` - Activity 栈管理 API
- `example/HelloActivity/` - 完整示例

## Toast Helper

Activity 也提供了基于对象的 Toast 快捷接口：

```c
void egui_activity_show_toast_info(egui_activity_t *self, const char *text);
void egui_activity_show_toast_info_with_duration(egui_activity_t *self, const char *text, uint16_t duration);
```

典型用法：

```c
static void button_next_click_cb(egui_view_t *self)
{
    egui_activity_t *activity = egui_view_get_activity(self);

    if (activity != NULL)
    {
        egui_activity_show_toast_info(activity, "Open next page");
    }
}
```

## Core 解析说明

当前实现里，`egui_activity_start(self, prev_activity)` 只使用 `self` 自身已经绑定的 `core`。
如果 `self` 或 `self.root_view` 不属于传入的目标 `core`，启动流程会直接返回，不会做自动补绑，也不依赖全局 active core。

`egui_activity_show_toast_info(...)` /
`egui_activity_show_toast_info_with_duration(...)` 这类对象式 helper，
则会通过 `activity` 自身绑定的 `core` 去查找默认 Toast。
因此只要 Activity 在初始化时已经带上目标屏幕的 `core`，
后续显示 Toast 时就不需要再额外传 `core`。

多屏场景下更应该遵守这个约束：创建对象时就明确绑定到目标屏幕的 `core`，而不是指望启动阶段推断或修正归属关系。
