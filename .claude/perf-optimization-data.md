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

## Before vs After (Original Baseline → Final)

Original RECTANGLE_FILL = 9.665ms, Final RECTANGLE_FILL = 3.923ms

| Test | Before (ms) | After (ms) | Improvement |
|------|------------|-----------|-------------|
| RECTANGLE_FILL | 9.665 | 3.923 | **-59%** |
| CIRCLE_FILL | 9.886 | 5.575 | **-44%** |
| ROUND_RECTANGLE_FILL | 9.944 | 5.612 | **-44%** |
| ROUND_RECTANGLE_CORNERS_FILL | 9.997 | 5.650 | **-43%** |
| TRIANGLE_FILL | 8.875 | 6.044 | **-32%** |
| IMAGE_565_8 | 32.536 | 13.604 | **-58%** |
| IMAGE_565_4 | 36.694 | 26.023 | **-29%** |
| IMAGE_565_2 | 36.694 | 26.023 | **-29%** |
| IMAGE_565_1 | 36.464 | 25.792 | **-29%** |
| GRADIENT_RECT | 11.712 | 5.963 | **-49%** |
| CIRCLE_FILL_HQ | 11.049 | 6.635 | **-40%** |
| MASK_RECT_FILL_ROUND_RECT | 11.930 | 5.597 | **-53%** |
| MASK_ROUND_RECT_FILL_WITH_MASK | 11.930 | 5.597 | **-53%** |
| MASK_ROUND_RECT_FILL_NO_MASK | 9.825 | 4.113 | **-58%** |
| SHADOW | 29.937 | 24.825 | **-17%** |
| SHADOW_ROUND | 31.706 | 26.789 | **-15%** |
| ELLIPSE_FILL | 23.041 | 18.582 | **-19%** |

## Current Full Results (Final, sorted by time)

| Test | Time (ms) | vs RECT_FILL |
|------|-----------|-------------|
| ANIMATION_SCALE | 0.457 | 0.12x |
| ANIMATION_ALPHA | 0.464 | 0.12x |
| ANIMATION_TRANSLATE | 0.471 | 0.12x |
| ANIMATION_SET | 0.531 | 0.14x |
| IMAGE_565_QUARTER | 0.948 | 0.24x |
| MASK_RECT_FILL_NO_MASK_QUARTER | 1.251 | 0.32x |
| TRIANGLE_FILL_QUARTER | 1.511 | 0.39x |
| CIRCLE_FILL_QUARTER | 1.728 | 0.44x |
| MASK_RECT_FILL_ROUND_RECT_QUARTER | 1.731 | 0.44x |
| ROUND_RECTANGLE_FILL_QUARTER | 1.750 | 0.45x |
| TRIANGLE | 2.393 | 0.61x |
| TEXT | 2.440 | 0.62x |
| TEXT_RECT | 2.488 | 0.63x |
| IMAGE_565 | 2.710 | 0.69x |
| RECTANGLE | 3.075 | 0.78x |
| IMAGE_565_8_QUARTER | 3.671 | 0.94x |
| **RECTANGLE_FILL** | **3.923** | **1.00x** |
| CIRCLE_FILL_DOUBLE | 4.022 | 1.03x |
| ROUND_RECTANGLE_FILL_DOUBLE | 4.059 | 1.03x |
| MASK_ROUND_RECT_FILL_NO_MASK | 4.113 | 1.05x |
| TRIANGLE_FILL_DOUBLE | 4.433 | 1.13x |
| CIRCLE_FILL | 5.575 | 1.42x |
| MASK_RECT_FILL_ROUND_RECT | 5.597 | 1.43x |
| ROUND_RECTANGLE_FILL | 5.612 | 1.43x |
| ROUND_RECTANGLE_CORNERS_FILL | 5.650 | 1.44x |
| GRADIENT_RECT | 5.963 | 1.52x |
| TRIANGLE_FILL | 6.044 | 1.54x |
| ARC | 6.224 | 1.59x |
| CIRCLE_FILL_HQ | 6.635 | 1.69x |
| GRADIENT_TRIANGLE | 6.646 | 1.69x |
| POLYGON_FILL | 8.955 | 2.28x |
| BEZIER_CUBIC | 9.216 | 2.35x |
| BEZIER_QUAD | 10.648 | 2.71x |
| IMAGE_565_8 | 13.604 | 3.47x |
| POLYGON | 13.520 | 3.45x |
| GRADIENT_ROUND_RECT | 16.573 | 4.23x |
| ARC_HQ | 16.739 | 4.27x |
| LINE | 18.371 | 4.68x |
| ELLIPSE_FILL | 18.582 | 4.74x |
| ROUND_RECTANGLE_CORNERS | 22.954 | 5.85x |
| CIRCLE | 23.210 | 5.92x |
| ROUND_RECTANGLE | 23.328 | 5.95x |
| ARC_FILL | 24.399 | 6.22x |
| SHADOW | 24.825 | 6.33x |
| IMAGE_565_1 | 25.792 | 6.58x |
| IMAGE_565_2 | 26.023 | 6.63x |
| IMAGE_565_4 | 26.023 | 6.63x |
| SHADOW_ROUND | 26.789 | 6.83x |
| CIRCLE_HQ | 32.770 | 8.35x |
| LINE_HQ | 33.276 | 8.48x |
| IMAGE_RESIZE_565 | 36.386 | 9.28x |
| ELLIPSE | 45.728 | 11.66x |
| MASK_RECT_FILL_CIRCLE | 47.737 | 12.17x |
| MASK_IMAGE_ROUND_RECT | 51.227 | 13.06x |
| IMAGE_RESIZE_565_8 | 54.357 | 13.86x |
| IMAGE_RESIZE_565_1 | 59.887 | 15.27x |
| IMAGE_RESIZE_565_2/4 | 60.117 | 15.33x |
| ARC_FILL_HQ | 69.101 | 17.62x |
| GRADIENT_CIRCLE | 83.241 | 21.22x |
| MASK_IMAGE_CIRCLE | 89.266 | 22.76x |

## Remaining Optimization Targets

These require deeper algorithmic changes:

| Test | Time (ms) | Bottleneck | Potential Approach |
|------|-----------|-----------|-------------------|
| MASK_IMAGE_CIRCLE | 89.3 | circle mask per-pixel AA | Optimize mask_point lookup |
| GRADIENT_CIRCLE | 83.2 | per-pixel radial gradient calc | Incremental distance computation |
| ARC_FILL_HQ | 69.1 | sub-pixel arc sampling | Reduce sampling density |
| IMAGE_RESIZE_565_* | 54-60 | bilinear interpolation per pixel | Row-level batch with precomputed offsets |
| MASK_RECT_FILL_CIRCLE | 47.7 | circle mask edge AA overhead | Same as MASK_IMAGE_CIRCLE |
| ELLIPSE | 45.7 | ellipse outline algorithm | Bresenham-style incremental |
| LINE_HQ | 33.3 | anti-aliased line sub-pixel | Reduce AA sampling |
| CIRCLE_HQ | 32.8 | anti-aliased circle sub-pixel | Cache circle AA lookup |
