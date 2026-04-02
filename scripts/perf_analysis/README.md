# perf_analysis

`perf_analysis/` 用来集中放置性能分析相关脚本，默认都在仓库根目录执行。

## 常用入口

- `python scripts/perf_analysis/code_perf_check.py --profile cortex-m3`
  运行单个 QEMU 性能基准，输出到 `perf_output/`。
- `python scripts/perf_analysis/code_perf_check.py --full-check --doc`
  跑完整 CPU / PFB / SPI 基准，并生成 `doc/source/performance/` 下的文档和图片。
- `python scripts/perf_analysis/perf_to_doc.py`
  基于已有 `perf_output/*.json` 重新生成性能报告。
- `python scripts/perf_analysis/perf_scene_capture.py`
  采集 HelloPerformance 场景截图并拼接 `perf_scenes.png`。

## 其他脚本

- `compare_virtual_showcase_heap_qemu.py`
  对比 `HelloShowcase` 和 `HelloVirtual/virtual_stage_showcase` 的 QEMU 堆峰值。
- `compare_virtual_showcase_ram.py`
  对比两个示例自有对象的静态 SRAM 占用。
- `dirty_region_stats_report.py`
  把 `DIRTY_REGION_STATS` 日志整理成 Markdown / CSV 报告。
- `perf_compare.py`
  对比两份 `perf_results.json`，输出 Markdown 表格。
- `perf_cpu_profiles.json`
  维护 `code_perf_check.py` 使用的 CPU / PFB / SPI 测试矩阵。

## 使用约定

- 性能数据以 QEMU 结果为准，PC 运行只用于截图和可视化辅助。
- 主要输出目录是 `perf_output/`。
- 更新性能文档时，优先使用 `code_perf_check.py --doc` 或 `--full-check --doc`，避免报告和图表不同步。
- 生成性能图表依赖 `matplotlib`；首次在虚拟环境执行前先运行 `pip install -r requirements.txt`。
