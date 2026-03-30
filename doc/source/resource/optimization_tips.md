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
#define EGUI_CONFIG_PFB_BUFFER_COUNT 2
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

### RAM 缓存配置优化

EmbeddedGUI 提供了多个缓存机制来提升渲染性能，但这些缓存会占用 RAM。根据项目的 RAM 预算，可以选择性地禁用或调整这些缓存。

#### 字体渲染缓存

##### Draw Prefix Cache（~2.6 KB BSS）

**最大的 RAM 占用项**，缓存字符串的字形布局元数据（每个字符的 x 坐标、bbox、advance、字形下标等）。

```c
// 默认配置（禁用以节省 RAM）
#define EGUI_CONFIG_FONT_STD_DRAW_PREFIX_CACHE_MAX_GLYPHS 0
#define EGUI_CONFIG_FONT_STD_DRAW_PREFIX_CACHE_SLOTS 0
// RAM 占用：0 B

// 启用缓存（适合静态文字 UI）
#define EGUI_CONFIG_FONT_STD_DRAW_PREFIX_CACHE_MAX_GLYPHS 64
#define EGUI_CONFIG_FONT_STD_DRAW_PREFIX_CACHE_SLOTS 2
// RAM 占用：2 slots × 64 glyphs × ~20 B ≈ 2612 B BSS
```

**适用场景**：静态文字 UI（label、title 等），同一字符串在多帧重复绘制时跳过字符串扫描和字形查找。

**使用建议**：
- 默认关闭以节省 RAM
- 如果应用有大量静态文字（如仪表盘、状态栏），可以开启以提升性能
- 全帧刷新场景（如性能测试、动画）缓存命中率为 0，不应开启

##### Line Cache（~164 B heap）

缓存多行文本的行分割结果，避免每次 `get_str_size` 或绘制时重新扫描 `\n`。

```c
// 默认配置（禁用以节省 RAM）
#define EGUI_CONFIG_FONT_STD_LINE_CACHE_ENABLE 0
// RAM 占用：0 B

// 启用缓存（适合多行文本）
#define EGUI_CONFIG_FONT_STD_LINE_CACHE_ENABLE 1
// RAM 占用：~164 B heap
```

**适用场景**：多行文本控件（如多行 label）。

**使用建议**：
- 默认关闭以节省 RAM
- 如果应用有大量多行文本，可以开启以提升性能
- 仅单行文本或固定字符串的场景无需开启

##### ASCII Lookup Cache（~140 B heap）

为 ASCII (0~127) 字符预建直查表，将字形查找从 O(log n) 降至 O(1)。

```c
// 默认配置（禁用以节省 RAM）
#define EGUI_CONFIG_FONT_STD_ASCII_LOOKUP_CACHE_ENABLE 0
// RAM 占用：0 B

// 启用缓存（适合纯英文 UI）
#define EGUI_CONFIG_FONT_STD_ASCII_LOOKUP_CACHE_ENABLE 1
// RAM 占用：~140 B heap + 8 B BSS
```

**适用场景**：纯英文 UI 应用，频繁 ASCII 字符渲染。

**性能提升**：
- 纯英文 UI：整体渲染加速约 10-20%
- 中英混合 UI：整体渲染加速约 5-10%
- 纯中�� UI：整体渲染加速约 1-2%

**使用建议**：
- 默认关闭以节省 RAM
- 即使禁用，系统仍有多级优化缓存（last_code、block、相邻字符），性能已经很好
- 仅在纯英文 UI 且追求极致性能时开启
- 中文 UI 或低 RAM 场景无需开启

##### ASCII Lookup Index 8-bit（~128 B heap）

将 ASCII lookup 索引从 uint16_t 降为 uint8_t（需配合 `ASCII_LOOKUP_CACHE_ENABLE=1`）。

```c
// 默认配置（支持大字体）
#define EGUI_CONFIG_FONT_STD_ASCII_LOOKUP_INDEX_8BIT 0
// 索引宽度：uint16_t (2 字节)

// 低 RAM 优化（小字体子集）
#define EGUI_CONFIG_FONT_STD_ASCII_LOOKUP_INDEX_8BIT 1
// 索引宽度：uint8_t (1 字节)
// 节省：~128 B heap
// 限制：字体字形数 ≤ 255
```

**适用场景**：小 ASCII 子集字体（如只有 88/93 个字形）。

#### 图像解码缓存

##### RLE External Cache Window（~1 KB BSS）

RLE 外部资源解码���的 I/O 窗口缓存，缓存控制字节（操作码+长度字段）以减少 semihosting I/O 调用。

```c
// 默认配置（适合常规场景）
#define EGUI_CONFIG_IMAGE_RLE_EXTERNAL_CACHE_WINDOW_SIZE 1024
// RAM 占用：~1024 B BSS

// 低 RAM 优化
#define EGUI_CONFIG_IMAGE_RLE_EXTERNAL_CACHE_WINDOW_SIZE 64
// 节省：~960 B BSS
// 说明：像素字面量行（如 240px × 2B = 480 B）超过窗口大小时自动走直接 load，正确性不受影响
```

**适用场景**：外部 RLE 压缩图像资源。

**低 RAM 建议**：64 字节窗口满足控制流缓存需求，同时节省 960 B。

#### 其他小型缓存（< 100 字节）

以下缓存占用较小，可根据需要调整：

```c
// Alpha Opaque Cache（~33 B BSS）
// 缓存"RGB565+alpha 图片的 alpha 通道是否全为不透明"的探测结果
#define EGUI_CONFIG_IMAGE_STD_ALPHA_OPAQUE_CACHE_SLOTS 4  // 默认
#define EGUI_CONFIG_IMAGE_STD_ALPHA_OPAQUE_CACHE_SLOTS 0  // 禁用，节省 ~33 B

// Code Lookup Cache ASCII Compact（~20 B BSS）
// 使用 uint8_t 紧凑字段（仅适用于纯 ASCII 字体）
#define EGUI_CONFIG_FONT_STD_CODE_LOOKUP_CACHE_ASCII_COMPACT 0  // 默认（支持 Unicode）
#define EGUI_CONFIG_FONT_STD_CODE_LOOKUP_CACHE_ASCII_COMPACT 1  // 紧凑模式，节省 ~20 B
```

#### 低 RAM 配置示例

参考 `example/HelloPerformance/app_egui_config.h` 的激进低 RAM 配置：

```c
// 禁用所有字体缓存（节省 ~2.9 KB）
#define EGUI_CONFIG_FONT_STD_DRAW_PREFIX_CACHE_MAX_GLYPHS 0
#define EGUI_CONFIG_FONT_STD_DRAW_PREFIX_CACHE_SLOTS 0
#define EGUI_CONFIG_FONT_STD_ASCII_LOOKUP_CACHE_ENABLE 0
#define EGUI_CONFIG_FONT_STD_LINE_CACHE_ENABLE 0

// 缩小 RLE 窗口（节省 ~960 B）
#define EGUI_CONFIG_IMAGE_RLE_EXTERNAL_CACHE_WINDOW_SIZE 64

// 禁用 Alpha Opaque Cache（节省 ~33 B）
#define EGUI_CONFIG_IMAGE_STD_ALPHA_OPAQUE_CACHE_SLOTS 0

// 启用紧凑模式（节省 ~20 B）
#define EGUI_CONFIG_FONT_STD_CODE_LOOKUP_CACHE_ASCII_COMPACT 1
#define EGUI_CONFIG_FONT_STD_ASCII_LOOKUP_INDEX_8BIT 1
```

**总节省**：约 3.9 KB RAM（BSS + heap）

**代价**：字体渲染性能下降（每帧重新扫描字符串、重新查找字形）。适合全帧刷新的性能测试场景，不适合常规 UI 应用。

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
