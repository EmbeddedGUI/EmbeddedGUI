---
name: figmamake-to-egui
description: Strict workflow for converting Figma Make projects to EmbeddedGUI C code with pixel-level verification
---

# Figma Make → EGUI 严格转换工作流

AI 必须严格按照以下 4 个阶段顺序执行，不得跳过任何步骤。每个阶段有明确的输入、输出和验收标准。

## 前置条件

- Figma Make URL 或本地已有 figmamake_src
- Python 3.8+、Node.js 18+、Playwright 已安装
- Material Symbols Outlined 字体: `scripts/tools/MaterialSymbolsOutlined-Regular.ttf`

## 目录约定

所有中间产物保存在 `example/{APP}/.eguiproject/` 下，便于用户 review：

```
example/{APP}/.eguiproject/
├── figmamake_src/          # Stage 1 输出: Figma Make 源码
├── reference_frames/       # Stage 1 输出: Playwright 渲染参考图
│   ├── {page}/static.png       # 静态终态
│   ├── {page}/anim_start.png   # 动效起始帧
│   ├── {page}/anim_mid.png     # 动效中间帧
│   └── {page}/anim_end.png     # 动效结束帧
├── layout/                 # Stage 2 输出: XML 布局文件
├── resources/              # Stage 2 输出: 导出的图标/图片
├── rendered_frames/        # Stage 3 输出: EGUI 渲染截图
│   └── {page}/frame_NNNN.png
└── comparison/             # Stage 4 输出: 对比报告
    ├── {page}_compare.png      # 并排对比图
    └── regression_report.html  # SSIM 回归报告
```

---

## Stage 1: CAPTURE — 拉取源码 + 本地渲染参考图

### 1.1 拉取 Figma Make 源码

如果用户提供了 Figma Make URL：
```bash
# 通过 Figma MCP 获取项目源码，保存到 .eguiproject/figmamake_src/
# 或者用户已手动拷贝源码到该目录
```

如果 `.eguiproject/figmamake_src/` 已存在，跳过拉取。

**验收**: `figmamake_src/src/app/` 目录存在，包含 TSX 组件文件。

### 1.2 本地 Playwright 渲染参考图

服务器不支持渲染截图，必须在本地执行：

```bash
cd example/{APP}/.eguiproject/figmamake_src
npm install
npm run dev &   # 用户手动启动 dev server

# AI 等待用户确认 dev server 已启动后，执行截图
python scripts/figmamake/figmamake_capture.py \
    --url "http://localhost:5173" \
    --output-dir example/{APP}/.eguiproject/reference_frames/ \
    --width {WIDTH} --height {HEIGHT}
```

需要截取的内容（每个页面）：
1. **静态终态图** — 等待所有动画完成后 500ms 截图
2. **动效三帧** — 动画 0%（起始）、50%（中间）、100%（结束）

**验收**: 每个页面目录下至少有 `static.png`，动效页面还有 `anim_*.png`。

### 1.3 如果 Playwright 不可用

AI 可以手动分析 TSX 源码，结合 Figma MCP 截图作为参考：
```
MCP call: figma__get_screenshot(fileKey, nodeId)
```
将截图保存到 `reference_frames/{page}/static.png`。

---

## Stage 2: CONVERT — 源码分析 + 转换为 EGUI

### 2.1 分析 Figma Make 源码

AI 必须逐个读取 TSX 组件文件，提取：

| 提取项 | 来源 | EGUI 映射 |
|--------|------|-----------|
| 组件树结构 | JSX 嵌套 | Group/Label/Image 层级 |
| 布局属性 | Tailwind class | x/y/width/height |
| 文本内容 | JSX 文本节点 | Label text |
| 字体样式 | Tailwind font class | font_builtin 引用 |
| 颜色值 | Tailwind/CSS 变量 | EGUI_COLOR_HEX() |
| SVG 图标 | lucide-react 等 | 导出为 PNG → alpha 格式图片资源 |
| 字体符号图标 | Material Symbols | 转为文本 Label（使用对应字体） |
| framer-motion 动效 | motion.* / animate | egui_animation_* |
| 交互行为 | onClick/navigate | on_click_listener 回调 |

### 2.2 图标资源处理

**SVG 图标（lucide-react 等）**:
1. 从 node_modules 或在线获取 SVG
2. 转换为目标尺寸 PNG（推荐 14x14 或 16x16）
3. 保存到 `resource/src/icon_{name}.png`
4. 在 `app_resource_config.json` 中注册为 alpha 格式

**字体符号图标（Material Symbols）**:
1. 查找对应 Unicode 码点
2. 用 `scripts/tools/generate_icon_png.py` 生成 PNG
3. 或直接用 Label + Material Symbols 字体渲染

### 2.3 生成 XML 布局

每个页面生成一个 XML 文件到 `.eguiproject/layout/{page}.xml`。

XML 规则（参考 figma-mcp-to-egui skill）：
- Root 是 `<Page>`，直接子元素是 `<Group id="root">`
- 所有控件必须有 `id`, `x`, `y`, `width`, `height`
- `id` 必须是唯一合法 C 标识符
- Alpha 图片必须设置 `image_color` 和 `image_color_alpha`
- 图片尺寸≠资源尺寸时设置 `image_resize="true"`

### 2.4 生成 C 代码

```bash
python scripts/html2egui_helper.py generate-code --app {APP}
python scripts/html2egui_helper.py gen-resource --app {APP}
```

或 AI 直接手写 C 代码（当自动工具不满足需求时）。

### 2.5 动效映射

| Figma Make (framer-motion) | EGUI 动画类型 | 状态 |
|---|---|---|
| `opacity: 0→1` | `egui_animation_alpha_t` | ✅ |
| `x` / `y` translate | `egui_animation_translate_t` | ✅ |
| `scale` | `egui_animation_scale_size_t` | ✅ |
| `spring` transition | overshoot 插值器 | ✅ |
| `stagger delay` | 多动画 + 递增 delay | ✅ |
| `width`/`height` grow | `egui_animation_resize_t` | ✅ |
| `color` transition | `egui_animation_color_t` | ✅ |

遇到不支持的动效 → 生成扩展提案到 `extension_proposals/`，暂停并通知用户。

**验收**: 所有页面的 `*_layout.c`、`*.h`、`*.c` 文件已生成，资源文件已就位。

---

## Stage 3: BUILD & RUN — 编译运行截帧

### 3.1 编译

```bash
make clean APP={APP}
make all APP={APP} PORT=pc
```

必须 0 error。Warning 可接受但应记录。

### 3.2 运行截帧

```bash
python scripts/code_runtime_check.py --app {APP} --keep-screenshots
```

或手动运行模拟器截帧。

截图保存到 `.eguiproject/rendered_frames/{page}/`。

### 3.3 AI 视觉检查

AI 必须用 Read 工具读取每个页面的渲染截图，确认：
- 不是黑屏/空白
- 控件可见且位置合理
- 文字可读
- 图标正确显示

**验收**: 编译通过 + 运行不崩溃 + 截图非空白。

---

## Stage 4: VERIFY — 像素级对比验证

### 4.1 生成对比图

对每个页面，将参考图和渲染图并排对比：

```bash
python scripts/figmamake/figma_visual_compare.py \
    --design example/{APP}/.eguiproject/reference_frames/{page}/static.png \
    --rendered example/{APP}/.eguiproject/rendered_frames/{page}/frame_final.png \
    --output example/{APP}/.eguiproject/comparison/{page}_compare.png
```

### 4.2 SSIM 回归验证

```bash
python scripts/figmamake/figmamake_regression.py \
    --reference-dir example/{APP}/.eguiproject/reference_frames/ \
    --rendered-dir example/{APP}/.eguiproject/rendered_frames/ \
    --output example/{APP}/.eguiproject/comparison/regression_report.html
```

### 4.3 通过标准

| 类型 | SSIM 阈值 | 判定 |
|------|-----------|------|
| 静态终态 | ≥ 0.85 | PASS |
| 动画关键帧 | ≥ 0.70 | PASS |
| 任何页面 | < 0.60 | FAIL — 必须修复 |

### 4.4 AI 视觉对比

AI 必须用 Read 工具读取对比图，逐页检查：
1. 读取 `comparison/{page}_compare.png`（三栏：设计稿 | 渲染 | 差异）
2. 差异图中红色/亮色区域 = 不匹配区域
3. 定位对应控件，修复 XML/C 代码
4. 重新执行 Stage 3-4 直到通过

### 4.5 迭代修复循环

```
WHILE 任何页面 SSIM < 0.85:
    1. 读取对比图，识别差异区域
    2. 定位对应的 XML/C 代码
    3. 修复（位置/尺寸/颜色/缺失控件）
    4. 重新编译运行截帧 (Stage 3)
    5. 重新对比 (Stage 4.1-4.3)
```

**验收**: 所有页面 SSIM ≥ 0.85，对比图和报告已保存到 `.eguiproject/comparison/`。

---

## 交付清单

AI 完成转换后，必须确认以下文件存在并通知用户：

| 文件 | 说明 |
|------|------|
| `.eguiproject/figmamake_src/` | Figma Make 原始源码 |
| `.eguiproject/reference_frames/` | Playwright/Figma 参考渲染图 |
| `.eguiproject/rendered_frames/` | EGUI 渲染截图 |
| `.eguiproject/comparison/` | 对比图 + 回归报告 |
| `example/{APP}/*_layout.c` | 生成的布局代码 |
| `example/{APP}/*.h` | 页面头文件 |
| `example/{APP}/*.c` | 页面逻辑代码 |
| `example/{APP}/resource/` | 图片/字体资源 |

## 核心原则

1. **不跳过验证** — 编译通过不等于完成，必须走完 Stage 4
2. **保留所有中间产物** — 参考图、渲染图、对比图全部保存在 `.eguiproject/`
3. **以转换驱动框架演进** — 遇到不支持的特效，设计新 EGUI 能力而非降级
4. **像素级追求** — SSIM < 0.85 不交付，持续迭代修复
