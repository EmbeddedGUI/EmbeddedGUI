# UI Designer 用户代码跳转与回调骨架补齐计划

## 背景

`ui_designer` 已经支持在属性面板编辑控件事件回调、在页面面板编辑 page timers，但用户仍需要手动找到 `{page}.c` 并补写回调函数。随着项目持续演进，旧的 `page.c` 还会缺少后来新增的 timer / event callback 骨架，容易导致预览与用户代码脱节。

## 目标

1. 在属性面板与页面计时器面板提供直接进入用户代码的入口。
2. 在打开用户代码前，自动确保当前页 `{page}.c` 存在。
3. 对已有但缺失目标回调函数的 `page.c`，自动补齐缺失骨架。
4. 复用生成器逻辑，避免 UI 层手写多套回调模板。
5. 写文件后刷新 Designer 的项目监视快照，避免误判为外部修改而打断实时预览。

## 实施方案

### 1. 生成器补齐回调骨架能力

- 在 `scripts/ui_designer/generator/code_generator.py` 中新增：
  - `collect_page_callback_stubs(page)`
  - `render_page_callback_stub(page, callback_name, signature, kind="view")`
- `generate_page_user_source()` 的 `// USER CODE BEGIN callbacks` 区域现在会包含：
  - `onClick` 回调骨架
  - 控件事件回调骨架
  - page timer 回调骨架

### 2. 面板入口

- `PropertyPanel`
  - 为每个 callback 编辑框增加 `Code` 按钮
  - 新增 `user_code_requested(callback_name, signature)` 信号
  - 多选 mixed 状态下禁用打开按钮，避免打开到不确定目标

- `PageTimersPanel`
  - 新增 `Open User Code` 按钮
  - 依据当前选中 timer 发出 `user_code_requested(callback_name, "void {func_name}(egui_timer_t *timer)")`

### 3. MainWindow 统一处理

- 统一监听 `PropertyPanel` / `PageTimersPanel` 的 `user_code_requested`
- 在 `MainWindow` 中新增：
  - `{page}.c` 路径解析
  - 用户源码初次生成
  - 缺失回调骨架插入
  - 系统默认编辑器打开文件
  - 写文件后刷新项目监视快照

## 已完成

- 生成器已自动输出控件事件 / timer 回调骨架
- 属性面板支持直接打开当前 callback 对应用户代码
- 页面计时器面板支持直接打开当前 timer callback 对应用户代码
- 主窗口可为新旧 `page.c` 自动补齐缺失骨架后再打开文件
- 写入用户源码后已刷新 watch snapshot，避免误触发“外部修改”保护

## 验证

- 定向测试：
  - `python -m pytest -c scripts/ui_designer/pyproject.toml scripts/ui_designer/tests/generator/test_code_generator.py -q`
  - `python -m pytest -c scripts/ui_designer/pyproject.toml scripts/ui_designer/tests/ui/test_property_panel_file_flow.py scripts/ui_designer/tests/ui/test_page_timers_panel.py scripts/ui_designer/tests/ui/test_main_window_file_flow.py -q`

- 全量回归：
  - `python -m pytest -c scripts/ui_designer/pyproject.toml scripts/ui_designer/tests -q`

- 预览烟测：
  - `python scripts/ui_designer_preview_smoke.py`

## 后续建议

1. 将“打开用户代码”继续扩展到 `on_open` / `on_close` / `init` / page fields 等页面级入口。
2. 进一步改造 `callbacks` 区域的 preserve 策略，让后续保存时也能自动合并新增骨架，而不只依赖按需打开时补齐。
3. 如果后续要独立打包为 exe，可在外部编辑器设置页中允许用户配置首选打开方式（系统默认 / VS Code / Explorer Reveal）。
