# Size Reading Map

本页用于说明 `size/` 目录中每份文档分别回答什么问题，以及建议的阅读顺序。

## 1. 先看哪一份

| 你的问题 | 先看 | 再看 |
|---------|------|------|
| 整个应用最终有多大 | `size_report.md` | `size_selection_guide.md` |
| 想看 fast-path 相关配置现在还能调什么，以及各自的 `size/perf` 取舍 | `fast_path_config.md` | `size_selection_guide.md` |
| 某条 HQ 路径值不值得保留 | `hq_size_report.md` | `size_selection_guide.md` |
| 某条 canvas 渲染路径会引入多少代码 | `canvas_path_size_report.md` | `canvas_feature_size_report.md` |
| 某类 canvas 能力整体打开后会增加多少 ROM | `canvas_feature_size_report.md` | `size_selection_guide.md` |
| 某个真实 widget 会带来多少代码体积 | `widget_feature_size_report.md` | `size_selection_guide.md` |
| 几个 feature 或 widget 能不能简单相加 | `size_selection_guide.md` | `widget_feature_size_report.md` |
| 想直接挑一个预设配置开始用 | `size_preset_profiles.md` | `size_preset_validation.md` |
| 想确认模板能否直接编译通过 | `size_preset_validation.md` | `example/HelloSizeAnalysis/ConfigProfiles/README.md` |

## 2. 各文档的定位

| 文档 | 作用 | 适合谁看 |
|------|------|---------|
| `size_overview.md` | 说明 qemu size 口径、统计范围和生成方法 | 框架维护者 |
| `fast_path_config.md` | 汇总当前仍建议用户关注的 fast-path 配置，以及对应的 `size/perf` 取舍 | 应用开发者、性能与裁剪负责人 |
| `size_report.md` | 真实示例集合的整体 `ROM/RAM/heap/stack` 报告 | 平台、发布、性能负责人 |
| `hq_size_report.md` | `line_hq / circle_hq / arc_hq` 的单独链接增量 | 渲染和框架维护者 |
| `canvas_path_size_report.md` | 普通 canvas 细分场景的路径级增量 | 底层渲染开发者 |
| `canvas_feature_size_report.md` | canvas feature 级别的组合增量 | 框架裁剪负责人 |
| `widget_feature_size_report.md` | 真实 widget 级别的组合增量 | 应用开发者 |
| `size_selection_guide.md` | 把几份报告整理成可执行的取舍顺序 | 所有人 |
| `size_preset_profiles.md` | 四档可直接复用的模板说明 | 应用开发者 |
| `size_preset_validation.md` | 模板是否能直接编译通过的验证结果 | 框架维护者、CI |

## 3. 三种口径不要混着看

### 3.1 隔离增量

特点：

- 某条路径被强制链接后，额外增加多少 `.text/.rodata`
- 适合回答“这项能力本身贵不贵”

代表文档：

- `hq_size_report.md`
- `canvas_path_size_report.md`

### 3.2 组合增量

特点：

- 一组相关路径或控件同时打开后的真实组合成本
- 适合回答“业务真正需要这组能力时，总体要付出多少 ROM”

代表文档：

- `canvas_feature_size_report.md`
- `widget_feature_size_report.md`

### 3.3 最终应用结果

特点：

- 真实应用 ELF 和 qemu 运行时的最终 size
- 包含 feature、widget、资源、应用代码和运行期统计

代表文档：

- `size_report.md`

## 4. 按场景阅读

### 4.1 做框架裁剪

建议顺序：

1. `canvas_feature_size_report.md`
2. `hq_size_report.md`
3. `size_selection_guide.md`
4. `size_preset_profiles.md`

适合解决：

- 是否保留 `mask / codec / transform`
- 是否保留默认 HQ 路径
- 应该从哪档模板起步

### 4.2 做应用选型

建议顺序：

1. `widget_feature_size_report.md`
2. `size_selection_guide.md`
3. `size_preset_profiles.md`

适合解决：

- `slider / switch / page_indicator / stepper` 这类基础控件怎么选
- `gauge / activity_ring / chart_line` 这类重控件是否值得引入
- 不同控件组合的大致 ROM 档位

### 4.3 做发布和平台验收

建议顺序：

1. `size_report.md`
2. `size_preset_validation.md`
3. `size_selection_guide.md`

适合解决：

- 最终应用整体指标是否达标
- 模板配置是否能直接交给用户使用

## 5. 模板入口

预设模板位于：

- `example/HelloSizeAnalysis/ConfigProfiles/tiny_rom/app_egui_config.h`
- `example/HelloSizeAnalysis/ConfigProfiles/basic_ui/app_egui_config.h`
- `example/HelloSizeAnalysis/ConfigProfiles/dashboard/app_egui_config.h`
- `example/HelloSizeAnalysis/ConfigProfiles/full_feature/app_egui_config.h`

模板说明见：

- `size_preset_profiles.md`

模板编译验证见：

- `size_preset_validation.md`

## 6. 总入口

如果不想逐个记脚本入口，可以直接使用统一调度脚本：

```bash
python scripts/size_analysis/run_size_suite.py --quick
python scripts/size_analysis/run_size_suite.py --full
python scripts/size_analysis/run_size_suite.py --only hq,widget
python scripts/size_analysis/run_size_suite.py --list-steps
```

说明：

- `--quick`：每类报告只跑一小组代表性变体
- `--full`：跑各类报告的完整默认集合
- `--only`：只跑指定子集
- `--list-steps`：列出可用步骤

## 7. 一句话总结

做框架裁剪，先看 feature。

做应用选型，先看 widget。

做最终交付，先看整体 `size_report.md`，再回到 feature、widget 和模板页做取舍。
