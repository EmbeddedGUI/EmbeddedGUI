# HelloCustomWidgets 设计方案

## 背景

项目当前有 ~63 个核心控件（`src/widget/`），计划扩展至 1000+ 自定义控件。核心控件保持在 `src/widget/` 不变，新增控件统一放在 `example/HelloCustomWidgets/` 下，用户按需取用。

## 控件定位

- 既有独立新控件（如日历、颜色选择器），也有现有核心控件的增强变体（如圆角按钮、渐变进度条）
- 所有自定义控件依赖 `src/widget/` 的核心控件，通过组合/继承实现

## 目录结构

```
example/HelloCustomWidgets/
├── build.mk                        # 构建入口，解析两级 APP_SUB
├── app_egui_config.h               # 统一屏幕配置
├── input/                          # 输入类控件
│   ├── color_picker/
│   │   ├── egui_view_color_picker.c    # 控件实现
│   │   ├── egui_view_color_picker.h    # 控件头文件
│   │   ├── test.c                      # 示例/演示代码
│   │   ├── widget_register.py          # 可选：UI Designer 注册
│   │   └── resource/src/              # 可选：控件专用资源
│   ├── date_picker/
│   └── ...
├── display/                        # 展示类控件
│   ├── circular_gauge/
│   ├── weather_card/
│   └── ...
├── layout/                         # 布局类控件
├── chart/                          # 图表类控件
├── navigation/                     # 导航类控件
├── feedback/                       # 反馈类（toast 变体、弹窗等）
├── media/                          # 媒体类
└── decoration/                     # 装饰类（分割线变体、边框等）
```

### 每个控件的标准文件结构

```
{category}/{widget_name}/
├── egui_view_{widget_name}.c       # 必须：控件实现
├── egui_view_{widget_name}.h       # 必须：控件头文件
├── test.c                          # 必须：可独立编译运行的示例
├── widget_register.py              # 可选：UI Designer 注册文件
└── resource/src/                   # 可选：控件专用图片/字体资源
```

### 构建命令

```bash
# 编译单个控件示例
make all APP=HelloCustomWidgets APP_SUB=input/color_picker PORT=pc

# 运行时验证
python scripts/code_runtime_check.py --app HelloCustomWidgets --app-sub input/color_picker

# CMake 构建
cmake -B build_cmake/HelloCustomWidgets_input_color_picker \
  -DAPP=HelloCustomWidgets -DAPP_SUB=input/color_picker -DPORT=pc -G "MinGW Makefiles"
cmake --build build_cmake/HelloCustomWidgets_input_color_picker -j
```

## 构建系统改造

### build.mk

```makefile
# APP_SUB 格式：category/widget_name（如 input/color_picker）
WIDGET_DIR = $(EXAMPLE_DIR)/HelloCustomWidgets/$(APP_SUB)

# 控件源码 = 控件实现 + 示例代码
EGUI_CODE_SRC += $(wildcard $(WIDGET_DIR)/*.c)

# 控件头文件路径
EGUI_CODE_INCLUDE += $(WIDGET_DIR)

# 独立 obj 目录，避免冲突（/ 替换为 _）
APP_OBJ_SUFFIX = HelloCustomWidgets_$(subst /,_,$(APP_SUB))
```

### CMakeLists.txt

```cmake
set(WIDGET_DIR ${CMAKE_CURRENT_SOURCE_DIR}/${APP_SUB})
file(GLOB WIDGET_SOURCES ${WIDGET_DIR}/*.c)
target_sources(app_lib PRIVATE ${WIDGET_SOURCES})
target_include_directories(app_lib PRIVATE ${WIDGET_DIR})
```

### code_compile_check.py 改造

```python
# 自动发现所有控件
custom_widgets = discover_widgets("example/HelloCustomWidgets/")
# 返回 [("input", "color_picker"), ("input", "date_picker"), ...]

# 按分类并行编译
with ProcessPoolExecutor(max_workers=cpu_count) as executor:
    for category, widget in custom_widgets:
        executor.submit(compile_code,
            f" APP=HelloCustomWidgets APP_SUB={category}/{widget}")
```

## CI 分层策略

### 日常 PR（核心检查）

现有 `compile-check` 逻辑不变，只编译核心控件和现有示例，耗时 ~19 秒。

### 自定义控件检查（触发条件）

1. `example/HelloCustomWidgets/**` 下有文件变动
2. `src/widget/**` 核心控件变动（可能影响自定义控件）
3. 每周定时全量

### GitHub Actions 改造

```yaml
custom-widgets-check:
  strategy:
    matrix:
      category: [input, display, layout, chart, navigation, feedback, media, decoration]
  steps:
    - run: python scripts/code_compile_check.py --custom-widgets --category ${{ matrix.category }}
```

8 个分类矩阵并行，每个分类 ~125 个控件，预估 2-3 分钟/分类。

## UI Designer 集成

### 按需加载

```python
# widget_registry.py 扩展
def _load_builtins(self):
    load_custom_widgets("scripts/ui_designer/custom_widgets")  # 核心控件

def load_extended_widgets(self, category=None):
    """按需加载自定义控件注册"""
    base = "example/HelloCustomWidgets"
    if category:
        load_custom_widgets(f"{base}/{category}/*/widget_register.py")
    else:
        load_custom_widgets(f"{base}/**/widget_register.py")
```

核心控件默认加载，自定义控件按需加载。

## 控件索引

```python
# scripts/generate_widget_index.py
# 扫描 example/HelloCustomWidgets/**/egui_view_*.h
# 输出 example/HelloCustomWidgets/INDEX.md
# 内容：分类、控件名、描述、截图链接
```

## 分类参考（8 大类）

| 分类 | 说明 | 示例控件 |
|------|------|----------|
| input | 输入类 | color_picker, date_picker, time_picker, search_bar |
| display | 展示类 | circular_gauge, weather_card, stat_card, badge |
| layout | 布局类 | waterfall_layout, flex_layout, accordion |
| chart | 图表类 | radar_chart, treemap, heatmap, candlestick |
| navigation | 导航类 | breadcrumb, sidebar, bottom_nav, drawer |
| feedback | 反馈类 | skeleton, toast_variant, alert, snackbar |
| media | 媒体类 | audio_player, video_thumbnail, carousel |
| decoration | 装饰类 | gradient_divider, shadow_box, rounded_frame |
