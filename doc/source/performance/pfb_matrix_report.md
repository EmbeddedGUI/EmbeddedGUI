# PFB Matrix Report

- Commit: `249c22b0`
- Date: 2026-04-29T17:10:08.501136
- Profile: cortex-m3

![PFB Matrix Heatmap](images/pfb_matrix_report.png)

| Test Case | small (15x15) | middle (30x30) | fullwidth (240x1) | fullheight (1x240) | fullscreen (240x240) |
|-----------|----------:|----------:|----------:|----------:|----------:|
| LINE | 0.962 | 0.596 | 0.632 | 5.688 | 0.383 |
| IMAGE_565 | 0.570 | 0.312 | 0.329 | 1.809 | 0.151 |
| IMAGE_565_8 | 0.991 | 0.612 | 0.543 | 5.962 | 0.362 |
| TEXT | 3.221 | 0.893 | 3.057 | 3.207 | 0.126 |
| INTERNAL_TEXT_RLE4 | 3.363 | 0.964 | 4.272 | 3.297 | 0.195 |
| CIRCLE_FILL | 1.277 | 0.556 | 0.403 | 5.054 | 0.229 |
| LINE_HQ | 1.796 | 1.401 | 1.491 | 6.105 | 1.187 |
| GRADIENT_RECT | 1.071 | 0.547 | 0.317 | 11.048 | 0.159 |
| IMAGE_COLOR | 1.033 | 0.857 | 0.947 | 2.203 | 0.776 |
| MASK_RECT_FILL_ROUND_RECT | 0.771 | 0.405 | 0.353 | 5.722 | 0.167 |
| MASK_IMAGE_IMAGE | 0.628 | 0.330 | 0.428 | 3.301 | 0.181 |
| TEXT_ROTATE | 3.045 | 2.499 | 22.554 | 30.875 | 1.591 |
| IMAGE_QOI_565_8 | 3.014 | 2.129 | 1.154 | 1.120 | 0.883 |
| EXTERN_IMAGE_QOI_565_8 | 3.890 | 2.786 | 1.373 | 1.339 | 1.101 |
| MASK_IMAGE_QOI_8_ROUND_RECT | 3.339 | 2.340 | 1.259 | 1.343 | 0.956 |
| EXTERN_MASK_IMAGE_QOI_8_ROUND_RECT | 4.216 | 2.998 | 1.478 | 1.564 | 1.175 |
| IMAGE_RLE_565_8 | 1.948 | 1.403 | 0.862 | 0.896 | 0.651 |
| EXTERN_IMAGE_RLE_565_8 | 3.084 | 2.255 | 1.147 | 1.180 | 0.935 |
| MASK_IMAGE_RLE_8_ROUND_RECT | 2.278 | 1.618 | 0.970 | 1.122 | 0.724 |
| EXTERN_MASK_IMAGE_RLE_8_ROUND_RECT | 3.414 | 2.469 | 1.254 | 1.406 | 1.009 |
| CHART_LINE_DENSE | 3.015 | 1.058 | 4.901 | 2.482 | 0.561 |
| CHART_BAR_DENSE | 1.272 | 0.704 | 3.249 | 1.696 | 0.438 |
| CHART_SCATTER_DENSE | 1.835 | 1.020 | 2.771 | 3.720 | 0.648 |
| CHART_PIE_DENSE | 3.549 | 2.077 | 6.075 | 3.428 | 1.470 |