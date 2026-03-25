# UI Designer 多选批量 Callback 编辑

## 背景

`ui_designer` 之前已经支持单选控件时编辑 `onClick` 和 widget-specific callback，但多选模式下只能批量改 geometry / common properties，无法统一配置事件回调。

这会让常见场景很低效，例如：

1. 多个 slider 需要统一挂同一个 `onValueChanged`
2. 一组按钮需要统一挂同一个 `onClick`
3. 用户想先框选一批控件，再批量接相同 callback

## 目标

1. 多选属性面板也显示 `Callbacks` 分组
2. 仅暴露所有选中控件都支持的共享 callback
3. 支持 mixed 状态，避免误清空原有 callback
4. 编辑后复用现有 `property_changed -> dirty/XML/undo` 主链路

## 已完成

### 1. 多选共享 callback 收集

更新：

- `scripts/ui_designer/ui/property_panel.py`

新增：

- `_callback_entry_map(...)`
- `_collect_multi_callback_entries(...)`

行为：

- 多选时自动求所有选中控件的 callback 交集
- 始终包含通用 `onClick`
- widget-specific event 只有在所有选中控件都具备，且 `signature/use_event_dict` 一致时才显示
- 兼容旧项目里残留的 legacy event 字段

### 2. 多选 Callbacks 分组

更新：

- `scripts/ui_designer/ui/property_panel.py`

新增：

- `_suggest_multi_callback_name(...)`
- `_multi_callback_tooltip(...)`
- `_apply_multi_callback_editor_state(...)`
- `_build_multi_callbacks_group(...)`
- `_on_multi_callback_editing_finished(...)`

行为：

- 多选表单新增 `Callbacks` 分组
- 共享 callback 会显示为可批量编辑输入框
- mixed 场景下：
  - label 显示 `(Mixed)`
  - editor 显示 `Mixed values`
  - tooltip 说明“编辑后会统一所有选中控件”
- 如果用户只是聚焦后直接离开，不会因为 mixed 空文本误把所有 callback 清空

### 3. 批量 callback 编辑与校验

行为：

- 输入 callback 名后一次性写回全部选中控件
- 保持与单选一致的规则：
  - 允许留空，表示清空该 callback
  - 自动把空格规范成下划线
  - 非法 C 标识符直接拒绝并恢复原状态
- summary 面板中的 `Mixed:` 统计已把 callback mixed 状态计入

### 4. 主窗口联动

更新：

- `scripts/ui_designer/tests/ui/test_main_window_file_flow.py`

行为：

- 多选 callback 编辑继续走现有 `property_changed -> _on_property_changed` 链路
- 编辑后会同步：
  - widget model
  - 当前页 XML
  - dirty 状态
  - undo/history 状态

## 测试

更新：

- `scripts/ui_designer/tests/ui/test_property_panel_file_flow.py`
- `scripts/ui_designer/tests/ui/test_main_window_file_flow.py`

新增覆盖：

- 多选时显示共享 callback 编辑器
- mixed callback 状态展示正确
- 多选 callback 编辑会批量写回所有控件
- 非法 callback 名会被拒绝
- 主窗口 dirty/XML 联动正确

## 验证

已通过：

- `python -m pytest -c scripts/ui_designer/pyproject.toml scripts/ui_designer/tests/ui/test_property_panel_file_flow.py -q`
- `python -m pytest -c scripts/ui_designer/pyproject.toml scripts/ui_designer/tests/ui/test_main_window_file_flow.py -q -k "callback_edit"`
- `python -m pytest -c scripts/ui_designer/pyproject.toml scripts/ui_designer/tests -q`
- `python scripts/ui_designer_preview_smoke.py`

结果：

- `1046 passed`
- preview smoke passed

## 后续建议

1. 可以继续补“跳转到用户代码骨架”的 callback 辅助入口。
2. 可以把多选 callback 编辑扩展到 page-level timers / fields 等统一的回调名输入场景。
3. 可以继续推进更完整的批量属性编辑，例如颜色、布局和资源引用的更强批量工具。
