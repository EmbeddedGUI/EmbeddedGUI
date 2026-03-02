# 选择控件

## 概述

选择控件用于在多个选项中进行选择或导航。EmbeddedGUI 提供了四种选择类控件：Combobox 下拉选择框，TabBar 标签栏，Menu 多级菜单，PageIndicator 页面指示器(圆点)。

---

## Combobox

下拉选择框，点击展开显示选项列表，选中后折叠。支持自定义颜色、字体和最大可见项数。

### 效果展示

[在线演示](https://embeddedgui.github.io/EmbeddedGUI/basic.html?app=combobox)

### API

| 函数 | 说明 |
|------|------|
| `egui_view_combobox_init(self)` | 初始化 Combobox |
| `egui_view_combobox_init_with_params(self, params)` | 使用参数初始化 |
| `egui_view_combobox_set_items(self, items, count)` | 设置选项列表 |
| `egui_view_combobox_set_current_index(self, index)` | 设置当前选中项 |
| `egui_view_combobox_get_current_index(self)` | 获取当前选中索引 |
| `egui_view_combobox_get_current_text(self)` | 获取当前选中文本 |
| `egui_view_combobox_set_max_visible_items(self, max)` | 设置展开时最大可见项数 |
| `egui_view_combobox_set_font(self, font)` | 设置字体 |
| `egui_view_combobox_expand(self)` | 程序化展开 |
| `egui_view_combobox_collapse(self)` | 程序化折叠 |
| `egui_view_combobox_is_expanded(self)` | 查询是否展开 |
| `egui_view_combobox_set_on_selected_listener(self, listener)` | 设置选中回调 |

### 参数宏

```c
EGUI_VIEW_COMBOBOX_PARAMS_INIT(name, x, y, w, h, items, count, index);
```

### 代码示例

```c
static egui_view_combobox_t combobox;

static const char *options[] = {
    "Option A", "Option B", "Option C", "Option D"
};

EGUI_VIEW_COMBOBOX_PARAMS_INIT(cb_params, 10, 10, 160, 30,
    options, 4, 0);

static void on_selected(egui_view_t *self, uint8_t index)
{
    EGUI_LOG_INF("Selected: %s\n", options[index]);
}

void init_ui(void)
{
    egui_view_combobox_init_with_params(
        EGUI_VIEW_OF(&combobox), &cb_params);
    egui_view_combobox_set_max_visible_items(
        EGUI_VIEW_OF(&combobox), 3);
    egui_view_combobox_set_on_selected_listener(
        EGUI_VIEW_OF(&combobox), on_selected);

    egui_core_add_user_root_view(EGUI_VIEW_OF(&combobox));
}
```

---

## TabBar

标签栏控件，显示一组文本标签，点击切换当前选中项，选中项下方显示指示条。常与 ViewPage 配合使用实现 Tab 页面切换。

### 效果展示

[在线演示](https://embeddedgui.github.io/EmbeddedGUI/basic.html?app=tab_bar)

### API

| 函数 | 说明 |
|------|------|
| `egui_view_tab_bar_init(self)` | 初始化 TabBar |
| `egui_view_tab_bar_init_with_params(self, params)` | 使用参数初始化 |
| `egui_view_tab_bar_set_tabs(self, tab_texts, tab_count)` | 设置标签文本和数量 |
| `egui_view_tab_bar_set_current_index(self, index)` | 设置当前选中标签 |
| `egui_view_tab_bar_set_on_tab_changed_listener(self, listener)` | 设置标签切换回调 |

### 参数宏

```c
EGUI_VIEW_TAB_BAR_PARAMS_INIT(name, x, y, w, h, texts, count);
```

### 代码示例

```c
static egui_view_tab_bar_t tab_bar;

static const char *tabs[] = {"Home", "Search", "Profile"};

EGUI_VIEW_TAB_BAR_PARAMS_INIT(tb_params, 0, 0, 240, 36, tabs, 3);

static void on_tab_changed(egui_view_t *self, uint8_t index)
{
    EGUI_LOG_INF("Tab changed: %d\n", index);
}

void init_ui(void)
{
    egui_view_tab_bar_init_with_params(
        EGUI_VIEW_OF(&tab_bar), &tb_params);
    egui_view_tab_bar_set_on_tab_changed_listener(
        EGUI_VIEW_OF(&tab_bar), on_tab_changed);

    egui_core_add_user_root_view(EGUI_VIEW_OF(&tab_bar));
}
```

---

## Menu

多级菜单控件，支持页面嵌套导航和返回操作。每个菜单页包含标题和若干菜单项，菜单项可以是叶子节点(触发回调)或子菜单入口。

### 效果展示

[在线演示](https://embeddedgui.github.io/EmbeddedGUI/basic.html?app=menu)

### API

| 函数 | 说明 |
|------|------|
| `egui_view_menu_init(self)` | 初始化 Menu |
| `egui_view_menu_init_with_params(self, params)` | 使用参数初始化 |
| `egui_view_menu_set_pages(self, pages, page_count)` | 设置菜单页面数据 |
| `egui_view_menu_navigate_to(self, page_index)` | 导航到指定页面 |
| `egui_view_menu_go_back(self)` | 返回上一级菜单 |
| `egui_view_menu_set_on_item_click(self, callback)` | 设置菜单项点击回调 |
| `egui_view_menu_set_header_height(self, height)` | 设置标题栏高度 |
| `egui_view_menu_set_item_height(self, height)` | 设置菜单项高度 |

### 参数宏

```c
EGUI_VIEW_MENU_PARAMS_INIT(name, x, y, w, h, header_h, item_h);
```

### 数据结构

```c
// 菜单项
typedef struct egui_view_menu_item
{
    const char *text;
    uint8_t sub_page_index; // 0xFF = 叶子节点(无子菜单)
} egui_view_menu_item_t;

// 菜单页
typedef struct egui_view_menu_page
{
    const char *title;
    const egui_view_menu_item_t *items;
    uint8_t item_count;
} egui_view_menu_page_t;
```

### 常量

| 常量 | 说明 |
|------|------|
| `EGUI_VIEW_MENU_MAX_PAGES` | 最大页面数(默认 8) |
| `EGUI_VIEW_MENU_MAX_ITEMS` | 每页最大菜单项数(默认 8) |
| `EGUI_VIEW_MENU_MAX_STACK` | 最大导航深度(默认 4) |
| `EGUI_VIEW_MENU_ITEM_LEAF` | 叶子节点标记(0xFF) |

### 代码示例

```c
static egui_view_menu_t menu;

static const egui_view_menu_item_t main_items[] = {
    {"Display", 1},                      // 进入子菜单
    {"Sound", EGUI_VIEW_MENU_ITEM_LEAF}, // 叶子节点
    {"About", EGUI_VIEW_MENU_ITEM_LEAF},
};

static const egui_view_menu_item_t display_items[] = {
    {"Brightness", EGUI_VIEW_MENU_ITEM_LEAF},
    {"Theme", EGUI_VIEW_MENU_ITEM_LEAF},
};

static const egui_view_menu_page_t pages[] = {
    {"Settings", main_items, 3},
    {"Display", display_items, 2},
};

EGUI_VIEW_MENU_PARAMS_INIT(menu_params, 0, 0, 200, 180, 30, 30);

static void on_item_click(egui_view_t *self,
    uint8_t page_index, uint8_t item_index)
{
    EGUI_LOG_INF("Menu: page=%d, item=%d\n",
        page_index, item_index);
}

void init_ui(void)
{
    egui_view_menu_init_with_params(
        EGUI_VIEW_OF(&menu), &menu_params);
    egui_view_menu_set_pages(EGUI_VIEW_OF(&menu), pages, 2);
    egui_view_menu_set_on_item_click(
        EGUI_VIEW_OF(&menu), on_item_click);

    egui_core_add_user_root_view(EGUI_VIEW_OF(&menu));
}
```

---

## PageIndicator

页面指示器控件，以圆点形式显示当前页面位置，常与 ViewPage 配合使用。当前页对应的圆点高亮显示。

### 效果展示

[在线演示](https://embeddedgui.github.io/EmbeddedGUI/basic.html?app=page_indicator)

### API

| 函数 | 说明 |
|------|------|
| `egui_view_page_indicator_init(self)` | 初始化 PageIndicator |
| `egui_view_page_indicator_init_with_params(self, params)` | 使用参数初始化 |
| `egui_view_page_indicator_set_total_count(self, total)` | 设置总页数 |
| `egui_view_page_indicator_set_current_index(self, index)` | 设置当前页索引 |

### 参数宏

```c
EGUI_VIEW_PAGE_INDICATOR_PARAMS_INIT(name, x, y, w, h, total, current);
```

### 代码示例

```c
static egui_view_page_indicator_t indicator;
static egui_view_viewpage_t viewpage;

EGUI_VIEW_PAGE_INDICATOR_PARAMS_INIT(
    ind_params, 80, 210, 80, 20, 4, 0);

static void on_page_changed(egui_view_t *self, int page_index)
{
    egui_view_page_indicator_set_current_index(
        EGUI_VIEW_OF(&indicator), (uint8_t)page_index);
}

void init_ui(void)
{
    // 初始化 ViewPage (省略页面添加)
    egui_view_viewpage_set_on_page_changed(
        EGUI_VIEW_OF(&viewpage), on_page_changed);

    // 初始化 PageIndicator
    egui_view_page_indicator_init_with_params(
        EGUI_VIEW_OF(&indicator), &ind_params);

    egui_core_add_user_root_view(EGUI_VIEW_OF(&viewpage));
    egui_core_add_user_root_view(EGUI_VIEW_OF(&indicator));
}
```
