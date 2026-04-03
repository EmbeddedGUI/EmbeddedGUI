# Virtual Section List Basic

## 这个示例是做什么的

`HelloVirtual/virtual_section_list_basic` 是 `virtual_section_list` 的最小入门示例。

它只保留：

- 一个简单的 action bar
- 一个纵向 `virtual_section_list`
- 清晰分开的 `section header + grouped row`

顶部不再显示 `Header`，避免用户把注意力放到额外摘要 UI 上。

## 这个示例重点演示什么

1. 用 `egui_view_virtual_section_list_init_with_setup()` 接好 `params + data_source + context`
2. 把 section 和 item 状态放在 view 外部
3. 在点击回调里用 `egui_view_virtual_section_list_resolve_entry_by_view()` 反查命中的 header 或 item
4. header 折叠/展开后调用 `notify_data_changed()`
5. item patch 后按情况调用 `notify_item_changed_by_stable_id()` 或 `notify_item_resized_by_stable_id()`

## 为什么它适合入门

相比 `HelloVirtual/virtual_section_list`，这个 basic case 去掉了：

- 更复杂的结构增删
- 更重的动画和状态缓存
- 更多视觉 tone 变化
- 顶部摘要 Header

保留下来的，是最小但仍然清晰的 grouped list 闭环：

- 多个 section
- header 点击折叠/展开
- item 点击命中
- patch 一个 item
- jump 到另一个 section 的 item
- reset 恢复初始状态

## 建议阅读顺序

1. 先看 `section_list_basic_reset_model()`
2. 再看 `section_list_basic_data_source`
3. 再看 `section_list_basic_header_click_cb()` 和 `section_list_basic_item_click_cb()`
4. 最后看 `section_list_basic_patch_selected()` 和 `section_list_basic_jump_to_next()`

## 运行方式

```bash
make all APP=HelloVirtual APP_SUB=virtual_section_list_basic PORT=pc
python scripts/code_runtime_check.py --app HelloVirtual --app-sub virtual_section_list_basic --keep-screenshots
python scripts/checks/hello_basic_render_workflow.py --app HelloVirtual --widgets virtual_section_list_basic --skip-unit-tests
```

## 看图时重点确认

- 首屏先看到 section header，再看到组内 item
- 点击 header 后，可见 entry 总数会变化
- 点击 item 后，命中和选中态会变化
- `Patch` 后目标 item 高度可以切换
- `Jump` 后目标 item 会进入可视区
- `Reset` 后折叠状态和滚动位置会恢复
