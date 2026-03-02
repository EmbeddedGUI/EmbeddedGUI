# 9 种插值器详解

## 概述

插值器（Interpolator）控制动画的速度曲线。它将线性的时间进度（0.0~1.0）映射为非线性的动画进度，从而实现加速、减速、弹跳等效果。

所有插值器继承自 `egui_interpolator_t` 基类，通过虚函数 `get_interpolation()` 实现映射：

```c
struct egui_interpolator_api
{
    egui_float_t (*get_interpolation)(egui_interpolator_t *self, egui_float_t input);
};
```

输入值 `input` 范围为 [0.0, 1.0]，输出值可以超出此范围（如 overshoot 和 anticipate 类型）。

所有数值运算使用定点数（`egui_float_t`），无需浮点运算单元。

## Linear -- 匀速运动

输出 = 输入，动画匀速进行，无加减速。

适用场景：进度条填充、匀速滚动、透明度线性渐变。

```c
static egui_interpolator_linear_t interp;
egui_interpolator_linear_init((egui_interpolator_t *)&interp);
egui_animation_interpolator_set(EGUI_ANIM_OF(&anim), (egui_interpolator_t *)&interp);
```

## Accelerate -- 加速运动

动画从慢到快，开始时缓慢，结束时速度最大。可通过 `factor` 参数调节加速程度，默认 factor=1.0 时为二次曲线。

适用场景：物体自由落体、页面快速退出。

```c
static egui_interpolator_accelerate_t interp;
egui_interpolator_accelerate_init((egui_interpolator_t *)&interp);

// 可选：调节加速因子（默认 1.0）
egui_interpolator_accelerate_factor_set((egui_interpolator_t *)&interp, EGUI_FLOAT_VALUE(2.0f));

egui_animation_interpolator_set(EGUI_ANIM_OF(&anim), (egui_interpolator_t *)&interp);
```

## Decelerate -- 减速运动

动画从快到慢，开始时速度最大，结束时缓慢停止。`factor` 参数控制减速程度，factor=1.0 时为抛物线曲线。

适用场景：页面滑入、元素入场、列表滚动惯性。

```c
static egui_interpolator_decelerate_t interp;
egui_interpolator_decelerate_init((egui_interpolator_t *)&interp);
egui_animation_interpolator_set(EGUI_ANIM_OF(&anim), (egui_interpolator_t *)&interp);
```

## AccelerateDecelerate -- 先加速后减速

动画前半段加速，后半段减速，中间速度最快。基于余弦曲线实现，产生自然的运动感。

适用场景：通用的 UI 过渡动画、元素位移、大多数默认动画场景。

```c
static egui_interpolator_accelerate_decelerate_t interp;
egui_interpolator_accelerate_decelerate_init((egui_interpolator_t *)&interp);
egui_animation_interpolator_set(EGUI_ANIM_OF(&anim), (egui_interpolator_t *)&interp);
```

## Anticipate -- 回拉后前进

动画先向反方向运动一小段（蓄力），然后加速冲向目标。`tension` 参数控制回拉幅度，tension=0 时退化为普通加速插值器。

适用场景：按钮按下反馈、弹弓效果、需要"蓄力感"的动画。

```c
static egui_interpolator_anticipate_t interp;
egui_interpolator_anticipate_init((egui_interpolator_t *)&interp);

// 可选：调节回拉张力（默认 2.0）
egui_interpolator_anticipate_tension_set((egui_interpolator_t *)&interp, EGUI_FLOAT_VALUE(3.0f));

egui_animation_interpolator_set(EGUI_ANIM_OF(&anim), (egui_interpolator_t *)&interp);
```

## Overshoot -- 越过后回弹

动画冲过目标值后回弹到终点。`tension` 参数控制越过幅度，tension=0 时退化为普通减速插值器。

适用场景：弹性入场、开关切换、元素弹入。

```c
static egui_interpolator_overshoot_t interp;
egui_interpolator_overshoot_init((egui_interpolator_t *)&interp);

// 可选：调节越过张力（默认 2.0）
egui_interpolator_overshoot_tension_set((egui_interpolator_t *)&interp, EGUI_FLOAT_VALUE(1.5f));

egui_animation_interpolator_set(EGUI_ANIM_OF(&anim), (egui_interpolator_t *)&interp);
```

## AnticipateOvershoot -- 回拉 + 回弹

结合 Anticipate 和 Overshoot 的效果：先向反方向回拉，然后冲过目标，最后回弹到终点。`tension` 参数同时控制回拉和越过的幅度。

适用场景：强调性入场动画、需要"弹性"质感的交互。

```c
static egui_interpolator_anticipate_overshoot_t interp;
egui_interpolator_anticipate_overshoot_init((egui_interpolator_t *)&interp);

// 可选：调节张力
egui_interpolator_anticipate_overshoot_tension_set(
    (egui_interpolator_t *)&interp, EGUI_FLOAT_VALUE(2.0f));

egui_animation_interpolator_set(EGUI_ANIM_OF(&anim), (egui_interpolator_t *)&interp);
```

## Bounce -- 弹跳效果

动画到达终点后像球落地一样弹跳数次，最终停在终点。无可调参数。

适用场景：物体落地、通知弹出、数值到达目标后的弹跳反馈。

```c
static egui_interpolator_bounce_t interp;
egui_interpolator_bounce_init((egui_interpolator_t *)&interp);
egui_animation_interpolator_set(EGUI_ANIM_OF(&anim), (egui_interpolator_t *)&interp);
```

## Cycle -- 周期运动

动画按正弦曲线做周期性运动。`cycles` 参数控制在动画时长内完成的周期数。

适用场景：震动提示、左右摇摆、呼吸灯效果。

```c
static egui_interpolator_cycle_t interp;
egui_interpolator_cycle_init((egui_interpolator_t *)&interp);

// 设置周期数（如 2 个完整周期）
egui_interpolator_cycle_tension_set((egui_interpolator_t *)&interp, EGUI_FLOAT_VALUE(2.0f));

egui_animation_interpolator_set(EGUI_ANIM_OF(&anim), (egui_interpolator_t *)&interp);
```

## 插值器对照表

| 插值器 | 结构体 | 可调参数 | 运动特征 |
|--------|--------|----------|----------|
| Linear | `egui_interpolator_linear_t` | 无 | 匀速 |
| Accelerate | `egui_interpolator_accelerate_t` | factor | 慢->快 |
| Decelerate | `egui_interpolator_decelerate_t` | factor | 快->慢 |
| AccelerateDecelerate | `egui_interpolator_accelerate_decelerate_t` | 无 | 慢->快->慢 |
| Anticipate | `egui_interpolator_anticipate_t` | tension | 回拉->前进 |
| Overshoot | `egui_interpolator_overshoot_t` | tension | 越过->回弹 |
| AnticipateOvershoot | `egui_interpolator_anticipate_overshoot_t` | tension | 回拉->越过->回弹 |
| Bounce | `egui_interpolator_bounce_t` | 无 | 弹跳 |
| Cycle | `egui_interpolator_cycle_t` | cycles | 正弦周期 |

## 选型建议

- 通用过渡：AccelerateDecelerate 或 Decelerate
- 入场动画：Decelerate（平滑停止）或 Overshoot（弹性入场）
- 退场动画：Accelerate（加速离开）
- 物理模拟：Bounce（落地弹跳）
- 强调效果：AnticipateOvershoot（回拉+回弹）
- 持续动效：Cycle（周期运动）配合 repeat_count=-1
- 进度/加载：Linear（匀速填充）
