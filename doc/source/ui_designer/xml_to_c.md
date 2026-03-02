# XML -> C 代码生成

## 概述

XML -> C 代码生成是 UI Designer 转换管道的核心环节。代码生成器读取 XML 布局文件和 Widget 注册信息，生成完整的、可编译的 EGUI C 代码。

代码生成器位于 `scripts/ui_designer/generator/code_generator.py`，采用 MFC 风格的多文件输出架构，实现了生成代码与用户代码的零重叠。

## 生成文件架构

每个页面生成三个文件，加上全局框架文件：

```
example/MyApp/
├── {page_name}.h              # 页面结构体（生成，含 USER CODE 保留区域）
├── {page_name}_layout.c       # 布局初始化（100% 生成，每次覆盖）
├── {page_name}.c              # 用户实现骨架（仅首次创建，不覆盖）
├── uicode.h                   # 页面枚举 + 导出函数（生成）
├── uicode.c                   # 页面切换后端（生成）
└── app_egui_config.h          # 屏幕配置（仅首次创建）
```

### 文件所有权分类

| 文件 | 分类 | 说明 |
|------|------|------|
| `*_layout.c` | GENERATED_ALWAYS | 每次保存都完全覆盖 |
| `*.h` | GENERATED_PRESERVED | 覆盖但保留 USER CODE 区域 |
| `*.c` (用户文件) | USER_OWNED | 仅首次创建，永不覆盖 |
| `uicode.h/c` | GENERATED_ALWAYS | 每次覆盖 |
| `app_egui_config.h` | USER_OWNED | 仅首次创建 |

## XML 解析和控件树构建

### XML 文件结构

```xml
<?xml version="1.0" encoding="utf-8"?>
<Page>
    <Group id="root" x="0" y="0" width="320" height="240">
        <Background type="solid" color="EGUI_COLOR_HEX(0x0F172A)" alpha="EGUI_ALPHA_100" />
        <Label id="title" x="16" y="8" width="200" height="23" text="Hello" />
        <LinearLayout id="list" x="16" y="40" width="288" height="180"
                      orientation="vertical" align_type="EGUI_ALIGN_CENTER">
            <Button id="btn1" x="0" y="0" width="100" height="40" text="Click" />
        </LinearLayout>
    </Group>
</Page>
```

### 解析流程

1. `Page.from_xml_string()` 解析 XML 字符串
2. 根据 XML 标签通过 `WidgetRegistry.tag_to_type()` 查找控件类型
3. `WidgetModel.from_xml_element()` 递归构建控件树
4. 特殊子元素 `<Background>`、`<Shadow>`、`<Animation>` 解析为对应模型
5. 属性值根据注册信息中的 `type` 进行类型转换（int/bool/string）

```python
# 解析流程伪代码
root_elem = ET.fromstring(xml_string)
for child in root_elem:
    if child.tag == "Background":
        page.root_widget.background = BackgroundModel.from_xml_element(child)
    elif child.tag == "Animation":
        page.root_widget.animations.append(AnimationModel.from_xml_element(child))
    else:
        widget = WidgetModel.from_xml_element(child)
        # widget_type = WidgetRegistry.instance().tag_to_type(child.tag)
```

## C 代码生成规则

### 1. 页面头文件 ({page_name}.h)

生成页面结构体，包含所有控件成员和动画成员：

```c
#ifndef _MAIN_PAGE_H_
#define _MAIN_PAGE_H_

#include "egui.h"

// USER CODE BEGIN includes
// USER CODE END includes

typedef struct egui_main_page egui_main_page_t;
struct egui_main_page
{
    egui_page_base_t base;

    // UI widgets (auto-generated, do not edit)
    egui_view_group_t root;
    egui_view_label_t title;
    egui_view_linearlayout_t list;
    egui_view_button_t btn1;

    // Animations (auto-generated, do not edit)
    egui_animation_alpha_t anim_title_alpha;

    // USER CODE BEGIN user_fields
    // USER CODE END user_fields
};

void egui_main_page_layout_init(egui_page_base_t *self);
void egui_main_page_init(egui_page_base_t *self);

// USER CODE BEGIN declarations
// USER CODE END declarations

#endif /* _MAIN_PAGE_H_ */
```

控件成员的 C 类型从 Widget 注册信息的 `c_type` 字段获取。

### 2. 布局源文件 ({page_name}_layout.c)

100% 自动生成，包含完整的控件初始化、属性设置、层级构建和布局代码。

生成过程按以下顺序：

#### (a) 静态变量声明

背景、阴影、动画的参数宏在文件作用域声明：

```c
// Background declarations
static egui_background_color_t bg_main_page_root;
EGUI_BACKGROUND_COLOR_PARAM_INIT_SOLID(bg_main_page_root_param_normal,
    EGUI_COLOR_HEX(0x0F172A), EGUI_ALPHA_100);
EGUI_BACKGROUND_PARAM_INIT(bg_main_page_root_params,
    &bg_main_page_root_param_normal, NULL, NULL);

// Animation declarations
EGUI_ANIMATION_ALPHA_PARAMS_INIT(anim_title_alpha_params, EGUI_ALPHA_0, EGUI_ALPHA_100);
static egui_interpolator_decelerate_t anim_title_alpha_interpolator;
```

#### (b) 前向声明

onClick 回调和事件回调的 extern 声明：

```c
// Forward declarations for onClick callbacks
extern void on_btn1_click(egui_view_t *self);

// Forward declarations for event callbacks
extern void on_switch_changed(egui_view_t *self, int is_checked);
```

#### (c) layout_init 函数

```c
void egui_main_page_layout_init(egui_page_base_t *self)
{
    egui_main_page_t *local = (egui_main_page_t *)self;

    // Init views
    // root (group)
    egui_view_group_init((egui_view_t *)&local->root);
    egui_view_set_position((egui_view_t *)&local->root, 0, 0);
    egui_view_set_size((egui_view_t *)&local->root, 320, 240);

    // title (label)
    egui_view_label_init((egui_view_t *)&local->title);
    egui_view_set_position((egui_view_t *)&local->title, 16, 8);
    egui_view_set_size((egui_view_t *)&local->title, 200, 23);
    egui_view_label_set_text((egui_view_t *)&local->title, "Hello");

    // Set backgrounds
    egui_background_color_init_with_params(
        (egui_background_t *)&bg_main_page_root, &bg_main_page_root_params);
    egui_view_set_background(
        (egui_view_t *)&local->root, (egui_background_t *)&bg_main_page_root);

    // Build hierarchy
    egui_view_group_add_child(
        (egui_view_t *)&local->root, (egui_view_t *)&local->title);
    egui_view_group_add_child(
        (egui_view_t *)&local->root, (egui_view_t *)&local->list);
    egui_view_group_add_child(
        (egui_view_t *)&local->list, (egui_view_t *)&local->btn1);

    // Re-layout children
    egui_view_linearlayout_layout_childs((egui_view_t *)&local->list);

    // Init animations
    egui_animation_alpha_init(EGUI_ANIM_OF(&local->anim_title_alpha));
    // ...

    // Add to page root
    egui_page_base_add_view(self, (egui_view_t *)&local->root);
}
```

### 3. 参数宏展开

代码生成器使用 `_simple_init_func()` 获取不带参数的初始化函数（去掉 `_with_params` 后缀）：

```python
def _simple_init_func(widget_type):
    info = _get_type_info(widget_type)
    func = info.get("init_func", "")
    return func.replace("_with_params", "")
```

例如：`egui_view_label_init_with_params` -> `egui_view_label_init`

### 4. 属性 setter 调用

属性代码生成由 `_emit_property_code()` 函数驱动，根据 `code_gen.kind` 生成不同形式的 C 代码：

```python
def _emit_property_code(widget, prop_name, prop_def, cg, cast, indent):
    kind = cg["kind"]
    func = cg["func"]
    value = widget.properties.get(prop_name, prop_def.get("default"))

    if kind == "setter":
        return f"{indent}{func}({cast}, {c_value});"
    elif kind == "text_setter":
        return f'{indent}{func}({cast}, "{text}");'
    elif kind == "multi_setter":
        return f"{indent}{func}({cast}, {c_args});"
    elif kind == "derived_setter":
        return f"{indent}{func}({cast}, {cast_prefix}{expr});"
```

### 5. add_child 层级构建

遍历所有有子控件的容器，使用注册信息中的 `add_child_func` 构建父子关系：

```python
for w in all_widgets:
    if not w.children:
        continue
    type_info = _get_type_info(w.widget_type)
    add_func = type_info.get("add_child_func")
    if add_func:
        for child in w.children:
            # egui_view_group_add_child(parent, child);
```

### 6. 布局调用

对于有 `layout_func` 的容器（如 LinearLayout），在层级构建后调用布局函数：

```c
egui_view_linearlayout_layout_childs((egui_view_t *)&local->list);
```

如果子控件有显式的非零 x/y 位置，则跳过自动布局。

## 资源生成

### app_resource_config.json

`ResourceConfigGenerator` 扫描所有页面的控件，收集图片和字体资源配置：

```python
class ResourceConfigGenerator:
    def generate(self, project):
        for page in project.pages:
            for widget in page.get_all_widgets():
                if widget.widget_type == "image":
                    cfg = self._collect_image_config(widget)
                elif widget.widget_type in ("label", "button"):
                    cfg = self._collect_font_config(widget)
        return {"img": img_configs, "font": font_configs}
```

输出示例：

```json
{
    "img": [
        {"file": "icon_wifi.png", "format": "alpha", "alpha": "4", "external": "0", "swap": "0"}
    ],
    "font": [
        {"file": "custom.ttf", "pixelsize": "16", "fontbitsize": "4",
         "external": "0", "text": "_generated_text_custom_16_4.txt"}
    ]
}
```

### 图片资源去重

同一图片文件的不同格式/alpha 配置会去重。如果同一文件有不同尺寸（dim），会添加尺寸后缀避免命名冲突。

### 字体资源合并

相同字体文件 + 像素大小 + 位深度的配置会合并，字符集取并集。内联文本字符会生成 `_generated_text_*.txt` 文件。

## 用户代码保留

`user_code_preserver.py` 实现了 USER CODE 区域的保留机制：

```c
// USER CODE BEGIN user_fields
int my_counter;          // 用户添加的字段
egui_timer_t my_timer;   // 用户添加的定时器
// USER CODE END user_fields
```

当 `.h` 文件重新生成时，`preserve_user_code()` 函数：

1. 读取旧文件中所有 `USER CODE BEGIN xxx` ... `USER CODE END xxx` 区域
2. 在新生成的内容中找到对应区域
3. 用旧内容替换新内容中的空区域

## 生成代码示例（XML -> C 对照）

### 输入 XML

```xml
<?xml version="1.0" encoding="utf-8"?>
<Page>
    <Group id="root" x="0" y="0" width="320" height="240">
        <Background type="solid" color="EGUI_COLOR_HEX(0x060608)" alpha="EGUI_ALPHA_100" />
        <Label id="title" x="16" y="8" width="200" height="23"
               text="Dashboard"
               font_builtin="&amp;egui_res_font_montserrat_18_4"
               color="EGUI_COLOR_WHITE" alpha="EGUI_ALPHA_100"
               align_type="EGUI_ALIGN_LEFT | EGUI_ALIGN_VCENTER" />
        <ProgressBar id="soc_bar" x="16" y="40" width="200" height="8" value="75" />
        <Switch id="power_switch" x="250" y="8" width="48" height="24"
                is_checked="true" onCheckedChanged="on_power_changed" />
    </Group>
</Page>
```

### 生成的 .h 文件

```c
#ifndef _DASHBOARD_H_
#define _DASHBOARD_H_

#include "egui.h"

typedef struct egui_dashboard egui_dashboard_t;
struct egui_dashboard
{
    egui_page_base_t base;

    // UI widgets (auto-generated, do not edit)
    egui_view_group_t root;
    egui_view_label_t title;
    egui_view_progress_bar_t soc_bar;
    egui_view_switch_t power_switch;

    // USER CODE BEGIN user_fields
    // USER CODE END user_fields
};

void egui_dashboard_layout_init(egui_page_base_t *self);
void egui_dashboard_init(egui_page_base_t *self);

#endif /* _DASHBOARD_H_ */
```

### 生成的 _layout.c 文件

```c
// ===== Auto-generated by EmbeddedGUI Designer =====
#include "egui.h"
#include <stdlib.h>
#include "uicode.h"
#include "dashboard.h"

// Forward declarations for event callbacks
extern void on_power_changed(egui_view_t *self, int is_checked);

// Background declarations
static egui_background_color_t bg_dashboard_root;
EGUI_BACKGROUND_COLOR_PARAM_INIT_SOLID(bg_dashboard_root_param_normal,
    EGUI_COLOR_HEX(0x060608), EGUI_ALPHA_100);
EGUI_BACKGROUND_PARAM_INIT(bg_dashboard_root_params,
    &bg_dashboard_root_param_normal, NULL, NULL);

void egui_dashboard_layout_init(egui_page_base_t *self)
{
    egui_dashboard_t *local = (egui_dashboard_t *)self;

    // Init views
    // root (group)
    egui_view_group_init((egui_view_t *)&local->root);
    egui_view_set_position((egui_view_t *)&local->root, 0, 0);
    egui_view_set_size((egui_view_t *)&local->root, 320, 240);

    // title (label)
    egui_view_label_init((egui_view_t *)&local->title);
    egui_view_set_position((egui_view_t *)&local->title, 16, 8);
    egui_view_set_size((egui_view_t *)&local->title, 200, 23);
    egui_view_label_set_text((egui_view_t *)&local->title, "Dashboard");
    egui_view_label_set_font_color((egui_view_t *)&local->title,
        EGUI_COLOR_WHITE, EGUI_ALPHA_100);
    egui_view_label_set_align_type((egui_view_t *)&local->title,
        EGUI_ALIGN_LEFT | EGUI_ALIGN_VCENTER);

    // soc_bar (progress_bar)
    egui_view_progress_bar_init((egui_view_t *)&local->soc_bar);
    egui_view_set_position((egui_view_t *)&local->soc_bar, 16, 40);
    egui_view_set_size((egui_view_t *)&local->soc_bar, 200, 8);
    egui_view_progress_bar_set_process((egui_view_t *)&local->soc_bar, 75);

    // power_switch (switch)
    egui_view_switch_init((egui_view_t *)&local->power_switch);
    egui_view_set_position((egui_view_t *)&local->power_switch, 250, 8);
    egui_view_set_size((egui_view_t *)&local->power_switch, 48, 24);
    egui_view_switch_set_checked((egui_view_t *)&local->power_switch, 1);
    egui_view_switch_set_on_checked_listener(
        (egui_view_t *)&local->power_switch, on_power_changed);

    // Set backgrounds
    egui_background_color_init_with_params(
        (egui_background_t *)&bg_dashboard_root, &bg_dashboard_root_params);
    egui_view_set_background(
        (egui_view_t *)&local->root, (egui_background_t *)&bg_dashboard_root);

    // Build hierarchy
    egui_view_group_add_child(
        (egui_view_t *)&local->root, (egui_view_t *)&local->title);
    egui_view_group_add_child(
        (egui_view_t *)&local->root, (egui_view_t *)&local->soc_bar);
    egui_view_group_add_child(
        (egui_view_t *)&local->root, (egui_view_t *)&local->power_switch);

    // Add to page root
    egui_page_base_add_view(self, (egui_view_t *)&local->root);
}
```

### 生成的用户 .c 文件（骨架）

```c
// User implementation for dashboard page
#include "egui.h"
#include "uicode.h"
#include "dashboard.h"

// USER CODE BEGIN includes
// USER CODE END includes

static void on_power_changed(egui_view_t *self, int is_checked)
{
    // USER CODE BEGIN on_power_changed
    // USER CODE END on_power_changed
}

void egui_dashboard_init(egui_page_base_t *self)
{
    egui_dashboard_t *local = (egui_dashboard_t *)self;

    // Call auto-generated layout init
    egui_dashboard_layout_init(self);

    // USER CODE BEGIN init
    // USER CODE END init
}
```

## 背景生成规则

背景类型与宏的映射：

| bg_type | 宏后缀 | 参数 |
|---------|--------|------|
| `solid` | `SOLID` | color, alpha |
| `round_rectangle` | `ROUND_RECTANGLE` | color, alpha, radius |
| `round_rectangle_corners` | `ROUND_RECTANGLE_CORNERS` | color, alpha, 4个圆角 |
| `circle` | `CIRCLE` | color, alpha, radius |
| `gradient` | (专用宏) | direction, start_color, end_color, alpha |

带描边时宏后缀追加 `_STROKE`，额外参数：stroke_width, stroke_color, stroke_alpha。

支持三态背景：normal（必选）、pressed（可选）、disabled（可选）。

## 动画生成规则

支持的动画类型：

| 类型 | C 类型 | 参数 |
|------|--------|------|
| `alpha` | `egui_animation_alpha_t` | from_alpha, to_alpha |
| `translate` | `egui_animation_translate_t` | from_x, to_x, from_y, to_y |
| `scale` | `egui_animation_scale_size_t` | from_scale, to_scale |
| `resize` | `egui_animation_resize_t` | from_w_ratio, to_w_ratio, from_h_ratio, to_h_ratio, mode |
| `color` | `egui_animation_color_t` | from_color, to_color |

支持的插值器：linear, accelerate, decelerate, accelerate_decelerate, anticipate, overshoot, anticipate_overshoot, bounce, cycle。
