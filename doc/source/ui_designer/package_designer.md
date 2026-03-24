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

补充说明，当前独立版首启 SDK 行为已经更新为：

- Designer 会先自动发现已有 SDK
- 如果当前没有检测到有效 SDK，且当前没有自动打开工程，会弹一次首启引导框
- 引导框提供 `Download SDK Automatically`、`Select SDK Root...`、`Skip for Now` 三个动作
- 引导框会直接显示自动下载默认写入的目标路径，避免用户不清楚 SDK 会被放到哪里

如果用户选择自动下载，下载顺序是：

- GitHub 主分支 zip 包
- Gitee zip 包
- Gitee `git clone` 回退路径（仅在本机存在 `git` 时启用）

实际验证中，GitHub zip 是最稳定的主通路；Gitee 的匿名 zip 入口有时会返回 HTML 页面而不是压缩包，因此工具会自动继续尝试后续回退路径。
如果当前下载源长时间无法完成，工具也会自动超时并继续回退，不会无限期卡在第一条下载链路上。
如需调整这个归档下载超时，可通过环境变量 `EMBEDDEDGUI_SDK_ARCHIVE_TIMEOUT_SECONDS` 覆盖默认值。

自动下载成功后，Designer 会自动保存 `sdk_root`，并恢复真实编译预览链路。

自动下载的 SDK 默认不会写入 exe 目录，而是缓存到当前用户配置目录下：

```text
{config_dir}/sdk/EmbeddedGUI
```

这样可以避免安装目录不可写的问题，也方便独立 exe 升级后复用同一份 SDK 缓存。

如果自动下载失败，Designer 仍然保持可编辑状态，预览继续退回 Python fallback。用户之后仍可通过以下入口重试：

- 欢迎页 `Download SDK...`
- `File -> Download SDK Copy...`
- `Set SDK Root...`

独立 exe 首次运行时，不一定已经知道 SDK 路径。当前设计是：

- Designer 先尝试自动发现 SDK
- 如果没找到有效 SDK，仍然可以打开工程和编辑
- 预览会自动退回 Python fallback
- 用户可以在欢迎页或菜单里点击 `Set SDK Root...` 补设 SDK
- 手动选择 SDK 时，不要求必须精确点到最终根目录；选中其上层目录、`sdk/` 容器目录后，Designer 也会自动纠正

自动发现会优先扫描项目目录、Designer 可执行文件所在目录、当前工作目录附近的常见布局，例如：
- SDK 就放在同级 `EmbeddedGUI/`
- SDK 根目录本身就叫 `sdk/`
- `sdk/` 下面再包一层 `EmbeddedGUI-main/`、`EmbeddedGUI-sdk/` 之类的目录

这意味着打包后的工具不要求必须和 SDK 放在同一目录。

## 与发布检查的关系

`scripts/release_check.py` 的 `ui_package` 步骤已经复用这套本地打包脚本：

```bash
python scripts/release_check.py --skip perf,perf_doc,wasm,doc
```

这样本地打包和发布前检查走的是同一条构建路径，不容易长期漂移。
