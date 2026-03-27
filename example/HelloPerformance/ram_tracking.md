# HelloPerformance Memory Tracking

## Scope

- This file tracks `HelloPerformance` static RAM, heap, and stack after each RAM-focused change.
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
- Stack numbers use the QEMU stack-usage build:
  - `make clean`
  - `make all APP=HelloPerformance PORT=qemu CPU_ARCH=cortex-m3 USER_CFLAGS="-fstack-usage"`
  - summarize `output/obj/HelloPerformance_qemu/**/*.su`
- `-fstack-usage` reports per-function stack frame size, not the full runtime call-chain peak.

## History

| Date | Change | text | data | bss | static RAM | Heap | Max stack frame | Notes |
| --- | --- | ---: | ---: | ---: | ---: | --- | ---: | --- |
| 2026-03-27 | Alpha change tables moved to rodata | 2172668 | 52 | 22132 | 22184 | 0 / 0 | n/a | Baseline before this round |
| 2026-03-27 | Disable HelloPerformance RLE checkpoint fallback state | 2172576 | 52 | 22116 | 22168 | 0 / 0 | n/a | `static RAM -16B`, `rle_checkpoint` removed |
| 2026-03-27 | Disable HelloPerformance touch pipeline | 2168220 | 52 | 22036 | 22088 | 0 / 0 | n/a | `static RAM -80B`, removed unused touch/input state for timer-driven recording |
| 2026-03-27 | Compact HelloPerformance QOI RGB565 index | 2168836 | 52 | 21972 | 22024 | 0 / 0 | n/a | `static RAM -64B`, `qoi_state 276B -> 212B`, text `+616B` |
| 2026-03-27 | Tighten HelloPerformance text transform stack bounds | 2166116 | 52 | 21972 | 22024 | 0 / 0 | 4456 | `text -2720B`, rotated-text top frames `6248/4888 -> 4456/3416`, heap stays `0` |
| 2026-03-27 | Shrink HelloPerformance external text transform stack buffers | 2166120 | 52 | 21972 | 22024 | 0 / 0 | 4456 | `text +4B`, `egui_canvas_draw_text_transform 3416 -> 2936`, external glyph helpers `856/864 -> 392/400`, heap stays `0` |
| 2026-03-27 | Shrink HelloPerformance shadow corner stack LUT | 2166120 | 52 | 21972 | 22024 | 0 / 0 | 4456 | `egui_shadow_draw_corner 1048 -> 792`, `SHADOW_ROUND 7.158 -> 6.113 (-14.6%)`, heap stays `0` |

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
- Current normal QEMU build: `text=2166120`, `data=52`, `bss=21972`, `static RAM=22024`.

### Heap Measurement

| Build | idle current | idle peak | interaction delta current | interaction delta peak | interaction total current | interaction total peak | action count |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `QEMU_HEAP_MEASURE=1` | 0 | 0 | 0 | 0 | 0 | 0 | 222 |

- Heap log contains `HEAP_EXIT`.
- This round keeps the existing `heap = 0` requirement unchanged.

### Stack Measurement

| Function | File | Frame (B) | Relevance | Notes |
| --- | --- | ---: | --- | --- |
| `text_transform_draw_visible_alpha8_tile_layout` | `src/core/egui_canvas_transform.c` | 4456 | HelloPerformance rotated-text runtime path | Dominated by `alpha8_stack_buf[4352]`; unchanged in this round |
| `egui_canvas_draw_text_transform` | `src/core/egui_canvas_transform.c` | 2936 | HelloPerformance rotated-text runtime path | External row/glyph scratch bounds now `16B / 288B`; this round cut it from `3416B` |
| `egui_view_heart_rate_on_draw` | `src/widget/egui_view_heart_rate.c` | 1200 | Framework compiled hotspot, not in current HelloPerformance flow | Large locals: `points[240]`, `raw_vals[120]`, `smooth_vals[120]` |
| `egui_view_virtual_tree_walk_internal` | `src/widget/egui_view_virtual_tree.c` | 1112 | Framework compiled hotspot, not in current HelloPerformance flow | Large local stack frame: `frames[EGUI_VIEW_VIRTUAL_TREE_MAX_DEPTH]` |
| `egui_canvas_draw_ellipse` | `src/core/egui_canvas_ellipse.c` | 1048 | HelloPerformance ellipse scenes | Current HelloPerformance non-text top stack frame |
| `line_hq_draw_polyline_internal` | `src/core/egui_canvas_line_hq.c` | 1016 | HelloPerformance line/polyline scenes | Current HelloPerformance non-text top stack frame |
| `egui_shadow_draw_corner` | `src/shadow/egui_shadow.c` | 792 | HelloPerformance shadow scenes | `EGUI_CONFIG_SHADOW_DSQ_LUT_MAX 256 -> 128`; local LUT stack reduced by `256B` |
| `rasterize_external_glyph4_to_packed_inside_common` | `src/core/egui_canvas_transform.c` | 400 | HelloPerformance external rotated-text helper | Local external glyph scratch shrank from `864B` |
| `rasterize_external_glyph4_to_alpha8_inside_common` | `src/core/egui_canvas_transform.c` | 392 | HelloPerformance external rotated-text helper | Local external glyph scratch shrank from `856B` |

- Current `HelloPerformance` stack risk is still concentrated in the rotated-text path used by `TEXT_ROTATE*` and `EXTERN_TEXT_ROTATE*` benchmark scenes.
- After the shadow LUT cut, the next `HelloPerformance` non-text stack candidates are `egui_canvas_draw_ellipse (1048B)` and `line_hq_draw_polyline_internal (1016B)`.
- This round only tightened the HelloPerformance shadow LUT bound. It did not increase static RAM, did not reintroduce heap, and did not touch `pfb`.
- Current stack-usage build top frames `>= 1KB`: `4456`, `2936`, `1200`, `1112`, `1048`, `1016`.

### RLE Checkpoint A/B

Same code, only toggle `EGUI_CONFIG_IMAGE_RLE_CHECKPOINT_ENABLE`.

| Test | checkpoint off (ms) | checkpoint on (ms) | Delta |
| --- | ---: | ---: | ---: |
| `MASK_IMAGE_RLE_NO_MASK` | 1.538 | 1.539 | +0.1% |
| `EXTERN_MASK_IMAGE_RLE_ROUND_RECT` | 5.712 | 5.713 | +0.0% |
| `IMAGE_TILED_RLE_565_0` | 0.940 | 0.939 | -0.1% |

- The measured impact stays in noise range (`<= 0.1%`), so keeping the `16B` RAM saving is justified.
