# UI Designer 跨页面 Callback 冲突诊断计划

## 背景

`ui_designer` 的用户源码骨架会在各个 `{page}.c` 中生成 callback 函数。如果多个页面复用同一个 callback 名，即使签名相同，也可能在多个页面源码里生成同名全局函数，最终触发链接冲突。

此前 Designer 只检查单页内 callback 签名冲突，无法提前暴露跨页面重复定义风险。

## 目标

1. 在诊断面板中提前发现跨页面 callback 复用导致的链接冲突风险。
2. 区分：
   - 同签名跨页重复
   - 异签名跨页冲突
3. 保持现有诊断面板模型，不把所有页面问题都汇总进来，只增加 project 级 callback 冲突。

## 方案

### 1. 统一收集 callback 绑定

在 `scripts/ui_designer/model/diagnostics.py` 中抽出 page callback 绑定收集逻辑，覆盖：

- `onClick`
- widget events
- page timers

### 2. 增加 project 级分析

新增 `analyze_project_callback_conflicts(project)`：

- 若同名 callback 出现在多个页面：
  - 同签名：报 `project_callback_duplicate`
  - 异签名：报 `project_callback_signature_conflict`

诊断信息中明确列出：

- callback 名
- 解析后的函数签名
- 来源页面与具体绑定点

### 3. MainWindow 接入

在 `scripts/ui_designer/ui/main_window.py` 的诊断更新流程中：

- 保留当前页 `analyze_page(...)`
- 保留当前选择 `analyze_selection(...)`
- 额外合并 `analyze_project_callback_conflicts(self.project)`

## 已完成

- 单页与跨页 callback 收集已共用一套绑定收集逻辑。
- 诊断面板现在可提示：
  - 同名 callback 在不同页面重复定义
  - 同名 callback 在不同页面签名不兼容

## 验证

- 定向测试：
  - `python -m pytest -c scripts/ui_designer/pyproject.toml scripts/ui_designer/tests/model/test_diagnostics.py scripts/ui_designer/tests/ui/test_main_window_file_flow.py -q`

- 全量回归：
  - `python -m pytest -c scripts/ui_designer/pyproject.toml scripts/ui_designer/tests -q`

- 预览烟测：
  - `python scripts/ui_designer_preview_smoke.py`

## 后续建议

1. 在生成代码前加入更强的 project-level preflight，直接阻止已知会导致链接失败的导出/编译。
2. 后续可扩展到：
   - 跨页面重复 page timer callback
   - 跨页面共享 user-defined helper function 命名冲突
   - 统一 callback 重命名辅助
