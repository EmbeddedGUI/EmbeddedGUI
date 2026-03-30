# QEMU Size Analysis

EmbeddedGUI 的 size 文档现在只覆盖指定示例集合，并统一基于 qemu 口径生成静态 size 与运行期 heap/stack 数据。

## Scope

- HelloBasic 全子 case、HelloSimple、HelloPerformance、HelloShowcase、HelloStyleDemo、HelloVirtual(virtual_stage_showcase)

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
python scripts/utils_analysis_elf_size.py
python scripts/size_to_doc.py
```
## QEMU vs stm32g0_empty

- 对比范围：61 个共同成功 case；qemu 为当前正式口径，`stm32g0_empty` 仅用于静态 size 交叉验证。
- Code delta（qemu - stm32g0_empty）：min -728 B / median -728 B / max -292 B。
- ROM delta（code + resource）：min -520 B / median -520 B / max -80 B。
- RAM delta（不含 PFB）：min -120 B / median -112 B / max 48 B。
- PFB delta：min 0 B / median 0 B / max 0 B。
- 超过阈值统计：`code > +10KB` 为 0 个，`ram > +1KB` 为 0 个。
- 结论：当前 qemu 口径没有引入用户担心的 `>10KB` 代码膨胀或 `>1KB` 静态 RAM 膨胀，并且额外提供了运行期 heap/stack 数据。
- 旧链路额外维护成本：`stm32g0_empty` 在 Windows 下仍有 3 个大 case 无法稳定完成链接：HelloBasic(mp4)、HelloPerformance、HelloVirtual(virtual_stage_showcase)。
