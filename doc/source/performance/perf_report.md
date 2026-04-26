# Performance Report

- Commit: `19e728f2`
- Date: 2026-04-26T18:32:28.644686
- Profile: cortex-m3

![Performance Chart](images/perf_report.png)

## Scene Contact Sheet

Timing data comes from QEMU. The contact sheet below is rendered with the PC simulator for scene reference.

![Scene Contact Sheet](images/perf_scenes.png)

## Basic Shapes

| Test Case | Time (ms) |
|-----------|-----------|
| LINE | 0.543 |
| LINE_HQ | 1.357 |
| RECTANGLE_FILL | 0.163 |
| CIRCLE_FILL | 0.581 |

## Text

| Test Case | Time (ms) |
|-----------|-----------|
| TEXT | 1.026 |
| INTERNAL_TEXT_RLE4 | 1.165 |
| TEXT_ROTATE | 2.502 |

## Image Direct Draw

| Test Case | Time (ms) |
|-----------|-----------|
| IMAGE_565 | 0.248 |
| IMAGE_565_8 | 0.513 |

## Image Color Tint

| Test Case | Time (ms) |
|-----------|-----------|
| IMAGE_COLOR | 0.849 |

## Compress

| Test Case | Time (ms) |
|-----------|-----------|
| EXTERN_IMAGE_QOI_565_8 | 2.801 |
| EXTERN_MASK_IMAGE_QOI_8_ROUND_RECT | 3.013 |
| EXTERN_IMAGE_RLE_565_8 | 2.263 |
| EXTERN_MASK_IMAGE_RLE_8_ROUND_RECT | 2.477 |
| IMAGE_QOI_565_8 | 2.143 |
| IMAGE_RLE_565_8 | 1.411 |
| MASK_IMAGE_QOI_8_ROUND_RECT | 2.355 |
| MASK_IMAGE_RLE_8_ROUND_RECT | 1.624 |

## Gradient

| Test Case | Time (ms) |
|-----------|-----------|
| GRADIENT_RECT | 0.400 |

## Mask

| Test Case | Time (ms) |
|-----------|-----------|
| MASK_RECT_FILL_ROUND_RECT | 0.327 |
| MASK_IMAGE_IMAGE | 0.305 |

## Widgets

| Test Case | Time (ms) |
|-----------|-----------|
| CHART_LINE_DENSE | 1.220 |
| CHART_BAR_DENSE | 0.777 |
| CHART_SCATTER_DENSE | 1.068 |
| CHART_PIE_DENSE | 2.325 |

## Animation

| Test Case | Time (ms) |
|-----------|-----------|
| ANIMATION_TRANSLATE | 0.138 |
| ANIMATION_SCALE | 0.146 |
