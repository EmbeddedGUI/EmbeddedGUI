# 调试脚本

这里集中说明和调试、运行时验证、截图录制直接相关的脚本。

## 总表

| 脚本 | 主要用途 | 常用命令 | 主要输出 |
| --- | --- | --- | --- |
| `scripts/code_runtime_check.py` | 运行示例、自动录制、保留截图 | `python scripts/code_runtime_check.py --app HelloBasic --app-sub slider --keep-screenshots` | `runtime_check_output/<app>/` |
| `scripts/code_compile_check.py` | 批量编译示例并跑 `HelloUnitTest` | `python scripts/code_compile_check.py --full-check` | 终端日志 |
| `scripts/release_check.py` | 串起多步骤本地回归入口 | `python scripts/release_check.py --scope multi-display` | 汇总日志 |
| `scripts/checks/code_dirty_animation_check.py` | 检查代表性动画是否仍是局部刷新 | `python scripts/checks/code_dirty_animation_check.py --scenario showcase` | `runtime_check_output/dirty_animation_check/` |
| `scripts/perf_analysis/dirty_region_stats_report.py` | 把 `DIRTY_REGION_STATS` 日志转成 Markdown/CSV | `python scripts/perf_analysis/dirty_region_stats_report.py --input showcase=perf_output/showcase.log` | `*.md`、`*.csv` |
| `scripts/checks/hello_basic_render_workflow.py` | 批量检查 `HelloBasic` / `HelloVirtual` 的录制动作、截图和交互变化 | `python scripts/checks/hello_basic_render_workflow.py --app HelloVirtual --suite basic` | JSON 报告和截图 |
| `scripts/checks/showcase_stage_parity_check.py` | 对比 `HelloShowcase` 和 `virtual_stage_showcase` 的关键画面状态 | `python scripts/checks/showcase_stage_parity_check.py --bits64` | 对比图、diff 图 |

## `code_runtime_check.py`

用途：

- 编译指定应用
- 打开录制模式运行
- 校验是否生成 PNG 帧
- 检查是否出现 `[RUNTIME_CHECK_FAIL]`

常用命令：

```bash
python scripts/code_runtime_check.py --app HelloShowcase --keep-screenshots
python scripts/code_runtime_check.py --app HelloBasic --app-sub slider --keep-screenshots
python scripts/code_runtime_check.py --app HelloVirtual --app-sub virtual_stage_showcase --keep-screenshots
python scripts/code_runtime_check.py --scope multi-display --jobs 2 --keep-screenshots
```

其中 `--scope multi-display` 会只跑：

- `HelloMultiDisplay`
- `HelloMultiDisplayHetero`

并额外按 `EGUI_CONFIG_MAX_DISPLAY_COUNT` 校验各个 display 的录制帧是否成套对应；主屏使用 `frame_XXXX.png`，额外屏幕使用 `frame_XXXX_dispN.png`。对于 `HelloMultiDisplay` 和 `HelloMultiDisplayHetero`，还会校验录制阶段标签序列，确认主屏点击、副屏点击/拖动等关键步骤确实走到了目标状态，并校验退出摘要里的 `begin_shutdown -> core_threads_stopped -> sdl_cleanup -> deinit_done` 顺序。

适合场景：

- 修改单个示例后，先做一次最基本的运行确认
- 想保留截图给文档或 review
- 做多屏改动后，快速回归双屏录制和副屏输入链路

## `code_compile_check.py`

用途：

- 批量编译标准示例
- 在全量模式下跑 `HelloUnitTest`
- 同时覆盖 `HelloBasic`、`HelloVirtual`、`HelloSizeAnalysis` 的子示例

常用命令：

```bash
python scripts/code_compile_check.py --full-check
python scripts/code_compile_check.py --scope multi-display --case-jobs 2
```

其中 `--scope multi-display` 会只编译多屏相关示例，适合多屏改动后的快速编译回归。

适合场景：

- 提交前做一次编译面回归
- 修改多屏入口、descriptor、线程模型后，先快速确认双屏示例都还能编过

## `release_check.py`

用途：

- 把多个检查步骤串成一条本地回归命令
- 适合需要一次性收口 compile、runtime 和文档校验的时候使用

常用命令：

```bash
python scripts/release_check.py --skip perf,wasm,doc
python scripts/release_check.py --scope multi-display
python scripts/release_check.py --scope multi-display --only runtime
```

其中 `--scope multi-display` 当前会收敛到：

- `code_compile_check.py --scope multi-display`
- `code_runtime_check.py --scope multi-display`
- Sphinx dummy 文档校验

其中 runtime scope 会补充校验：

- 多屏录制帧是否按 display 成套输出
- 录制阶段标签是否完整
- `HelloMultiDisplay` 的主屏点击隔离与双屏并发 activity 动画
- `HelloMultiDisplayHetero` 的副屏 tick 连续性与点击复位
- `begin_shutdown -> core_threads_stopped -> sdl_cleanup -> deinit_done` 退出顺序

适合场景：

- 多屏线程、descriptor、副屏输入或录制脚本改动后的本地快速收口
- 不想手动拆 compile/runtime/doc 三条命令时

如果一时记不清 scoped profile 的覆盖范围，直接执行 `python scripts/release_check.py --help`，末尾的 `Scoped profiles` 会把 `multi-display` 的 apps、runtime checks、`--only` 可用 step、drill-down 命令和关键产物目录直接列出来；真正运行 `python scripts/release_check.py --scope multi-display` 时，脚本开头也会直接打印这些信息和当前激活的 `--only/--skip` 过滤结果，方便失败后继续拆查，summary 尾部会把失败步骤对应的 `python scripts/release_check.py --scope multi-display --only <step>`、底层命令与产物位置再列一遍，通过时也会把关键产物目录再汇总一遍。

## `code_dirty_animation_check.py`

用途：

- 对代表性动画做 dirty region 专项回归
- 自动打开 `EGUI_CONFIG_DEBUG_DIRTY_REGION_STATS=1`
- 统计 unique frame、partial frame、平均脏区比例

常用命令：

```bash
python scripts/checks/code_dirty_animation_check.py --scenario showcase
python scripts/checks/code_dirty_animation_check.py --scenario virtual_stage_showcase
```

适合场景：

- 优化脏区或动画后，确认没有回退成全屏刷新
- 检查 `HelloShowcase` 和 `virtual_stage_showcase` 这类复杂动效页面

## `dirty_region_stats_report.py`

用途：

- 解析 `DIRTY_REGION_STATS:` 日志
- 输出 Markdown 报告和 CSV 数据

常用命令：

```bash
python scripts/perf_analysis/dirty_region_stats_report.py \
  --input showcase=perf_output/dirty_region_logs/showcase.log \
  --input stage=perf_output/dirty_region_logs/virtual_stage_showcase.log \
  --output-prefix perf_output/dirty_region_compare
```

适合场景：

- 想把两组日志转成可比较的报告
- 想把 dirty area、dirty ratio、pfb tiles 给到文档或评审

## `hello_basic_render_workflow.py`

用途：

- 面向 `HelloBasic` 和 `HelloVirtual` 的批量回归入口
- 除了录制截图，还会检查交互前后是否真的发生画面变化
- 会输出结构化 JSON 报告，方便 CI 使用

常用命令：

```bash
python scripts/checks/hello_basic_render_workflow.py --app HelloBasic --suite smoke
python scripts/checks/hello_basic_render_workflow.py --app HelloBasic --suite basic
python scripts/checks/hello_basic_render_workflow.py --app HelloVirtual --widgets virtual_stage_showcase
```

适合场景：

- 基础控件和 virtual 相关示例做一轮批量回归
- 想确认录制动作没有缺失，截图也不是空白

## `showcase_stage_parity_check.py`

用途：

- 对比 `HelloShowcase` 和 `HelloVirtual/virtual_stage_showcase` 的关键状态帧
- 自动生成并排图和差异图

常用命令：

```bash
python scripts/checks/showcase_stage_parity_check.py --timeout 35 --bits64
```

适合场景：

- 做 virtual stage 替换或优化后，确认视觉结果没有跑偏
- 需要保留对比图给本地调试记录

## 一个简单的使用建议

- 先用 `code_runtime_check.py` 看“能不能跑、画面是不是对”
- 再用 `hello_basic_render_workflow.py` 或 `code_dirty_animation_check.py` 做专项回归
- 涉及多屏时，优先先跑 `release_check.py --scope multi-display`，需要拆看编译日志和副屏截图时再分别跑 compile/runtime scope
- 需要定量分析时，再接 `dirty_region_stats_report.py`
