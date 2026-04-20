# Performance Report

- Commit: `e82db1af`
- Date: 2026-04-20T14:31:08.641669
- Profile: cortex-m3

![Performance Chart](images/perf_report.png)

## Scene Contact Sheet

Timing data comes from QEMU. The contact sheet below is rendered with the PC simulator for scene reference.

![Scene Contact Sheet](images/perf_scenes.png)

## Basic Shapes

| Test Case | Time (ms) |
|-----------|-----------|
| LINE | 0.544 |
| LINE_HQ | 1.358 |
| RECTANGLE_FILL | 0.164 |
| CIRCLE_FILL | 0.581 |

## Text

| Test Case | Time (ms) |
|-----------|-----------|
| TEXT | 1.036 |
| INTERNAL_TEXT_RLE4 | 1.173 |
| TEXT_ROTATE | 2.504 |

## Image Direct Draw

| Test Case | Time (ms) |
|-----------|-----------|
| IMAGE_565 | 0.249 |
| IMAGE_565_8 | 0.513 |

## Image Color Tint

| Test Case | Time (ms) |
|-----------|-----------|
| IMAGE_COLOR | 0.849 |

## Compress

| Test Case | Time (ms) |
|-----------|-----------|
| EXTERN_IMAGE_QOI_565_8 | 2.800 |
| EXTERN_MASK_IMAGE_QOI_8_ROUND_RECT | 3.013 |
| EXTERN_IMAGE_RLE_565_8 | 2.462 |
| EXTERN_MASK_IMAGE_RLE_8_ROUND_RECT | 2.676 |
| IMAGE_QOI_565_8 | 2.145 |
| IMAGE_RLE_565_8 | 1.404 |
| MASK_IMAGE_QOI_8_ROUND_RECT | 2.356 |
| MASK_IMAGE_RLE_8_ROUND_RECT | 1.618 |

## Gradient

| Test Case | Time (ms) |
|-----------|-----------|
| GRADIENT_RECT | 0.400 |

## Mask

| Test Case | Time (ms) |
|-----------|-----------|
| MASK_RECT_FILL_ROUND_RECT | 0.327 |
| MASK_IMAGE_IMAGE | 0.307 |

## Widgets

| Test Case | Time (ms) |
|-----------|-----------|
| CHART_LINE_DENSE | 1.205 |
| CHART_BAR_DENSE | 0.767 |
| CHART_SCATTER_DENSE | 1.056 |
| CHART_PIE_DENSE | 2.325 |

## Animation

| Test Case | Time (ms) |
|-----------|-----------|
| ANIMATION_TRANSLATE | 0.139 |
| ANIMATION_SCALE | 0.146 |
