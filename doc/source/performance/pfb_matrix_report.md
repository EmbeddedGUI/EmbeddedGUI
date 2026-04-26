# PFB Matrix Report

- Commit: `19e728f2`
- Date: 2026-04-26T18:30:04.213420
- Profile: cortex-m3

![PFB Matrix Heatmap](images/pfb_matrix_report.png)

| Test Case | small (15x15) | middle (30x30) | fullwidth (240x1) | fullheight (1x240) | fullscreen (240x240) |
|-----------|----------:|----------:|----------:|----------:|----------:|
| LINE | 0.962 | 0.596 | 0.632 | 5.688 | 0.383 |
| IMAGE_565 | 0.570 | 0.312 | 0.329 | 1.809 | 0.151 |
| IMAGE_565_8 | 0.991 | 0.612 | 0.543 | 5.961 | 0.362 |
| TEXT | 3.221 | 0.894 | 3.057 | 3.208 | 0.126 |
| INTERNAL_TEXT_RLE4 | 3.363 | 0.963 | 4.271 | 3.298 | 0.195 |
| CIRCLE_FILL | 1.277 | 0.557 | 0.403 | 5.054 | 0.229 |
| LINE_HQ | 1.797 | 1.402 | 1.491 | 6.105 | 1.187 |
| GRADIENT_RECT | 1.072 | 0.547 | 0.317 | 11.048 | 0.159 |
| IMAGE_COLOR | 1.033 | 0.858 | 0.948 | 2.203 | 0.777 |
| MASK_RECT_FILL_ROUND_RECT | 0.771 | 0.406 | 0.354 | 5.722 | 0.167 |
| MASK_IMAGE_IMAGE | 0.628 | 0.330 | 0.428 | 3.302 | 0.182 |
| TEXT_ROTATE | 3.044 | 2.498 | 22.554 | 30.876 | 1.591 |
| IMAGE_QOI_565_8 | 3.014 | 2.129 | 1.154 | 1.121 | 0.883 |
| EXTERN_IMAGE_QOI_565_8 | 3.890 | 2.785 | 1.373 | 1.340 | 1.101 |
| MASK_IMAGE_QOI_8_ROUND_RECT | 3.338 | 2.341 | 1.259 | 1.343 | 0.956 |
| EXTERN_MASK_IMAGE_QOI_8_ROUND_RECT | 4.217 | 2.998 | 1.478 | 1.564 | 1.175 |
| IMAGE_RLE_565_8 | 1.948 | 1.403 | 0.862 | 0.896 | 0.651 |
| EXTERN_IMAGE_RLE_565_8 | 3.084 | 2.255 | 1.147 | 1.181 | 0.935 |
| MASK_IMAGE_RLE_8_ROUND_RECT | 2.277 | 1.617 | 0.969 | 1.123 | 0.724 |
| EXTERN_MASK_IMAGE_RLE_8_ROUND_RECT | 3.414 | 2.469 | 1.253 | 1.407 | 1.008 |
| CHART_LINE_DENSE | 3.014 | 1.058 | 4.901 | 2.482 | 0.562 |
| CHART_BAR_DENSE | 1.271 | 0.704 | 3.250 | 1.696 | 0.438 |
| CHART_SCATTER_DENSE | 1.835 | 1.020 | 2.771 | 3.720 | 0.648 |
| CHART_PIE_DENSE | 3.548 | 2.077 | 6.074 | 3.429 | 1.470 |