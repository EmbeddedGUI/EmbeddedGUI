# Canvas Feature QEMU Code Size Report

- Commit: `09190fa`
- Date: 2026-04-02T11:13:37.925806
- Build target: `APP=HelloSizeAnalysis APP_SUB=canvas_path_probe PORT=qemu CPU_ARCH=cortex-m0plus`
- Measurement method: compile a dedicated probe app, then rewrite app-local feature override + probe config headers to enable one feature set.
- Scope: feature-level static qemu ELF sections only (`.text/.rodata/.data/.bss`).

## 说明

- 这份报告回答的是“整项 canvas 能力值不值得打开”，不是单个函数的边际成本。
- baseline 会明确关闭 `MASK / QOI / RLE` 这类有全局开关的能力，再和 feature-on 变体比较。
- 对于没有全局总开关的能力，例如 `IMAGE_TRANSFORM`、`TEXT_TRANSFORM`、`GRADIENT`，这里使用代表性 probe 场景来近似描述该 feature 的整体成本。
- HQ 的 `line/circle/arc` 路径不在本页统计范围内，请结合 `hq_size_report.md` 一起看。

## Baseline

| Variant | Text | Rodata | Data | Bss | Total ROM |
|---------|-----:|-------:|-----:|----:|----------:|
| BASELINE | 11420 | 568 | 24 | 5920 | 11988 |

## Feature Summary

| Feature | Delta Text | Delta Rodata | Delta Data | Delta Bss | Delta ROM |
|---------|-----------:|-------------:|-----------:|----------:|----------:|
| QOI_CODEC | +7132 | +136 | +0 | +28 | +7268 |

## Feature Definition

| Feature | Included Scenes | Description |
|---------|-----------------|-------------|
| QOI_CODEC | `QOI_DRAW` | QOI codec 能力，口径是打开 codec 并强制链接一个代表性绘制场景后的整体增量。 |

## Detailed Variants

| Variant | Feature Config | Text | Rodata | Data | Bss | Total ROM |
|---------|-------------------|-----:|-------:|-----:|----:|----------:|
| BASELINE | `-DEGUI_CONFIG_FUNCTION_SUPPORT_MASK=0 -DEGUI_CONFIG_IMAGE_CODEC_QOI_ENABLE=0 -DEGUI_CONFIG_IMAGE_CODEC_RLE_ENABLE=0` | 11420 | 568 | 24 | 5920 | 11988 |
| QOI_CODEC | `-DEGUI_CONFIG_FUNCTION_SUPPORT_MASK=0 -DEGUI_CONFIG_IMAGE_CODEC_QOI_ENABLE=1 -DEGUI_CONFIG_IMAGE_CODEC_RLE_ENABLE=0 -DEGUI_SIZE_PROBE_QOI_DRAW_PATH=1` | 18552 | 704 | 24 | 5948 | 19256 |

## Reproduce

```bash
python scripts/size_analysis/canvas_feature_size_to_doc.py
```

Raw JSON is written to `output/canvas_feature_size_results.json`.
