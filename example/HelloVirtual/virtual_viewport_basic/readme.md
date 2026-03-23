# Virtual Viewport Basic

## 这个示例是做什么的

`HelloVirtual/virtual_viewport_basic` 是 `virtual_viewport` 的最小入门示例。

它只保留三块内容：

- 一个简单的 action bar
- 一个纵向 `virtual_viewport`
- 两种基础行类型：`button` 和 `slider`

顶部不再放 `Header`，避免用户先被说明性 UI 分散注意力。

## 这个示例重点演示什么

1. 用 `egui_view_virtual_viewport_init_with_setup()` 一次接好 `params + adapter + context`
2. 把业务状态放在外部 item 数组里，不放进 view
3. 在回调里用 `egui_view_virtual_viewport_resolve_item_by_view()` 反查命中的 item
4. 单项变化后调用 `egui_view_virtual_viewport_notify_item_changed_by_stable_id()`
5. 用 `scroll_to / ensure_visible / visit_visible_items` 做最基本的可见区控制

## 为什么它适合入门

相比 `HelloVirtual/virtual_viewport`，这个 basic case 去掉了：

- scene 化语义
- 混合画布内容
- 更复杂的状态缓存和数据流
- 顶部摘要 Header

保留下来的只有最小闭环：

- 大量 item
- 少量 live slot
- 点击
- 拖动
- 跳转
- 重置

## 建议阅读顺序

1. 先看 `basic_init_items()`
2. 再看 `basic_viewport_adapter`
3. 再看 `basic_button_click_cb()` 和 `basic_slider_value_changed_cb()`
4. 最后看 `basic_patch_selected()` 和 `basic_jump_to_next()`

## 运行方式

```bash
make all APP=HelloVirtual APP_SUB=virtual_viewport_basic PORT=pc
python scripts/code_runtime_check.py --app HelloVirtual --app-sub virtual_viewport_basic --keep-screenshots
python scripts/hello_basic_render_workflow.py --app HelloVirtual --widgets virtual_viewport_basic --skip-unit-tests
```

## 看图时重点确认

- 首屏不是空白 viewport
- 点击按钮行后，行状态会变化
- 拖动 slider 后，数值会变化
- 拖动 viewport 后，可见窗口会移动
- `Jump` 会把目标项带回可视区
- `Reset` 会回到初始状态和顶部位置
