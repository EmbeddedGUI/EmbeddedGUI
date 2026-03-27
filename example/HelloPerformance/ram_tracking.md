# HelloPerformance Memory Tracking

## Scope

- This file tracks `HelloPerformance` static RAM, heap, and stack after each RAM-focused change.
- `PFB` is listed for completeness, but it is user-configurable and not an optimization target.
- Any buffer whose size follows font size, image size, screen size, or `PFB` size must use `heap`; do not replace that with macro-fixed RAM, static storage, or large stack locals just to keep `heap=0`.
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
| 2026-03-27 | Refactor HelloPerformance ellipse outline stack frame | 2162556 | 52 | 21972 | 22024 | 0 / 0 | 4456 | `text -3564B`, `egui_canvas_draw_ellipse 1048 -> 360`, helper frames `40/144`, `ELLIPSE 2.253 ms`, heap stays `0` |
| 2026-03-27 | Refactor HelloPerformance line HQ polyline stack frame | 2162556 | 52 | 21972 | 22024 | 0 / 0 | 4456 | `line_hq_draw_polyline_internal 1016 -> 152`, new `line_hq_draw_polyline_segment 976`, `LINE_HQ 3.557 ms`, heap stays `0` |
| 2026-03-27 | Reduce HelloPerformance shadow d_sq LUT cap to 96 | 2162556 | 52 | 21972 | 22024 | 0 / 0 | 4456 | `EGUI_CONFIG_SHADOW_DSQ_LUT_MAX 128 -> 96`, `egui_shadow_draw_corner 792 -> 728`, `SHADOW_ROUND 6.113 -> 5.584 (-8.7%)`, heap stays `0` |
| 2026-03-27 | Reduce HelloPerformance shadow d_sq LUT cap to 64 | 2162556 | 52 | 21972 | 22024 | 0 / 0 | 4456 | `EGUI_CONFIG_SHADOW_DSQ_LUT_MAX 96 -> 64`, `egui_shadow_draw_corner 728 -> 664`, `SHADOW_ROUND 5.584 -> 5.584`, same heap `0` |
| 2026-03-27 | Remove HelloPerformance round-rect gradient cache LUT | 2162492 | 52 | 21972 | 22024 | 0 / 0 | 4456 | `text -64B`, `egui_canvas_draw_round_rectangle_corners_fill_gradient 992 -> 472`, `GRADIENT_ROUND_RECT 3.766 -> 3.766`, `GRADIENT_ROUND_RECT_CORNERS 2.235 -> 2.235`, heap stays `0` |
| 2026-03-27 | Shrink HelloPerformance circle gradient cache LUT | 2162644 | 52 | 21972 | 22024 | 0 / 0 | 4456 | `text +152B`, `color_cache 256 -> 129`, `egui_canvas_draw_circle_fill_gradient 1000 -> 752`, `GRADIENT_CIRCLE 10.428 -> 9.802 (-6.0%)`, heap stays `0` |

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
- Current normal QEMU build: `text=2162644`, `data=52`, `bss=21972`, `static RAM=22024`.

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
| `line_hq_draw_polyline_segment` | `src/core/egui_canvas_line_hq.c` | 976 | HelloPerformance line/polyline scenes | Split out from `line_hq_draw_polyline_internal`; remaining frame is scalar segment state, no large local arrays |
| `egui_canvas_draw_line_hq` | `src/core/egui_canvas_line_hq.c` | 824 | HelloPerformance line scenes | Current lower-level line HQ hotspot after the polyline split |
| `egui_canvas_draw_rectangle_fill_gradient` | `src/core/egui_canvas_gradient.c` | 792 | HelloPerformance gradient rectangle scenes | Large local fixed LUT: `color_cache[256]`; fixed-size and not tied to font/image/screen/PFB |
| `egui_canvas_draw_circle_fill_gradient` | `src/core/egui_canvas_gradient.c` | 752 | HelloPerformance gradient circle scenes | Fixed LUT shrunk to `129` entries for `t={0,2,...,254,255}`; still fixed-size and not tied to font/image/screen/PFB |
| `egui_canvas_draw_line_round_cap_hq` | `src/core/egui_canvas_line_hq.c` | 744 | HelloPerformance line round-cap scenes | Below current polyline helper; unchanged in this round |
| `egui_canvas_draw_image_transform` | `src/core/egui_canvas_transform.c` | 736 | HelloPerformance image rotate scenes | Current image-transform hotspot; no large dynamic-size stack buffer |
| `egui_canvas_draw_polygon_fill_gradient` | `src/core/egui_canvas_gradient.c` | 720 | HelloPerformance gradient polygon scenes | Gradient scanline scratch is scalar-only after prior cleanups |
| `egui_shadow_draw_corner` | `src/shadow/egui_shadow.c` | 664 | HelloPerformance shadow scenes | `EGUI_CONFIG_SHADOW_DSQ_LUT_MAX 96 -> 64`; same `dsq_shift`, local LUT stack reduced by another `64B` |
| `egui_canvas_draw_triangle_fill_gradient` | `src/core/egui_canvas_gradient.c` | 544 | HelloPerformance gradient triangle scenes | Current next-tier gradient hotspot after round-rect cleanup |
| `egui_canvas_draw_ellipse_fill_gradient` | `src/core/egui_canvas_gradient.c` | 544 | Framework compiled hotspot | Scalar-only gradient fill path after previous ellipse cleanup |
| `egui_canvas_draw_round_rectangle_corners_fill_gradient` | `src/core/egui_canvas_gradient.c` | 472 | HelloPerformance gradient round-rect scenes | Removed local `color_cache[256]`; non-vertical branches now resolve colors directly and the frame dropped out of the top-stack set |

- Current `HelloPerformance` stack risk is still concentrated in the rotated-text path used by `TEXT_ROTATE*` and `EXTERN_TEXT_ROTATE*` benchmark scenes.
- This round shrank the fixed `color_cache` in `egui_canvas_draw_circle_fill_gradient` from `256` to `129` entries; measured `1000B -> 752B`, while `GRADIENT_CIRCLE` improved from `10.428 ms` to `9.802 ms`. PC runtime check frame `frame_0116.png` shows the radial circle remains visually correct.
- Current `HelloPerformance` non-text heads are `line_hq_draw_polyline_segment (976B)`, `egui_canvas_draw_line_hq (824B)`, `egui_canvas_draw_rectangle_fill_gradient (792B)`, `egui_canvas_draw_circle_fill_gradient (752B)`, `egui_canvas_draw_line_round_cap_hq (744B)`, `egui_canvas_draw_image_transform (736B)`, `egui_canvas_draw_polygon_fill_gradient (720B)`, and `egui_shadow_draw_corner (664B)`.
- `HelloPerformance` non-text paths now have no stack frame `>= 1KB`; the remaining `>= 1KB` frames are `4456`, `2936`, `1200`, `1112`.
- This round did not increase static RAM, did not reintroduce heap, and did not touch `pfb`.

### RLE Checkpoint A/B

Same code, only toggle `EGUI_CONFIG_IMAGE_RLE_CHECKPOINT_ENABLE`.

| Test | checkpoint off (ms) | checkpoint on (ms) | Delta |
| --- | ---: | ---: | ---: |
| `MASK_IMAGE_RLE_NO_MASK` | 1.538 | 1.539 | +0.1% |
| `EXTERN_MASK_IMAGE_RLE_ROUND_RECT` | 5.712 | 5.713 | +0.0% |
| `IMAGE_TILED_RLE_565_0` | 0.940 | 0.939 | -0.1% |

- The measured impact stays in noise range (`<= 0.1%`), so keeping the `16B` RAM saving is justified.
