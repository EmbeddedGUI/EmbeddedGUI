# Virtual Viewport / Virtual List

## 这个例程展示了什么

这个例程不是一个“只有一列按钮”的简单列表，而是一个完整的虚拟列表示范页，覆盖了下面这些场景：

- 1200 个 item 的大列表
- item 只在进入可视区时创建和绑定
- item 高度可变
- item 外观可变：Hero / Detail / Compact 三种显示模式
- item 可点击，点击后会更新顶部状态区
- 点击后 item 会展开，触发 `notify_item_resized`
- 部分 item 带脉冲动画，用来演示动画场景
- 选中 item 和 live item 进入 keepalive，避免滚出屏幕后立即丢状态

## 组件关系

- `egui_view_virtual_viewport_t`
  - 通用虚拟化容器
  - 支持纵向 / 横向
  - 适合做 list / page / timeline / grid 之类的基础底座
- `egui_view_virtual_list_t`
  - `virtual_viewport` 的垂直列表薄封装
  - 如果你做的是普通纵向列表，优先直接用它

## 最小接入步骤

### 1. 定义 adapter

至少要提供这几个回调：

- `get_count`
- `create_view`
- `bind_view`

通常建议同时提供：

- `get_stable_id`
- `find_index_by_stable_id`
- `measure_main_size`
- `unbind_view`
- `should_keep_alive`

### 2. 初始化列表

```c
static egui_view_virtual_list_t my_list;

EGUI_VIEW_VIRTUAL_LIST_PARAMS_INIT(my_list_params, 8, 40, 224, 272);

egui_view_virtual_list_init_with_params(EGUI_VIEW_OF(&my_list), &my_list_params);
egui_view_virtual_list_set_adapter(EGUI_VIEW_OF(&my_list), &my_adapter, &my_context);
egui_view_virtual_list_set_estimated_item_height(EGUI_VIEW_OF(&my_list), 72);
egui_view_virtual_list_set_overscan(EGUI_VIEW_OF(&my_list), 1, 1);
egui_view_virtual_list_set_keepalive_limit(EGUI_VIEW_OF(&my_list), 4);
egui_core_add_user_root_view(EGUI_VIEW_OF(&my_list));
```

### 3. 在 `create_view` 里创建“槽位视图”

不要把每个数据项都创建成真实 view。

正确做法是：

- 只创建最多 `EGUI_VIEW_VIRTUAL_VIEWPORT_MAX_SLOTS` 个 view
- 滚动时复用这些 view
- 在 `bind_view` 里把数据重新绑定到 view 上

### 4. 在 `bind_view` 里刷新 UI

`bind_view` 负责：

- 根据 index / stable_id 更新文本
- 更新样式
- 更新位置相关的子布局
- 根据数据决定是否显示动画、进度条、标签等子元素

## `stable_id` 和 keepalive 的作用

如果 item 只是静态展示，不需要 keepalive。

但如果 item 里有下面这些状态，建议配合 `stable_id + should_keep_alive` 一起用：

- 动画正在播放
- 文本正在编辑
- 焦点在这个 item 上
- 拖拽 / 手势还没结束
- 你希望滚出屏幕后回来还能保留同一个 view 实例

本例中：

- 选中 item 会进入 keepalive
- `live` item 带脉冲动画，也会进入 keepalive

这样滚动时不会在热路径上频繁 `malloc/free`，也更不容易把瞬时状态弄丢。

## 变量高度怎么做

如果 item 高度会变化，adapter 里实现 `measure_main_size`：

```c
static int32_t demo_measure_main_size(void *adapter_context, uint32_t index, int32_t cross_size_hint)
{
    EGUI_UNUSED(adapter_context);
    EGUI_UNUSED(cross_size_hint);
    return demo_get_item_height(index);
}
```

当高度变化时，调用：

```c
egui_view_virtual_list_notify_item_resized(EGUI_VIEW_OF(&my_list), index);
```

本例点击 item 后会展开卡片，就是通过这个接口触发重排。

## 数据变更通知

按场景选择下面这些接口：

- `egui_view_virtual_list_notify_data_changed`
- `egui_view_virtual_list_notify_item_changed`
- `egui_view_virtual_list_notify_item_inserted`
- `egui_view_virtual_list_notify_item_removed`
- `egui_view_virtual_list_notify_item_moved`
- `egui_view_virtual_list_notify_item_resized`

如果你只是换了文本和颜色，用 `notify_item_changed`。

如果高度也变了，用 `notify_item_resized`。

## 这个例程里的实现建议

- item 根节点不是直接把整行填满，而是“外层 group + 内层 card”，这样行与行之间可以留出呼吸感
- 顶部单独做了状态卡片，用来展示点击反馈
- item 内部子控件全部在槽位 view 里复用，不跟数据量线性增长
- 动画放在槽位 view 内部，只对少量需要保活的 item 开 keepalive

## 相关文件

- `src/widget/egui_view_virtual_viewport.h`
- `src/widget/egui_view_virtual_viewport.c`
- `src/widget/egui_view_virtual_list.h`
- `src/widget/egui_view_virtual_list.c`
- `example/HelloBasic/virtual_viewport/test.c`
