# HelloCustomWidgets Implementation Plan

> **For Claude:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task.

**Goal:** Create `example/HelloCustomWidgets/` 框架，支持按分类组织 1000+ 自定义控件，每个控件可通过 `APP_SUB=category/widget_name` 独立编译运行。

**Architecture:** 在现有 HelloBasic 的 `APP_SUB` 机制基础上扩展为两级寻址（`category/widget_name`）。每个控件包含 .c/.h 实现和 test.c 示例。构建系统（Make + CMake）、编译检查脚本、WASM 构建脚本同步适配。

**Tech Stack:** C99, GNU Make, CMake 3.10+, Python 3, GitHub Actions

---

### Task 1: 创建 HelloCustomWidgets 基础目录和公共文件

**Files:**
- Create: `example/HelloCustomWidgets/build.mk`
- Create: `example/HelloCustomWidgets/app_egui_config.h`
- Create: `example/HelloCustomWidgets/uicode.h`
- Create: `example/HelloCustomWidgets/uicode.c`

**Step 1: 创建 app_egui_config.h**

```c
// example/HelloCustomWidgets/app_egui_config.h
#ifndef _APP_EGUI_CONFIG_H_
#define _APP_EGUI_CONFIG_H_

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif
/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _APP_EGUI_CONFIG_H_ */
```

使用默认屏幕配置（240x320），与 HelloBasic 保持一致。

**Step 2: 创建 uicode.h**

```c
// example/HelloCustomWidgets/uicode.h
#ifndef _UICODE_H_
#define _UICODE_H_

#include "egui.h"

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

void uicode_create_ui(void);

extern void test_init_ui(void);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _UICODE_H_ */
```

与 HelloBasic 的 `uicode.h` 完全相同，每个子控件的 `test.c` 实现 `test_init_ui()`。

**Step 3: 创建 uicode.c**

```c
// example/HelloCustomWidgets/uicode.c
#include "egui.h"
#include <stdlib.h>
#include "uicode.h"

void uicode_create_ui(void)
{
    test_init_ui();
}
```

**Step 4: 创建 build.mk（核心：两级 APP_SUB 支持）**

```makefile
# example/HelloCustomWidgets/build.mk
# HelloCustomWidgets: 支持两级 APP_SUB（category/widget_name）
#
# 用法：make all APP=HelloCustomWidgets APP_SUB=input/color_picker PORT=pc

EGUI_CODE_SRC		+= $(EGUI_APP_PATH)

EGUI_CODE_INCLUDE	+= $(EGUI_APP_PATH)


# select the sub app
APP_SUB ?= display/sample_widget

EGUI_APP_SUB_PATH := $(EGUI_APP_PATH)/$(APP_SUB)

# Each sub-app has its own obj directory (/ replaced with _)
APP_OBJ_SUFFIX := HelloCustomWidgets_$(subst /,_,$(APP_SUB))

EGUI_CODE_SRC		+= $(EGUI_APP_SUB_PATH)
EGUI_CODE_SRC		+= $(EGUI_APP_SUB_PATH)/resource
EGUI_CODE_SRC		+= $(EGUI_APP_SUB_PATH)/resource/img
EGUI_CODE_SRC		+= $(EGUI_APP_SUB_PATH)/resource/font

EGUI_CODE_INCLUDE	+= $(EGUI_APP_SUB_PATH)
EGUI_CODE_INCLUDE	+= $(EGUI_APP_SUB_PATH)/resource

EGUI_APP_RESOURCE_PATH ?= $(EGUI_APP_SUB_PATH)/resource
```

关键点：`APP_OBJ_SUFFIX` 使用 `subst /,_` 将斜杠替换为下划线，保证每个控件独立编译目录。

**Step 5: 编译验证空框架**

Run: `make all APP=HelloCustomWidgets APP_SUB=display/sample_widget PORT=pc`
Expected: 此时会失败，因为 `display/sample_widget/` 目录还不存在。这是预期行为，Task 2 将创建示例控件。

**Step 6: Commit**

```bash
git add example/HelloCustomWidgets/build.mk \
        example/HelloCustomWidgets/app_egui_config.h \
        example/HelloCustomWidgets/uicode.h \
        example/HelloCustomWidgets/uicode.c
git commit -m "feat: add HelloCustomWidgets skeleton with two-level APP_SUB support"
```

---

### Task 2: 创建示例控件 display/sample_widget 验证构建

**Files:**
- Create: `example/HelloCustomWidgets/display/sample_widget/egui_view_sample_widget.h`
- Create: `example/HelloCustomWidgets/display/sample_widget/egui_view_sample_widget.c`
- Create: `example/HelloCustomWidgets/display/sample_widget/test.c`

**Step 1: 创建控件头文件**

```c
// example/HelloCustomWidgets/display/sample_widget/egui_view_sample_widget.h
#ifndef _EGUI_VIEW_SAMPLE_WIDGET_H_
#define _EGUI_VIEW_SAMPLE_WIDGET_H_

#include "egui.h"

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

typedef struct egui_view_sample_widget egui_view_sample_widget_t;
struct egui_view_sample_widget
{
    egui_view_label_t base;
    egui_color_int_t border_color;
    uint8_t border_width;
};

void egui_view_sample_widget_set_border(egui_view_t *self, egui_color_int_t color, uint8_t width);
void egui_view_sample_widget_init(egui_view_t *self);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _EGUI_VIEW_SAMPLE_WIDGET_H_ */
```

这是一个最简示例控件：带边框的 label，用于验证构建流程。

**Step 2: 创建控件实现**

```c
// example/HelloCustomWidgets/display/sample_widget/egui_view_sample_widget.c
#include <stdlib.h>
#include "egui_view_sample_widget.h"

static void egui_view_sample_widget_on_draw(egui_view_t *self, egui_canvas_t *canvas)
{
    EGUI_LOCAL_INIT(egui_view_sample_widget_t);

    // Draw border
    if (local->border_width > 0)
    {
        egui_dim_t x = self->area.location.x;
        egui_dim_t y = self->area.location.y;
        egui_dim_t w = self->area.size.width;
        egui_dim_t h = self->area.size.height;

        for (uint8_t i = 0; i < local->border_width; i++)
        {
            egui_canvas_draw_rectangle(canvas, x + i, y + i, w - 2 * i, h - 2 * i, local->border_color, EGUI_ALPHA_100);
        }
    }

    // Draw base label
    egui_view_label_on_draw(self, canvas);
}

void egui_view_sample_widget_set_border(egui_view_t *self, egui_color_int_t color, uint8_t width)
{
    EGUI_LOCAL_INIT(egui_view_sample_widget_t);
    local->border_color = color;
    local->border_width = width;
    egui_view_invalidate(self);
}

static const egui_view_api_t egui_view_sample_widget_api = {
    .draw = egui_view_sample_widget_on_draw,
    .layout = NULL,
};

void egui_view_sample_widget_init(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_sample_widget_t);

    // Init base label
    egui_view_label_init(self);

    // Override draw API
    self->api = &egui_view_sample_widget_api;

    // Default border
    local->border_color = EGUI_COLOR_WHITE;
    local->border_width = 2;
}
```

**Step 3: 创建示例代码**

```c
// example/HelloCustomWidgets/display/sample_widget/test.c
#include "egui.h"
#include <stdlib.h>
#include "uicode.h"
#include "egui_view_sample_widget.h"

static egui_view_sample_widget_t sample;

static char sample_str[] = "Sample Widget";

void test_init_ui(void)
{
    egui_view_sample_widget_init(EGUI_VIEW_OF(&sample));
    egui_view_set_size(EGUI_VIEW_OF(&sample), 160, 40);
    egui_view_label_set_text(EGUI_VIEW_OF(&sample), sample_str);
    egui_view_sample_widget_set_border(EGUI_VIEW_OF(&sample), EGUI_COLOR_GREEN, 2);

    egui_core_add_user_root_view(EGUI_VIEW_OF(&sample));
    egui_core_layout_childs_user_root_view(EGUI_LAYOUT_VERTICAL, EGUI_ALIGN_HCENTER | EGUI_ALIGN_VCENTER);
}

#if EGUI_CONFIG_RECORDING_TEST
bool egui_port_get_recording_action(int action_index, egui_sim_action_t *p_action)
{
    return false;
}
#endif
```

**Step 4: 编译验证**

Run: `make all APP=HelloCustomWidgets APP_SUB=display/sample_widget PORT=pc`
Expected: BUILD SUCCESS

**Step 5: 运行时验证**

Run: `python scripts/code_runtime_check.py --app HelloCustomWidgets --app-sub display/sample_widget --timeout 10`
Expected: 截图中显示绿色边框的 "Sample Widget" 标签居中显示

**Step 6: Commit**

```bash
git add example/HelloCustomWidgets/display/sample_widget/
git commit -m "feat: add sample_widget to verify HelloCustomWidgets build pipeline"
```

---

### Task 3: 创建第二个分类 input/ 的示例控件，验证多分类

**Files:**
- Create: `example/HelloCustomWidgets/input/sample_input/egui_view_sample_input.h`
- Create: `example/HelloCustomWidgets/input/sample_input/egui_view_sample_input.c`
- Create: `example/HelloCustomWidgets/input/sample_input/test.c`

**Step 1: 创建 input/sample_input 控件**

这是一个简单的带标签的滑动条组合控件，用于验证另一个分类也能正常构建。

```c
// example/HelloCustomWidgets/input/sample_input/egui_view_sample_input.h
#ifndef _EGUI_VIEW_SAMPLE_INPUT_H_
#define _EGUI_VIEW_SAMPLE_INPUT_H_

#include "egui.h"

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

typedef struct egui_view_sample_input egui_view_sample_input_t;
struct egui_view_sample_input
{
    egui_view_linearlayout_t base;
    egui_view_label_t label;
    egui_view_slider_t slider;
};

void egui_view_sample_input_init(egui_view_t *self, egui_dim_t x, egui_dim_t y, egui_dim_t w, egui_dim_t h);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _EGUI_VIEW_SAMPLE_INPUT_H_ */
```

```c
// example/HelloCustomWidgets/input/sample_input/egui_view_sample_input.c
#include <stdlib.h>
#include "egui_view_sample_input.h"

void egui_view_sample_input_init(egui_view_t *self, egui_dim_t x, egui_dim_t y, egui_dim_t w, egui_dim_t h)
{
    EGUI_LOCAL_INIT(egui_view_sample_input_t);

    // Init container
    egui_view_linearlayout_init(self);
    egui_view_set_position(self, x, y);
    egui_view_set_size(self, w, h);
    egui_view_linearlayout_set_orientation(self, EGUI_LAYOUT_VERTICAL);
    egui_view_linearlayout_set_align_type(self, EGUI_ALIGN_HCENTER);

    // Init label
    egui_view_label_init(EGUI_VIEW_OF(&local->label));
    egui_view_set_size(EGUI_VIEW_OF(&local->label), w, 20);

    // Init slider
    egui_view_slider_init(EGUI_VIEW_OF(&local->slider));
    egui_view_set_size(EGUI_VIEW_OF(&local->slider), w - 20, 20);

    // Add children
    egui_view_group_add_child(self, EGUI_VIEW_OF(&local->label));
    egui_view_group_add_child(self, EGUI_VIEW_OF(&local->slider));

    egui_view_linearlayout_layout_childs(self);
}
```

```c
// example/HelloCustomWidgets/input/sample_input/test.c
#include "egui.h"
#include <stdlib.h>
#include "uicode.h"
#include "egui_view_sample_input.h"

static egui_view_sample_input_t sample_input;
static char label_str[] = "Sample Input";

void test_init_ui(void)
{
    egui_view_sample_input_init(EGUI_VIEW_OF(&sample_input), 0, 0, 200, 60);
    egui_view_label_set_text(EGUI_VIEW_OF(&sample_input.label), label_str);

    egui_core_add_user_root_view(EGUI_VIEW_OF(&sample_input));
    egui_core_layout_childs_user_root_view(EGUI_LAYOUT_VERTICAL, EGUI_ALIGN_HCENTER | EGUI_ALIGN_VCENTER);
}

#if EGUI_CONFIG_RECORDING_TEST
bool egui_port_get_recording_action(int action_index, egui_sim_action_t *p_action)
{
    return false;
}
#endif
```

**Step 2: 编译验证**

Run: `make all APP=HelloCustomWidgets APP_SUB=input/sample_input PORT=pc`
Expected: BUILD SUCCESS

**Step 3: 验证两个分类互不干扰**

Run: `make all APP=HelloCustomWidgets APP_SUB=display/sample_widget PORT=pc`
Expected: BUILD SUCCESS（证明两个分类的 OBJDIR 独立）

**Step 4: 运行时验证**

Run: `python scripts/code_runtime_check.py --app HelloCustomWidgets --app-sub input/sample_input --timeout 10`
Expected: 截图中显示标签和滑动条

**Step 5: Commit**

```bash
git add example/HelloCustomWidgets/input/sample_input/
git commit -m "feat: add sample_input widget to verify multi-category build"
```

---

### Task 4: 适配 CMakeLists.txt

**Files:**
- Create: `example/HelloCustomWidgets/CMakeLists.txt`
- Modify: `d:/workspace/gitee/EmbeddedGUI/CMakeLists.txt:31-42`

**Step 1: 创建 example/HelloCustomWidgets/CMakeLists.txt**

```cmake
# HelloCustomWidgets: uses two-level APP_SUB (category/widget_name)
# Usage: cmake -DAPP=HelloCustomWidgets -DAPP_SUB=display/sample_widget ..

# Common source files (uicode.c)
file(GLOB HELLO_CUSTOM_SRC *.c)

# Sub-app source files
if(NOT APP_SUB STREQUAL "")
    file(GLOB APP_SUB_SRC ${APP_SUB}/*.c)
    file(GLOB_RECURSE APP_SUB_RESOURCE_SRC ${APP_SUB}/resource/*.c)
    list(APPEND HELLO_CUSTOM_SRC ${APP_SUB_SRC} ${APP_SUB_RESOURCE_SRC})
endif()

add_library(example_lib ${HELLO_CUSTOM_SRC})
```

**Step 2: 修改根 CMakeLists.txt 支持两级 APP_SUB include**

在 `CMakeLists.txt` 第 33-42 行，当前只处理 `HelloBasic`，需要同时处理 `HelloCustomWidgets`：

将第 33 行的条件：
```cmake
if(APP STREQUAL "HelloBasic" AND NOT APP_SUB STREQUAL "")
```

改为同时匹配 HelloCustomWidgets：
```cmake
if((APP STREQUAL "HelloBasic" OR APP STREQUAL "HelloCustomWidgets") AND NOT APP_SUB STREQUAL "")
```

**Step 3: CMake 编译验证**

Run: `cmake -B build_cmake/HelloCustomWidgets_display_sample_widget -DAPP=HelloCustomWidgets -DAPP_SUB=display/sample_widget -DPORT=pc -G "MinGW Makefiles" && cmake --build build_cmake/HelloCustomWidgets_display_sample_widget -j`
Expected: BUILD SUCCESS

**Step 4: Commit**

```bash
git add example/HelloCustomWidgets/CMakeLists.txt CMakeLists.txt
git commit -m "feat: add CMake support for HelloCustomWidgets two-level APP_SUB"
```

---

### Task 5: 适配 code_compile_check.py 支持自定义控件检查

**Files:**
- Modify: `scripts/code_compile_check.py`

**Step 1: 添加自定义控件发现函数**

在 `get_example_basic_list()` 函数（第 28-39 行）之后，添加：

```python
def get_custom_widgets_list(category=None):
    """Discover HelloCustomWidgets sub-apps (category/widget_name pairs)."""
    base = 'example/HelloCustomWidgets'
    if not os.path.isdir(base):
        return []

    result = []
    categories = os.listdir(base)
    for cat in sorted(categories):
        cat_path = os.path.join(base, cat)
        if not os.path.isdir(cat_path) or cat.startswith('.'):
            continue
        # Skip non-category entries (files like uicode.c, build.mk)
        if not any(os.path.isdir(os.path.join(cat_path, w)) for w in os.listdir(cat_path)):
            continue
        if category and cat != category:
            continue
        for widget in sorted(os.listdir(cat_path)):
            widget_path = os.path.join(cat_path, widget)
            if os.path.isdir(widget_path) and os.path.exists(os.path.join(widget_path, 'test.c')):
                result.append(f"{cat}/{widget}")
    return result
```

**Step 2: 添加命令行参数**

在 `parse_args()` 函数中（第 176-203 行），在 `--cmake` 参数之后添加：

```python
    parser.add_argument("--custom-widgets",
                        action="store_true",
                        default=False,
                        help="Check HelloCustomWidgets instead of standard apps.")

    parser.add_argument("--category",
                        type=str,
                        default=None,
                        help="Only check specific category (e.g. input, display).")
```

**Step 3: 在 main 逻辑中添加自定义控件检查分支**

在第 256 行 `full_check = args.full_check` 之后、full_check 分支之前，添加：

```python
    # Custom widgets check mode
    if args.custom_widgets:
        custom_list = get_custom_widgets_list(args.category)
        total_work_cnt = len(custom_list)
        current_work_cnt = 0
        for widget_sub in custom_list:
            current_work_cnt += 1
            process_app(current_work_cnt, total_work_cnt, "HelloCustomWidgets", "pc", widget_sub, params)

        elapsed = time.time() - start_time
        print("=================================================================================")
        print("Custom widgets check passed! Time: %.1fs" % elapsed)
        print("=================================================================================")
        sys.exit(0)
```

**Step 4: 在 full_check 循环中跳过 HelloCustomWidgets**

在第 268 行的 `for app in app_sets:` 循环中，`if app == "HelloBasic":` 之前添加：

```python
                if app == "HelloCustomWidgets":
                    # Custom widgets checked separately via --custom-widgets
                    continue
```

**Step 5: 验证**

Run: `python scripts/code_compile_check.py --custom-widgets`
Expected: 编译 display/sample_widget 和 input/sample_input 两个控件，全部通过

Run: `python scripts/code_compile_check.py --custom-widgets --category display`
Expected: 只编译 display 分类下的控件

**Step 6: Commit**

```bash
git add scripts/code_compile_check.py
git commit -m "feat: add --custom-widgets and --category flags to compile check"
```

---

### Task 6: 适配 wasm_build_demos.py 支持自定义控件

**Files:**
- Modify: `scripts/wasm_build_demos.py`

**Step 1: 添加自定义控件发现函数**

在 `get_example_basic_list()` 函数（第 34-38 行）之后，添加：

```python
def get_custom_widgets_list():
    """Discover HelloCustomWidgets sub-apps."""
    base = 'example/HelloCustomWidgets'
    if not os.path.isdir(base):
        return []

    result = []
    for cat in sorted(os.listdir(base)):
        cat_path = os.path.join(base, cat)
        if not os.path.isdir(cat_path):
            continue
        for widget in sorted(os.listdir(cat_path)):
            widget_path = os.path.join(cat_path, widget)
            if os.path.isdir(widget_path) and os.path.exists(os.path.join(widget_path, 'test.c')):
                result.append((cat, widget))
    return result
```

**Step 2: 在构建列表生成中处理 HelloCustomWidgets**

在第 192-199 行的 `for app in app_sets:` 循环中，`if app == "HelloBasic":` 分支之后、`else:` 之前，添加对 HelloCustomWidgets 的处理：

```python
            elif app == "HelloCustomWidgets":
                for cat, widget in get_custom_widgets_list():
                    build_list.append((app, f"{cat}/{widget}", "HelloCustomWidgets"))
```

**Step 3: 将 HelloCustomWidgets 视为共享 OBJDIR 组（与 HelloBasic 类似策略）**

在第 205-206 行的分组逻辑中，将 HelloCustomWidgets 也作为顺序构建组：

```python
    basic_group = [(a, s, c) for a, s, c in build_list if c == "HelloBasic"]
    custom_group = [(a, s, c) for a, s, c in build_list if c == "HelloCustomWidgets"]
    standalone_list = [(a, s, c) for a, s, c in build_list if c not in ("HelloBasic", "HelloCustomWidgets")]
```

在 basic_group 处理完成后（第 226 行之后），添加 custom_group 的处理（复制 basic_group 的处理逻辑）：

```python
    # Build HelloCustomWidgets sub-apps sequentially
    if custom_group:
        print(f"\n--- HelloCustomWidgets ({len(custom_group)} demos, sequential) ---")
        for app, sub, category in custom_group:
            count += 1
            name = f"{app}_{sub.replace('/', '_')}" if sub else app
            print(f"\n[{count}/{total}] {name}")
            result = build_demo(root_dir, app, sub, args.emsdk_path, output_dir)
            result["category"] = category
            if "error" in result:
                print(f"  FAILED: {result['error']}")
                failed.append(result["name"])
            else:
                print(f"  OK -> {os.path.join(output_dir, result['name'])}")
                demos_built.append(make_demo_entry(root_dir, result, category))
```

**Step 4: 修改 build_demo 中的 demo_name 生成**

在 `build_demo()` 函数（第 91 行），当 `app_sub` 包含 `/` 时，demo_name 需要用 `_` 替代：

将第 97 行：
```python
        demo_name = f"{app}_{app_sub}"
```
改为：
```python
        demo_name = f"{app}_{app_sub.replace('/', '_')}"
```

**Step 5: 验证（无需实际 WASM 构建，只验证发现逻辑）**

Run: `python -c "import sys; sys.path.insert(0, 'scripts'); exec(open('scripts/wasm_build_demos.py').read().split('def main')[0]); print(get_custom_widgets_list())"`
Expected: `[('display', 'sample_widget'), ('input', 'sample_input')]`

**Step 6: Commit**

```bash
git add scripts/wasm_build_demos.py
git commit -m "feat: add HelloCustomWidgets support to WASM build script"
```

---

### Task 7: 添加 GitHub Actions 自定义控件检查 Workflow

**Files:**
- Create: `.github/workflows/custom-widgets-check.yml`

**Step 1: 创建 workflow 文件**

```yaml
# .github/workflows/custom-widgets-check.yml
name: Custom Widgets Check

on:
  # Trigger on push when custom widgets or core widgets change
  push:
    paths:
      - 'example/HelloCustomWidgets/**'
      - 'src/widget/**'
      - 'src/core/**'

  # Weekly full check
  schedule:
    - cron: '0 2 * * 1'  # Every Monday at 02:00 UTC

  # Manual trigger with optional category filter
  workflow_dispatch:
    inputs:
      category:
        description: 'Widget category to check (empty for all)'
        required: false
        type: string

jobs:
  check:
    runs-on: windows-latest
    strategy:
      fail-fast: false
      matrix:
        category: [input, display, layout, chart, navigation, feedback, media, decoration]
    steps:
      - uses: actions/checkout@v4

      - uses: rlalik/setup-cpp-compiler@v1.2

      - name: Setup Python
        uses: actions/setup-python@v5
        with:
          python-version: '3.11'

      - name: Check custom widgets (${{ matrix.category }})
        run: python scripts/code_compile_check.py --custom-widgets --category ${{ matrix.category }} --bits64
```

**Step 2: Commit**

```bash
git add .github/workflows/custom-widgets-check.yml
git commit -m "ci: add custom widgets check workflow with category matrix"
```

---

### Task 8: 创建分类目录占位和控件模板脚本

**Files:**
- Create: `scripts/create_custom_widget.py`

**Step 1: 创建控件脚手架脚本**

```python
#!/usr/bin/env python3
"""Create a new custom widget from template.

Usage:
    python scripts/create_custom_widget.py --category display --name weather_icon
"""

import argparse
import os
import sys

VALID_CATEGORIES = [
    "input", "display", "layout", "chart",
    "navigation", "feedback", "media", "decoration"
]

HEADER_TEMPLATE = '''#ifndef _EGUI_VIEW_{NAME_UPPER}_H_
#define _EGUI_VIEW_{NAME_UPPER}_H_

#include "egui.h"

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {{
#endif

typedef struct egui_view_{name} egui_view_{name}_t;
struct egui_view_{name}
{{
    egui_view_t base;
    // TODO: add widget fields
}};

void egui_view_{name}_init(egui_view_t *self);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}}
#endif

#endif /* _EGUI_VIEW_{NAME_UPPER}_H_ */
'''

SOURCE_TEMPLATE = '''#include <stdlib.h>
#include "egui_view_{name}.h"

void egui_view_{name}_init(egui_view_t *self)
{{
    EGUI_LOCAL_INIT(egui_view_{name}_t);

    // Init base view
    egui_view_init(self);

    // TODO: initialize widget
}}
'''

TEST_TEMPLATE = '''#include "egui.h"
#include <stdlib.h>
#include "uicode.h"
#include "egui_view_{name}.h"

static egui_view_{name}_t widget;

void test_init_ui(void)
{{
    egui_view_{name}_init(EGUI_VIEW_OF(&widget));
    egui_view_set_size(EGUI_VIEW_OF(&widget), 160, 40);

    egui_core_add_user_root_view(EGUI_VIEW_OF(&widget));
    egui_core_layout_childs_user_root_view(EGUI_LAYOUT_VERTICAL, EGUI_ALIGN_HCENTER | EGUI_ALIGN_VCENTER);
}}

#if EGUI_CONFIG_RECORDING_TEST
bool egui_port_get_recording_action(int action_index, egui_sim_action_t *p_action)
{{
    return false;
}}
#endif
'''


def main():
    parser = argparse.ArgumentParser(description="Create a new custom widget")
    parser.add_argument("--category", required=True, choices=VALID_CATEGORIES,
                        help="Widget category")
    parser.add_argument("--name", required=True,
                        help="Widget name (snake_case, e.g. weather_icon)")
    args = parser.parse_args()

    name = args.name
    name_upper = name.upper()
    category = args.category

    widget_dir = os.path.join("example", "HelloCustomWidgets", category, name)

    if os.path.exists(widget_dir):
        print(f"Error: {widget_dir} already exists!")
        return 1

    os.makedirs(widget_dir, exist_ok=True)

    # Write files
    with open(os.path.join(widget_dir, f"egui_view_{name}.h"), "w", encoding="utf-8") as f:
        f.write(HEADER_TEMPLATE.format(name=name, NAME_UPPER=name_upper))

    with open(os.path.join(widget_dir, f"egui_view_{name}.c"), "w", encoding="utf-8") as f:
        f.write(SOURCE_TEMPLATE.format(name=name))

    with open(os.path.join(widget_dir, "test.c"), "w", encoding="utf-8") as f:
        f.write(TEST_TEMPLATE.format(name=name))

    print(f"Created widget: {widget_dir}/")
    print(f"  egui_view_{name}.h")
    print(f"  egui_view_{name}.c")
    print(f"  test.c")
    print(f"\nBuild: make all APP=HelloCustomWidgets APP_SUB={category}/{name} PORT=pc")
    return 0


if __name__ == "__main__":
    sys.exit(main())
```

**Step 2: 验证脚手架脚本**

Run: `python scripts/create_custom_widget.py --category chart --name sample_chart`
Expected: 在 `example/HelloCustomWidgets/chart/sample_chart/` 下生成 3 个文件

Run: `make all APP=HelloCustomWidgets APP_SUB=chart/sample_chart PORT=pc`
Expected: BUILD SUCCESS

**Step 3: Commit**

```bash
git add scripts/create_custom_widget.py example/HelloCustomWidgets/chart/sample_chart/
git commit -m "feat: add create_custom_widget.py scaffold script and chart/sample_chart"
```

---

### Task 9: 最终全量验证

**Step 1: 全量编译检查（核心不受影响）**

Run: `python scripts/code_compile_check.py --full-check`
Expected: 所有现有示例编译通过，HelloCustomWidgets 被跳过

**Step 2: 自定义控件编译检查**

Run: `python scripts/code_compile_check.py --custom-widgets`
Expected: 3 个控件（display/sample_widget, input/sample_input, chart/sample_chart）全部编译通过

**Step 3: CMake 验证**

Run: `cmake -B build_cmake/HelloCustomWidgets_input_sample_input -DAPP=HelloCustomWidgets -DAPP_SUB=input/sample_input -DPORT=pc -G "MinGW Makefiles" && cmake --build build_cmake/HelloCustomWidgets_input_sample_input -j`
Expected: BUILD SUCCESS

**Step 4: 运行时验证所有示例控件**

Run: `python scripts/code_runtime_check.py --app HelloCustomWidgets --app-sub display/sample_widget --timeout 10`
Run: `python scripts/code_runtime_check.py --app HelloCustomWidgets --app-sub input/sample_input --timeout 10`
Run: `python scripts/code_runtime_check.py --app HelloCustomWidgets --app-sub chart/sample_chart --timeout 10`
Expected: 全部通过，截图正确

**Step 5: Commit（如有修正）**

```bash
git add -A
git commit -m "fix: address issues found during final verification"
```

---

## 附录：后续扩展指南

### 添加新控件（日常操作）

```bash
# 1. 用脚手架生成
python scripts/create_custom_widget.py --category display --name weather_icon

# 2. 编辑生成的文件，实现控件逻辑

# 3. 编译验证
make all APP=HelloCustomWidgets APP_SUB=display/weather_icon PORT=pc

# 4. 运行时验证
python scripts/code_runtime_check.py --app HelloCustomWidgets --app-sub display/weather_icon --timeout 10
```

### 分类说明

| 分类 | 说明 | 适合放置 |
|------|------|----------|
| input | 用户输入 | color_picker, date_picker, search_bar |
| display | 数据展示 | stat_card, badge, weather_icon |
| layout | 容器布局 | waterfall, flex, accordion |
| chart | 图表 | radar, treemap, heatmap |
| navigation | 导航 | breadcrumb, sidebar, drawer |
| feedback | 反馈提示 | skeleton, alert, snackbar |
| media | 媒体 | audio_player, carousel |
| decoration | 装饰 | gradient_divider, shadow_box |
