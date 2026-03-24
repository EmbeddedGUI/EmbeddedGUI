# UI Designer 概述与架构

## 简介

EmbeddedGUI UI Designer 是一个可视化 UI 设计工具，用于为 EmbeddedGUI 框架创建嵌入式 GUI 界面。它提供了从设计稿到 C 代码的完整转换管道，支持两种输入路径：

- 可视化编辑器（PyQt5 桌面应用）：拖拽式 UI 设计，实时预览
- 设计稿转换管道（HTML/JSX -> XML -> C）：从 Stitch HTML 或 Figma Make 项目自动生成

最终输出为可直接编译运行的 EGUI C 代码。

## 整体架构

```
                    +-------------------+
                    |   输入源           |
                    +-------------------+
                    |                   |
            +-------v-------+   +------v--------+
            | PyQt5 可视化   |   | HTML/JSX 设计稿 |
            | 编辑器         |   | (Stitch/Figma) |
            +-------+-------+   +------+---------+
                    |                   |
                    v                   v
            +-------+-------+   +------+---------+
            | XML Layout    |<--| html2egui      |
            | (.xml 文件)    |   | _helper.py     |
            +-------+-------+   +----------------+
                    |
                    v
            +-------+-------+
            | Widget        |
            | Registry      |
            | (控件注册中心)  |
            +-------+-------+
                    |
            +-------v-------+
            | Code Generator|
            | (C 代码生成器)  |
            +-------+-------+
                    |
            +-------v-------+
            | Resource      |
            | Generator     |
            | (资源生成器)    |
            +-------+-------+
                    |
                    v
            +-------+-------+
            | C 源文件       |
            | (.h / .c)     |
            +---------------+
```

## 核心模块

UI Designer 的源码位于 `scripts/ui_designer/`，包含以下子模块：

### 目录结构

```
scripts/ui_designer/
├── main.py                    # 入口文件（PyQt5 桌面应用）
├── model/                     # 数据模型层
│   ├── widget_registry.py     # Widget 注册中心（单例）
│   ├── widget_model.py        # 控件模型（WidgetModel, BackgroundModel, AnimationModel）
│   ├── page.py                # 页面模型（Page）
│   ├── project.py             # 项目模型（Project）
│   ├── config.py              # 配置管理
│   ├── resource_catalog.py    # 资源目录
│   ├── string_resource.py     # 字符串资源（i18n）
│   └── undo_manager.py        # 撤销/重做管理
├── custom_widgets/            # Widget 注册插件（每控件一个 .py）
│   ├── label.py
│   ├── button.py
│   ├── image.py
│   ├── linearlayout.py
│   ├── progress_bar.py
│   └── ... (50+ 控件)
├── generator/                 # 代码生成器
│   ├── code_generator.py      # C 代码生成（核心）
│   ├── resource_config_generator.py  # 资源配置生成
│   ├── string_resource_generator.py  # 字符串资源生成
│   └── user_code_preserver.py # 用户代码保留（USER CODE 区域）
├── engine/                    # 引擎层
│   ├── compiler.py            # 编译引擎（增量编译 + 双缓冲预览）
│   ├── designer_bridge.py     # 设计器与 exe 进程的通信桥
│   ├── layout_engine.py       # Python 端布局引擎（复刻 C 端算法）
│   ├── auto_fixer.py          # 自动修复引擎（标签高度/容器溢出/ID冲突）
│   └── python_renderer.py     # Python 端渲染器
├── ui/                        # PyQt5 界面层
│   ├── main_window.py         # 主窗口
│   ├── property_panel.py      # 属性面板
│   ├── widget_tree.py         # 控件树
│   ├── preview_panel.py       # 实时预览面板
│   ├── editor_tabs.py         # XML 编辑器标签页
│   └── ...
├── importers/                 # 外部格式导入器
│   ├── sketch_importer.py     # Sketch 导入
│   └── xd_importer.py         # Adobe XD 导入
├── utils/                     # 工具函数
│   └── header_parser.py       # C 头文件解析
└── tests/                     # 单元测试
```

### model/ -- 数据模型层

数据模型层是整个系统的核心，定义了控件、页面、项目的数据结构。

- `WidgetRegistry`：单例注册中心，管理所有控件类型的描述符（C 类型、初始化函数、属性定义、代码生成规则）
- `WidgetModel`：单个控件的模型，包含位置、尺寸、属性、背景、阴影、动画、子控件树
- `Page`：页面模型，对应一个 XML 布局文件，包含根控件和用户自定义字段
- `Project`：项目模型，管理多个页面、屏幕配置、资源目录

### custom_widgets/ -- Widget 注册插件

每个 `.py` 文件注册一个控件类型，定义其 C 层映射关系：

```python
WidgetRegistry.instance().register(
    type_name="label",
    descriptor={
        "c_type": "egui_view_label_t",
        "init_func": "egui_view_label_init_with_params",
        "params_macro": "EGUI_VIEW_LABEL_PARAMS_INIT",
        "is_container": False,
        "properties": { ... },
    },
    xml_tag="Label",
)
```

目前已注册 50+ 控件，涵盖基础控件、布局容器、图表、时钟等。

### generator/ -- 代码生成器

代码生成器读取 XML 布局和 Widget 注册信息，生成完整的 C 代码：

- `{page}_layout.c`：100% 自动生成的布局代码，每次保存都会覆盖
- `{page}.h`：页面结构体定义，包含 USER CODE 保留区域
- `{page}.c`：用户实现骨架，仅首次创建，不会覆盖
- `uicode.h/c`：页面管理框架代码
- `app_resource_config.json`：资源配置（图片格式、字体参数）

### engine/ -- 引擎层

引擎层提供编译、预览、布局计算等运行时能力：

- `CompilerEngine`：管理增量编译和 exe 进程生命周期。支持两级编译策略：快速路径（直接调用 gcc 编译变更文件）和慢速路径（完整 make 构建）。使用双缓冲机制避免预览闪烁
- `LayoutEngine`：Python 端复刻 C 端的 LinearLayout 布局算法，用于设计器内的控件定位预览
- `AutoFixer`：自动检测并修复常见布局问题（标签高度不足、容器溢出、ID 冲突、图片缩放缺失）

## 转换管道

### 管道一：可视化编辑器

```
PyQt5 UI 操作 -> WidgetModel 修改 -> XML 序列化 -> Code Generator -> C 代码
                                                                      |
                                                              CompilerEngine
                                                                      |
                                                              实时预览 (exe)
```

### 管道二：设计稿转换（HTML/JSX -> XML -> C）

```
HTML/JSX 设计稿
    |
    v
html2egui_helper.py extract-layout    # 解析为结构化 JSON
    |
    v
html2egui_helper.py scaffold          # 创建 .egui 项目结构
    |
    v
html2egui_helper.py export-icons      # 导出图标 PNG
    |
    v
编写 XML 布局文件                       # 基于 JSON 分析编写 XML
    |
    v
html2egui_helper.py generate-code     # XML -> C 代码
    |
    v
html2egui_helper.py gen-resource      # 生成 C 资源数组
    |
    v
make all APP=MyApp                    # 编译
    |
    v
code_runtime_check.py                 # 运行时验证
```

## 安装与启动

### 环境准备

运行项目根目录的安装脚本，默认就会安装完整依赖（包含 UI Designer）：

```bat
setup.bat
```

## 实时预览 Smoke 检查

为了避免后续改动破坏 UI Designer 的真实编译预览链路，仓库增加了一个端到端 smoke 脚本：

```bash
python scripts/ui_designer_preview_smoke.py
```

这个脚本会在 SDK 目录外创建一个临时标准 App 工作区，并自动完成以下检查：

1. 生成并保存最小 `.egui` 工程，确认外部工作区的 `sdk_root` 可正确恢复
2. 通过 `CompilerEngine` 编译该外部 App，验证 SDK 与工程分离场景可用
3. 启动 headless preview bridge 并抓取首帧
4. 观察动画区域是否随时间变化，验证实时取帧链路
5. 注入一次触摸点击，确认交互事件可以驱动渲染结果更新

该脚本已经接入 `scripts/release_check.py` 和 release workflow，用来拦截“单元测试通过，但实时预览已经失效”的回归。

Linux / macOS 使用：

```bash
./setup.sh
```

默认流程会：

1. 创建 `.venv`
2. 安装 `requirements.txt`
3. 安装 `scripts/ui_designer/requirements-desktop.txt`
4. 安装 `playwright` Python 包
5. 校验 `PyQt5`、`qfluentwidgets` 和 `ui_designer.main`
6. 在 Windows 下检查 `make.exe` / `gcc.exe`，必要时自动安装 `w64devkit`

如果只想安装基础依赖，可执行：

```bash
python scripts/setup_env.py --python-mode basic
```

### 启动 UI Designer

```bash
# 从项目根目录启动
python scripts/ui_designer/main.py

# 指定项目文件
python scripts/ui_designer/main.py --project example/HelloDesigner/HelloDesigner.egui

# 指定 APP 名称
python scripts/ui_designer/main.py --app HelloDesigner

# 指定 SDK 根目录（兼容 --root 别名）
python scripts/ui_designer/main.py --sdk-root /path/to/EmbeddedGUI
```

启动后，UI Designer 会：

1. 自动检测项目根目录（通过 Makefile 位置）
2. 加载 `WidgetRegistry`（扫描 `custom_widgets/` 目录下所有插件）
3. 如果有上次打开的项目，自动加载
4. 显示主窗口（包含控件树、属性面板、XML 编辑器、实时预览）
5. 恢复上次保存的窗口尺寸、停靠面板布局与工具栏位置

## 外部变更监控

为了降低项目持续演进时把 Designer 预览链路悄悄改坏的风险，当前版本补了两层保护：

- 常规 CI 保护：GitHub Actions 的日常编译检查流程也会执行 `python scripts/ui_designer_preview_smoke.py`
- 仓库级保护：`python scripts/ui_designer_preview_smoke.py` 已接入 `release_check.py` 和 release workflow，真实验证编译预览链路。
- 运行时保护：Designer 会周期扫描当前项目下的 `.egui` 与 `.eguiproject/` 内容变化。

运行时扫描的行为如下：

- 当前项目没有未保存修改时，检测到外部文件变更会自动重新加载项目，并尽量保持当前页面不变。
- 当前项目存在未保存修改时，Designer 不会直接覆盖内存中的编辑结果，而是提示用户先保存，或使用 `File -> Reload Project From Disk` 手动同步。
- `Reload Project From Disk` 默认快捷键为 `Ctrl+Shift+R`。

这个机制主要覆盖以下场景：

- 用户在文件管理器、编辑器或脚本里直接改了 `.egui`、页面 XML、字符串资源、资源目录内容。
- 仓库更新后，Designer 仍停留在旧内存状态，导致预览结果和磁盘内容不一致。
- 团队多人协作时，外部脚本或生成器重写了 `.eguiproject` 内的文件。

## SDK 根目录与项目目录

新的 UI Designer 工作区模型拆分为三类路径：

- `sdk_root`：EmbeddedGUI SDK 根目录，包含 `Makefile`、`src/`、`porting/designer/`
- `project_dir`：当前 `.egui` 项目所在目录，也是标准 App 目录
- `app_dir`：编译时传给 Make 的 App 目录，当前等同于 `project_dir`

这意味着：

- 项目目录不再要求必须位于 SDK 目录下面
- 只要目标目录是标准 EmbeddedGUI App 结构，就可以独立保存、独立打开
- 编译时通过 `EGUI_APP_ROOT_PATH` 把外部 App 挂接到 SDK 的构建系统中

### SDK 自动发现与手动指定

补充说明，当前版本在“无 SDK 首次启动”场景下又加了一层首启引导：

- 仅当没有检测到有效 SDK，且当前没有自动打开任何项目时，才会弹出
- 引导框只提示一次，避免后续每次启动都打断用户
- 用户可以直接选择自动下载 SDK，或手动指定已有 SDK 根目录
- 引导框会直接显示自动下载默认使用的缓存路径

自动下载链路当前按下面顺序回退：

- GitHub 主分支 zip 包
- Gitee zip 包
- Gitee `git clone` 回退路径（仅在本机存在 `git` 时启用）

实际验证中，GitHub zip 是最稳定的主通路；Gitee 的匿名 zip 入口有时会返回 HTML 页面而不是压缩包，因此工具会自动继续尝试后续回退路径。
如果当前下载源长时间无法完成，工具也会自动超时并继续回退，不会无限期卡在第一条下载链路上。
如需调整这个归档下载超时，可通过环境变量 `EMBEDDEDGUI_SDK_ARCHIVE_TIMEOUT_SECONDS` 覆盖默认值。

自动下载的 SDK 默认缓存到当前用户配置目录：

```text
{config_dir}/sdk/EmbeddedGUI
```

这样不依赖 Designer 安装目录可写，也更适合独立 exe 分发。

如果自动下载失败，Designer 不会退出，也不会阻止继续编辑；用户后续仍可通过 `File -> Download SDK Copy...` 或 `File -> Set SDK Root...` 继续补齐 SDK。

启动时会按下面顺序查找 SDK 根目录：

1. 命令行 `--sdk-root`
2. 当前项目路径附近的常见目录（包括 `sdk_root/example/app` 这种可直接反推的情况）
3. 配置文件中保存的 SDK 路径
4. 环境变量 `EMBEDDEDGUI_SDK_ROOT`
5. 当前可执行文件或脚本所在目录附近的常见目录
6. 当前工作目录附近的常见目录

“常见目录”当前会覆盖这些实际布局：

- 父目录本身就是 SDK 根目录
- 父目录下直接有 `EmbeddedGUI/`、`embeddedgui/`
- 父目录下直接有 `sdk/`、`SDK/`，且该目录本身就是 SDK
- `sdk/`、`SDK/` 下面再包一层名字包含 `EmbeddedGUI` 的目录，例如 `sdk/EmbeddedGUI-main/`

如果没有找到有效 SDK，Designer 仍然可以打开并编辑项目，只是会进入编辑模式并使用 Python 预览回退。用户之后可以在 `File -> Set SDK Root...` 里补设 SDK 路径。
手动指定 SDK 时，不要求必须精确点到 SDK 根目录；如果你选择的是它的上层目录、`sdk/` 容器目录，或 `sdk/EmbeddedGUI-main/` 这类常见目录层级，Designer 也会自动纠正到真正的 SDK 根目录。

欢迎页也提供了 `Open Project File...`、`Open SDK Example...`、`Set SDK Root...`、`Download SDK...` 四个直接入口，并显示当前 SDK 状态与路径，便于独立 exe 场景快速定位“为什么现在只能走 Python 预览”。
当 SDK 缺失或无效时，欢迎页还会直接显示默认自动下载缓存路径，方便用户提前判断是否符合本机目录规划。

### 新建项目与目录冲突

如果本机还没有 SDK，也可以先创建项目，后续再通过 `File -> Download SDK Copy...` 自动补装一份本地 SDK 缓存。

`New Project...` 现在直接创建标准 App 目录，并自动补齐最小骨架：

- `build.mk`
- `app_egui_config.h`
- `resource/src/app_resource_config.json`
- `.egui` 与 `.eguiproject/`

`SDK Root` 现在是可选项：

- 如果已经配置有效 SDK，默认会落到 `sdk_root/example/`
- 如果暂时没有 SDK，也可以先创建项目并进入纯编辑模式，之后再通过 `File -> Set SDK Root...` 补上编译预览能力

如果目标目录已存在且非空，会直接报错，避免把 Designer 项目写进一个有冲突内容的目录。

### 打开 SDK Example

`Open SDK Example...` 默认只显示已经包含 `.egui` 的示例，减少把纯源码示例误当成 Designer 工程打开的情况。

- 如需查看旧示例，可勾选 `Show legacy examples without .egui`
- 支持按示例名实时搜索过滤，示例较多时可以快速定位目标工程
- 打开 legacy example 时，Designer 会在原目录初始化 `.egui` 工程
- 如果目录里已经有 `.eguiproject` 但没有 `.egui`，会直接报冲突，不会继续覆盖

最近项目如果已经被移动或删除，点击时会提示是否从最近项目列表里移除，避免列表长期残留失效路径。

### 预览策略

运行预览优先使用真实编译预览：

1. 生成代码与资源
2. 通过 SDK Makefile 编译目标 App
3. 启动 headless preview bridge 获取实时帧

如果出现下面任一情况，会自动退回 Python 预览：

- SDK 根目录缺失或无效
- 外部 App 路径无法被 Make 正常表达
- 编译成功但预览桥接自检失败
- 预览运行中断开，无法继续取帧

Python 预览只保证基础静态渲染与布局可见，不能替代真实交互验证；SDK 配置恢复后，Designer 会重新走真实编译预览链路。

### 本地打包

如果需要把 Designer 单独打包成桌面工具，推荐直接使用：

```bash
python scripts/package_ui_designer.py
```

它会统一调用 `ui_designer.spec`，并在 `dist/` 下生成运行目录与归档包。详见 [UI Designer 本地打包](package_designer.md)。

### 使用设计稿转换管道

无需启动 UI Designer 桌面应用，直接通过命令行完成转换：

```bash
# 1. 解析 HTML 设计稿
python scripts/html2egui_helper.py extract-layout --input design.html

# 2. 创建项目骨架
python scripts/html2egui_helper.py scaffold --app MyApp --width 320 --height 480

# 3. 导出图标
python scripts/html2egui_helper.py export-icons --input design.html --app MyApp --size 24

# 4. 编写 XML 布局（手动或 AI 辅助）

# 5. 生成 C 代码
python scripts/html2egui_helper.py generate-code --app MyApp

# 6. 生成资源
python scripts/html2egui_helper.py gen-resource --app MyApp

# 7. 构建验证
python scripts/html2egui_helper.py verify --app MyApp
```

## 项目文件格式

### .egui 项目文件

```xml
<Project app_name="MyApp" screen_width="320" screen_height="240"
         page_mode="easy_page" startup="main_page" egui_root="../..">
    <Pages>
        <PageRef file="layout/main_page.xml" />
        <PageRef file="layout/settings.xml" />
    </Pages>
    <Resources catalog="resources.xml" />
</Project>
```

### XML 布局文件

```xml
<?xml version="1.0" encoding="utf-8"?>
<Page>
    <Group id="root" x="0" y="0" width="320" height="240">
        <Label id="title" x="16" y="8" width="200" height="23" text="Hello" />
    </Group>
</Page>
```

### 生成的 C 文件结构

```
example/MyApp/
├── main_page.h              # 页面结构体（含 USER CODE 区域）
├── main_page_layout.c       # 布局初始化（100% 自动生成）
├── main_page.c              # 用户逻辑（仅首次创建）
├── uicode.h                 # 页面枚举和导出函数
├── uicode.c                 # 页面切换后端
└── app_egui_config.h        # 屏幕配置
```
