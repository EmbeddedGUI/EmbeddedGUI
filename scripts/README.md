# Scripts Layout

根目录只保留高频手动入口：

- `code_compile_check.py`
- `code_runtime_check.py`
- `code_format.py`
- `release_check.py`
- `setup_env.py`

低频或专项脚本按用途分目录：

- `checks/`
  专项校验与回归脚本，如 icon font、dirty animation、parity、render workflow、touch 语义检查。
- `platform/`
  平台/工程维护脚本，如 `keil_project_sync.py`。
- `web/`
  Emscripten / WASM 相关脚本，如 `wasm_build_demos.py`、`emcc_wrapper.py`。
- `recording/`
  录制与 GIF 导出辅助脚本。

整理原则：

- 高频、面向大多数开发者的入口留在根目录。
- 专项验证、平台专用、发布打包、历史兼容脚本进入子目录。
- 如果某类能力后续完全迁出仓库，对应子目录优先整体删除，而不是重新回到根目录。
