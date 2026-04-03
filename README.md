# EmbeddedGUI

[![Compile Check](https://github.com/EmbeddedGUI/EmbeddedGUI/actions/workflows/github-actions-demo.yml/badge.svg)](https://github.com/EmbeddedGUI/EmbeddedGUI/actions/workflows/github-actions-demo.yml) [![Documentation Status](https://readthedocs.org/projects/embeddedgui/badge/?version=latest)](https://embeddedgui.readthedocs.io/en/latest/?badge=latest)

> **面向资源受限嵌入式系统的轻量级 C GUI 框架**
> RAM < 8 KB · ROM < 64 KB · CPU ~100 MHz · 无 FPU · 纯 C99 · MIT 许可

**[在线体验](https://embeddedgui.github.io/EmbeddedGUI/) · [完整文档](https://embeddedgui.readthedocs.io/en/latest/) · [Gitee](https://gitee.com/embeddedgui/EmbeddedGUI) · [GitHub](https://github.com/EmbeddedGUI/EmbeddedGUI)**

---

## 📸 效果预览

HelloShowcase，所有控件的简单示例。

![HelloShowcase](doc/source/images/HelloShowcase.gif)

HelloStyleDemo，一个常规的多页面应用。

![HelloStyleDemo](doc/source/images/HelloStyleDemo.gif)

---

## ✨ 超轻量 PFB 架构

不需要全屏帧缓冲，30×40 像素的小块缓冲（RGB565 仅 **2.4 KB**）即可驱动整个屏幕。

**HelloSimple 资源占用**：

| Code | Resource | RAM | PFB | Total ROM |
|------|----------|-----|-----|-----------|
| 20,780 B (~20 KB) | 8,016 B (~8 KB) | 968 B | 2,400 B | 28,796 B |

- **脏矩形**：只重绘变化区域，静态画面零 CPU 消耗，降低功耗
- **纯 C99**：无第三方依赖，支持 C++ 调用，易于移植
- **抗锯齿**：圆 / 弧 / 线全部支持 4×4 子像素抗锯齿，可降级为快速查表模式

---

## 🔄 Virtual 控件 —— 按需实例化，省 RAM

传统方式 100 个列表项 = 100 个控件实例；Virtual 模式**只实例化屏幕可见项**，其余回收复用。

| | 传统模式 | Virtual 模式 |
|---|---|---|
| 100 项列表 | 100 个实例 | 仅可见项实例化 |
| 内存策略 | 全部常驻 | 可见区 + overscan 缓冲 |

**底层组件**（`src/widget/egui_view_virtual_*.h`）：Viewport · Page · Grid · Strip · Section List · Tree · Stage

**高级封装**（ViewHolder 模式）：ListView · GridView

> 示例：[example/HelloVirtual](example/HelloVirtual)（19 个子应用）

---

## ✏️ 4 种遮罩，像素级视觉效果

| 遮罩 | 用途 |
|------|------|
| `mask_circle` | 圆形剪切 |
| `mask_round_rectangle` | 圆角矩形剪切 |
| `mask_gradient` | Alpha 渐变遮罩 |
| `mask_image` | 图片 Alpha 遮罩 |

支持**行级批处理优化**，非逐像素暴力遍历，在保证效果的同时兼顾性能。

---

## 📊 QEMU 微秒级性能基准 —— 每次提交都可量化

基于 QEMU 指令级模拟（`-icount shift=0`），同一代码在任何机器上得到**相同结果**，回归检测阈值 10%。

![Performance Scenes](doc/source/performance/images/perf_scenes.png)

**关键数据**（Cortex-M3 profile）：

| 场景 | 耗时 |
|------|------|
| TEXT | 0.904 ms |
| CIRCLE_FILL | 1.648 ms |
| GRADIENT_CIRCLE | 10.461 ms |
| MASK_IMAGE_CIRCLE | 2.957 ms |
| ANIMATION_TRANSLATE | 0.276 ms |

覆盖 **~100 个测试场景**，涵盖图形 / 文本 / 图片 / 遮罩 / 动画，支持 PFB Matrix / SPI Matrix 矩阵测试。

---

## 📱 三种页面方案，按需选择

| | Activity | Page | Virtual Stage |
|---|---|---|---|
| 复杂度 | 完整生命周期（6 状态） | 轻量（open/close） | 控件级容器 |
| 内存 | 栈式管理，常驻 | union 可重用 RAM | 回收池复用 |
| 适用场景 | 导航栈、完整 App | 简单多页切换 | 仪表盘、叠层布局 |

- **Activity**：类 Android 生命周期（CREATE → RESUME → PAUSE → DESTROY），配套 Dialog（浮层对话框）和 Toast（通知气泡）
- **Page**：精简模式，RAM 通过 union 重用，有基本输入事件分发
- **Virtual Stage**：绝对定位 + Z 轴排序，支持 pin/unpin 和 hit testing

> 小项目用 Page 省到极致，复杂项目用 Activity 完整管控，高级场景用 Virtual Stage 动态编排。

---

## 🚀 快速开始

```bash
git clone https://gitee.com/embeddedgui/EmbeddedGUI.git
cd EmbeddedGUI
# Windows
setup.bat
# Linux / macOS
./setup.sh
make all APP=HelloStyleDemo && make run
```

> 安装脚本默认会创建 `.venv` 并安装当前仓库所需的 Python 依赖。
> 更多说明见[环境搭建文档](https://embeddedgui.readthedocs.io/en/latest/)。

### 平台支持

| 平台 | 说明 |
|------|------|
| **PC (SDL2)** | 桌面模拟器，截图输出，快速开发验证 |
| **STM32G0** | ARM Cortex-M0+ 裸机移植 |
| **QEMU** | 微秒级计时器，用于性能基准测试 |
| **WebAssembly** | Emscripten 编译，在线 Demo 直接运行 |
| **自定义移植** | 仅需实现 `draw_area` + `get_tick_ms` 两个接口 |

构建系统：**GNU Make** 与 **CMake** 双支持。

---

## 🧩 控件库（62 个）

**布局**：Group · LinearLayout · GridLayout · Scroll · ViewPage · ViewPageCache · TileView · Window · Card

**显示**：Label · DynamicLabel · Image · Divider · Line · Textblock · Spangroup

**输入**：Button · ImageButton · ButtonMatrix · Switch · Checkbox · RadioButton · ToggleButton · Slider · ArcSlider · NumberPicker · Roller · Combobox · Spinner · TextInput · Menu

**进度**：ProgressBar · CircularProgressBar · ActivityRing · PageIndicator · TabBar · Led · NotificationBadge · Scale · Gauge

**图表**：ChartLine · ChartScatter · ChartBar · ChartPie

**时间**：AnalogClock · DigitalClock · Stopwatch · MiniCalendar · Compass · HeartRate · AnimatedImage · Mp4

**列表**：List · Table

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
| `EGUI_CONFIG_FUNCTION_SUPPORT_LAYER` | 0 | Z 轴图层系统 |
| `EGUI_CONFIG_FUNCTION_SUPPORT_SCROLLBAR` | 1 | 自动滚动条指示器 |
| `EGUI_CONFIG_WIDGET_ENHANCED_DRAW` | 0 | 增强渲染（渐变 + 阴影，自动开启两者） |
| `EGUI_CONFIG_FUNCTION_GRADIENT_DITHERING` | 0 | 渐变抖动（消除 16 位色带） |

### 抗锯齿质量

| 宏 | 默认 | 说明 |
|----|------|------|
| `EGUI_CONFIG_CIRCLE_HQ_SAMPLE_2X2` | 0 | 2×2 快速采样（0 = 4×4 高质量） |
| `EGUI_CONFIG_LINE_HQ_SAMPLE_2X2` | 0 | 同上，用于线段 |

### 性能 / 内存

| 宏 | 默认 | 说明 |
|----|------|------|
| `EGUI_CONFIG_PFB_BUFFER_COUNT` | 2 | PFB 缓冲数（≥2 支持 DMA 流水线） |
| `EGUI_CONFIG_MAX_FPS` | 60 | 帧率上限 |
| `EGUI_CONFIG_DIRTY_AREA_COUNT` | 5 | 脏矩形区域槽位数 |
| `EGUI_CONFIG_PERFORMANCE_USE_FLOAT` | 0 | 有 FPU 时启用浮点加速 |
| `EGUI_CONFIG_SOFTWARE_ROTATION` | 0 | 软件旋转 PFB 输出 |

### 主题系统

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

---

## 🛠️ UI Designer

UI Designer 桌面端和设计稿转换链路已经迁移到独立仓库 `EmbeddedGUI_Designer` 维护。

```
Figma / HTML / JSX ──→ XML ──→ C 源文件 (uicode.c / .h)
```

- **当前仓库定位**：保留 SDK、运行时、资源生成、示例和文档
- **Designer 仓库定位**：维护桌面设计器、设计稿导入、XML 编辑和预览打包
- **迁移入口**：见 `doc/source/ui_designer/designer_repo_migration.md`

![UI_Designer](doc/source/images/UI_Designer.gif)

### 字体 / 图像

**字体**：资源字体（`egui_font_std`），内置 Montserrat，UTF-8 解码，支持多行对齐。

**图像格式**：RGB32（ARGB8888）、RGB565、RGB565 调色板（1/2/4/8 bit）、Alpha 遮罩（1/2/4/8 bit），全部可按需关闭以节省 ROM。

---

## 🔧 工具链

| 工具 | 命令 | 说明 |
|------|------|------|
| 构建 | `make all APP=<APP>` | 编译指定示例 |
| 运行 | `make run` | 启动 PC 模拟器 |
| 资源生成 | `make resource_refresh APP=<APP>` | 生成字体 / 图片 C 文件 |
| 运行时验证 | `python scripts/code_runtime_check.py --app <APP> [--app-sub <SUB>]` | 截图验证渲染正确性 |
| 体积分析 | `python scripts/size_analysis/utils_analysis_elf_size.py` | 生成 ROM/RAM 报告 |
| CI 编译检查 | `python scripts/code_compile_check.py --full-check` | 全量编译检查 |

---

## 📦 资源占用

| App | Code (Bytes) | Resource (Bytes) | RAM (Bytes) | PFB (Bytes) | Total ROM (Bytes) |
|-----|-------------|-----------------|------------|------------|------------------|
| HelloSimple | 20780 | 8016 | 968 | 2400 | 28796 |
| HelloActivity | 29956 | 11244 | 1560 | 2400 | 41200 |
| HelloBasic(button) | 21000 | 10592 | 1176 | 2400 | 31592 |
| HelloBasic(label) | 17368 | 6056 | 1092 | 2400 | 23424 |
| HelloBasic(list) | 22564 | 10692 | 2120 | 2400 | 33256 |
| HelloBasic(mask) | 44692 | 31324 | 872 | 2400 | 76016 |
| HelloBasic(textinput) | 30840 | 11528 | 4836 | 2400 | 42368 |
| HelloCanvas | 71076 | 25312 | 1596 | 7200 | 96388 |
| HelloChart | 45348 | 17012 | 2176 | 2400 | 62360 |
| HelloPerformance | 114624 | 303508 | 1164 | 2400 | 418132 |
| HelloStyleDemo | 118308 | 105604 | 7368 | 9600 | 223912 |

> 运行 `python scripts/size_analysis/utils_analysis_elf_size.py` 生成完整报告（含 HelloBasic 全部 62 个子应用和 HelloVirtual 全部 19 个子应用）。

---

## 📚 示例应用（17 个）

| 示例 | 说明 |
|------|------|
| `HelloSimple` | 最小 Hello World |
| `HelloActivity` | Activity 生命周期演示 |
| `HelloAPP` | 完整多页面应用 |
| `HelloBasic` | **62 个独立控件演示** |
| `HelloVirtual` | **19 个 Virtual 控件示例** |
| `HelloCanvas` | 绘图图元展示 |
| `HelloChart` | 图表控件演示 |
| `HelloGradient` | 渐变填充效果 |
| `HelloLayer` | Z 轴图层演示 |
| `HelloStyleDemo` | 主题 / 增强样式演示 |
| `HelloPerformance` | FPS / 性能基准 |
| `HelloPFB` | PFB 渲染演示 |
| `HelloResourceManager` | 外部资源管理器 |
| `HelloEasyPage` | EasyPage API 演示 |
| `HelloShowcase` | UI 全景展示 |
| `HelloUnitTest` | 单元测试 |
| `HelloViewPageAndScroll` | 翻页 + 滚动演示 |

---

## 📝 写在最后

作为芯片从业人员，国产芯片资源有限（512 KB ROM、20 KB RAM、96 MHz CPU），需要跑手环级别的触控 GUI。评估了 [GuiLite](https://github.com/idea4good/GuiLite)（无 PFB）、[Arm-2D](https://github.com/ARM-software/Arm-2D)（无控件管理）、[LVGL](https://github.com/lvgl/lvgl)（资源需求太高），各有优势但都不完全满足需求，最终决定自己写一套——什么都可控，极限优化。

现在有 AI 加持，渐变、KEY 输入、Focus 系统、Layer、UI Designer 等功能不断加入，框架越来越好用，也更方便维护。

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
