# UI Designer 页面级用户代码入口补充计划

## 背景

`ui_designer` 已经支持从控件 callback 和 page timer 直接打开 `{page}.c`，但页面级生命周期逻辑仍缺少明确入口。用户在编辑 `user_fields`、页面初始化和清理逻辑时，仍需要手动找到 `on_open` / `on_close` / `init`。

## 目标

1. 给页面级编辑区域补上生命周期代码入口。
2. 复用现有 `{page}.c` 自动创建与打开逻辑，不新增分叉实现。
3. 保持入口位置与 page 级配置靠近，降低用户寻找成本。

## 方案

### 1. PageFieldsPanel 承担 page 级代码入口

- 在 `scripts/ui_designer/ui/page_fields_panel.py` 中新增：
  - `user_code_section_requested(str)` 信号
  - `Open on_open`
  - `Open on_close`
  - `Open init`

理由：

- `Page Fields` 本身就是 page 级编辑面板，和生命周期逻辑关联度高。
- 不需要新增一个独立 dock，避免界面继续膨胀。

### 2. MainWindow 复用已有打开逻辑

- `scripts/ui_designer/ui/main_window.py`
  - 连接 `page_fields_panel.user_code_section_requested`
  - 将原 callback 打开逻辑收敛到 `_open_page_user_source(...)`
  - 对 section 打开场景复用已有：
    - 保存未刷新的 XML
    - 若缺少 `{page}.c` 则自动生成
    - 打开系统默认编辑器
    - 刷新 watch snapshot
    - 状态栏反馈

## 已完成

- `Page Fields` 面板顶部已增加页面生命周期按钮。
- 可以直接从 Designer 打开当前页面的：
  - `on_open`
  - `on_close`
  - `init`
- 主窗口已将 callback 和 section 两类打开动作统一到一套用户源码处理流程。

## 验证

- 定向测试：
  - `python -m pytest -c scripts/ui_designer/pyproject.toml scripts/ui_designer/tests/ui/test_page_fields_panel.py scripts/ui_designer/tests/ui/test_main_window_file_flow.py -q`

- 全量回归：
  - `python -m pytest -c scripts/ui_designer/pyproject.toml scripts/ui_designer/tests -q`

- 预览烟测：
  - `python scripts/ui_designer_preview_smoke.py`

## 后续建议

1. 给 `Page Fields` 面板增加 `Open page source` 总入口，兼容只想打开文件、不关心具体 section 的场景。
2. 后续可继续把 page 级用户代码入口扩展到：
   - `on_key_pressed`
   - page resource setup
   - generated header / struct member quick-open
3. 如果后续接入外部编辑器配置页，可让这些入口优先跳到用户配置的 IDE。
