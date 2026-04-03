# Widget Scripts

该目录保留 HelloBasic / Widget 级别的验收与评审脚本。

当前仍建议保留的入口：

- `widget_acceptance_flow.py`
  单控件运行时、截图、交互和文档门禁。
- `widget_api_review_gate.py`
  API 与 params 结构检查。
- `widget_iteration_gate.py`
  迭代记录与门禁。
- `widget_readme_review_gate.py`
  readme 质量检查。

已清理：

- 依赖旧 `scripts/ui_designer/` 注册表的 bootstrap / design review 脚本
- 这些能力若仍需要，应转移到 `EmbeddedGUI_Designer` 或新的独立工作流中维护
