# size_analysis

`size_analysis/` 用来集中放置体积分析脚本，默认都在仓库根目录执行。

## 常用入口

- `python scripts/size_analysis/utils_analysis_elf_size.py --case-set typical`
  统计典型示例的代码、资源、RAM、PFB 占用，输出到 `output/size_results.json` 和 `output/README.md`。
- `python scripts/size_analysis/utils_analysis_elf_size.py --case-set typical --doc`
  在生成体积结果的同时刷新 `doc/source/size/` 文档。
- `python scripts/size_analysis/size_to_doc.py`
  基于已有 `output/size_results.json` 重新生成总览文档和图表。
- `python scripts/size_analysis/run_size_suite.py --quick`
  快速跑一遍 HQ / canvas / widget / preset 这几组专项体积报告。

## 专项报告脚本

- `hq_size_to_doc.py`
  分析 HQ 绘制路径的链接体积。
- `canvas_path_size_to_doc.py`
  分析基础 canvas path 的体积拆分。
- `canvas_feature_size_to_doc.py`
  分析 canvas feature 组合能力的体积成本。
- `widget_feature_size_to_doc.py`
  分析真实 widget 的链接体积。
- `size_preset_validation_to_doc.py`
  对比 `HelloSizeAnalysis/ConfigProfiles` 里的配置模板。

## 使用约定

- 主要输出在 `output/`，文档输出在 `doc/source/size/`。
- 需要批量刷新报告时优先用 `run_size_suite.py`。
- 这些脚本会修改 `example/HelloSizeAnalysis/` 下的 probe / override 头文件，跑完后如需复查，先看对应输出文档是否已更新。
