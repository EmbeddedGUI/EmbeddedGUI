# Virtual Viewport / Virtual List / Virtual Page

## 这个示例在展示什么

这个示例不是单一大列表，而是一个多场景演示页，用同一套虚拟化底座覆盖三类常见业务：

- `Feed`
  - 1000+ 条监控流 / 动态卡片
  - 不同 item 高度
  - 点击展开
  - 脉冲动画 + keepalive
- `Chat`
  - 对话流 / 时间线
  - 左右不同气泡布局
  - typing / unread 状态
  - 程序化跳转到指定消息
- `Ops`
  - 任务队列 / 工单流
  - 进度条、告警态、完成态
  - 插入、删除、移动、刷新后的稳定滚动

顶部按钮 `Feed / Chat / Ops` 用来切换数据模型。  
工具按钮 `Add / Del / Move / Patch / Jump` 分别对应插入、删除、移动、局部更新和跳转。

## 这套能力解决什么问题

- 子 view 只在进入可见区时创建和绑定
- 槽位数被限制在 `EGUI_VIEW_VIRTUAL_VIEWPORT_MAX_SLOTS`
- 支持超过 1000 个 item
- 支持 item 高度变化、位置变化、插入删除移动
- 支持点击，能确认具体点击到了哪个 item
- 支持动画场景下的 keepalive，避免频繁销毁导致状态丢失

## 推荐怎么用

### 1. 普通纵向列表优先用 `virtual_list`

`virtual_list` 是面向“行列表”的轻量封装，适合大多数 feed、chat、task、timeline 场景。

最少只需要实现：

- `get_count`
- `create_item_view`
- `bind_item_view`

常见可选能力：

- `get_stable_id`
- `find_index_by_stable_id`
- `measure_item_height`
- `unbind_item_view`
- `should_keep_alive`

常用便捷接口：

- `egui_view_virtual_list_find_index_by_stable_id()`
- `egui_view_virtual_list_scroll_to_stable_id()`
- `egui_view_virtual_list_notify_item_changed_by_stable_id()`
- `egui_view_virtual_list_notify_item_resized_by_stable_id()`

默认行为：

- 不实现 `get_view_type` 时，默认 `view_type = 0`
- 不实现 `measure_item_height` 时，默认返回 `estimated_item_height`
- 不实现 `find_index_by_stable_id` 但实现了 `get_stable_id` 时，框架会自动线性查找
- 两者都不实现时，默认 stable id 为 `index + 1`

```c
static egui_view_virtual_list_t my_list;

static const egui_view_virtual_list_data_source_t my_data_source = {
        .get_count = my_get_count,
        .get_stable_id = my_get_stable_id,
        .measure_item_height = my_measure_item_height,
        .create_item_view = my_create_item_view,
        .bind_item_view = my_bind_item_view,
        .should_keep_alive = my_should_keep_alive,
};

EGUI_VIEW_VIRTUAL_LIST_PARAMS_INIT(my_list_params, 8, 40, 224, 272);

egui_view_virtual_list_init_with_params(EGUI_VIEW_OF(&my_list), &my_list_params);
egui_view_virtual_list_set_data_source(EGUI_VIEW_OF(&my_list), &my_data_source, &my_context);
egui_view_virtual_list_set_estimated_item_height(EGUI_VIEW_OF(&my_list), 72);
egui_view_virtual_list_set_overscan(EGUI_VIEW_OF(&my_list), 1, 1);
egui_view_virtual_list_set_keepalive_limit(EGUI_VIEW_OF(&my_list), 4);
egui_core_add_user_root_view(EGUI_VIEW_OF(&my_list));
```

### 2. 长页面 / section 容器优先用 `virtual_page`

如果你的业务更像“一个页面里有很多 section”，而不是一行一行的列表，直接用 `virtual_page` 会更顺手。

适合场景：

- 首页聚合页
- 详情页
- 配置页
- 表单页
- 多 section dashboard

它底层仍然复用同一个 `virtual_viewport`，但接口语义从 `item` 换成了 `section`。

对应也提供 stable-id 友好的 section 接口：

- `egui_view_virtual_page_find_section_index_by_stable_id()`
- `egui_view_virtual_page_scroll_to_section_by_stable_id()`
- `egui_view_virtual_page_notify_section_changed_by_stable_id()`
- `egui_view_virtual_page_notify_section_resized_by_stable_id()`

```c
static egui_view_virtual_page_t dashboard_page;

static const egui_view_virtual_page_data_source_t dashboard_sections = {
        .get_count = dashboard_get_section_count,
        .get_stable_id = dashboard_get_section_id,
        .measure_section_height = dashboard_measure_section_height,
        .create_section_view = dashboard_create_section_view,
        .bind_section_view = dashboard_bind_section_view,
        .should_keep_alive = dashboard_should_keep_alive,
};

EGUI_VIEW_VIRTUAL_PAGE_PARAMS_INIT(dashboard_page_params, 8, 8, 224, 304);

egui_view_virtual_page_init_with_params(EGUI_VIEW_OF(&dashboard_page), &dashboard_page_params);
egui_view_virtual_page_set_data_source(EGUI_VIEW_OF(&dashboard_page), &dashboard_sections, &dashboard_ctx);
egui_view_virtual_page_set_estimated_section_height(EGUI_VIEW_OF(&dashboard_page), 96);
egui_view_virtual_page_set_keepalive_limit(EGUI_VIEW_OF(&dashboard_page), 4);
egui_core_add_user_root_view(EGUI_VIEW_OF(&dashboard_page));
```

### 3. 只有在这些场景下再直接用 `virtual_viewport`

- 需要横向主轴或更底层控制
- 需要直接操作 viewport adapter
- 需要完全自定义生命周期桥接

## 一个关键原则：只创建“槽位 view”

不要给每个数据项都真实创建一个 view。  
正确做法是：

- 最多只创建 `EGUI_VIEW_VIRTUAL_VIEWPORT_MAX_SLOTS` 个槽位 view
- 滚动时复用这些 view
- 在 `bind_item_view` 或 `bind_section_view` 里重新绑定当前数据

## 高度变化后要显式通知

如果高度会变化，业务状态更新后需要通知虚拟容器重新计算锚点和布局。

列表场景：

```c
egui_view_virtual_list_notify_item_resized(EGUI_VIEW_OF(&my_list), index);
```

页面场景：

```c
egui_view_virtual_page_notify_section_resized(EGUI_VIEW_OF(&dashboard_page), index);
```

## 动画和状态怎么保住

如果 item 只是纯展示，直接复用即可。  
如果 item 内部存在下面这些状态，建议配合 `stable_id + should_keep_alive`：

- 动画正在播放
- 输入态 / 编辑态未结束
- 焦点停留在该 item
- 手势过程还没结束
- 你希望它滚出屏幕后回来仍然是同一个 view 实例

本示例里：

- 选中 item 会 keepalive
- `Feed` 里的 live / warn 行会 keepalive
- `Chat` 里的 typing 行会 keepalive
- `Ops` 里的 running / blocked 行会 keepalive

## 相关文件

- `src/widget/egui_view_virtual_viewport.h`
- `src/widget/egui_view_virtual_viewport.c`
- `src/widget/egui_view_virtual_list.h`
- `src/widget/egui_view_virtual_list.c`
- `src/widget/egui_view_virtual_page.h`
- `src/widget/egui_view_virtual_page.c`
- `example/HelloBasic/virtual_viewport/test.c`
