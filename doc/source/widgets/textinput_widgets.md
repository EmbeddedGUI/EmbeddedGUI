# 文本输入控件

## 概述

文本输入控件提供文字编辑和虚拟键盘能力，适用于表单填写、搜索框、密码输入等嵌入式交互场景。包括文本输入框和虚拟键盘两个控件。

## TextInput

文本输入框控件，支持光标移动、字符插入/删除、占位文字、文本变更回调和提交回调。

### 效果展示

[在线演示](https://embeddedgui.github.io/EmbeddedGUI/basic.html?app=textinput)

### API

| 函数 | 说明 |
|------|------|
| `egui_view_textinput_init(self)` | 初始化文本输入框 |
| `egui_view_textinput_set_text(self, text)` | 设置文本内容 |
| `egui_view_textinput_get_text(self)` | 获取文本内容 |
| `egui_view_textinput_clear(self)` | 清空文本 |
| `egui_view_textinput_insert_char(self, c)` | 在光标处插入字符 |
| `egui_view_textinput_delete_char(self)` | 删除光标前一个字符 |
| `egui_view_textinput_delete_forward(self)` | 删除光标后一个字符 |
| `egui_view_textinput_set_cursor_pos(self, pos)` | 设置光标位置 |
| `egui_view_textinput_move_cursor_left(self)` | 光标左移 |
| `egui_view_textinput_move_cursor_right(self)` | 光标右移 |
| `egui_view_textinput_move_cursor_home(self)` | 光标移到行首 |
| `egui_view_textinput_move_cursor_end(self)` | 光标移到行尾 |
| `egui_view_textinput_set_font(self, font)` | 设置字体 |
| `egui_view_textinput_set_text_color(self, color, alpha)` | 设置文字颜色 |
| `egui_view_textinput_set_placeholder(self, placeholder)` | 设置占位文字 |
| `egui_view_textinput_set_placeholder_color(self, color, alpha)` | 设置占位文字颜色 |
| `egui_view_textinput_set_cursor_color(self, color)` | 设置光标颜色 |
| `egui_view_textinput_set_max_length(self, max_length)` | 设置最大输入长度 |
| `egui_view_textinput_set_on_text_changed(self, listener)` | 设置文本变更回调 |
| `egui_view_textinput_set_on_submit(self, listener)` | 设置提交回调 |

### 回调函数原型

```c
typedef void (*egui_view_textinput_on_text_changed_t)(egui_view_t *self, const char *text);
typedef void (*egui_view_textinput_on_submit_t)(egui_view_t *self, const char *text);
```

### 结构体字段

| 字段 | 类型 | 说明 |
|------|------|------|
| `text` | `char[]` | 文本缓冲区 |
| `text_len` | `uint8_t` | 当前文本长度 |
| `cursor_pos` | `uint8_t` | 光标位置 |
| `max_length` | `uint8_t` | 最大输入长度 |
| `text_color` | `egui_color_t` | 文字颜色 |
| `placeholder_color` | `egui_color_t` | 占位文字颜色 |
| `cursor_color` | `egui_color_t` | 光标颜色 |
| `align_type` | `uint8_t` | 文字对齐方式 |

### 配置选项

| 宏 | 说明 |
|----|------|
| `EGUI_CONFIG_TEXTINPUT_MAX_LENGTH` | 文本输入最大字符数 |

### 使用说明

- TextInput 内置光标闪烁定时器，聚焦时自动显示闪烁光标
- 文本超出控件宽度时自动水平滚动，通过 `scroll_offset_x` 内部管理
- `on_text_changed` 在每次字符插入或删除后触发
- `on_submit` 在用户按下回车键（通过 Keyboard 的 Enter 键）时触发
- 通常与 Keyboard 控件配合使用，也可通过外部按键事件直接调用 `insert_char` / `delete_char`

### 代码示例

```c
static egui_view_textinput_t input;

static void on_text_changed(egui_view_t *self, const char *text)
{
    EGUI_LOG_INF("Text: %s\n", text);
}

static void on_submit(egui_view_t *self, const char *text)
{
    EGUI_LOG_INF("Submit: %s\n", text);
}

void init_ui(void)
{
    egui_view_textinput_init(EGUI_VIEW_OF(&input));
    egui_view_set_position(EGUI_VIEW_OF(&input), 10, 10);
    egui_view_set_size(EGUI_VIEW_OF(&input), 200, 30);
    egui_view_textinput_set_placeholder(EGUI_VIEW_OF(&input), "Enter text...");
    egui_view_textinput_set_max_length(EGUI_VIEW_OF(&input), 32);
    egui_view_textinput_set_on_text_changed(EGUI_VIEW_OF(&input),
        on_text_changed);
    egui_view_textinput_set_on_submit(EGUI_VIEW_OF(&input), on_submit);
    egui_core_add_user_root_view(EGUI_VIEW_OF(&input));
}
```

---

## Keyboard

虚拟键盘控件，继承自 Group，提供完整的 QWERTY 键盘布局，支持大小写切换和符号模式。与 TextInput 配合使用，自动处理键盘弹出时的视图避让。

### 效果展示

[在线演示](https://embeddedgui.github.io/EmbeddedGUI/basic.html?app=textinput)

### 前置条件

Keyboard 需要同时启用按键和焦点支持：

```c
#define EGUI_CONFIG_FUNCTION_SUPPORT_KEY   1
#define EGUI_CONFIG_FUNCTION_SUPPORT_FOCUS 1
```

### API

| 函数 | 说明 |
|------|------|
| `egui_view_keyboard_init(self)` | 初始化键盘 |
| `egui_view_keyboard_set_font(self, font)` | 设置按键字体 |
| `egui_view_keyboard_show(self, target_textinput)` | 显示键盘并绑定目标输入框 |
| `egui_view_keyboard_hide(self)` | 隐藏键盘 |
| `egui_view_keyboard_set_mode(self, mode)` | 设置键盘模式 |

### 键盘模式常量

| 常量 | 值 | 说明 |
|------|----|------|
| `EGUI_KEYBOARD_MODE_LOWERCASE` | 0 | 小写字母 |
| `EGUI_KEYBOARD_MODE_UPPERCASE` | 1 | 大写字母 |
| `EGUI_KEYBOARD_MODE_SYMBOLS` | 2 | 符号 |

### 布局常量

| 常量 | 说明 |
|------|------|
| `EGUI_KEYBOARD_ROW_COUNT` | 键盘行数(4) |
| `EGUI_KEYBOARD_TOTAL_KEYS` | 按键总数(31) |
| `EGUI_KEYBOARD_DEFAULT_WIDTH` | 默认宽度(屏幕宽度) |
| `EGUI_KEYBOARD_DEFAULT_HEIGHT` | 默认高度(128) |

### 代码示例

```c
static egui_view_textinput_t input;
static egui_view_keyboard_t keyboard;

void init_ui(void)
{
    // 初始化输入框
    egui_view_textinput_init(EGUI_VIEW_OF(&input));
    egui_view_set_position(EGUI_VIEW_OF(&input), 10, 10);
    egui_view_set_size(EGUI_VIEW_OF(&input), 220, 30);
    egui_view_textinput_set_placeholder(EGUI_VIEW_OF(&input), "Tap to type");
    egui_core_add_user_root_view(EGUI_VIEW_OF(&input));

    // 初始化键盘
    egui_view_keyboard_init(EGUI_VIEW_OF(&keyboard));
    egui_view_set_position(EGUI_VIEW_OF(&keyboard),
        0, EGUI_CONFIG_SCEEN_HEIGHT - EGUI_KEYBOARD_DEFAULT_HEIGHT);
    egui_core_add_user_root_view(EGUI_VIEW_OF(&keyboard));
}

void show_keyboard(void)
{
    egui_view_keyboard_show(EGUI_VIEW_OF(&keyboard), EGUI_VIEW_OF(&input));
}

void hide_keyboard(void)
{
    egui_view_keyboard_hide(EGUI_VIEW_OF(&keyboard));
}
```

### 特殊按键索引

| 常量 | 值 | 说明 |
|------|----|------|
| `EGUI_KEYBOARD_KEY_IDX_SHIFT` | 19 | Shift 键 |
| `EGUI_KEYBOARD_KEY_IDX_BACKSPACE` | 27 | 退格键 |
| `EGUI_KEYBOARD_KEY_IDX_MODE` | 28 | 模式切换键 |
| `EGUI_KEYBOARD_KEY_IDX_SPACE` | 29 | 空格键 |
| `EGUI_KEYBOARD_KEY_IDX_ENTER` | 30 | 回车键 |

### 使用说明

- Keyboard 在调用 `show()` 时自动绑定目标 TextInput，按键输入直接写入目标输入框
- 键盘弹出时会自动调整被遮挡的视图位置（键盘避让），隐藏时恢复原位
- Shift 键切换大小写，Mode 键在字母和符号之间切换
- 键盘宽度默认为屏幕宽度 `EGUI_CONFIG_SCEEN_WIDTH`，适配不同分辨率需调整按键宽度常量
- 键盘内部使用 LinearLayout 排列按键行，每行按键数量固定
