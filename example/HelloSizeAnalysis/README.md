# HelloSizeAnalysis

本目录集中放置所有 `size analysis` 相关例程和配置模板，方便统一维护。

当前包含：

- `hq_path_probe`
- `canvas_path_probe`
- `widget_feature_probe`
- `preset_validation`
- `ConfigProfiles`

常用构建命令示例：

```bash
make all APP=HelloSizeAnalysis APP_SUB=canvas_path_probe PORT=qemu CPU_ARCH=cortex-m0plus
make all APP=HelloSizeAnalysis APP_SUB=hq_path_probe PORT=qemu CPU_ARCH=cortex-m0plus
make all APP=HelloSizeAnalysis APP_SUB=widget_feature_probe PORT=qemu CPU_ARCH=cortex-m0plus
make all APP=HelloSizeAnalysis APP_SUB=preset_validation PORT=qemu CPU_ARCH=cortex-m0plus
```

CMake 示例：

```bash
cmake -B build/size_probe -DAPP=HelloSizeAnalysis -DAPP_SUB=canvas_path_probe -DPORT=qemu
cmake --build build/size_probe -j
```

批量报告脚本位于 `scripts/size_analysis/`，文档输出位于 `doc/source/size/`。
