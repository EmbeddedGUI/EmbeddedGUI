# PFB Matrix Report

- Commit: `a98fa1b3`
- Date: 2026-04-25T09:41:29.795449
- Profile: cortex-m3

![PFB Matrix Heatmap](images/pfb_matrix_report.png)

| Test Case | small (15x15) | middle (30x30) | fullwidth (240x1) | fullheight (1x240) | fullscreen (240x240) |
|-----------|----------:|----------:|----------:|----------:|----------:|
| LINE | 0.961 | 0.595 | 0.632 | 5.688 | 0.382 |
| IMAGE_565 | 0.571 | 0.312 | 0.329 | 1.809 | 0.150 |
| IMAGE_565_8 | 0.989 | 0.611 | 0.542 | 5.960 | 0.362 |
| TEXT | 3.247 | 0.902 | 3.105 | 3.374 | 0.126 |
| INTERNAL_TEXT_RLE4 | 3.383 | 0.970 | 4.308 | 3.435 | 0.194 |
| CIRCLE_FILL | 1.276 | 0.556 | 0.402 | 5.054 | 0.229 |
| LINE_HQ | 1.796 | 1.401 | 1.490 | 6.104 | 1.186 |
| GRADIENT_RECT | 1.071 | 0.546 | 0.317 | 11.048 | 0.158 |
| IMAGE_COLOR | 1.033 | 0.857 | 0.946 | 2.202 | 0.776 |
| MASK_RECT_FILL_ROUND_RECT | 0.770 | 0.405 | 0.353 | 5.721 | 0.166 |
| MASK_IMAGE_IMAGE | 0.627 | 0.330 | 0.426 | 3.301 | 0.181 |
| TEXT_ROTATE | 3.045 | 2.499 | 22.554 | 30.876 | 1.592 |
| IMAGE_QOI_565_8 | 3.013 | 2.128 | 1.153 | 1.119 | 0.883 |
| EXTERN_IMAGE_QOI_565_8 | 3.889 | 2.785 | 1.373 | 1.339 | 1.101 |
| MASK_IMAGE_QOI_8_ROUND_RECT | 3.338 | 2.340 | 1.258 | 1.343 | 0.956 |
| EXTERN_MASK_IMAGE_QOI_8_ROUND_RECT | 4.216 | 2.998 | 1.478 | 1.563 | 1.175 |
| IMAGE_RLE_565_8 | 1.937 | 1.395 | 0.859 | 0.891 | 0.648 |
| EXTERN_IMAGE_RLE_565_8 | 3.353 | 2.456 | 1.213 | 1.245 | 1.002 |
| MASK_IMAGE_RLE_8_ROUND_RECT | 2.268 | 1.609 | 0.967 | 1.118 | 0.721 |
| EXTERN_MASK_IMAGE_RLE_8_ROUND_RECT | 3.682 | 2.670 | 1.320 | 1.471 | 1.075 |
| CHART_LINE_DENSE | 2.989 | 1.046 | 4.829 | 2.453 | 0.556 |
| CHART_BAR_DENSE | 1.248 | 0.692 | 3.182 | 1.572 | 0.432 |
| CHART_SCATTER_DENSE | 1.809 | 1.005 | 2.699 | 3.589 | 0.642 |
| CHART_PIE_DENSE | 3.547 | 2.076 | 6.074 | 3.428 | 1.469 |