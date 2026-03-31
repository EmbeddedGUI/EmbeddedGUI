# 扩展新控件

## 概述

当设计转换过程中遇到不支持的控件或属性时，需要扩展 Widget 注册系统。扩展流程分为三步：确认 C 层实现、编写注册文件、验证生成代码。

本文档以实战示例演示完整的扩展流程。

## 完整扩展流程

```
1. 确认 C 层 API
   src/widget/egui_view_{type}.h  -- 查找结构体、init 函数、setter 函数
       |
       v
2. 编写注册文件
   scripts/ui_designer/custom_widgets/{type}.py  -- 注册控件描述符
       |
       v
3. 验证
   generate-code -> gen-resource -> verify  -- 构建并运行时验证
```

## 第一步：确认 C 层实现

在 `src/widget/` 目录下查找目标控件的头文件，确认以下信息：

### 需要确认的 API

| 信息 | 在头文件中查找 | 示例 |
|------|--------------|------|
| 结构体类型 | `typedef struct ... _t` | `egui_view_gauge_t` |
| 初始化函数 | `void egui_view_{type}_init(...)` | `egui_view_gauge_init` |
| 带参数初始化 | `void egui_view_{type}_init_with_params(...)` | `egui_view_gauge_init_with_params` |
| 参数宏 | `#define EGUI_VIEW_{TYPE}_PARAMS_INIT(...)` | `EGUI_VIEW_GAUGE_PARAMS_INIT` |
| 参数类型 | `typedef struct ... _params_t` | `egui_view_gauge_params_t` |
| setter 函数 | `void egui_view_{type}_set_*(...)` | `egui_view_gauge_set_value` |
| 是否容器 | 是否有 `add_child` 相关函数 | -- |

### 示例：查看 gauge 控件

```bash
# 查找头文件
ls src/widget/egui_view_gauge*

# 查看 API 声明
grep -n "void egui_view_gauge" src/widget/egui_view_gauge.h
```

典型头文件结构：

```c
// src/widget/egui_view_gauge.h

typedef struct egui_view_gauge egui_view_gauge_t;

typedef struct egui_view_gauge_params
{
    egui_view_params_t base;
    // gauge-specific params
} egui_view_gauge_params_t;

#define EGUI_VIEW_GAUGE_PARAMS_INIT(_name, ...)  \
    static egui_view_gauge_params_t _name = { ... }

void egui_view_gauge_init(egui_view_t *self);
void egui_view_gauge_init_with_params(egui_view_t *self, egui_view_gauge_params_t *params);
void egui_view_gauge_set_value(egui_view_t *self, int value);
void egui_view_gauge_set_range(egui_view_t *self, int min, int max);
void egui_view_gauge_set_stroke_width(egui_view_t *self, int width);
```

## 第二步：编写注册文件

在 `scripts/ui_designer/custom_widgets/` 下创建 `{type}.py`：

### 注册文件模板

```python
"""Gauge widget plugin for EmbeddedGUI Designer."""
from ui_designer.model.widget_registry import WidgetRegistry

WidgetRegistry.instance().register(
    type_name="gauge",                          # 内部类型名（小写下划线）
    descriptor={
        "c_type": "egui_view_gauge_t",          # 必须与头文件一致
        "init_func": "egui_view_gauge_init_with_params",  # 必须与头文件一致
        "params_macro": "EGUI_VIEW_GAUGE_PARAMS_INIT",    # 必须与头文件一致
        "params_type": "egui_view_gauge_params_t",
        "is_container": False,                  # gauge 不是容器
        "add_child_func": None,
        "layout_func": None,
        "properties": {
            "value": {
                "type": "int",
                "default": 0,
                "min": 0,
                "max": 100,
                "code_gen": {
                    "kind": "setter",
                    "func": "egui_view_gauge_set_value",  # 必须与头文件一致
                },
            },
            "stroke_width": {
                "type": "int",
                "default": 6,
                "code_gen": {
                    "kind": "setter",
                    "func": "egui_view_gauge_set_stroke_width",
                    "skip_default": True,
                },
            },
        },
    },
    xml_tag="Gauge",                            # XML 中使用的标签名
    display_name="Gauge",                       # UI 菜单显示名
)
```

### 属性 code_gen 选择指南

| 场景 | kind | 说明 |
|------|------|------|
| 单参数 setter | `setter` | `func(view, value)` |
| 设置文本 | `text_setter` | 自动处理 `@string/` 引用 |
| 多参数 setter | `multi_setter` | `func(view, arg1, arg2)` |
| 字体/图片资源 | `derived_setter` | 从多个属性派生 C 表达式 |
| 图片设置 | `image_setter` | 专用图片资源引用 |
| 不生成代码 | `None` | 仅用于 UI 编辑，不影响 C 代码 |

### 容器控件的额外字段

如果控件是容器（可包含子控件），需要设置：

```python
"is_container": True,
"add_child_func": "egui_view_group_add_child",    # 添加子控件的函数
"layout_func": "egui_view_xxx_layout_childs",      # 布局子控件的函数（可选）
```

### 事件回调的注册

```python
"events": {
    "onValueChanged": {
        "setter": "egui_view_gauge_set_on_value_changed_listener",
        "signature": "void {func_name}(egui_view_t *self, int value)",
    },
},
```

## 第三步：验证

### 编写测试 XML

在项目的 `.eguiproject/layout/` 下添加使用新控件的 XML：

```xml
<Gauge id="my_gauge" x="50" y="50" width="100" height="100"
       value="75" stroke_width="8" />
```

### 生成并构建

```bash
# 在 EmbeddedGUI_Designer 仓库根目录执行
python html2egui_helper.py generate-code --app MyApp

# 生成资源
python html2egui_helper.py gen-resource --app MyApp

# 构建
make all APP=MyApp PORT=pc BITS=64

# 运行时验证
python scripts/code_runtime_check.py --app MyApp --bits64 --keep-screenshots
```

### 检查截图

查看 `runtime_check_output/MyApp/` 下的 PNG 截图，确认新控件渲染正确。

## 检查清单

扩展新控件时，逐项确认以下内容：

- [ ] `src/widget/` 中存在对应的 C 实现（`egui_view_{type}.h/c`）
- [ ] `c_type` 与 C 头文件中的 `typedef` 一致
- [ ] `init_func` 与 C 头文件中的初始化函数名一致
- [ ] `params_macro` 与 C 头文件中的参数宏名一致
- [ ] 所有 `code_gen.func` 函数名在 C 头文件中有声明
- [ ] `xml_tag` 与 `EmbeddedGUI_Designer` 仓库中的 HTML 转换文档示例保持一致
- [ ] 属性的 `type` 和 `default` 值合理
- [ ] 容器控件设置了 `is_container`、`add_child_func`
- [ ] 构建通过（无 `undefined reference` 错误）
- [ ] 运行时验证截图正确

## 排错指南

### undefined reference to 'egui_view_xxx_set_yyy'

原因：`code_gen.func` 中的函数名与 C 头文件不一致。

排查步骤：

```bash
# 1. 查找 C 头文件中的实际函数名
grep "void egui_view_xxx" src/widget/egui_view_xxx.h

# 2. 对比注册文件中的 func 字段
grep "func" scripts/ui_designer/custom_widgets/xxx.py

# 3. 修正函数名
```

常见错误：

| 注册文件中的错误 | 正确值 |
|----------------|--------|
| `set_value` | `set_process`（ProgressBar） |
| `set_text_color` | `set_font_color`（Label） |
| `set_orientation` 缺少 value_map | 需要 `{"horizontal": "1", "vertical": "0"}` |

### 渲染异常：控件不显示

可能原因：

1. `init_func` 错误 -- 控件未正确初始化
2. 缺少 `add_child_func` -- 子控件未添加到父容器
3. 尺寸为 0 -- XML 中缺少 width/height
4. alpha 为 0 -- 检查透明度设置

### 渲染异常：属性不生效

可能原因：

1. `skip_default: True` 导致默认值被跳过 -- 设置 `skip_default: False`
2. `value_map` 缺失 -- 枚举值需要映射为 C 常量
3. `group` 标识导致重复跳过 -- 检查同组属性的 code_gen 配置
4. `code_gen: None` -- 属性未配置代码生成

### 编译警告：implicit declaration

原因：C 头文件未被包含。检查 `build.mk` 是否包含了控件所在目录。

## 实战示例：扩展 Gauge 控件

### 1. 确认 C 层 API

```bash
grep -n "void egui_view_gauge" src/widget/egui_view_gauge.h
```

输出：

```
15: void egui_view_gauge_init(egui_view_t *self);
16: void egui_view_gauge_init_with_params(egui_view_t *self, egui_view_gauge_params_t *params);
17: void egui_view_gauge_set_value(egui_view_t *self, int value);
18: void egui_view_gauge_set_range(egui_view_t *self, int min_val, int max_val);
19: void egui_view_gauge_set_stroke_width(egui_view_t *self, int width);
20: void egui_view_gauge_set_color(egui_view_t *self, egui_color_t color, egui_alpha_t alpha);
```

### 2. 编写注册文件

创建 `scripts/ui_designer/custom_widgets/gauge.py`：

```python
"""Gauge widget plugin for EmbeddedGUI Designer."""
from ui_designer.model.widget_registry import WidgetRegistry

WidgetRegistry.instance().register(
    type_name="gauge",
    descriptor={
        "c_type": "egui_view_gauge_t",
        "init_func": "egui_view_gauge_init_with_params",
        "params_macro": "EGUI_VIEW_GAUGE_PARAMS_INIT",
        "params_type": "egui_view_gauge_params_t",
        "is_container": False,
        "add_child_func": None,
        "layout_func": None,
        "properties": {
            "value": {
                "type": "int", "default": 0, "min": 0, "max": 100,
                "code_gen": {"kind": "setter", "func": "egui_view_gauge_set_value"},
            },
            "stroke_width": {
                "type": "int", "default": 6,
                "code_gen": {"kind": "setter", "func": "egui_view_gauge_set_stroke_width",
                             "skip_default": True},
            },
            "color": {
                "type": "color", "default": "EGUI_COLOR_WHITE",
                "code_gen": {"kind": "multi_setter",
                             "func": "egui_view_gauge_set_color",
                             "args": "{color}, {alpha}",
                             "group": "gauge_color",
                             "skip_default": True},
            },
            "alpha": {
                "type": "alpha", "default": "EGUI_ALPHA_100",
                "code_gen": None,
            },
        },
    },
    xml_tag="Gauge",
    display_name="Gauge",
)
```

### 3. 编写测试 XML

```xml
<?xml version="1.0" encoding="utf-8"?>
<Page>
    <Group id="root" x="0" y="0" width="240" height="320">
        <Gauge id="speed_gauge" x="70" y="60" width="100" height="100"
               value="75" stroke_width="8"
               color="EGUI_COLOR_HEX(0x00F3FF)" alpha="EGUI_ALPHA_100" />
    </Group>
</Page>
```

### 4. 生成并验证

```bash
# 在 EmbeddedGUI_Designer 仓库根目录执行
python html2egui_helper.py generate-code --app MyApp
make all APP=MyApp PORT=pc BITS=64
python scripts/code_runtime_check.py --app MyApp --bits64 --keep-screenshots
```

### 5. 检查生成的 C 代码

确认 `*_layout.c` 中生成了正确的调用：

```c
// speed_gauge (gauge)
egui_view_gauge_init((egui_view_t *)&local->speed_gauge);
egui_view_set_position((egui_view_t *)&local->speed_gauge, 70, 60);
egui_view_set_size((egui_view_t *)&local->speed_gauge, 100, 100);
egui_view_gauge_set_value((egui_view_t *)&local->speed_gauge, 75);
egui_view_gauge_set_stroke_width((egui_view_t *)&local->speed_gauge, 8);
egui_view_gauge_set_color((egui_view_t *)&local->speed_gauge,
    EGUI_COLOR_HEX(0x00F3FF), EGUI_ALPHA_100);
```

## 扩展已有控件的属性

如果已注册的控件缺少某个属性，只需在对应的 `custom_widgets/*.py` 中添加属性定义：

```python
# 在 properties 字典中新增
"new_property": {
    "type": "int",
    "default": 0,
    "code_gen": {
        "kind": "setter",
        "func": "egui_view_xxx_set_new_property",
    },
},
```

然后重新运行 `generate-code` 即可。无需修改代码生成器本身。
