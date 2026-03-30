# HelloPerformance 基准 Override 保留记录

## 范围

- 本文只记录当前仍需保留的基准矩阵入口：
  - `EGUI_CONFIG_PFB_WIDTH`
  - `EGUI_CONFIG_PFB_HEIGHT`
  - `EGUI_CONFIG_PFB_BUFFER_COUNT`
- 判定规则仍沿用本轮 small static-RAM cleanup：
  - 只回收 `static RAM <100B`
  - 且 `HelloPerformance` QEMU perf 最大回退 `<5%`

## 本次支撑报告

- 测量时间：`2026-03-31`
- 测量提交：`885c25c`
- 命令：
  - `python scripts/code_perf_check.py --pfb-matrix`
  - `python scripts/code_perf_check.py --spi-matrix`
- 这些报告对应 `scripts/code_perf_check.py` 里通过 `USER_CFLAGS` 做的外部覆盖：
  - `EGUI_CONFIG_PFB_WIDTH`
  - `EGUI_CONFIG_PFB_HEIGHT`
  - `EGUI_CONFIG_PFB_BUFFER_COUNT`

## 当前 PFB 几何的 SRAM 量级

- 当前 HelloPerformance 固定值：`48 x 16 x 1buf`
  - `48 * 16 * 2 = 1536B`
- 若只把 `PFB_WIDTH/PFB_HEIGHT` 回退到库默认 `30 x 30`，并保持 `1buf`
  - `30 * 30 * 2 = 1800B`
  - 相比当前 `+264B`
- 若只把 `PFB_BUFFER_COUNT` 从 `1` 回到 `2`
  - `48 * 16 * 2 * 2 = 3072B`
  - 相比当前 `+1536B`
- 若三者全部回到库默认 `30 x 30 x 2buf`
  - `30 * 30 * 2 * 2 = 3600B`
  - 相比当前 `+2064B`

结论：这三项都不是 `<100B` 级别的小 SRAM 宏，本轮不属于“默认打开后顺手去掉宏管理”的对象。

## 宏结论

### `EGUI_CONFIG_PFB_WIDTH` / `EGUI_CONFIG_PFB_HEIGHT`

- `--pfb-matrix` 当前就是靠外部覆盖这两个宏来生成基准矩阵；它们不是死 override，而是现行测量入口。
- `2026-03-31` 的 QEMU `PFB` matrix 显示，`PFB` 几何本身就是明显的 RAM/perf 取舍轴，不存在“回到默认值几乎没影响”的结论。例如：
  - `TEXT`: `15x15=3.783ms`, `30x30=1.108ms`, `240x1=3.675ms`, `1x240=4.646ms`, `240x240=0.204ms`
  - `IMAGE_QOI_565_8`: `15x15=2.432ms`, `30x30=1.771ms`, `240x1=2.191ms`, `1x240=2.237ms`, `240x240=1.633ms`
  - `IMAGE_TILED_RLE_565_0`: `15x15=2.728ms`, `30x30=0.895ms`, `240x1=3.744ms`, `1x240=6.399ms`, `240x240=0.228ms`
- 当前 shipped 几何 `48x16` 的作用是把 HelloPerformance 固定在低 SRAM 路径上继续测量，而不是宣称它是普适最快配置。
- 处理结论：保留 `#ifndef` 外部覆盖入口，同时继续固定 HelloPerformance 默认 `48x16`。

### `EGUI_CONFIG_PFB_BUFFER_COUNT`

- `--spi-matrix` 当前就是靠外部覆盖 `EGUI_CONFIG_PFB_BUFFER_COUNT` 配合 SPI 速率做传输仿真；它同样不是死 override。
- `2026-03-31` 的 QEMU `SPI` matrix 显示，这个宏描述的是“CPU-only”与“带传输瓶颈的双缓冲仿真”两类基准环境，不是小 SRAM 微调。例如：
  - `ANIMATION_SCALE`: `no_spi=0.320ms`, `spi16_buf2=60.000ms`, `spi64_buf2=15.000ms`
  - `TEXT_ROTATE_BUFFERED`: `no_spi=11.498ms`, `spi16_buf2=64.200ms`, `spi64_buf2=17.100ms`
  - `EXTERN_IMAGE_ROTATE_565_8`: `no_spi=18.990ms`, `spi16_buf2=77.000ms`, `spi64_buf2=32.000ms`
- 同时，当前几何下 `1buf -> 2buf` 直接增加 `1536B` PFB 静态占用，也明显超出本轮 `<100B` 门槛。
- 处理结论：保留 `#ifndef` 外部覆盖入口，同时继续固定 HelloPerformance 默认 `1buf`。

## 最终结论

- `EGUI_CONFIG_PFB_WIDTH`
- `EGUI_CONFIG_PFB_HEIGHT`
- `EGUI_CONFIG_PFB_BUFFER_COUNT`

以上三项都应归类为 HelloPerformance 的基准矩阵入口，而不是 small static-RAM cleanup 候选。
