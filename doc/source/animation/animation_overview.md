# 动画系统概述

## 架构简介

EmbeddedGUI 的动画系统采用类 Android 的设计思路，核心概念为：将 Animation 对象绑定到目标 View，由 duration（时长）和 interpolator（插值器）共同驱动属性变化。

动画系统的核心组件关系如下：

```
Animation (基类)
  |-- duration        : 动画时长（毫秒）
  |-- interpolator    : 插值器，控制动画曲线
  |-- target_view     : 目标 View
  |-- handle          : 回调句柄（start/repeat/end）
  |-- repeat_mode     : 重复模式
  |-- repeat_count    : 重复次数
```

动画引擎在每帧调用 `egui_animation_update()`，根据已过时间计算 fraction（0.0~1.0），经插值器变换后传递给具体动画子类的 `on_update()` 方法，由子类将变换结果应用到目标 View 的属性上。

## 动画类型

EmbeddedGUI 提供 6 种内置动画类型：

| 类型 | 结构体 | 说明 |
|------|--------|------|
| Alpha | `egui_animation_alpha_t` | 透明度动画，实现淡入淡出 |
| Translate | `egui_animation_translate_t` | 平移动画，改变 View 的 x/y 位置 |
| ScaleSize | `egui_animation_scale_size_t` | 缩放动画，按比例缩放 View 尺寸 |
| Resize | `egui_animation_resize_t` | 尺寸动画，按宽高比例独立变化 |
| Color | `egui_animation_color_t` | 颜色动画，R/G/B 通道逐通道插值 |
| AnimationSet | `egui_animation_set_t` | 组合动画，同时播放多个子动画 |

## 生命周期

一个动画从创建到结束的完整流程：

```
init -> params_set -> target_view_set -> [interpolator_set] -> [handle_set] -> start
                                                                                  |
                                                                           on_start 回调
                                                                                  |
                                                                          update (每帧)
                                                                                  |
                                                                     fraction 到达 1.0?
                                                                        /            \
                                                                      否              是
                                                                      |               |
                                                                  继续 update    检查 repeat
                                                                                /         \
                                                                          需要重复      不重复
                                                                              |            |
                                                                       on_repeat 回调   on_end 回调
                                                                              |            |
                                                                         重置 start_time   stop
```

### 初始化步骤

```c
// 1. 初始化动画对象
egui_animation_translate_init(EGUI_ANIM_OF(&anim));

// 2. 设置动画参数
EGUI_ANIMATION_TRANSLATE_PARAMS_INIT(params, from_x, to_x, from_y, to_y);
egui_animation_translate_params_set(&anim, &params);

// 3. 设置时长
egui_animation_duration_set(EGUI_ANIM_OF(&anim), 500);

// 4. 绑定目标 View
egui_animation_target_view_set(EGUI_ANIM_OF(&anim), EGUI_VIEW_OF(&my_view));

// 5. （可选）设置插值器
egui_animation_interpolator_set(EGUI_ANIM_OF(&anim), (egui_interpolator_t *)&interp);

// 6. 启动动画
egui_animation_start(EGUI_ANIM_OF(&anim));
```

## 回调机制

通过 `egui_animation_handle_t` 结构体注册三个回调：

```c
typedef struct egui_animation_handle egui_animation_handle_t;
struct egui_animation_handle
{
    void (*start)(egui_animation_t *self);   // 动画开始时触发
    void (*repeat)(egui_animation_t *self);  // 每次重复时触发
    void (*end)(egui_animation_t *self);     // 动画结束时触发
};
```

使用方式：

```c
static void on_start(egui_animation_t *self) { /* ... */ }
static void on_end(egui_animation_t *self)   { /* ... */ }
static void on_repeat(egui_animation_t *self){ /* ... */ }

static const egui_animation_handle_t handle = {
    .start  = on_start,
    .end    = on_end,
    .repeat = on_repeat,
};

egui_animation_handle_set(EGUI_ANIM_OF(&anim), &handle);
```

回调为 `const` 指针，可声明为 `static const` 以节省 RAM。

## 重复模式

| 模式 | 宏定义 | 行为 |
|------|--------|------|
| 重新开始 | `EGUI_ANIMATION_REPEAT_MODE_RESTART` | 每次从起始值重新播放 |
| 反向 | `EGUI_ANIMATION_REPEAT_MODE_REVERSE` | 奇数次正向，偶数次反向，形成往返效果 |

`repeat_count` 设置为 `-1` 表示无限重复，设置为 `0` 表示不重复（播放一次），正整数表示重复指定次数。

```c
// 无限往返
egui_animation_repeat_count_set(EGUI_ANIM_OF(&anim), -1);
egui_animation_repeat_mode_set(EGUI_ANIM_OF(&anim), EGUI_ANIMATION_REPEAT_MODE_REVERSE);
```

## 填充模式

| API | 说明 |
|-----|------|
| `egui_animation_is_fill_before_set(anim, true)` | 动画结束后恢复到起始值（fraction=0） |
| `egui_animation_is_fill_after_set(anim, true)` | 动画结束后保持在终止值（fraction=1） |

`fill_before` 常用于页面退出动画，确保旧页面在动画结束后回到初始位置。

## 常用 API 一览

### 基础 API（egui_animation.h）

| API | 说明 |
|-----|------|
| `egui_animation_init(anim)` | 初始化基类（由子类 init 内部调用） |
| `egui_animation_duration_set(anim, ms)` | 设置动画时长（毫秒） |
| `egui_animation_interpolator_set(anim, interp)` | 设置插值器 |
| `egui_animation_target_view_set(anim, view)` | 绑定目标 View |
| `egui_animation_handle_set(anim, handle)` | 设置回调句柄 |
| `egui_animation_repeat_mode_set(anim, mode)` | 设置重复模式 |
| `egui_animation_repeat_count_set(anim, count)` | 设置重复次数（-1=无限） |
| `egui_animation_is_fill_before_set(anim, flag)` | 结束后恢复起始值 |
| `egui_animation_is_fill_after_set(anim, flag)` | 结束后保持终止值 |
| `egui_animation_start(anim)` | 启动动画 |
| `egui_animation_stop(anim)` | 停止动画 |

### 子类初始化 API

| API | 说明 |
|-----|------|
| `egui_animation_alpha_init(anim)` | 初始化 Alpha 动画 |
| `egui_animation_translate_init(anim)` | 初始化 Translate 动画 |
| `egui_animation_scale_size_init(anim)` | 初始化 ScaleSize 动画 |
| `egui_animation_resize_init(anim)` | 初始化 Resize 动画 |
| `egui_animation_color_init(anim)` | 初始化 Color 动画 |
| `egui_animation_set_init(anim)` | 初始化 AnimationSet |

### 参数设置 API

| API | 说明 |
|-----|------|
| `egui_animation_alpha_params_set(self, params)` | 设置透明度起止值 |
| `egui_animation_translate_params_set(self, params)` | 设置平移起止坐标 |
| `egui_animation_scale_size_params_set(self, params)` | 设置缩放起止比例 |
| `egui_animation_resize_params_set(self, params)` | 设置宽高比例和锚点模式 |
| `egui_animation_color_params_set(self, params)` | 设置起止颜色 |
| `egui_animation_set_add_animation(set, anim)` | 向 AnimationSet 添加子动画 |

## 类型转换宏

动画系统大量使用 `EGUI_ANIM_OF()` 宏进行向上转换：

```c
// 将具体动画类型转换为 egui_animation_t* 基类指针
egui_animation_start(EGUI_ANIM_OF(&my_translate_anim));
```

插值器同理，使用强制类型转换：

```c
egui_animation_interpolator_set(EGUI_ANIM_OF(&anim), (egui_interpolator_t *)&my_interpolator);
```

## 内存使用建议

- 动画参数使用 `PARAMS_INIT` 宏声明为 `static const`，存放在 ROM 中
- 动画对象本身为 `static` 变量，占用 RAM
- 插值器对象同样为 `static` 变量
- 一个 Translate 动画约占 60 字节 RAM，Alpha 动画约 40 字节
- AnimationSet 本身不额外分配内存，子动画通过链表管理
