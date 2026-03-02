# Binary Size Analysis

EmbeddedGUI is designed for resource-constrained embedded systems. Binary size analysis helps track ROM/RAM usage across all examples and detect unexpected size growth.

## Measurement Method

- **Platform**: STM32G0 (Cortex-M0+, stm32g0_empty board)
- **Toolchain**: arm-none-eabi-gcc
- **Data Source**: ELF symbols (`__code_size`, `__rodata_size`, `__data_size`, `__bss_size`, `__bss_pfb_size`)

## Size Categories

| Category | Description |
|----------|-------------|
| Code | Text section (.text) - executable code |
| Resource | Read-only data (.rodata) - fonts, images, etc. |
| RAM | Data + BSS sections - global variables |
| PFB | Partial Frame Buffer - BSS reserved for PFB |

## Generation Command

```bash
# Generate size analysis
python scripts/utils_analysis_elf_size.py

# Generate documentation report
python scripts/size_to_doc.py
```