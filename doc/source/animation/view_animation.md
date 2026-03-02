# View 动画

本章详细介绍 EmbeddedGUI 提供的 5 种 View 属性动画和 AnimationSet 组合动画。

## Alpha 动画 -- 透明度渐变

Alpha 动画在 `from_alpha` 和 `to_alpha` 之间插值，改变目标 View 的透明度。

### 参数结构

```c
struct egui_animation_alpha_params
{
    egui_alpha_t from_alpha;  // 起始透明度
    egui_alpha_t to_alpha;    // 终止透明度
};
```

透明度值范围：`EGUI_ALPHA_0`（完全透明）到 `EGUI_ALPHA_100`（完全不透明）。

### 参数宏

```c
EGUI_ANIMATION_ALPHA_PARAMS_INIT(name, from_alpha, to_alpha);
```

### 完整示例：淡入效果

```c
static egui_animation_alpha_t anim_fade_in;
EGUI_ANIMATION_ALPHA_PARAMS_INIT(fade_in_params, EGUI_ALPHA_0, EGUI_ALPHA_100);
static egui_interpolator_decelerate_t interp;

void setup_fade_in(egui_view_t *view)
{
    egui_animation_alpha_init(EGUI_ANIM_OF(&anim_fade_in));
    egui_animation_alpha_params_set(&anim_fade_in, &fade_in_params);
    egui_animation_duration_set(EGUI_ANIM_OF(&anim_fade_in), 500);

    egui_interpolator_decelerate_init((egui_interpolator_t *)&interp);
    egui_animation_interpolator_set(EGUI_ANIM_OF(&anim_fade_in), (egui_interpolator_t *)&interp);

    egui_animation_target_view_set(EGUI_ANIM_OF(&anim_fade_in), view);
    egui_animation_start(EGUI_ANIM_OF(&anim_fade_in));
}
```

### 完整示例：呼吸灯效果（无限往返）

```c
EGUI_ANIMATION_ALPHA_PARAMS_INIT(breathe_params, EGUI_ALPHA_100, EGUI_ALPHA_20);
static egui_animation_alpha_t anim_breathe;
static egui_interpolator_linear_t interp_linear;

void setup_breathe(egui_view_t *view)
{
    egui_animation_alpha_init(EGUI_ANIM_OF(&anim_breathe));
    egui_animation_alpha_params_set(&anim_breathe, &breathe_params);
    egui_animation_duration_set(EGUI_ANIM_OF(&anim_breathe), 1500);
    egui_animation_repeat_count_set(EGUI_ANIM_OF(&anim_breathe), -1);
    egui_animation_repeat_mode_set(EGUI_ANIM_OF(&anim_breathe), EGUI_ANIMATION_REPEAT_MODE_REVERSE);

    egui_interpolator_linear_init((egui_interpolator_t *)&interp_linear);
    egui_animation_interpolator_set(EGUI_ANIM_OF(&anim_breathe), (egui_interpolator_t *)&interp_linear);

    egui_animation_target_view_set(EGUI_ANIM_OF(&anim_breathe), view);
    egui_animation_start(EGUI_ANIM_OF(&anim_breathe));
}
```

## Translate 动画 -- 平移

Translate 动画改变 View 的位置偏移量，在 (from_x, from_y) 和 (to_x, to_y) 之间插值。偏移量相对于 View 的原始位置。

### 参数结构

```c
struct egui_animation_translate_params
{
    egui_dim_t from_x;  // 起始 X 偏移
    egui_dim_t to_x;    // 终止 X 偏移
    egui_dim_t from_y;  // 起始 Y 偏移
    egui_dim_t to_y;    // 终止 Y 偏移
};
```

### 参数宏

```c
EGUI_ANIMATION_TRANSLATE_PARAMS_INIT(name, from_x, to_x, from_y, to_y);
```

### 完整示例：从右侧滑入

```c
EGUI_ANIMATION_TRANSLATE_PARAMS_INIT(slide_in_params, EGUI_CONFIG_SCEEN_WIDTH, 0, 0, 0);
static egui_animation_translate_t anim_slide_in;
static egui_interpolator_decelerate_t interp;

void setup_slide_in(egui_view_t *view)
{
    egui_animation_translate_init(EGUI_ANIM_OF(&anim_slide_in));
    egui_animation_translate_params_set(&anim_slide_in, &slide_in_params);
    egui_animation_duration_set(EGUI_ANIM_OF(&anim_slide_in), 300);

    egui_interpolator_decelerate_init((egui_interpolator_t *)&interp);
    egui_animation_interpolator_set(EGUI_ANIM_OF(&anim_slide_in), (egui_interpolator_t *)&interp);

    egui_animation_target_view_set(EGUI_ANIM_OF(&anim_slide_in), view);
    egui_animation_start(EGUI_ANIM_OF(&anim_slide_in));
}
```

### 完整示例：弹跳下落

```c
EGUI_ANIMATION_TRANSLATE_PARAMS_INIT(bounce_params, 0, 0, 0, 200);
static egui_animation_translate_t anim_bounce;
static egui_interpolator_bounce_t interp_bounce;

void setup_bounce_drop(egui_view_t *view)
{
    egui_animation_translate_init(EGUI_ANIM_OF(&anim_bounce));
    egui_animation_translate_params_set(&anim_bounce, &bounce_params);
    egui_animation_duration_set(EGUI_ANIM_OF(&anim_bounce), 1500);
    egui_animation_repeat_count_set(EGUI_ANIM_OF(&anim_bounce), -1);
    egui_animation_repeat_mode_set(EGUI_ANIM_OF(&anim_bounce), EGUI_ANIMATION_REPEAT_MODE_REVERSE);

    egui_interpolator_bounce_init((egui_interpolator_t *)&interp_bounce);
    egui_animation_interpolator_set(EGUI_ANIM_OF(&anim_bounce), (egui_interpolator_t *)&interp_bounce);

    egui_animation_target_view_set(EGUI_ANIM_OF(&anim_bounce), view);
    egui_animation_start(EGUI_ANIM_OF(&anim_bounce));
}
```

## ScaleSize 动画 -- 缩放

ScaleSize 动画按比例缩放 View 的宽高，在 `from_scale` 和 `to_scale` 之间插值。缩放基于 View 的原始尺寸。

### 参数结构

```c
struct egui_animation_scale_size_params
{
    egui_float_t from_scale;  // 起始缩放比例（定点数，1.0 = 原始大小）
    egui_float_t to_scale;    // 终止缩放比例
};
```

### 参数宏

```c
EGUI_ANIMATION_SCALE_SIZE_PARAMS_INIT(name, from_scale, to_scale);
```

### 完整示例：弹性放大

```c
EGUI_ANIMATION_SCALE_SIZE_PARAMS_INIT(scale_params,
    EGUI_FLOAT_VALUE(0.3f), EGUI_FLOAT_VALUE(1.0f));
static egui_animation_scale_size_t anim_scale;
static egui_interpolator_overshoot_t interp;

void setup_scale_in(egui_view_t *view)
{
    egui_animation_scale_size_init(EGUI_ANIM_OF(&anim_scale));
    egui_animation_scale_size_params_set(&anim_scale, &scale_params);
    egui_animation_duration_set(EGUI_ANIM_OF(&anim_scale), 800);

    egui_interpolator_overshoot_init((egui_interpolator_t *)&interp);
    egui_animation_interpolator_set(EGUI_ANIM_OF(&anim_scale), (egui_interpolator_t *)&interp);

    egui_animation_target_view_set(EGUI_ANIM_OF(&anim_scale), view);
    egui_animation_start(EGUI_ANIM_OF(&anim_scale));
}
```

## Resize 动画 -- 宽高独立变化

Resize 动画可以独立控制宽度和高度的缩放比例，并支持三种锚点模式。与 ScaleSize 的区别在于：ScaleSize 宽高等比缩放，Resize 可以宽高独立变化。

### 参数结构

```c
struct egui_animation_resize_params
{
    egui_float_t from_width_ratio;   // 起始宽度比例
    egui_float_t to_width_ratio;     // 终止宽度比例
    egui_float_t from_height_ratio;  // 起始高度比例
    egui_float_t to_height_ratio;    // 终止高度比例
    uint8_t mode;                    // 锚点模式
};
```

### 锚点模式

| 模式 | 宏定义 | 说明 |
|------|--------|------|
| 左/上锚定 | `EGUI_ANIMATION_RESIZE_MODE_LEFT` | 左上角固定，向右下方向扩展 |
| 居中锚定 | `EGUI_ANIMATION_RESIZE_MODE_CENTER` | 中心固定，向四周扩展 |
| 右/下锚定 | `EGUI_ANIMATION_RESIZE_MODE_RIGHT` | 右下角固定，向左上方向扩展 |

### 参数宏

```c
EGUI_ANIMATION_RESIZE_PARAMS_INIT(name, from_w, to_w, from_h, to_h, mode);
```

### 完整示例：从中心展开

```c
EGUI_ANIMATION_RESIZE_PARAMS_INIT(expand_params,
    EGUI_FLOAT_VALUE(0.0f), EGUI_FLOAT_VALUE(1.0f),
    EGUI_FLOAT_VALUE(0.0f), EGUI_FLOAT_VALUE(1.0f),
    EGUI_ANIMATION_RESIZE_MODE_CENTER);
static egui_animation_resize_t anim_expand;

void setup_expand(egui_view_t *view)
{
    egui_animation_resize_init(EGUI_ANIM_OF(&anim_expand));
    egui_animation_resize_params_set(&anim_expand, &expand_params);
    egui_animation_duration_set(EGUI_ANIM_OF(&anim_expand), 400);
    egui_animation_target_view_set(EGUI_ANIM_OF(&anim_expand), view);
    egui_animation_start(EGUI_ANIM_OF(&anim_expand));
}
```

## Color 动画 -- 颜色渐变

Color 动画在两个颜色之间逐通道（R/G/B）插值，将结果应用到目标 View 的 label 字体颜色。

### 参数结构

```c
struct egui_animation_color_params
{
    egui_color_t from_color;  // 起始颜色
    egui_color_t to_color;    // 终止颜色
};
```

### 参数宏

```c
EGUI_ANIMATION_COLOR_PARAMS_INIT(name, from_color, to_color);
```

### 完整示例：文字颜色渐变

```c
EGUI_ANIMATION_COLOR_PARAMS_INIT(color_params,
    EGUI_COLOR_MAKE(0xFF, 0x00, 0x00),   // 红色
    EGUI_COLOR_MAKE(0x00, 0x00, 0xFF));  // 蓝色
static egui_animation_color_t anim_color;

void setup_color_change(egui_view_t *label_view)
{
    egui_animation_color_init(EGUI_ANIM_OF(&anim_color));
    egui_animation_color_params_set(&anim_color, &color_params);
    egui_animation_duration_set(EGUI_ANIM_OF(&anim_color), 1000);
    egui_animation_repeat_count_set(EGUI_ANIM_OF(&anim_color), -1);
    egui_animation_repeat_mode_set(EGUI_ANIM_OF(&anim_color), EGUI_ANIMATION_REPEAT_MODE_REVERSE);
    egui_animation_target_view_set(EGUI_ANIM_OF(&anim_color), label_view);
    egui_animation_start(EGUI_ANIM_OF(&anim_color));
}
```

## AnimationSet -- 组合动画

AnimationSet 将多个动画组合在一起同步播放。通过 mask 机制，可以将 Set 级别的属性（duration、repeat、interpolator 等）统一下发给所有子动画。

### 结构体

```c
struct egui_animation_set
{
    egui_animation_t base;
    uint8_t is_mask_repeat_count : 1;
    uint8_t is_mask_repeat_mode : 1;
    uint8_t is_mask_duration : 1;
    uint8_t is_mask_target_view : 1;
    uint8_t is_mask_interpolator : 1;
    egui_slist_t childs;
};
```

### API

```c
// 初始化
egui_animation_set_init(EGUI_ANIM_OF(&set));

// 添加子动画
egui_animation_set_add_animation(&set, EGUI_ANIM_OF(&child_anim));

// 设置 mask（1=将 Set 的属性覆盖到子动画）
egui_animation_set_set_mask(&set,
    is_mask_repeat_count,
    is_mask_repeat_mode,
    is_mask_duration,
    is_mask_target_view,
    is_mask_interpolator);
```

### 完整示例：平移 + 淡出组合

```c
// 子动画参数
EGUI_ANIMATION_TRANSLATE_PARAMS_INIT(set_translate_params, 0, 0, 0, 200);
static egui_animation_translate_t set_translate;

EGUI_ANIMATION_ALPHA_PARAMS_INIT(set_alpha_params, EGUI_ALPHA_100, EGUI_ALPHA_20);
static egui_animation_alpha_t set_alpha;

// 组合动画
static egui_animation_set_t anim_set;
static egui_interpolator_accelerate_decelerate_t interp;

void setup_combined(egui_view_t *view)
{
    // 初始化子动画（不需要单独设置 duration/target/interpolator）
    egui_animation_translate_init(EGUI_ANIM_OF(&set_translate));
    egui_animation_translate_params_set(&set_translate, &set_translate_params);

    egui_animation_alpha_init(EGUI_ANIM_OF(&set_alpha));
    egui_animation_alpha_params_set(&set_alpha, &set_alpha_params);

    // 初始化 AnimationSet
    egui_animation_set_init(EGUI_ANIM_OF(&anim_set));
    egui_animation_set_add_animation(&anim_set, EGUI_ANIM_OF(&set_translate));
    egui_animation_set_add_animation(&anim_set, EGUI_ANIM_OF(&set_alpha));

    // 启用所有 mask：Set 的属性覆盖到子动画
    egui_animation_set_set_mask(&anim_set, 1, 1, 1, 1, 1);

    // 设置 Set 级别的属性
    egui_animation_duration_set(EGUI_ANIM_OF(&anim_set), 1500);
    egui_animation_repeat_count_set(EGUI_ANIM_OF(&anim_set), -1);
    egui_animation_repeat_mode_set(EGUI_ANIM_OF(&anim_set), EGUI_ANIMATION_REPEAT_MODE_REVERSE);

    egui_interpolator_accelerate_decelerate_init((egui_interpolator_t *)&interp);
    egui_animation_interpolator_set(EGUI_ANIM_OF(&anim_set), (egui_interpolator_t *)&interp);

    egui_animation_target_view_set(EGUI_ANIM_OF(&anim_set), view);
    egui_animation_handle_set(EGUI_ANIM_OF(&anim_set), &my_handle);
    egui_animation_start(EGUI_ANIM_OF(&anim_set));
}
```

### mask 机制说明

当 mask 位为 1 时，AnimationSet 在启动时会将自身的对应属性复制到所有子动画：

| mask 位 | 覆盖属性 |
|---------|----------|
| `is_mask_duration` | 子动画使用 Set 的 duration |
| `is_mask_repeat_count` | 子动画使用 Set 的 repeat_count |
| `is_mask_repeat_mode` | 子动画使用 Set 的 repeat_mode |
| `is_mask_target_view` | 子动画使用 Set 的 target_view |
| `is_mask_interpolator` | 子动画使用 Set 的 interpolator |

通常建议全部设为 1，统一由 Set 管理。如果需要子动画有不同的时长或插值器，可以将对应 mask 位设为 0，让子动画保留自己的设置。
