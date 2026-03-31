---
name: performance-analysis
description: QEMU性能分析工作流 - 用于绘制优化、PFB设计、多缓冲策略等框架设计决策的量化依据
---

# Performance Analysis Skill

用于框架级性能分析（绘制、PFB、SPI多缓冲）。性能数据必须基于 QEMU，不使用 PC 模拟器计时。

## 前置条件

- 已安装 `qemu-system-arm`（Windows 常见路径 `C:\Program Files\qemu\`）
- 可执行：
  ```bash
  make all APP=HelloPerformance PORT=qemu CPU_ARCH=cortex-m3
  ```
  说明：项目目录名是 `HelloPerformance`（按仓库实际拼写）。

## 常用命令

### 1) CPU基准（单配置）

```bash
python scripts/code_perf_check.py --profile cortex-m3
```

输出：
- `perf_output/perf_results.json`
- `perf_output/perf_report.md`

### 2) PFB矩阵

```bash
python scripts/code_perf_check.py --pfb-matrix
```

输出：
- `perf_output/pfb_matrix_results.json`
- `perf_output/pfb_matrix_report.md`

### 3) SPI矩阵（缓冲区数量/传输速率）

```bash
python scripts/code_perf_check.py --spi-matrix
```

输出：
- `perf_output/spi_matrix_results.json`
- `perf_output/spi_matrix_report.md`

### 4) 一次跑全套

```bash
python scripts/code_perf_check.py --full-check
```

## 建议工作流

1. 修改前先跑一次目标测试，保存结果文件
2. 修改后再跑同一测试
3. 对比两次 `perf_output/*.json` 或报告中的关键耗时项
4. 若变慢超过阈值（建议 10%），定位回归点再优化

## 报告交付要求

每次性能测试完成后，需向用户展示对应报告文件中的关键表格（不要只给一句结论）：

- CPU基准：`perf_output/perf_report.md`
- PFB矩阵：`perf_output/pfb_matrix_report.md`
- SPI矩阵：`perf_output/spi_matrix_report.md`

## 配置位置

- CPU/PFB/SPI 测试矩阵：`scripts/perf_cpu_profiles.json`
- QEMU SPI 模拟实现：`porting/qemu/egui_port_qemu.c`
- QEMU 入口：`porting/qemu/port_main.c`

## 适用场景

- 绘制算法优化前后对比
- PFB 尺寸选型
- 单/双/三/四缓冲收益评估
- SPI 瓶颈与 CPU 瓶颈判定

## 生成性能报告图片

每次更新性能测试报告时，必须同时更新文档中的性能报告图片。使用 `--doc` 参数会自动生成所有图片并复制到文档目录。

### 一次性生成所有报告和图片（推荐）

```bash
python scripts/code_perf_check.py --full-check --doc
```

这个命令会：
1. 运行 CPU 基准测试、PFB 矩阵测试、SPI 矩阵测试
2. 自动生成场景截图（246 个测试场景）
3. 生成所有性能报告图表：
   - `doc/source/performance/images/perf_report.png` - CPU 基准性能柱状图
   - `doc/source/performance/images/pfb_matrix_report.png` - PFB 矩阵热力图
   - `doc/source/performance/images/spi_matrix_report.png` - SPI 矩阵对比图
   - `doc/source/performance/images/perf_scenes.png` - 场景截图合集（2.6MB）
4. 更新所有 Markdown 报告文件

### 仅生成文档和图片（使用已有测试结果）

```bash
python scripts/code_perf_check.py --doc
```

如果 `perf_output/` 目录下已经有测试结果（`perf_results.json`, `pfb_matrix_results.json`, `spi_matrix_results.json`），这个命令会：
1. 重新运行一次 CPU 基准测试（用于生成场景截图）
2. 生成场景截图
3. 生成所有图表和报告

### 图片生成说明

- **perf_report.png**：由 `scripts/perf_to_doc.py` 的 `generate_perf_report()` 生成，使用 matplotlib 绘制分类柱状图
- **pfb_matrix_report.png**：由 `scripts/perf_to_doc.py` 的 `generate_pfb_matrix_report()` 生成，使用 matplotlib 绘制热力图
- **spi_matrix_report.png**：由 `scripts/perf_to_doc.py` 的 `generate_spi_matrix_report()` 生成，使用 matplotlib 绘制对比柱状图
- **perf_scenes.png**：由 `scripts/perf_scene_capture.py` 生成，使用 PC 模拟器渲染所有测试场景并拼接成联系表

### 重要提示

- 性能数据必须基于 QEMU（使用微秒级计时器），不使用 PC 模拟器计时
- 场景截图使用 PC 模拟器渲染（仅用于视觉参考，不影响性能数据）
- 所有图片会自动复制到 `doc/source/performance/images/` 目录
- 更新性能报告时，必须同时更新所有图片，确保数据一致性
