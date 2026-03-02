# 动画实战案例

本章通过分析项目中的实际代码，展示动画系统在真实场景中的应用。

## HelloStyleDemo Dashboard 页面动效

源文件：`example/HelloStyleDemo/uicode_dashboard.c`

Dashboard 页面展示了一个完整的数据面板，包含 KPI 卡片、折线图、柱状图和饼图。页面进入时，所有数据从零开始"生长"到目标值，形成入场动效。

### 动效设计

- 动画方式：Timer 驱动，20 帧完成，每帧间隔 40ms（总时长 800ms）
- 缓动曲线：手动实现减速缓动 `t' = 1 - (1-t)^2`
- 动画内容：KPI 数字递增 + 图表数据从零生长到目标值

### 核心实现

数据定义和 Timer 初始化：

```c
static egui_timer_t db_growth_timer;
static int db_growth_frame = 0;
#define DB_GROWTH_FRAMES 20
#define DB_GROWTH_INTERVAL 40

// 目标数据
static const int16_t db_line_target_y[] = {20, 45, 30, 60, 40, 75, 55};
static const int16_t db_bar_target_y[] = {30, 50, 40, 70, 60};
static const uint16_t db_pie_target_vals[] = {40, 30, 20, 10};

// 可变的动画数据（从 0 插值到目标）
static egui_chart_point_t db_line_pts_anim[] = {
    {0, 0}, {1, 0}, {2, 0}, {3, 0}, {4, 0}, {5, 0}, {6, 0},
};
```

Timer 回调中的逐帧更新：

```c
static void db_growth_timer_callback(egui_timer_t *timer)
{
    (void)timer;
    db_growth_frame++;
    if (db_growth_frame > DB_GROWTH_FRAMES)
    {
        egui_timer_stop_timer(&db_growth_timer);
        return;
    }

    // 计算减速缓动进度
    int progress = (db_growth_frame * 100) / DB_GROWTH_FRAMES;
    int t_inv = 100 - progress;
    int decel = 100 - (t_inv * t_inv) / 100;

    // 更新折线图数据点
    for (int i = 0; i < 7; i++)
    {
        db_line_pts_anim[i].y = (int16_t)((db_line_target_y[i] * decel) / 100);
    }
    egui_view_chart_line_set_series(EGUI_VIEW_OF(&db_line_chart), db_line_series_anim, 1);

    // 更新 KPI 数字显示
    db_update_kpi_display(decel);
}
```

页面进入时重置并启动动画：

```c
void uicode_page_dashboard_on_enter(void)
{
    // 重置所有动画数据为零
    db_growth_frame = 0;
    for (int i = 0; i < 7; i++) db_line_pts_anim[i].y = 0;
    for (int i = 0; i < 5; i++) db_bar_pts_anim[i].y = 0;
    for (int i = 0; i < 4; i++) db_pie_slices_anim[i].value = 0;
    db_pie_slices_anim[0].value = 1;  // 饼图至少需要一个非零值
    db_update_kpi_display(0);

    // 启动生长动画
    egui_timer_start_timer(&db_growth_timer, DB_GROWTH_INTERVAL, DB_GROWTH_INTERVAL);
}
```

### 设计要点

1. 每次进入页面都重置状态，确保动画可重复播放
2. 饼图需要保证至少一个 slice 非零，否则渲染异常
3. 减速缓动使数据在前期快速增长、后期缓慢趋近目标，视觉上更自然
4. 一个 Timer 同时驱动 KPI、折线图、柱状图、饼图四类数据的更新

## Activity 切换动画

源文件：`example/HelloActivity/uicode.c`

Activity 是 EmbeddedGUI 的页面管理单元，类似 Android 的 Activity。页面切换时可以配置入场和退场动画。

### 水平滑动切换

新 Activity 从右侧滑入，旧 Activity 向左侧滑出：

```c
// 新页面打开：从屏幕右侧滑入到原位
EGUI_ANIMATION_TRANSLATE_PARAMS_INIT(anim_start_open_param,
    EGUI_CONFIG_SCEEN_WIDTH, 0, 0, 0);
egui_animation_translate_t anim_start_open;

// 旧页面关闭：从原位滑出到屏幕左侧
EGUI_ANIMATION_TRANSLATE_PARAMS_INIT(anim_start_close_param,
    0, -EGUI_CONFIG_SCEEN_WIDTH, 0, 0);
egui_animation_translate_t anim_start_close;

// 返回时，旧页面从左侧滑回
EGUI_ANIMATION_TRANSLATE_PARAMS_INIT(anim_finish_open_param,
    -EGUI_CONFIG_SCEEN_WIDTH, 0, 0, 0);
egui_animation_translate_t anim_finish_open;

// 返回时，当前页面向右滑出
EGUI_ANIMATION_TRANSLATE_PARAMS_INIT(anim_finish_close_param,
    0, EGUI_CONFIG_SCEEN_WIDTH, 0, 0);
egui_animation_translate_t anim_finish_close;
```

初始化和注册：

```c
void uicode_init_ui(void)
{
    // 初始化 start_open 动画
    egui_animation_translate_init(EGUI_ANIM_OF(&anim_start_open));
    egui_animation_translate_params_set(&anim_start_open, &anim_start_open_param);
    egui_animation_duration_set(EGUI_ANIM_OF(&anim_start_open), 300);

    // 初始化 start_close 动画（需要 fill_before 确保结束后回到初始位置）
    egui_animation_translate_init(EGUI_ANIM_OF(&anim_start_close));
    egui_animation_translate_params_set(&anim_start_close, &anim_start_close_param);
    egui_animation_duration_set(EGUI_ANIM_OF(&anim_start_close), 300);
    egui_animation_is_fill_before_set(EGUI_ANIM_OF(&anim_start_close), true);

    // ... finish_open 和 finish_close 类似 ...

    // 注册到 Activity 系统
    egui_core_activity_set_start_anim(
        EGUI_ANIM_OF(&anim_start_open),
        EGUI_ANIM_OF(&anim_start_close));
    egui_core_activity_set_finish_anim(
        EGUI_ANIM_OF(&anim_finish_open),
        EGUI_ANIM_OF(&anim_finish_close));
}
```

### 四个动画的配合关系

| 场景 | 新页面动画 | 旧页面动画 |
|------|-----------|-----------|
| 打开新 Activity | start_open（右->中） | start_close（中->左） |
| 返回上一 Activity | finish_open（左->中） | finish_close（中->右） |

`fill_before` 的作用：退场动画（start_close、finish_close）结束后，需要将 View 恢复到原始位置（偏移量=0），否则下次显示时 View 仍停留在屏幕外。

### 垂直滑动切换（备选方案）

代码中通过 `#if 0` 保留了垂直方向的切换方案：

```c
// 新页面从底部滑入
EGUI_ANIMATION_TRANSLATE_PARAMS_INIT(anim_start_open_param,
    0, 0, EGUI_CONFIG_SCEEN_HEIGHT, 0);

// 旧页面向上滑出
EGUI_ANIMATION_TRANSLATE_PARAMS_INIT(anim_start_close_param,
    0, 0, 0, -EGUI_CONFIG_SCEEN_HEIGHT);
```

## Dialog 弹出动画

Dialog 使用从底部上滑的入场动画和向下滑出的退场动画：

```c
// Dialog 入场：从屏幕底部滑到原位
EGUI_ANIMATION_TRANSLATE_PARAMS_INIT(anim_dialog_start_param,
    0, 0, EGUI_CONFIG_SCEEN_HEIGHT, 0);
egui_animation_translate_t anim_dialog_start;

// Dialog 退场：从原位滑到屏幕底部
EGUI_ANIMATION_TRANSLATE_PARAMS_INIT(anim_dialog_finish_param,
    0, 0, 0, -EGUI_CONFIG_SCEEN_HEIGHT);
egui_animation_translate_t anim_dialog_finish;
```

初始化和注册：

```c
// 入场动画
egui_animation_translate_init(EGUI_ANIM_OF(&anim_dialog_start));
egui_animation_translate_params_set(&anim_dialog_start, &anim_dialog_start_param);
egui_animation_duration_set(EGUI_ANIM_OF(&anim_dialog_start), 500);

// 退场动画
egui_animation_translate_init(EGUI_ANIM_OF(&anim_dialog_finish));
egui_animation_translate_params_set(&anim_dialog_finish, &anim_dialog_finish_param);
egui_animation_duration_set(EGUI_ANIM_OF(&anim_dialog_finish), 500);
egui_animation_is_fill_before_set(EGUI_ANIM_OF(&anim_dialog_finish), true);

// 注册到 Dialog 系统
egui_core_dialog_set_anim(
    EGUI_ANIM_OF(&anim_dialog_start),
    EGUI_ANIM_OF(&anim_dialog_finish));
```

Dialog 动画时长（500ms）比 Activity 切换（300ms）更长，营造从容的弹出感。

## HelloBasic/anim 综合动画演示

源文件：`example/HelloBasic/anim/test.c`

这个示例在一个屏幕上同时展示 4 种动画效果，是学习动画系统的最佳入口。

### 四列布局

| 列 | 动画类型 | 插值器 | 视觉效果 |
|----|---------|--------|----------|
| 1 | Translate | Bounce | 红色圆形上下弹跳 |
| 2 | Alpha | Linear | 绿色方块匀速淡入淡出 |
| 3 | ScaleSize | Overshoot | 蓝色圆形弹性缩放 |
| 4 | AnimationSet | AccelerateDecelerate | 橙色方块下移+淡出 |

所有动画均设置为无限往返（repeat_count=-1, REVERSE 模式），持续播放。

### 关键代码片段

Translate + Bounce 组合：

```c
EGUI_ANIMATION_TRANSLATE_PARAMS_INIT(anim_translate_param,
    0, 0, 0, EGUI_CONFIG_SCEEN_HEIGHT - VIEW1_RADIUS * 2);
static egui_animation_translate_t anim_translate;
static egui_interpolator_bounce_t interp_bounce;

egui_animation_translate_init(EGUI_ANIM_OF(&anim_translate));
egui_animation_translate_params_set(&anim_translate, &anim_translate_param);
egui_animation_duration_set(EGUI_ANIM_OF(&anim_translate), 1500);
egui_animation_repeat_count_set(EGUI_ANIM_OF(&anim_translate), -1);
egui_animation_repeat_mode_set(EGUI_ANIM_OF(&anim_translate), EGUI_ANIMATION_REPEAT_MODE_REVERSE);
egui_interpolator_bounce_init((egui_interpolator_t *)&interp_bounce);
egui_animation_interpolator_set(EGUI_ANIM_OF(&anim_translate), (egui_interpolator_t *)&interp_bounce);
egui_animation_target_view_set(EGUI_ANIM_OF(&anim_translate), EGUI_VIEW_OF(&view_translate));
egui_animation_start(EGUI_ANIM_OF(&anim_translate));
```

AnimationSet 组合动画（第 4 列）展示了如何将 Translate 和 Alpha 合并为一个同步播放的动画组，并通过 mask 机制统一管理属性。
