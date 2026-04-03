# Virtual Strip Basic

## 这个示例是做什么的

`HelloVirtual/virtual_strip_basic` 是 `virtual_strip` 的最小入门示例。

它只保留：

- 一个简单的 action bar
- 一个横向 `virtual_strip`
- 一种基础卡片视图，带三种宽度变化

顶部不再显示 `Header`，方便用户直接理解横向虚拟容器本身。

## 这个示例重点演示什么

1. 用 `egui_view_virtual_strip_init_with_setup()` 接好 `params + data_source + context`
2. 把 item 状态放在外部数组里
3. 在点击回调里用 `egui_view_virtual_strip_resolve_item_by_view()` 反查 item
4. 单项变化后按情况调用 `notify_item_changed_by_stable_id()` 或 `notify_item_resized_by_stable_id()`
5. 用 `scroll_to_stable_id()` 和 `ensure_item_visible_by_stable_id()` 做基础定位

## 为什么它适合入门

相比 `HelloVirtual/virtual_strip`，这个 basic case 去掉了：

- 更复杂的 scene 和 overlay
- 更重的状态缓存组合
- 更复杂的轨道语义
- 顶部摘要 Header

保留下来的就是最基本的 strip 闭环：

- 很多 item
- 少量 live slot
- 点击
- 横向滚动
- patch
- jump
- reset

## 建议阅读顺序

1. 先看 `strip_basic_init_items()`
2. 再看 `strip_basic_data_source`
3. 再看 `strip_basic_item_click_cb()`
4. 最后看 `strip_basic_patch_selected()` 和 `strip_basic_jump_to_next()`

## 运行方式

```bash
make all APP=HelloVirtual APP_SUB=virtual_strip_basic PORT=pc
python scripts/code_runtime_check.py --app HelloVirtual --app-sub virtual_strip_basic --keep-screenshots
python scripts/checks/hello_basic_render_workflow.py --app HelloVirtual --widgets virtual_strip_basic --skip-unit-tests
```

## 看图时重点确认

- 首屏不是空白 strip
- 点击卡片后，选中态会变化
- `Patch` 后卡片内容会变化，必要时宽度也会变化
- 横向滚动后，可见窗口会移动
- `Jump` 会把目标卡片带回可视区
- `Reset` 会回到初始状态和起始位置
