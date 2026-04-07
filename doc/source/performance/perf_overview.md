# 性能测试框架介绍

## 概述

EmbeddedGUI 的性能测试框架基于 QEMU ARM 仿真环境，提供确定性、可重复的性能基准测试。框架自动构建测试固件、在 QEMU 中运行、解析结果并与基线对比，检测性能回归。

## 为什么用 QEMU 而不是 PC 模拟器

PC 模拟器（SDL 后端）的系统计时器精度只有 1ms，对于微秒级的绘图操作来说粒度太粗。同一段代码在 PC 上多次运行的计时结果波动很大，无法作为可靠的性能基准。

QEMU 配合 `-icount shift=0` 参数可以实现指令级确定性计时：

- 每条 ARM 指令消耗固定的虚拟时钟周期
- 不受宿主机负载影响，结果完全可重复
- 通过 `qemu_get_tick_us` 获取微秒级时间戳
- 同一代码、同一 commit 多次运行结果一致

这意味着即使 0.1ms 的性能变化也能被可靠检测到。

## 测试环境

### QEMU 配置

测试使用 `qemu-system-arm` 仿真 ARM Cortex-M 系列 MCU：

```bash
qemu-system-arm \
    -machine mps2-an385 \
    -cpu cortex-m3 \
    -nographic \
    -semihosting-config enable=on,target=native \
    -icount shift=0 \
    -kernel output/main.elf
```

关键参数说明：

- `-icount shift=0`：确定性指令计数模式，1 条指令 = 1 个虚拟纳秒
- `-semihosting`：允许固件通过 semihosting 输出结果到宿主机终端
- `-nographic`：无图形界面，纯串口输出

### CPU Profile 配置

框架支持多种 Cortex-M 内核配置，定义在 `scripts/perf_analysis/perf_cpu_profiles.json` 中：

| Profile | QEMU Machine | 典型芯片 |
|---------|-------------|---------|
| cortex-m3 | mps2-an385 | STM32F1, STM32F2 |
| cortex-m4 | mps2-an386 | STM32F4, STM32L4 |
| cortex-m7 | mps2-an500 | STM32F7, STM32H7 |

每个 profile 使用对应的 QEMU machine 和 CPU 模型，确保指令集和流水线行为与真实硬件一致。

## 测试用例

测试应用为 `HelloPerformance`，每个测试用例在 240x320 屏幕上重复绘制 100 次，测量总耗时。

### 基础图元

| 测试项 | 说明 |
|--------|------|
| LINE | 直线绘制 |
| RECTANGLE / RECTANGLE_FILL | 矩形描边/填充 |
| CIRCLE / CIRCLE_FILL | 圆形描边/填充 |
| ARC / ARC_FILL | 圆弧描边/填充 |
| ROUND_RECTANGLE / ROUND_RECTANGLE_FILL | 圆角矩形描边/填充 |
| ROUND_RECTANGLE_CORNERS / ROUND_RECTANGLE_CORNERS_FILL | 独立圆角矩形 |
| TRIANGLE / TRIANGLE_FILL | 三角形描边/填充 |
| ELLIPSE / ELLIPSE_FILL | 椭圆描边/填充 |
| POLYGON / POLYGON_FILL | 多边形描边/填充 |

### 曲线

| 测试项 | 说明 |
|--------|------|
| BEZIER_QUAD | 二次贝塞尔曲线 |
| BEZIER_CUBIC | 三次贝塞尔曲线 |

### 高质量抗锯齿 (HQ)

| 测试项 | 说明 |
|--------|------|
| CIRCLE_HQ / CIRCLE_FILL_HQ | 抗锯齿圆形 |
| ARC_HQ / ARC_FILL_HQ | 抗锯齿圆弧 |
| LINE_HQ | 抗锯齿直线 |

### 图片与文本

| 测试项 | 说明 |
|--------|------|
| IMAGE_565 | RGB565 图片绘制 |
| IMAGE_565_1/2/4/8 | 不同压缩比的 RGB565 图片 |
| IMAGE_RESIZE_565 | 缩放图片绘制 |
| TEXT / TEXT_RECT | 文本渲染 / 带裁剪区域的文本渲染 |

### 高级特效

| 测试项 | 说明 |
|--------|------|
| GRADIENT_RECT / GRADIENT_ROUND_RECT | 渐变矩形/圆角矩形 |
| GRADIENT_CIRCLE / GRADIENT_TRIANGLE | 渐变圆形/三角形 |
| SHADOW / SHADOW_ROUND | 阴影效果 |

## 运行方法

### 完整测试

```bash
# 运行所有 CPU profile 的完整性能测试
python scripts/perf_analysis/main.py --full-check

# 指定单个 profile
python scripts/perf_analysis/main.py --profile cortex-m3

# 自定义超时时间（秒）
python scripts/perf_analysis/main.py --profile cortex-m3 --timeout 600
```

### PFB 矩阵测试

测试不同 PFB（局部帧缓冲）尺寸对性能的影响：

```bash
python scripts/perf_analysis/main.py --pfb-matrix
```

可用的 PFB 配置：

| 名称 | 尺寸 | 分块数 | 内存占用 |
|------|------|--------|---------|
| default | 30x40 | 64 | 2.4 KB |
| medium | 60x40 | 32 | 4.8 KB |
| large | 120x80 | 8 | 19.2 KB |
| fullwidth | 240x20 | 16 | 9.6 KB |
| fullscreen | 240x320 | 1 | 153.6 KB |

### SPI 传输矩阵测试

测试不同 SPI 速率和缓冲区配置对帧传输的影响：

```bash
python scripts/perf_analysis/main.py --spi-matrix
```

## 基线管理

### 结果对比

当前性能入口会输出最新 `perf_results.json`、`pfb_matrix_results.json`、`spi_matrix_results.json` 和对应 Markdown 报告。
如需把当前结果与某份历史基线 JSON 做对比，可使用：

```bash
python scripts/perf_analysis/main.py perf-compare perf_output/perf_baseline_999bb88.json perf_output/perf_results.json
```

`perf-compare` 会输出 Markdown 对比表，便于在评审或 CI 后处理阶段做人工复核。

### 回归检测

框架文档默认使用 10% 作为回归判定阈值。运行性能脚本时可把当前采用的阈值写入报告：

```bash
# 使用自定义阈值
python scripts/perf_analysis/main.py --threshold 15
```

回归检测公式：

```
change_pct = (current_ms - baseline_ms) / baseline_ms * 100
regression = change_pct > threshold_percent
```

## 输出文件

测试完成后，结果保存在 `perf_output/` 目录：

| 文件 | 说明 |
|------|------|
| `perf_results.json` | 当前 CPU profile 的性能结果 |
| `pfb_matrix_results.json` | PFB 尺寸矩阵测试结果 |
| `spi_matrix_results.json` | SPI 配置矩阵测试结果 |
| `perf_report.md` | 文本格式的简要报告 |

### 生成图表文档

使用统一入口把 JSON 数据转换为带图表的 Markdown 文档：

```bash
python scripts/perf_analysis/main.py perf-to-doc
```

生成的文档位于 `doc/source/performance/` 目录，包含柱状图、热力图等可视化图表。

## CI 集成

在 CI 流水线中，性能测试作为回归检查的一部分运行：

```bash
# CI 中的典型用法
python scripts/perf_analysis/main.py --full-check --threshold 10
```

当前脚本会在构建、QEMU 运行或文档生成失败时返回非零退出码；如需把性能回归阈值纳入 CI 判定，建议在后续步骤中结合 `perf-compare` 或自定义基线脚本处理。
