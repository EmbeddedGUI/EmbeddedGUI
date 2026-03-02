# Performance Report

- Commit: `520b214`
- Date: 2026-02-21T21:51:20.581528
- Profile: cortex-m3
- Regression threshold: 10%

![Performance Chart](images/perf_report.png)

| Test Case | Current (ms) | Baseline (ms) | Change (%) | Status |
|-----------|-------------|---------------|-----------|--------|
| LINE | 8.785 | 8.785 | +0.0 | OK |
| IMAGE_565 | 9.115 | 9.114 | +0.0 | OK |
| IMAGE_565_1 | 0.192 | 0.192 | +0.0 | OK |
| IMAGE_565_2 | 0.192 | 0.192 | +0.0 | OK |
| IMAGE_565_4 | 0.192 | 0.192 | +0.0 | OK |
| IMAGE_565_8 | 0.192 | 0.192 | +0.0 | OK |
| IMAGE_RESIZE_565 | 12.249 | 12.249 | +0.0 | OK |
| IMAGE_RESIZE_565_1 | 0.192 | 0.192 | +0.0 | OK |
| IMAGE_RESIZE_565_2 | 0.192 | 0.192 | +0.0 | OK |
| IMAGE_RESIZE_565_4 | 0.192 | 0.192 | +0.0 | OK |
| IMAGE_RESIZE_565_8 | 0.192 | 0.192 | +0.0 | OK |
| TEXT | 1.851 | 10.314 | -82.1 | IMPROVED |
| TEXT_RECT | 1.900 | 30.575 | -93.8 | IMPROVED |
| RECTANGLE | 2.403 | 2.403 | +0.0 | OK |
| CIRCLE | 6.193 | 6.194 | -0.0 | OK |
| ARC | 2.693 | 2.694 | -0.0 | OK |
| ROUND_RECTANGLE | 6.651 | 6.650 | +0.0 | OK |
| ROUND_RECTANGLE_CORNERS | 6.476 | 6.476 | +0.0 | OK |
| RECTANGLE_FILL | 3.339 | 3.339 | +0.0 | OK |
| CIRCLE_FILL | 2.944 | 4.482 | -34.3 | IMPROVED |
| ARC_FILL | 6.923 | 6.923 | +0.0 | OK |
| ROUND_RECTANGLE_FILL | 3.766 | 5.304 | -29.0 | IMPROVED |
| ROUND_RECTANGLE_CORNERS_FILL | 3.811 | 5.326 | -28.4 | IMPROVED |
| TRIANGLE | 1.570 | 1.570 | +0.0 | OK |
| TRIANGLE_FILL | 4.000 | 4.000 | +0.0 | OK |
| ELLIPSE | 8.652 | 89.939 | -90.4 | IMPROVED |
| ELLIPSE_FILL | 5.890 | 60.671 | -90.3 | IMPROVED |
| POLYGON | 7.534 | 7.534 | +0.0 | OK |
| POLYGON_FILL | 5.654 | 5.654 | +0.0 | OK |
| BEZIER_QUAD | 2.646 | 2.646 | +0.0 | OK |
| BEZIER_CUBIC | 2.690 | 2.690 | +0.0 | OK |
| CIRCLE_HQ | 9.229 | 9.229 | +0.0 | OK |
| CIRCLE_FILL_HQ | 4.399 | 4.399 | +0.0 | OK |
| ARC_HQ | 6.826 | 6.826 | +0.0 | OK |
| ARC_FILL_HQ | 18.184 | 18.184 | +0.0 | OK |
| LINE_HQ | 11.283 | 11.283 | +0.0 | OK |
| GRADIENT_RECT | 4.708 | 12.398 | -62.0 | IMPROVED |
| GRADIENT_ROUND_RECT | 7.269 | 155.100 | -95.3 | IMPROVED |
| GRADIENT_CIRCLE | 23.217 | 33.500 | -30.7 | IMPROVED |
| GRADIENT_TRIANGLE | 3.826 | 5.738 | -33.3 | IMPROVED |
| SHADOW | 10.753 | 10.827 | -0.7 | OK |
| SHADOW_ROUND | 12.261 | 13.348 | -8.1 | IMPROVED |