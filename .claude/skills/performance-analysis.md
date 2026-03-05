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
  make all APP=HelloPerformace PORT=qemu CPU_ARCH=cortex-m3
  ```
  说明：项目目录名是 `HelloPerformace`（按仓库实际拼写）。

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
