# HelloPerformance Memory Tracking

## Scope

- This file tracks `HelloPerformance` static RAM, heap, and stack after each RAM-focused change.
- `PFB` is listed for completeness, but it is user-configurable and not an optimization target.
- If a change yields less than `5%` performance gain, prefer the lower-RAM option instead of keeping extra SRAM for a marginal speedup.
- Any buffer whose size follows font size, image size, screen size, or `PFB` size must use `heap`; do not replace that with macro-fixed RAM, static storage, or large stack locals just to keep `heap=0`.
- If such a size-related buffer must stay resident on heap, record the owner, lifetime, and byte size in this file instead of leaving it implicit.
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
| 2026-03-27 | Localize HelloPerformance animation benchmark context | 2163408 | 52 | 10412 | 10464 | 0 / 11520 | 2936 | `static RAM -56B`, removed persistent animation view/interpolator/init flag from `.bss`; animation test stack frames are only `128/128/136/208B`, runtime screenshots stay `223/223` with `changed=0`, no new `>5%` perf regression |
| 2026-03-27 | Pack HelloPerformance QOI row index and drop decode-cache dead metadata | 2163424 | 48 | 10408 | 10456 | 0 / 11520 | 2936 | `static RAM -8B`, `qoi_state 212 -> 208`, `egui_image_decode_cache_state 12 -> 8`; heap peak stays `11520B`, runtime screenshots stay `223/223` with `changed=0`, no new `>5%` perf regression |
| 2026-03-27 | Drop HelloPerformance dead root-group touch flag and core PFB size cache | 2163452 | 48 | 10392 | 10440 | 0 / 11520 | 2936 | `static RAM -16B`, `egui_core 192 -> 180`; `touch=0` now compiles `egui_view_root_group_t 64 -> 60`, `egui_core` no longer stores `pfb_total_buffer_size`, heap peak stays `11520B`, runtime screenshots stay `223/223` with `changed=0`, no new `>5%` perf regression |
| 2026-03-27 | Trim HelloPerformance canvas dead clip/spec metadata | 2163340 | 48 | 10384 | 10432 | 0 / 11520 | 2936 | `static RAM -8B`, `text -112B`, `canvas_data 44 -> 32`; compile out canvas extra-clip pointer and spec-circle registration state, runtime screenshots stay `223/223` with `changed=0`, heap peak stays `11520B`, no new `>5%` perf regression |
| 2026-03-27 | Compile out default QEMU heap measurement counters | 2163152 | 48 | 10360 | 10408 | 0 / 11520 | 2936 | `static RAM -24B`, `text -188B`; normal HelloPerformance build no longer keeps QEMU heap current/peak accounting in `.bss`, while the `QEMU_HEAP_MEASURE=1` build still reports the same `interaction_total_peak=11520`, runtime screenshots stay `223/223` with `changed=0`, no new `>5%` perf regression |
| 2026-03-28 | Alias HelloPerformance user root to core root group | 2163104 | 48 | 10296 | 10344 | 0 / 11520 | 2936 | `static RAM -64B`, `text -48B`, `egui_core 180 -> 120`; runtime screenshots stay `223/223` with `changed=0`, heap peak stays `11520B`, stack peak stays `2936B`, QEMU perf profile rerun with no hot-path change expected |
| 2026-03-28 | Compile out HelloPerformance text-transform layout heap fallback | 2162384 | 48 | 10280 | 10328 | 0 / 11520 | 2936 | `static RAM -16B`, `text -720B`; compile out dead text-layout fallback metadata for HelloPerformance, runtime screenshots stay `223/223` with `changed=0`, heap peak stays `11520B`, stack peak stays `2936B`, no active-path perf change expected |
| 2026-03-28 | Reuse HelloPerformance codec row-cache handle for single-row scratch | 2162320 | 48 | 10272 | 10320 | 0 / 11520 | 2936 | `static RAM -8B`, `text -64B`; single-row codec pixel scratch now borrows the row-cache pixel heap handle, and current decode capacity metadata compacts to 16-bit under HelloPerformance bounds; runtime screenshots stay `223/223` with `changed=0`, heap peak stays `11520B`, stack peak stays `2936B`, no new `>5%` perf regression |
| 2026-03-28 | Localize HelloPerformance external font desc scratch | 2162420 | 48 | 10256 | 10304 | 0 / 11520 | 2936 | `static RAM -16B`, `text +100B`; removed persistent `g_selected_char_desc`, runtime screenshots stay `223/223` with `changed=0` vs `0ed8fdb`, heap peak stays `11520B`, stack peak stays `2936B`, `python scripts/code_perf_check.py --profile cortex-m3` PASSED |
| 2026-03-28 | Move HelloPerformance external font row/glyph scratch to transient heap | 2162436 | 48 | 10256 | 10304 | 0 / 11520 | 2920 | `text +16B`, static RAM unchanged; external text transform row cache, visible-tile glyph bitmap scratch, and direct external glyph pixel scratch now follow actual glyph size on heap and free after use, runtime screenshots stay `223/223` with `changed=0` vs `e8d5ef8`, heap peak stays `11520B` with alloc/free `716 / 716`, stack peak `2936 -> 2920`, `python scripts/code_perf_check.py --profile cortex-m3` PASSED |
| 2026-03-28 | Move HelloPerformance image resize and round-rect scratch to transient heap | 2160520 | 48 | 10256 | 10304 | 0 / 11616 | 2920 | `text -1916B`, static RAM unchanged; resize `src_x_map` and round-rect fast row cache now follow actual draw width / tile height on heap and free after use, runtime screenshots stay `223/223` with `changed=0` vs `e8d5ef8`, heap peak `11520 -> 11616`, alloc/free `716 / 716 -> 5666 / 5666`, `python scripts/code_perf_check.py --profile cortex-m3 --threshold 5` PASSED |
| 2026-03-28 | Move HelloPerformance circle mask row cache to frame heap | 2159800 | 48 | 10272 | 10320 | 0 / 11616 | 2920 | `text -720B`, `static RAM +16B`; circle-mask `PFB_HEIGHT` row cache moves from `egui_mask_circle_t` to frame heap scratch (`3 * EGUI_CONFIG_PFB_HEIGHT * sizeof(egui_dim_t) = 96B` at current config) and releases at frame end, runtime screenshots stay `223/223` with `changed=0` vs `04cc338`, heap peak stays `11616B` with alloc/free `5683 / 5683`, `python scripts/code_perf_check.py --profile cortex-m3 --threshold 5` PASSED |
| 2026-03-28 | Move HelloPerformance rotated-text layout/tile scratch to transient heap | 2160560 | 48 | 10272 | 10320 | 0 / 11616 | 1200 | `text +760B`, static RAM unchanged; rotated-text layout/tile collectors now allocate by actual glyph/line counts and free at `cleanup`, `egui_canvas_draw_text_transform 2920 -> 760`, heap peak stays `11616B` with alloc/free `5683 / 5683 -> 7403 / 7403`, runtime screenshots stay `223/223` with `changed=0` vs `04cc338`, `python scripts/code_perf_check.py --profile cortex-m3 --threshold 5` PASSED |

## Current Breakdown

### Top Static RAM Symbols

| Symbol | Section | Size (B) | Notes |
| --- | --- | ---: | --- |
| `egui_pfb` | `.bss` | 1536 | User-configurable PFB, not an optimization target |
| `qoi_state` | `.bss` | 208 | Compact QOI decoder state (`rgb565[64] + aux[64]`), with 8-bit current-row cursor for HelloPerformance's 40/120/240px assets |
| `egui_core` | `.bss` | 120 | Core runtime state; removed cached `pfb_total_buffer_size`, `touch=0` strips dead root-group touch flags, and HelloPerformance aliases the user root to the main root group |
| `test_view` | `.bss` | 56 | HelloPerformance scene object |
| `canvas_data` | `.bss` | 32 | Canvas runtime state; HelloPerformance compiles out extra clip/spec-circle metadata |
| `ui_timer` | `.bss` | 20 | App timer state |
| `port_display_driver` | `.data` | 16 | QEMU display driver state |
| `g_egui_mask_circle_frame_row_cache` | `.bss` | 16 | Circle-mask frame-cache metadata only; the `PFB_HEIGHT` row scratch itself is heap-backed and released at frame end |
| `rle_state` | `.bss` | 16 | Remaining RLE decoder state |
| `g_font_std_code_lookup_cache` | `.data` | 12 | Font lookup cache |
| `egui_image_decode_cache_state` | `.data` | 8 | Decode cache metadata; dropped unused `row_count` field |
| `g_text_transform_visible_tile_cache` | `.bss` | 8 | Visible alpha8 tile heap cache handle |
| `s_qemu_resource_handle` | `.data` | 4 | External resource semihosting handle |

- This build no longer contains `egui_input_info`, `input_motion_pool*`, or `egui_view_group_touch_state`.
- Current normal QEMU build: `text=2160560`, `data=48`, `bss=10272`, `static RAM=10320`.
- Section split from `llvm-size -A`: `.bss=2080`, `._user_heap_stack=8192`.
- The only new fixed RAM in this round is `g_egui_mask_circle_frame_row_cache (16B)` metadata; the former `3 * PFB_HEIGHT * sizeof(egui_dim_t)` circle row arrays no longer live inside `egui_mask_circle_t`.
- The codec row-band pixel/alpha cache no longer occupies fixed `.bss`; it is now allocated per frame from heap because its required size follows active image width, alpha format, and `PFB` row-band height.
- The single-row decode pixel/opaque-alpha scratch also no longer occupies fixed `.bss`; it is now allocated per frame from heap because `EGUI_CONFIG_IMAGE_DECODE_ROW_BUF_WIDTH` follows screen width in the default config, and the required bytes follow pixel format / alpha row width.
- There is no longer any decode row scratch parked in fixed SRAM between frames; the remaining static decode footprint is only metadata such as `qoi_state`, `rle_state`, and `egui_image_decode_cache_state`.
- The animation benchmark helper no longer parks `anim_perf_view`, `anim_perf_interp`, or an init flag in fixed SRAM; each animation scene now uses a tiny local context and releases it with the draw call.
- The decode row-band metadata no longer stores an unused `row_count`; the remaining `8B` only tracks `image_info`, `row_band_start`, and cache mode.
- The codec single-row pixel scratch no longer keeps a second persistent heap handle/capacity pair when row-band cache is enabled; it now borrows the same row-cache pixel buffer and invalidates any previous cache tag before reuse.
- Under the current HelloPerformance decode bounds (`single-row <= 480B`, `row-cache pixel <= 7680B`, `row-cache alpha <= 3840B`), the remaining decode capacity metadata also compacts from 32-bit to 16-bit counters.
- HelloPerformance now opts into an 8-bit QOI current-row cursor because its generated QOI resources are only `40/120/240` pixels tall; `egui_image_qoi_prepare_decode_info()` asserts that bound before decoding.
- `egui_core` no longer stores a cached `pfb_total_buffer_size`; the few resize/clear paths derive bytes or pixel count from `pfb_width * pfb_height` on demand, which keeps the perf check within noise while removing fixed core RAM.
- With `EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH=0`, `egui_view_root_group_t` no longer carries the `is_disallow_process_touch_event` field, so the root-group object shrinks from `64B` to `60B`.
- With `EGUI_CONFIG_CORE_SEPARATE_USER_ROOT_GROUP_ENABLE=0`, HelloPerformance no longer keeps a second `user_root_view_group`; `egui_core_get_user_root_view()` and the add/remove/layout helpers now alias directly to `root_view_group`, which cuts another `60B` from `egui_core` without affecting the single-root benchmark app flow.
- With `EGUI_CONFIG_CANVAS_EXTRA_CLIP_ENABLE=0` and `EGUI_CONFIG_CANVAS_SPEC_CIRCLE_INFO_ENABLE=0`, `canvas_data` no longer carries the scroll/list extra-clip pointer or out-of-range circle registration metadata; HelloPerformance stays within built-in circle LUT range and does not use nested clip widgets.
- The QEMU port now uses a self-managed runtime heap carved from `[_ebss, _estack - _Min_Stack_Size)`, so enabling `EGUI_CONFIG_QEMU_PLATFORM_MALLOC_ENABLE` does not pull in newlib `malloc` state.
- The default QEMU build no longer keeps heap measurement counters in fixed SRAM; only the allocator topology (`free_list` / init flag) remains resident, while `QEMU_HEAP_MEASURE=1` recompiles the counters back in for telemetry.
- With `EGUI_CONFIG_TEXT_TRANSFORM_LAYOUT_HEAP_ENABLE=0`, HelloPerformance no longer keeps persistent text-layout fallback pointers/capacities in fixed SRAM; with `EGUI_CONFIG_TEXT_TRANSFORM_SCRATCH_HEAP_ENABLE=1`, the active rotated-text layout/tile scratch now follows actual glyph and line counts on transient heap instead of bounded hot-path stack arrays.
- HelloPerformance no longer keeps macro-sized external font row/glyph/pixel scratch on stack; external text transform now allocates a two-row scanline cache and visible-glyph bitmap scratch by actual glyph dimensions on heap, and direct external font drawing reuses a per-call heap scratch sized by the loaded glyph bitmap.
- HelloPerformance no longer keeps `PFB_WIDTH`-sized resize maps or `PFB_HEIGHT`-sized round-rect row caches on stack; image resize and round-rect fast paths now allocate those buffers from heap by current draw width / tile height and release them immediately after use.
- HelloPerformance no longer keeps `PFB_HEIGHT`-sized circle-mask row caches inside mask objects; the active circle mask now borrows a frame-local heap scratch shared by the current `egui_mask_circle_t`, because the required bytes follow tile height.
- There is still no size-related heap that stays resident at idle for rotated text, external text, image resize, or circle mask; the new text/font/image/circle scratch buffers are allocated only inside the active draw or frame walk and return to `0B` current heap after the recording finishes.

### Heap Measurement

| Build | idle current | idle peak | interaction delta current | interaction delta peak | interaction total current | interaction total peak | action count |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `QEMU_HEAP_MEASURE=1` | 0 | 0 | 0 | 11616 | 0 | 11616 | 222 |

- Heap log contains `HEAP_EXIT`.
- Interaction alloc/free count is `7403 / 7403`; the measured heap peak is still the transient codec row-band decode cache plus rotated-text / image-resize / circle-mask scratch, while all size-related buffers still release back to `0B` current heap after the recording.
- This round intentionally keeps `heap peak = 11616B`: rotated-text layout/tile scratch now follows actual glyph bounds and line count, image resize / circle-mask scratch follows image or `PFB` dimensions, and the decode row-band cache still dominates the peak window. Per the RAM rule, these buffers must stay heap-backed instead of returning to fixed `.bss` or large stack locals.
- The new rotated-text scratch is owned by `egui_canvas_draw_text_transform()`, allocated as two transient blocks after measuring the current string layout, and released at `cleanup`; it does not stay resident at idle.
- The new rotated-text scratch increases alloc/free activity (`5683 / 5683 -> 7403 / 7403`) but keeps the active text scenes within the `5%` perf threshold, so the RAM tradeoff is acceptable.
- The new image-size / `PFB`-size scratch does not stay resident after rendering; resize `src_x_map` and round-rect fast row cache are allocated per draw call and released immediately after use. If a later change keeps them persistent to reduce alloc/free count, it must be listed here with explicit bytes and lifetime.
- The circle-mask frame scratch is owned by the active `egui_mask_circle_t`, sized `3 * EGUI_CONFIG_PFB_HEIGHT * sizeof(egui_dim_t) = 96B` at the current `240x240 / PFB_HEIGHT=16` config, allocated on first row use within the frame, and released by `egui_mask_circle_release_frame_cache()` at frame end; it does not stay resident at idle.
- The measured heap build reaches the former `.bss` cache footprint only while decode-heavy or rotated-text scenes are active, and releases it back to `0B` at idle.

### Stack Measurement

| Function | File | Frame (B) | Relevance | Notes |
| --- | --- | ---: | --- | --- |
| `egui_view_heart_rate_on_draw` | `src/widget/egui_view_heart_rate.c` | 1200 | Framework compiled hotspot, not in current HelloPerformance flow | Large locals: `points[240]`, `raw_vals[120]`, `smooth_vals[120]` |
| `egui_view_virtual_tree_walk_internal` | `src/widget/egui_view_virtual_tree.c` | 1112 | Framework compiled hotspot, not in current HelloPerformance flow | Large local stack frame: `frames[EGUI_VIEW_VIRTUAL_TREE_MAX_DEPTH]` |
| `line_hq_draw_polyline_segment` | `src/core/egui_canvas_line_hq.c` | 976 | HelloPerformance line/polyline scenes | Current active HelloPerformance max frame after rotated-text scratch moves off stack |
| `egui_canvas_draw_line_hq` | `src/core/egui_canvas_line_hq.c` | 824 | HelloPerformance line scenes | Current lower-level line HQ hotspot after the polyline split |
| `egui_canvas_draw_text_transform` | `src/core/egui_canvas_transform.c` | 760 | HelloPerformance rotated-text runtime path | Layout/tile collectors now allocate by actual glyph and line count on transient heap, trimming this frame from `2920B` |
| `egui_canvas_draw_circle_fill_gradient` | `src/core/egui_canvas_gradient.c` | 752 | HelloPerformance gradient circle scenes | Fixed LUT shrunk to `129` entries for `t={0,2,...,254,255}`; still fixed-size and not tied to font/image/screen/PFB |
| `egui_canvas_draw_line_round_cap_hq` | `src/core/egui_canvas_line_hq.c` | 744 | HelloPerformance line round-cap scenes | Below current polyline helper; unchanged in this round |
| `egui_canvas_draw_image_transform` | `src/core/egui_canvas_transform.c` | 736 | HelloPerformance image rotate scenes | Current image-transform hotspot; no large dynamic-size stack buffer |
| `egui_canvas_draw_polygon_fill_gradient` | `src/core/egui_canvas_gradient.c` | 720 | HelloPerformance gradient polygon scenes | Gradient scanline scratch is scalar-only after prior cleanups |
| `egui_shadow_draw_corner` | `src/shadow/egui_shadow.c` | 664 | HelloPerformance shadow scenes | `EGUI_CONFIG_SHADOW_DSQ_LUT_MAX 96 -> 64`; same `dsq_shift`, local LUT stack reduced by another `64B` |
| `egui_canvas_draw_triangle_fill_gradient` | `src/core/egui_canvas_gradient.c` | 544 | HelloPerformance gradient triangle scenes | Current next-tier gradient hotspot after round-rect cleanup |
| `egui_canvas_draw_ellipse_fill_gradient` | `src/core/egui_canvas_gradient.c` | 544 | Framework compiled hotspot | Scalar-only gradient fill path after previous ellipse cleanup |
| `egui_canvas_draw_rectangle_fill_gradient` | `src/core/egui_canvas_gradient.c` | 528 | HelloPerformance gradient rectangle scenes | Fixed LUT shrunk to `129` entries for `t={0,2,...,254,255}`; still not tied to font/image/screen/PFB |
| `egui_canvas_draw_round_rectangle_corners_fill_gradient` | `src/core/egui_canvas_gradient.c` | 472 | HelloPerformance gradient round-rect scenes | Removed local `color_cache[256]`; non-vertical branches now resolve colors directly |
| `egui_image_std_set_image_resize_rgb565_8_common` | `src/image/egui_image_std.c` | 384 | HelloPerformance image resize + alpha path | Former `src_x_map[EGUI_CONFIG_PFB_WIDTH]` now uses heap; active frame is scalar-only plus small state |
| `egui_image_std_set_image_resize_rgb565` | `src/image/egui_image_std.c` | 328 | HelloPerformance image resize rgb565 path | Resize `src_x_map` and round-rect row cache now allocate from heap by draw width / tile height |
| `text_transform_draw_visible_alpha8_tile_layout` | `src/core/egui_canvas_transform.c` | 128 | HelloPerformance rotated-text helper | Former `4352B` stack scratch removed; visible alpha8 tile now allocates from heap according to actual transformed bounds |
| `text_transform_rasterize_visible_alpha8_layout` | `src/core/egui_canvas_transform.c` | 96 | HelloPerformance external rotated-text visible-tile helper | External glyph bitmap scratch moved from local arrays to transient heap sized by visible glyph bitmap bytes |
| `egui_font_std_draw_single_char_desc_external_stream` | `src/font/egui_font_std.c` | 88 | HelloPerformance external font draw path | Removed former `pixel_buf[256]`; full glyph bitmap now loads into heap scratch, reused by `fast_4` / `fast_4_mask` string draw calls |

- Current compiled frames `>= 1KB` are only `1200` and `1112`, and both belong to framework code that is not exercised by the current HelloPerformance recording path.
- Current active HelloPerformance max frame is `line_hq_draw_polyline_segment (976B)`; there is no active runtime path `>= 1KB` after this round.
- The latest rotated-text scratch cleanup trims `egui_canvas_draw_text_transform` from `2920B` to `760B` by moving layout/tile collectors to transient heap sized by the measured glyph and line count of the current string.
- The visible alpha8 tile scratch, external font scanline row cache, visible-glyph bitmap scratch, and direct-draw glyph pixel scratch all remain heap-backed, which is the required placement for these font-size-related buffers.
- The image resize / round-rect scratch cleanup still caps the modified image frames at `384B` (`egui_image_std_set_image_resize_rgb565_8_common`) and `328B` (`egui_image_std_set_image_resize_rgb565`); the former `PFB_WIDTH` / `PFB_HEIGHT` sized locals are gone.
- The circle-mask row-cache cleanup still keeps the modified benchmark entry frames at only `72B` (`egui_view_test_performance_test_mask_rect_fill_circle`) and `64B` (`egui_view_test_performance_test_mask_image_circle`, `egui_view_test_performance_test_mask_image_test_perf_circle`, `egui_view_test_performance_test_extern_mask_image_circle`, `egui_view_test_performance_test_extern_mask_image_test_perf_circle`).
- The decode heap path still does not introduce a new stack hotspot: `egui_image_qoi_draw_image` is `240B`, `egui_image_rle_draw_image` is `280B`, `egui_image_decode_prepare_single_row_pixel_buf` is `24B`, and the row-scratch accessors stay at `16B / 24B`.
- The latest round keeps the user-configurable `pfb` pixel buffer unchanged and moves pressure from stack to transient heap instead of fixed static RAM.

### RLE Checkpoint A/B

Same code, only toggle `EGUI_CONFIG_IMAGE_RLE_CHECKPOINT_ENABLE`.

| Test | checkpoint off (ms) | checkpoint on (ms) | Delta |
| --- | ---: | ---: | ---: |
| `MASK_IMAGE_RLE_NO_MASK` | 1.538 | 1.539 | +0.1% |
| `EXTERN_MASK_IMAGE_RLE_ROUND_RECT` | 5.712 | 5.713 | +0.0% |
| `IMAGE_TILED_RLE_565_0` | 0.940 | 0.939 | -0.1% |

- The measured impact stays in noise range (`<= 0.1%`), so keeping the `16B` RAM saving is justified.
