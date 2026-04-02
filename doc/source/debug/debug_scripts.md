# 调试脚本

这部分只列和调试、渲染验证直接相关的脚本。

## 总表

| 脚本 | 主要用途 | 常用命令 | 主要输出 |
| --- | --- | --- | --- |
| `scripts/code_runtime_check.py` | 运行示例、自动录制、保留截图 | `python scripts/code_runtime_check.py --app HelloBasic --app-sub slider --keep-screenshots` | `runtime_check_output/<app>/` |
| `scripts/code_compile_check.py` | 批量编译示例并跑 `HelloUnitTest` | `python scripts/code_compile_check.py --full-check` | 终端日志 |
| `scripts/code_dirty_animation_check.py` | 检查代表性动画是否仍是局部刷新 | `python scripts/code_dirty_animation_check.py --scenario showcase` | `runtime_check_output/dirty_animation_check/` |
| `scripts/perf_analysis/dirty_region_stats_report.py` | 把 `DIRTY_REGION_STATS` 日志转成 Markdown/CSV | `python scripts/perf_analysis/dirty_region_stats_report.py --input showcase=perf_output/showcase.log` | `*.md`、`*.csv` |
| `scripts/hello_basic_render_workflow.py` | 批量检查 HelloBasic / HelloVirtual 的录制动作、截图和交互变化 | `python scripts/hello_basic_render_workflow.py --app HelloVirtual --suite basic` | JSON 报告和截图 |
| `scripts/showcase_stage_parity_check.py` | 对比 `HelloShowcase` 和 `virtual_stage_showcase` 的关键画面状态 | `python scripts/showcase_stage_parity_check.py --bits64` | 对比图、diff 图 |

## `code_runtime_check.py`

用途：

- 编译指定应用。

- 打开录制模式运行。

- 校验是否生成 PNG 帧。

- 检查是否出现 `[RUNTIME_CHECK_FAIL]`。

常用命令：

```bash
python scripts/code_runtime_check.py --app HelloShowcase --keep-screenshots
python scripts/code_runtime_check.py --app HelloBasic --app-sub slider --keep-screenshots
python scripts/code_runtime_check.py --app HelloVirtual --app-sub virtual_stage_showcase --keep-screenshots
```

适合场景：

- 修改示例后先做一次最基本的运行确认。

- 想保留截图给文档或 review。

## `code_compile_check.py`

用途：

- 批量编译标准示例。

- 在全量模式下跑 `HelloUnitTest`。

- 同时覆盖 `HelloBasic`、`HelloVirtual` 的子例程。

常用命令：

```bash
python scripts/code_compile_check.py --full-check
python scripts/code_compile_check.py --custom-widgets --category input
```

适合场景：

- 提交前做一次编译面回归。

- 检查某一类 `HelloCustomWidgets` 是否都能编过去。

## `code_dirty_animation_check.py`

用途：

- 对代表性动画做脏区动画专项回归。

- 自动打开 `EGUI_CONFIG_DEBUG_DIRTY_REGION_STATS=1`。

- 统计 unique frame、partial frame、平均脏区比例。

常用命令：

```bash
python scripts/code_dirty_animation_check.py --scenario showcase
python scripts/code_dirty_animation_check.py --scenario virtual_stage_showcase
```

适合场景：

- 优化脏区或动画后，确认没有回退成全屏刷。

- 检查 `HelloShowcase` 和 `virtual_stage_showcase` 这类复杂动效页面。

## `dirty_region_stats_report.py`

用途：

- 解析 `DIRTY_REGION_STATS:` 日志。

- 输出 Markdown 报告和 CSV 数据。

常用命令：

```bash
python scripts/perf_analysis/dirty_region_stats_report.py \
  --input showcase=perf_output/dirty_region_logs/showcase.log \
  --input stage=perf_output/dirty_region_logs/virtual_stage_showcase.log \
  --output-prefix perf_output/dirty_region_compare
```

适合场景：

- 想把两组日志转成可比的报告。

- 想把 dirty area、dirty ratio、pfb tiles 给到文档或评审。

## `hello_basic_render_workflow.py`

用途：

- 面向 `HelloBasic` 和 `HelloVirtual` 的批量回归入口。

- 除了录制截图，还会检查交互前后是否真的发生变化。

- 会输出结构化 JSON 报告，方便 CI 使用。

常用命令：

```bash
python scripts/hello_basic_render_workflow.py --app HelloBasic --suite smoke
python scripts/hello_basic_render_workflow.py --app HelloBasic --suite basic
python scripts/hello_basic_render_workflow.py --app HelloVirtual --widgets virtual_stage_showcase
```

适合场景：

- 基础控件和 virtual 相关例程做一轮批量回归。

- 想确认录制动作没有缺失，截图也不是空白。

## `showcase_stage_parity_check.py`

用途：

- 对比 `HelloShowcase` 和 `HelloVirtual/virtual_stage_showcase` 的关键状态帧。

- 自动生成并排图和差异图。

常用命令：

```bash
python scripts/showcase_stage_parity_check.py --timeout 35 --bits64
```

适合场景：

- 做 virtual stage 替换或优化后，确认视觉结果没有跑偏。

- 需要保留对比图给本地调试记录。

## 一个简单的使用建议

- 先用 `code_runtime_check.py` 看“能不能跑、画面是不是对”。

- 再用 `hello_basic_render_workflow.py` 或 `code_dirty_animation_check.py` 做专项回归。

- 需要定量分析时，再接 `dirty_region_stats_report.py`。
