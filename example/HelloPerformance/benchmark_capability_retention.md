# HelloPerformance Benchmark Capability 保留记录

## 范围

- 本文记录当前仍保留的 benchmark capability 宏：
  - `EGUI_CONFIG_FUNCTION_SUPPORT_SHADOW`
  - `EGUI_CONFIG_FUNCTION_SUPPORT_MASK`
  - `EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_RGB565_1`
  - `EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_RGB565_2`
  - `EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_RGB565_8`
  - `EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_ALPHA_8`
  - `EGUI_CONFIG_FUNCTION_EXTERNAL_RESOURCE`
  - `EGUI_CONFIG_IMAGE_CODEC_QOI_ENABLE`
  - `EGUI_CONFIG_IMAGE_CODEC_RLE_ENABLE`
- 这组宏的作用是定义 HelloPerformance 要覆盖哪些 benchmark workload，不是 small static-RAM cleanup 的候选。

## 本次支撑报告

- 测量时间：`2026-03-31`
- 测量提交：`c11cbcd`
- 命令：
  - `python scripts/perf_analysis/code_perf_check.py --profile cortex-m3 --threshold 5 --clean`
- 结果：
  - 当前 clean perf 报告共输出 `222` 个测试项
  - 其中：
    - `SHADOW*` 场景 `2` 项
    - `MASK_*` 场景 `49` 项
    - `EXTERN_*` 场景 `62` 项
    - `*QOI*` 场景 `22` 项
    - `*RLE*` 场景 `22` 项

## 宏结论

### `EGUI_CONFIG_FUNCTION_SUPPORT_SHADOW`

- 当前 perf 报告直接覆盖：
  - `SHADOW 4.039ms`
  - `SHADOW_ROUND 4.949ms`
- 这是 benchmark workload 开关，不是 small-SRAM toggle。
- 处理结论：保留。

### `EGUI_CONFIG_FUNCTION_SUPPORT_MASK`

- 当前 perf 报告中有 `49` 个 `MASK_*` 场景，覆盖普通 mask、图片 mask、QOI/RLE mask、渐变 mask 和多尺寸变体。
- 代表项：
  - `MASK_IMAGE_ROUND_RECT 2.694ms`
  - `MASK_IMAGE_QOI_8_ROUND_RECT 2.460ms`
  - `MASK_IMAGE_RLE_8_ROUND_RECT 1.995ms`
- 如果关闭这个宏，整个 mask 场景集合都会从 HelloPerformance 消失。
- 处理结论：保留。

### `EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_RGB565_1`

- 当前 perf 报告直接覆盖：
  - `IMAGE_565_1 0.863ms`
  - `IMAGE_RESIZE_565_1 2.154ms`
  - `IMAGE_ROTATE_565_1 3.444ms`
  - `EXTERN_IMAGE_565_1 1.460ms`
- 这是 benchmark 覆盖的图片格式族，不是 SRAM 微调项。
- 处理结论：保留。

### `EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_RGB565_2`

- 当前 perf 报告直接覆盖：
  - `IMAGE_565_2 1.112ms`
  - `IMAGE_RESIZE_565_2 2.313ms`
  - `IMAGE_ROTATE_565_2 3.600ms`
  - `EXTERN_IMAGE_565_2 1.709ms`
- 处理结论：保留。

### `EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_RGB565_8`

- 当前 perf 报告直接覆盖：
  - `IMAGE_565_8 1.035ms`
  - `IMAGE_RESIZE_565_8 1.749ms`
  - `IMAGE_ROTATE_565_8 3.452ms`
  - `EXTERN_IMAGE_565_8 1.632ms`
- 处理结论：保留。

### `EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_ALPHA_8`

- 当前 perf 报告仍在覆盖 alpha8 路径，代表项：
  - `MASK_IMAGE_QOI_8_NO_MASK 1.792ms`
  - `EXTERN_MASK_IMAGE_QOI_8_NO_MASK 2.261ms`
  - `MASK_IMAGE_RLE_8_NO_MASK 1.588ms`
  - `EXTERN_MASK_IMAGE_RLE_8_NO_MASK 2.497ms`
- 关闭后会直接删掉 alpha8 相关 workload 覆盖。
- 处理结论：保留。

### `EGUI_CONFIG_FUNCTION_EXTERNAL_RESOURCE`

- 当前 perf 报告中有 `62` 个 `EXTERN_*` 场景。
- 代表项：
  - `EXTERN_TEXT 1.364ms`
  - `EXTERN_IMAGE_565 1.114ms`
  - `EXTERN_IMAGE_ROTATE_565_8 18.990ms`
  - `EXTERN_MASK_IMAGE_QOI_ROUND_RECT 13.404ms`
- 这个宏定义的是“是否保留外部资源 benchmark 维度”，不是 SRAM 小优化。
- 处理结论：保留。

### `EGUI_CONFIG_IMAGE_CODEC_QOI_ENABLE`

- 当前 perf 报告中有 `22` 个 `*QOI*` 场景。
- 代表项：
  - `IMAGE_QOI_565 6.118ms`
  - `IMAGE_QOI_565_8 1.791ms`
  - `EXTERN_IMAGE_QOI_565 12.826ms`
  - `MASK_IMAGE_QOI_8_ROUND_RECT 2.460ms`
- 关闭后 QOI benchmark 族会整体消失。
- 处理结论：保留。

### `EGUI_CONFIG_IMAGE_CODEC_RLE_ENABLE`

- 当前 perf 报告中有 `22` 个 `*RLE*` 场景。
- 代表项：
  - `IMAGE_RLE_565 1.819ms`
  - `IMAGE_RLE_565_8 1.587ms`
  - `EXTERN_IMAGE_RLE_565 5.234ms`
  - `MASK_IMAGE_RLE_8_ROUND_RECT 1.995ms`
- 关闭后 RLE benchmark 族会整体消失。
- 处理结论：保留。

## 最终结论

- 这组宏不是“为省几十字节 SRAM 临时引入的功能开关”。
- 它们决定 HelloPerformance 是否继续覆盖阴影、mask、外部资源、QOI/RLE、RGB565/alpha8 等 workload 维度。
- 因此本轮不做“默认打开后移除宏管理”的 small static-RAM 清理，而是保持为当前 benchmark 能力定义。
