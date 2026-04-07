# QEMU Size Analysis

本目录下的 size 文档统一采用 qemu 口径，目标是同时提供：

- 静态二进制体积
- 运行期 `heap`
- 运行期 `stack`

相比旧的纯 ELF 静态统计方式，这套口径更完整，也更适合做配置取舍和回归比较。

## Scope

- `HelloBasic(button,image,label)`, `HelloSimple`, `HelloShowcase`, `HelloVirtual(virtual_stage_showcase)`

## Measurement Method

- **Static size build**: `make all PORT=qemu CPU_ARCH=cortex-m0plus`
- **Static data source**: `output/main.map` input sections from repo-side `src/` + `example/` objects
- **Static size scope**: Map input sections from repo-side `src/` + `example/` objects only; exclude toolchain libraries, `driver/`, `porting/`; `ram_bytes` excludes `PFB`, and `pfb_bytes` still comes from `.bss.pfb_area`.
- **Runtime measure**: `qemu-system-arm -machine mps2-an385 -cpu cortex-m3 -icount shift=0`
- **Runtime flags**: `-DQEMU_HEAP_MEASURE=1 -DQEMU_HEAP_ACTIONS_APP_RECORDING=1 -DEGUI_CONFIG_RECORDING_TEST=1`
- **Heap peak definition**: `max(idle_peak, interaction_total_peak)`
- **Stack peak definition**: qemu 侧保留栈区的 watermark 高水位统计

## Size Categories

| Category | Description |
|----------|-------------|
| Code | `.text` 可执行代码大小 |
| Resource | `.rodata` 只读资源大小 |
| RAM | `.data + .bss - .bss.pfb_area` 静态 RAM |
| PFB | `Partial Frame Buffer` 静态 RAM |
| Heap Idle | UI 建立完成后的稳定 `heap` 占用 |
| Heap Peak | 初始化或交互阶段出现的最大 `heap` 占用 |
| Stack Peak | qemu 运行时记录到的栈峰值 |

## Generation Command

```bash
python scripts/size_analysis/main.py --case-set typical
python scripts/size_analysis/main.py size-to-doc
```