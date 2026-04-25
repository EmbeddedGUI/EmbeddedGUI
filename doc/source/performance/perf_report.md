# Performance Report

- Commit: `a98fa1b3`
- Date: 2026-04-25T09:44:36.886915
- Profile: cortex-m3

![Performance Chart](images/perf_report.png)

## Scene Contact Sheet

Timing data comes from QEMU. The contact sheet below is rendered with the PC simulator for scene reference.

![Scene Contact Sheet](images/perf_scenes.png)

## Basic Shapes

| Test Case | Time (ms) |
|-----------|-----------|
| LINE | 0.543 |
| LINE_HQ | 1.356 |
| RECTANGLE_FILL | 0.163 |
| CIRCLE_FILL | 0.580 |

## Text

| Test Case | Time (ms) |
|-----------|-----------|
| TEXT | 1.034 |
| INTERNAL_TEXT_RLE4 | 1.171 |
| TEXT_ROTATE | 2.503 |

## Image Direct Draw

| Test Case | Time (ms) |
|-----------|-----------|
| IMAGE_565 | 0.247 |
| IMAGE_565_8 | 0.512 |

## Image Color Tint

| Test Case | Time (ms) |
|-----------|-----------|
| IMAGE_COLOR | 0.848 |

## Compress

| Test Case | Time (ms) |
|-----------|-----------|
| EXTERN_IMAGE_QOI_565_8 | 2.800 |
| EXTERN_MASK_IMAGE_QOI_8_ROUND_RECT | 3.012 |
| EXTERN_IMAGE_RLE_565_8 | 2.463 |
| EXTERN_MASK_IMAGE_RLE_8_ROUND_RECT | 2.678 |
| IMAGE_QOI_565_8 | 2.143 |
| IMAGE_RLE_565_8 | 1.403 |
| MASK_IMAGE_QOI_8_ROUND_RECT | 2.355 |
| MASK_IMAGE_RLE_8_ROUND_RECT | 1.616 |

## Gradient

| Test Case | Time (ms) |
|-----------|-----------|
| GRADIENT_RECT | 0.399 |

## Mask

| Test Case | Time (ms) |
|-----------|-----------|
| MASK_RECT_FILL_ROUND_RECT | 0.326 |
| MASK_IMAGE_IMAGE | 0.305 |

## Widgets

| Test Case | Time (ms) |
|-----------|-----------|
| CHART_LINE_DENSE | 1.204 |
| CHART_BAR_DENSE | 0.766 |
| CHART_SCATTER_DENSE | 1.055 |
| CHART_PIE_DENSE | 2.324 |

## Animation

| Test Case | Time (ms) |
|-----------|-----------|
| ANIMATION_TRANSLATE | 0.137 |
| ANIMATION_SCALE | 0.144 |
