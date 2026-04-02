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
  - `python scripts/perf_analysis/code_perf_check.py --profile cortex-m3 --clean`
  - `python scripts/perf_analysis/code_perf_check.py --profile cortex-m3 --clean --extra-cflags=-DEGUI_CONFIG_FONT_STD_FAST_DRAW_ENABLE=0`

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

## 本轮新增 A/B: `image mask circle / round-rect fast path`

### 测试环境

- 日期：`2026-03-31`
- 提交基线：`e1144cf`
- `HelloBasic(mask)` code size A/B
  - `make all APP=HelloBasic APP_SUB=mask PORT=qemu CPU_ARCH=cortex-m0plus`
  - `make all APP=HelloBasic APP_SUB=mask PORT=qemu CPU_ARCH=cortex-m0plus USER_CFLAGS=-DEGUI_CONFIG_IMAGE_STD_MASK_FAST_ENABLE=0`
- `HelloPerformance` perf A/B
  - 基线结果：`.claude/perf_fast_path_image_mask_fast_on_results.json`
  - 关闭 fast path：`.claude/perf_fast_path_image_mask_fast_off_results.json`

### 代码量证据

- `HelloBasic(mask)` 默认开启时：
  - `__code_size = 95660`
  - `__rodata_size = 43972`
  - `__data_size = 56`
  - `__bss_size = 6212`
  - 链接汇总：`text=139748 data=56 bss=276552`
- 强制关闭 fast path 后：
  - `__code_size = 90496`
  - `__rodata_size = 43972`
  - `__data_size = 56`
  - `__bss_size = 6212`
  - 链接汇总：`text=134584 data=56 bss=276552`
- 结论：
  - 只看代码段，关闭后减少 `5164B`
  - `rodata/data/bss` 都没有变化，说明这块主要是纯代码体积
  - 这部分不会拉低 `HelloSimple`，但会实打实进入带 mask image 的小项目

### 性能证据

| 场景 | 默认开启 | 强制关闭 | 变化 |
| --- | ---: | ---: | ---: |
| `MASK_IMAGE_RLE_CIRCLE` | `1.880ms` | `3.327ms` | `+77.0%` |
| `EXTERN_MASK_IMAGE_RLE_CIRCLE` | `2.788ms` | `4.362ms` | `+56.5%` |
| `MASK_IMAGE_RLE_8_CIRCLE` | `1.702ms` | `2.533ms` | `+48.8%` |
| `MASK_IMAGE_QOI_CIRCLE` | `3.127ms` | `4.598ms` | `+47.0%` |
| `MASK_IMAGE_QOI_8_CIRCLE` | `1.892ms` | `2.716ms` | `+43.6%` |
| `MASK_IMAGE_RLE_ROUND_RECT` | `1.724ms` | `2.420ms` | `+40.4%` |
| `EXTERN_MASK_IMAGE_QOI_CIRCLE` | `6.448ms` | `8.048ms` | `+24.8%` |
| `MASK_IMAGE_TEST_PERF_CIRCLE` | `1.318ms` | `1.632ms` | `+23.8%` |
| `MASK_IMAGE_QOI_ROUND_RECT` | `2.970ms` | `3.667ms` | `+23.5%` |
| `MASK_IMAGE_QOI_8_ROUND_RECT` | `1.790ms` | `2.024ms` | `+13.1%` |
| `EXTERN_MASK_IMAGE_QOI_8_ROUND_RECT` | `1.980ms` | `2.214ms` | `+11.8%` |
| `EXTERN_MASK_IMAGE_QOI_ROUND_RECT` | `6.291ms` | `6.987ms` | `+11.1%` |

- 同时也能看到，这块对纯 resize 路径基本没收益负担：
  - `IMAGE_RESIZE_565_8 1.045 -> 1.050 (+0.5%)`
  - `EXTERN_IMAGE_RESIZE_565_8 1.118 -> 1.121 (+0.3%)`
  - `IMAGE_RESIZE_TILED_565_8 1.575 -> 1.521 (-3.4%)`
  - `EXTERN_IMAGE_RESIZE_TILED_565_8 1.462 -> 1.417 (-3.1%)`
- 说明这条 specialized path 的价值集中在 masked image 的 circle / round-rect 热点，尤其是 QOI/RLE 和 external resource 组合。

### 结论

- 这条路径属于“中等偏大代码量，但高价值”的 fast path。
- 它并不符合“收益低于 10% 且代码量又大的路径应该回收”的条件。
- 因此：
  - 不能拿掉
  - 不应该保留长期用户宏
  - 本轮实验开关只用于 A/B，结论确认后应回到默认始终开启

## 当前 fast path 结论矩阵

| 路径 | 代码/资源证据 | HelloPerformance 证据 | 当前结论 |
| --- | --- | --- | --- |
| `font std fast draw` | `HelloSimple __code_size -8328B`；主 helper 合计约 `6KB+` | 关闭后 `TEXT +27.3%`、`TEXT_RECT +316.6%`、`TEXT_RECT_GRADIENT +464.8%` | 保留，且不要新增长期宏 |
| `image mask circle / round-rect fast path` | `HelloBasic(mask) __code_size -5164B`；主要是 masked image 专用代码体积 | 关闭后 `MASK_IMAGE_RLE_CIRCLE +77.0%`、`MASK_IMAGE_QOI_CIRCLE +47.0%`、`MASK_IMAGE_RLE_ROUND_RECT +40.4%` | 保留，且不要新增长期宏 |
| rotated-text visible alpha8 fast path | `text_transform_draw_visible_alpha8_tile_layout` 当前约 `8128B` | 历史 A/B 显示 `5120` 相比 `4096` 时 `TEXT_ROTATE_BUFFERED -25.7%`、`EXTERN_TEXT_ROTATE_BUFFERED -28.2%`；`2560B` 已被拒绝 | 保留 fast path，本体不拆；仅把 low-RAM heap ceiling 继续放尾部按需块 |
| `shadow d_sq -> alpha LUT` 策略 | `egui_shadow_draw_corner` 当前约 `1980B`；本质是 stack/LUT 上限策略，不是常规小项目 code-size 宏 | `256 -> 64` 时 `SHADOW_ROUND 6.306 -> 4.949 (-21.5%)`，其它锚点基本不动 | 继续放 `app_egui_config.h` 尾部 `#if 0` low-RAM 块，默认不打开 |
| round-rect / circle `PFB_HEIGHT` row cache | 已有历史结论：关闭后 `text -708B`、`static RAM -16B` | 关闭后无 `>5%` 回退，且 `MASK_IMAGE_QOI_8_ROUND_RECT`、`MASK_IMAGE_RLE_8_ROUND_RECT` 还变快 | 已处理完，默认关闭，不再作为 active override |
| `IMAGE_STD_ALPHA_OPAQUE_CACHE_SLOTS` | 约 `33B` BSS，不是大代码块 | 关闭后最差 `EXTERN_IMAGE_RESIZE_TILED_565_8 +17.78%` | 保留 |
| `IMAGE_CODEC_ROW_CACHE_ENABLE` | 主要影响 heap，不是 small-project code-size toggle | `row-cache off` 会把 QOI/RLE 热点打到 `+1800% ~ +3600%` 量级 | 保留 |

## 当前收敛结论

- 已经能明确确认的“大代码快路径”里：
  - `font std fast draw`
  - `image mask circle / round-rect fast path`
  - 都不符合淘汰条件，必须保留。
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
  - `image mask circle / round-rect fast path`
  - `IMAGE_STD_ALPHA_OPAQUE_CACHE_SLOTS`
  - `IMAGE_CODEC_ROW_CACHE_ENABLE`
  - 都需要保留。

## 后续候选

- 下一批如果继续拆，建议优先看仍然存在明显 helper 体积、且能找到明确小项目 size case 的 specialized text/image 路径。
- 处理原则维持不变：
  - 先做只用于实验的编译期开关
  - 再补一个会真实下探的 code-size case
  - 最后用 `HelloPerformance` QEMU 跑完整 A/B 再决定是否回收
