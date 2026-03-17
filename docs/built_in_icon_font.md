# 内置图标字体说明

## 目标

为库层提供一套可直接使用的常用图标字体，优先覆盖通用 UI 场景，避免每个示例或应用重复打包菜单、搜索、关闭、刷新等基础图标。

这套机制有几个原则：

- 直接内置到库里，应用侧不需要再配置 `app_resource_config.json`
- 不依赖 UI Designer 的控件注册或元数据机制
- 常用图标走库级公共字体，业务专用图标仍建议走应用资源管线
- 图标以字体字形方式渲染，方便按文字颜色统一着色，缩放效果也更稳定

## 当前产物

- 配置源：`scripts/tools/build_in/material_symbols_common_icons.json`
- 中间文本：`scripts/tools/build_in/material_symbols_common_text.txt`
- 生成脚本：`scripts/tools/build_in/built_in_icon_font_gen.py`
- 总入口脚本：`scripts/tools/build_in/built_in_font_gen.py`
- 头文件：`src/resource/egui_icon_material_symbols.h`
- 字体资源：
  - `src/resource/egui_res_font_material_symbols_common_16_4.c`
  - `src/resource/egui_res_font_material_symbols_common_20_4.c`
  - `src/resource/egui_res_font_material_symbols_common_24_4.c`

## 使用方式

库头文件已经通过 `src/resource/egui_resource.h` 导出，因此正常包含 `egui.h` 即可使用。

```c
#include "egui.h"

static egui_view_label_t icon_label;

egui_view_label_init(&icon_label);
egui_view_label_set_font(EGUI_VIEW_OF(&icon_label), EGUI_FONT_ICON_MS_20);
egui_view_label_set_text(EGUI_VIEW_OF(&icon_label), EGUI_ICON_MS_SETTINGS);
```

目前提供三档常用尺寸：

- `EGUI_FONT_ICON_MS_16`
- `EGUI_FONT_ICON_MS_20`
- `EGUI_FONT_ICON_MS_24`

图标内容通过宏直接取字符串，例如：

- `EGUI_ICON_MS_SEARCH`
- `EGUI_ICON_MS_MENU`
- `EGUI_ICON_MS_CLOSE`
- `EGUI_ICON_MS_CHECK`
- `EGUI_ICON_MS_SETTINGS`

另外补了几个更适合直接当“符号字”使用的别名：

- `EGUI_ICON_MS_CHECK_MARK` 对应 `√`
- `EGUI_ICON_MS_CROSS` 对应 `×`
- `EGUI_ICON_MS_HEART` 对应 `♥`

如果一个控件里既要显示普通文本，又要显示图标，建议：

- 用两个独立 label 分开渲染
- 或者继续走应用资源管线，按项目需要生成自己的混合字库

## 示例约定

库内部目前仍保留一层“按控件尺寸自动选择图标字体”的兼容逻辑，这样旧代码不至于立即失效。

但官方 `example/` 示例现在统一约定：

- 只要示例代码显式调用了图标相关 API，就同时显式调用对应的 `*_set_icon_font()`
- 示例不再依赖控件内部的自动推断逻辑
- 推荐优先使用 `EGUI_FONT_ICON_MS_16`、`EGUI_FONT_ICON_MS_20`、`EGUI_FONT_ICON_MS_24`

这样做的目的主要有两点：

1. 示例行为更稳定，后续控件内部策略调整时不容易出现“看起来还能编译，但视觉效果悄悄变了”
2. 示例代码本身能更直接地告诉用户“图标字符串”和“图标字体”是要成对考虑的

## 检查脚本

为防止示例回退到隐式推断，仓库增加了检查脚本：

- `scripts/check_example_icon_font.py`

这个脚本会扫描 `example/` 下的 `.c` 文件（跳过 `resource/`），检查常见图标控件是否在设置图标内容后，也设置了对应的 icon font。

当前它已经接入：

- `scripts/release_check.py`

也就是说，执行 release check 时，这条约定会自动校验；如果某个示例只设置了图标内容、没有设置 icon font，检查会直接报出具体文件和计数，便于回归修复。

## 重新生成

只更新内置图标字体：

```bash
python scripts/tools/build_in/built_in_icon_font_gen.py
```

同时更新 Montserrat 与内置图标字体：

```bash
python scripts/tools/build_in/built_in_font_gen.py
```

如果要扩充图标集合，流程是：

1. 编辑 `scripts/tools/build_in/material_symbols_common_icons.json`
2. 重新运行 `python scripts/tools/build_in/built_in_icon_font_gen.py`
3. 检查 `src/resource/egui_icon_material_symbols.h` 和 3 个字体资源是否已更新
4. 再执行一次常规构建验证

## 关于应用侧资源

这套内置图标字体适合做公共基础图标，不替代应用资源系统：

- 业务 logo
- 特定品牌图标
- 只在单个应用里使用的私有图标
- 需要和正文混排的定制字库

这类内容仍建议放在 `example/{APP}/resource/src/` 下，通过现有资源生成流程管理。
