# egui_view_flexlayout 弹性布局容器

## 概述

`egui_view_flexlayout` 是 EmbeddedGUI 中的 Flexbox 风格布局容器，用于在资源受限环境下完成更灵活的子 view 排列。

它继承自 `egui_view_group_t`，本质上仍然是一个容器 view，但在普通 `view_group` 的子节点管理能力之上，增加了以下布局能力：

- 横向或纵向排列子 view。
- 子 view 超出主轴空间时换行。
- 沿主轴做 start、end、center、space-between、space-around、space-evenly 分布。
- 沿交叉轴做 start、end、center、stretch 对齐。
- 多行内容沿交叉轴分布。
- 支持 row gap / column gap。
- 支持子 view 通过 `flex_grow` 按比例占用剩余空间。

它适合用来构建按钮组、标签流、工具栏、卡片列表、上下结构页面等比 `LinearLayout` 更灵活的布局。

## 启用方式

`egui_view_flexlayout` 受 `EGUI_CONFIG_FUNCTION_FLEXLAYOUT` 控制，默认关闭。

```c
// app_egui_config.h
#define EGUI_CONFIG_FUNCTION_FLEXLAYOUT 1
```

启用后，`egui_view_t` 会新增一个 `uint8_t flex_grow` 字段，用于所有 view 在 flex 容器中的伸展比例。关闭该宏可以避免这 1 字节左右的单 view 实例 RAM 开销。

布局计算使用固定大小的栈数组，不使用 heap。最大行数由 `EGUI_CONFIG_FLEXLAYOUT_MAX_LINES` 控制，默认值为 `16`。

```c
// app_egui_config.h
#define EGUI_CONFIG_FLEXLAYOUT_MAX_LINES 16
```

## 数据结构

`egui_view_flexlayout_t` 定义如下：

```c
struct egui_view_flexlayout
{
    egui_view_group_t base;
    uint8_t           direction;
    uint8_t           wrap;
    uint8_t           justify_content;
    uint8_t           align_items;
    uint8_t           align_content;
    egui_dim_t        row_gap;
    egui_dim_t        col_gap;
};
```

字段含义：

| 字段 | 说明 |
| --- | --- |
| `direction` | 主轴方向，横向 row 或纵向 column |
| `wrap` | 子 view 超出主轴空间时是否换行 |
| `justify_content` | 单行内，子 view 沿主轴如何分布 |
| `align_items` | 单行内，子 view 沿交叉轴如何对齐 |
| `align_content` | 多行内容沿交叉轴如何分布 |
| `row_gap` | 行间距 |
| `col_gap` | 列间距 |

默认值：

| 属性 | 默认值 |
| --- | --- |
| `direction` | `EGUI_FLEX_DIRECTION_ROW` |
| `wrap` | `EGUI_FLEX_WRAP_NOWRAP` |
| `justify_content` | `EGUI_FLEX_JUSTIFY_START` |
| `align_items` | `EGUI_FLEX_ALIGN_STRETCH` |
| `align_content` | `EGUI_FLEX_JUSTIFY_START` |
| `row_gap` | `0` |
| `col_gap` | `0` |

## 核心概念

### 主轴和交叉轴

FlexLayout 的布局方向由 `direction` 决定：

| direction | 主轴 | 交叉轴 |
| --- | --- | --- |
| `EGUI_FLEX_DIRECTION_ROW` | X 轴，子 view 从左到右排列 | Y 轴 |
| `EGUI_FLEX_DIRECTION_COLUMN` | Y 轴，子 view 从上到下排列 | X 轴 |

在 row 模式下，`col_gap` 是主轴 item 间距，`row_gap` 是换行后的行间距。

在 column 模式下，`row_gap` 是主轴 item 间距，`col_gap` 是换列后的列间距。

### 内容区域

FlexLayout 会使用容器的 content area 排列子 view：

```text
content_width  = container_width  - padding_left - padding_right
content_height = container_height - padding_top  - padding_bottom
```

子 view 的最终坐标会加上父容器 padding 和自身 margin。

### margin

布局计算会把子 view 的 margin 计入尺寸：

- row 模式主轴尺寸：`width + margin_left + margin_right`
- row 模式交叉轴尺寸：`height + margin_top + margin_bottom`
- column 模式则相反

因此 margin 会影响换行、对齐和剩余空间计算。

### is_gone

`is_gone` 的子 view 不参与 FlexLayout 布局。

`is_visible == 0` 的子 view 当前仍会参与布局，只是不绘制。需要完全不占布局空间时，应使用 `egui_view_set_gone(view, 1)`。

## 支持的布局属性

### direction

```c
egui_view_flexlayout_set_direction(EGUI_VIEW_OF(&layout), EGUI_FLEX_DIRECTION_ROW);
egui_view_flexlayout_set_direction(EGUI_VIEW_OF(&layout), EGUI_FLEX_DIRECTION_COLUMN);
```

### wrap

```c
egui_view_flexlayout_set_wrap(EGUI_VIEW_OF(&layout), EGUI_FLEX_WRAP_NOWRAP);
egui_view_flexlayout_set_wrap(EGUI_VIEW_OF(&layout), EGUI_FLEX_WRAP_WRAP);
```

开启 wrap 后，当当前行再放入一个子 view 会超过主轴空间时，会创建新行。

### justify_content

`justify_content` 决定每一行内部沿主轴如何分配剩余空间。

| 值 | 说明 |
| --- | --- |
| `EGUI_FLEX_JUSTIFY_START` | 从起点排列 |
| `EGUI_FLEX_JUSTIFY_END` | 靠终点排列 |
| `EGUI_FLEX_JUSTIFY_CENTER` | 居中排列 |
| `EGUI_FLEX_JUSTIFY_SPACE_BETWEEN` | 首尾贴边，中间等距 |
| `EGUI_FLEX_JUSTIFY_SPACE_AROUND` | 每个 item 两侧分配空间 |
| `EGUI_FLEX_JUSTIFY_SPACE_EVENLY` | item 与边缘之间完全等距 |

### align_items

`align_items` 决定单行内部沿交叉轴如何对齐。

| 值 | 说明 |
| --- | --- |
| `EGUI_FLEX_ALIGN_START` | 靠交叉轴起点 |
| `EGUI_FLEX_ALIGN_END` | 靠交叉轴终点 |
| `EGUI_FLEX_ALIGN_CENTER` | 交叉轴居中 |
| `EGUI_FLEX_ALIGN_STRETCH` | 拉伸子 view 的交叉轴尺寸以填满本行交叉轴尺寸 |

`STRETCH` 会修改子 view 的尺寸。row 模式会改高度，column 模式会改宽度。

### align_content

`align_content` 只在多行布局时有意义，用于决定多行整体沿交叉轴如何分布。它复用 `EGUI_FLEX_JUSTIFY_*` 枚举：

```c
egui_view_flexlayout_set_align_content(EGUI_VIEW_OF(&layout), EGUI_FLEX_JUSTIFY_SPACE_EVENLY);
```

### gap

```c
egui_view_flexlayout_set_gap(EGUI_VIEW_OF(&layout), row_gap, col_gap);
```

row 模式下：

- `col_gap`：同一行内子 view 的水平间距。
- `row_gap`：多行之间的垂直间距。

column 模式下：

- `row_gap`：同一列内子 view 的垂直间距。
- `col_gap`：多列之间的水平间距。

### flex_grow

`flex_grow` 是子 view 的属性，用于按比例分配主轴剩余空间。

```c
egui_view_set_flex_grow(EGUI_VIEW_OF(&body), 1);
```

规则：

- `flex_grow == 0` 表示不伸展。
- 同一行内所有 `flex_grow > 0` 的子 view 按比例分配剩余主轴空间。
- row 模式会增加子 view 宽度。
- column 模式会增加子 view 高度。
- 计算使用整数除法，可能存在 1 像素级余数。

示例：容器宽度 200，两个子 view 初始宽度都为 20，`flex_grow` 分别是 2 和 1。剩余空间是 160，两个子 view 分别增加 `160 * 2 / 3 = 106` 和 `160 * 1 / 3 = 53` 像素。

## 使用流程

FlexLayout 当前不会自动在 `calculate_layout` 阶段执行排版。设置完容器属性、子 view 尺寸、margin、padding、`flex_grow`，并把子 view 添加到容器后，需要显式调用：

```c
egui_view_flexlayout_layout_childs(EGUI_VIEW_OF(&layout));
```

如果后续修改了以下内容，也需要重新调用一次：

- 容器尺寸或 padding。
- 子 view 的尺寸、margin、`is_gone`。
- `direction`、`wrap`、`justify_content`、`align_items`、`align_content`、gap。
- 子 view 的 `flex_grow`。
- 子 view 增删顺序。

## 示例

横向换行布局：

```c
static egui_view_flexlayout_t layout;
static egui_view_label_t item0;
static egui_view_label_t item1;
static egui_view_label_t item2;

void page_init(egui_core_t *core)
{
    egui_view_flexlayout_init(EGUI_VIEW_OF(&layout), core);
    egui_view_set_position(EGUI_VIEW_OF(&layout), 0, 0);
    egui_view_set_size(EGUI_VIEW_OF(&layout), 240, 120);
    egui_view_set_padding(EGUI_VIEW_OF(&layout), 8, 8, 8, 8);
    egui_view_flexlayout_set_wrap(EGUI_VIEW_OF(&layout), EGUI_FLEX_WRAP_WRAP);
    egui_view_flexlayout_set_justify_content(EGUI_VIEW_OF(&layout), EGUI_FLEX_JUSTIFY_SPACE_AROUND);
    egui_view_flexlayout_set_align_items(EGUI_VIEW_OF(&layout), EGUI_FLEX_ALIGN_CENTER);
    egui_view_flexlayout_set_align_content(EGUI_VIEW_OF(&layout), EGUI_FLEX_JUSTIFY_SPACE_EVENLY);
    egui_view_flexlayout_set_gap(EGUI_VIEW_OF(&layout), 6, 6);

    egui_view_label_init(EGUI_VIEW_OF(&item0), core);
    egui_view_set_size(EGUI_VIEW_OF(&item0), 60, 40);

    egui_view_label_init(EGUI_VIEW_OF(&item1), core);
    egui_view_set_size(EGUI_VIEW_OF(&item1), 60, 40);

    egui_view_label_init(EGUI_VIEW_OF(&item2), core);
    egui_view_set_size(EGUI_VIEW_OF(&item2), 60, 40);

    egui_view_group_add_child(EGUI_VIEW_OF(&layout), EGUI_VIEW_OF(&item0));
    egui_view_group_add_child(EGUI_VIEW_OF(&layout), EGUI_VIEW_OF(&item1));
    egui_view_group_add_child(EGUI_VIEW_OF(&layout), EGUI_VIEW_OF(&item2));

    egui_view_flexlayout_layout_childs(EGUI_VIEW_OF(&layout));
    egui_core_add_user_root_view(EGUI_VIEW_OF(&layout));
}
```

纵向布局中让 body 占满剩余高度：

```c
static egui_view_flexlayout_t layout;
static egui_view_label_t header;
static egui_view_label_t body;

void page_init(egui_core_t *core)
{
    egui_view_flexlayout_init(EGUI_VIEW_OF(&layout), core);
    egui_view_set_size(EGUI_VIEW_OF(&layout), 240, 160);
    egui_view_flexlayout_set_direction(EGUI_VIEW_OF(&layout), EGUI_FLEX_DIRECTION_COLUMN);
    egui_view_flexlayout_set_align_items(EGUI_VIEW_OF(&layout), EGUI_FLEX_ALIGN_STRETCH);

    egui_view_label_init(EGUI_VIEW_OF(&header), core);
    egui_view_set_size(EGUI_VIEW_OF(&header), 240, 32);

    egui_view_label_init(EGUI_VIEW_OF(&body), core);
    egui_view_set_size(EGUI_VIEW_OF(&body), 240, 40);
    egui_view_set_flex_grow(EGUI_VIEW_OF(&body), 1);

    egui_view_group_add_child(EGUI_VIEW_OF(&layout), EGUI_VIEW_OF(&header));
    egui_view_group_add_child(EGUI_VIEW_OF(&layout), EGUI_VIEW_OF(&body));

    egui_view_flexlayout_layout_childs(EGUI_VIEW_OF(&layout));
}
```

完整示例见 `example/HelloBasic/flexlayout/test.c`。

## 和其它布局方式的关系

| 布局方式 | 适用场景 |
| --- | --- |
| 手动布局 | 控件数量少、位置固定、对 ROM/RAM 最敏感 |
| `egui_view_linearlayout_t` | 单方向简单排列，可选 auto width / auto height |
| `egui_view_gridlayout_t` | 固定列数网格 |
| `egui_view_flexlayout_t` | 更灵活的流式排列、换行、对齐、伸展 |

如果只是简单的一列或一行，`LinearLayout` 更轻量。如果需要换行、space-around、stretch 或按比例占用剩余空间，再使用 FlexLayout。

## 设计边界

`egui_view_flexlayout` 是嵌入式场景下的轻量 Flexbox 子集，不是完整 CSS Flexbox。

需要注意：

- 不使用 heap，布局时使用固定栈数组。
- 默认关闭；启用后所有 view 会增加 `flex_grow` 字段。
- 当前不会自动随 view tree 的 layout 阶段重新排版，需要调用 `egui_view_flexlayout_layout_childs()`。
- 不支持 `flex_shrink`、`flex_basis`、order、min/max size、百分比尺寸等 CSS Flexbox 能力。
- `align_items = STRETCH` 和 `flex_grow` 会直接修改子 view 尺寸。
- `EGUI_CONFIG_FLEXLAYOUT_MAX_LINES` 同时限制布局计算可跟踪的行数和每行 item 数量，超出后结果会被截断或挤到最后可用记录中。
- 尺寸和间距都使用整数计算，空间分配存在整数除法余数。

相关单测位于 `example/HelloUnitTest/test/test_flexlayout.c` 和 `example/HelloUnitTest/test/test_flexlayout_get_state.c`，覆盖默认值、方向、主轴分布、交叉轴对齐、换行、gap、`flex_grow` 和 `is_gone` 行为。
