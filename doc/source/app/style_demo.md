# HelloStyleDemo 完整解析

HelloStyleDemo 是一个综合性示例，展示了 ViewPage 多页面管理、丰富的控件组合、入场动画和主题切换功能。整个应用包含 4 个页面，通过横向滑动切换。

## 整体架构

```
ViewPage (全屏)
  |-- page_smarthome (Group)   智能家居控制面板
  |-- page_music (Group)       音乐播放器
  |-- page_dashboard (Group)   数据仪表盘
  |-- page_watch (Group)       手表表盘
```

核心文件结构：

| 文件 | 职责 |
|------|------|
| `uicode.c` | 主入口，ViewPage 初始化、页面切换回调、主题管理 |
| `uicode.h` | 公共接口声明 |
| `uicode_smarthome.c` | Smart Home 页面 |
| `uicode_music.c` | Music 页面 |
| `uicode_dashboard.c` | Dashboard 页面 |
| `uicode_watch.c` | Watch 页面 |

## 主入口 (uicode.c)

### ViewPage 初始化

```c
static egui_view_viewpage_t viewpage;
static egui_view_group_t page_smarthome, page_music, page_dashboard, page_watch;

void uicode_create_ui(void)
{
    // 初始化 ViewPage（全屏）
    egui_view_viewpage_init_with_params(EGUI_VIEW_OF(&viewpage), &viewpage_params);

    // 初始化 4 个页面容器
    egui_view_group_init_with_params(EGUI_VIEW_OF(&page_smarthome), &page_params);
    // ... 其他页面 ...

    // 设置各页面背景色
    egui_view_set_background(EGUI_VIEW_OF(&page_smarthome), EGUI_BG_OF(&bg_smarthome));
    // ...

    // 初始化各页面内容
    uicode_init_page_smarthome(EGUI_VIEW_OF(&page_smarthome));
    uicode_init_page_music(EGUI_VIEW_OF(&page_music));
    uicode_init_page_dashboard(EGUI_VIEW_OF(&page_dashboard));
    uicode_init_page_watch(EGUI_VIEW_OF(&page_watch));

    // 添加到 ViewPage 并布局
    egui_view_viewpage_add_child(EGUI_VIEW_OF(&viewpage), EGUI_VIEW_OF(&page_smarthome));
    // ...
    egui_view_viewpage_layout_childs(EGUI_VIEW_OF(&viewpage));
    egui_core_add_user_root_view(EGUI_VIEW_OF(&viewpage));

    // 注册页面切换回调
    egui_view_viewpage_set_on_page_changed(EGUI_VIEW_OF(&viewpage), viewpage_on_page_changed);
    uicode_page_smarthome_on_enter();  // 触发首页入场动画
}
```

### 页面切换回调

每次切换页面时触发对应页面的入场动画：

```c
static void viewpage_on_page_changed(egui_view_t *self, int page_index)
{
    switch (page_index)
    {
    case 0: uicode_page_smarthome_on_enter(); break;
    case 1: uicode_page_music_on_enter(); break;
    case 2: uicode_page_dashboard_on_enter(); break;
    case 3: uicode_page_watch_on_enter(); break;
    }
}
```

## Smart Home 页面

控件组成：
- 标题 Label ("Smart Home")
- 主题切换 Button（Material Icons 图标）
- 4 张 Card，每张包含：
  - 房间名称 Label
  - Switch 开关 或 温度 Label
  - Slider 亮度/温度调节

入场动画：交错式卡片入场（staggered entrance）

```c
// 每张卡片的动画组合：平移 + 透明度
static egui_animation_translate_t sh_anim_trans[4];
static egui_animation_alpha_t sh_anim_alpha[4];
static egui_animation_set_t sh_anim_set[4];

// 平移参数：从下方 30px 滑入
static const egui_animation_translate_params_t sh_trans_params = {
    .from_x = 0, .to_x = 0, .from_y = 30, .to_y = 0
};
// 透明度参数：从 0 渐入到 100%
static const egui_animation_alpha_params_t sh_alpha_params = {
    .from_alpha = 0, .to_alpha = EGUI_ALPHA_100
};
```

交错触发机制：使用定时器每 100ms 启动下一张卡片的动画：

```c
void uicode_page_smarthome_on_enter(void)
{
    // 重置所有卡片为不可见
    for (int i = 0; i < 4; i++)
        egui_view_set_alpha(sh_card_views[i], 0);

    // 立即启动第一张卡片动画
    sh_stagger_idx = 0;
    egui_animation_start(EGUI_ANIM_OF(&sh_anim_set[0]));
    sh_stagger_idx = 1;
    // 每 100ms 启动下一张
    egui_timer_start_timer(&sh_stagger_timer, 100, 100);
}
```

## Music 页面

控件组成：
- Spinner 旋转弧线（专辑封面背景装饰）
- Image 专辑封面
- Label 歌曲名 + 艺术家
- Slider 播放进度条
- Label 当前时间 / 总时长
- 3 个 Button（上一首 / 播放暂停 / 下一首，Material Icons）
- Label 音量图标 + Slider 音量条

交互行为：
- 播放/暂停：切换图标，启动/停止 Spinner 旋转和进度定时器
- 切歌：淡出歌曲名 -> 更新数据 -> 淡入歌曲名

```c
// 切歌动画：淡出 -> 更新 -> 淡入
static void mu_switch_song(int direction)
{
    mu_current_song = (mu_current_song + direction + MU_SONG_COUNT) % MU_SONG_COUNT;
    egui_animation_start(EGUI_ANIM_OF(&mu_fade_out_anim));
}

// 淡出结束回调中更新数据并启动淡入
static void mu_fade_out_end(egui_animation_t *anim)
{
    const mu_song_info_t *song = &mu_songs[mu_current_song];
    egui_view_label_set_text(EGUI_VIEW_OF(&mu_song_title), song->title);
    egui_animation_start(EGUI_ANIM_OF(&mu_fade_in_anim));
}
```

入场动画：进入页面时自动开始播放。

## Dashboard 页面

控件组成：
- 标题 Label ("Dashboard") + 主题切换 Button
- 3 张 KPI Card（CPU / Users / Rate），每张包含数值 Label 和名称 Label
- ChartLine 折线图（CPU 趋势）
- ChartBar 柱状图（销售数据）
- ChartPie 饼图（分类占比）

入场动画：数据增长动画（growth animation）

```c
#define DB_GROWTH_FRAMES 20
#define DB_GROWTH_INTERVAL 40  // 40ms 一帧，总计 800ms

static void db_growth_timer_callback(egui_timer_t *timer)
{
    db_growth_frame++;
    int progress = (db_growth_frame * 100) / DB_GROWTH_FRAMES;
    // 减速缓动：t' = 1 - (1-t)^2
    int t_inv = 100 - progress;
    int decel = 100 - (t_inv * t_inv) / 100;

    // 更新折线图、柱状图、饼图数据
    for (int i = 0; i < 7; i++)
        db_line_pts_anim[i].y = (db_line_target_y[i] * decel) / 100;
    // ... 更新其他图表和 KPI 数值 ...
}
```

所有图表数据从 0 开始，通过定时器逐帧插值到目标值，使用减速缓动曲线。

## Watch 页面

控件组成：
- DigitalClock 数字时钟（12:45，大字体）
- Label 日期
- ActivityRing 三环活动圆环（红/绿/蓝）
- HeartRate 心率显示（带动画）
- Card 天气卡片
- Button 主题切换

入场动画：
- 活动圆环增长动画：从 0 增长到目标值（75/60/45），减速缓动
- 天气卡片淡入动画：alpha 从 0 到 100%

```c
void uicode_page_watch_on_enter(void)
{
    // 重置圆环值
    wt_ring_frame = 0;
    for (int i = 0; i < 3; i++)
        egui_view_activity_ring_set_value(EGUI_VIEW_OF(&wt_ring), i, 0);

    // 启动圆环增长动画
    egui_timer_start_timer(&wt_ring_timer, WT_RING_INTERVAL, WT_RING_INTERVAL);

    // 启动天气卡片淡入
    egui_view_set_alpha(EGUI_VIEW_OF(&wt_weather_card), 0);
    egui_animation_start(EGUI_ANIM_OF(&wt_weather_fade));
}
```

## 主题切换

应用支持亮色/暗色主题切换，通过 `egui_theme_set()` 全局切换：

```c
static uint8_t is_dark_theme = 0;

void uicode_toggle_theme(void)
{
    if (is_dark_theme)
    {
        egui_theme_set(&egui_theme_light);
        is_dark_theme = 0;
    }
    else
    {
        egui_theme_set(&egui_theme_dark);
        is_dark_theme = 1;
    }
    uicode_update_theme_icons();  // 更新所有页面的主题图标
}
```

Smart Home、Dashboard、Watch 三个页面都有主题切换按钮，使用 Material Symbols 图标：
- 亮色模式：`light_mode` (E518)
- 暗色模式：`dark_mode` (E51C)

## 资源配置

HelloStyleDemo 使用的字体资源：

| 字体 | 用途 |
|------|------|
| Montserrat 32pt | Watch 页面时钟 |
| Montserrat 22pt | Dashboard KPI 数值 |
| Montserrat 20pt | 页面标题、歌曲名 |
| Montserrat Medium 20pt | 温度数值 |
| Montserrat 16pt | 日期、艺术家名 |
| Montserrat Medium 14pt | 天气温度 |
| Montserrat 14pt | 卡片文字 |
| Montserrat 12pt | 图表标签、时间 |
| MaterialSymbols Outlined 20pt | 音乐控制图标 |
| MaterialSymbols Outlined 14pt | 主题切换、音量图标 |

图片资源：
- `album_cover` - 音乐页面专辑封面（RGB565 + 4bit alpha）

## 相关文件

- `example/HelloStyleDemo/uicode.c` - 主入口
- `example/HelloStyleDemo/uicode.h` - 公共接口
- `example/HelloStyleDemo/uicode_smarthome.c` - Smart Home 页面
- `example/HelloStyleDemo/uicode_music.c` - Music 页面
- `example/HelloStyleDemo/uicode_dashboard.c` - Dashboard 页面
- `example/HelloStyleDemo/uicode_watch.c` - Watch 页面
