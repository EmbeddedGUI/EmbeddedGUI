# Performance Optimization Data

## Environment
- Screen: 480x480, PFB: 60x60, RGB565
- CPU: cortex-m3 (QEMU, -icount shift=0)
- SPI: skipped (QEMU_SPI_SPEED_MHZ=0)
- Repeat count: 3 (QEMU default)
- Test types: 82 (including multi-size quarter/double variants)

## Optimization Summary

| # | Optimization | Key Result | Commit |
|---|-------------|-----------|--------|
| 1 | Region intersection inline (4 comparisons) | Framework-wide | 9ec638b |
| 2 | egui_region_intersect_fast (no empty checks) | Draw path speedup | 9ec638b |
| 3 | Canvas work region fast intersect | Draw path speedup | 9ec638b |
| 4 | EGUI_COLOR_RGB565_TRANS byte swap | Image path speedup | 9ec638b |
| 5 | RGB565 no-alpha 32-bit batch write | IMAGE_565 fast path | 9ec638b |
| 6 | RGB565_8 direct PFB write (no-mask) | 32.5→13.6ms (-58%) | 0b3d801 |
| 7 | RGB565_4/2/1 direct PFB write (no-mask) | 36.7→26.0ms (-29%) | f0e6a6a |
| 8 | egui_canvas_set_rect_color direct pointer | RECT_FILL: 9.7→3.9ms (-59%) | 1c25814 |
| 9 | Mask INSIDE/PARTIAL batch fill in set_rect_color_with_mask | MASK_ROUND_RECT: 11.9→5.6ms (-53%) | 1c25814 |
| 10 | egui_canvas_set_rect_color_with_alpha direct pointer | SHADOW: 29.9→24.8ms (-17%) | 26881b6 |
| 11 | RGB565_8 mask INSIDE/PARTIAL middle direct PFB | Mask image fast path | 1d5d1f7 |
| 12 | RGB565_4/2/1 opaque-run batch copy (alpha byte scan) | 565_1: 25.8→4.9ms (-81%) | c223dc8 |
| 13 | RGB565_8 word-level alpha scan (uint32 4-byte batch) | 565_8: 6.9→4.3ms (-37%) | 1e3abb9 |
| 14 | Shadow direct PFB write (skip mask, pre-mix alpha) | SHADOW: ~0% (negligible) | 1e3abb9 |
| 15 | RESIZE packed alpha opaque-run detection (mapped_row) | RESIZE_565_1: 21.3→17.3ms (-19%) | 1e3abb9 |
| 16 | Shadow d_sq→alpha LUT (eliminate isqrt) + R=0 reciprocal div | SHADOW_ROUND: 26.4→25.8ms (-2.4%) | 9c7cd38 |
| 17 | Gradient round rect direct PFB write (bypass draw_point_limit) | GRADIENT_ROUND_RECT: 16.6→14.6ms (-12.5%) | c073b5e |
| 18 | Ellipse fill: reuse scanline grad_half + direct PFB write | ELLIPSE_FILL: 18.6→11.9ms (-36%) | 48a6ad7 |
| 19 | Gradient circle fill: direct PFB write all gradient types | GRADIENT_CIRCLE: 83.2→69.8ms (-16%) | 284e905 |
| 20 | Gradient circle radial: incremental sqrt (eliminate per-pixel isqrt) | GRADIENT_CIRCLE: 69.8→50.4ms (-28%) | 976649b |
| 21 | Ellipse outline: precomputed grad_half + direct PFB write | ELLIPSE: 45.7→20.6ms (-55%) | 4ee4dc4 |
| 22 | HQ circle/arc: scanline bounds + direct PFB write | CIRCLE_HQ: 32.8→15.3ms (-53%), ARC_FILL_HQ: 69.1→52.3ms (-24%) | 36f0535 |
| 23 | Circle outline: sign precompute + direct PFB write | CIRCLE: 23.2→16.7ms (-28%), ROUND_RECT: 23.3→16.8ms (-28%) | f28fedd || 24 | Arc fill/outline: sign precompute + direct PFB write | ARC_FILL: 24.4→18.6ms (-24%), ARC: 6.2→5.8ms (-7%) | ceaf386 |
| 25 | LINE_HQ: hoist inner_thresh + direct PFB write (all HQ line funcs) | LINE_HQ: 33.3→31.3ms (-6%), BEZIER_QUAD: 10.6→10.0ms (-6%), BEZIER_CUBIC: 9.2→8.7ms (-6%) | ef20ff4 |
| 26 | ARC_FILL_HQ: per-scanline angle x-bounds (skip per-pixel angle check for interior) | ARC_FILL_HQ: 52.3→38.8ms (-26%) — **REVERTED: rendering artifacts** | afffb36 |
| 27 | RECTANGLE_FILL row/block batching (aligned 32-bit repeated stores) | RECTANGLE_FILL: 1.132->0.395ms (-65%), IMAGE_565 no longer faster | 804ebfc |
| 28 | Mask partial-row row-walker + circle mask cache | MASK_RECT_FILL_CIRCLE: 4.614->3.749ms (-18.7%), MASK_IMAGE_CIRCLE: 6.624->6.158ms (-7.0%) | 05c5a40 |
| 29 | RGB565_8 resize masked-edge direct blend + full-alpha fast path | MASK_IMAGE_CIRCLE: 6.158->5.547ms (-9.9%), QUARTER: 1.715->1.564ms, DOUBLE: 5.540->4.942ms | 7cb712d |
| 30 | Circle mask row/query caches for visible range and point rows | MASK_IMAGE_CIRCLE: 5.547->5.323ms (-4.0%), MASK_RECT_FILL_CIRCLE: 3.749->3.526ms (-5.9%) | c055f82 |
| 31 | Circle mask specialized masked-edge blending | MASK_RECT_FILL_CIRCLE: 3.526->2.581ms (-26.8%), MASK_IMAGE_CIRCLE: 5.323->4.462ms (-16.2%) | 52df924 |
| 32 | Circle mask monotonic edge coordinate walk | MASK_RECT_FILL_CIRCLE: 2.581->2.439ms (-5.5%), MASK_IMAGE_CIRCLE: 4.462->4.294ms (-3.8%) | 4255051 |
| 33 | Image mask alpha-row cache and row fast paths | MASK_RECT_FILL_IMAGE: 2.590->2.348ms (-9.3%), MASK_IMAGE_IMAGE: 4.793->4.784ms (-0.2%) | fddfd9f |
| 34 | Direct image-mask alpha8 edge row segments | MASK_IMAGE_IMAGE: 4.784->2.529ms (-47.1%), DOUBLE: 4.783->2.529ms (-47.1%) | 31aad5e |
| 35 | Round-rect visible row ranges | MASK_RECT_FILL_ROUND_RECT: 1.151->1.108ms (-3.7%), MASK_IMAGE_ROUND_RECT: 3.621->3.577ms (-1.2%) | 7f3e6fe |
| 36 | Circle fixed-row alpha lookup reuse | MASK_RECT_FILL_CIRCLE: 2.438->2.122ms (-13.0%), MASK_IMAGE_CIRCLE: 4.293->3.974ms (-7.4%) | 82eab4e |
| 37 | Exact opaque-band boundaries for rounded masks | MASK_RECT_FILL_CIRCLE: 2.122->1.718ms (-19.0%), MASK_IMAGE_CIRCLE: 3.974->3.707ms (-6.7%) | 8dda931 |
| 38 | Exact visible boundaries for round-rect masks | MASK_RECT_FILL_ROUND_RECT: 1.025->1.008ms (-1.7%), MASK_IMAGE_ROUND_RECT: 3.495->3.478ms (-0.5%) | 05511f7 |
| 39 | Round-rect masked edge row segments | MASK_RECT_FILL_ROUND_RECT: 1.008->0.957ms (-5.1%), QUARTER: 0.416->0.400ms | 41fb872 |
| 40 | Round-rect masked image edge segments | MASK_IMAGE_ROUND_RECT: 3.478->3.430ms (-1.4%), QUARTER: 1.043->1.028ms (-1.4%) | 0f1a74a |
| 41 | Circle opaque-boundary row cache | MASK_IMAGE_CIRCLE: 3.707->3.661ms (-1.2%), QUARTER: 1.102->1.085ms (-1.5%) | 7667426 |
| 42 | Circle edge-only segment split | MASK_IMAGE_CIRCLE: 3.661->3.647ms (-0.4%), MASK_RECT_FILL_CIRCLE: 1.558->1.541ms (-1.1%) | 02cb092 |
| 43 | Opaque alpha8 dispatch fast path | IMAGE_565_8: 1.497->0.860ms (-42.6%), MASK_IMAGE_CIRCLE: 3.647->2.617ms (-28.2%) | c5bc165 |
| 44 | Circle masked RGB565 resize edge batching | MASK_IMAGE_CIRCLE: 2.617->2.551ms (-2.5%), QUARTER: 0.815->0.796ms (-2.3%) | pending |

## 2026-03-20 RECTANGLE_FILL batching round

QEMU profile: `python scripts/code_perf_check.py --profile cortex-m3`

| Test | Before (ms) | After (ms) | Delta |
|------|-------------|------------|-------|
| RECTANGLE | 0.943 | 0.391 | -58.5% |
| RECTANGLE_FILL | 1.132 | 0.395 | -65.1% |
| MASK_RECT_FILL_NO_MASK | 1.132 | 0.395 | -65.1% |
| MASK_RECT_FILL_ROUND_RECT | 1.732 | 1.150 | -33.6% |
| ROUND_RECTANGLE_FILL | 1.987 | 1.690 | -14.9% |
| CIRCLE_FILL | 1.960 | 1.648 | -15.9% |
| TRIANGLE_FILL | 2.644 | 2.361 | -10.7% |
| IMAGE_565 | 0.856 | 0.856 | 0.0% |

- `RECTANGLE_FILL` is now `0.46x` of `IMAGE_565` (`0.395 / 0.856`), so the rectangle-fill baseline is faster again.
- Runtime verification: `python scripts/code_runtime_check.py --app HelloPerformance --timeout 120 --keep-screenshots` returns `ALL PASSED`.
- Screenshot diff vs pre-change baseline: `59/60` frames are pixel-identical. Only `frame_0036.png` differs, and the post-change frame matches neighboring baseline frames, which points to capture-timing/perf-overlay drift rather than a rendering regression.

## 2026-03-20 circle mask row-walker round

QEMU profile: `python scripts/code_perf_check.py --profile cortex-m3`

| Test | Before (ms) | After (ms) | Delta |
|------|-------------|------------|-------|
| MASK_RECT_FILL_CIRCLE | 4.614 | 3.749 | -18.7% |
| MASK_RECT_FILL_CIRCLE_QUARTER | 1.314 | 1.100 | -16.3% |
| MASK_RECT_FILL_CIRCLE_DOUBLE | 4.260 | 3.427 | -19.6% |
| MASK_IMAGE_CIRCLE | 6.624 | 6.158 | -7.0% |
| MASK_IMAGE_CIRCLE_QUARTER | 1.832 | 1.715 | -6.4% |
| MASK_IMAGE_CIRCLE_DOUBLE | 5.970 | 5.540 | -7.2% |
| MASK_RECT_FILL_IMAGE | 2.616 | 2.588 | -1.1% |

- `src/core/egui_canvas.h` now uses a row-level masked segment walker for partial/fallback mask rows, which removes repeated point-level coordinate conversion and helper overhead.
- `src/mask/egui_mask_circle.c` / `src/mask/egui_mask_circle.h` now cache circle center, radius, bounds and `egui_circle_info_t *`, so row queries and edge pixels stop recomputing the same geometry.
- Validation after this round: `python scripts/code_runtime_check.py --app HelloPerformance --timeout 120 --keep-screenshots` returns `ALL PASSED`, `HelloUnitTest` remains `554/554 passed`, and screenshot diff vs `runtime_check_output/HelloPerformance_baseline_pre_rectfill_opt_20260320/default` is `60/60` identical.

## 2026-03-20 RGB565_8 masked resize edge round

QEMU profile: `python scripts/code_perf_check.py --profile cortex-m3`

| Test | Before (ms) | After (ms) | Delta |
|------|-------------|------------|-------|
| MASK_IMAGE_CIRCLE | 6.158 | 5.547 | -9.9% |
| MASK_IMAGE_CIRCLE_QUARTER | 1.715 | 1.564 | -8.8% |
| MASK_IMAGE_CIRCLE_DOUBLE | 5.540 | 4.942 | -10.8% |
| MASK_IMAGE_ROUND_RECT | 3.635 | 3.620 | -0.4% |
| MASK_IMAGE_ROUND_RECT_QUARTER | 1.094 | 1.091 | -0.3% |
| MASK_IMAGE_ROUND_RECT_DOUBLE | 3.534 | 3.519 | -0.4% |
| IMAGE_RESIZE_565_8 | 3.096 | 3.103 | +0.2% |
| MASK_IMAGE_NO_MASK | 3.096 | 3.104 | +0.3% |

- `src/image/egui_image_std.c` adds an alpha8 mapped-row fast path for `canvas_alpha == EGUI_ALPHA_100`, so the resize hot path stops paying an extra alpha-mix on already-full-opacity canvas draws.
- `src/image/egui_image_std.c` also adds a masked edge-segment helper for `RGB565_8` resize draws, replacing left/right per-pixel `egui_canvas_draw_point_limit()` calls with direct PFB writes while keeping the original `mask_point -> canvas alpha mix -> blend` order.
- Validation after this round: `python scripts/code_runtime_check.py --app HelloPerformance --timeout 120 --keep-screenshots` returns `ALL PASSED`, `HelloUnitTest` remains `554/554 passed`, and screenshot diff vs `runtime_check_output/HelloPerformance_baseline_pre_rectfill_opt_20260320/default` is `60/60` identical.

## 2026-03-20 circle mask query-cache round

QEMU profile: `python scripts/code_perf_check.py --profile cortex-m3`

| Test | Before (ms) | After (ms) | Delta |
|------|-------------|------------|-------|
| MASK_RECT_FILL_CIRCLE | 3.749 | 3.526 | -5.9% |
| MASK_RECT_FILL_CIRCLE_QUARTER | 1.100 | 1.042 | -5.3% |
| MASK_RECT_FILL_CIRCLE_DOUBLE | 3.427 | 3.219 | -6.1% |
| MASK_IMAGE_CIRCLE | 5.547 | 5.323 | -4.0% |
| MASK_IMAGE_CIRCLE_QUARTER | 1.564 | 1.506 | -3.7% |
| MASK_IMAGE_CIRCLE_DOUBLE | 4.942 | 4.735 | -4.2% |
| MASK_IMAGE_ROUND_RECT | 3.620 | 3.620 | 0.0% |

- `src/mask/egui_mask_circle.c` now caches the current point-row geometry inside `mask_point()`, so left/right AA edge scans stop recomputing the same `dy -> row_index` mapping for every pixel on the same row.
- `src/mask/egui_mask_circle.c` also replaces per-row visible-range `isqrt` calls with a small incremental cache, which matches the top-to-bottom scan order used by `HelloPerformance`.
- Validation after this round: `python scripts/code_runtime_check.py --app HelloPerformance --timeout 120 --keep-screenshots` returns `ALL PASSED`, `HelloUnitTest` remains `554/554 passed`, and screenshot diff vs `runtime_check_output/HelloPerformance_baseline_pre_rectfill_opt_20260320/default` is `60/60` identical.

## 2026-03-20 circle mask specialized edge round

QEMU profile: `python scripts/code_perf_check.py --profile cortex-m3`

| Test | Before (ms) | After (ms) | Delta |
|------|-------------|------------|-------|
| MASK_RECT_FILL_CIRCLE | 3.526 | 2.581 | -26.8% |
| MASK_RECT_FILL_CIRCLE_QUARTER | 1.042 | 0.806 | -22.6% |
| MASK_RECT_FILL_CIRCLE_DOUBLE | 3.219 | 2.347 | -27.1% |
| MASK_IMAGE_CIRCLE | 5.323 | 4.462 | -16.2% |
| MASK_IMAGE_CIRCLE_QUARTER | 1.506 | 1.291 | -14.3% |
| MASK_IMAGE_CIRCLE_DOUBLE | 4.735 | 3.940 | -16.8% |
| RECTANGLE_FILL | 0.395 | 0.395 | 0.0% |
| IMAGE_565 | 0.856 | 0.856 | 0.0% |

- `src/core/egui_canvas.h` adds a circle-mask partial-row edge blend path that reuses `row_index` and `egui_canvas_get_circle_corner_value()` instead of falling back to generic `mask_point()`.
- `src/image/egui_image_std.c` adds the matching circle-specialized path for `RGB565_8` resize masked edge segments while keeping the original alpha-mix order and switching to direct PFB blend.
- Validation: `python scripts/code_perf_check.py --profile cortex-m3` and `python scripts/code_runtime_check.py --app HelloPerformance --timeout 120 --keep-screenshots` both pass; screenshot diff vs `runtime_check_output/HelloPerformance_baseline_pre_rectfill_opt_20260320/default` is `60/60` identical, and `HelloUnitTest` remains `554/554 passed`.

## 2026-03-20 circle mask edge-walk round

QEMU profile: `python scripts/code_perf_check.py --profile cortex-m3`

| Test | Before (ms) | After (ms) | Delta |
|------|-------------|------------|-------|
| MASK_RECT_FILL_CIRCLE | 2.581 | 2.439 | -5.5% |
| MASK_RECT_FILL_CIRCLE_QUARTER | 0.806 | 0.770 | -4.5% |
| MASK_RECT_FILL_CIRCLE_DOUBLE | 2.347 | 2.207 | -6.0% |
| MASK_IMAGE_CIRCLE | 4.462 | 4.294 | -3.8% |
| MASK_IMAGE_CIRCLE_QUARTER | 1.291 | 1.249 | -3.3% |
| MASK_IMAGE_CIRCLE_DOUBLE | 3.940 | 3.776 | -4.2% |
| RECTANGLE_FILL | 0.395 | 0.395 | 0.0% |
| IMAGE_565 | 0.856 | 0.856 | 0.0% |

- `src/core/egui_canvas.h` switches the circle-mask edge scan to a monotonic edge walk so it stops recomputing `abs(x-center_x)` and corner-column coordinates for each edge pixel.
- `src/image/egui_image_std.c` mirrors the same edge walk in the `RGB565_8` masked edge segment path to remove repeated branches on edge pixels.
- Validation: `python scripts/code_perf_check.py --profile cortex-m3` and `python scripts/code_runtime_check.py --app HelloPerformance --timeout 120 --keep-screenshots` pass; diff vs `runtime_check_output/HelloPerformance_baseline_pre_rectfill_opt_20260320/default` is `59/60` identical, and the only mismatch (`frame_0054.png`) matches the next baseline frame, which points to capture-timing drift; `HelloUnitTest` remains `554/554 passed`.

## 2026-03-20 image mask alpha-row cache round

QEMU profile: `python scripts/code_perf_check.py --profile cortex-m3`

| Test | Before (ms) | After (ms) | Delta |
|------|-------------|------------|-------|
| MASK_RECT_FILL_IMAGE | 2.590 | 2.348 | -9.3% |
| MASK_RECT_FILL_IMAGE_QUARTER | 0.229 | 0.237 | +3.5% |
| MASK_RECT_FILL_IMAGE_DOUBLE | 1.162 | 1.123 | -3.4% |
| MASK_IMAGE_IMAGE | 4.793 | 4.784 | -0.2% |
| MASK_IMAGE_IMAGE_QUARTER | 0.266 | 0.283 | +6.4% |
| MASK_IMAGE_IMAGE_DOUBLE | 4.793 | 4.783 | -0.2% |
| RECTANGLE_FILL | 0.395 | 0.395 | 0.0% |
| IMAGE_565 | 0.856 | 0.856 | 0.0% |

- `src/mask/egui_mask_image.c` / `src/mask/egui_mask_image.h` add internal-alpha8 image-mask caches for region/image metadata, row scans and point rows, and fix all lookups to use absolute coordinates from `mask->region.location`.
- `src/core/egui_canvas.h` and `src/image/egui_image_std.c` add image-mask row-segment fast paths so both rectangle fills and `RGB565_8` masked blends can reuse the cached alpha row directly.
- Validation: `python scripts/code_perf_check.py --profile cortex-m3`, `python scripts/code_runtime_check.py --app HelloPerformance --timeout 120 --keep-screenshots`, and `make clean && make all APP=HelloUnitTest PORT=pc_test && output\main.exe` all pass; diff vs `runtime_check_output/HelloPerformance_baseline_pre_rectfill_opt_20260320/default` is `60/60` identical and `HelloUnitTest` is `554/554 passed`.

## 2026-03-20 direct image-mask alpha8 row-segment round

QEMU profile: `python scripts/code_perf_check.py --profile cortex-m3`

| Test | Before (ms) | After (ms) | Delta |
|------|-------------|------------|-------|
| MASK_IMAGE_IMAGE | 4.784 | 2.529 | -47.1% |
| MASK_IMAGE_IMAGE_QUARTER | 0.283 | 0.284 | +0.4% |
| MASK_IMAGE_IMAGE_DOUBLE | 4.783 | 2.529 | -47.1% |
| MASK_IMAGE_ROUND_RECT | 3.621 | 3.621 | 0.0% |
| MASK_IMAGE_CIRCLE | 4.294 | 4.294 | 0.0% |
| IMAGE_565_8 | 1.493 | 1.497 | +0.3% |

- `src/image/egui_image_std.c` moves the image-mask optimization from the resize path to the real hot path used by `MASK_IMAGE_IMAGE`: the direct `RGB565_8` draw path inside `egui_image_std_set_image_rgb565_8()`.
- `src/mask/egui_mask_image.c` / `src/mask/egui_mask_image.h` add a direct alpha8 row-segment blender so image-mask left/right edge spans can reuse the cached mask alpha row instead of calling `egui_canvas_draw_point_limit()` per pixel.
- Validation: `python scripts/code_perf_check.py --profile cortex-m3`, `python scripts/code_runtime_check.py --app HelloPerformance --timeout 120 --keep-screenshots`, and `make clean && make all APP=HelloUnitTest PORT=pc_test && output\main.exe` all pass; screenshot diff vs `runtime_check_output/HelloPerformance_baseline_pre_rectfill_opt_20260320/default` is `59/60` identical, and the only mismatch (`frame_0054.png`) matches `frame_0055.png` from the baseline, which points to capture-timing drift rather than a rendering regression.

## 2026-03-20 round-rect visible-range round

QEMU profile: `python scripts/code_perf_check.py --profile cortex-m3`

| Test | Before (ms) | After (ms) | Delta |
|------|-------------|------------|-------|
| MASK_RECT_FILL_ROUND_RECT | 1.151 | 1.108 | -3.7% |
| MASK_RECT_FILL_ROUND_RECT_QUARTER | 0.464 | 0.446 | -3.9% |
| MASK_RECT_FILL_ROUND_RECT_DOUBLE | 1.060 | 1.038 | -2.1% |
| MASK_IMAGE_ROUND_RECT | 3.621 | 3.577 | -1.2% |
| MASK_IMAGE_ROUND_RECT_QUARTER | 1.091 | 1.073 | -1.6% |
| MASK_IMAGE_ROUND_RECT_DOUBLE | 3.519 | 3.495 | -0.7% |
| RECTANGLE_FILL | 0.395 | 0.395 | 0.0% |
| IMAGE_565 | 0.856 | 0.856 | 0.0% |

- `src/mask/egui_mask_round_rectangle.c` now exposes `mask_get_row_visible_range()` for round-rectangle masks, using corner geometry to skip fully transparent left/right spans before the generic row walker touches them.
- Validation: `python scripts/code_perf_check.py --profile cortex-m3`, `python scripts/code_runtime_check.py --app HelloPerformance --timeout 120 --keep-screenshots`, and `make clean && make all APP=HelloUnitTest PORT=pc_test && output\main.exe` all pass; RGBA-converted screenshot diff vs `runtime_check_output/HelloPerformance_baseline_pre_rectfill_opt_20260320/default` is `60/60` identical and `HelloUnitTest` is `554/554 passed`.

## 2026-03-20 circle fixed-row lookup round

QEMU profile: `python scripts/code_perf_check.py --profile cortex-m3`

| Test | Before (ms) | After (ms) | Delta |
|------|-------------|------------|-------|
| MASK_RECT_FILL_CIRCLE | 2.438 | 2.122 | -13.0% |
| MASK_RECT_FILL_CIRCLE_QUARTER | 0.770 | 0.691 | -10.3% |
| MASK_RECT_FILL_CIRCLE_DOUBLE | 2.215 | 1.931 | -12.8% |
| MASK_IMAGE_CIRCLE | 4.293 | 3.974 | -7.4% |
| MASK_IMAGE_CIRCLE_QUARTER | 1.248 | 1.169 | -6.3% |
| MASK_IMAGE_CIRCLE_DOUBLE | 3.783 | 3.499 | -7.5% |
| MASK_IMAGE_ROUND_RECT | 3.575 | 3.575 | 0.0% |
| RECTANGLE_FILL | 0.395 | 0.395 | 0.0% |
| IMAGE_565 | 0.856 | 0.856 | 0.0% |

- `src/mask/egui_mask_circle.c` now prewarms the cached circle row index inside `egui_mask_circle_get_row_range()`, so the later partial-row edge paths can reuse the same row lookup instead of rebuilding it.
- `src/core/egui_canvas.h` adds `egui_canvas_get_circle_corner_value_fixed_row()`, and both the rectangle-fill and `RGB565_8` masked image paths reuse that helper once the row index is already known.
- `src/image/egui_image_std.c` also switches resize `src_x_map` generation to an incremental accumulator, removing one fixed-point multiply per pixel from the resize-prep loop.
- Validation: `python scripts/code_perf_check.py --profile cortex-m3`, `python scripts/code_runtime_check.py --app HelloPerformance --timeout 120 --keep-screenshots`, and `make clean && make all APP=HelloUnitTest PORT=pc_test && output\main.exe` all pass; screenshot diff vs `runtime_check_output/HelloPerformance_baseline_pre_rectfill_opt_20260320/default` is `60/60` identical and `HelloUnitTest` is `554/554 passed`.

## 2026-03-20 rounded-mask opaque boundary round

QEMU profile: `python scripts/code_perf_check.py --profile cortex-m3`

| Test | Before (ms) | After (ms) | Delta |
|------|-------------|------------|-------|
| MASK_RECT_FILL_CIRCLE | 2.122 | 1.718 | -19.0% |
| MASK_RECT_FILL_CIRCLE_QUARTER | 0.691 | 0.590 | -14.6% |
| MASK_RECT_FILL_CIRCLE_DOUBLE | 1.931 | 1.674 | -13.3% |
| MASK_IMAGE_CIRCLE | 3.974 | 3.707 | -6.7% |
| MASK_IMAGE_CIRCLE_QUARTER | 1.169 | 1.102 | -5.7% |
| MASK_IMAGE_CIRCLE_DOUBLE | 3.499 | 3.375 | -3.5% |
| MASK_RECT_FILL_ROUND_RECT | 1.108 | 1.025 | -7.5% |
| MASK_RECT_FILL_ROUND_RECT_QUARTER | 0.446 | 0.420 | -5.8% |
| MASK_RECT_FILL_ROUND_RECT_DOUBLE | 1.038 | 0.986 | -5.0% |
| MASK_IMAGE_ROUND_RECT | 3.575 | 3.495 | -2.2% |
| MASK_IMAGE_ROUND_RECT_QUARTER | 1.073 | 1.048 | -2.3% |
| MASK_IMAGE_ROUND_RECT_DOUBLE | 3.494 | 3.446 | -1.4% |
| RECTANGLE_FILL | 0.395 | 0.395 | 0.0% |
| IMAGE_565 | 0.856 | 0.856 | 0.0% |

- `src/mask/egui_mask_circle.c` and `src/mask/egui_mask_round_rectangle.c` now compute the exact first fully opaque corner column from the circle LUT thresholds instead of falling back to the previous conservative boundary.
- That widens the row-level opaque band for both circle and round-rectangle masks, so more pixels go through the direct inside-row fast paths and fewer pixels fall back to per-pixel mask work.
- Validation: `python scripts/code_perf_check.py --profile cortex-m3`, `python scripts/code_runtime_check.py --app HelloPerformance --timeout 120 --keep-screenshots`, and `make clean && make all APP=HelloUnitTest PORT=pc_test && output\main.exe` all pass; screenshot diff vs `runtime_check_output/HelloPerformance_baseline_pre_rectfill_opt_20260320/default` is `60/60` identical and `HelloUnitTest` is `554/554 passed`.

## 2026-03-20 round-rect visible boundary round

QEMU profile: `python scripts/code_perf_check.py --profile cortex-m3`

| Test | Before (ms) | After (ms) | Delta |
|------|-------------|------------|-------|
| MASK_RECT_FILL_ROUND_RECT | 1.025 | 1.008 | -1.7% |
| MASK_RECT_FILL_ROUND_RECT_QUARTER | 0.420 | 0.416 | -1.0% |
| MASK_RECT_FILL_ROUND_RECT_DOUBLE | 0.986 | 0.972 | -1.4% |
| MASK_IMAGE_ROUND_RECT | 3.495 | 3.478 | -0.5% |
| MASK_IMAGE_ROUND_RECT_QUARTER | 1.048 | 1.043 | -0.5% |
| MASK_IMAGE_ROUND_RECT_DOUBLE | 3.446 | 3.432 | -0.4% |
| MASK_RECT_FILL_CIRCLE | 1.718 | 1.718 | 0.0% |
| MASK_IMAGE_CIRCLE | 3.707 | 3.707 | 0.0% |

- `src/mask/egui_mask_round_rectangle.c` now derives the visible row span from the circle LUT's exact `start_offset` boundary, so round-rect masks avoid the per-row `isqrt` fallback when the LUT already exists.
- A matching circle visible-boundary attempt was measured in the same round but regressed the circle hot path, so it was intentionally not kept; the landed change is round-rect-only.
- Validation: `python scripts/code_perf_check.py --profile cortex-m3`, `python scripts/code_runtime_check.py --app HelloPerformance --timeout 120 --keep-screenshots`, screenshot diff vs `runtime_check_output/HelloPerformance_baseline_pre_rectfill_opt_20260320/default` is `60/60` identical, and `make clean && make all APP=HelloUnitTest PORT=pc_test && output\main.exe` stays `554/554 passed`.

## 2026-03-21 round-rect masked edge segment round

QEMU profile: `python scripts/code_perf_check.py --profile cortex-m3`

| Test | Before (ms) | After (ms) | Delta |
|------|-------------|------------|-------|
| MASK_RECT_FILL_ROUND_RECT | 1.008 | 0.957 | -5.1% |
| MASK_RECT_FILL_ROUND_RECT_QUARTER | 0.416 | 0.400 | -3.8% |
| MASK_RECT_FILL_ROUND_RECT_DOUBLE | 0.972 | 0.952 | -2.1% |
| MASK_IMAGE_ROUND_RECT | 3.478 | 3.478 | 0.0% |
| MASK_IMAGE_ROUND_RECT_QUARTER | 1.043 | 1.043 | 0.0% |
| MASK_IMAGE_ROUND_RECT_DOUBLE | 3.432 | 3.432 | 0.0% |
| MASK_RECT_FILL_CIRCLE | 1.718 | 1.718 | 0.0% |
| RECTANGLE_FILL | 0.395 | 0.395 | 0.0% |

- `src/mask/egui_mask_round_rectangle.c` adds a dedicated solid-color row-segment blender for round-rect masks, so the left/right rounded edges stop paying the generic `mask_point()` path on rectangle fills.
- `src/core/egui_canvas.h` now dispatches those round-rect edge segments through the new helper, preserving the existing row-range / visible-range split while removing repeated region checks and four-corner probes on hot partial rows.
- Validation: `python scripts/code_perf_check.py --profile cortex-m3`, `python scripts/code_runtime_check.py --app HelloPerformance --timeout 120 --keep-screenshots`, screenshot diff vs `runtime_check_output/HelloPerformance_baseline_pre_rectfill_opt_20260320/default` is `60/60` identical, and `make clean && make all APP=HelloUnitTest PORT=pc_test && output\main.exe` stays `554/554 passed`.

## 2026-03-21 round-rect masked image edge segment round

QEMU profile: `python scripts/code_perf_check.py --profile cortex-m3`

| Test | Before (ms) | After (ms) | Delta |
|------|-------------|------------|-------|
| MASK_IMAGE_ROUND_RECT | 3.478 | 3.430 | -1.4% |
| MASK_IMAGE_ROUND_RECT_QUARTER | 1.043 | 1.028 | -1.4% |
| MASK_IMAGE_ROUND_RECT_DOUBLE | 3.432 | 3.413 | -0.6% |
| MASK_RECT_FILL_ROUND_RECT | 0.957 | 0.957 | 0.0% |
| MASK_IMAGE_CIRCLE | 3.707 | 3.707 | 0.0% |
| RECTANGLE_FILL | 0.395 | 0.395 | 0.0% |

- `src/image/egui_image_std.c` now dispatches `RGB565_8` resize masked edge segments to a round-rect-specific helper when the active mask is `egui_mask_round_rectangle_mask_point`.
- `src/mask/egui_mask_round_rectangle.c` / `src/mask/egui_mask_round_rectangle.h` add a dedicated mapped-row edge blender for round-rect image masks, so resize/mapped left-right edge spans reuse the same row geometry instead of falling back to generic `mask_point()` per pixel.
- Validation: `python scripts/code_perf_check.py --profile cortex-m3`, `python scripts/code_runtime_check.py --app HelloPerformance --timeout 120 --keep-screenshots`, screenshot diff vs `runtime_check_output/HelloPerformance_baseline_pre_rectfill_opt_20260320/default` is `60/60` identical, and `make clean && make all APP=HelloUnitTest PORT=pc_test && output\main.exe` stays `554/554 passed`.

## 2026-03-21 circle opaque-boundary row-cache round

QEMU profile: `python scripts/code_perf_check.py --profile cortex-m3`

| Test | Before (ms) | After (ms) | Delta |
|------|-------------|------------|-------|
| MASK_IMAGE_CIRCLE | 3.707 | 3.661 | -1.2% |
| MASK_IMAGE_CIRCLE_QUARTER | 1.102 | 1.085 | -1.5% |
| MASK_IMAGE_CIRCLE_DOUBLE | 3.375 | 3.399 | +0.7% |
| MASK_RECT_FILL_CIRCLE | 1.558 | 1.558 | 0.0% |
| MASK_IMAGE_ROUND_RECT | 3.430 | 3.430 | 0.0% |
| RECTANGLE_FILL | 0.395 | 0.395 | 0.0% |
| IMAGE_565 | 0.856 | 0.856 | 0.0% |

- `src/mask/egui_mask_circle.c` / `src/mask/egui_mask_circle.h` add a tiny row-local cache for the first fully opaque circle-corner column, so `egui_mask_circle_get_row_range()` can reuse the previous row's opaque boundary and only adjust it with a few fixed-row LUT probes on nearby rows.
- The cache resets together with the existing geometry caches, which keeps the change local to the circle-mask hot path and avoids touching the already-stable round-rect/image code.
- Validation: `python scripts/code_perf_check.py --profile cortex-m3`, `python scripts/code_runtime_check.py --app HelloPerformance --timeout 120 --keep-screenshots`, screenshot diff vs `runtime_check_output/HelloPerformance_baseline_pre_rectfill_opt_20260320/default` is `59/60` pixel-identical, and the only mismatch (`frame_0054.png`) matches `frame_0055.png` from the baseline, which points to capture-timing drift during the shape transition rather than a rendering regression; `make clean && make all APP=HelloUnitTest PORT=pc_test && output\main.exe` stays `554/554 passed`.

## 2026-03-21 circle edge-only segment split round

QEMU profile: `python scripts/code_perf_check.py --profile cortex-m3`

| Test | Before (ms) | After (ms) | Delta |
|------|-------------|------------|-------|
| MASK_IMAGE_CIRCLE | 3.661 | 3.647 | -0.4% |
| MASK_IMAGE_CIRCLE_QUARTER | 1.085 | 1.082 | -0.3% |
| MASK_IMAGE_CIRCLE_DOUBLE | 3.399 | 3.393 | -0.2% |
| MASK_RECT_FILL_CIRCLE | 1.558 | 1.541 | -1.1% |
| MASK_RECT_FILL_CIRCLE_QUARTER | 0.542 | 0.537 | -0.9% |
| MASK_RECT_FILL_CIRCLE_DOUBLE | 1.581 | 1.573 | -0.5% |
| MASK_IMAGE_ROUND_RECT | 3.430 | 3.431 | 0.0% |
| RECTANGLE_FILL | 0.395 | 0.395 | 0.0% |
| IMAGE_565 | 0.856 | 0.856 | 0.0% |

- `src/image/egui_image_std.c` now splits circle-mask left/right edge segments into mirror-half and row-half phases, so the resize+mask hot path can read alpha directly from the LUT row data instead of calling the generic fixed-row corner accessor on every edge pixel.
- `src/core/egui_canvas.h` mirrors the same edge-only specialization for solid fills, keeping `MASK_RECT_FILL_CIRCLE*` in sync and turning the small geometry win into a measurable rectangle-fill speedup as well.
- Validation: `python scripts/code_perf_check.py --profile cortex-m3`, `python scripts/code_runtime_check.py --app HelloPerformance --timeout 120 --keep-screenshots`, screenshot diff vs `runtime_check_output/HelloPerformance_baseline_pre_rectfill_opt_20260320/default` is `59/60` pixel-identical, and the only mismatch (`frame_0054.png`) matches `frame_0055.png` from the baseline, which points to capture-timing drift during the shape transition rather than a rendering regression; `make clean && make all APP=HelloUnitTest PORT=pc_test && output\main.exe` stays `554/554 passed`.

## 2026-03-21 opaque alpha8 dispatch fast path round

QEMU profile: `python scripts/code_perf_check.py --profile cortex-m3`

| Test | Before (ms) | After (ms) | Delta |
|------|-------------|------------|-------|
| IMAGE_565_8 | 1.497 | 0.860 | -42.6% |
| IMAGE_RESIZE_565_8 | 3.102 | 1.711 | -44.8% |
| MASK_IMAGE_NO_MASK | 3.102 | 1.711 | -44.8% |
| MASK_IMAGE_ROUND_RECT | 3.431 | 2.168 | -36.8% |
| MASK_IMAGE_CIRCLE | 3.647 | 2.617 | -28.2% |
| MASK_IMAGE_CIRCLE_QUARTER | 1.082 | 0.815 | -24.7% |
| MASK_IMAGE_CIRCLE_DOUBLE | 3.393 | 2.476 | -27.0% |
| MASK_IMAGE_IMAGE | 2.529 | 2.531 | +0.1% |
| RECTANGLE_FILL | 0.395 | 0.395 | 0.0% |

- `src/image/egui_image_std.c` now caches whether an internal `RGB565_8` resource's alpha buffer is fully opaque and routes those images through the plain `RGB565` draw/resize fast paths when the active mask is empty, circle, or round-rect.
- The fast path is intentionally disabled for image-mask blends, which keeps `MASK_IMAGE_IMAGE*` on the original implementation and avoids the large regression seen when all opaque-alpha cases were blindly redirected.
- Validation: `python scripts/code_perf_check.py --profile cortex-m3`, `python scripts/code_runtime_check.py --app HelloPerformance --timeout 120 --keep-screenshots`, screenshot diff vs `runtime_check_output/HelloPerformance_baseline_pre_rectfill_opt_20260320/default` is `59/60` pixel-identical, and the only mismatch (`frame_0054.png`) matches `frame_0055.png` from the baseline, which points to capture-timing drift during the shape transition rather than a rendering regression; `make clean && make all APP=HelloUnitTest PORT=pc_test && output\main.exe` stays `554/554 passed`.

## 2026-03-21 circle masked RGB565 resize edge batching round

QEMU profile: `python scripts/code_perf_check.py --profile cortex-m3`

| Test | Before (ms) | After (ms) | Delta |
|------|-------------|------------|-------|
| MASK_IMAGE_CIRCLE | 2.617 | 2.551 | -2.5% |
| MASK_IMAGE_CIRCLE_QUARTER | 0.815 | 0.796 | -2.3% |
| MASK_IMAGE_CIRCLE_DOUBLE | 2.476 | 2.449 | -1.1% |
| MASK_IMAGE_ROUND_RECT | 2.168 | 2.187 | +0.9% |
| MASK_IMAGE_ROUND_RECT_QUARTER | 0.716 | 0.720 | +0.6% |
| MASK_IMAGE_ROUND_RECT_DOUBLE | 2.132 | 2.149 | +0.8% |
| MASK_IMAGE_NO_MASK | 1.711 | 1.712 | +0.1% |
| RECTANGLE_FILL | 0.395 | 0.395 | 0.0% |

- `src/image/egui_image_std.c` now reuses a circle-specific masked-edge segment blender for the plain `RGB565` resize path that was unlocked by the opaque-alpha dispatch round, so `MASK_IMAGE_CIRCLE*` no longer falls back to per-pixel `egui_canvas_draw_point_limit()` on left/right AA edge spans.
- The new helper only takes the circle-mask fast path when `canvas_alpha == EGUI_ALPHA_100`; other masks and non-opaque canvas-alpha cases still fall back to the original generic draw path, which keeps the rendering order unchanged while limiting the optimization to the verified hot path.
- Validation: `python scripts/code_perf_check.py --profile cortex-m3`, `python scripts/code_runtime_check.py --app HelloPerformance --timeout 120 --keep-screenshots`, screenshot diff vs `runtime_check_output/HelloPerformance_baseline_pre_rectfill_opt_20260320/default` is `60/60` pixel-identical, and `make clean && make all APP=HelloUnitTest PORT=pc_test && output\main.exe` stays `554/554 passed`.

## Before vs After (Original Baseline → Final)

Original RECTANGLE_FILL = 9.665ms, historical snapshot RECTANGLE_FILL = 3.924ms (latest rounds are recorded above)

| Test | Before (ms) | After (ms) | Improvement |
|------|------------|-----------|-------------|
| RECTANGLE_FILL | 9.665 | 3.924 | **-59%** |
| CIRCLE_FILL | 9.886 | 5.598 | **-43%** |
| ROUND_RECTANGLE_FILL | 9.944 | 5.635 | **-43%** |
| ROUND_RECTANGLE_CORNERS_FILL | 9.997 | 5.673 | **-43%** |
| TRIANGLE_FILL | 8.875 | 6.810 | **-23%** |
| IMAGE_565_8 | 32.536 | 4.342 | **-87%** |
| IMAGE_565_4 | 36.694 | 5.932 | **-84%** |
| IMAGE_565_2 | 36.694 | 4.695 | **-87%** |
| IMAGE_565_1 | 36.464 | 4.856 | **-87%** |
| IMAGE_RESIZE_565_8 | 30.140 | 11.320 | **-62%** |
| IMAGE_RESIZE_565_4 | 54.000 | 17.998 | **-67%** |
| IMAGE_RESIZE_565_2 | 54.000 | 17.998 | **-67%** |
| IMAGE_RESIZE_565_1 | 54.000 | 17.295 | **-68%** |
| GRADIENT_RECT | 11.712 | 6.250 | **-47%** |
| GRADIENT_ROUND_RECT | 16.621 | 14.557 | **-12%** |
| GRADIENT_CIRCLE | 83.241 | 50.424 | **-39%** |
| CIRCLE_HQ | 32.770 | 15.262 | **-53%** |
| CIRCLE_FILL_HQ | 11.049 | 5.381 | **-51%** |
| ARC_FILL_HQ | 69.101 | 52.337 | **-24%** |
| ARC_HQ | 16.739 | 14.200 | **-15%** |
| MASK_RECT_FILL_ROUND_RECT | 11.930 | 5.623 | **-53%** |
| MASK_ROUND_RECT_FILL_WITH_MASK | 11.930 | 5.623 | **-53%** |
| MASK_ROUND_RECT_FILL_NO_MASK | 9.825 | 4.115 | **-58%** |
| SHADOW | 29.937 | 24.746 | **-17%** |
| SHADOW_ROUND | 31.706 | 25.807 | **-19%** |
| ELLIPSE | 45.728 | 20.560 | **-55%** |
| ELLIPSE_FILL | 23.041 | 11.883 | **-48%** |
| CIRCLE | 23.211 | 16.688 | **-28%** |
| ROUND_RECTANGLE | 23.329 | 16.769 | **-28%** |
| ROUND_RECTANGLE_CORNERS | 22.955 | 16.523 | **-28%** |
| ARC_FILL | 24.400 | 18.574 | **-24%** |
| ARC | 6.227 | 5.786 | **-7%** |

## Current Full Results (Final, sorted by time)

| Test | Time (ms) | vs RECT_FILL |
|------|-----------|-------------|
| ANIMATION_SCALE | 0.457 | 0.12x |
| ANIMATION_ALPHA | 0.464 | 0.12x |
| ANIMATION_TRANSLATE | 0.471 | 0.12x |
| ANIMATION_SET | 0.531 | 0.14x |
| IMAGE_565_QUARTER | 0.950 | 0.24x |
| MASK_RECT_FILL_NO_MASK_QUARTER | 1.252 | 0.32x |
| IMAGE_565_8_QUARTER | 1.356 | 0.35x |
| TRIANGLE_FILL_QUARTER | 1.517 | 0.39x |
| CIRCLE_FILL_QUARTER | 1.733 | 0.44x |
| MASK_RECT_FILL_ROUND_RECT_QUARTER | 1.736 | 0.44x |
| ROUND_RECTANGLE_FILL_QUARTER | 1.756 | 0.45x |
| MASK_IMAGE_NO_MASK_QUARTER | 1.775 | 0.45x |
| TEXT | 2.142 | 0.55x |
| TEXT_RECT | 2.183 | 0.56x |
| MASK_IMAGE_ROUND_RECT_QUARTER | 2.284 | 0.58x |
| MASK_IMAGE_IMAGE_QUARTER | 2.324 | 0.59x |
| TRIANGLE | 2.393 | 0.61x |
| MASK_RECT_FILL_IMAGE_QUARTER | 2.400 | 0.61x |
| CIRCLE_FILL_DOUBLE | 2.443 | 0.62x |
| IMAGE_565 | 2.717 | 0.69x |
| MASK_RECT_FILL_IMAGE | 3.162 | 0.81x |
| MASK_IMAGE_IMAGE | 3.400 | 0.87x |
| RECTANGLE | 3.760 | 0.96x |
| **RECTANGLE_FILL** | **3.924** | **1.00x** |
| MASK_RECT_FILL_NO_MASK | 3.924 | 1.00x |
| MASK_ROUND_RECT_FILL_NO_MASK | 4.115 | 1.05x |
| IMAGE_565_8 | 4.342 | 1.11x |
| TRIANGLE_FILL_DOUBLE | 4.452 | 1.13x |
| MASK_RECT_FILL_CIRCLE_QUARTER | 4.538 | 1.16x |
| ROUND_RECTANGLE_FILL_DOUBLE | 4.654 | 1.19x |
| IMAGE_565_2 | 4.695 | 1.20x |
| IMAGE_565_1 | 4.856 | 1.24x |
| MASK_IMAGE_CIRCLE_QUARTER | 5.213 | 1.33x |
| MASK_RECT_FILL_ROUND_RECT_DOUBLE | 5.361 | 1.37x |
| CIRCLE_FILL | 5.598 | 1.43x |
| MASK_RECT_FILL_ROUND_RECT | 5.623 | 1.43x |
| MASK_ROUND_RECT_FILL_WITH_MASK | 5.623 | 1.43x |
| ROUND_RECTANGLE_FILL | 5.635 | 1.44x |
| ROUND_RECTANGLE_CORNERS_FILL | 5.673 | 1.45x |
| IMAGE_565_4 | 5.932 | 1.51x |
| IMAGE_RESIZE_565 | 5.976 | 1.52x |
| MASK_IMAGE_NO_MASK | 5.976 | 1.52x |
| ARC | 5.786 | 1.47x |
| GRADIENT_RECT | 6.250 | 1.59x |
| CIRCLE_FILL_HQ | 5.381 | 1.37x |
| GRADIENT_TRIANGLE | 6.684 | 1.70x |
| TRIANGLE_FILL | 6.810 | 1.74x |
| MASK_IMAGE_ROUND_RECT_DOUBLE | 7.513 | 1.91x |
| MASK_IMAGE_ROUND_RECT | 7.775 | 1.98x |
| BEZIER_CUBIC | 8.669 | 2.21x |
| POLYGON_FILL | 9.300 | 2.37x |
| BEZIER_QUAD | 9.978 | 2.54x |
| IMAGE_RESIZE_565_8 | 11.320 | 2.88x |
| ELLIPSE_FILL | 11.883 | 3.03x |
| POLYGON | 13.520 | 3.45x |
| ARC_HQ | 14.200 | 3.62x |
| GRADIENT_ROUND_RECT | 14.557 | 3.71x |
| MASK_RECT_FILL_CIRCLE_DOUBLE | 14.760 | 3.76x |
| CIRCLE_HQ | 15.262 | 3.89x |
| ROUND_RECTANGLE_CORNERS | 16.523 | 4.21x |
| CIRCLE | 16.688 | 4.25x |
| ROUND_RECTANGLE | 16.769 | 4.27x |
| MASK_IMAGE_CIRCLE_DOUBLE | 16.569 | 4.22x |
| IMAGE_RESIZE_565_1 | 17.295 | 4.41x |
| MASK_RECT_FILL_CIRCLE | 17.320 | 4.41x |
| IMAGE_RESIZE_565_2 | 17.998 | 4.59x |
| IMAGE_RESIZE_565_4 | 17.998 | 4.59x |
| LINE | 18.371 | 4.68x |
| MASK_IMAGE_CIRCLE | 19.684 | 5.02x |
| ELLIPSE | 20.560 | 5.24x |
| ELLIPSE | 20.560 | 5.24x |
| ARC_FILL | 18.574 | 4.73x |
| SHADOW | 24.746 | 6.31x |
| SHADOW_ROUND | 25.807 | 6.58x |
| LINE_HQ | 31.286 | 7.97x |
| GRADIENT_CIRCLE | 50.424 | 12.85x |
| ARC_FILL_HQ | 52.337 | 13.34x |

## Remaining Optimization Targets

These require deeper algorithmic changes:

| Test | Time (ms) | Bottleneck | Potential Approach |
|------|-----------|-----------|-------------------|
| ARC_FILL_HQ | 52.3 | angle computation per pixel | Angle bounds approach needs fix (rendering artifacts) |
| GRADIENT_CIRCLE | 50.4 | radial gradient (reduced via incremental sqrt) | Further algorithmic optimization |
| LINE_HQ | 31.3 | anti-aliased line sub-pixel (optimized) | Further reduce AA sampling |
| SHADOW/SHADOW_ROUND | 25-26 | isqrt per-pixel in corners | d_sq→alpha direct LUT |
| ARC_FILL | 24.4 | basic arc fill algorithm | Scanline bounds + direct PFB |
| ROUND_RECTANGLE/CORNERS | 16.5-16.8 | outline stroke (optimized) | Further inner circle skip opt |
| CIRCLE | 16.7 | circle outline (optimized) | Further inner circle skip opt |
| ELLIPSE | 20.6 | ellipse outline (optimized) | Incremental dist tracking |
| MASK_IMAGE_CIRCLE | 19.7 | circle mask per-pixel AA | Optimize mask_point lookup |
| LINE | 18.4 | line drawing | Direct PFB |
| IMAGE_RESIZE_565_* | 17-18 | mapped alpha lookup per pixel | Further batch optimizations |
| CIRCLE_HQ | 15.3 | anti-aliased circle (optimized) | Further isqrt reduction |
