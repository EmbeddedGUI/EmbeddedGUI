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
| 2026-03-27 | Disable HelloPerformance touch pipeline | 2168220 | 52 | 22036 | 22088 | 0 / 0 | `static RAM -80B`, removed unused touch/input state for timer-driven recording |
| 2026-03-27 | Compact HelloPerformance QOI RGB565 index | 2168836 | 52 | 21972 | 22024 | 0 / 0 | `static RAM -64B`, `qoi_state 276B -> 212B`, text `+616B` |

## Current Breakdown

### Top Static RAM Symbols

| Symbol | Section | Size (B) | Notes |
| --- | --- | ---: | --- |
| `egui_image_decode_row_cache_pixel` | `.bss` | 7680 | Decode row-band pixel cache |
| `egui_image_decode_row_cache_alpha` | `.bss` | 3840 | Decode row-band alpha cache |
| `egui_pfb` | `.bss` | 1536 | User-configurable PFB, not an optimization target |
| `qoi_state` | `.bss` | 212 | Compact QOI decoder state (`rgb565[64] + aux[64]`) |
| `egui_core` | `.bss` | 228 | Core runtime state |
| `test_view` | `.bss` | 56 | HelloPerformance scene object |
| `anim_perf_view` | `.bss` | 52 | HelloPerformance scene object |
| `canvas_data` | `.bss` | 44 | Canvas runtime state |
| `ui_timer` | `.bss` | 20 | App timer state |
| `rle_state` | `.bss` | 16 | Remaining RLE decoder state |
| `port_display_driver` | `.data` | 16 | QEMU display driver state |
| `g_font_std_code_lookup_cache` | `.data` | 12 | Font lookup cache |
| `egui_image_decode_cache_state` | `.data` | 12 | Decode cache metadata |
| `g_selected_char_desc` | `.bss` | 12 | Text transform selection state |

- This build no longer contains `egui_input_info`, `input_motion_pool*`, or `egui_view_group_touch_state`.
- Current normal QEMU build: `text=2168836`, `data=52`, `bss=21972`, `static RAM=22024`.

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
