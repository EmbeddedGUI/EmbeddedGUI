# dirty_passthrough 结构容器脏区透传

`dirty_passthrough` 用来标记“只负责组织子控件、不绘制自身像素”的结构容器。开启后，layout 引起的脏区不再默认使用容器自身的整块 `region_screen`，而是透传到直接子控件，按子控件实际扫过区域生成脏矩形。

## 适用条件

自定义容器满足以下条件时，可以调用 `egui_view_set_dirty_passthrough(view, 1)`：

- 容器自身不绘制背景、边框、阴影或其它像素。
- 容器的视觉变化完全由子控件决定。
- 容器移动或翻页时，希望脏区覆盖子控件扫过区域，而不是覆盖整个容器。

如果容器带有 background，并且容器自身的位置或尺寸发生变化，框架仍会为容器自身生成 swept 脏区，保证背景正确刷新。若 background 容器的位置和尺寸不变，仅内部子控件移动，则仍可以只更新子控件脏区。

## API

```c
#define EGUI_CONFIG_FUNCTION_SUPPORT_DIRTY_PASSTHROUGH 1

egui_view_set_dirty_passthrough(view, 1);
egui_view_get_dirty_passthrough(view);
```

该能力由 `EGUI_CONFIG_FUNCTION_SUPPORT_DIRTY_PASSTHROUGH` 控制，默认关闭。宏关闭时 setter 是 no-op，getter 返回 `0`，普通应用仍按常规容器脏区工作。该标记不改变 visible、alpha、pressed、background 等绘制路径。它只影响 layout 引起的结构容器脏区分发。

## 内置使用

启用 `EGUI_CONFIG_FUNCTION_SUPPORT_DIRTY_PASSTHROUGH=1` 后，以下内置结构会在初始化时开启透传标记：

- root group
- page root view
- scroll 及其内部线性容器
- viewpage 及其内部线性容器

## 示例和单测

HelloBasic 增加了 3 个示例：

- `dirty_passthrough_container`：滚动容器、稀疏子控件、混合结构容器的对比场景。
- `dirty_passthrough_page`：真实多 Page 场景，第二页使用 LinearLayout，第三页使用 ViewPage，第四页使用 ViewPage + Scroll + LinearLayout。
- `dirty_passthrough_activity`：真实多 Activity 场景，同样覆盖 LinearLayout、ViewPage、ViewPage + Scroll + LinearLayout。

这些示例会在自己的 `app_egui_config.h` 中开启 `EGUI_CONFIG_FUNCTION_SUPPORT_DIRTY_PASSTHROUGH` 和 dirty region 调试宏，方便直接观察蓝色脏区边框、统计和 trace。`DIRTY_PASSTHROUGH_CONTAINER_DEMO_BASELINE=1` 可用于关闭容器透传，观察 before/after 差异。

HelloUnitTest 中的 `dirty_passthrough_container` suite 覆盖：

- plain group 平移、尺寸变化、background、空容器。
- scroll 内部容器拖动、首次 attach、background、空容器。
- viewpage 翻页、首次 attach、空容器。
- page root 和 activity root 的子控件脏区透传。
- 稀疏垂直列表、水平分页、嵌套结构页等优势对比 case。

## 调试建议

开启 `EGUI_CONFIG_DEBUG_DIRTY_REGION_TRACE` 后，局部脏区来源会出现 `dirty_passthrough_swept` 或 `dirty_passthrough_self`。如果最终仍是全屏脏区，优先检查：

- 是否有普通可见控件或 background 容器覆盖了整屏。
- 真正移动的内容根是否也开启了 `dirty_passthrough`。
- `EGUI_CONFIG_DIRTY_AREA_COUNT` 是否过小，导致多个小脏区被合并成大脏区。
