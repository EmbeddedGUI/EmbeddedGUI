# PFB Matrix Report

- Commit: `e82db1af`
- Date: 2026-04-20T14:28:18.493792
- Profile: cortex-m3

![PFB Matrix Heatmap](images/pfb_matrix_report.png)

| Test Case | small (15x15) | middle (30x30) | fullwidth (240x1) | fullheight (1x240) | fullscreen (240x240) |
|-----------|----------:|----------:|----------:|----------:|----------:|
| LINE | 0.966 | 0.596 | 0.636 | 5.692 | 0.382 |
| IMAGE_565 | 0.574 | 0.313 | 0.332 | 1.812 | 0.150 |
| IMAGE_565_8 | 0.994 | 0.612 | 0.546 | 5.965 | 0.362 |
| TEXT | 3.251 | 0.904 | 3.109 | 3.378 | 0.126 |
| INTERNAL_TEXT_RLE4 | 3.387 | 0.971 | 4.313 | 3.439 | 0.194 |
| CIRCLE_FILL | 1.280 | 0.557 | 0.406 | 5.058 | 0.229 |
| LINE_HQ | 1.800 | 1.402 | 1.494 | 6.108 | 1.186 |
| GRADIENT_RECT | 1.075 | 0.548 | 0.320 | 11.052 | 0.158 |
| IMAGE_COLOR | 1.037 | 0.858 | 0.951 | 2.206 | 0.776 |
| MASK_RECT_FILL_ROUND_RECT | 0.775 | 0.406 | 0.357 | 5.724 | 0.166 |
| MASK_IMAGE_IMAGE | 0.631 | 0.330 | 0.431 | 3.304 | 0.181 |
| TEXT_ROTATE | 3.049 | 2.500 | 22.559 | 30.880 | 1.591 |
| IMAGE_QOI_565_8 | 3.019 | 2.129 | 1.157 | 1.124 | 0.882 |
| EXTERN_IMAGE_QOI_565_8 | 3.893 | 2.785 | 1.376 | 1.343 | 1.101 |
| MASK_IMAGE_QOI_8_ROUND_RECT | 3.343 | 2.341 | 1.262 | 1.347 | 0.956 |
| EXTERN_MASK_IMAGE_QOI_8_ROUND_RECT | 4.219 | 2.997 | 1.481 | 1.566 | 1.174 |
| IMAGE_RLE_565_8 | 1.943 | 1.397 | 0.864 | 0.896 | 0.647 |
| EXTERN_IMAGE_RLE_565_8 | 3.354 | 2.454 | 1.217 | 1.249 | 1.000 |
| MASK_IMAGE_RLE_8_ROUND_RECT | 2.273 | 1.610 | 0.971 | 1.121 | 0.721 |
| EXTERN_MASK_IMAGE_RLE_8_ROUND_RECT | 3.684 | 2.669 | 1.323 | 1.474 | 1.074 |
| CHART_LINE_DENSE | 2.993 | 1.048 | 4.833 | 2.457 | 0.556 |
| CHART_BAR_DENSE | 1.251 | 0.693 | 3.185 | 1.576 | 0.433 |
| CHART_SCATTER_DENSE | 1.814 | 1.006 | 2.703 | 3.593 | 0.642 |
| CHART_PIE_DENSE | 3.553 | 2.077 | 6.078 | 3.432 | 1.470 |