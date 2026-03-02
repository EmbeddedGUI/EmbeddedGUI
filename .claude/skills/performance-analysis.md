---
name: performance-analysis
description: QEMU性能分析工作流 - 用于绘制优化、PFB设计、多缓冲策略等框架设计决策的量化依据
---

# Performance Analysis Skill

在进行绘制优化、PFB尺寸调整、多缓冲策略评估等框架设计工作时，使用此skill获取量化性能数据作为决策依据。

## 前置条件

- 已安装 QEMU (`qemu-system-arm`)，Windows 默认路径 `C:\Program Files\qemu\`
- 项目根目录下可正常执行 `make all APP=HelloPerformace PORT=qemu CPU_ARCH=cortex-m3`

## 测试模式

### 模式1：CPU纯计算性能基准

测量各绘制原语的纯CPU耗时（无SPI传输开销），用于评估绘制算法优化效果。

```bash
python scripts/code_perf_check.py --profile cortex-m3
```

输出：`perf_output/perf_results.json` + `perf_output/perf_report.md`

适用场景：
- 优化某个绘制函数（圆、弧、渐变等）后，对比前后性能
- 评估不同CPU架构的绘制性能差异

### 模式2：PFB尺寸矩阵

遍历多种PFB尺寸配置，测量对各绘制原语的性能影响，用于PFB设计决策。

```bash
python scripts/code_perf_check.py --pfb-matrix
```

输出：`perf_output/pfb_matrix_results.json` + `perf_output/pfb_matrix_report.md`

PFB配置定义在 `scripts/perf_cpu_profiles.json` 的 `pfb_configs` 中，可按需增删：
```json
"pfb_configs": {
    "default":    {"pfb_width": 30,  "pfb_height": 40,  "description": "Default (30x40)"},
    "small":      {"pfb_width": 15,  "pfb_height": 20,  "description": "Small (15x20)"},
    "large":      {"pfb_width": 120, "pfb_height": 80,  "description": "Large (120x80)"},
    "fullwidth":  {"pfb_width": 240, "pfb_height": 20,  "description": "Full-width strip"},
    "fullscreen": {"pfb_width": 240, "pfb_height": 320, "description": "Full-screen"}
}
```

适用场景：
- 确定最优PFB尺寸（平衡RAM占用与绘制效率）
- 评估全宽条带 vs 小块PFB的性能差异

### 模式3：SPI传输模拟矩阵

模拟真实SPI总线传输延迟，测量不同缓冲区数量下的CPU/SPI重叠效果。

```bash
python scripts/code_perf_check.py --spi-matrix
```

输出：`perf_output/spi_matrix_results.json` + `perf_output/spi_matrix_report.md`

SPI配置定义在 `scripts/perf_cpu_profiles.json` 的 `spi_configs` 中：
```json
"spi_configs": [
    {"name": "no_spi",     "spi_mhz": 0,  "buffer_count": 1, "desc": "CPU only"},
    {"name": "spi32_buf1", "spi_mhz": 32, "buffer_count": 1, "desc": "32MHz SPI, single buffer"},
    {"name": "spi32_buf2", "spi_mhz": 32, "buffer_count": 2, "desc": "32MHz SPI, double buffer"},
    {"name": "spi32_buf3", "spi_mhz": 32, "buffer_count": 3, "desc": "32MHz SPI, triple buffer"},
    {"name": "spi32_buf4", "spi_mhz": 32, "buffer_count": 4, "desc": "32MHz SPI, quad buffer"}
]
```

适用场景：
- 评估多缓冲策略的实际收益（CPU/SPI重叠度）
- 确定不同SPI速率下的最优缓冲区数量
- 分析SPI瓶颈 vs CPU瓶颈的临界点

### 模式4：回归检测

对比当前代码与基线的性能差异，检测性能退化。

```bash
# 保存当前结果为基线
python scripts/code_perf_check.py --update-baseline

# 后续检查是否有回归（默认阈值10%）
python scripts/code_perf_check.py --profile cortex-m3 --threshold 10
```

适用场景：
- 重构核心绘制代码前后的回归检测
- CI集成性能门禁

## 典型工作流

### 重要：测试完成后必须展示报告

每次性能测试运行完成后，AI **必须**使用 Read 工具读取并向用户展示以下报告文件的内容：

- PFB矩阵测试后：读取并展示 `perf_output/pfb_matrix_report.md`
- SPI矩阵测试后：读取并展示 `perf_output/spi_matrix_report.md`
- CPU基准测试后：读取并展示 `perf_output/perf_report.md`

如果同时运行了多种测试，所有对应的报告都要展示。不要只给摘要，要展示完整的报告表格。

### 绘制算法优化

1. 运行基准获取优化前数据：
   ```bash
   python scripts/code_perf_check.py --update-baseline
   ```
2. 修改绘制代码
3. 运行对比：
   ```bash
   python scripts/code_perf_check.py --profile cortex-m3
   ```
4. 查看 `perf_output/perf_report.md` 中的 `vs Baseline` 列

### PFB框架设计

1. 运行PFB矩阵：
   ```bash
   python scripts/code_perf_check.py --pfb-matrix
   ```
2. 查看 `perf_output/pfb_matrix_report.md` 中的 `Best PFB Config Per Test` 表
3. 如需评估SPI场景下的PFB影响，可在 `spi_configs` 中添加带 `pfb_width`/`pfb_height` 的配置

### 多缓冲策略评估

1. 运行SPI矩阵：
   ```bash
   python scripts/code_perf_check.py --spi-matrix
   ```
2. 查看 `perf_output/spi_matrix_report.md` 中的 `Multi-Buffer Speedup` 表
3. 关键指标：
   - SPI floor = 屏幕像素 × 2字节 × 8 / SPI速率（240×320×16/32MHz = 38.4ms）
   - 单缓冲帧时间 = CPU时间 + SPI时间（串行）
   - 多缓冲帧时间 ≈ max(CPU时间, SPI时间)（理想重叠）

## 自定义SPI配置

在 `scripts/perf_cpu_profiles.json` 的 `spi_configs` 数组中添加条目即可。支持的字段：

| 字段 | 说明 |
|------|------|
| `name` | 配置名称（用于报告列标题） |
| `spi_mhz` | SPI时钟频率，0表示无SPI模拟 |
| `buffer_count` | PFB缓冲区数量（1-4） |
| `pfb_width` | 可选，覆盖PFB宽度 |
| `pfb_height` | 可选，覆盖PFB高度 |
| `desc` | 描述文字 |

## SPI模拟原理

SPI模拟在 `porting/qemu/egui_port_qemu.c` 中实现：
- `QEMU_SPI_SPEED_MHZ` 宏控制SPI速率（在 `app_egui_config.h` 中定义）
- SysTick 100us周期提供定时基准
- `draw_area_async()` 启动异步传输，设置tick deadline
- `SysTick_Handler` 中的 `spi_sim_poll()` 检测传输完成，调用 `egui_pfb_notify_flush_complete()`
- 多缓冲时CPU可在SPI传输期间渲染下一个PFB块

## 测试覆盖的绘制原语（42项）

LINE, IMAGE_565, IMAGE_565_1/2/4/8, IMAGE_RESIZE_565, IMAGE_RESIZE_565_1/2/4/8, TEXT, TEXT_RECT, RECTANGLE, CIRCLE, ARC, ROUND_RECTANGLE, ROUND_RECTANGLE_CORNERS, RECTANGLE_FILL, CIRCLE_FILL, ARC_FILL, ROUND_RECTANGLE_FILL, ROUND_RECTANGLE_CORNERS_FILL, TRIANGLE, TRIANGLE_FILL, ELLIPSE, ELLIPSE_FILL, POLYGON, POLYGON_FILL, BEZIER_QUAD, BEZIER_CUBIC, CIRCLE_HQ, CIRCLE_FILL_HQ, ARC_HQ, ARC_FILL_HQ, LINE_HQ, SHADOW, SHADOW_ROUND, GRADIENT_RECT, GRADIENT_CIRCLE, GRADIENT_ROUND_RECT, GRADIENT_TRIANGLE

## 文件参考

| 文件 | 说明 |
|------|------|
| `scripts/code_perf_check.py` | 性能测试自动化脚本 |
| `scripts/perf_cpu_profiles.json` | CPU/PFB/SPI配置定义 |
| `porting/qemu/egui_port_qemu.c` | QEMU移植层（含SPI模拟） |
| `porting/qemu/port_main.c` | QEMU入口（多缓冲分配） |
| `porting/qemu/startup_qemu.s` | 启动文件和中断向量表 |
| `example/HelloPerformace/uicode.c` | 性能测试用例代码 |
| `example/HelloPerformace/app_egui_config.h` | 测试配置（SPI速率、缓冲区数等） |
| `perf_output/` | 测试结果输出目录 |
