# API 与接入模板

## 通用接入步骤

所有 virtual 容器的使用节奏都高度一致，可以概括成五步。

## 1. 定义 `params`

`params` 描述的是容器自身，而不是数据内容。常见字段包括：

- `region`
- `overscan_before`
- `overscan_after`
- `max_keepalive_slots`
- `estimated_*`

这些字段分别负责：

- 容器位置与大小
- 可见区前后的超扫量
- keepalive 上限
- 初始阶段的预估尺寸

## 2. 定义 `data_source` 或 `adapter`

高层容器用 `*_data_source_t`，底层 `virtual_viewport` 用 `adapter`。  
它们都需要回答这些问题：

- 一共有多少项
- 索引对应哪个 `stable_id`
- 能否通过 `stable_id` 反查索引
- 当前项对应什么 `view_type`
- 当前项需要多大主轴尺寸
- 如何创建 view
- 如何绑定 view
- 如何解绑 view
- 是否需要 keepalive
- 如何保存和恢复状态

## 3. 优先使用 `setup`

推荐优先使用：

- `*_init_with_setup()`
- `*_apply_setup()`

而不是逐个 setter 配置。  
因为 `setup` 可以把以下内容一次性收口：

- `params`
- `data_source`
- `data_source_context`
- `state_cache_max_entries`
- `state_cache_max_bytes`

典型模式：

```c
static egui_view_virtual_grid_t grid_view;
static app_context_t grid_ctx;

static const egui_view_virtual_grid_setup_t grid_setup = {
        .params = &grid_params,
        .data_source = &grid_source,
        .data_source_context = &grid_ctx,
        .state_cache_max_entries = 96,
        .state_cache_max_bytes = 96 * sizeof(my_item_state_t),
};

egui_view_virtual_grid_init_with_setup(EGUI_VIEW_OF(&grid_view), &grid_setup);
```

## 4. 在 `bind` 里只做当前项的视觉绑定

推荐在 `bind` 阶段做这些事：

- 设置文本
- 设置颜色
- 设置图标
- 调整当前项的局部布局
- 恢复当前项的临时状态

不建议在 `bind` 阶段做这些事：

- 遍历全量数据集
- 直接改动其他项的数据
- 把索引当长期身份使用

## 5. 数据变化后发送精确通知

最常见的问题不是“控件不支持”，而是“业务层没有发对通知”。  
只要通知发对了，slot 复用、布局修正和滚动锚点通常都能稳定工作。

---

## API 命名规律

为了降低学习成本，high-level wrapper 的命名非常统一。学会一个之后，迁移到其他容器时一般只需替换业务名词。

### 初始化类

- `*_apply_params()`
- `*_init_with_params()`
- `*_apply_setup()`
- `*_init_with_setup()`

### 数据源查询类

- `get_data_source()`
- `get_data_source_context()`

raw viewport 对应：

- `get_adapter()`
- `get_adapter_context()`

### 滚动与定位类

- `scroll_by(...)`
- `scroll_to_*()`
- `scroll_to_*_by_stable_id()`
- `get_scroll_x()` / `get_scroll_y()` / `get_logical_offset()`
- `find_*_by_stable_id()`
- `resolve_*_by_view()`
- `find_view_by_stable_id()`
- `ensure_*_visible_by_stable_id()`

### 可见项遍历类

- `visit_visible_*()`
- `find_first_visible_*_view()`
- `get_slot_count()`
- `get_slot()`
- `find_slot_by_stable_id()`

### 状态缓存类

- `set_state_cache_limits(...)`
- `get_state_cache_entry_limit(...)`
- `get_state_cache_byte_limit(...)`
- `clear_*_state_cache()`
- `remove_*_state_by_stable_id()`
- `write_*_state(...)`
- `read_*_state(...)`
- `write_*_state_for_view(...)`
- `read_*_state_for_view(...)`

现在 raw viewport 和所有高层容器都统一提供了 state cache limit 的 getter，便于：

- 运行时自检
- 单元测试
- 场景切换后确认 setup 是否正确写入

---

## 各容器最小接入模板

下面给出每类容器最小骨架与最常用 helper。

## 1. `virtual_viewport`

头文件：

- `src/widget/egui_view_virtual_viewport.h`

适合：

- 自定义画布块
- 聊天气泡
- 看板卡片
- 不适合被命名为 list/page/grid/tree 的异构内容

推荐入门 case：

- `example/HelloVirtual/virtual_viewport_basic/`
- `example/HelloVirtual/virtual_viewport/`

```c
static egui_view_virtual_viewport_t viewport;
static app_context_t viewport_ctx;

static const egui_view_virtual_viewport_params_t viewport_params = {
        .region = {{8, 72}, {224, 240}},
        .orientation = EGUI_VIEW_VIRTUAL_VIEWPORT_ORIENTATION_VERTICAL,
        .overscan_before = 1,
        .overscan_after = 1,
        .max_keepalive_slots = 4,
        .estimated_item_extent = 72,
};

static const egui_view_virtual_viewport_adapter_t viewport_adapter = {
        .get_count = app_get_count,
        .get_stable_id = app_get_stable_id,
        .find_index_by_stable_id = app_find_index,
        .get_view_type = app_get_view_type,
        .measure_main_size = app_measure_main_size,
        .create_view = app_create_view,
        .destroy_view = app_destroy_view,
        .bind_view = app_bind_view,
        .unbind_view = app_unbind_view,
        .should_keep_alive = app_should_keep_alive,
        .save_state = app_save_state,
        .restore_state = app_restore_state,
};

static const egui_view_virtual_viewport_setup_t viewport_setup = {
        .params = &viewport_params,
        .adapter = &viewport_adapter,
        .adapter_context = &viewport_ctx,
        .state_cache_max_entries = 64,
        .state_cache_max_bytes = 64 * sizeof(my_item_state_t),
};

egui_view_virtual_viewport_init_with_setup(EGUI_VIEW_OF(&viewport), &viewport_setup);
```

最常用 helper：

- `egui_view_virtual_viewport_resolve_item_by_view()`
- `egui_view_virtual_viewport_find_view_by_stable_id()`
- `egui_view_virtual_viewport_ensure_item_visible_by_stable_id()`
- `egui_view_virtual_viewport_visit_visible_items()`
- `egui_view_virtual_viewport_find_first_visible_item_view()`

## 2. `virtual_list`

头文件：

- `src/widget/egui_view_virtual_list.h`

```c
static egui_view_virtual_list_t list_view;
static app_context_t list_ctx;

static const egui_view_virtual_list_data_source_t list_source = {
        .get_count = app_get_count,
        .get_stable_id = app_get_stable_id,
        .find_index_by_stable_id = app_find_index,
        .get_view_type = app_get_view_type,
        .measure_item_height = app_measure_item_height,
        .create_item_view = app_create_item_view,
        .bind_item_view = app_bind_item_view,
        .unbind_item_view = app_unbind_item_view,
        .should_keep_alive = app_should_keep_alive,
        .save_item_state = app_save_item_state,
        .restore_item_state = app_restore_item_state,
        .default_view_type = 0,
};
```

最常用 helper：

- `egui_view_virtual_list_resolve_item_by_view()`
- `egui_view_virtual_list_find_view_by_stable_id()`
- `egui_view_virtual_list_ensure_item_visible_by_stable_id()`
- `egui_view_virtual_list_notify_item_changed_by_stable_id()`
- `egui_view_virtual_list_notify_item_resized_by_stable_id()`

## 3. `virtual_page`

头文件：

- `src/widget/egui_view_virtual_page.h`

关键点：

- 最小单元是 section，不是 row
- section 可以是高度异构的大模块

推荐入门 case：

- `example/HelloVirtual/virtual_page_basic/`
- `example/HelloVirtual/virtual_page/`

```c
static egui_view_virtual_page_t page_view;
static app_context_t page_ctx;

static const egui_view_virtual_page_data_source_t page_source = {
        .get_count = app_get_section_count,
        .get_stable_id = app_get_section_stable_id,
        .find_index_by_stable_id = app_find_section_index,
        .get_section_type = app_get_section_type,
        .measure_section_height = app_measure_section_height,
        .create_section_view = app_create_section_view,
        .bind_section_view = app_bind_section_view,
        .unbind_section_view = app_unbind_section_view,
        .should_keep_alive = app_should_keep_alive,
        .save_section_state = app_save_section_state,
        .restore_section_state = app_restore_section_state,
        .default_section_type = 0,
};
```

最常用 helper：

- `egui_view_virtual_page_resolve_section_by_view()`
- `egui_view_virtual_page_scroll_to_section_by_stable_id()`
- `egui_view_virtual_page_ensure_section_visible_by_stable_id()`
- `egui_view_virtual_page_notify_section_resized_by_stable_id()`

## 4. `virtual_strip`

头文件：

- `src/widget/egui_view_virtual_strip.h`

推荐入门 case：

- `example/HelloVirtual/virtual_strip_basic/`
- `example/HelloVirtual/virtual_strip/`

关键点：

- 主轴是横向
- 测量函数是 `measure_item_width`

```c
static egui_view_virtual_strip_t strip_view;
static app_context_t strip_ctx;

static const egui_view_virtual_strip_data_source_t strip_source = {
        .get_count = app_get_count,
        .get_stable_id = app_get_stable_id,
        .find_index_by_stable_id = app_find_index,
        .get_item_view_type = app_get_item_view_type,
        .measure_item_width = app_measure_item_width,
        .create_item_view = app_create_item_view,
        .bind_item_view = app_bind_item_view,
        .unbind_item_view = app_unbind_item_view,
        .should_keep_alive = app_should_keep_alive,
        .save_item_state = app_save_item_state,
        .restore_item_state = app_restore_item_state,
        .default_item_view_type = 0,
};
```

最常用 helper：

- `egui_view_virtual_strip_resolve_item_by_view()`
- `egui_view_virtual_strip_get_item_x_by_stable_id()`
- `egui_view_virtual_strip_get_item_width_by_stable_id()`
- `egui_view_virtual_strip_scroll_to_stable_id()`
- `egui_view_virtual_strip_ensure_item_visible_by_stable_id()`

## 5. `virtual_grid`

头文件：

- `src/widget/egui_view_virtual_grid.h`

推荐入门 case：

- `example/HelloVirtual/virtual_grid_basic/`
- `example/HelloVirtual/virtual_grid/`

关键点：

- slot 语义是 row slot，不是 item slot
- item 高度可以依赖列宽重新测量

```c
static egui_view_virtual_grid_t grid_view;
static app_context_t grid_ctx;

static const egui_view_virtual_grid_data_source_t grid_source = {
        .get_count = app_get_count,
        .get_stable_id = app_get_stable_id,
        .find_index_by_stable_id = app_find_index,
        .get_item_view_type = app_get_item_view_type,
        .measure_item_height = app_measure_item_height,
        .create_item_view = app_create_item_view,
        .bind_item_view = app_bind_item_view,
        .unbind_item_view = app_unbind_item_view,
        .should_keep_alive = app_should_keep_alive,
        .save_item_state = app_save_item_state,
        .restore_item_state = app_restore_item_state,
        .default_item_view_type = 0,
};
```

最常用 helper：

- `egui_view_virtual_grid_resolve_item_by_view()`
- `egui_view_virtual_grid_get_item_x_by_stable_id()`
- `egui_view_virtual_grid_get_item_y_by_stable_id()`
- `egui_view_virtual_grid_get_slot_item_count()`
- `egui_view_virtual_grid_get_slot_item_view()`
- `egui_view_virtual_grid_ensure_item_visible_by_stable_id()`

## 6. `virtual_section_list`

头文件：

- `src/widget/egui_view_virtual_section_list.h`

推荐入门 case：

- `example/HelloVirtual/virtual_section_list_basic/`
- `example/HelloVirtual/virtual_section_list/`

关键点：

- header 与 item 都是一级公民
- 可以分别测量、分别缓存、分别通知

```c
static egui_view_virtual_section_list_t section_list;
static app_context_t section_ctx;

static const egui_view_virtual_section_list_data_source_t section_source = {
        .get_section_count = app_get_section_count,
        .get_section_stable_id = app_get_section_stable_id,
        .find_section_index_by_stable_id = app_find_section_index,
        .get_item_count = app_get_item_count,
        .get_item_stable_id = app_get_item_stable_id,
        .find_item_position_by_stable_id = app_find_item_position,
        .get_section_header_view_type = app_get_header_view_type,
        .get_item_view_type = app_get_item_view_type,
        .measure_section_header_height = app_measure_header_height,
        .measure_item_height = app_measure_item_height,
        .create_section_header_view = app_create_header_view,
        .create_item_view = app_create_item_view,
        .bind_section_header_view = app_bind_header_view,
        .bind_item_view = app_bind_item_view,
        .default_section_header_view_type = 0,
        .default_item_view_type = 0,
};
```

最常用 helper：

- `egui_view_virtual_section_list_resolve_entry_by_view()`
- `egui_view_virtual_section_list_find_item_position_by_stable_id()`
- `egui_view_virtual_section_list_ensure_entry_visible_by_stable_id()`
- `egui_view_virtual_section_list_notify_section_header_changed_by_stable_id()`
- `egui_view_virtual_section_list_notify_item_resized_by_stable_id()`

## 7. `virtual_tree`

头文件：

- `src/widget/egui_view_virtual_tree.h`

推荐入门 case：

- `example/HelloVirtual/virtual_tree_basic/`
- `example/HelloVirtual/virtual_tree/`

关键点：

- 数据源需要回答根节点、子节点和展开态
- helper 很多围绕可见节点流工作

```c
static egui_view_virtual_tree_t tree_view;
static app_context_t tree_ctx;

static const egui_view_virtual_tree_data_source_t tree_source = {
        .get_root_count = app_get_root_count,
        .get_root_stable_id = app_get_root_stable_id,
        .get_child_count = app_get_child_count,
        .get_child_stable_id = app_get_child_stable_id,
        .is_node_expanded = app_is_node_expanded,
        .get_node_view_type = app_get_node_view_type,
        .measure_node_height = app_measure_node_height,
        .create_node_view = app_create_node_view,
        .bind_node_view = app_bind_node_view,
        .unbind_node_view = app_unbind_node_view,
        .should_keep_alive = app_should_keep_alive,
        .save_node_state = app_save_node_state,
        .restore_node_state = app_restore_node_state,
        .default_view_type = 0,
};
```

最常用 helper：

- `egui_view_virtual_tree_resolve_node_by_view()`
- `egui_view_virtual_tree_find_visible_index_by_stable_id()`
- `egui_view_virtual_tree_scroll_to_node_by_stable_id()`
- `egui_view_virtual_tree_ensure_node_visible_by_stable_id()`
- `egui_view_virtual_tree_notify_node_resized_by_stable_id()`

---

## 点击、定位与可见项遍历

围绕 `stable_id` 使用 helper，是最稳妥的做法。

例如点击命中：

```c
static void card_click_cb(egui_view_t *self)
{
    egui_view_virtual_grid_entry_t entry;

    if (!egui_view_virtual_grid_resolve_item_by_view(EGUI_VIEW_OF(&grid_view), self, &entry))
    {
        return;
    }

    select_item(entry.index, entry.stable_id);
}
```

例如把某个目标项尽量保持在视口内：

```c
egui_view_virtual_grid_ensure_item_visible_by_stable_id(EGUI_VIEW_OF(&grid_view), stable_id, inset);
```

例如遍历当前可见项：

```c
egui_view_virtual_viewport_visit_visible_items(EGUI_VIEW_OF(&viewport), visitor, context);
```

---

## 动画、选中态和临时状态如何避免丢失

推荐分层如下：

| 状态类型 | 推荐存放位置 |
| --- | --- |
| 长期业务状态 | 业务模型 |
| 可恢复的临时 UI 状态 | `state cache` |
| 必须保留同一对象实例的短期状态 | `keepalive` |

### 什么时候优先用 `keepalive`

- 当前项正在播放关键动画
- 当前项处在复杂编辑态
- 当前项短时间内会持续交互

### 什么时候优先用 `state cache`

- 展开/折叠态
- pulse 动画阶段
- 某个临时编辑值
- 某个局部高亮状态

如果动画本质上可以数据驱动，推荐保存：

- 动画起始 tick
- phase
- progress

而不是把所有动画内部变量都压在某个 view 私有对象上。

---

## 推荐验证流程

virtual 控件不能只看编译通过，必须配合运行时和渲染检查。

推荐流程：

1. 编译目标示例
2. 运行 `code_runtime_check.py`
3. 检查截图
4. 观察滚动、点击、选中、插入、删除、patch、jump 后的效果
5. 运行 `HelloUnitTest`

常用命令：

```bash
make -j1 all APP=HelloVirtual APP_SUB=virtual_grid PORT=pc
python scripts/code_runtime_check.py --app HelloVirtual --app-sub virtual_grid --keep-screenshots

make -j1 all APP=HelloUnitTest PORT=pc_test
output\main.exe
```

渲染检查时重点看：

- 文本是否压边或越界
- 半截 item 在滚动裁剪时是否稳定
- 点击后选中态是否清晰
- patch 后是否有错位
- 不同容器是否真的体现出各自语义

---

## 相关文件速查

示例目录：

- `example/HelloVirtual/virtual_viewport_basic/`
- `example/HelloVirtual/virtual_viewport/`
- `example/HelloVirtual/virtual_page_basic/`
- `example/HelloVirtual/virtual_page/`
- `example/HelloVirtual/virtual_strip_basic/`
- `example/HelloVirtual/virtual_strip/`
- `example/HelloVirtual/virtual_grid_basic/`
- `example/HelloVirtual/virtual_grid/`
- `example/HelloVirtual/virtual_section_list_basic/`
- `example/HelloVirtual/virtual_section_list/`
- `example/HelloVirtual/virtual_tree_basic/`
- `example/HelloVirtual/virtual_tree/`

核心头文件：

- `src/widget/egui_view_virtual_viewport.h`
- `src/widget/egui_view_virtual_list.h`
- `src/widget/egui_view_virtual_page.h`
- `src/widget/egui_view_virtual_strip.h`
- `src/widget/egui_view_virtual_grid.h`
- `src/widget/egui_view_virtual_section_list.h`
- `src/widget/egui_view_virtual_tree.h`

如果现有六类高层容器都不贴切，建议先从 `virtual_viewport` 起步；等业务语义稳定之后，再决定是否值得继续往上抽象新的 wrapper。

## 8. `virtual_stage`

头文件与示例：

- `src/widget/egui_view_virtual_stage.h`
- `example/HelloVirtual/virtual_stage_basic/test.c`
- `example/HelloVirtual/virtual_stage_showcase/test.c`
- `example/HelloVirtual/virtual_stage/test.c`
- `doc/source/architecture/virtual_stage.md`

适合：

- 固定大画布里的绝对定位节点
- 大量 render-only 节点 + 少量真实交互控件
- 希望把状态尽量外置到业务层，而不是让所有节点都常驻 view

推荐先看 `virtual_stage_basic`，它展示的是最小闭环：

```c
typedef struct app_stage_node
{
    egui_virtual_stage_node_desc_t desc;
} app_stage_node_t;

static egui_view_virtual_stage_t stage_view;
static app_context_t stage_ctx;
static app_stage_node_t stage_nodes[12];

EGUI_VIEW_VIRTUAL_STAGE_NODE_ARRAY_INTERACTIVE_BRIDGE_INIT_WITH_LIMIT(stage_bridge, 8, 80, 304, 200, 3, stage_nodes, app_stage_node_t, desc,
                                                                      app_stage_create_view, app_stage_destroy_view, app_stage_bind_view,
                                                                      app_stage_draw_node, app_stage_hit_test, app_stage_should_keep_alive, &stage_ctx);

EGUI_VIEW_VIRTUAL_STAGE_INIT_ARRAY_BRIDGE(&stage_view, &stage_bridge);
EGUI_VIEW_VIRTUAL_STAGE_ADD_ROOT(&stage_view);
```

如果节点集合本身就是固定数组，而且回调也就在当前文件里，推荐直接走这条一步式 bridge，不再手写 `get_count / get_desc`，也不用再自己拼 `adapter + ops + setup`。

如果 stage 本身就是整屏页面，再进一步用 `EGUI_VIEW_VIRTUAL_STAGE_NODE_ARRAY_SCREEN_*_BRIDGE_INIT_WITH_LIMIT(...)`，把整屏区域参数也一起收掉。

如果 `params` 已经单独存在，例如你还想在别处复用或切换 `live_slot_limit`，或者 `ops` 需要跨文件导出，再回退到 `EGUI_VIEW_VIRTUAL_STAGE_NODE_ARRAY_BRIDGE_INIT(...) + EGUI_VIEW_VIRTUAL_STAGE_ARRAY_OPS_*_INIT(...)` 这套分层 helper。

`array ops` 也建议按复杂度选 helper：

- 只有 `create/bind/draw` 用 `EGUI_VIEW_VIRTUAL_STAGE_NODE_ARRAY_SIMPLE_BRIDGE_INIT_WITH_LIMIT(...)`
- 再带 `hit_test/should_keep_alive` 用 `EGUI_VIEW_VIRTUAL_STAGE_NODE_ARRAY_INTERACTIVE_BRIDGE_INIT_WITH_LIMIT(...)`
- 还要跨 materialize 保存控件状态时用 `EGUI_VIEW_VIRTUAL_STAGE_NODE_ARRAY_STATEFUL_BRIDGE_INIT_WITH_LIMIT(...)`
- 如果 `ops` 要跨文件导出，再用对应的 `EGUI_VIEW_VIRTUAL_STAGE_ARRAY_OPS_*_CONST_INIT(...)`
- 如果 stage 变量本身就是 `egui_view_virtual_stage_t`，初始化和通知优先用 `EGUI_VIEW_VIRTUAL_STAGE_INIT_ARRAY_BRIDGE(...)`、`EGUI_VIEW_VIRTUAL_STAGE_ADD_ROOT(...)`、`EGUI_VIEW_VIRTUAL_STAGE_SET_BACKGROUND(...)`、`EGUI_VIEW_VIRTUAL_STAGE_REQUEST_LAYOUT(...)`、`EGUI_VIEW_VIRTUAL_STAGE_NOTIFY_NODE(...)`、`EGUI_VIEW_VIRTUAL_STAGE_NOTIFY_IDS(...)`、`EGUI_VIEW_VIRTUAL_STAGE_NOTIFY_NODES(...)`、`EGUI_VIEW_VIRTUAL_STAGE_NOTIFY_BOUNDS_IDS(...)`、`EGUI_VIEW_VIRTUAL_STAGE_NOTIFY_NODES_BOUNDS(...)`、`EGUI_VIEW_VIRTUAL_STAGE_PIN_IDS(...)`、`EGUI_VIEW_VIRTUAL_STAGE_UNPIN_IDS(...)`、`EGUI_VIEW_VIRTUAL_STAGE_TOGGLE_PIN(...)` 这些 convenience 宏，少写一层 `EGUI_VIEW_OF(...)`
- 如果一次只联动少量 `stable_id`，优先直接用 `EGUI_VIEW_VIRTUAL_STAGE_NOTIFY_IDS(...)` / `EGUI_VIEW_VIRTUAL_STAGE_NOTIFY_BOUNDS_IDS(...)`；如果 `stable_id` 数组要复用，继续用 `EGUI_VIEW_VIRTUAL_STAGE_NOTIFY_NODES(...)` / `EGUI_VIEW_VIRTUAL_STAGE_NOTIFY_NODES_BOUNDS(...)`

如果你想更快对照“showcase 风格页面如何落到 `virtual_stage`”，再看 `virtual_stage_showcase`：

- 深色大画布和多 panel 布局更接近 `HelloShowcase`
- panel、标题、状态卡片继续走 `draw_node()`
- `Button / ComboBox / Switch / Checkbox / ProgressBar / CircularProgressBar / Slider / NumberPicker` 这 8 个控件保持真实 widget
- 适合回答“stage 里能不能直接放原本控件”这个问题

`virtual_stage` 的几个高频 helper：

- `egui_view_virtual_stage_resolve_node_by_view()`
- `EGUI_VIEW_VIRTUAL_STAGE_RESOLVE_ID_BY_VIEW(...)`
- `egui_view_virtual_stage_find_slot_by_stable_id()`
- `egui_view_virtual_stage_find_view_by_stable_id()`
- `EGUI_VIEW_VIRTUAL_STAGE_INIT_ARRAY_BRIDGE(...)`
- `EGUI_VIEW_VIRTUAL_STAGE_ADD_ROOT(...)`
- `egui_view_virtual_stage_is_node_pinned()`
- `EGUI_VIEW_VIRTUAL_STAGE_PIN(...)`
- `EGUI_VIEW_VIRTUAL_STAGE_PIN_IDS(...)`
- `EGUI_VIEW_VIRTUAL_STAGE_UNPIN_IDS(...)`
- `EGUI_VIEW_VIRTUAL_STAGE_TOGGLE_PIN(...)`
- `EGUI_VIEW_VIRTUAL_STAGE_NOTIFY_NODE(...)`
- `EGUI_VIEW_VIRTUAL_STAGE_NOTIFY_NODE_BOUNDS(...)`
- `EGUI_VIEW_VIRTUAL_STAGE_NOTIFY_IDS(...)`
- `EGUI_VIEW_VIRTUAL_STAGE_NOTIFY_NODES(...)`
- `EGUI_VIEW_VIRTUAL_STAGE_NOTIFY_BOUNDS_IDS(...)`
- `EGUI_VIEW_VIRTUAL_STAGE_NOTIFY_NODES_BOUNDS(...)`
- `EGUI_VIEW_VIRTUAL_STAGE_NOTIFY_DATA(...)`

补充约束：

- `EGUI_VIEW_VIRTUAL_STAGE_PIN(...)` 只会接受“当前数据集里存在且可 materialize”的节点；render-only 节点、hidden 节点或不存在的 `stable_id` 都会返回失败。
- `EGUI_VIRTUAL_STAGE_NODE_FLAG_INTERACTIVE` 只有在节点本身能 materialize 成真实 view 时才有意义；render-only 节点即使误带这个 flag，也不会参与触摸命中。
- 如果实现了 `adapter->hit_test()` 来裁剪热点区域，`DOWN` 选中节点后，后续 `MOVE/UP` 也会继续按热点内外处理；拖出热点会取消按压，拖回热点后可恢复。
- 如果一个已 capture 的节点在交互过程中被改成 noninteractive，`virtual_stage` 会向旧 view 发送 cancel 语义并终止这次点击，不会在 `UP` 时误触发旧控件。
- 如果一次 `notify_data_changed()` 或 `set_adapter()` 会在 capture 尚未结束时移除当前节点，`virtual_stage` 也会先 cancel 旧 view，再回收或复用 slot，避免 pooled view 残留 pressed 等临时状态。
- 如果某个节点原来没有 live slot，但你把它改成了 `KEEPALIVE`、或者让一个已 pin 节点从 hidden/零尺寸恢复为可见状态，更新 desc 后要继续调用 `notify_node_changed()` 或 `notify_node_bounds_changed()`，让 stage 重新 materialize 它。

它和 `virtual_viewport` 家族的最大差异是：这里没有“滚动数据集”的 `data_source` 语义，只有“固定页面节点”的 `adapter` 语义。

第一次接 `virtual_stage` 时，可以把 callback 先分成三层来理解：

- 最小必需：`create_view`、`bind_view`
- 常见补充：`draw_node`
- 按需再补：`hit_test`、`save_state`、`restore_state`、`should_keep_alive`

默认缺省行为也比较直接：

- 不写 `hit_test` 时默认按节点矩形命中
- 不写 `should_keep_alive` 时默认不额外保活
- 不写 `save_state` / `restore_state` 时，回收后的业务状态由接入方自己负责
