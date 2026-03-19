# Virtual Viewport / Virtual List

## 这个示例现在演示什么

这个示例已经不是单一的大列表，而是一个多场景演示页，用同一套 `virtual_list` / `virtual_viewport` 能力覆盖三类常见业务：

- `Feed`
  - 1000+ 条监控流 / 遥测流
  - 不同卡片高度
  - 脉冲动画和 keepalive
  - 点击后展开，触发 `notify_item_resized`
- `Chat`
  - 对话流 / 消息时间线
  - 左右不同气泡布局
  - 未读 / typing 状态
  - 程序化跳转到指定消息
- `Ops`
  - 任务队列 / 工单流
  - 进度条、告警态、完成态
  - 插入、删除、移动、更新都可视化演示

## 顶部按钮对应的能力

- 场景按钮 `Feed / Chat / Ops`
  - 切换整套数据模型
  - 示例里会调用 `egui_view_virtual_list_notify_data_changed()`
- 操作按钮 `Add / Del / Move / Patch / Jump`
  - `Add`
    - 插入一条新 item
    - 对应 `notify_item_inserted`
  - `Del`
    - 删除当前 item
    - 对应 `notify_item_removed`
  - `Move`
    - 调整 item 位置
    - 对应 `notify_item_moved`
  - `Patch`
    - 更新文本、状态、尺寸
    - 对应 `notify_item_changed + notify_item_resized`
  - `Jump`
    - 程序化滚动到目标 item
    - 对应 `scroll_to_item`

## 这个示例重点验证了什么

- item 只在进入可见区时创建和绑定
- slot 数量被限制在 `EGUI_VIEW_VIRTUAL_VIEWPORT_MAX_SLOTS`
- item 高度可变
- item 可点击，能确认具体点击到了哪个 index
- item 动画不会因为普通复用而频繁丢失
- 选中项和关键状态项通过 `should_keep_alive` 保活
- 数据插入、删除、移动、刷新后，列表仍能稳定工作

## 最小接入步骤

### 1. 定义 adapter

至少实现：

- `get_count`
- `create_view`
- `bind_view`

建议同时实现：

- `get_stable_id`
- `find_index_by_stable_id`
- `measure_main_size`
- `unbind_view`
- `should_keep_alive`

### 2. 初始化 `virtual_list`

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

### 3. 只创建“槽位 view”

不要给每个数据项都真的创建一个 view。

正确方式是：

- 只创建最多 `EGUI_VIEW_VIRTUAL_VIEWPORT_MAX_SLOTS` 个 view
- 滚动时复用这些 view
- 在 `bind_view` 里把当前数据重新绑定到槽位 view

### 4. 高度变化时显式通知

如果 item 高度会变化，adapter 里实现 `measure_main_size`，然后在业务状态变化后调用：

```c
egui_view_virtual_list_notify_item_resized(EGUI_VIEW_OF(&my_list), index);
```

## 关于动画和状态保留

如果 item 只是纯展示，可以直接复用。

如果 item 内部存在下面这些状态，建议配合 `stable_id + should_keep_alive`：

- 动画正在播放
- 输入态 / 编辑态还没结束
- 焦点停留在该 item
- 手势过程还没结束
- 你希望该 item 滚出屏幕后回来仍是同一个 view 实例

本例里：

- 选中 item 会 keepalive
- `Feed` 里的 live / warn 行会 keepalive
- `Chat` 里的 typing 行会 keepalive
- `Ops` 里的 running / blocked 行会 keepalive

## 相关文件

- `src/widget/egui_view_virtual_viewport.h`
- `src/widget/egui_view_virtual_viewport.c`
- `src/widget/egui_view_virtual_list.h`
- `src/widget/egui_view_virtual_list.c`
- `example/HelloBasic/virtual_viewport/test.c`
