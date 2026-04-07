# perf_analysis

`perf_analysis/` 用来集中放置性能分析相关脚本，默认都在仓库根目录执行。

## 常用入口

- `python scripts/perf_analysis/main.py`
  统一入口，默认运行性能基准检查。
- `python scripts/perf_analysis/main.py --profile cortex-m3`
  运行单个 QEMU 性能基准，输出到 `perf_output/`。
- `python scripts/perf_analysis/main.py --full-check --doc`
  跑完整 CPU / PFB / SPI 基准，并生成 `doc/source/performance/` 下的文档和图片。
- `python scripts/perf_analysis/main.py perf-to-doc`
  基于已有 `perf_output/*.json` 重新生成性能报告。
- `python scripts/perf_analysis/main.py scene-capture`
  采集 HelloPerformance 场景截图并拼接 `perf_scenes.png`。
- `python scripts/perf_analysis/main.py help`
  查看统一入口支持的子命令列表。

## 其他入口

- `python scripts/perf_analysis/main.py compare-virtual-heap-qemu`
  对比 `HelloShowcase` 和 `HelloVirtual/virtual_stage_showcase` 的 QEMU 堆峰值。
- `python scripts/perf_analysis/main.py compare-virtual-ram`
  对比两个示例自有对象的静态 SRAM 占用。
- `python scripts/perf_analysis/main.py dirty-region-report --input ...`
  把 `DIRTY_REGION_STATS` 日志整理成 Markdown / CSV 报告。
- `python scripts/perf_analysis/main.py perf-compare <baseline.json> <current.json>`
  对比两份 `perf_results.json`，输出 Markdown 表格。
- `perf_cpu_profiles.json`
  维护默认性能入口使用的 CPU / PFB / SPI 测试矩阵。

## 使用约定

- 性能数据以 QEMU 结果为准，PC 运行只用于截图和可视化辅助。
- 主要输出目录是 `perf_output/`。
- 更新性能文档时，优先使用 `python scripts/perf_analysis/main.py --doc` 或 `python scripts/perf_analysis/main.py --full-check --doc`，避免报告和图表不同步。
- 生成性能图表依赖 `matplotlib`，首次在虚拟环境执行前先运行 `pip install -r requirements.txt`。
- 旧的脚本直调方式仍保留兼容，但文档统一使用 `main.py` 入口。
