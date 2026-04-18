# resvg 迁移计划

## 目标

将仓库的 SVG 光栅化链路从 `CairoSVG` / `cairo_runtime` 迁移到官方 `resvg` CLI，并让 `setup.bat` / `setup.sh` / CI 都能直接补齐依赖。

## 执行项

1. 接入 `resvg` 工具层
   - 新增 `scripts/resvg_tool.py`
   - 新增 `scripts/setup_resvg.py`
   - 支持仓库内本地安装、`RESVG` 环境变量和 `PATH` 发现
   - 增加官方地址 + 镜像回退下载

2. 替换资源生成与校验脚本
   - `scripts/tools/svg2img.py` 改为调用 `resvg`
   - `scripts/checks/svg_validation_check.py` 改为 `resvg` / `edge` 双后端
   - 删除 `scripts/cairo_runtime.py`

3. 接入 setup 主流程
   - `scripts/setup_env.py` 增加 `--skip-resvg` / `--install-resvg`
   - 默认检查并自动安装 `resvg`
   - Summary 输出 `resvg` 状态

4. 清理 CI 与文档
   - CI action 改为 `.github/actions/setup-resvg`
   - workflow 改为 `Setup resvg`
   - `requirements.txt` 删除 `cairosvg`
   - 修正文档与 `HelloSVGSpec` 说明
   - 清理 manifest 中残留的 Cairo 文案

## 验证

- `python -m py_compile scripts/resvg_tool.py scripts/setup_resvg.py scripts/tools/svg2img.py scripts/checks/svg_validation_check.py scripts/setup_env.py`
- `python scripts/setup_env.py --python-mode none --skip-toolchain --skip-ffmpeg --skip-emsdk --skip-build-check`
- `python scripts/tools/app_resource_generate.py -r example/HelloAPP/resource -o output\\tmp_resvg_check`
- `python scripts/checks/svg_validation_check.py --list-cases`
- 内联调用 `resolve_reference_backend('resvg', None)` 返回仓库内 `resvg.exe`
