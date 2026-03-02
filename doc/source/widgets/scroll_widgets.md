# 滚动控件

## 概述

滚动控件用于在有限的显示区域内展示超出屏幕范围的内容。EmbeddedGUI 提供了四种滚动容器：Scroll 支持垂直滚动，ViewPage 和 ViewPageCache 支持水平翻页，TileView 支持二维网格式的瓦片导航。这些控件都内置了触摸拖拽和惯性滑动支持。

---

## Scroll

垂直滚动容器，内部使用 LinearLayout 自动排列子控件。当子控件总高度超过容器高度时，支持触摸拖拽和惯性滚动。

### 效果展示

[在线演示](https://embeddedgui.github.io/EmbeddedGUI/basic.html?app=scroll)

### API

| 函数 | 说明 |
|------|------|
| `egui_view_scroll_init(self)` | 初始化 Scroll |
| `egui_view_scroll_init_with_params(self, params)` | 使用参数初始化 |
| `egui_view_scroll_add_child(self, child)` | 添加子控件 |
| `egui_view_scroll_set_size(self, width, height)` | 设置容器大小 |
| `egui_view_scroll_start_container_scroll(self, diff_y)` | 程序化滚动指定偏移 |
| `egui_view_scroll_fling(self, velocity_y)` | 以指定速度惯性滚动 |
| `egui_view_scroll_set_scrollbar_enabled(self, enabled)` | 启用滚动条(需开启 SCROLLBAR) |

### 参数宏

```c
EGUI_VIEW_SCROLL_PARAMS_INIT(name, x, y, w, h);
```

### 代码示例

```c
static egui_view_scroll_t scroll;
static egui_view_label_t items[10];

EGUI_VIEW_SCROLL_PARAMS_INIT(scroll_params, 0, 0, 200, 120);

void init_ui(void)
{
    egui_view_scroll_init_with_params(
        EGUI_VIEW_OF(&scroll), &scroll_params);

    char buf[16];
    for (int i = 0; i < 10; i++)
    {
        egui_view_label_init(EGUI_VIEW_OF(&items[i]));
        egui_view_set_size(EGUI_VIEW_OF(&items[i]), 180, 30);
        egui_view_label_set_text(EGUI_VIEW_OF(&items[i]), "Item");
        egui_view_set_margin_all(EGUI_VIEW_OF(&items[i]), 2);
        egui_view_scroll_add_child(
            EGUI_VIEW_OF(&scroll), EGUI_VIEW_OF(&items[i]));
    }

    egui_core_add_user_root_view(EGUI_VIEW_OF(&scroll));
}
```

### 配置选项

| 宏 | 说明 |
|----|------|
| `EGUI_CONFIG_FUNCTION_SUPPORT_SCROLLBAR` | 启用滚动条显示 |

---

## ViewPage

水平翻页容器，每次显示一个子页面，支持触摸左右滑动切换页面。适合实现引导页、Tab 页面等场景。

### 效果展示

[在线演示](https://embeddedgui.github.io/EmbeddedGUI/basic.html?app=viewpage)

### API

| 函数 | 说明 |
|------|------|
| `egui_view_viewpage_init(self)` | 初始化 ViewPage |
| `egui_view_viewpage_init_with_params(self, params)` | 使用参数初始化 |
| `egui_view_viewpage_add_child(self, child)` | 添加页面 |
| `egui_view_viewpage_remove_child(self, child)` | 移除页面 |
| `egui_view_viewpage_set_current_page(self, page_index)` | 直接跳转到指定页 |
| `egui_view_viewpage_scroll_to_page(self, page_index)` | 平滑滚动到指定页 |
| `egui_view_viewpage_set_on_page_changed(self, callback)` | 设置页面切换回调 |
| `egui_view_viewpage_set_size(self, width, height)` | 设置容器大小 |
| `egui_view_viewpage_set_scrollbar_enabled(self, enabled)` | 启用滚动条(需开启 SCROLLBAR) |

### 参数宏

```c
EGUI_VIEW_VIEWPAGE_PARAMS_INIT(name, x, y, w, h);
```

### 代码示例

```c
static egui_view_viewpage_t viewpage;
static egui_view_label_t page1, page2, page3;

EGUI_VIEW_VIEWPAGE_PARAMS_INIT(vp_params, 0, 0, 240, 200);

static void on_page_changed(egui_view_t *self, int page_index)
{
    EGUI_LOG_INF("Page changed to: %d\n", page_index);
}

void init_ui(void)
{
    egui_view_viewpage_init_with_params(
        EGUI_VIEW_OF(&viewpage), &vp_params);

    egui_view_label_init(EGUI_VIEW_OF(&page1));
    egui_view_set_size(EGUI_VIEW_OF(&page1), 240, 200);
    egui_view_label_set_text(EGUI_VIEW_OF(&page1), "Page 1");

    egui_view_label_init(EGUI_VIEW_OF(&page2));
    egui_view_set_size(EGUI_VIEW_OF(&page2), 240, 200);
    egui_view_label_set_text(EGUI_VIEW_OF(&page2), "Page 2");

    egui_view_label_init(EGUI_VIEW_OF(&page3));
    egui_view_set_size(EGUI_VIEW_OF(&page3), 240, 200);
    egui_view_label_set_text(EGUI_VIEW_OF(&page3), "Page 3");

    egui_view_viewpage_add_child(
        EGUI_VIEW_OF(&viewpage), EGUI_VIEW_OF(&page1));
    egui_view_viewpage_add_child(
        EGUI_VIEW_OF(&viewpage), EGUI_VIEW_OF(&page2));
    egui_view_viewpage_add_child(
        EGUI_VIEW_OF(&viewpage), EGUI_VIEW_OF(&page3));

    egui_view_viewpage_set_on_page_changed(
        EGUI_VIEW_OF(&viewpage), on_page_changed);

    egui_core_add_user_root_view(EGUI_VIEW_OF(&viewpage));
}
```

### 配置选项

| 宏 | 说明 |
|----|------|
| `EGUI_CONFIG_FUNCTION_SUPPORT_SCROLLBAR` | 启用滚动条显示 |

---

## ViewPageCache

带缓存机制的翻页容器，适用于页面数量较多的场景。仅缓存当前页及相邻页面(最多 3 页)，通过回调动态加载和释放页面，大幅降低内存占用。

### 效果展示

[在线演示](https://embeddedgui.github.io/EmbeddedGUI/basic.html?app=viewpage_cache)

### API

| 函数 | 说明 |
|------|------|
| `egui_view_viewpage_cache_init(self)` | 初始化 ViewPageCache |
| `egui_view_viewpage_cache_init_with_params(self, params)` | 使用参数初始化 |
| `egui_view_viewpage_cache_set_child_total_cnt(self, cnt)` | 设置总页面数 |
| `egui_view_viewpage_cache_set_current_page(self, page_index)` | 直接跳转到指定页 |
| `egui_view_viewpage_cache_scroll_to_page(self, page_index)` | 平滑滚动到指定页 |
| `egui_view_viewpage_cache_set_on_page_changed_listener(self, listener)` | 设置页面切换回调 |
| `egui_view_viewpage_cache_set_on_page_load_listener(self, listener)` | 设置页面加载回调 |
| `egui_view_viewpage_cache_set_on_page_free_listener(self, listener)` | 设置页面释放回调 |
| `egui_view_viewpage_cache_set_size(self, width, height)` | 设置容器大小 |
| `egui_view_viewpage_cache_on_paged_free_all(self)` | 释放所有缓存页面 |

### 参数宏

```c
EGUI_VIEW_VIEWPAGE_CACHE_PARAMS_INIT(name, x, y, w, h);
```

### 回调函数签名

```c
// 页面切换回调
typedef void (*egui_view_viewpage_cache_on_page_changed_listener_t)(
    egui_view_t *self, int current_page_index);

// 页面加载回调 - 返回创建的页面视图指针
typedef void *(*egui_view_viewpage_cache_on_page_load_listener_t)(
    egui_view_t *self, int current_page_index);

// 页面释放回调
typedef void (*egui_view_viewpage_cache_on_page_free_listener_t)(
    egui_view_t *self, int current_page_index, egui_view_t *page);
```

### 代码示例

```c
static egui_view_viewpage_cache_t vp_cache;
static egui_view_label_t cached_pages[3];

EGUI_VIEW_VIEWPAGE_CACHE_PARAMS_INIT(vpc_params, 0, 0, 240, 200);

static void *on_page_load(egui_view_t *self, int page_index)
{
    int cache_idx = page_index % 3;
    egui_view_label_init(EGUI_VIEW_OF(&cached_pages[cache_idx]));
    egui_view_set_size(
        EGUI_VIEW_OF(&cached_pages[cache_idx]), 240, 200);
    egui_view_label_set_text(
        EGUI_VIEW_OF(&cached_pages[cache_idx]), "Cached Page");
    return &cached_pages[cache_idx];
}

static void on_page_free(egui_view_t *self, int page_index,
    egui_view_t *page)
{
    // 释放页面资源
}

void init_ui(void)
{
    egui_view_viewpage_cache_init_with_params(
        EGUI_VIEW_OF(&vp_cache), &vpc_params);
    egui_view_viewpage_cache_set_child_total_cnt(
        EGUI_VIEW_OF(&vp_cache), 10);
    egui_view_viewpage_cache_set_on_page_load_listener(
        EGUI_VIEW_OF(&vp_cache), on_page_load);
    egui_view_viewpage_cache_set_on_page_free_listener(
        EGUI_VIEW_OF(&vp_cache), on_page_free);
    egui_view_viewpage_cache_set_current_page(
        EGUI_VIEW_OF(&vp_cache), 0);

    egui_core_add_user_root_view(EGUI_VIEW_OF(&vp_cache));
}
```

---

## TileView

二维瓦片导航容器，子视图按行列网格排列，支持上下左右四方向滑动切换。适合智能手表等需要多方向导航的场景。

### 效果展示

[在线演示](https://embeddedgui.github.io/EmbeddedGUI/basic.html?app=tileview)

### API

| 函数 | 说明 |
|------|------|
| `egui_view_tileview_init(self)` | 初始化 TileView |
| `egui_view_tileview_init_with_params(self, params)` | 使用参数初始化 |
| `egui_view_tileview_add_tile(self, tile_view, col, row)` | 在指定行列添加瓦片 |
| `egui_view_tileview_set_current(self, col, row)` | 切换到指定行列的瓦片 |
| `egui_view_tileview_set_on_changed(self, callback)` | 设置瓦片切换回调 |

### 参数宏

```c
EGUI_VIEW_TILEVIEW_PARAMS_INIT(name, x, y, w, h);
```

### 常量

| 常量 | 说明 |
|------|------|
| `EGUI_VIEW_TILEVIEW_MAX_TILES` | 最大瓦片数量(默认 9) |
| `EGUI_VIEW_TILEVIEW_SWIPE_THRESH` | 滑动触发阈值(默认 20 像素) |

### 代码示例

```c
static egui_view_tileview_t tileview;
static egui_view_label_t tile_main, tile_right, tile_bottom;

EGUI_VIEW_TILEVIEW_PARAMS_INIT(tv_params, 0, 0, 240, 240);

static void on_tile_changed(egui_view_t *self,
    uint8_t col, uint8_t row)
{
    EGUI_LOG_INF("Tile: col=%d, row=%d\n", col, row);
}

void init_ui(void)
{
    egui_view_tileview_init_with_params(
        EGUI_VIEW_OF(&tileview), &tv_params);

    egui_view_label_init(EGUI_VIEW_OF(&tile_main));
    egui_view_set_size(EGUI_VIEW_OF(&tile_main), 240, 240);
    egui_view_label_set_text(EGUI_VIEW_OF(&tile_main), "Main");

    egui_view_label_init(EGUI_VIEW_OF(&tile_right));
    egui_view_set_size(EGUI_VIEW_OF(&tile_right), 240, 240);
    egui_view_label_set_text(EGUI_VIEW_OF(&tile_right), "Right");

    egui_view_label_init(EGUI_VIEW_OF(&tile_bottom));
    egui_view_set_size(EGUI_VIEW_OF(&tile_bottom), 240, 240);
    egui_view_label_set_text(EGUI_VIEW_OF(&tile_bottom), "Bottom");

    // col=0,row=0: 主页; col=1,row=0: 右侧; col=0,row=1: 下方
    egui_view_tileview_add_tile(
        EGUI_VIEW_OF(&tileview), EGUI_VIEW_OF(&tile_main), 0, 0);
    egui_view_tileview_add_tile(
        EGUI_VIEW_OF(&tileview), EGUI_VIEW_OF(&tile_right), 1, 0);
    egui_view_tileview_add_tile(
        EGUI_VIEW_OF(&tileview), EGUI_VIEW_OF(&tile_bottom), 0, 1);

    egui_view_tileview_set_on_changed(
        EGUI_VIEW_OF(&tileview), on_tile_changed);
    egui_view_tileview_set_current(EGUI_VIEW_OF(&tileview), 0, 0);

    egui_core_add_user_root_view(EGUI_VIEW_OF(&tileview));
}
```
