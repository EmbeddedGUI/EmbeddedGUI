# Config Profiles

本目录提供可直接复用的 `app_egui_config.h` 模板，作为不同 ROM 档位的起始配置。

当前包含 4 个预设：

- `tiny_rom`
- `basic_ui`
- `dashboard`
- `full_feature`

推荐使用方式：

1. 将对应目录下的 `app_egui_config.h` 复制到你的应用目录，作为应用自己的基础配置。
2. 在此基础上，再按业务需要补充本应用专属开关或裁剪项。

`scripts/size_analysis/` 下的分析脚本已经改为通过应用本地 override 头文件切换这些模板，不再依赖 `USER_CFLAGS` 做批量变体编译。

相关文档：

- `doc/source/size/size_preset_profiles.md`
- `doc/source/size/size_preset_validation.md`
