---
name: code-size-optimization
description: Use when reducing or analyzing code size, ROM, RAM, PFB, heap, stack, feature macros, widget selection, resource format cost, HQ drawing paths, or size regressions for a specific EmbeddedGUI APP/APP_SUB; use doc/source/size as reference material but build baselines around the target application scenario
---

# Code Size Optimization Skill

用于**具体应用**的体积优化和 size 回归分析。默认目标不是 `HelloSizeAnalysis`，而是用户指定的 `APP` / `APP_SUB` / 页面 / 资源组合 / 交互场景。`doc/source/size`、`scripts/size_analysis` 和 `HelloSizeAnalysis` 只作为统计口径、参考报告、专项探针和模板来源。

## 先锁定目标应用

开始优化前必须明确：

- 目标：`APP`、可选 `APP_SUB`、页面、控件组合、资源集合。
- 预算：ROM、静态 RAM、PFB、heap peak、stack peak 的目标或上限。
- 场景：首次进入、稳定 idle、页面切换、滚动、图片加载、录制动作、后端数据刷新。
- 平台：`PORT`、`CPU_ARCH`、屏幕尺寸、PFB 宽高、buffer count、资源生成配置。
- 配置：应用本地 `app_egui_config.h`、额外 override、关键 `EGUI_CONFIG_*` 宏。

如果用户只说“优化 code size”，先从仓库中找到该 APP 的配置、源码、资源配置和录制动作，再建立应用级基线。不要默认只跑 `HelloSizeAnalysis` 或只看 `HelloPerformance`。

## 参考资料

按问题类型读取 `doc/source/size/`：

- 总入口和阅读顺序：`size_reading_map.md`
- 统计口径：`size_overview.md`
- 最终应用结果：`size_report.md`
- 取舍流程：`size_selection_guide.md`
- 预设模板：`size_preset_profiles.md`、`size_preset_validation.md`
- 当前建议用户关注的 fast-path 配置：`fast_path_config.md`
- HQ 路径成本：`hq_size_report.md`
- canvas 路径和 feature 成本：`canvas_path_size_report.md`、`canvas_feature_size_report.md`
- widget 组合成本：`widget_feature_size_report.md`

优先使用这些文档的结论，不要从历史报告里复活已经不推荐公开给用户配置的微型宏。

## 应用级基线

具体 APP 也要建立 size 基线，基线单位是“应用配置 + 场景”，不是“框架示例集合”。

建议记录：

- `app`: `APP` / `APP_SUB`
- `scenario`: 例如 `settings_idle`、`dashboard_enter`、`image_page_scroll`、`data_refresh`
- `commit`: 当前 commit
- `config`: 屏幕尺寸、PFB、关键宏、profile 模板、资源版本
- `recording`: 固定录制动作或手动复现步骤
- `outputs`: `output/size_results.json`、`output/README.md`、`runtime_check_output/` 截图、`main.map`
- `metrics`: `Code`、`Resource`、`RAM`、`PFB`、`Heap Idle`、`Heap Peak`、`Stack Peak`、`Total ROM`

QEMU size 口径用于最终 size 和运行期内存比较。PC 运行用于视觉检查和快速预览，不作为 heap/stack 口径。

## 常用命令

先查看统一入口，确认当前脚本支持的子命令：

```bash
python scripts/size_analysis/main.py help
```

生成典型应用 size 结果和文档：

```bash
python scripts/size_analysis/main.py --case-set typical
python scripts/size_analysis/main.py size-to-doc
```

同时生成结果和文档：

```bash
python scripts/size_analysis/main.py --case-set typical --doc
```

刷新 HQ / canvas / widget / preset 专项报告：

```bash
python scripts/size_analysis/main.py run-size-suite --quick
python scripts/size_analysis/main.py run-size-suite --full
python scripts/size_analysis/main.py run-size-suite --only hq,canvas_feature,widget
```

专项入口：

```bash
python scripts/size_analysis/main.py hq-size-to-doc
python scripts/size_analysis/main.py canvas-path-size-to-doc
python scripts/size_analysis/main.py canvas-feature-size-to-doc
python scripts/size_analysis/main.py widget-feature-size-to-doc
python scripts/size_analysis/main.py size-preset-validation-to-doc
```

说明：

- `scripts/size_analysis` 的主要输出在 `output/`，文档输出在 `doc/source/size/`。
- 专项脚本会临时修改 `example/HelloSizeAnalysis/` 下的 probe / override 头文件，跑完后检查 git diff，避免误提交中间状态。
- 对用户指定 APP 做优化时，如果该 APP 不在 size 脚本默认 scope 中，先用当前构建系统生成 `PORT=qemu CPU_ARCH=cortex-m0plus` 的 ELF/map，再按 `doc/source/size/size_overview.md` 的口径比对 `Code/Resource/RAM/PFB/heap/stack`；必要时再扩展 size 脚本 case 列表。

## 优化顺序

1. **选起点模板**
   - 极限小 ROM：从 `tiny_rom` 起步。
   - 普通设置页、表单页：从 `basic_ui` 起步。
   - 仪表盘、展示页：从 `dashboard` 起步。
   - 高配设备或演示工程：从 `full_feature` 起步。
   - 模板位于 `example/HelloSizeAnalysis/ConfigProfiles/*/app_egui_config.h`，推荐复制或转换为应用本地配置，不依赖 `USER_CFLAGS` 长期切换。

2. **先裁全局 feature**
   - 优先复查 `EGUI_CONFIG_FUNCTION_SUPPORT_MASK`。
   - 优先复查 `EGUI_CONFIG_FUNCTION_IMAGE_CODEC_QOI` / `EGUI_CONFIG_FUNCTION_IMAGE_CODEC_RLE`，只在资源实际使用对应格式时打开。
   - 优先复查 `EGUI_CONFIG_FUNCTION_WIDGET_ENHANCED_DRAW`、`EGUI_CONFIG_FUNCTION_SUPPORT_SHADOW`、`EGUI_CONFIG_FUNCTION_SUPPORT_LAYER`。
   - 优先复查 `EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_RGB565_8`、`EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_ALPHA_8`。
   - 不要因为“以后可能会用”提前打开大类功能。

3. **再裁 widget 和业务引用**
   - 对照 `widget_feature_size_report.md` 判断真实 widget 组合成本。
   - 能用基础控件表达的场景，不默认引入重控件。
   - 没有全局总开关的能力，优先通过“不引用对应 API / widget / draw path”让编译器和链接器回收。
   - 不要把 widget delta 直接线性相加；共享代码会改变真实组合成本。

4. **再看资源和 codec**
   - 对照 `Resource` 和资源配置，区分代码体积和只读资源体积。
   - PNG/raw/QOI/RLE 的选择要和资源生成结果一致。
   - `QOI/RLE` codec 只在资源确实使用时打开。
   - 大图、字体、图标字符集变动后，必须重新生成资源并重新测 size。

5. **最后评估 HQ**
   - 默认不要把 `EGUI_CONFIG_CIRCLE_DEFAULT_ALGO_HQ` 全局改成 `1`。
   - `_hq` API 只在明确需要画质收益的控件或场景中显式调用。
   - `hq_size_report.md` 中的 HQ 增量是隔离探针结果，真实 APP 仍以最终链接结果为准。

6. **谨慎调整 fast-path**
   - 当前用户侧主要关注 `fast_path_config.md` 中保留的少数配置。
   - 压 ROM 时，优先保持 `EGUI_CONFIG_FUNCTION_IMAGE_CODEC_FAST_DRAW=0` 和两个 `*_PERSISTENT_CACHE_MAX_BYTES=0`。
   - 压 SRAM 时，优先评估 `EGUI_CONFIG_IMAGE_DECODE_MAX_PIXEL_SIZE` 和 `EGUI_CONFIG_IMAGE_RLE_EXTERNAL_CACHE_WINDOW_SIZE`。
   - 只有存在明确压缩图片或 external image 热点时，再用真实 APP 做 fast-path A/B。

## 重要判断规则

- 不要把 `hq_size_report.md`、`canvas_path_size_report.md`、`widget_feature_size_report.md` 的 delta 简单相加。
- 先看最终应用结果，再用 feature/widget/HQ 报告解释来源。
- `Code` 是 `.text`，`Resource` 是 `.rodata`；优化方向不同，不能混为一谈。
- `RAM` 不包含 PFB；PFB 要单独看。
- `Heap Peak` 包含 idle peak 和 interaction peak 的最大值；必须覆盖真实录制动作。
- `Stack Peak` 依赖 QEMU watermark，PC 模拟不能替代。
- 不要用固定大数组或大栈数组替代 heap 来制造“heap=0”；尺寸相关 buffer 必须遵守 `CLAUDE.md` 的 Buffer 分配约束。
- 公共框架改动会影响多个 APP 时，必须跑 size suite 或至少跑相关专项报告，不能只看单个 APP 变小。

## 常见诊断

| 现象 | 优先判断 | 处理方向 |
| --- | --- | --- |
| `Code` 突然增大 | 新增 widget、canvas path、HQ、mask、codec、enhanced draw | 看 map 和 size 专项报告，裁未用 feature |
| `Resource` 很大 | 图片、字体、视频、图标字符集 | 查资源配置，压缩或拆分资源，去掉未用字符 |
| `RAM` 增大 | 静态对象、cache、全局 buffer | 查 `.bss/.data`，避免尺寸相关静态大 buffer |
| `PFB` 增大 | 屏幕尺寸、PFB 尺寸、buffer count | 按 RAM 预算重选 PFB，不把 PFB 混入普通 RAM |
| `Heap Peak` 高 | 图片解码、SVG、动态 buffer、页面创建 | 用录制动作覆盖峰值，检查 decode/cache/生命周期 |
| `Stack Peak` 高 | 大局部数组、递归或深调用 | 移除大栈数组，改为受控 heap 或小 scratch |
| 小 ROM 项目超标 | 模板过高或功能提前打开 | 回到 `tiny_rom/basic_ui`，按需逐项加回 |
| 多个 APP 同时变大 | 框架公共路径或默认宏变化 | 跑 size suite，查公共对象和配置默认值 |

## 验证闭环

每次改动后至少完成：

1. 用同一 `APP/APP_SUB/PORT/CPU_ARCH/资源版本/录制动作` 重测 before/after。
2. 对比 `Code`、`Resource`、`RAM`、`PFB`、`Heap Idle`、`Heap Peak`、`Stack Peak`、`Total ROM`。
3. 运行目标 APP 的 runtime check，确认裁剪没有破坏渲染和交互：
   ```bash
   python scripts/code_runtime_check.py --app {APP} --keep-screenshots
   python scripts/code_runtime_check.py --app {APP} --app-sub {SUB} --keep-screenshots
   ```
4. 如果修改了公共 `src/`、默认配置、资源生成、widget 或 canvas 基础能力，额外运行相关 size suite：
   ```bash
   python scripts/size_analysis/main.py run-size-suite --quick
   ```
5. 如果更新 `doc/source/size` 报告，确保报告 Markdown、JSON 和图片来自同一轮数据。

## 交付要求

最终回复必须包含：

- 目标 APP/APP_SUB、场景、资源版本和关键配置。
- 优化前后的命令、输出文件路径和指标表。
- 具体裁剪项：feature 宏、widget/API 引用、资源格式、HQ 路径、fast-path 配置。
- 视觉和运行验证结果：runtime_check 截图目录、是否覆盖所有相关页面。
- 是否运行了 size suite 或专项报告；没跑时说明为什么不需要。
