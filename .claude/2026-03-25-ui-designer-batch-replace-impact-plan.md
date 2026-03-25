# UI Designer 缺失资源批量替换影响预览

## 背景

此前 `Replace Missing...` 虽然已经支持批量替换缺失资源，并在 rename 后通过 `resource_renamed` 自动改写页面引用，但真正执行前没有统一的 grouped impact preview。

这会带来两个问题：

1. 用户无法在批量替换前先看到哪些 rename 会影响页面和控件。
2. 用户即使怀疑某个 rename 会改动很多页面，也只能直接执行，不能先跳转检查 usage。

本轮同时发现一个实际打开流程问题：

- `ResourcePanel.set_resource_catalog()` 在 `resource_dir` 还未设置时会把刚传入的 catalog 覆盖为空，导致主窗口打开项目后资源面板 catalog 可能丢失。

## 目标

1. 为批量 `Replace Missing...` 增加 grouped impact preview。
2. preview 中支持按 rename 分组查看影响范围，并能跳转到具体 usage。
3. 用户取消或跳转检查时，不执行替换。
4. 仅 restore 场景不弹 preview。
5. 修复主窗口打开项目后资源面板 catalog 可能被清空的问题。

## 已完成

### 1. 批量替换影响预览

更新：

- `scripts/ui_designer/ui/resource_panel.py`

新增：

- `_BatchReplaceImpactDialog`
- `_collect_batch_replace_impacts(...)`
- `_confirm_batch_replace_impact(...)`

行为：

- `Replace Missing...` 在真正执行替换前，先分析 `old_name -> new_name`
- 仅对 `new_name != old_name` 的 rename 做 impact 汇总
- 仅对存在 widget usage 的 rename 显示 grouped preview
- summary 按 rename 分组显示：
  - missing resource
  - replacement file
  - affected widget count
  - affected page count
- 选中分组后，下方显示该 rename 对应的 usage 列表
- 支持：
  - `Open Selected Usage`
  - usage 双击跳转
- 跳转后取消当前替换，不会误执行

### 2. restore-only 分支保持轻量

行为：

- 当 replacement 文件名与缺失资源名一致时，视为 restore
- restore-only 场景直接沿用原流程，不弹 grouped impact preview

### 3. 打开项目后的资源 catalog 丢失修复

更新：

- `scripts/ui_designer/ui/resource_panel.py`

修复：

- `set_resource_catalog()` 在 `resource_dir` 尚未配置时，不再调用 `set_resource_dir("")`
- 这样主窗口 `_refresh_all_panels()` 中先设 catalog、后设 resource dir 的流程可以正常保留资源列表

## 测试

更新：

- `scripts/ui_designer/tests/ui/test_resource_panel_file_flow.py`
- `scripts/ui_designer/tests/ui/test_main_window_file_flow.py`

新增覆盖：

- batch impact preview 可跳转到 selected usage
- batch impact preview 会在执行前被调用
- batch impact preview 可取消且不执行替换
- restore-only 场景不弹 batch impact preview
- 主窗口 batch replace 确认后，仍可跨页改写资源引用并保持状态信息
- `set_resource_catalog()` 在 resource dir 未设置前不会丢失 catalog

## 验证

已通过：

- `python -m pytest -c scripts/ui_designer/pyproject.toml scripts/ui_designer/tests/ui/test_resource_panel_file_flow.py -q`
- `python -m pytest -c scripts/ui_designer/pyproject.toml scripts/ui_designer/tests/ui/test_main_window_file_flow.py -q`
- `python -m pytest -c scripts/ui_designer/pyproject.toml scripts/ui_designer/tests -q`
- `python scripts/ui_designer_preview_smoke.py`

结果：

- `1038 passed`
- preview smoke passed

## 后续建议

1. 将 grouped impact preview 进一步复用到单资源 `Replace Missing Resource` 流程。
2. 为 grouped impact preview 增加 `Current Page Only` 过滤，和资源面板 usage 视图保持一致。
3. 继续补 editor maturity 方向的剩余能力，例如事件编辑器或更完整的批量属性编辑。
