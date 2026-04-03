# Virtual Page Basic

## 这个示例是做什么的

`HelloVirtual/virtual_page_basic` 是 `virtual_page` 的最小入门示例。

它只保留：

- 一个简单的 action bar
- 一个纵向 `virtual_page`
- 三种 section 类型：`hero`、`metric`、`checklist`

顶部不再显示 `Header`，让用户直接把注意力放在 section 级虚拟化上。

## 这个示例重点演示什么

1. 用 `egui_view_virtual_page_init_with_setup()` 接好 `params + data_source + context`
2. 把 section 状态放在 view 外部
3. 在点击回调里用 `egui_view_virtual_page_resolve_section_by_view()` 反查命中的 section
4. 单项变化后按情况调用 `notify_section_changed_by_stable_id()` 或 `notify_section_resized_by_stable_id()`
5. 用 `scroll_to_section_by_stable_id()` 和 `ensure_section_visible_by_stable_id()` 做基础定位

## 为什么它适合入门

相比 `HelloVirtual/virtual_page`，这个 basic case 去掉了：

- 更重的结构变化
- 更复杂的动画和状态缓存
- 更多视觉变体
- 顶部摘要 Header

保留下来的，是最小但仍然有代表性的 `virtual_page` 闭环：

- 很多 section
- 少量 live slot
- 点击 section
- patch 一个 section
- jump 到另一个 section
- reset 恢复初始状态

## 建议阅读顺序

1. 先看 `page_basic_init_sections()`
2. 再看 `page_basic_data_source`
3. 再看 `page_basic_section_click_cb()`
4. 最后看 `page_basic_patch_selected()` 和 `page_basic_jump_to_next()`

## 运行方式

```bash
make all APP=HelloVirtual APP_SUB=virtual_page_basic PORT=pc
python scripts/code_runtime_check.py --app HelloVirtual --app-sub virtual_page_basic --keep-screenshots
python scripts/checks/hello_basic_render_workflow.py --app HelloVirtual --widgets virtual_page_basic --skip-unit-tests
```

## 看图时重点确认

- 首屏看到的是模块，不是重复 row
- 点击 section 后，选中态会变化
- `Patch` 到 checklist 时，高度可以切换
- `Jump` 后目标 section 会进入可视区
- 拖动页面后，可见窗口会正确移动
- `Reset` 后状态和滚动位置会恢复
