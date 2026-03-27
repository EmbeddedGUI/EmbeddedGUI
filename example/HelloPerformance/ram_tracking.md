# HelloPerformance Memory Tracking

## Scope

- This file tracks `HelloPerformance` static RAM, heap, and stack after each RAM-focused change.
- `PFB` is listed for completeness, but it is user-configurable and not an optimization target.
- If a change yields less than `5%` performance gain, prefer the lower-RAM option instead of keeping extra SRAM for a marginal speedup.
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
| 2026-03-27 | Shrink HelloPerformance rectangle gradient cache LUT | 2162756 | 52 | 21972 | 22024 | 0 / 0 | 4456 | `text +112B`, `color_cache 256 -> 129`, `egui_canvas_draw_rectangle_fill_gradient 792 -> 528`, `GRADIENT_RADIAL 7.080 -> 6.727 (-5.0%)`, `GRADIENT_ANGULAR 7.427 -> 6.914 (-6.9%)`, heap stays `0` |
| 2026-03-27 | Shrink HelloPerformance pfb manager metadata arrays | 2162672 | 52 | 21932 | 21984 | 0 / 0 | 4456 | `static RAM -40B`, `egui_core 228 -> 192`, perf stays in noise, `frame_0171.png` / `frame_0175.png` confirm round-rect mask scene still correct |
| 2026-03-27 | Move visible alpha8 tile scratch to heap with QEMU self-managed allocator | 2162144 | 52 | 21964 | 22016 | 0 / 4096 | 2936 | `text -528B`, `static RAM +32B`, `text_transform_draw_visible_alpha8_tile_layout 4456 -> 104`, QEMU heap uses runtime RAM gap instead of libc/newlib |
| 2026-03-27 | Move codec row-band decode cache from `.bss` to frame heap | 2163132 | 52 | 10940 | 10992 | 0 / 11520 | 2936 | `static RAM -11024B`, row-band pixel/alpha cache now follows image width + row count on heap, `HelloPerformance` runtime screenshots `223/223` unchanged, only older saved perf snapshot (`git 789827a`) outlier is `IMAGE_TILED_QOI_565_0 0.888 -> 1.270 (+43.0%)` |
| 2026-03-27 | Move single-row decode scratch from `.bss` to frame heap | 2163368 | 52 | 10468 | 10520 | 0 / 11520 | 2936 | `static RAM -472B`, single-row pixel/opaque-alpha decode scratch now follows screen/image row width on heap, `HelloPerformance` runtime screenshots `223/223` unchanged with `changed=0`, no new `>5%` perf regression |

## Current Breakdown

### Top Static RAM Symbols

| Symbol | Section | Size (B) | Notes |
| --- | --- | ---: | --- |
| `egui_pfb` | `.bss` | 1536 | User-configurable PFB, not an optimization target |
| `qoi_state` | `.bss` | 212 | Compact QOI decoder state (`rgb565[64] + aux[64]`) |
| `egui_core` | `.bss` | 192 | Core runtime state; embedded `pfb_manager` metadata now follows `EGUI_CONFIG_PFB_BUFFER_COUNT=1` |
| `test_view` | `.bss` | 56 | HelloPerformance scene object |
| `anim_perf_view` | `.bss` | 52 | HelloPerformance scene object |
| `canvas_data` | `.bss` | 44 | Canvas runtime state |
| `ui_timer` | `.bss` | 20 | App timer state |
| `rle_state` | `.bss` | 16 | Remaining RLE decoder state |
| `port_display_driver` | `.data` | 16 | QEMU display driver state |
| `g_font_std_code_lookup_cache` | `.data` | 12 | Font lookup cache |
| `egui_image_decode_cache_state` | `.data` | 12 | Decode cache metadata |
| `g_selected_char_desc` | `.bss` | 12 | Text transform selection state |
| `g_text_transform_visible_tile_cache` | `.bss` | 8 | Visible alpha8 tile heap cache handle |

- This build no longer contains `egui_input_info`, `input_motion_pool*`, or `egui_view_group_touch_state`.
- Current normal QEMU build: `text=2163368`, `data=52`, `bss=10468`, `static RAM=10520`.
- The codec row-band pixel/alpha cache no longer occupies fixed `.bss`; it is now allocated per frame from heap because its required size follows active image width, alpha format, and `PFB` row-band height.
- The single-row decode pixel/opaque-alpha scratch also no longer occupies fixed `.bss`; it is now allocated per frame from heap because `EGUI_CONFIG_IMAGE_DECODE_ROW_BUF_WIDTH` follows screen width in the default config, and the required bytes follow pixel format / alpha row width.
- There is no longer any decode row scratch parked in fixed SRAM between frames; the remaining static decode footprint is only metadata such as `qoi_state`, `rle_state`, and `egui_image_decode_cache_state`.
- The QEMU port now uses a self-managed runtime heap carved from `[_ebss, _estack - _Min_Stack_Size)`, so enabling `EGUI_CONFIG_QEMU_PLATFORM_MALLOC_ENABLE` does not pull in newlib `malloc` state.

### Heap Measurement

| Build | idle current | idle peak | interaction delta current | interaction delta peak | interaction total current | interaction total peak | action count |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `QEMU_HEAP_MEASURE=1` | 0 | 0 | 0 | 11520 | 0 | 11520 | 222 |

- Heap log contains `HEAP_EXIT`.
- Interaction alloc/free count is `164 / 164`; the measured heap peak is the transient codec row-band decode cache plus the existing rotated-text visible alpha8 tile scratch.
- This round intentionally allows `heap peak = 11520B` because the moved row-band cache follows image width, alpha format, and `PFB` row count; per the RAM rule, this size-related buffer must stay heap-backed instead of returning to fixed `.bss` or large stack locals.
- Moving the single-row decode scratch to heap does not raise the measured peak beyond `11520B`; the extra row buffer demand stays below the existing decode-heavy peak window.
- The measured heap build now reaches the former `.bss` cache footprint only while the decode-heavy scenes are active, and releases it back to `0B` at idle.

### Stack Measurement

| Function | File | Frame (B) | Relevance | Notes |
| --- | --- | ---: | --- | --- |
| `egui_canvas_draw_text_transform` | `src/core/egui_canvas_transform.c` | 2936 | HelloPerformance rotated-text runtime path | Current max frame after visible alpha8 scratch moves to heap; external row/glyph scratch bounds stay `16B / 288B` |
| `text_transform_draw_visible_alpha8_tile_layout` | `src/core/egui_canvas_transform.c` | 104 | HelloPerformance rotated-text helper | Former `4352B` stack scratch removed; visible alpha8 tile now allocates from heap according to actual transformed bounds |
| `egui_view_heart_rate_on_draw` | `src/widget/egui_view_heart_rate.c` | 1200 | Framework compiled hotspot, not in current HelloPerformance flow | Large locals: `points[240]`, `raw_vals[120]`, `smooth_vals[120]` |
| `egui_view_virtual_tree_walk_internal` | `src/widget/egui_view_virtual_tree.c` | 1112 | Framework compiled hotspot, not in current HelloPerformance flow | Large local stack frame: `frames[EGUI_VIEW_VIRTUAL_TREE_MAX_DEPTH]` |
| `line_hq_draw_polyline_segment` | `src/core/egui_canvas_line_hq.c` | 976 | HelloPerformance line/polyline scenes | Split out from `line_hq_draw_polyline_internal`; remaining frame is scalar segment state, no large local arrays |
| `egui_canvas_draw_line_hq` | `src/core/egui_canvas_line_hq.c` | 824 | HelloPerformance line scenes | Current lower-level line HQ hotspot after the polyline split |
| `egui_canvas_draw_circle_fill_gradient` | `src/core/egui_canvas_gradient.c` | 752 | HelloPerformance gradient circle scenes | Fixed LUT shrunk to `129` entries for `t={0,2,...,254,255}`; still fixed-size and not tied to font/image/screen/PFB |
| `egui_canvas_draw_line_round_cap_hq` | `src/core/egui_canvas_line_hq.c` | 744 | HelloPerformance line round-cap scenes | Below current polyline helper; unchanged in this round |
| `egui_canvas_draw_image_transform` | `src/core/egui_canvas_transform.c` | 736 | HelloPerformance image rotate scenes | Current image-transform hotspot; no large dynamic-size stack buffer |
| `egui_canvas_draw_polygon_fill_gradient` | `src/core/egui_canvas_gradient.c` | 720 | HelloPerformance gradient polygon scenes | Gradient scanline scratch is scalar-only after prior cleanups |
| `egui_shadow_draw_corner` | `src/shadow/egui_shadow.c` | 664 | HelloPerformance shadow scenes | `EGUI_CONFIG_SHADOW_DSQ_LUT_MAX 96 -> 64`; same `dsq_shift`, local LUT stack reduced by another `64B` |
| `egui_canvas_draw_triangle_fill_gradient` | `src/core/egui_canvas_gradient.c` | 544 | HelloPerformance gradient triangle scenes | Current next-tier gradient hotspot after round-rect cleanup |
| `egui_canvas_draw_ellipse_fill_gradient` | `src/core/egui_canvas_gradient.c` | 544 | Framework compiled hotspot | Scalar-only gradient fill path after previous ellipse cleanup |
| `egui_canvas_draw_rectangle_fill_gradient` | `src/core/egui_canvas_gradient.c` | 528 | HelloPerformance gradient rectangle scenes | Fixed LUT shrunk to `129` entries for `t={0,2,...,254,255}`; still not tied to font/image/screen/PFB |
| `egui_canvas_draw_round_rectangle_corners_fill_gradient` | `src/core/egui_canvas_gradient.c` | 472 | HelloPerformance gradient round-rect scenes | Removed local `color_cache[256]`; non-vertical branches now resolve colors directly |

- Current `HelloPerformance` stack risk is still concentrated in the rotated-text path used by `TEXT_ROTATE*` and `EXTERN_TEXT_ROTATE*` benchmark scenes, but the max single frame is now `2936B` instead of `4456B`.
- The visible alpha8 tile scratch now follows transformed glyph bounds and therefore lives on heap, which is the required placement for this size-related buffer.
- The decode heap path still does not introduce a new stack hotspot: `egui_image_qoi_draw_image` is `240B`, `egui_image_rle_draw_image` is `280B`, `egui_image_decode_prepare_single_row_pixel_buf` is `24B`, and the new row-scratch accessors stay at `16B / 24B`.
- The earlier rectangle-gradient round shrank the fixed `color_cache` in `egui_canvas_draw_rectangle_fill_gradient` from `256` to `129` entries; measured `792B -> 528B`, while `GRADIENT_RADIAL` improved from `7.080 ms` to `6.727 ms` and `GRADIENT_ANGULAR` improved from `7.427 ms` to `6.914 ms`. PC runtime check frames `frame_0120.png` and `frame_0121.png` show the radial/angular rectangle gradients remain visually correct.
- Current `HelloPerformance` non-text heads are `line_hq_draw_polyline_segment (976B)`, `egui_canvas_draw_line_hq (824B)`, `egui_canvas_draw_circle_fill_gradient (752B)`, `egui_canvas_draw_line_round_cap_hq (744B)`, `egui_canvas_draw_image_transform (736B)`, `egui_canvas_draw_polygon_fill_gradient (720B)`, `egui_canvas_draw_polygon_fill (712B)`, `egui_shadow_draw_corner (664B)`, `egui_canvas_draw_triangle_fill_gradient (544B)`, `egui_canvas_draw_ellipse_fill_gradient (544B)`, and `egui_canvas_draw_rectangle_fill_gradient (528B)`.
- Current compiled frames `>= 1KB` are `2936`, `1200`, and `1112`; only `2936` belongs to an active HelloPerformance runtime path.
- The latest round keeps the user-configurable `pfb` pixel buffer unchanged and moves pressure from stack to transient heap instead of fixed static RAM.

### RLE Checkpoint A/B

Same code, only toggle `EGUI_CONFIG_IMAGE_RLE_CHECKPOINT_ENABLE`.

| Test | checkpoint off (ms) | checkpoint on (ms) | Delta |
| --- | ---: | ---: | ---: |
| `MASK_IMAGE_RLE_NO_MASK` | 1.538 | 1.539 | +0.1% |
| `EXTERN_MASK_IMAGE_RLE_ROUND_RECT` | 5.712 | 5.713 | +0.0% |
| `IMAGE_TILED_RLE_565_0` | 0.940 | 0.939 | -0.1% |

- The measured impact stays in noise range (`<= 0.1%`), so keeping the `16B` RAM saving is justified.
