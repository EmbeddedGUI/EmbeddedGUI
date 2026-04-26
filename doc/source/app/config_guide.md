# 应用配置

EmbeddedGUI 的配置系统基于 C 预处理器宏，通过 `app_egui_config.h` 文件覆盖默认值。每个示例应用都有自己的配置文件，可以根据目标硬件灵活调整。

## 配置机制

配置入口是 `src/config/egui_config.h`，当前加载顺序如下：

```
egui_config.h
    -> app_egui_config.h
    -> egui_config_default.h
        -> egui_config_multi_default.h
    -> egui_config_canvas_default.h
    -> egui_config_fast_path_default.h
    -> egui_config_widget_default.h
    -> egui_config_theme_default.h
    -> egui_config_debug_default.h
```

这些默认头都使用 `#ifndef` 保护，因此应用只需在 `app_egui_config.h` 中定义需要覆盖的宏即可。

## 屏幕参数

```c
// 屏幕宽度（像素），范围 8-32767
#define EGUI_CONFIG_SCREEN_WIDTH  240

// 屏幕高度（像素），范围 8-32767
#define EGUI_CONFIG_SCREEN_HEIGHT 320

// 色深：16（RGB565）或 32（ARGB8888）
#define EGUI_CONFIG_COLOR_DEPTH  16
```

注意：PC 模拟器会强制将色深设为 32 位，嵌入式平台通常使用 16 位 RGB565。

## 多屏配置

多屏相关默认值不再全部放在 `egui_config_default.h` 中，而是拆分为：

- `src/config/egui_config_default.h`：主屏基础默认值
- `src/config/egui_config_multi_default.h`：多屏默认值

多屏默认头当前提供：

```c
#define EGUI_CONFIG_MAX_DISPLAY_COUNT 1

#define EGUI_CONFIG_SCREEN_1_WIDTH  EGUI_CONFIG_SCREEN_WIDTH
#define EGUI_CONFIG_SCREEN_1_HEIGHT EGUI_CONFIG_SCREEN_HEIGHT
#define EGUI_CONFIG_PFB_1_WIDTH    EGUI_CONFIG_PFB_WIDTH
#define EGUI_CONFIG_PFB_1_HEIGHT   EGUI_CONFIG_PFB_HEIGHT

#define EGUI_CONFIG_SCREEN_2_WIDTH  EGUI_CONFIG_SCREEN_WIDTH
#define EGUI_CONFIG_SCREEN_2_HEIGHT EGUI_CONFIG_SCREEN_HEIGHT
#define EGUI_CONFIG_PFB_2_WIDTH    EGUI_CONFIG_PFB_WIDTH
#define EGUI_CONFIG_PFB_2_HEIGHT   EGUI_CONFIG_PFB_HEIGHT
```

注意：

- display 0 直接使用 `EGUI_CONFIG_SCREEN_WIDTH`、`EGUI_CONFIG_SCREEN_HEIGHT`、`EGUI_CONFIG_PFB_WIDTH`、`EGUI_CONFIG_PFB_HEIGHT`
- 当前没有 `EGUI_CONFIG_SCREEN_0_*` 或 `EGUI_CONFIG_PFB_0_*`
- 多屏应用可在 `app_egui_config.h` 中覆写 `EGUI_CONFIG_MAX_DISPLAY_COUNT`、`EGUI_CONFIG_SCREEN_1_*`、`EGUI_CONFIG_PFB_1_*`

例如 `HelloMultiDisplayHetero` 当前这样定义副屏 1：

```c
#define EGUI_CONFIG_MAX_DISPLAY_COUNT 2
#define EGUI_CONFIG_SCREEN_1_WIDTH  128
#define EGUI_CONFIG_SCREEN_1_HEIGHT 64
#define EGUI_CONFIG_PFB_1_WIDTH    16
#define EGUI_CONFIG_PFB_1_HEIGHT   8
```

这些多屏宏更适合表达“默认尺寸 / 默认 PFB”这类编译期几何信息，用来减少样板代码。像 `RGB565 byte swap`、`software rotation` 这种能力是否启用，尤其是在多屏应用里不同 core 可能不同的时候，优先放到运行时配置里处理，例如 `egui_display_setup_t.render_config`。

## 颜色选项

```c
// RGB565 字节序交换（SPI 接口 LCD 常用）
#define EGUI_CONFIG_COLOR_16_SWAP         0

// SDL 原生 RGB565 显示（PC 模拟器，模拟嵌入式色彩效果）
#define EGUI_CONFIG_SDL_NATIVE_COLOR      0
```

说明：

- `EGUI_CONFIG_COLOR_16_SWAP` 现在是主屏初始化辅助接口的默认 runtime 值。
- 如果走 `egui_setup_display()`，可以通过 `egui_display_setup_t.render_config->color_16_swap` 按 core 覆盖。
- 多屏场景下不要假设所有屏幕都共用同一个 byte swap 策略。

## PFB 参数

PFB（Partial Frame Buffer，局部帧缓冲）是 EmbeddedGUI 的核心设计，用小块缓冲区分次渲染整个屏幕。

```c
// PFB 块宽度（建议为屏幕宽度的整数约数）
#define EGUI_CONFIG_PFB_WIDTH  (EGUI_CONFIG_SCREEN_WIDTH / 8)

// PFB 块高度（建议为屏幕高度的整数约数）
#define EGUI_CONFIG_PFB_HEIGHT (EGUI_CONFIG_SCREEN_HEIGHT / 8)
```

PFB 内存占用计算：

```
PFB_RAM = PFB_WIDTH * PFB_HEIGHT * (COLOR_DEPTH / 8) * BUFFER_COUNT
```

例如 240x320 屏幕，PFB 30x40，RGB565，单缓冲：
```
30 * 40 * 2 * 1 = 2400 字节
```

### PFB 多缓冲

```c
// PFB 缓冲区数量
// 1 = 单缓冲（默认，同步渲染）
// 2 = 双缓冲（CPU/DMA 流水线，默认）
// 3 = 三缓冲（CPU 可提前 2 块）
#define EGUI_CONFIG_PFB_BUFFER_COUNT  2
```

多缓冲需要显示驱动的 `draw_area` 支持异步传输，DMA 完成中断中调用 `egui_pfb_notify_flush_complete(&core)`。

当前 `EGUI_CONFIG_PFB_BUFFER_COUNT` 的有效范围是 `1..4`。`EGUI_CONFIG_PFB_WIDTH` / `EGUI_CONFIG_PFB_HEIGHT` / `EGUI_CONFIG_DIRTY_AREA_COUNT` 也要求大于 0，这些约束现在统一由 `src/config/egui_config_validate.h` 做编译期检查。

## 性能参数

```c
// 最大帧率限制
#define EGUI_CONFIG_MAX_FPS  60

// 脏区域数量（越多越精确，但占用更多 RAM）
#define EGUI_CONFIG_DIRTY_AREA_COUNT  5

// 触摸输入运动缓存数量
#define EGUI_CONFIG_INPUT_MOTION_CACHE_COUNT  5

// 速度跟踪器（fling/scroll 速度估算；关闭时速度接口返回 0）
#define EGUI_CONFIG_FUNCTION_INPUT_VELOCITY_TRACKER  0

// 使用浮点运算（部分 CPU 浮点比定点快）
#define EGUI_CONFIG_PERFORMANCE_USE_FLOAT  0
```

## 平台服务 Hook

这些宏控制 `egui_api_*` 封装是否转发到平台层注册的回调。默认关闭时直接使用 libc 或内置实现；打开后如果运行时没有注册对应回调，仍会按各接口的 fallback 规则回退。

```c
// malloc/free 走平台回调
#define EGUI_CONFIG_PLATFORM_CUSTOM_MALLOC  0

// 自定义 malloc 回调缺失时保留 libc fallback
#define EGUI_CONFIG_PLATFORM_CUSTOM_MALLOC_LIBC_FALLBACK  1

// 日志输出走平台 printf 回调
#define EGUI_CONFIG_PLATFORM_CUSTOM_PRINTF  0

// memset/memcpy 走平台快速内存操作回调
#define EGUI_CONFIG_PLATFORM_CUSTOM_MEMORY_OP  0
```

## 功能开关

### 输入与交互

```c
// 触摸支持
#define EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH        1

// 多点触摸（缩放、滚轮）
#define EGUI_CONFIG_FUNCTION_SUPPORT_MULTI_TOUCH   0

// 按键事件支持
#define EGUI_CONFIG_FUNCTION_SUPPORT_KEY           0

// 焦点系统（自动启用按键支持）
#define EGUI_CONFIG_FUNCTION_SUPPORT_FOCUS          0

// 按键事件缓存数量
#define EGUI_CONFIG_INPUT_KEY_CACHE_COUNT           5
```

依赖关系：
- 多点触摸自动启用单点触摸
- 焦点系统自动启用按键支持

### 视觉功能

```c
// 阴影效果
#define EGUI_CONFIG_FUNCTION_SUPPORT_SHADOW     0

// 视图图层（Z 轴排序）
#define EGUI_CONFIG_FUNCTION_SUPPORT_LAYER      0

// 滚动条指示器
#define EGUI_CONFIG_FUNCTION_SUPPORT_SCROLLBAR  1

// 增强绘制模式（渐变填充，自动启用阴影和渐变）
#define EGUI_CONFIG_FUNCTION_WIDGET_ENHANCED_DRAW        0

// 软件旋转（90/270 度需要额外缓冲区）
#define EGUI_CONFIG_FUNCTION_SOFTWARE_ROTATION_ENABLE 0
```

说明：

- `EGUI_CONFIG_FUNCTION_SOFTWARE_ROTATION_ENABLE` 同时也是主屏初始化辅助接口的默认 runtime 值。
- 如果走 `egui_setup_display()`，推荐通过 `egui_display_setup_t.render_config->software_rotation` 按 core 控制。
- 如果 display 已经初始化完成，后续也可以调用 `egui_core_set_render_config(core, &config)` 在运行时切换，框架会自动触发一次整屏重绘；如果当前旋转策略会改变该 core 的逻辑宽高，逻辑尺寸也会同步更新。
- 90/270 度软件旋转需要 scratch buffer。若 `render_config.rotation_scratch == NULL`，框架会按当前 PFB 大小在 heap 上按需申请；也可以由调用方自行提供。

示例：

```c
static egui_core_render_config_t disp1_render_config = {
    .color_16_swap = 1,
    .software_rotation = 1,
    .rotation_scratch = NULL,
};

egui_display_setup_t setup = {0};
setup.screen_width = 128;
setup.screen_height = 64;
setup.pfb_width = 16;
setup.pfb_height = 8;
setup.pfb_buffers = disp1_pfb_bufs;
setup.pfb_buffer_count = 1;
setup.display_driver = disp1_driver;
setup.render_config = &disp1_render_config;
setup.uicode_init = uicode_disp1_init;
setup.display_id = 1;
```

### 图片格式支持

图片格式仍由配置宏控制，便于小 ROM 目标裁剪未使用的解码路径。默认只启用基础 RGB565 和 RGB565_4 调色板格式；需要运行时 SVG 时，必须同时启用 `EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_RGB565` 和 `EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_RGB565_8`。

```c
#define EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_RGB32      0
#define EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_RGB565     1
#define EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_RGB565_1   0
#define EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_RGB565_2   0
#define EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_RGB565_4   1
#define EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_RGB565_8   0
#define EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_ALPHA_1    0
#define EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_ALPHA_2    0
#define EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_ALPHA_4    0
#define EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_ALPHA_8    0
```

### Canvas 绘图功能

```c
// 默认圆弧算法使用 HQ
#define EGUI_CONFIG_CIRCLE_DEFAULT_ALGO_HQ          0
```

椭圆、多边形、贝塞尔曲线、高质量圆弧、渐变填充、高质量线段等 Canvas 绘图功能现在始终编译进框架，无需配置宏开关。未使用的绘图函数会被链接器垃圾回收自动移除。

### 文本输入

```c
// TextInput 最大长度
#define EGUI_CONFIG_TEXTINPUT_MAX_LENGTH        32

// TextInput 光标闪烁间隔（ms）
#define EGUI_CONFIG_TEXTINPUT_CURSOR_BLINK_MS   500

// Textblock 编辑缓冲区最大长度
#define EGUI_CONFIG_TEXTBLOCK_EDIT_MAX_LENGTH    256

// Textblock 光标闪烁间隔（ms）
#define EGUI_CONFIG_TEXTBLOCK_CURSOR_BLINK_MS   500
```

### 资源管理

```c
// 外部资源管理器（从外部存储加载资源）
#define EGUI_CONFIG_FUNCTION_EXTERNAL_RESOURCE   0

// 应用资源管理器
#define EGUI_CONFIG_FUNCTION_RESOURCE_MANAGER     0

// 默认字体
#define EGUI_CONFIG_FONT_DEFAULT  &egui_res_font_montserrat_14_4

// 圆角基础半径范围（影响查找表大小）
#define EGUI_CONFIG_CIRCLE_SUPPORT_RADIUS_BASIC_RANGE  50
```

### 其他参数

```c
// Toast 默认显示时间（ms）
#define EGUI_CONFIG_PARAM_TOAST_DEFAULT_SHOW_TIME  1000

// 录制测试模式（自动点击模拟）
#define EGUI_CONFIG_FUNCTION_RECORDING_TEST  0
```

## 代码体积优化

```c
// 限制 margin/padding 为 int8_t（-128~127）
#define EGUI_CONFIG_REDUCE_MARGIN_PADDING_SIZE   1
```

## 调试开关

```c
// PFB 刷新可视化
#define EGUI_CONFIG_DEBUG_PFB_REFRESH            0

// 脏区域刷新可视化
#define EGUI_CONFIG_DEBUG_DIRTY_REGION_REFRESH   0

// 刷新可视化延迟（ms）
#define EGUI_CONFIG_DEBUG_REFRESH_DELAY          0

// 屏幕显示调试信息
#define EGUI_CONFIG_DEBUG_PERF_MONITOR_SHOW      0
#define EGUI_CONFIG_DEBUG_MEM_MONITOR_SHOW       0

// 类名调试（增加 RAM 开销）
#define EGUI_CONFIG_DEBUG_CLASS_NAME             0

// 日志级别：NONE / ERR / WRN / INF / DBG
#define EGUI_CONFIG_DEBUG_LOG_LEVEL  EGUI_LOG_IMPL_LEVEL_NONE

// 简化日志格式
#define EGUI_CONFIG_DEBUG_LOG_SIMPLE             1
```

更完整的调试宏说明、运行截图和脚本流程见 {doc}`../debug/index`。

## 典型配置示例

### 240x320 SPI 彩屏（STM32G0，8KB RAM）

```c
// app_egui_config.h
#define EGUI_CONFIG_SCREEN_WIDTH          240
#define EGUI_CONFIG_SCREEN_HEIGHT         320
#define EGUI_CONFIG_COLOR_DEPTH          16
#define EGUI_CONFIG_COLOR_16_SWAP        1    // SPI 接口需要字节交换

#define EGUI_CONFIG_PFB_WIDTH            30   // 240/8
#define EGUI_CONFIG_PFB_HEIGHT           40   // 320/8
#define EGUI_CONFIG_PFB_BUFFER_COUNT     2    // 双缓冲，DMA 流水线

#define EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH  1
#define EGUI_CONFIG_MAX_FPS              30   // SPI 带宽有限
```

PFB 内存：30 * 40 * 2 * 2 = 4800 字节

### 128x128 OLED（低功耗 MCU，4KB RAM）

```c
// app_egui_config.h
#define EGUI_CONFIG_SCREEN_WIDTH          128
#define EGUI_CONFIG_SCREEN_HEIGHT         128
#define EGUI_CONFIG_COLOR_DEPTH          16

#define EGUI_CONFIG_PFB_WIDTH            32   // 128/4
#define EGUI_CONFIG_PFB_HEIGHT           16   // 128/8
#define EGUI_CONFIG_PFB_BUFFER_COUNT     1    // 单缓冲节省 RAM

#define EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH  0  // 无触摸
#define EGUI_CONFIG_FUNCTION_SUPPORT_KEY    1  // 按键控制
#define EGUI_CONFIG_MAX_FPS              30
#define EGUI_CONFIG_DIRTY_AREA_COUNT     3

// 极致精简
#define EGUI_CONFIG_FUNCTION_SUPPORT_SHADOW     0
#define EGUI_CONFIG_FUNCTION_SUPPORT_LAYER      0
#define EGUI_CONFIG_FUNCTION_SUPPORT_SCROLLBAR  0
#define EGUI_CONFIG_REDUCE_MARGIN_PADDING_SIZE  1
```

PFB 内存：32 * 16 * 2 * 1 = 1024 字节

### 480x272 RGB 接口屏（STM32H7，大 RAM）

```c
// app_egui_config.h
#define EGUI_CONFIG_SCREEN_WIDTH          480
#define EGUI_CONFIG_SCREEN_HEIGHT         272
#define EGUI_CONFIG_COLOR_DEPTH          16

#define EGUI_CONFIG_PFB_WIDTH            480  // 全宽
#define EGUI_CONFIG_PFB_HEIGHT           34   // 272/8
#define EGUI_CONFIG_PFB_BUFFER_COUNT     3    // 三缓冲

#define EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH       1
#define EGUI_CONFIG_FUNCTION_SUPPORT_MULTI_TOUCH 1
#define EGUI_CONFIG_MAX_FPS              60
#define EGUI_CONFIG_FUNCTION_WIDGET_ENHANCED_DRAW 1     // 渐变+阴影
```

PFB 内存：480 * 34 * 2 * 3 = 97920 字节（约 96KB）

## 相关文件

- `src/config/egui_config.h` - 配置入口
- `src/config/egui_config_default.h` - 主屏基础默认配置
- `src/config/egui_config_multi_default.h` - 多屏默认配置
- `example/*/app_egui_config.h` - 各示例的配置文件
