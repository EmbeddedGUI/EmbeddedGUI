# Fast Path Config

## 目标

`fast path` 相关的默认宏现在统一收口到 `src/core/egui_config_fast_path_default.h`。

这样做的目的有两点：

- 把影响代码体积、渲染热路径、codec/cache 策略的宏集中到一个入口，方便做 size/perf 取舍。
- 避免默认值分散在 `egui_config_default.h`、`egui_config_canvas_default.h` 以及源码内 `#ifndef` 中，降低维护成本。

## 当前分层

- `src/core/egui_config_default.h`
  - 保留通用基础配置，例如屏幕、PFB、功能开关、图片格式、共享 buffer 预算等。
- `src/core/egui_config_canvas_default.h`
  - 保留 canvas 基础行为、HQ 参数、字体格式与通用字体缓存配置。
- `src/core/egui_config_fast_path_default.h`
  - 集中 fast-path 开关，以及与 fast-path 强耦合的 codec/cache policy 宏。

## 新头文件包含的内容

`src/core/egui_config_fast_path_default.h` 主要收纳以下几类宏：

- std image draw/resize fast path
  - 例如 `EGUI_CONFIG_IMAGE_STD_FAST_DRAW_ENABLE`
  - 例如 `EGUI_CONFIG_IMAGE_STD_FAST_RESIZE_ENABLE`
- masked image fast path
  - 例如 `EGUI_CONFIG_IMAGE_STD_MASK_CIRCLE_DRAW_RGB565_FAST_PATH_ENABLE`
  - 例如 `EGUI_CONFIG_IMAGE_STD_MASK_ROUND_RECT_RESIZE_RGB565_FAST_PATH_ENABLE`
- canvas masked-fill fast path
  - 例如 `EGUI_CONFIG_CANVAS_MASK_FILL_ROW_RANGE_FAST_PATH_ENABLE`
  - 例如 `EGUI_CONFIG_CANVAS_MASK_FILL_CIRCLE_SEGMENT_FAST_PATH_ENABLE`
- font fast draw
  - 例如 `EGUI_CONFIG_FONT_STD_FAST_DRAW_ENABLE`
  - 例如 `EGUI_CONFIG_FONT_STD_MASK_ROW_BLEND_FAST_PATH_ENABLE`
- codec/cache policy
  - 例如 `EGUI_CONFIG_IMAGE_CODEC_ROW_CACHE_ENABLE`
  - 例如 `EGUI_CONFIG_IMAGE_QOI_CHECKPOINT_COUNT`
  - 例如 `EGUI_CONFIG_IMAGE_CODEC_TAIL_ROW_CACHE_ENABLE`
  - 例如 `EGUI_CONFIG_IMAGE_RLE_EXTERNAL_WINDOW_PERSISTENT_CACHE_ENABLE`
  - 例如 `EGUI_CONFIG_IMAGE_STD_EXTERNAL_ROW_PERSISTENT_CACHE_ENABLE`
  - 例如 `EGUI_CONFIG_IMAGE_TRANSFORM_EXTERNAL_ROW_PERSISTENT_CACHE_ENABLE`

这里的 `codec/cache policy` 不一定都带 `FAST_PATH` 后缀，但它们直接影响 fast-path 是否成立、热路径行为以及 size/perf/RAM 取舍，因此和 fast-path 默认值一起集中管理。

## 覆盖方式

覆盖方式不变，仍然优先使用应用侧或编译参数覆盖：

```c
#define EGUI_CONFIG_IMAGE_STD_MASK_CIRCLE_DRAW_RGB565_FAST_PATH_ENABLE 0
#define EGUI_CONFIG_CANVAS_MASK_FILL_ROW_RANGE_FAST_PATH_ENABLE 0
#define EGUI_CONFIG_IMAGE_CODEC_TAIL_ROW_CACHE_ENABLE 1
```

这些宏仍然使用 `#ifndef` 提供默认值，因此：

- 可以在 `app_egui_config.h` 中覆写
- 也可以通过 `USER_CFLAGS=-D...` 覆写

## 建议

以后如果要做 size-first 或 perf-first 调整，先看 `src/core/egui_config_fast_path_default.h`。

如果改动的是基础功能或共享资源预算，再回到：

- `src/core/egui_config_default.h`
- `src/core/egui_config_canvas_default.h`

这样可以保持“基础默认值”和“热路径策略默认值”两层职责清晰。
