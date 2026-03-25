# UI Designer 编译与导出硬预检计划

## 背景

`ui_designer` 之前已经能在诊断面板提示很多错误，但这些错误仍不会阻止：

- compile preview
- 导出 C 代码

这意味着用户仍可能在已知错误状态下继续生成或导出代码，最终把坏结果带到后续提交或集成流程中。

## 目标

1. 把已知的 fatal diagnostics 升级为 compile/export 阶段的阻断条件。
2. 只拦截“生成代码”相关链路，不影响普通保存。
3. 保持实时编辑流畅，compile preview 阻断时不弹模态框，只切回 Python preview 并给出原因。

## 方案

### 1. MainWindow 内新增统一预检 helper

在 `scripts/ui_designer/ui/main_window.py` 中新增：

- `_collect_codegen_blockers()`
- `_format_codegen_blocker_summary()`
- `_ensure_codegen_preflight(...)`

预检范围：

- 所有页面的 `analyze_page(...)` 中 `severity == error` 的项
- `analyze_project_callback_conflicts(project)` 的 `error` 项

### 2. compile preview 阻断

在 `_do_compile_and_run()` 中：

- 先刷新 pending XML
- 再执行预检
- 若存在 blocker：
  - 不运行资源生成
  - 不生成代码
  - 不触发编译 worker
  - 切回 Python preview，并显示 `Compile blocked by diagnostics`
  - 状态栏显示阻断信息

### 3. export 阻断

在 `_export_code()` 中：

- 选定导出目录后执行预检
- 若存在 blocker：
  - 不写任何导出文件
  - 弹出 warning 对话框
  - 状态栏提示阻断

## 已完成

- compile preview 已接入硬预检
- export 已接入硬预检
- 预检会自动展开 diagnostics dock，并记录 debug 日志

## 验证

- 定向测试：
  - `python -m pytest -c scripts/ui_designer/pyproject.toml scripts/ui_designer/tests/ui/test_main_window_file_flow.py -q`

- 全量回归：
  - `python -m pytest -c scripts/ui_designer/pyproject.toml scripts/ui_designer/tests -q`

- 预览烟测：
  - `python scripts/ui_designer_preview_smoke.py`

## 后续建议

1. 将同一套预检继续复用到未来的 exe 打包入口。
2. 对于 export blocked 的场景，可以增加“Copy diagnostics summary”按钮，方便直接贴给开发者。
3. 后续可考虑在保存时只做非阻断 warning，帮助用户更早发现会阻止 compile/export 的问题。
