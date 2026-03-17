# AI 辅助设计转换工作流

## 概述

EmbeddedGUI 项目深度集成了 AI 辅助开发能力，实现了从设计稿到嵌入式 C 代码的全自动转换。AI 代理（如 Claude Code）通过项目内置的脚本工具链、转换管道和验证框架，将 Figma 设计稿转换为可运行在资源受限 MCU 上的 GUI 代码。

核心理念：**以转换驱动框架演进** —— 遇到不支持的特效或控件时，扩展框架能力，而非降级或跳过。

### 为什么需要 AI 辅助

嵌入式 GUI 开发面临独特挑战：

- 设计稿使用 HTML/CSS/React，目标是纯 C 代码
- 嵌入式平台无浮点、无动态内存、只有定点运算
- 动画效果需要从 Framer Motion 语义映射到定时器 + 手动插值
- 布局需要绝对坐标，无 CSS flexbox/grid 引擎

AI 代理弥合了这些差距：解析设计稿语义 → 生成 XML 布局 → 调用脚本生成 C 代码 → 手写动态特效 → 自动验证。

## 转换管道架构

```
            设计稿输入
            ┌─────────────────────────────┐
            │  Figma Make (TSX/React)     │
            │  Stitch HTML (Tailwind CSS) │
            │  Figma 设计文件 (REST/MCP)   │
            └────────────┬────────────────┘
                         │
                ┌────────▼────────┐
                │  语义解析阶段     │  html2egui_helper.py extract-layout
                │  figmamake_parser │  figmamake-extract
                └────────┬────────┘
                         │ layout_description.json
                ┌────────▼────────┐
                │  项目搭建阶段     │  html2egui_helper.py scaffold
                │  (创建 .egui 项目) │
                └────────┬────────┘
                         │
                ┌────────▼────────┐
                │  XML 布局编写     │  AI 根据 JSON 分析编写 XML
                │  + 图标资源导出    │  export-icons / export-svgs
                └────────┬────────┘
                         │
                ┌────────▼────────┐
                │  C 代码生成      │  html2egui_helper.py generate-code
                │  + 资源生成       │  html2egui_helper.py gen-resource
                └────────┬────────┘
                         │
                ┌────────▼────────┐
                │  动态特效实现     │  AI 阅读 TSX 源码，手写定时器/动画
                └────────┬────────┘
                         │
                ┌────────▼────────┐
                │  构建 + 验证      │  html2egui_helper.py verify
                │                  │  code_runtime_check.py
                └──────────────────┘
```

## 脚本工具链

### 核心转换工具：html2egui_helper.py

统一入口脚本，提供 12 个子命令覆盖完整转换流程：

| 子命令 | 功能 | 典型用法 |
|--------|------|---------|
| `scaffold` | 创建 `.egui` 项目骨架 | `--app MyApp --width 320 --height 480 --pages main,settings` |
| `extract-layout` | 解析 Stitch HTML 为 JSON | `--input design.html` |
| `figmamake-extract` | 解析 Figma Make TSX 项目 | `--input figmamake_src/ --list-pages` |
| `export-icons` | 导出 Material/Lucide 图标为 PNG | `--input design.html --app MyApp --size 24 --auto-color` |
| `export-svgs` | 提取内联 SVG 转 PNG | `--input design.html --app MyApp --size 64` |
| `extract-text` | 提取文本字符集（用于字体生成） | `--input design.html --app MyApp` |
| `generate-code` | XML → C 代码生成 | `--app MyApp` |
| `gen-resource` | 生成 C 资源数组（图片/字体） | `--app MyApp` |
| `verify` | 构建 + 运行时验证一步完成 | `--app MyApp --bits64` |
| `figma2xml` | Figma REST API → XML 布局 | `--url "figma://..." --app MyApp` |
| `figma-mcp` | Figma MCP JSON → XML 布局 | `--input nodes.json --app MyApp` |

### Figma Make 转换工具链

位于 `scripts/figmamake/`，提供 Figma Make 项目的端到端转换：

| 脚本 | 功能 |
|------|------|
| `figmamake2egui.py` | 统一入口，串联 4 阶段管道 |
| `figmamake_parser.py` | TSX 语义解析，提取布局和颜色 |
| `figmamake_anim_extractor.py` | 提取 Framer Motion 动效定义 |
| `figmamake_codegen.py` | 生成动效相关的 C 代码片段 |
| `figmamake_capture.py` | Playwright 截取参考帧序列 |
| `figmamake_regression.py` | SSIM 像素级回归验证 |
| `figma_design_render.py` | Figma 设计稿渲染对比 |
| `figma_visual_compare.py` | 设计稿 vs 实际渲染视觉对比 |

### 自动化验证工具

| 脚本 | 功能 |
|------|------|
| `scripts/code_compile_check.py` | 全量编译检查（支持 Make/CMake，CI 集成；`--full-check` 含示例 icon font 检查） |
| `scripts/code_runtime_check.py` | 运行时验证（启动程序、截图、超时检测） |
| `scripts/code_format.py` | clang-format 代码格式化 |
| `scripts/check_example_icon_font.py` | 示例图标字体显式配置检查 |
| `scripts/release_check.py` | 一键发布验证（多步骤发布前流水线） |

## 三条输入路径

### 路径一：Stitch HTML

Stitch 生成的 HTML 使用 Tailwind CSS 描述布局：

```bash
# 1. 解析 HTML 布局
python scripts/html2egui_helper.py extract-layout --input design.html

# 2. 创建项目
python scripts/html2egui_helper.py scaffold --app MyApp --width 320 --height 480

# 3. 导出图标
python scripts/html2egui_helper.py export-icons --input design.html --app MyApp --auto-color

# 4. AI 编写 XML 布局（参考 JSON 分析结果）

# 5. 生成 C 代码 + 资源
python scripts/html2egui_helper.py generate-code --app MyApp
python scripts/html2egui_helper.py gen-resource --app MyApp

# 6. 验证
python scripts/html2egui_helper.py verify --app MyApp
```

### 路径二：Figma Make（TSX/React 项目）

Figma Make 生成完整的 React + Tailwind + Framer Motion 项目，包含动画定义：

```bash
# 全自动管道（串联 4 阶段）
python scripts/figmamake/figmamake2egui.py \
    --figma-url "https://www.figma.com/make/{fileKey}/..." \
    --app HelloBattery --width 320 --height 240

# 或分步执行：
# Stage 1: 采集参考帧
python scripts/figmamake/figmamake_capture.py --app HelloBattery

# Stage 2: 解析 TSX
python scripts/html2egui_helper.py figmamake-extract \
    --input example/HelloBattery/.eguiproject/figmamake_src/

# Stage 3: 生成代码 + 构建
python scripts/html2egui_helper.py generate-code --app HelloBattery
python scripts/html2egui_helper.py verify --app HelloBattery

# Stage 4: 回归验证
python scripts/figmamake/figmamake_regression.py --app HelloBattery
```

### 路径三：Figma 设计文件（REST API / MCP）

直接从 Figma 设计文件提取布局，支持 Auto Layout 映射：

```bash
# 通过 REST API
python scripts/html2egui_helper.py figma2xml \
    --url "https://www.figma.com/design/{fileKey}?node-id={nodeId}" \
    --app MyApp --token $FIGMA_TOKEN

# 通过 Figma MCP（无需 token，AI 代理直接调用）
python scripts/html2egui_helper.py figma-mcp \
    --input figma_nodes.json --app MyApp --pages main,settings
```

Auto Layout 映射规则：

| Figma 属性 | EGUI 映射 |
|------------|-----------|
| `layoutMode: VERTICAL` | `LinearLayout orientation="vertical"` |
| `layoutMode: HORIZONTAL` | `LinearLayout orientation="horizontal"` |
| `layoutWrap: WRAP` | `GridLayout` |
| `fills[0].color` | `Background color` |
| `cornerRadius` | `Background corner_radius` |

## 动态特效还原

静态布局转换完成后，AI 代理为每个页面补充动态特效。动效实现以 Figma Make TSX 源文件为准，不自行发挥。

### TSX → EGUI 动效映射

| TSX (React/Framer Motion) | EGUI 实现 | API |
|---------------------------|-----------|-----|
| `motion.div initial={{ opacity: 0 }} animate={{ opacity: 1 }}` | Alpha 动画 | `egui_animation_alpha_t` |
| `initial={{ y: -8 }} animate={{ y: 0 }}` | 位移动画 | `egui_animation_translate_t` |
| `initial={{ scale: 0.95 }} animate={{ scale: 1 }}` | 缩放动画 | `egui_animation_scale_size_t` |
| `initial={{ width: 0 }} animate={{ width: "80%" }}` | 尺寸动画 | `egui_animation_resize_t` |
| `transition: { ease: "easeOut" }` | 减速插值器 | decelerate easing |
| `transition: { type: "spring" }` | 弹簧插值器 | `egui_interpolator_overshoot_t` |
| `transition: { delay: i * 0.05 }` | 级联延迟 | 定时器回调中按 ID 延迟 |
| `setInterval(fn, 1000)` | 周期定时器 | `egui_timer_start_timer(&t, 1000, 1)` |
| `animate-pulse` | 呼吸灯效果 | `egui_timer_t` 周期 alpha 切换 |
| `AnimatePresence` | 页面切换动画 | 组合 alpha + translate |

### 实现模式

动态特效通常在用户文件（`{page}.c`）中实现，使用定时器驱动：

```c
// 1. 声明定时器
static egui_timer_t growth_timer;
static int anim_tick = 0;

// 2. 定时器回调：每帧更新控件状态
static void on_growth_timer(egui_timer_t *timer)
{
    anim_tick++;
    int progress = (anim_tick > 100) ? 100 : anim_tick;

    // 更新进度条
    egui_view_progress_bar_set_process(
        EGUI_VIEW_OF(&local->battery_bar), progress);

    // 更新数字标签
    char buf[8];
    snprintf(buf, sizeof(buf), "%d%%", progress);
    egui_view_label_set_text(EGUI_VIEW_OF(&local->percent_label), buf);
}

// 3. 页面进入时启动动画
void page_on_entry(void)
{
    anim_tick = 0;
    egui_timer_start_timer(&growth_timer, 16, 1); // 60fps
}
```

### 参考实现

- `example/HelloStyleDemo/uicode_dashboard.c` —— 完整的多控件动效（KPI 增长、图表数据、主题切换）
- `example/HelloBattery/dashboard.c` —— growth/pulse/clock 三定时器模式
- `example/HelloBattery/cell_details.c` —— 级联延迟（stagger）动画
- `example/HelloBattery/settings.c` —— Toggle 开关交互

## 自动化验证体系

### 运行时验证

每次代码变更后必须验证程序不会卡死、崩溃，且渲染正确：

```bash
# 验证单个示例
python scripts/code_runtime_check.py --app HelloBasic --app-sub button --timeout 10

# 验证所有示例
python scripts/code_runtime_check.py --full-check

# 多页面应用会自动通过 recording action 切换所有页面
```

验证流程：
1. 编译目标应用（PC 模拟器）
2. 启动程序，执行预设的 recording action 序列
3. 在关键时间点截取屏幕截图
4. 超时检测（防止卡死）
5. 截图存储到 `runtime_check_output/` 供人工审查

### 像素级回归验证

针对 Figma Make 项目，使用 SSIM（结构相似性）进行像素级回归：

```bash
python scripts/figmamake/figmamake_regression.py \
    --app HelloBattery \
    --reference-dir .eguiproject/reference_frames/
```

通过标准：

| 帧类型 | SSIM 阈值 | 判定 |
|--------|----------|------|
| 静态终态 | >= 0.85 | PASS |
| 动画关键帧 | >= 0.70 | PASS |
| 任何页面 | < 0.60 | FAIL |

### 发布验证

`scripts/release_check.py` 提供一键式发布前检查，串联完整的发布前流水线：

```bash
python scripts/release_check.py
```

| 步骤 | 说明 |
|------|------|
| 1. 代码格式化检查 | clang-format 验证 |
| 2. 示例图标字体检查 | 检查示例是否显式设置 `icon_font` |
| 3. Keil 工程同步检查 | 校验源码与 `.uvprojx` 配置一致 |
| 4. UI Designer 单元测试 | pytest 运行 UI Designer 测试 |
| 5. UI Designer 打包 | PyInstaller 构建桌面工具 |
| 6. 全量编译 | 编译所有示例应用 |
| 7. WASM Demo 构建 | 构建 WebAssembly 演示站点 |
| 8. 运行时截图验证 | 启动应用并截图检查渲染结果 |
| 9. 二进制大小分析 | ELF 内存占用分析 |
| 10. 大小文档生成 | 生成体积报告文档 |
| 11. QEMU 性能回归 | 微秒级性能基准测试 |
| 12. 性能文档生成 | 生成性能报告 |
| 13. Sphinx 文档构建 | 验证文档可正常生成 |

常见用法：

```bash
python scripts/release_check.py --keep-going
python scripts/release_check.py --skip perf,perf_doc,wasm,doc,ui_package
python scripts/release_check.py --cmake --skip runtime
```

## 框架扩展驱动机制

转换过程中遇到不支持的控件或特效时，AI 代理按以下流程扩展框架：

```
解析器标记 NEEDS_EXTENSION
    │
    ▼
确认 C 层 API（src/widget/egui_view_*.h）
    │
    ▼
编写 Widget 注册插件（custom_widgets/*.py）
    │
    ▼
重新生成代码 → 验证
```

### Widget 注册系统

每个控件在 `scripts/ui_designer/custom_widgets/` 下有一个注册文件，定义 C 层映射：

```python
WidgetRegistry.instance().register(
    type_name="progress_bar",
    descriptor={
        "c_type": "egui_view_progress_bar_t",
        "init_func": "egui_view_progress_bar_init_with_params",
        "params_macro": "EGUI_VIEW_PROGRESS_BAR_PARAMS_INIT",
        "is_container": False,
        "properties": {
            "value": {
                "type": "int", "default": 50,
                "code_gen": {
                    "kind": "setter",
                    "func": "egui_view_progress_bar_set_process"
                },
            },
        },
    },
    xml_tag="ProgressBar",
)
```

目前已注册 50+ 控件，涵盖基础控件、布局容器、图表、时钟、仪表等类型。详见 [扩展新控件](extend_widget.md)。

### 代码生成规则（code_gen kind）

| Kind | 说明 | 生成代码示例 |
|------|------|-------------|
| `setter` | 单参数设置 | `func(view, value)` |
| `text_setter` | 设置静态字符串 | `func(view, "text")` |
| `multi_setter` | 多参数调用 | `func(view, arg1, arg2)` |
| `derived_setter` | 从资源派生 | `func(view, &resource)` |
| `image_setter` | 设置图片资源 | `func(view, &image_res)` |
| `mapped_setter` | 枚举值映射 | `func(view, ENUM_VALUE)` |

## AI 代理的 Skill 系统

项目在 `.claude/skills/` 下维护了一组结构化技能文件，指导 AI 代理执行特定任务：

| Skill 文件 | 用途 |
|------------|------|
| `html-to-egui.md` | Stitch HTML → EGUI 全流程指导 |
| `figmamake-to-egui.md` | Figma Make → EGUI 全流程指导 |
| `figma-mcp-to-egui.md` | Figma MCP → EGUI 布局转换 |
| `dynamic-effects.md` | 动态特效还原规则和参考实现 |
| `resource-generation.md` | 资源生成（图片/字体 → C 数组） |
| `runtime-verification.md` | 运行时验证流程 |
| `recording-simulation.md` | Recording 录制动作配置 |
| `performance-analysis.md` | 性能分析与优化 |
| `build-and-debug.md` | 构建和调试指南 |
| `github-pages-web.md` | WASM Demo 站点管理 |

这些 skill 文件确保 AI 代理在执行转换时遵循一致的流程和最佳实践。

## 典型转换案例

### 案例：HelloStyleDemo（4 页面风格展示）

这个案例展示了从 Figma Make 设计到完整嵌入式 GUI 的转换过程：

**项目规格：**
- 分辨率：320 × 240
- 页面：Music、Dashboard、Watch、Smart Home
- 特效：旋转弧线、数据增长动画、级联渐入、主题切换

**转换步骤：**

1. **语义解析**：从 TSX 提取 4 个页面的布局结构、颜色方案、图标列表
2. **项目搭建**：创建 `.egui` 项目，配置 4 页面 ViewPage 架构
3. **XML 布局**：AI 将 JSON 分析结果转换为 XML，处理坐标映射和控件选择
4. **代码生成**：`generate-code` 生成 C 布局代码和页面管理框架
5. **资源生成**：图标 PNG → C 数组，字体 TTF → 位图数组
6. **动态特效**：
   - Music 页：旋转弧线 + 圆形专辑封面（定时器 + 弧线角度递增）
   - Dashboard 页：KPI 数值增长 + 进度条动画 + 图表数据动画
   - Watch 页：秒针旋转 + 时间更新
   - Smart Home 页：卡片级联渐入 + 开关交互
7. **页面切换**：ViewPage 回调触发入场动画
8. **验证**：运行时截图验证所有 4 个页面渲染正确

### 案例：HelloBattery（电池管理系统）

**项目规格：**
- 分辨率：320 × 240
- 页面：Boot、Dashboard、CellDetails、TempMonitor、Settings
- 特效：逐行渐入 Boot 序列、SOC 进度条增长、级联电压条、折线图绘制

**关键技术点：**
- Boot 序列使用状态机 + 定时器逐步显示
- 级联延迟（stagger）通过按 ID 延迟帧数实现
- 折线图绘制动画通过逐点增加数据点实现
- 弹簧开关动画使用 overshoot 插值器

## 环境配置

运行转换工具链需要以下环境：

```bash
# 运行 setup.bat 选择完整模式（选项 2）
setup.bat

# 安装内容：
# 1. make.exe + gcc.exe（可自动下载 w64devkit）
# 2. Python 3.8+
# 3. Python 依赖：freetype_py, json5, numpy, Pillow, pyelftools
# 4. UI Designer 依赖（可选）：PyQt5, PyQt-Fluent-Widgets
```

Figma Make 转换额外需要：
- Node.js（Figma Make 项目的 `npm install`/`npm run dev`）
- Playwright（参考帧截取）
- scikit-image（SSIM 回归验证）
