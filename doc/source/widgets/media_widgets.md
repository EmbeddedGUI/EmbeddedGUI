# 媒体控件

## 概述

媒体控件用于播放帧动画和序列图片，适用于启动画面、加载动画、视频预览等嵌入式场景。包括帧动画图片和 MP4 序列帧播放两个控件。

## AnimatedImage

帧动画控件，按指定间隔依次显示一组图片帧，支持循环播放和手动帧控制。

### 效果展示

[在线演示](https://embeddedgui.github.io/EmbeddedGUI/basic.html?app=animated_image)

### API

| 函数 | 说明 |
|------|------|
| `egui_view_animated_image_init(self)` | 初始化帧动画 |
| `egui_view_animated_image_init_with_params(self, params)` | 使用参数初始化 |
| `egui_view_animated_image_set_frames(self, frames, count)` | 设置帧数组和帧数 |
| `egui_view_animated_image_set_interval(self, ms)` | 设置帧间隔(毫秒) |
| `egui_view_animated_image_play(self)` | 开始播放 |
| `egui_view_animated_image_stop(self)` | 停止播放 |
| `egui_view_animated_image_set_loop(self, enable)` | 设置是否循环播放 |
| `egui_view_animated_image_set_current_frame(self, index)` | 跳转到指定帧 |
| `egui_view_animated_image_update(self, elapsed_ms)` | 手动更新播放进度 |

### 参数宏

```c
EGUI_VIEW_ANIMATED_IMAGE_PARAMS_INIT(name, x, y, w, h, interval_ms);
```

### 结构体字段

| 字段 | 类型 | 说明 |
|------|------|------|
| `frames` | `const egui_image_t **` | 帧图片数组指针 |
| `frame_count` | `uint8_t` | 总帧数 |
| `current_frame` | `uint8_t` | 当前帧索引 |
| `is_playing` | `uint8_t` | 是否正在播放 |
| `is_loop` | `uint8_t` | 是否循环 |
| `frame_interval_ms` | `uint16_t` | 帧间隔(毫秒) |

### 代码示例

```c
static egui_view_animated_image_t anim_img;

extern const egui_image_std_t frame_0, frame_1, frame_2, frame_3;
static const egui_image_t *anim_frames[] = {
    (const egui_image_t *)&frame_0,
    (const egui_image_t *)&frame_1,
    (const egui_image_t *)&frame_2,
    (const egui_image_t *)&frame_3,
};

EGUI_VIEW_ANIMATED_IMAGE_PARAMS_INIT(anim_params, 10, 10, 64, 64, 100);

void init_ui(void)
{
    egui_view_animated_image_init_with_params(
        EGUI_VIEW_OF(&anim_img), &anim_params);
    egui_view_animated_image_set_frames(
        EGUI_VIEW_OF(&anim_img), anim_frames, 4);
    egui_view_animated_image_set_loop(EGUI_VIEW_OF(&anim_img), 1);
    egui_view_animated_image_play(EGUI_VIEW_OF(&anim_img));
    egui_core_add_user_root_view(EGUI_VIEW_OF(&anim_img));
}
```

---

## Mp4

MP4 序列帧播放控件，通过定时器驱动逐帧显示图片列表，支持播放完成回调和对齐方式设置。与 AnimatedImage 的区别在于 Mp4 使用内部定时器自动驱动播放。

### 效果展示

[在线演示](https://embeddedgui.github.io/EmbeddedGUI/basic.html?app=mp4)

### API

| 函数 | 说明 |
|------|------|
| `egui_view_mp4_init(self)` | 初始化 Mp4 |
| `egui_view_mp4_init_with_params(self, params)` | 使用参数初始化 |
| `egui_view_mp4_set_mp4_image_list(self, list, count)` | 设置帧图片列表和帧数 |
| `egui_view_mp4_start_work(self, interval_ms)` | 开始播放(指定帧间隔) |
| `egui_view_mp4_stop_work(self)` | 停止播放 |
| `egui_view_mp4_set_align_type(self, align_type)` | 设置图片对齐方式 |
| `egui_view_mp4_set_callback(self, callback)` | 设置播放回调 |

### 回调函数原型

```c
typedef void (*egui_view_mp4_callback_func)(egui_view_mp4_t *self, int is_end);
```

回调在每帧更新时触发，`is_end` 为 1 表示播放到最后一帧。

### 参数宏

```c
EGUI_VIEW_MP4_PARAMS_INIT(name, x, y, w, h);
```

### 代码示例

```c
static egui_view_mp4_t mp4;

extern const egui_image_std_t mp4_frame_0, mp4_frame_1, mp4_frame_2;
static const egui_image_t *mp4_frames[] = {
    (const egui_image_t *)&mp4_frame_0,
    (const egui_image_t *)&mp4_frame_1,
    (const egui_image_t *)&mp4_frame_2,
};

EGUI_VIEW_MP4_PARAMS_INIT(mp4_params, 0, 0, 120, 120);

static void on_mp4_callback(egui_view_mp4_t *self, int is_end)
{
    if (is_end)
    {
        EGUI_LOG_INF("MP4 playback finished\n");
    }
}

void init_ui(void)
{
    egui_view_mp4_init_with_params(EGUI_VIEW_OF(&mp4), &mp4_params);
    egui_view_mp4_set_mp4_image_list(EGUI_VIEW_OF(&mp4), mp4_frames, 3);
    egui_view_mp4_set_callback(EGUI_VIEW_OF(&mp4), on_mp4_callback);
    egui_view_mp4_start_work(EGUI_VIEW_OF(&mp4), 33);
    egui_core_add_user_root_view(EGUI_VIEW_OF(&mp4));
}
```

### 结构体字段

| 字段 | 类型 | 说明 |
|------|------|------|
| `mp4_image_list` | `const egui_image_t **` | 帧图片列表指针 |
| `mp4_image_index` | `uint16_t` | 当前帧索引 |
| `mp4_image_count` | `uint16_t` | 总帧数 |
| `align_type` | `uint8_t` | 图片对齐方式 |
| `anim_timer` | `egui_timer_t` | 内部播放定时器 |

### AnimatedImage 与 Mp4 对比

| 特性 | AnimatedImage | Mp4 |
|------|---------------|-----|
| 播放驱动 | 外部调用 `update()` | 内部定时器自动驱动 |
| 循环控制 | 支持 `set_loop()` | 播放到末帧后停止，通过回调重启 |
| 帧间隔修改 | 可随时调用 `set_interval()` | 在 `start_work()` 时指定 |
| 适用场景 | 简单帧动画、加载指示器 | 视频预览、复杂序列动画 |
