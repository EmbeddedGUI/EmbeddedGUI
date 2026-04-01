# QEMU Size Analysis

EmbeddedGUI 的 size 文档现在只覆盖指定示例集合，并统一基于 qemu 口径生成静态 size 与运行期 heap/stack 数据。

## Scope

- `HelloBasic(button,image,label)`, `HelloSimple`, `HelloShowcase`, `HelloVirtual(virtual_stage_showcase)`

## Measurement Method

- **Static size build**: `make all PORT=qemu CPU_ARCH=cortex-m0plus`
- **Static data source**: ELF symbols `__code_size`, `__rodata_size`, `__data_size`, `__bss_size`, `__bss_pfb_size`
- **Runtime measure**: `qemu-system-arm -machine mps2-an385 -cpu cortex-m3 -icount shift=0`
- **Runtime flags**: `-DQEMU_HEAP_MEASURE=1 -DQEMU_HEAP_ACTIONS_APP_RECORDING=1 -DEGUI_CONFIG_RECORDING_TEST=1`
- **Heap peak definition**: `max(idle_peak, interaction_total_peak)`
- **Stack peak definition**: qemu 保留栈区的 watermark 高水位统计

## Size Categories

| Category | Description |
|----------|-------------|
| Code | `.text` 可执行代码大小 |
| Resource | `.rodata` 只读资源大小 |
| RAM | `.data + .bss - .bss.pfb_area` 静态 RAM |
| PFB | `Partial Frame Buffer` 静态 RAM |
| Heap Idle | UI 建立后稳定态 heap 占用 |
| Heap Peak | 初始化或交互阶段出现的最大 heap 占用 |
| Stack Peak | qemu 运行期记录到的栈高水位 |

## Generation Command

```bash
python scripts/utils_analysis_elf_size.py --case-set typical
python scripts/size_to_doc.py
```