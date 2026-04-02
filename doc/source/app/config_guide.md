# 应用配置

EmbeddedGUI 的配置系统基于 C 预处理器宏，通过 `app_egui_config.h` 文件覆盖默认值。每个示例应用都有自己的配置文件，可以根据目标硬件灵活调整。

## 配置机制

配置的加载顺序：

```
app_egui_config.h  (应用自定义，优先级最高)
    -> egui_config_default.h  (框架默认值)
        -> egui_config.h  (最终合并，PC 端强制 32 位色深)
```

所有配置项都使用 `#ifndef` 保护，应用只需在 `app_egui_config.h` 中定义需要修改的项即可。

## 屏幕参数

```c
// 屏幕宽度（像素），范围 8-32767
#define EGUI_CONFIG_SCEEN_WIDTH  240

// 屏幕高度（像素），范围 8-32767
#define EGUI_CONFIG_SCEEN_HEIGHT 320

// 色深：16（RGB565）或 32（ARGB8888）
#define EGUI_CONFIG_COLOR_DEPTH  16
```

注意：PC 模拟器会强制将色深设为 32 位，嵌入式平台通常使用 16 位 RGB565。

## 颜色选项

```c
// RGB565 字节序交换（SPI 接口 LCD 常用）
#define EGUI_CONFIG_COLOR_16_SWAP         0

// SDL 原生 RGB565 显示（PC 模拟器，模拟嵌入式色彩效果）
#define EGUI_CONFIG_SDL_NATIVE_COLOR      0
```

## PFB 参数

PFB（Partial Frame Buffer，局部帧缓冲）是 EmbeddedGUI 的核心设计，用小块缓冲区分次渲染整个屏幕。

```c
// PFB 块宽度（建议为屏幕宽度的整数约数）
#define EGUI_CONFIG_PFB_WIDTH  (EGUI_CONFIG_SCEEN_WIDTH / 8)

// PFB 块高度（建议为屏幕高度的整数约数）
#define EGUI_CONFIG_PFB_HEIGHT (EGUI_CONFIG_SCEEN_HEIGHT / 8)
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

多缓冲需要显示驱动的 `draw_area` 支持异步传输，DMA 完成中断中调用 `egui_pfb_notify_flush_complete()`。

## 性能参数

```c
// 最大帧率限制
#define EGUI_CONFIG_MAX_FPS  60

// 脏区域数量（越多越精确，但占用更多 RAM）
#define EGUI_CONFIG_DIRTY_AREA_COUNT  5

// 触摸输入运动缓存数量
#define EGUI_CONFIG_INPUT_MOTION_CACHE_COUNT  5

// 使用浮点运算（部分 CPU 浮点比定点快）
#define EGUI_CONFIG_PERFORMANCE_USE_FLOAT  0
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
#define EGUI_CONFIG_WIDGET_ENHANCED_DRAW        0

// 软件旋转（90/270 度需要额外缓冲区）
#define EGUI_CONFIG_SOFTWARE_ROTATION           0
```

### 图片格式支持

所有图片格式（RGB32、RGB565、RGB565 调色板压缩、Alpha 遮罩）现在始终编译进框架，无需配置宏开关。未使用的格式会被链接器垃圾回收（`-ffunction-sections` + `--gc-sections`）自动移除，不会增加最终二进制体积。

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
#define EGUI_CONFIG_RECORDING_TEST  0
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
#define EGUI_CONFIG_DEBUG_REFRESH_DELAY          1

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
#define EGUI_CONFIG_SCEEN_WIDTH          240
#define EGUI_CONFIG_SCEEN_HEIGHT         320
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
#define EGUI_CONFIG_SCEEN_WIDTH          128
#define EGUI_CONFIG_SCEEN_HEIGHT         128
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
#define EGUI_CONFIG_SCEEN_WIDTH          480
#define EGUI_CONFIG_SCEEN_HEIGHT         272
#define EGUI_CONFIG_COLOR_DEPTH          16

#define EGUI_CONFIG_PFB_WIDTH            480  // 全宽
#define EGUI_CONFIG_PFB_HEIGHT           34   // 272/8
#define EGUI_CONFIG_PFB_BUFFER_COUNT     3    // 三缓冲

#define EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH       1
#define EGUI_CONFIG_FUNCTION_SUPPORT_MULTI_TOUCH 1
#define EGUI_CONFIG_MAX_FPS              60
#define EGUI_CONFIG_WIDGET_ENHANCED_DRAW 1     // 渐变+阴影
```

PFB 内存：480 * 34 * 2 * 3 = 97920 字节（约 96KB）

## 相关文件

- `src/core/egui_config.h` - 配置入口
- `src/core/egui_config_default.h` - 所有默认配置项
- `example/*/app_egui_config.h` - 各示例的配置文件
