# egui_property 轻量属性系统

## 概述

`egui_property` 是 EmbeddedGUI 中面向 `egui_view_t` 的轻量属性读写层。它把位置、尺寸、可见性、透明度、文本、内外边距等常用 view 属性统一抽象成 `property id + property value` 的形式，调用方可以通过同一组 API 读写基础属性，而不需要为每个字段分别调用专用 setter/getter。

这个机制主要服务于以下场景：

- 布局、动画、样式、脚本或生成器需要用统一入口操作 view 基础属性。
- 通用工具需要按属性 ID 读取或设置控件状态，例如调试面板、录制回放、自动化测试。
- 不希望把大型反射系统、字符串表或控件专有属性默认带入基础配置，但仍需要一个小体积的公共属性接口。

## 启用方式

`egui_property` 受配置宏 `EGUI_CONFIG_FUNCTION_PROPERTY_LITE` 控制，默认关闭。

```c
// app_egui_config.h
#define EGUI_CONFIG_FUNCTION_PROPERTY_LITE 1
```

启用后，`src/egui.h` 会包含 `src/widget/egui_property.h`，并提供以下 API：

```c
int egui_view_set_property(egui_view_t *self, egui_property_id_t id, const egui_property_value_t *value);
int egui_view_get_property(egui_view_t *self, egui_property_id_t id, egui_property_value_t *value);
```

返回值约定：

- `0`：读写成功。
- `-1`：参数为空、属性 ID 不支持、属性类型不匹配，或当前属性不能按请求方式处理。

## 属性值类型

属性值使用 `egui_property_value_t` 表示：

```c
typedef struct egui_property_value
{
    egui_property_type_t type;
    union
    {
        int32_t i32;
        uint8_t u8;
        const char *str;
        void *ptr;
    } data;
} egui_property_value_t;
```

支持的类型如下：

| 类型 | 用途 |
| --- | --- |
| `EGUI_PROPERTY_TYPE_INT` | 坐标、尺寸、边距等整数属性 |
| `EGUI_PROPERTY_TYPE_U8` | 布尔状态、透明度等小整数属性 |
| `EGUI_PROPERTY_TYPE_STRING` | 文本指针 |
| `EGUI_PROPERTY_TYPE_PTR` | 预留给指针类属性，当前公共 view 属性未使用 |

对数值类属性，`INT` 和 `U8` 可以互转使用；其它类型会返回 `-1`。

## 当前支持的属性

| 属性 ID | 说明 | get 返回类型 | set 接受类型 |
| --- | --- | --- | --- |
| `EGUI_PROPERTY_X` | view 相对父容器的 X 坐标 | `INT` | `INT` / `U8` |
| `EGUI_PROPERTY_Y` | view 相对父容器的 Y 坐标 | `INT` | `INT` / `U8` |
| `EGUI_PROPERTY_WIDTH` | view 宽度 | `INT` | `INT` / `U8` |
| `EGUI_PROPERTY_HEIGHT` | view 高度 | `INT` | `INT` / `U8` |
| `EGUI_PROPERTY_VISIBLE` | 是否可见 | `U8` | `INT` / `U8` |
| `EGUI_PROPERTY_ENABLED` | 是否启用 | `U8` | `INT` / `U8` |
| `EGUI_PROPERTY_CLICKABLE` | 是否可点击 | `U8` | `INT` / `U8` |
| `EGUI_PROPERTY_ALPHA` | view 透明度 | `U8` | `INT` / `U8` |
| `EGUI_PROPERTY_TEXT` | label 文本指针 | `STRING` | `STRING` |
| `EGUI_PROPERTY_PADDING_LEFT` | 左内边距 | `INT` | `INT` / `U8` |
| `EGUI_PROPERTY_PADDING_RIGHT` | 右内边距 | `INT` | `INT` / `U8` |
| `EGUI_PROPERTY_PADDING_TOP` | 上内边距 | `INT` | `INT` / `U8` |
| `EGUI_PROPERTY_PADDING_BOTTOM` | 下内边距 | `INT` | `INT` / `U8` |
| `EGUI_PROPERTY_MARGIN_LEFT` | 左外边距 | `INT` | `INT` / `U8` |
| `EGUI_PROPERTY_MARGIN_RIGHT` | 右外边距 | `INT` | `INT` / `U8` |
| `EGUI_PROPERTY_MARGIN_TOP` | 上外边距 | `INT` | `INT` / `U8` |
| `EGUI_PROPERTY_MARGIN_BOTTOM` | 下外边距 | `INT` | `INT` / `U8` |

## 行为细节

### 坐标和尺寸

`X`、`Y`、`WIDTH`、`HEIGHT` 分别映射到底层的 `egui_view_set_position()` 和 `egui_view_set_size()`。设置其中一个字段时，另一个字段保持当前值不变。

例如设置 `X` 时，只改变 X 坐标，Y 坐标仍然使用 `egui_view_get_y(self)` 的当前值。

### 状态属性

`VISIBLE`、`ENABLED`、`CLICKABLE` 接受数值类型。写入值为 `0` 时表示关闭，非 `0` 表示开启。

### 透明度

`ALPHA` 接受数值类型，写入时会被限制到 `0..EGUI_ALPHA_100` 范围内，避免越界值直接进入 view 状态。

### 文本

`TEXT` 使用 `EGUI_PROPERTY_TYPE_STRING`，内部调用 `egui_view_label_set_text()` 和 `egui_view_label_get_text()`。

需要注意：

- `egui_property` 不会复制字符串内容，只保存调用方传入的字符串指针。
- `TEXT` 适用于 label 兼容对象。普通 `egui_view_t` 没有文本字段，不应随意通过 `TEXT` 属性设置文本。
- 调用方需要保证字符串生命周期长于控件使用周期。

### Padding 和 Margin

单边 padding/margin 属性会读取其它三边的当前值，然后整体调用 `egui_view_set_padding()` 或 `egui_view_set_margin()` 写回。因此只改一边不会清空其它边。

## 使用示例

```c
egui_property_value_t value;

value.type = EGUI_PROPERTY_TYPE_INT;
value.data.i32 = 16;
egui_view_set_property(EGUI_VIEW_OF(&label), EGUI_PROPERTY_X, &value);

value.type = EGUI_PROPERTY_TYPE_U8;
value.data.u8 = EGUI_ALPHA_100 / 2;
egui_view_set_property(EGUI_VIEW_OF(&label), EGUI_PROPERTY_ALPHA, &value);

value.type = EGUI_PROPERTY_TYPE_STRING;
value.data.str = "Hello";
egui_view_set_property(EGUI_VIEW_OF(&label), EGUI_PROPERTY_TEXT, &value);

if (egui_view_get_property(EGUI_VIEW_OF(&label), EGUI_PROPERTY_WIDTH, &value) == 0)
{
    int32_t width = value.data.i32;
    (void)width;
}
```

## 设计边界

`egui_property` 是轻量属性访问层，不是完整反射系统。

它刻意不做以下事情：

- 不保存属性名称字符串，也不提供字符串到属性 ID 的运行时解析。
- 不枚举每个控件的全部专有能力。
- 不自动检查 `TEXT` 对应对象是否一定是 label 类型。
- 不复制字符串和外部指针指向的数据。
- 不替代已有的强类型 setter/getter API。

因此，在业务代码明确知道控件类型和目标属性时，优先使用强类型 API；在生成器、动画、样式、调试工具等需要统一属性入口的地方，再使用 `egui_property`。

## 扩展原则

新增属性时应保持这个模块的“小而通用”定位：

- 优先加入所有 view 都稳定支持的基础属性。
- 谨慎加入控件专有属性，避免一个少数控件使用的能力增加所有启用者的代码体积。
- 不应为了属性系统引入额外 RAM 字段；属性读写应尽量复用已有 view 状态。
- 如果属性会带来明显 ROM/RAM 增量，应继续受独立功能宏控制，示例按需显式开启。

相关单测位于 `example/HelloUnitTest/test/test_property_lite.c`，覆盖基础属性、文本属性和 spacing 属性。
