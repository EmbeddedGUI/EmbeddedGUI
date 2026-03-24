# UI Designer 本地打包

## 目标

`ui_designer` 已经支持通过 PyInstaller 打包为独立桌面工具。为了避免每次都去翻 `release.yml` 或 `ui_designer.spec`，仓库提供了统一的本地打包脚本：

```bash
python scripts/package_ui_designer.py
```

这个脚本适合两类场景：

- 本地快速验证 `ui_designer.spec` 是否还能正常构建
- 手工产出一个可分发的 Designer 包，用于内部试用或回归

## 前置条件

先准备桌面依赖：

```bash
setup.bat
```

或：

```bash
./setup.sh
```

如果只缺打包依赖，也可以单独安装：

```bash
python -m pip install pyinstaller
```

## 最常用命令

### 直接打包

```bash
python scripts/package_ui_designer.py
```

默认行为：

- 调用 `scripts/ui_designer/ui_designer.spec`
- 输出目录为 `dist/`
- 工作目录为 `build/pyinstaller/`
- 默认执行 `--clean`
- Windows 默认产出 `.zip`
- Linux / macOS 默认产出 `.tar.gz`

### 只构建目录，不打包归档

```bash
python scripts/package_ui_designer.py --archive none
```

这适合本地只想验证 PyInstaller 构建是否通过，不关心最终压缩包。

### 带版本后缀的归档

```bash
python scripts/package_ui_designer.py --package-suffix v1.0.0
```

例如在 Windows x64 下会生成：

```text
dist/EmbeddedGUI-Designer-windows-x64-v1.0.0.zip
```

## 产物说明

打包成功后，通常会得到两类产物：

- `dist/EmbeddedGUI-Designer/`
  - PyInstaller 生成的实际运行目录
- `dist/EmbeddedGUI-Designer-{platform-tag}[-suffix].zip|tar.gz`
  - 便于分发的归档文件

其中 `{platform-tag}` 会自动根据当前平台生成，例如：

- `windows-x64`
- `linux-x64`
- `macos-arm64`

## 首次运行说明

独立 exe 首次运行时，不一定已经知道 SDK 路径。当前设计是：

- Designer 先尝试自动发现 SDK
- 如果没找到有效 SDK，仍然可以打开工程和编辑
- 预览会自动退回 Python fallback
- 用户可以在欢迎页或菜单里点击 `Set SDK Root...` 补设 SDK

这意味着打包后的工具不要求必须和 SDK 放在同一目录。

## 与发布检查的关系

`scripts/release_check.py` 的 `ui_package` 步骤已经复用这套本地打包脚本：

```bash
python scripts/release_check.py --skip perf,perf_doc,wasm,doc
```

这样本地打包和发布前检查走的是同一条构建路径，不容易长期漂移。
