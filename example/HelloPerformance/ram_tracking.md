# HelloPerformance RAM Tracking

## Scope

- This file tracks `HelloPerformance` static RAM and heap after each RAM-focused change.
- `PFB` is listed for completeness, but it is user-configurable and not an optimization target.
- Static RAM numbers use the normal QEMU performance build:
  - `make clean`
  - `make all APP=HelloPerformance PORT=qemu CPU_ARCH=cortex-m3`
  - `llvm-size output/main.elf`
- Heap numbers use the QEMU measurement build:
  - `make clean`
  - `make all APP=HelloPerformance PORT=qemu CPU_ARCH=cortex-m3 USER_CFLAGS="-DQEMU_HEAP_MEASURE=1 -DQEMU_HEAP_ACTIONS_APP_RECORDING=1 -DEGUI_CONFIG_RECORDING_TEST=1"`
  - `Copy-Item example\HelloPerformance\resource\app_egui_resource_merge.bin output\app_egui_resource_merge.bin -Force`
  - `qemu-system-arm ... -kernel output/main.elf`

## History

| Date | Change | text | data | bss | static RAM | Heap | Notes |
| --- | --- | ---: | ---: | ---: | ---: | --- | --- |
| 2026-03-27 | Alpha change tables moved to rodata | 2172668 | 52 | 22132 | 22184 | 0 / 0 | Baseline before this round |
| 2026-03-27 | Disable HelloPerformance RLE checkpoint fallback state | 2172576 | 52 | 22116 | 22168 | 0 / 0 | `static RAM -16B`, `rle_checkpoint` removed |

## Current Breakdown

### Top Static RAM Symbols

| Symbol | Section | Size (B) | Notes |
| --- | --- | ---: | --- |
| `egui_image_decode_row_cache_pixel` | `.bss` | 7680 | Decode row-band pixel cache |
| `egui_image_decode_row_cache_alpha` | `.bss` | 3840 | Decode row-band alpha cache |
| `egui_pfb` | `.bss` | 1536 | User-configurable PFB, not an optimization target |
| `qoi_state` | `.bss` | 276 | QOI decoder state |
| `egui_core` | `.bss` | 236 | Core runtime state |
| `test_view` | `.bss` | 60 | HelloPerformance scene object |
| `anim_perf_view` | `.bss` | 56 | HelloPerformance scene object |
| `canvas_data` | `.bss` | 44 | Canvas runtime state |
| `ui_timer` | `.bss` | 20 | App timer state |
| `rle_state` | `.bss` | 16 | Remaining RLE decoder state |
| `port_display_driver` | `.data` | 16 | QEMU display driver state |
| `input_motion_pool_data_storage` | `.bss` | 16 | Input motion pool |
| `input_motion_pool` | `.bss` | 16 | Input motion pool metadata |
| `g_font_std_code_lookup_cache` | `.data` | 12 | Font lookup cache |
| `egui_image_decode_cache_state` | `.data` | 12 | Decode cache metadata |

### Heap Measurement

| Build | idle current | idle peak | interaction delta current | interaction delta peak | interaction total current | interaction total peak | action count |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `QEMU_HEAP_MEASURE=1` | 0 | 0 | 0 | 0 | 0 | 0 | 222 |

- Heap log contains `HEAP_EXIT`.
- This round keeps the existing `heap = 0` requirement unchanged.

### RLE Checkpoint A/B

Same code, only toggle `EGUI_CONFIG_IMAGE_RLE_CHECKPOINT_ENABLE`.

| Test | checkpoint off (ms) | checkpoint on (ms) | Delta |
| --- | ---: | ---: | ---: |
| `MASK_IMAGE_RLE_NO_MASK` | 1.538 | 1.539 | +0.1% |
| `EXTERN_MASK_IMAGE_RLE_ROUND_RECT` | 5.712 | 5.713 | +0.0% |
| `IMAGE_TILED_RLE_565_0` | 0.940 | 0.939 | -0.1% |

- The measured impact stays in noise range (`<= 0.1%`), so keeping the `16B` RAM saving is justified.
