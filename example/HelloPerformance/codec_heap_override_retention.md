# HelloPerformance Codec Heap Override 保留记录

## 范围

- 本文记录当前仍保留的 codec/decode 侧 override：
  - `EGUI_CONFIG_IMAGE_CODEC_ROW_CACHE_ENABLE`
  - `EGUI_CONFIG_IMAGE_DECODE_MAX_PIXEL_SIZE`
  - `EGUI_CONFIG_IMAGE_DECODE_OPAQUE_ALPHA_ROW_USE_ROW_CACHE`
  - `EGUI_CONFIG_IMAGE_QOI_CHECKPOINT_COUNT`
  - `EGUI_CONFIG_IMAGE_CODEC_TAIL_ROW_CACHE_ENABLE`
- 这些宏控制的是瞬时 decode heap 形态和 codec 行为，不是本轮 small static-RAM cleanup 的 `<100B static RAM` 候选。

## 证据来源

- `example/HelloPerformance/ram_tracking.md`
- `../../doc/source/performance/low_ram_config_macros.md`
- 相关 QEMU 检查命令均已在历史 A/B 中使用：
  - `python scripts/perf_analysis/code_perf_check.py --profile cortex-m3 --threshold 5`
  - `python scripts/perf_analysis/code_perf_check.py --profile cortex-m3 --threshold 10`

## 宏结论

### `EGUI_CONFIG_IMAGE_CODEC_ROW_CACHE_ENABLE`

- `2026-03-28` 已做 `row-cache off` A/B。
- 结果不是“小代价可去宏管理”，而是默认路径直接崩坏：
  - heap peak 虽然 `11616B -> 7520B`
  - 但实验构建的 `static RAM` 反而升到 `13280B`
  - 压缩图热点 perf 灾难性回退：
    - `IMAGE_TILED_QOI_565_0 1.232 -> 46.212 (+3651%)`
    - `IMAGE_QOI_565 12.271 -> 434.547 (+3441%)`
    - `EXTERN_IMAGE_QOI_565 18.747 -> 589.348 (+3044%)`
    - `EXTERN_IMAGE_RLE_565 4.647 -> 91.420 (+1867%)`
- 处理结论：必须保持 `1`，同时保留 `#ifndef` 外部覆盖入口用于后续 A/B。

### `EGUI_CONFIG_IMAGE_DECODE_MAX_PIXEL_SIZE`

- `../../doc/source/performance/low_ram_config_macros.md` 已给出当前量级：
  - 默认 `4`
  - HelloPerformance 固定 `2`
  - 对应约 `7680B` 的 heap 峰值上界差异
- HelloPerformance 的压缩图资源当前是 RGB565-only，因此这个宏本质上是在约束 decode heap 上界，不是 fixed static RAM 开关。
- 处理结论：保留 `2`，并保留 `#ifndef` 外部覆盖入口；它不属于本轮 small static-RAM cleanup。

### `EGUI_CONFIG_IMAGE_DECODE_OPAQUE_ALPHA_ROW_USE_ROW_CACHE`

- `../../doc/source/performance/low_ram_config_macros.md` 已说明此宏依赖 `IMAGE_CODEC_ROW_CACHE_ENABLE=1`，作用是复用现有 row-cache 的 alpha 缓冲头部，而不是再单独保留一份不透明 alpha 参考行。
- 当前量级约为：
  - `~240B` alpha 行数据
  - `+8B` 指针/容量元数据
  - 合计约 `248B`
- 这属于 decode heap 组织方式，且与 row-cache A/B 绑定，不是可按 `<100B static RAM` 规则清掉的 app 小宏。
- 处理结论：保留 `1`，并保留 `#ifndef` 外部覆盖入口。

### `EGUI_CONFIG_IMAGE_QOI_CHECKPOINT_COUNT`

- 当前 shipped 值为 `0`，因为现行 row-band + tail-row 默认已经可以把 QOI 场景压到可接受的 heap/perf 区间。
- `2026-03-29` 的 follow-up A/B 已验证：在更窄 tail cache 的失败方案里，把 `EGUI_CONFIG_IMAGE_QOI_CHECKPOINT_COUNT` 提到 `1/2` 也救不回来，相关变体仍有 `+45% ~ +66%` 的 perf 回退。
- 这说明它是 codec 恢复路径的 heap/perf 调参点，不是本轮 small static-RAM cleanup 范围内的固定 SRAM 宏。
- 处理结论：继续保持 `0`，并保留 `#ifndef` 外部覆盖入口给后续 codec A/B。

### `EGUI_CONFIG_IMAGE_CODEC_TAIL_ROW_CACHE_ENABLE`

- `2026-03-28` 已接受当前 low-RAM codec 模式：
  - whole-run heap peak `11616B -> 10032B`，减少 `1584B`
  - `static RAM` 仅 `+8B`
  - 最坏已验证 perf 为 `IMAGE_RLE_565_8 +8.35%`
  - `python scripts/perf_analysis/code_perf_check.py --profile cortex-m3 --threshold 10` PASSED
- 这里的主要取舍是“大幅调整瞬时 heap 形态”，而不是处理 `<100B static RAM` 的小宏。
- `2026-03-29` 后续也验证了更窄 tail cap + checkpoint 组合仍然会掉到 `+45% ~ +66%` 的 perf cliff，说明这条路径后续还需要保留外部覆盖能力做 codec A/B。
- 处理结论：保持 `1`，并继续保留 `#ifndef` 外部覆盖入口。

## 最终结论

- 这 5 个宏都属于 HelloPerformance 的 codec/decode heap 调参入口。
- 它们的主要影响面是 heap 峰值、codec 行缓存形态和压缩图热点性能，而不是 `<100B` 的固定 static RAM。
- 因此本轮不做“默认打开后移除宏管理”，仅保留当前 shipped 默认值和外部 A/B 能力。
