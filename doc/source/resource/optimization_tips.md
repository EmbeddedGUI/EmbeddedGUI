# 资源优化技巧

嵌入式 GUI 项目中，Flash 和 RAM 资源极其有限。本文介绍一系列实用的优化技巧，帮助在有限资源下实现最佳的 GUI 效果。

## 字体裁剪

字体资源通常是 Code Size 的主要消耗者之一。

### 只包含需要的字符

在 `supported_text.txt` 中只列出项目实际使用的字符，避免包含多余字符：

```
# 只包含界面上实际显示的文本
Hello World
Settings
OK Cancel
0123456789
```

### 选择合适的字体位深

| fontbitsize | 效果 | 存储占用 |
|-------------|------|---------|
| 1 | 无抗锯齿，锯齿明显 | 最小 |
| 2 | 4 级灰度，轻微抗锯齿 | 较小 |
| 4 | 16 级灰度，效果良好 | 适中（推荐） |
| 8 | 256 级灰度，最佳效果 | 最大 |

对于小字体（12px 以下），1-bit 或 2-bit 即可；中等字体（14-20px）推荐 4-bit；大字体（24px 以上）可考虑 8-bit。

### 控制字体大小种类

每增加一个字体大小，就需要额外的一套字形数据。尽量统一界面中的字体大小，减少字体种类：

```json
{
    "font": [
        {
            "file": "font.ttf",
            "text": "all_text.txt",
            "pixelsize": "16",
            "fontbitsize": "4"
        }
    ]
}
```

避免使用 `"pixelsize": "all"` 或 `"fontbitsize": "all"`，这会生成大量不必要的资源。

## 图片压缩

图片资源往往是占用空间最大的部分。

### 选择合适的像素格式

```
存储占用对比（100x100 像素图片）：
RGB32 (无 Alpha):  40,000 字节
RGB565 (无 Alpha): 20,000 字节
Gray8 (无 Alpha):  10,000 字节
```

优先使用 RGB565，仅在需要高色彩精度时使用 RGB32。

### 降低 Alpha 位深

```
Alpha 通道占用对比（100x100 像素图片）：
Alpha 8-bit: 10,000 字节
Alpha 4-bit:  5,000 字节
Alpha 2-bit:  2,500 字节
Alpha 1-bit:  1,250 字节
Alpha 0:          0 字节
```

大多数 UI 图标使用 4-bit Alpha 即可获得良好的视觉效果。

### 缩放到实际显示尺寸

不要使用超过实际显示尺寸的图片。在 `app_resource_config.json` 中使用 `dim` 参数：

```json
{
    "file": "background.png",
    "dim": "120,160",
    "format": "rgb565",
    "alpha": "0"
}
```

### 将大图片放到外部存储

对于大尺寸、低频访问的图片，使用外部资源：

```json
{
    "file": "splash_screen.png",
    "external": "1",
    "format": "rgb565",
    "alpha": "0"
}
```

## 代码裁剪（条件编译）

EmbeddedGUI 通过 `app_egui_config.h` 中的宏开关控制功能模块的编译。

### 关闭不需要的功能

```c
// 不需要触摸支持
#define EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH 0

// 不需要按键支持
#define EGUI_CONFIG_FUNCTION_SUPPORT_KEY 0

// 不需要焦点系统
#define EGUI_CONFIG_FUNCTION_SUPPORT_FOCUS 0

// 不需要外部资源
#define EGUI_CONFIG_FUNCTION_EXTERNAL_RESOURCE 0

// 不需要 ResourceManager
#define EGUI_CONFIG_FUNCTION_RESOURCE_MANAGER 0

// 不需要软件旋转
#define EGUI_CONFIG_SOFTWARE_ROTATION 0

// 关闭调试信息
#define EGUI_CONFIG_DEBUG_LOG_LEVEL EGUI_LOG_IMPL_LEVEL_NONE
#define EGUI_CONFIG_DEBUG_PFB_REFRESH 0
#define EGUI_CONFIG_DEBUG_DIRTY_REGION_REFRESH 0
#define EGUI_CONFIG_DEBUG_INFO_SHOW 0
```

### 控件级裁剪

项目的构建系统基于 `build.mk` 模块化设计。只在 `build.mk` 中包含实际使用的控件源文件，未引用的控件不会被编译。

## PFB 尺寸优化

PFB（Partial Frame Buffer）大小直接影响 RAM 占用和渲染效率。

### PFB 大小计算

```
PFB RAM = PFB_WIDTH x PFB_HEIGHT x COLOR_BYTES
```

RGB565 下的典型配置：

| PFB 配置 | RAM 占用 | 每帧 draw_area 次数（240x320 屏） |
|----------|---------|----------------------------------|
| 30x40 | 2,400B | 64 次 |
| 60x40 | 4,800B | 32 次 |
| 120x40 | 9,600B | 16 次 |
| 240x40 | 19,200B | 8 次 |
| 240x320 | 153,600B | 1 次（全屏缓冲） |

### 选择策略

- RAM < 4KB：使用最小 PFB（如 15x20），接受较多的 draw_area 调用
- RAM 4-8KB：使用 30x40 的 PFB，平衡 RAM 和效率
- RAM > 16KB：可以使用较大的 PFB 或全屏缓冲

PFB 宽高必须是屏幕宽高的整数约数。

### 双缓冲优化

如果平台支持 DMA 异步传输，启用双缓冲可以让 CPU 和 DMA 并行工作：

```c
#define EGUI_CONFIG_PFB_DOUBLE_BUFFER 1
```

代价是额外一份 PFB 大小的 RAM，但可以显著提升帧率。

## RAM 优化

### Page Union 模式

对于多页面应用，不同页面的控件实例可以使用 `union` 共享内存。因为同一时刻只有一个页面处于活跃状态：

```c
// 多个页面共享同一块内存
typedef union {
    struct {
        egui_view_label_t title;
        egui_view_button_t btn_start;
        egui_view_image_t logo;
    } home_page;
    struct {
        egui_view_label_t setting_title;
        egui_view_switch_t switch_wifi;
        egui_view_switch_t switch_bt;
        egui_view_slider_t slider_brightness;
    } settings_page;
} page_views_t;

static page_views_t page_views;
```

这样，两个页面的控件共享同一块 RAM，总 RAM 占用等于最大页面的占用。

### 减少静态实例

- 使用参数宏（`EGUI_VIEW_XXX_PARAMS_INIT`）初始化控件，参数存储在 Flash 而非 RAM
- 避免不必要的全局变量
- 字符串常量使用 `const` 修饰，确保存储在 Flash

### 栈空间优化

GUI 框架的栈使用量取决于控件嵌套深度。典型配置下 2-4KB 栈空间即可。避免在回调函数中分配大的局部数组。

## 综合优化检查清单

- [ ] 字体只包含实际使用的字符
- [ ] 字体位深选择 4-bit 或更低
- [ ] 统一字体大小种类（不超过 3 种）
- [ ] 图片使用 RGB565 格式
- [ ] Alpha 通道使用 4-bit
- [ ] 图片缩放到实际显示尺寸
- [ ] 大图片使用外部资源
- [ ] 关闭不需要的功能模块
- [ ] 关闭调试日志和调试显示
- [ ] PFB 大小适配 RAM 预算
- [ ] 多页面使用 union 共享内存
- [ ] 运行 `utils_analysis_elf_size.py` 确认优化效果
