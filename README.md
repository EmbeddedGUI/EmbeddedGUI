# EmbeddedGUI

[![Compile Check](https://github.com/EmbeddedGUI/EmbeddedGUI/actions/workflows/github-actions-demo.yml/badge.svg)](https://github.com/EmbeddedGUI/EmbeddedGUI/actions/workflows/github-actions-demo.yml) [![Documentation Status](https://readthedocs.org/projects/embeddedgui/badge/?version=latest)](https://embeddedgui.readthedocs.io/en/latest/?badge=latest)

> **面向资源受限嵌入式系统的轻量级 C GUI 框架**  
> RAM < 8 KB · ROM < 64 KB · CPU ~100 MHz · 无 FPU · 纯 C99 · MIT 许可

**[在线体验](https://embeddedgui.github.io/EmbeddedGUI/) · [完整文档](https://embeddedgui.readthedocs.io/en/latest/) · [Gitee](https://gitee.com/embeddedgui/EmbeddedGUI) · [GitHub](https://github.com/EmbeddedGUI/EmbeddedGUI)**

---

## ✨ 为什么选 EmbeddedGUI

| | |
|---|---|
| **超轻量** | HelloSimple：Code 15 KB · Resource 6.6 KB · RAM 800 B · PFB 1.5 KB |
| **PFB 设计** | 局部帧缓冲，小块渲染，无需全屏缓冲，典型 tile 仅 2.4 KB |
| **脏矩形** | 只重绘变化区域，降低功耗，提高帧率 |
| **纯 C99** | 无第三方依赖，支持 C++ 调用，易于移植 |
| **抗锯齿** | 圆 / 弧 / 线全部支持 4×4 子像素抗锯齿，可降级为快速查表模式 |
| **62 个控件** | 从按钮到图表、时钟、仪表盘，开箱即用 |
| **动画系统** | 6 种动画类型 × 9 种插值器，类 Android API |
| **主题系统** | 一套 `EGUI_THEME_*` 设计令牌，全控件统一风格 |
| **中英文** | UTF-8 字体，位图字库 + 资源生成工具链 |
| **UI Designer** | HTML/Figma → XML → C 代码，设计稿直接出代码 |
| **多平台** | PC / STM32G0 / QEMU / WebAssembly / 自定义移植 |

---

## 📸 效果预览



HelloShowcase，所有控件的简单示例。



![HelloShowcase](doc/source/images/HelloShowcase.gif)



HelloStyleDemo，一个常规的多页面应用。

![HelloStyleDemo](doc/source/images/HelloStyleDemo.gif)



---

## 🚀 快速开始

```bash
git clone https://gitee.com/embeddedgui/EmbeddedGUI.git
cd EmbeddedGUI && setup.bat
make all APP=HelloStyleDemo && make run
```

> Linux/Mac 用户参考[环境搭建文档](https://embeddedgui.readthedocs.io/en/latest/)。

---

## 🧩 控件库（62 个）

### 布局 / 容器

| 控件 | 说明 |
|------|------|
| `Group` | 通用容器，Activity 根视图 |
| `LinearLayout` | 水平 / 垂直线性布局 |
| `GridLayout` | 固定列数网格布局 |
| `Scroll` | 可滚动容器（含滚动条） |
| `ViewPage` | 滑动翻页容器 |
| `ViewPageCache` | 带缓存的 ViewPage |
| `TileView` | 分块翻页 |
| `Window` | 浮动窗口 / 叠层 |
| `Card` | 圆角卡片面板 |

### 显示

| 控件 | 说明 |
|------|------|
| `Label` | 静态 UTF-8 文本（多种对齐） |
| `DynamicLabel` | 运行时格式化文本（printf 风格） |
| `Image` | 位图显示（支持缩放） |
| `Divider` | 水平 / 垂直分隔线 |
| `Line` | 通用线段控件 |
| `Textblock` | 多行可编辑文本块 |
| `Spangroup` | 富文本（多段不同样式） |

### 输入 / 交互

| 控件 | 说明 |
|------|------|
| `Button` | 填充按钮，支持点击回调 |
| `ImageButton` | 图标按钮 |
| `ButtonMatrix` | 按钮矩阵 |
| `Switch` | 开关切换 |
| `Checkbox` | 复选框 |
| `RadioButton` | 单选按钮 |
| `ToggleButton` | 保持状态的拨动按钮 |
| `Slider` | 线性拖拽输入 |
| `ArcSlider` | 弧形拖拽输入 |
| `NumberPicker` | 滚轮数字选择 |
| `Roller` | 无限滚动鼓轮 |
| `Combobox` | 下拉组合框 |
| `Spinner` | 加载动画指示器 |
| `TextInput` | 单行键盘文本输入 |
| `Menu` | 上下文 / 下拉菜单 |

### 进度 / 指示

| 控件 | 说明 |
|------|------|
| `ProgressBar` | 水平 / 垂直进度条 |
| `CircularProgressBar` | 圆环进度条 |
| `ActivityRing` | 多环活动环（健身风格） |
| `PageIndicator` | 圆点分页指示器 |
| `TabBar` | 标签导航栏 |
| `Led` | LED 指示灯 |
| `NotificationBadge` | 小徽章计数器 |
| `Scale` | 刻度标尺 |
| `Gauge` | 指针仪表盘 |

### 图表

| 控件 | 说明 |
|------|------|
| `ChartLine` | 折线 / 面积图 |
| `ChartScatter` | 散点 / 气泡图 |
| `ChartBar` | 柱状图 |
| `ChartPie` | 饼图 / 环形图 |

### 时间 / 专业

| 控件 | 说明 |
|------|------|
| `AnalogClock` | 模拟时钟表盘 |
| `DigitalClock` | 数字时间显示 |
| `Stopwatch` | 秒表（含圈计时） |
| `MiniCalendar` | 月历选择器 |
| `Compass` | 指南针方向显示 |
| `HeartRate` | 心率波形 |
| `AnimatedImage` | 帧动画精灵 |
| `Mp4` | JPEG / 序列帧播放 |

### 列表 / 表格

| 控件 | 说明 |
|------|------|
| `List` | 虚拟化滚动列表 |
| `Table` | 行列数据表格 |

---

## 🎨 绘图图元

| 类别 | 能力 |
|------|------|
| 基础图形 | 点、线、矩形、圆角矩形（可独立圆角）、三角形 |
| 圆 / 弧 | 查表模式（快速）+ 4×4 子像素 HQ 抗锯齿模式，支持圆头弧帽 |
| 线段 | 距离场 AA + 4×4 HQ 子像素，支持圆头线帽 |
| 折线 | 普通 / HQ / 圆头折线 |
| 曲线 | 二次 / 三次贝塞尔曲线（可选编译） |
| 椭圆 | 填充 / 描边椭圆（可选编译） |
| 多边形 | 填充 / 描边多边形（可选编译） |
| 渐变填充 | 线性（垂直 / 水平）+ 径向渐变，多停止点，可选抖动 |
| 文本 | 区域内绘制 / 多行 / 对齐，UTF-8 支持 |
| 图片 | 原尺寸 / 缩放绘制，支持染色 |

---

## ✏️ 遮罩系统（4 种）

| 遮罩 | 用途 |
|------|------|
| `mask_circle` | 圆形剪切 |
| `mask_round_rectangle` | 圆角矩形剪切 |
| `mask_gradient` | Alpha 渐变遮罩 |
| `mask_image` | 图片 Alpha 遮罩 |

---

## 🎬 动画系统

**6 种动画类型**：Alpha（淡入淡出）· Translate（平移）· Scale/Size（缩放）· Resize（尺寸过渡）· Color（颜色插值）· AnimationSet（组合动画）

**9 种插值器**：

| 插值器 | 效果 |
|--------|------|
| Linear | 匀速 |
| Accelerate | 先慢后快 |
| Decelerate | 先快后慢 |
| AccelerateDecelerate | 缓入缓出 |
| Anticipate | 先回退再前进 |
| Overshoot | 超出目标后回弹 |
| AnticipateOvershoot | 回退 + 超出回弹 |
| Bounce | 末端弹跳 |
| Cycle | 正弦循环 |

支持：循环次数、RESTART / REVERSE 模式、fill_before / fill_after、start / repeat / end 回调。

---

## ⚙️ 可配置功能开关

所有功能均可在 `app_egui_config.h` 中按需裁剪，零开销禁用。

### 输入系统

| 宏 | 默认 | 说明 |
|----|------|------|
| `EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH` | 1 | 单点触控 |
| `EGUI_CONFIG_FUNCTION_SUPPORT_MULTI_TOUCH` | 0 | 多点触控（双指 + 滚轮） |
| `EGUI_CONFIG_FUNCTION_SUPPORT_KEY` | 0 | 硬件按键事件 |
| `EGUI_CONFIG_FUNCTION_SUPPORT_FOCUS` | 0 | 键盘焦点导航（依赖 KEY） |

### UI 特效

| 宏 | 默认 | 说明 |
|----|------|------|
| `EGUI_CONFIG_FUNCTION_SUPPORT_SHADOW` | 0 | 阴影渲染 |
| `EGUI_CONFIG_FUNCTION_SUPPORT_LAYER` | 1 | Z 轴图层系统 |
| `EGUI_CONFIG_FUNCTION_SUPPORT_SCROLLBAR` | 1 | 自动滚动条指示器 |
| `EGUI_CONFIG_WIDGET_ENHANCED_DRAW` | 0 | 增强渲染（渐变 + 阴影，自动开启两者） |
| `EGUI_CONFIG_FUNCTION_CANVAS_DRAW_GRADIENT` | 0 | 渐变填充 |
| `EGUI_CONFIG_FUNCTION_GRADIENT_DITHERING` | 0 | 渐变抖动（消除 16 位色带） |

### 抗锯齿质量

| 宏 | 默认 | 说明 |
|----|------|------|
| `EGUI_CONFIG_FUNCTION_CANVAS_DRAW_CIRCLE_HQ` | 1 | 圆 / 弧 HQ 子像素模式 |
| `EGUI_CONFIG_FUNCTION_CANVAS_DRAW_LINE_HQ` | 1 | 线段 HQ 子像素模式 |
| `EGUI_CONFIG_CIRCLE_HQ_SAMPLE_2X2` | 0 | 2×2 快速采样（0 = 4×4 高质量） |
| `EGUI_CONFIG_LINE_HQ_SAMPLE_2X2` | 0 | 同上，用于线段 |

### 图像格式

| 宏 | 默认 | 说明 |
|----|------|------|
| `EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_RGB32` | 1 | ARGB8888 |
| `EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_RGB565` | 1 | RGB565 |
| `EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_RGB565_{1/2/4/8}` | 1 | 调色板压缩图像 |
| `EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_ALPHA_{1/2/4/8}` | 1 | Alpha 遮罩格式 |

### 性能 / 内存

| 宏 | 默认 | 说明 |
|----|------|------|
| `EGUI_CONFIG_PFB_BUFFER_COUNT` | 1 | PFB 缓冲数（≥2 支持 DMA 流水线） |
| `EGUI_CONFIG_MAX_FPS` | 60 | 帧率上限 |
| `EGUI_CONFIG_DIRTY_AREA_COUNT` | 5 | 脏矩形区域槽位数 |
| `EGUI_CONFIG_PERFORMANCE_USE_FLOAT` | 0 | 有 FPU 时启用浮点加速 |
| `EGUI_CONFIG_REDUCE_IMAGE_CODE_SIZE` | 0 | 减小图像代码体积（增加 CPU 开销） |
| `EGUI_CONFIG_SOFTWARE_ROTATION` | 0 | 软件旋转 PFB 输出 |

---

## 🖼️ 主题系统

通过 `EGUI_THEME_*` 宏统一管理全局视觉风格，所有控件从同一套设计令牌取色。

| 令牌 | 默认值 | 语义 |
|------|--------|------|
| `EGUI_THEME_PRIMARY` | `#2563EB` | 主题蓝（按钮 / 选中） |
| `EGUI_THEME_SECONDARY` | `#14B8A6` | 次要色（Teal） |
| `EGUI_THEME_SUCCESS` | `#16A34A` | 成功（绿） |
| `EGUI_THEME_WARNING` | `#F59E0B` | 警告（琥珀） |
| `EGUI_THEME_DANGER` | `#DC2626` | 危险（红） |
| `EGUI_THEME_SURFACE` | `#FFFFFF` | 控件背景 |
| `EGUI_THEME_TEXT_PRIMARY` | `#111827` | 正文颜色 |
| `EGUI_THEME_TEXT_SECONDARY` | `#6B7280` | 辅助文字颜色 |
| `EGUI_THEME_FOCUS` | PRIMARY | 焦点环颜色 |
| `EGUI_THEME_RADIUS_MD` | 10 px | 中等圆角半径 |

配合 `EGUI_CONFIG_WIDGET_ENHANCED_DRAW=1` 即可开启全控件渐变 + 阴影的精致外观。

---

## 📱 App 框架

类 Android 多页面架构：

- **Activity**：完整生命周期（CREATE → RESUME → PAUSE → DESTROY），栈式管理，只有栈顶接收输入
- **Dialog**：与 Activity 生命周期同步的浮层对话框，支持点击外部区域关闭
- **Toast**：定时通知气泡，`egui_toast_show(self, text)` 即用即走

简易 多页面架构：

- **Pages**：精简模式，支持多Page切换，RAM可重用（union管理），有基本输入事件分发机制。

---

## 🛠️ UI Designer

从设计稿到 C 代码的全流程工具链：

```
Figma / HTML / JSX ──→ XML ──→ C 源文件 (uicode.c / .h)
```

- **支持导入源**：Stitch HTML · Figma Make JSX/TSX · Figma MCP 插件
- **代码生成**：57 个控件注册描述符，自动生成初始化 / 属性调用代码
- **资源协同**：自动提取字符串，配合 `make resource_refresh` 生成字体 / 图片 C 文件
- **扩展机制**：`scripts/ui_designer/custom_widgets/` 渐进注册新控件，无需修改核心
- **入口**：`python scripts/ui_designer/main.py`
- **实时渲染交互**：支持实时渲染并支持交互。



![UI_Designer](doc/source/images/UI_Designer.gif)

---

## 🔤 字体 / 图像

**字体**：资源字体（`egui_font_std`），内置 Montserrat ，UTF-8 解码，支持多行对齐。

**图像格式**：RGB32（ARGB8888）、RGB565、RGB565 调色板（1/2/4/8 bit）、Alpha 遮罩（1/2/4/8 bit），全部可按需关闭以节省 ROM。

---

## 🖥️ 平台支持

| 平台 | 说明 |
|------|------|
| **PC (SDL2)** | 桌面模拟器，截图输出，快速开发验证 |
| **STM32G0** | ARM Cortex-M0+ 裸机移植 |
| **QEMU** | 微秒级计时器，用于性能基准测试 |
| **WebAssembly** | Emscripten 编译，在线 Demo 直接运行 |
| **自定义移植** | 仅需实现 `draw_area` + `get_tick_ms` 两个接口 |

构建系统：**GNU Make** 与 **CMake** 双支持。

---

## 📦 资源占用

| App | Code (Bytes) | Resource (Bytes) | RAM (Bytes) | PFB (Bytes) | Total ROM (Bytes) |
|-----|-------------|-----------------|------------|------------|------------------|
| HelloActivity | 29956 | 11244 | 1560 | 2400 | 41200 |
| HelloAPP | 58764 | 78644 | 1496 | 2400 | 137408 |
| HelloBasic(activity_ring) | 33460 | 6156 | 924 | 2400 | 39616 |
| HelloBasic(analog_clock) | 29240 | 4700 | 924 | 2400 | 33940 |
| HelloBasic(anim) | 24092 | 4208 | 1172 | 2400 | 28300 |
| HelloBasic(animated_image) | 42808 | 33616 | 884 | 2400 | 76424 |
| HelloBasic(arc_slider) | 26992 | 5164 | 1092 | 2400 | 32156 |
| HelloBasic(button) | 21000 | 10592 | 1176 | 2400 | 31592 |
| HelloBasic(button_img) | 46080 | 38132 | 784 | 2400 | 84212 |
| HelloBasic(button_matrix) | 23136 | 9284 | 860 | 2400 | 32420 |
| HelloBasic(card) | 26176 | 10848 | 1808 | 2400 | 37024 |
| HelloBasic(checkbox) | 27140 | 10552 | 1128 | 2400 | 37692 |
| HelloBasic(circular_progress_bar) | 25948 | 5096 | 1060 | 2400 | 31044 |
| HelloBasic(combobox) | 28176 | 9416 | 1208 | 2400 | 37592 |
| HelloBasic(compass) | 31584 | 10552 | 948 | 2400 | 42136 |
| HelloBasic(digital_clock) | 26136 | 9384 | 900 | 2400 | 35520 |
| HelloBasic(divider) | 14312 | 440 | 1012 | 2400 | 14752 |
| HelloBasic(enhanced_widgets) | 56508 | 11620 | 2484 | 2400 | 68128 |
| HelloBasic(gauge) | 34768 | 5564 | 1060 | 2400 | 40332 |
| HelloBasic(gridlayout) | 20272 | 4960 | 1064 | 2400 | 25232 |
| HelloBasic(heart_rate) | 21640 | 5976 | 948 | 2400 | 27616 |
| HelloBasic(image) | 42592 | 7944 | 768 | 2400 | 50536 |
| HelloBasic(image_button) | 45192 | 8080 | 1064 | 2400 | 53272 |
| HelloBasic(label) | 17368 | 6056 | 1092 | 2400 | 23424 |
| HelloBasic(led) | 19736 | 3752 | 1124 | 2400 | 23488 |
| HelloBasic(line) | 20752 | 508 | 892 | 2400 | 21260 |
| HelloBasic(linearlayout) | 25236 | 9380 | 940 | 2400 | 34616 |
| HelloBasic(list) | 22564 | 10692 | 2120 | 2400 | 33256 |
| HelloBasic(mask) | 44692 | 31324 | 872 | 2400 | 76016 |
| HelloBasic(menu) | 16848 | 6100 | 804 | 2400 | 22948 |
| HelloBasic(mini_calendar) | 24740 | 9368 | 788 | 2400 | 34108 |
| HelloBasic(notification_badge) | 21136 | 9296 | 1108 | 2400 | 30432 |
| HelloBasic(number_picker) | 21788 | 6016 | 1156 | 2400 | 27804 |
| HelloBasic(page_indicator) | 27496 | 9600 | 1212 | 2400 | 37096 |
| HelloBasic(progress_bar) | 20172 | 4956 | 1048 | 2400 | 25128 |
| HelloBasic(radio_button) | 23020 | 9348 | 1148 | 2400 | 32368 |
| HelloBasic(roller) | 17852 | 6020 | 1160 | 2400 | 23872 |
| HelloBasic(scale) | 20756 | 5928 | 876 | 2400 | 26684 |
| HelloBasic(scroll) | 26900 | 9512 | 1116 | 2400 | 36412 |
| HelloBasic(slider) | 20528 | 4952 | 1048 | 2400 | 25480 |
| HelloBasic(spangroup) | 16232 | 5960 | 868 | 2400 | 22192 |
| HelloBasic(spinner) | 25648 | 4820 | 1124 | 2400 | 30468 |
| HelloBasic(stopwatch) | 26404 | 9404 | 988 | 2400 | 35808 |
| HelloBasic(switch) | 20280 | 4960 | 1064 | 2400 | 25240 |
| HelloBasic(table) | 21308 | 5972 | 1300 | 2400 | 27280 |
| HelloBasic(tab_bar) | 27752 | 9640 | 1240 | 2400 | 37392 |
| HelloBasic(textblock) | 23144 | 9568 | 908 | 2400 | 32712 |
| HelloBasic(textinput) | 30840 | 11528 | 4836 | 2400 | 42368 |
| HelloBasic(tileview) | 24976 | 9572 | 1156 | 2400 | 34548 |
| HelloBasic(toggle_button) | 20828 | 9336 | 1124 | 2400 | 30164 |
| HelloBasic(viewpage) | 27040 | 9512 | 1124 | 2400 | 36552 |
| HelloBasic(viewpage_cache) | 26088 | 9484 | 900 | 2400 | 35572 |
| HelloBasic(window) | 16500 | 6080 | 1100 | 2400 | 22580 |
| HelloCanvas | 71076 | 25312 | 1596 | 7200 | 96388 |
| HelloChart | 45348 | 17012 | 2176 | 2400 | 62360 |
| HelloEasyPage | 28248 | 10904 | 1408 | 2400 | 39152 |
| HelloGradient | 62388 | 34480 | 1316 | 4800 | 96868 |
| HelloLayer | 27008 | 32176 | 1624 | 2400 | 59184 |
| HelloPerformace | 114624 | 303508 | 1164 | 2400 | 418132 |
| HelloPFB | 25324 | 10308 | 1036 | 5120 | 35632 |
| HelloResourceManager | 56744 | 14820 | 1140 | 2400 | 71564 |
| HelloSimple | 20780 | 8016 | 968 | 2400 | 28796 |
| HelloStyleDemo | 118308 | 105604 | 7368 | 9600 | 223912 |
| HelloTest | 60776 | 57768 | 1744 | 1536 | 118544 |
| HelloUnitTest | 28168 | 8544 | 1816 | 2400 | 36712 |
| HelloViewPageAndScroll | 29548 | 11208 | 1824 | 2400 | 40756 |

> 运行 `python scripts/utils_analysis_elf_size.py` 生成完整报告。

---

## 📚 示例应用（16 个）

| 示例 | 说明 |
|------|------|
| `HelloSimple` | 最小 Hello World |
| `HelloActivity` | Activity 生命周期演示 |
| `HelloAPP` | 完整多页面应用 |
| `HelloBasic` | **62 个独立控件演示**（含所有控件子应用） |
| `HelloCanvas` | 绘图图元展示 |
| `HelloChart` | 图表控件演示 |
| `HelloGradient` | 渐变填充效果 |
| `HelloLayer` | Z 轴图层演示 |
| `HelloStyleDemo` | 主题 / 增强样式演示 |
| `HelloPerformace` | FPS / 性能基准 |
| `HelloPFB` | PFB 渲染演示 |
| `HelloResourceManager` | 外部资源管理器 |
| `HelloEasyPage` | EasyPage API 演示 |
| `HelloShowcase` | UI 全景展示 |
| `HelloUnitTest` | 单元测试 |
| `HelloViewPageAndScroll` | 翻页 + 滚动演示 |

`HelloBasic` 子应用（60 个）：`anim` · `button` · `slider` · `textinput` · `table` · `menu` · `window` · `analog_clock` · `digital_clock` · `stopwatch` · `gauge` · `compass` · `heart_rate` · `activity_ring` · `list` · `spangroup` · `tileview` · `chart_line` · `chart_bar` · `chart_pie` … 等。

---

## 🔧 工具链

| 工具 | 命令 | 说明 |
|------|------|------|
| 构建 | `make all APP=<APP>` | 编译指定示例 |
| 运行 | `make run` | 启动 PC 模拟器 |
| 资源生成 | `make resource_refresh APP=<APP>` | 生成字体 / 图片 C 文件 |
| 运行时验证 | `python scripts/code_runtime_check.py --app <APP>` | 截图验证渲染正确性 |
| 体积分析 | `python scripts/utils_analysis_elf_size.py` | 生成 ROM/RAM 报告 |
| CI 编译检查 | `python scripts/code_compile_check.py --full-check` | 全量编译检查 |
| UI Designer | `python scripts/ui_designer/main.py` | 设计稿转 C 代码 |





---


## 写在最后

作为芯片从业人员，国产芯片普遍资源有限（ROM和RAM比较少-都是成本，CPU速度比较高-100MHz），需要在512KB ROM，20KB左右RAM资源上实现手环之类的GUI操作（要有触摸），CPU可以跑96MHz。

第一次搞嵌入式GUI，问了一圈朋友，LVGL直接放弃（太绚丽了，个人觉得也不可能跑得动，而且代码应该也比较复杂，魔改会比较困难），有人建议`手撸`，那要死人了。

有朋友推荐了[GuiLite](https://github.com/idea4good/GuiLite)，看了下介绍，GUI简单直接，所需的ROM和RAM也比较少，效果图里面也有很多所需的场景，持续有更新，[ Apache-2.0 license](https://github.com/idea4good/egui#Apache-2.0-1-ov-file)，比较符合我的需求。但是实际看了下，Framebuffer设计所需资源太多了，并没有PFB设计，多个Surface需要多个Framebuffer（跟着屏幕大小来的）；此外自定义控件需要考虑自己清除像素，涉及到透明度和滑动等业务场景时，就非常痛苦了。当然项目也有优势，用户完全掌握UI操作代码（4000行代码，可以轻松看懂）后，基本就不会有多余的MIPS浪费了。

又有朋友推荐了[Arm-2D](https://github.com/ARM-software/Arm-2D)，看了下介绍，很有吸引力，支持PFB，小资源芯片就可以跑高帧率，大公司靠谱。实际一看，各种宏定义，由于设计考虑的是卖芯片，所以基本不会去实现GUI的控件管理和触摸管理，脏矩阵还得自己去定义，不要太麻烦。里面实现了很多酷炫的效果，有很好的借鉴意义。

[lvgl](https://github.com/lvgl/lvgl)，很成熟的一个架构了，玩家也很多，看了下最新的代码v9.1.0，找了一会没找到底层canvas实现，看来是我能力太弱了，效果很酷炫，公司芯片基本没有能跑动的可能，想了下，还是放弃。

综上所述，还是自己写一套吧，什么都可控，酷炫的效果无法实现，那就贴图好了。

现在又有AI了，加入更多渐变、KEY输入、Focus系统、Layer和UI Designer功能，这个固件越来越好用，而且方便维护。



---

## 🔗 相关链接

- 在线体验：https://embeddedgui.github.io/EmbeddedGUI/
- 文档：https://embeddedgui.readthedocs.io/en/latest/
- Gitee：https://gitee.com/embeddedgui/EmbeddedGUI
- GitHub：https://github.com/EmbeddedGUI/EmbeddedGUI

---

## 💬 社区

欢迎大家入群交流讨论。

<table>
  <tr>
    <td align="center"><img src="https://markdown-1306347444.cos.ap-shanghai.myqcloud.com/img/23901725079354_.pic.jpg" width="300px;" height="400px"/><br /><sub><b>QQ</b></sub></a>
  </tr>
</table>
