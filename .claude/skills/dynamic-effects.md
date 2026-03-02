---
name: dynamic-effects
description: Add dynamic effects and interactions to EGUI pages based on Figma Make TSX source
---

# 动态特效与交互行为增强 Skill

Figma/HTML → EGUI 转换管道只生成静态布局。转换完成后，必须根据 Figma Make TSX 源文件中定义的动效和交互，在 EGUI C 代码中实现对应效果。

## 核心原则

**所有动效必须以 Figma Make TSX 源文件为准，不得自行发挥。**

实现前必须：
1. 阅读 `.eguiproject/figmamake_src/src/app/components/*.tsx`
2. 提取其中的 `motion`、`animate`、`transition`、`useState`、`useEffect`、`onClick` 等
3. 将 React/Framer Motion 语义映射为 EGUI Timer/Animation API

## 何时触发

1. 静态布局转换完成并验证通过后
2. 用户要求"加动效"、"加交互"、"让页面动起来"
3. 页面 `*.c` 中只有空的生命周期回调，无定时器/动画/点击

## TSX → EGUI 动效映射规则

| TSX (React/Framer Motion) | EGUI 对应实现 |
|---------------------------|---------------|
| `motion.div initial/animate width` | `egui_timer_t` + `egui_view_progress_bar_set_process()` |
| `transition: { duration, ease: "easeOut" }` | decelerate easing: `100 - (t_inv * t_inv) / 100` |
| `transition: { delay: N * 0.05 }` | 定时器回调中按 id 延迟帧数 |
| `animate-pulse` | `egui_timer_t` 周期性 alpha 切换 |
| `useState + onClick toggle` | `onClick` handler + 状态变量 |
| `whileTap: { scale: 0.98 }` | EGUI 无缩放，可用 alpha 反馈代替 |
| `motion layout spring` | `egui_timer_t` 模拟弹簧位移 |
| `setInterval(fn, 1000)` | `egui_timer_start_timer(&t, 1000, 1)` 周期定时器 |
| `AnimatePresence opacity/x` | 页面切换时 alpha 淡入 |
| `animationDuration: 1500` (recharts) | 图表数据点逐帧增长，总时长 1500ms |
| active tab 高亮 + glow | `on_open` 中 `set_color(cyan)` / `set_color(gray)` |

## 实现步骤

1. 阅读目标页面的 TSX 源文件，列出所有动效和交互
2. 在 `_page.h` 中添加 `egui_timer_t` 和动画状态字段
3. 在 `on_open` 中初始化并启动定时器
4. 在定时器回调中按 TSX 的 duration/easing 插值更新控件
5. 在 `on_close` 中停止定时器
6. 导航栏高亮在 `on_open` 中根据页面 index 设置

## 通用动效代码模板

### 数值增长（对应 motion initial→animate）

```c
static egui_timer_t growth_timer;
static int growth_frame = 0;
#define GROWTH_FRAMES 25       // 帧数
#define GROWTH_INTERVAL 40     // 40ms/帧 → 总时长 1000ms

static void growth_callback(egui_timer_t *timer) {
    growth_frame++;
    if (growth_frame > GROWTH_FRAMES) {
        egui_timer_stop_timer(timer);
        return;
    }
    int progress = (growth_frame * 100) / GROWTH_FRAMES;
    int t_inv = 100 - progress;
    int decel = 100 - (t_inv * t_inv) / 100; // easeOut
    // 用 decel 插值更新控件...
}
```

### 级联延迟（对应 delay: id * 0.05）

```c
// 在回调中，每个元素有不同的起始帧
int cell_start_frame = cell_id * 2; // 2帧 ≈ 80ms delay
if (growth_frame > cell_start_frame) {
    int local_progress = ...;
    // 更新该 cell 的进度条
}
```

## 关键 API

```c
// 定时器
egui_timer_init_timer(&timer, user_data, callback);
egui_timer_start_timer(&timer, interval_ms, period); // period=0一次性, period=1周期
egui_timer_stop_timer(&timer);

// Label 更新
egui_view_label_set_text(EGUI_VIEW_OF(&label), buf);

// 进度条
egui_view_progress_bar_set_process(EGUI_VIEW_OF(&bar), value);

// 颜色
egui_view_set_color(EGUI_VIEW_OF(&view), EGUI_COLOR_HEX(0xRRGGBB));

// 图表
egui_view_chart_line_set_series(EGUI_VIEW_OF(&chart), series, count);
```

## 参考实现

- `example/HelloBattery/uicode.c` — 导航点击回调 + 页面切换防重复
- `example/HelloBattery/dashboard.c` — 完整动效参考（growth/pulse/clock 三定时器）
- `example/HelloBattery/cell_details.c` — 级联延迟动画
- `example/HelloBattery/settings.c` — toggle 开关交互
- `example/HelloStyleDemo/uicode_dashboard.c` — 另一个完整动效参考
- TSX 源文件: `.eguiproject/figmamake_src/src/app/components/*.tsx` — 动效定义的唯一权威来源

## 导航栏点击回调模式

所有页面共享的 `on_nav_*_click` 回调应放在 `uicode.c` 中（而非各页面 .c 文件），因为：
1. 所有 `*_layout.c` 都声明了 `extern void on_nav_*_click()`
2. 回调逻辑完全相同（调用 `uicode_switch_page()`）
3. 避免多重定义链接错误
