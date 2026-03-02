# Widget 注册系统原理

## 概述

Widget 注册系统是 UI Designer 的核心基础设施，它提供了一个统一的注册中心，将 XML 标签、Python 模型和 C 代码生成规则关联在一起。所有控件的类型信息、属性定义、代码生成规则都通过注册系统管理，实现了"数据驱动"的代码生成。

## WidgetRegistry 单例架构

`WidgetRegistry` 位于 `scripts/ui_designer/model/widget_registry.py`，采用单例模式：

```python
class WidgetRegistry:
    _instance = None

    def __init__(self):
        self._types = {}        # type_name -> descriptor dict
        self._tag_map = {}      # XML tag -> type_name
        self._rev_tag_map = {}  # type_name -> XML tag
        self._addable = []      # [(display_name, type_name), ...]

    @classmethod
    def instance(cls):
        if cls._instance is None:
            cls._instance = cls()
            cls._instance._load_builtins()
        return cls._instance
```

首次调用 `instance()` 时，自动扫描 `custom_widgets/` 目录下所有 `.py` 插件文件并执行，每个插件调用 `register()` 注册自己的控件类型。

### 核心 API

| 方法 | 说明 |
|------|------|
| `register(type_name, descriptor, xml_tag, display_name)` | 注册控件类型 |
| `get(type_name)` | 获取控件描述符 |
| `has(type_name)` | 检查控件是否已注册 |
| `tag_to_type(tag)` | XML 标签 -> 内部类型名 |
| `type_to_tag(type_name)` | 内部类型名 -> XML 标签 |
| `addable_types()` | 返回可添加控件列表（用于 UI 菜单） |
| `container_types()` | 返回所有容器类型集合 |
| `all_types()` | 返回所有已注册类型 |
| `load_custom_widgets(*dirs)` | 扫描目录加载插件 |

### 插件加载机制

```python
def load_custom_widgets(self, *dirs):
    for d in dirs:
        for fname in sorted(os.listdir(d)):
            if not fname.endswith(".py") or fname.startswith("_"):
                continue
            spec = importlib.util.spec_from_file_location(mod_name, path)
            mod = importlib.util.module_from_spec(spec)
            spec.loader.exec_module(mod)  # 执行插件，触发 register()
```

以 `_` 开头的文件会被跳过（如 `__init__.py`）。插件按文件名排序加载。

## 注册文件格式详解

每个 `custom_widgets/*.py` 文件描述一个控件的完整代码生成规则。以下是各字段的详细说明。

### 基本结构

```python
from ui_designer.model.widget_registry import WidgetRegistry

WidgetRegistry.instance().register(
    type_name="progress_bar",          # 内部类型名（小写下划线）
    descriptor={
        "c_type": "egui_view_progress_bar_t",           # C 结构体类型
        "init_func": "egui_view_progress_bar_init_with_params",  # 初始化函数
        "params_macro": "EGUI_VIEW_PROGRESS_BAR_PARAMS_INIT",   # 参数宏
        "params_type": "egui_view_progress_bar_params_t",        # 参数类型
        "is_container": False,         # 是否是容器控件
        "add_child_func": None,        # 添加子控件函数（容器专用）
        "layout_func": None,           # 布局函数（容器专用）
        "properties": { ... },         # 属性定义
        "events": { ... },             # 事件定义
    },
    xml_tag="ProgressBar",             # XML 中的标签名
    display_name="ProgressBar",        # UI 菜单显示名称
)
```

### descriptor 字段说明

| 字段 | 类型 | 说明 |
|------|------|------|
| `c_type` | string | C 结构体类型名，如 `egui_view_label_t` |
| `init_func` | string | 带参数的初始化函数，如 `egui_view_label_init_with_params` |
| `params_macro` | string | 参数初始化宏，如 `EGUI_VIEW_LABEL_PARAMS_INIT` |
| `params_type` | string | 参数结构体类型，如 `egui_view_label_params_t` |
| `is_container` | bool | 是否可包含子控件 |
| `add_child_func` | string/None | 添加子控件的 C 函数，如 `egui_view_group_add_child` |
| `layout_func` | string/None | 布局子控件的 C 函数，如 `egui_view_linearlayout_layout_childs` |
| `properties` | dict | 属性定义字典 |
| `events` | dict | 事件回调定义字典 |
| `addable` | bool | 是否出现在"添加控件"菜单中（默认 True） |

### properties 属性定义

每个属性是一个字典，包含类型、默认值和代码生成规则：

```python
"properties": {
    "value": {
        "type": "int",              # 属性类型
        "default": 50,              # 默认值
        "min": 0,                   # 最小值（可选）
        "max": 100,                 # 最大值（可选）
        "ui_group": "main",         # UI 面板分组（可选）
        "ui_visible_when": {...},   # 条件可见性（可选）
        "code_gen": {               # 代码生成规则
            "kind": "setter",
            "func": "egui_view_progress_bar_set_process",
        },
    },
}
```

#### 属性类型映射

| type 值 | Python 类型 | 说明 |
|---------|------------|------|
| `"int"` | int | 整数值 |
| `"bool"` | bool | 布尔值 |
| `"string"` | str | 字符串 |
| `"color"` | str | 颜色常量，如 `EGUI_COLOR_WHITE` |
| `"alpha"` | str | 透明度常量，如 `EGUI_ALPHA_100` |
| `"font"` | str | 内置字体引用 |
| `"font_file"` | str | 自定义字体文件名 |
| `"font_pixelsize"` | str | 字体像素大小 |
| `"font_fontbitsize"` | str | 字体位深度 |
| `"font_external"` | str | 字体是否外部存储 |
| `"text_file"` | str | 文本文件引用 |
| `"align"` | str | 对齐常量，如 `EGUI_ALIGN_CENTER` |
| `"orientation"` | str | 方向，`"vertical"` 或 `"horizontal"` |
| `"image_file"` | str | 图片文件名 |
| `"image_format"` | str | 图片格式，如 `rgb565`, `alpha` |
| `"image_alpha"` | str | 图片 alpha 位深度 |
| `"image_external"` | str | 图片是否外部存储 |

### events 事件定义

```python
"events": {
    "onCheckedChanged": {
        "setter": "egui_view_switch_set_on_checked_listener",
        "signature": "void {func_name}(egui_view_t *self, int is_checked)",
    },
}
```

- `setter`：注册回调的 C 函数
- `signature`：回调函数签名模板，`{func_name}` 会被替换为用户指定的函数名

## code_gen kind 类型详解

`code_gen` 字段定义了属性值如何转换为 C 代码。`kind` 字段决定生成策略：

### setter -- 简单 setter 调用

最常见的类型，生成 `func(view, value)` 形式的调用。

```python
"code_gen": {
    "kind": "setter",
    "func": "egui_view_progress_bar_set_process",
    "skip_default": True,      # 值等于默认值时跳过（默认 True）
    "skip_values": [],         # 跳过特定值列表
    "value_map": {},           # 值映射表
    "bool_to_int": False,      # 布尔值转 0/1
}
```

生成示例：

```c
egui_view_progress_bar_set_process((egui_view_t *)&local->bar, 75);
```

带 `value_map` 的示例（LinearLayout 方向）：

```python
"code_gen": {
    "kind": "setter",
    "func": "egui_view_linearlayout_set_orientation",
    "value_map": {"horizontal": "1", "vertical": "0"},
    "skip_values": ["vertical"],
}
```

### text_setter -- 文本设置

用于设置静态字符串属性，自动处理 `@string/` 国际化引用。

```python
"code_gen": {"kind": "text_setter", "func": "egui_view_label_set_text"}
```

生成示例：

```c
// 普通文本
egui_view_label_set_text((egui_view_t *)&local->title, "Hello");

// 国际化引用
egui_view_label_set_text((egui_view_t *)&local->title, egui_i18n_get(EGUI_STR_HELLO));
```

### multi_setter -- 多参数 setter

当一个 C 函数需要多个属性值作为参数时使用。通过 `args` 模板引用其他属性。

```python
"color": {
    "type": "color", "default": "EGUI_COLOR_WHITE",
    "code_gen": {
        "kind": "multi_setter",
        "func": "egui_view_label_set_font_color",
        "args": "{color}, {alpha}",    # 引用 color 和 alpha 两个属性
        "group": "font_color",         # 分组标识，同组只生成一次
    },
},
"alpha": {
    "type": "alpha", "default": "EGUI_ALPHA_100",
    "code_gen": None,  # alpha 属性本身不生成代码，由 color 的 multi_setter 引用
},
```

生成示例：

```c
egui_view_label_set_font_color((egui_view_t *)&local->title, EGUI_COLOR_WHITE, EGUI_ALPHA_100);
```

### derived_setter -- 派生 setter

属性值不直接使用，而是从多个属性派生出 C 表达式。常用于字体和图片资源引用。

```python
"font_file": {
    "type": "font_file", "default": "",
    "code_gen": {
        "kind": "derived_setter",
        "func": "egui_view_label_set_font",
        "derive": "font",              # 派生类型：font 或 image
        "cast": "(egui_font_t *)",     # 类型转换前缀
    },
},
```

派生逻辑（以 font 为例）：

1. 读取 `font_file`、`font_pixelsize`、`font_fontbitsize` 属性
2. 组合为 C 表达式：`&egui_res_font_{name}_{pixelsize}_{bitsize}`
3. 如果 `font_file` 为空，使用 `font_builtin` 的值

生成示例：

```c
egui_view_label_set_font((egui_view_t *)&local->title, (egui_font_t *)&egui_res_font_custom_16_4);
```

### image_setter -- 图片 setter

专用于图片资源引用，从 `image_file`、`image_format`、`image_alpha` 派生 C 表达式。

```python
"image_file": {
    "type": "image_file", "default": "",
    "code_gen": {
        "kind": "image_setter",
        "func": "egui_view_image_set_image",
        "cast": "(egui_image_t *)",
    },
},
```

生成示例：

```c
egui_view_image_set_image((egui_view_t *)&local->icon, (egui_image_t *)&egui_res_image_star_rgb565_4);
```

## 控件注册示例

### 简单控件：ProgressBar

```python
# custom_widgets/progress_bar.py
WidgetRegistry.instance().register(
    type_name="progress_bar",
    descriptor={
        "c_type": "egui_view_progress_bar_t",
        "init_func": "egui_view_progress_bar_init_with_params",
        "params_macro": "EGUI_VIEW_PROGRESS_BAR_PARAMS_INIT",
        "params_type": "egui_view_progress_bar_params_t",
        "is_container": False,
        "properties": {
            "value": {
                "type": "int", "default": 50, "min": 0, "max": 100,
                "code_gen": {"kind": "setter", "func": "egui_view_progress_bar_set_process"},
            },
        },
        "events": {
            "onProgressChanged": {
                "setter": "egui_view_progress_bar_set_on_progress_listener",
                "signature": "void {func_name}(egui_view_t *self, uint8_t progress)",
            },
        },
    },
    xml_tag="ProgressBar",
    display_name="ProgressBar",
)
```

### 容器控件：LinearLayout

```python
# custom_widgets/linearlayout.py
WidgetRegistry.instance().register(
    type_name="linearlayout",
    descriptor={
        "c_type": "egui_view_linearlayout_t",
        "init_func": "egui_view_linearlayout_init_with_params",
        "params_macro": "EGUI_VIEW_LINEARLAYOUT_PARAMS_INIT",
        "params_type": "egui_view_linearlayout_params_t",
        "is_container": True,
        "add_child_func": "egui_view_group_add_child",
        "layout_func": "egui_view_linearlayout_layout_childs",
        "properties": {
            "align_type": {
                "type": "align", "default": "EGUI_ALIGN_CENTER",
                "code_gen": {"kind": "setter", "func": "egui_view_linearlayout_set_align_type"},
            },
            "orientation": {
                "type": "orientation", "default": "vertical",
                "code_gen": {"kind": "setter", "func": "egui_view_linearlayout_set_orientation",
                             "value_map": {"horizontal": "1", "vertical": "0"},
                             "skip_values": ["vertical"]},
            },
        },
    },
    xml_tag="LinearLayout",
    display_name="LinearLayout",
)
```

### 带事件的控件：Switch

```python
# custom_widgets/switch.py
WidgetRegistry.instance().register(
    type_name="switch",
    descriptor={
        "c_type": "egui_view_switch_t",
        "init_func": "egui_view_switch_init_with_params",
        "params_macro": "EGUI_VIEW_SWITCH_PARAMS_INIT",
        "params_type": "egui_view_switch_params_t",
        "is_container": False,
        "properties": {
            "is_checked": {
                "type": "bool", "default": False,
                "code_gen": {"kind": "setter", "func": "egui_view_switch_set_checked",
                             "bool_to_int": True, "skip_default": True},
            },
        },
        "events": {
            "onCheckedChanged": {
                "setter": "egui_view_switch_set_on_checked_listener",
                "signature": "void {func_name}(egui_view_t *self, int is_checked)",
            },
        },
    },
    xml_tag="Switch",
    display_name="Switch",
)
```

## 已注册控件一览表

| XML 标签 | type_name | 容器 | 注册文件 |
|----------|-----------|------|----------|
| `Group` | group | 是 | group.py |
| `Card` | card | 是 | card.py |
| `LinearLayout` | linearlayout | 是 | linearlayout.py |
| `GridLayout` | gridlayout | 是 | gridlayout.py |
| `Scroll` | scroll | 是 | scroll.py |
| `ViewPage` | viewpage | 是 | viewpage.py |
| `TileView` | tileview | 是 | tileview.py |
| `Window` | window | 是 | window.py |
| `Menu` | menu | 是 | menu.py |
| `Label` | label | 否 | label.py |
| `DynamicLabel` | dynamic_label | 否 | dynamic_label.py |
| `Textblock` | textblock | 否 | textblock.py |
| `Button` | button | 否 | button.py |
| `ImageButton` | image_button | 否 | image_button.py |
| `Image` | image | 否 | image.py |
| `AnimatedImage` | animated_image | 否 | animated_image.py |
| `Switch` | switch | 否 | switch.py |
| `Checkbox` | checkbox | 否 | checkbox.py |
| `RadioButton` | radio_button | 否 | radio_button.py |
| `ToggleButton` | toggle_button | 否 | toggle_button.py |
| `ProgressBar` | progress_bar | 否 | progress_bar.py |
| `CircularProgressBar` | circular_progress_bar | 否 | circular_progress_bar.py |
| `Slider` | slider | 否 | slider.py |
| `ArcSlider` | arc_slider | 否 | arc_slider.py |
| `Spinner` | spinner | 否 | spinner.py |
| `Led` | led | 否 | led.py |
| `Combobox` | combobox | 否 | combobox.py |
| `Roller` | roller | 否 | roller.py |
| `NumberPicker` | number_picker | 否 | number_picker.py |
| `TabBar` | tab_bar | 否 | tab_bar.py |
| `PageIndicator` | page_indicator | 否 | page_indicator.py |
| `ChartLine` | chart_line | 否 | chart_line.py |
| `ChartScatter` | chart_scatter | 否 | chart_scatter.py |
| `ChartBar` | chart_bar | 否 | chart_bar.py |
| `ChartPie` | chart_pie | 否 | chart_pie.py |
| `AnalogClock` | analog_clock | 否 | analog_clock.py |
| `DigitalClock` | digital_clock | 否 | digital_clock.py |
| `Stopwatch` | stopwatch | 否 | stopwatch.py |
| `ActivityRing` | activity_ring | 否 | activity_ring.py |
| `HeartRate` | heart_rate | 否 | heart_rate.py |
| `Compass` | compass | 否 | compass.py |
| `NotificationBadge` | notification_badge | 否 | notification_badge.py |
| `MiniCalendar` | mini_calendar | 否 | mini_calendar.py |
| `Line` | line | 否 | line.py |
| `Scale` | scale | 否 | scale.py |
| `ButtonMatrix` | button_matrix | 否 | button_matrix.py |
| `Table` | table | 否 | table.py |
| `List` | list | 否 | list.py |
| `Spangroup` | spangroup | 否 | spangroup.py |

## 注册系统与其他模块的关系

```
custom_widgets/*.py
    |
    | register()
    v
WidgetRegistry (单例)
    |
    +---> WidgetModel.from_xml_element()   # XML 解析时查询控件类型
    |
    +---> code_generator.py                # 代码生成时查询 c_type/init_func/properties
    |
    +---> property_panel.py                # 属性面板根据 properties 动态生成 UI
    |
    +---> widget_tree.py                   # 控件树使用 addable_types() 构建菜单
    |
    +---> layout_engine.py                 # 布局引擎查询 layout_func 判断布局方式
```
