# HelloPerformance Fast Path Retention

## 范围

- 本文整理当前已经暴露出来的 fast path / specialized path，目标是回答两个问题：
  - 这条路径是否值得继续保留
  - 是否值得再增加一个长期维护的用户宏
- 本轮判定规则沿用当前用户要求：
  - 只有当某条快路径代码量明显偏大
  - 且关闭后的 `HelloPerformance` QEMU 性能回退 `<10%`
  - 才考虑拿掉，或者改成用户按需启用
- 数据来源分两类：
  - 本轮新做的定向 A/B：字体 `std fast draw`
  - 已有 retention / perf 资料：`stack_heap_policy_retention.md`、`text_transform_heap_override_retention.md`、`static_ram_override_retention.md`、`codec_heap_override_retention.md`、`ram_tracking.md`

## 本轮新增 A/B: `font std fast draw`

### 测试环境

- 日期：`2026-03-31`
- 提交基线：`214edcf`
- `HelloSimple` code size A/B
  - `make all APP=HelloSimple PORT=qemu CPU_ARCH=cortex-m0plus`
  - `make all APP=HelloSimple PORT=qemu CPU_ARCH=cortex-m0plus USER_CFLAGS=-DEGUI_CONFIG_FONT_STD_FAST_DRAW_ENABLE=0`
- `HelloPerformance` perf A/B
  - `python scripts/code_perf_check.py --profile cortex-m3 --clean`
  - `python scripts/code_perf_check.py --profile cortex-m3 --clean --extra-cflags=-DEGUI_CONFIG_FONT_STD_FAST_DRAW_ENABLE=0`

### 代码量证据

- `HelloSimple` 默认开启时：
  - `__code_size = 31412`
  - `__rodata_size = 8308`
  - `__data_size = 84`
  - `__bss_size = 5840`
- 强制关闭 fast draw 后：
  - `__code_size = 23084`
  - `__rodata_size = 8308`
  - `__data_size = 84`
  - `__bss_size = 5840`
- 结论：
  - 只看代码段，关闭后减少 `8328B`
  - `rodata/data/bss` 没变化，说明它基本就是纯代码体积
- 默认构建下能直接看到的大函数：
  - `egui_font_std_draw_fast_mask = 2836B`
  - `egui_font_std_draw_fast_4_ctx = 1812B`
  - `egui_font_std_draw_string_fast_4 = 572B`
  - `egui_font_std_draw_string_fast_4_mask = 432B`
  - `egui_font_std_try_draw_string_in_rect_fast = 376B`

### 性能证据

| 场景 | 默认开启 | 强制关闭 | 变化 |
| --- | ---: | ---: | ---: |
| `TEXT` | `2.278ms` | `2.900ms` | `+27.3%` |
| `TEXT_RECT` | `0.935ms` | `3.895ms` | `+316.6%` |
| `EXTERN_TEXT` | `2.316ms` | `2.922ms` | `+26.2%` |
| `EXTERN_TEXT_RECT` | `1.173ms` | `4.026ms` | `+243.2%` |
| `TEXT_ROTATE_NONE` | `0.934ms` | `3.803ms` | `+307.2%` |
| `TEXT_ROTATE_BUFFERED_NONE` | `0.935ms` | `3.803ms` | `+306.7%` |
| `TEXT_GRADIENT` | `0.601ms` | `1.401ms` | `+133.1%` |
| `TEXT_RECT_GRADIENT` | `1.173ms` | `6.625ms` | `+464.8%` |

- 同时可以看到，它对真正的 rotate 路径几乎没影响：
  - `TEXT_ROTATE 3.371 -> 3.371`
  - `TEXT_ROTATE_BUFFERED 3.372 -> 3.372`
  - `EXTERN_TEXT_ROTATE_BUFFERED 3.586 -> 3.586`
- 这说明该 fast path 的收益主要集中在普通文本、rect 文本、gradient 文本和不走 transform 的文本路径。

### 结论

- 这条路径虽然代码量大，但绝不是“小收益大体积”。
- 它在多个文本热点上都是 `+26% ~ +465%` 量级的回退，远超当前 `<10%` 门槛。
- 因此：
  - 不能拿掉
  - 不应该新增长期维护的用户宏
  - 本轮实验开关只用于 A/B，结论确认后应回到始终开启

## 当前 fast path 结论矩阵

| 路径 | 代码/资源证据 | HelloPerformance 证据 | 当前结论 |
| --- | --- | --- | --- |
| `font std fast draw` | `HelloSimple __code_size -8328B`；主 helper 合计约 `6KB+` | 关闭后 `TEXT +27.3%`、`TEXT_RECT +316.6%`、`TEXT_RECT_GRADIENT +464.8%` | 保留，且不要新增长期宏 |
| rotated-text visible alpha8 fast path | `text_transform_draw_visible_alpha8_tile_layout` 当前约 `8128B` | 历史 A/B 显示 `5120` 相比 `4096` 时 `TEXT_ROTATE_BUFFERED -25.7%`、`EXTERN_TEXT_ROTATE_BUFFERED -28.2%`；`2560B` 已被拒绝 | 保留 fast path，本体不拆；仅把 low-RAM heap ceiling 继续放尾部按需块 |
| `shadow d_sq -> alpha LUT` 策略 | `egui_shadow_draw_corner` 当前约 `1980B`；本质是 stack/LUT 上限策略，不是常规小项目 code-size 宏 | `256 -> 64` 时 `SHADOW_ROUND 6.306 -> 4.949 (-21.5%)`，其它锚点基本不动 | 继续放 `app_egui_config.h` 尾部 `#if 0` low-RAM 块，默认不打开 |
| round-rect / circle `PFB_HEIGHT` row cache | 已有历史结论：关闭后 `text -708B`、`static RAM -16B` | 关闭后无 `>5%` 回退，且 `MASK_IMAGE_QOI_8_ROUND_RECT`、`MASK_IMAGE_RLE_8_ROUND_RECT` 还变快 | 已处理完，默认关闭，不再作为 active override |
| `IMAGE_STD_ALPHA_OPAQUE_CACHE_SLOTS` | 约 `33B` BSS，不是大代码块 | 关闭后最差 `EXTERN_IMAGE_RESIZE_TILED_565_8 +17.78%` | 保留 |
| `IMAGE_CODEC_ROW_CACHE_ENABLE` | 主要影响 heap，不是 small-project code-size toggle | `row-cache off` 会把 QOI/RLE 热点打到 `+1800% ~ +3600%` 量级 | 保留 |

## 当前收敛结论

- 已经能明确确认的“大代码快路径”里，`font std fast draw` 不符合淘汰条件，必须保留。
- 已有的 low-RAM policy 项里：
  - `shadow dsq LUT`
  - `text transform` 的 stack/transient-heap 策略
  - 都已经归到 `app_egui_config.h` 尾部的 `#if 0` 可选块，不再污染默认构建面。
- 已经证明确实“不值得默认开”的快路径缓存项里：
  - `EGUI_CONFIG_IMAGE_STD_ROUND_RECT_FAST_ROW_CACHE_ENABLE`
  - `EGUI_CONFIG_MASK_CIRCLE_FRAME_ROW_CACHE_ENABLE`
  - 这两项已经处理完，默认关闭。
- 已经证明确实“不能回收”的快路径项里：
  - `font std fast draw`
  - `IMAGE_STD_ALPHA_OPAQUE_CACHE_SLOTS`
  - `IMAGE_CODEC_ROW_CACHE_ENABLE`
  - 都需要保留。

## 后续候选

- 下一批如果继续拆，最值得单独做 A/B 的是 `src/image/egui_image_std.c` 里的 circle / round-rect masked-row fast implementation。
- 这块当前有明显的 specialized 代码体积，但还没有独立的总开关和单独 retention 结论。
- 如果继续下一轮，应先做一个只用于实验的编译期开关，再按同样方法量化：
  - 一个会真正拉下来的小项目 code-size case
  - 对应的 `MASK_IMAGE_*_CIRCLE` / `MASK_IMAGE_*_ROUND_RECT` QEMU perf A/B
