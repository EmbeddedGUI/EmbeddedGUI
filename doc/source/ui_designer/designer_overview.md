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

# 指定项目根目录
python scripts/ui_designer/main.py --root /path/to/EmbeddedGUI
```

启动后，UI Designer 会：

1. 自动检测项目根目录（通过 Makefile 位置）
2. 加载 `WidgetRegistry`（扫描 `custom_widgets/` 目录下所有插件）
3. 如果有上次打开的项目，自动加载
4. 显示主窗口（包含控件树、属性面板、XML 编辑器、实时预览）

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
