---
name: github-pages-web
description: Use when adding new demo apps to the GitHub Pages site, creating new page types, or modifying the web layout
---

# GitHub Pages Web Publishing Skill

EmbeddedGUI 的在线 Demo 站点通过 GitHub Pages 发布，源码在 `web/` 目录，由 CI 自动构建部署。

## 架构概览

```
web/
├── index.html          # 首页，聚焦 HelloShowcase，并提供到 basic/examples/Widgets 仓库的入口
├── basic.html          # 侧边栏页（HelloBasic 所有子 widget demo）
├── examples.html       # 侧边栏页（除 HelloBasic/HelloShowcase 外的 Standalone demo）
├── style.css           # 共享样式（含 CSS 变量布局令牌）
├── i18n.js             # 语言切换辅助
├── doc-render.js       # Markdown → HTML 渲染工具
├── lib/marked.min.js   # Markdown 解析库（vendored，无外部依赖）
└── demos/
    ├── demos.json      # 所有 demo 的注册表（三个页面共同读取）
    ├── HelloSimple/    # 每个 demo 一个目录
    │   ├── HelloSimple.html    # WASM 宿主页（Emscripten 生成）
    │   ├── HelloSimple.js
    │   ├── HelloSimple.wasm
    │   └── README.md           # demo 文档（可选）
    └── HelloBasic_button/      # HelloBasic 子 demo 命名格式
```

**数据驱动**：页面运行时 fetch `demos/demos.json`，按 `category` 和 `name` 过滤渲染内容。**不要手动维护 HTML 里的 demo 列表**；构建脚本会维护注册表。

---

## demos.json 结构

每个 demo 条目字段：

```json
{
  "name": "HelloSimple",       // 目录名 / URL fragment 标识符
  "app": "HelloSimple",        // Emscripten 编译产物的 APP 名（.html/.js/.wasm 文件前缀）
  "category": "Standalone",   // "Standalone" | "HelloBasic"
  "width": 240,                // 从 app_egui_config.h 读取，缺省 240
  "height": 320,               // 从 app_egui_config.h 读取，缺省 320
  "doc": "demos/HelloSimple/README.md"  // 可选，存在时渲染文档面板
}
```

- `category = "Standalone"` → 出现在 `examples.html`；`HelloShowcase` 作为首页主 demo 处理
- `category = "HelloBasic"` → 出现在 `basic.html` 侧边栏列表
- HelloBasic 子 demo 的 `name` = `HelloBasic_{sub}`，`app` = `"HelloBasic"`，并带 `appSub`

---

## 新增一个独立 Demo（Standalone）

### 步骤 1：创建示例应用
```
example/MyNewApp/
├── main.c / uicode_disp0.c / uicode_disp0.h
├── build.mk
├── app_egui_config.h
└── readme.md              # 会自动发布为文档面板内容
```

### 步骤 2：本地测试构建
```bash
# PC 构建验证
make all APP=MyNewApp

# WASM 单独构建（需要 Emscripten 环境）
python scripts/web/wasm_build_demos.py --app MyNewApp
```

`wasm_build_demos.py` 会自动：
- 调用 Emscripten 编译
- 把产物复制到 `web/demos/MyNewApp/`
- 把 `readme.md` 复制为 `web/demos/MyNewApp/README.md`
- 在 `web/demos/demos.json` 追加条目

### 步骤 3：提交 PR / 推送 main
GitHub Actions (`wasm-deploy.yml`) 在 push 到 main 时自动全量构建并部署到 GitHub Pages。

---

## 新增一个 HelloBasic 子 demo

### 步骤 1：创建子应用目录
```
example/HelloBasic/my_widget/
├── test.c
├── app_egui_config.h          # 如需覆盖默认屏幕/PFB 配置
└── readme.md
```

并在 `example/HelloBasic/build.mk` 中注册。

### 步骤 2：本地 WASM 构建
```bash
python scripts/web/wasm_build_demos.py --app HelloBasic_my_widget
# 等价于：
python scripts/web/wasm_build_demos.py --app-sub my_widget
```

产物目录为 `web/demos/HelloBasic_my_widget/`，条目自动写入 demos.json。

---

## 新增一个全新的 Web 页面

当一组 demo 既不适合首页、`basic.html`，也不适合 `examples.html` 时，可以新建页面类型。

### 推荐方式：复用 CSS class

`style.css` 提供两种已有布局 class，直接套用：

| class | 适用场景 | 关键特性 |
|-------|---------|---------|
| `page-grid` | 首页/卡片式内容 | 内容居中，max-width 限制 |
| `page-sidebar` | 左侧导航 + 右侧 demo/文档内容 | sidebar 固定宽，内容区 max-width 居中 |

新页面 HTML 最小模板（以侧边栏页为例）：

```html
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="utf-8">
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <title>EmbeddedGUI - My Page</title>
    <link rel="stylesheet" href="style.css">
</head>
<body class="page-sidebar">
    <div class="sidebar-shell">
        <aside id="sidebar">
            <div class="sidebar-header">
                <h1>My Section</h1>
            </div>
            <nav id="demo-list"></nav>
        </aside>
        <main id="content">
            <div id="demo-wrapper" style="display:none">
                <iframe id="demo-frame"></iframe>
                <div id="demo-doc" class="doc-panel"></div>
            </div>
        </main>
    </div>
    <script src="lib/marked.min.js"></script>
    <script src="doc-render.js"></script>
    <script>
        // fetch demos.json, filter by your category, render list
    </script>
</body>
</html>
```

如果需要入口链接，优先复用现有 `.hero-links` / `.inline-links` / `.button-link` 样式，在 `index.html`、`basic.html` 或 `examples.html` 的导航区域补充。

### demos.json 新增 category

若使用自定义 category（如 `"Charts"`），在新页面 JS 里按 category 过滤即可：

```js
demos.forEach(function(d) {
    if (d.category !== 'Charts') return;
    // render...
});
```

---

## 响应式布局令牌

所有尺寸通过 `:root` CSS 变量统一控制，新页面继承后无需重新写媒体查询：

```css
:root {
    --content-max-width: 1200px;   /* 内容区最大宽度 */
    --sidebar-width: 320px;        /* 大屏侧边栏宽度 */
    --doc-line-width: 72ch;        /* 文档面板最大行宽 */
}
```

**三档断点**：

| 档位 | 范围 | 主要行为 |
|------|------|---------|
| 桌面（默认） | ≥ 1200px | 内容居中，两侧留白，文档行宽 72ch |
| 平板 | 769–1199px | sidebar/内容区按 `style.css` 媒体查询收窄 |
| 移动 | ≤ 768px | sidebar 与内容区垂直堆叠 |

---

## WASM Demo 注意事项

### 录制功能：已在 WASM 中强制禁用

`porting/emscripten/build.mk` 中固定了：
```makefile
COMMON_FLAGS += -DEGUI_CONFIG_FUNCTION_RECORDING_TEST=0
```

**WASM demo 不会自动播放操作序列**，用户直接用鼠标/触摸与 canvas 交互。
- `egui_port_get_recording_action()` 函数在 uicode.c 中可以存在（由 `#if EGUI_CONFIG_FUNCTION_RECORDING_TEST` 包裹），**不影响 WASM 构建**
- 无需为 WASM 单独实现交互逻辑，SDL 输入事件由 Emscripten 自动转发

### Canvas 尺寸

Demo shell (`porting/emscripten/shell.html`) 中：
```css
#canvas {
    max-width: 100%;
    max-height: calc(100vh - 24px);
}
```
Canvas 会自动按比例缩放，不需要额外处理。

### 资源文件

有 `resource/` 目录的应用，构建脚本会自动通过 `--preload-file` 把 `.bin` 预加载进 Emscripten 虚拟文件系统，app 代码无需修改。

---

## CI/CD 流程

```
push → main
  └─ wasm-deploy.yml
       ├─ python scripts/web/wasm_build_demos.py  （全量构建）
       └─ actions/deploy-pages → GitHub Pages  （部署 web/ 目录）
```

本地预览：
```bash
cd web
python -m http.server 8080
# 浏览器访问 http://localhost:8080
```
