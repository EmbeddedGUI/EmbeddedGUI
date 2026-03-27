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
| 2026-03-27 | Localize HelloPerformance animation benchmark context | 2163408 | 52 | 10412 | 10464 | 0 / 11520 | 2936 | `static RAM -56B`, removed persistent animation view/interpolator/init flag from `.bss`; animation test stack frames are only `128/128/136/208B`, runtime screenshots stay `223/223` with `changed=0`, no new `>5%` perf regression |
| 2026-03-27 | Pack HelloPerformance QOI row index and drop decode-cache dead metadata | 2163424 | 48 | 10408 | 10456 | 0 / 11520 | 2936 | `static RAM -8B`, `qoi_state 212 -> 208`, `egui_image_decode_cache_state 12 -> 8`; heap peak stays `11520B`, runtime screenshots stay `223/223` with `changed=0`, no new `>5%` perf regression |
| 2026-03-27 | Drop HelloPerformance dead root-group touch flag and core PFB size cache | 2163452 | 48 | 10392 | 10440 | 0 / 11520 | 2936 | `static RAM -16B`, `egui_core 192 -> 180`; `touch=0` now compiles `egui_view_root_group_t 64 -> 60`, `egui_core` no longer stores `pfb_total_buffer_size`, heap peak stays `11520B`, runtime screenshots stay `223/223` with `changed=0`, no new `>5%` perf regression |
| 2026-03-27 | Trim HelloPerformance canvas dead clip/spec metadata | 2163340 | 48 | 10384 | 10432 | 0 / 11520 | 2936 | `static RAM -8B`, `text -112B`, `canvas_data 44 -> 32`; compile out canvas extra-clip pointer and spec-circle registration state, runtime screenshots stay `223/223` with `changed=0`, heap peak stays `11520B`, no new `>5%` perf regression |
| 2026-03-27 | Compile out default QEMU heap measurement counters | 2163152 | 48 | 10360 | 10408 | 0 / 11520 | 2936 | `static RAM -24B`, `text -188B`; normal HelloPerformance build no longer keeps QEMU heap current/peak accounting in `.bss`, while the `QEMU_HEAP_MEASURE=1` build still reports the same `interaction_total_peak=11520`, runtime screenshots stay `223/223` with `changed=0`, no new `>5%` perf regression |
| 2026-03-28 | Alias HelloPerformance user root to core root group | 2163104 | 48 | 10296 | 10344 | 0 / 11520 | 2936 | `static RAM -64B`, `text -48B`, `egui_core 180 -> 120`; runtime screenshots stay `223/223` with `changed=0`, heap peak stays `11520B`, stack peak stays `2936B`, QEMU perf profile rerun with no hot-path change expected |
| 2026-03-28 | Compile out HelloPerformance text-transform layout heap fallback | 2162384 | 48 | 10280 | 10328 | 0 / 11520 | 2936 | `static RAM -16B`, `text -720B`; compile out dead text-layout fallback metadata for HelloPerformance, runtime screenshots stay `223/223` with `changed=0`, heap peak stays `11520B`, stack peak stays `2936B`, no active-path perf change expected |
| 2026-03-28 | Reuse HelloPerformance codec row-cache handle for single-row scratch | 2162320 | 48 | 10272 | 10320 | 0 / 11520 | 2936 | `static RAM -8B`, `text -64B`; single-row codec pixel scratch now borrows the row-cache pixel heap handle, and current decode capacity metadata compacts to 16-bit under HelloPerformance bounds; runtime screenshots stay `223/223` with `changed=0`, heap peak stays `11520B`, stack peak stays `2936B`, no new `>5%` perf regression |
| 2026-03-28 | Localize HelloPerformance external font desc scratch | 2162420 | 48 | 10256 | 10304 | 0 / 11520 | 2936 | `static RAM -16B`, `text +100B`; removed persistent `g_selected_char_desc`, runtime screenshots stay `223/223` with `changed=0` vs `0ed8fdb`, heap peak stays `11520B`, stack peak stays `2936B`, `python scripts/code_perf_check.py --profile cortex-m3` PASSED |

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
| `rle_state` | `.bss` | 16 | Remaining RLE decoder state |
| `g_font_std_code_lookup_cache` | `.data` | 12 | Font lookup cache |
| `egui_image_decode_cache_state` | `.data` | 8 | Decode cache metadata; dropped unused `row_count` field |
| `g_text_transform_visible_tile_cache` | `.bss` | 8 | Visible alpha8 tile heap cache handle |
| `s_qemu_resource_handle` | `.data` | 4 | External resource semihosting handle |

- This build no longer contains `egui_input_info`, `input_motion_pool*`, or `egui_view_group_touch_state`.
- Current normal QEMU build: `text=2162420`, `data=48`, `bss=10256`, `static RAM=10304`.
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
- With `EGUI_CONFIG_TEXT_TRANSFORM_LAYOUT_HEAP_ENABLE=0`, HelloPerformance no longer keeps persistent text-layout fallback pointers/capacities in fixed SRAM; the rotated-text benchmarks stay on the existing bounded stack layout path, and exceeding that bound now asserts instead of retaining a dead heap fallback.

### Heap Measurement

| Build | idle current | idle peak | interaction delta current | interaction delta peak | interaction total current | interaction total peak | action count |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `QEMU_HEAP_MEASURE=1` | 0 | 0 | 0 | 11520 | 0 | 11520 | 222 |

- Heap log contains `HEAP_EXIT`.
- Interaction alloc/free count is `164 / 164`; the measured heap peak is the transient codec row-band decode cache plus the existing rotated-text visible alpha8 tile scratch.
- This round intentionally allows `heap peak = 11520B` because the moved row-band cache follows image width, alpha format, and `PFB` row count; per the RAM rule, this size-related buffer must stay heap-backed instead of returning to fixed `.bss` or large stack locals.
- Moving the single-row decode scratch to heap does not raise the measured peak beyond `11520B`; the extra row buffer demand stays below the existing decode-heavy peak window.
- Localizing the animation benchmark context does not change heap usage; the view/interpolator are small fixed-size temporaries and stay on stack.
- Packing the QOI row cursor and removing decode-cache dead metadata does not change heap usage; both changes only shrink fixed decode metadata.
- The latest core/root-group metadata cleanup also does not change heap usage; it only removes fixed-size static fields from `egui_core` / `egui_view_root_group_t`.
- The latest canvas metadata cleanup also does not change heap usage; it only removes fixed canvas-side state that HelloPerformance never uses.
- The latest QEMU heap-stats cleanup also does not change heap usage; it only removes normal-build telemetry counters from `.bss`, and the dedicated heap-measure build still reports the same `11520B` interaction peak.
- The latest user-root alias cleanup also does not change heap usage; it only drops one persistent root-group object from `egui_core`, and the dedicated heap-measure build still reports the same `11520B` interaction peak.
- The latest text-transform layout fallback cleanup also does not change heap usage; HelloPerformance never entered that fallback at runtime, so the dedicated heap-measure build still reports the same `11520B` interaction peak.
- The latest codec scratch-handle cleanup also does not change heap usage; it only reuses the existing row-cache pixel handle and shrinks fixed metadata, while the dedicated heap-measure build still reports the same `11520B` interaction peak.
- HelloPerformance no longer keeps a persistent external-font descriptor scratch in `.bss`; external glyph desc loads now reuse tiny caller-local scratch instead of the removed `g_selected_char_desc`.
- The latest external-font desc scratch cleanup also does not change heap usage; it only removes a fixed `12B` `.bss` scratch object and reuses small caller-local stack temporaries while the dedicated heap-measure build still reports the same `11520B` interaction peak.
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
- The animation benchmark context move adds only small per-function frames: `ANIMATION_TRANSLATE 128B`, `ANIMATION_ALPHA 128B`, `ANIMATION_SCALE 136B`, `ANIMATION_SET 208B`; it removes `56B` of fixed static RAM without creating a new stack hotspot.
- The latest decode-metadata cleanup does not change any measured stack frame; the current `2936B` max frame remains in the rotated-text path.
- The latest core/root-group cleanup also keeps stack flat: `egui_core_calc_pfb_buffer_size` is only a `16B` helper frame, `egui_core_clear_screen` is `40B`, and the max frame remains `2936B`.
- The latest canvas metadata cleanup also keeps stack flat; it only removes unused fields from `canvas_data`, and the current max frame remains `2936B`.
- The latest QEMU heap-stats cleanup also keeps stack flat; it only gates normal-build heap telemetry bookkeeping and does not add any new local buffers.
- The latest user-root alias cleanup also keeps stack flat; it only removes a persistent wrapper object from `egui_core`, while `egui_core_calc_pfb_buffer_size` stays `16B`, `egui_core_clear_screen` stays `40B`, and the max frame remains `2936B`.
- The latest text-transform layout fallback cleanup also keeps stack flat; the rotated-text path still peaks at `2936B`, and the removed fallback only strips dead persistent metadata from the normal build.
- The latest codec scratch-handle cleanup also keeps stack flat; it only reuses existing heap-backed decode storage and shrinks fixed metadata, so the max frame remains `2936B`.
- The latest external-font desc scratch cleanup also keeps stack flat; moving the external font descriptor scratch out of `.bss` only raises small caller frames to `24B` (`egui_font_std_prepare_draw_prefix_cache`), `88B` (`text_transform_build_layout` / `egui_font_std_get_str_size`), `144B` (`egui_font_std_draw_string`), `152B` (`egui_font_std_draw_string_fast_4_mask`), and `224B` (`egui_font_std_draw_string_fast_4`), while the max frame remains `2936B`.
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
