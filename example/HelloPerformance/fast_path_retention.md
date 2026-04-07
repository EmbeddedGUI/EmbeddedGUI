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
  - 本轮新做的定向 A/B：
    - 字体 `std fast draw`
    - 基本图片 `std image fast draw`
  - 已有 retention / perf 资料：`stack_heap_policy_retention.md`、`text_transform_heap_override_retention.md`、`static_ram_override_retention.md`、`codec_heap_override_retention.md`、`ram_tracking.md`

## 当前公开口径（2026-04-06）

- 本文保留了大量历史 A/B、优先级结论和“值得保留的实验记录”，但它们不等于当前主线仍公开暴露的 `EGUI_CONFIG_*` 集合。
- 当前主线已经没有继续挂在 shared default 头里的公共 fast-path 宏。
- 当前框架侧已经不再保留公开 `EGUI_CONFIG_*` fast-path/perf-opt 入口。
- 当前仍保留的示例局部 override 兼容入口只有 `EGUI_CONFIG_FONT_STD_FAST_DRAW_ENABLE` 与 `EGUI_CONFIG_CIRCLE_FILL_BASIC`。
- 像 `EGUI_CONFIG_CANVAS_MASK_FILL_CIRCLE_SEGMENT_FAST_PATH_ENABLE`、`EGUI_CONFIG_CANVAS_MASK_FILL_ROW_BLEND_FAST_PATH_ENABLE`、`EGUI_CONFIG_CANVAS_MASK_FILL_IMAGE_FAST_PATH_ENABLE`、`EGUI_CONFIG_FONT_STD_FAST_MASK_DRAW_ENABLE`、`EGUI_CONFIG_FONT_STD_MASK_ROW_BLEND_FAST_PATH_ENABLE`、`EGUI_CONFIG_MASK_IMAGE_IDENTITY_SCALE_BLEND_FAST_PATH_ENABLE`、`EGUI_CONFIG_IMAGE_CODEC_MASK_IMAGE_ROW_BLOCK_FAST_PATH_ENABLE`、`EGUI_CONFIG_IMAGE_STD_ROW_OVERLAY_ALPHA8_FAST_PATH_ENABLE` 这类名字，本文里继续保留的是 A/B 证据和优先级结论；当前主线实现已经把它们收成 private policy，不再建议应用侧继续把它们当公开配置依赖。
- `EGUI_CONFIG_IMAGE_STD_ROW_OVERLAY_NON_RGB565_FAST_PATH_ENABLE`、`EGUI_CONFIG_CANVAS_MASK_FILL_ROW_RANGE_FAST_PATH_ENABLE`、`EGUI_CONFIG_CANVAS_MASK_FILL_ROW_PARTIAL_FAST_PATH_ENABLE`、`EGUI_CONFIG_CANVAS_MASK_FILL_ROW_INSIDE_FAST_PATH_ENABLE` 这 4 个名字虽然历史上保留过应用侧 bridge，但当前仓内已经没有真实配置继续依赖，主线实现也已经把 bridge 收回。
- 上面这 2 个保留名字现在都只是应用局部 override，而不是框架级共享默认 fast-path 配置。
- 因此，阅读本文时需要区分两层：
  - “当前仍公开暴露的宏”
  - “历史上做过、但现在只保留证据记录的实验宏”

## 本轮新增 A/B: `font std fast draw`

### 测试环境

- 日期：`2026-03-31`
- 提交基线：`214edcf`
- 说明：下面命令已经按当前 app-local 名字更新；原始历史 A/B 当时仍使用框架侧旧名。
- `HelloSimple` code size A/B
  - `make all APP=HelloSimple PORT=qemu CPU_ARCH=cortex-m0plus`
  - `make all APP=HelloSimple PORT=qemu CPU_ARCH=cortex-m0plus USER_CFLAGS=-DAPP_EGUI_FONT_STD_FAST_DRAW_ENABLE=0`
- `HelloPerformance` perf A/B
  - `python scripts/perf_analysis/code_perf_check.py --profile cortex-m3 --clean`
  - `python scripts/perf_analysis/code_perf_check.py --profile cortex-m3 --clean --extra-cflags=-DAPP_EGUI_FONT_STD_FAST_DRAW_ENABLE=0`

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

## 本轮继续拆分 A/B: `font std fast mask draw`

### 测试环境

- 日期：`2026-04-03`
- 提交基线：`dabeeef`
- `HelloPerformance` code size A/B
  - `make -j APP=HelloPerformance PORT=qemu CPU_ARCH=cortex-m0plus APP_OBJ_SUFFIX=hptxtmaskon`
  - `make -j APP=HelloPerformance PORT=qemu CPU_ARCH=cortex-m0plus APP_OBJ_SUFFIX=hptxtmaskoff USER_CFLAGS=-DEGUI_CONFIG_FONT_STD_FAST_MASK_DRAW_ENABLE=0`
- `HelloPerformance` perf A/B
  - `python scripts/perf_analysis/code_perf_check.py --profile cortex-m3 --clean --filter RECTANGLE,CIRCLE,ROUND_RECTANGLE,TEXT,EXTERN_TEXT,IMAGE_565,EXTERN_IMAGE_565`
  - `python scripts/perf_analysis/code_perf_check.py --profile cortex-m3 --clean --filter RECTANGLE,CIRCLE,ROUND_RECTANGLE,TEXT,EXTERN_TEXT,IMAGE_565,EXTERN_IMAGE_565 --extra-cflags=-DEGUI_CONFIG_FONT_STD_FAST_MASK_DRAW_ENABLE=0`
- 运行/单测验证
  - `python scripts/code_runtime_check.py --app HelloPerformance --timeout 10`
  - `make -j APP=HelloUnitTest PORT=pc_test APP_OBJ_SUFFIX=utfontmask`
  - `output/main.exe`

### 代码量证据

- `HelloPerformance` 链接汇总：
  - 默认开启：`text=2056200 data=72 bss=3832`
  - 关闭 masked fast draw：`text=2053624 data=72 bss=3832`
- 结论：
  - `text -2576B`
  - `data/bss` 不变
- 对象级证据也一致：
  - `output/obj/hptxtmaskon_qemu/src/font/egui_font_std.c.o = 114764B`
  - `output/obj/hptxtmaskoff_qemu/src/font/egui_font_std.c.o = 87228B`
  - 主要被编掉的是：
    - `egui_font_std_draw_fast_mask`
    - `egui_font_std_draw_string_fast_4_mask`

### 性能证据

| 场景 | 默认开启 | 关闭 masked fast draw | 变化 |
| --- | ---: | ---: | ---: |
| `RECTANGLE` | `0.179ms` | `0.179ms` | `+0.0%` |
| `CIRCLE` | `0.810ms` | `0.810ms` | `+0.0%` |
| `ROUND_RECTANGLE` | `0.812ms` | `0.812ms` | `+0.0%` |
| `TEXT` | `1.026ms` | `1.023ms` | `-0.3%` |
| `EXTERN_TEXT` | `1.065ms` | `1.060ms` | `-0.5%` |
| `IMAGE_565` | `0.239ms` | `0.239ms` | `+0.0%` |
| `EXTERN_IMAGE_565` | `0.417ms` | `0.417ms` | `+0.0%` |

- 这说明被拆出来的主要是 `mask->mask_blend_row_color()` 这一支专用文字快路径，当前基础 `TEXT` / `EXTERN_TEXT` 直绘场景不依赖它。
- 当前 `HelloPerformance` 过滤集里没有单独的 masked-text 场景，因此这轮结论只对基础 text 路径成立，不外推到所有带 mask 的文本场景。

### 结论

- 这条子路径满足：
  - 代码量收益 `>1KB`
  - 当前基础优先场景性能回退 `<10%`
- 因此：
  - 保留 `EGUI_CONFIG_FONT_STD_FAST_MASK_DRAW_ENABLE`
  - 它比整条 `font std fast draw` 更适合做细粒度 size-first 宏
  - 仍建议默认保持开启，只在“不关心 masked text 热点”的项目里按需关闭

## 本轮继续细拆 A/B: `font std mask row-blend fast path`

### 测试环境

- 日期：`2026-04-04`
- 提交基线：`b3717fd`
- `HelloPerformance` code size A/B
  - `make -j APP=HelloPerformance PORT=qemu CPU_ARCH=cortex-m0plus APP_OBJ_SUFFIX=hpfontrowbase`
  - `make -j APP=HelloPerformance PORT=qemu CPU_ARCH=cortex-m0plus APP_OBJ_SUFFIX=hpfontrowoff USER_CFLAGS=-DEGUI_CONFIG_FONT_STD_MASK_ROW_BLEND_FAST_PATH_ENABLE=0`
- `HelloPerformance` perf A/B
  - `python scripts/perf_analysis/code_perf_check.py --profile cortex-m3 --clean --filter RECTANGLE,CIRCLE,ROUND_RECTANGLE,TEXT,EXTERN_TEXT,TEXT_GRADIENT,TEXT_RECT_GRADIENT,TEXT_ROTATE_GRADIENT,IMAGE_565,EXTERN_IMAGE_565`
  - `python scripts/perf_analysis/code_perf_check.py --profile cortex-m3 --clean --filter RECTANGLE,CIRCLE,ROUND_RECTANGLE,TEXT,EXTERN_TEXT,TEXT_GRADIENT,TEXT_RECT_GRADIENT,TEXT_ROTATE_GRADIENT,IMAGE_565,EXTERN_IMAGE_565 --extra-cflags=-DEGUI_CONFIG_FONT_STD_MASK_ROW_BLEND_FAST_PATH_ENABLE=0`
- 运行 / 单测验证
  - `HelloPerformance` baseline / off 通过 `scripts.code_runtime_check.compile_app()` + `run_app()` 分别注入默认配置和 `-DEGUI_CONFIG_FONT_STD_MASK_ROW_BLEND_FAST_PATH_ENABLE=0` 录制
  - `make -j APP=HelloUnitTest PORT=pc_test APP_OBJ_SUFFIX=utfontrowblendbase`
  - `make -j APP=HelloUnitTest PORT=pc_test APP_OBJ_SUFFIX=utfontrowblendoff USER_CFLAGS=-DEGUI_CONFIG_FONT_STD_MASK_ROW_BLEND_FAST_PATH_ENABLE=0`
  - `output/main.exe`

### 代码量证据

- `HelloPerformance` 链接汇总：
  - 默认开启：`text=2056256 data=72 bss=3832`
  - 关闭 row-blend child：`text=2054624 data=72 bss=3832`
- 结论：
  - `text -1632B`
  - `data/bss` 不变
- 对象级符号扫描显示：
  - `egui_font_std_draw_fast_mask = 0x700 = 1792B`
  - 关闭 child 后，这个符号被完全编掉
  - `egui_font_std_draw_string_fast_4_mask.constprop.0` 只从 `0x1d8` 收到 `0x1c4`
  - 剩余 fallback 主要复用现有 `egui_font_std_draw_single_char_desc`

### 性能证据

| 场景 | 默认开启 | 关闭 row-blend child | 变化 |
| --- | ---: | ---: | ---: |
| `RECTANGLE` | `0.179ms` | `0.179ms` | `+0.0%` |
| `CIRCLE` | `0.810ms` | `0.810ms` | `+0.0%` |
| `ROUND_RECTANGLE` | `0.812ms` | `0.812ms` | `+0.0%` |
| `TEXT` | `1.027ms` | `1.026ms` | `-0.1%` |
| `EXTERN_TEXT` | `1.065ms` | `1.066ms` | `+0.1%` |
| `TEXT_GRADIENT` | `0.232ms` | `0.759ms` | `+227.2%` |
| `TEXT_RECT_GRADIENT` | `0.761ms` | `4.322ms` | `+467.9%` |
| `TEXT_ROTATE_GRADIENT` | `2.552ms` | `2.551ms` | `-0.0%` |
| `IMAGE_565` | `0.239ms` | `0.239ms` | `+0.0%` |
| `EXTERN_IMAGE_565` | `0.417ms` | `0.417ms` | `+0.0%` |

- 这说明新 child 基本就是 `mask->mask_blend_row_color()` 的 glyph 级 row-blend 专用实现。
- 基础 `TEXT` / `EXTERN_TEXT` 和其它优先主路径都保持在噪声范围。
- 代价只落在 internal gradient masked-text 热点：`TEXT_GRADIENT`、`TEXT_RECT_GRADIENT`。
- `TEXT_ROTATE_GRADIENT` 不受影响，说明 rotated-text 仍主要走 text-transform 路径，而不是这条 std-font child。

### 运行 / 单测

- `HelloPerformance` runtime：
  - baseline / off 都是 `241 frames`
  - `frame_0000.png` 哈希一致：`A4337E1408AB6C17F77FC5F1D5BB6815E8EEAB83B934854E1EEBDAF37340D7A5`
  - `frame_0240.png` 哈希一致：`619EA7069DAC066ABA1A27113EAC475C9B13391BCCD078D03CBFBB113F576EA2`
- `HelloUnitTest`：
  - baseline / off 都是 `688/688 passed`

### 结论

- 这颗 child 满足：
  - 代码量收益 `>1KB`
  - 基础优先场景性能回退 `<10%`
  - hotspot 退化边界比 `EGUI_CONFIG_FONT_STD_FAST_MASK_DRAW_ENABLE` 更清楚，只集中在 gradient masked-text
- 因此：
  - 保留 `EGUI_CONFIG_FONT_STD_MASK_ROW_BLEND_FAST_PATH_ENABLE`
  - 默认仍建议保持开启，不适合默认关
  - 若项目要继续做 font 侧 size-first 裁剪，优先尝试这颗 child，再考虑直接动 `EGUI_CONFIG_FONT_STD_FAST_MASK_DRAW_ENABLE`

## 本轮新增 A/B: `std image fast draw`

### 测试环境

- 日期：`2026-04-03`
- 提交基线：`dabeeef`
- `HelloPerformance` code size A/B
  - `make -j APP=HelloPerformance PORT=qemu CPU_ARCH=cortex-m0plus APP_OBJ_SUFFIX=hpimgbase`
  - `make -j APP=HelloPerformance PORT=qemu CPU_ARCH=cortex-m0plus APP_OBJ_SUFFIX=hpimgsz USER_CFLAGS=-DEGUI_CONFIG_IMAGE_STD_FAST_DRAW_ENABLE=0`
- `HelloPerformance` perf A/B
  - `python scripts/perf_analysis/code_perf_check.py --profile cortex-m3 --clean --filter RECTANGLE,CIRCLE,ROUND_RECTANGLE,TEXT,IMAGE_565,EXTERN_IMAGE_565`
  - `python scripts/perf_analysis/code_perf_check.py --profile cortex-m3 --clean --filter RECTANGLE,CIRCLE,ROUND_RECTANGLE,TEXT,IMAGE_565,EXTERN_IMAGE_565 --extra-cflags=-DEGUI_CONFIG_IMAGE_STD_FAST_DRAW_ENABLE=0`
- 运行/单测验证
  - `python scripts/code_runtime_check.py --app HelloPerformance --timeout 10`
  - `make -j APP=HelloUnitTest PORT=pc_test APP_OBJ_SUFFIX=utimgfast`
  - `output/main.exe`

### 代码量证据

- 默认开启时：
  - 链接汇总：`text=2056196 data=72 bss=3832`
- 强制关闭 fast draw 后：
  - 链接汇总：`text=2017968 data=72 bss=3792`
- 结论：
  - `text -38228B`
  - `bss -40B`
  - 这是一块值得暴露成实验宏的大代码体积

### 性能证据

| 场景 | 默认开启 | 关闭 fast draw | 变化 |
| --- | ---: | ---: | ---: |
| `RECTANGLE` | `0.179ms` | `0.179ms` | `+0.0%` |
| `CIRCLE` | `0.810ms` | `0.810ms` | `+0.0%` |
| `ROUND_RECTANGLE` | `0.812ms` | `0.812ms` | `+0.0%` |
| `TEXT` | `1.026ms` | `1.026ms` | `+0.0%` |
| `EXTERN_TEXT` | `1.065ms` | `1.065ms` | `+0.0%` |
| `IMAGE_565` | `0.239ms` | `2.817ms` | `+1078.7%` |
| `IMAGE_565_1` | `0.418ms` | `2.753ms` | `+558.6%` |
| `IMAGE_565_8` | `0.533ms` | `2.383ms` | `+347.1%` |
| `EXTERN_IMAGE_565` | `0.417ms` | `0.089ms` | `-78.7%` |
| `EXTERN_IMAGE_565_8` | `0.704ms` | `0.089ms` | `-87.4%` |

- 可以明确看到，这个开关对 `RECTANGLE`、`CIRCLE`、`ROUND_RECTANGLE`、`TEXT` 基本路径没有副作用。
- 但它会把 internal `RGB565` 直接绘制路径从 specialized draw/resize 回退到逐像素 `get_pixel` 通路，`IMAGE_565` 系列退化达到 `3.3x ~ 11.8x`。
- `EXTERN_IMAGE_565` 当前结果反而更快，推测与 generic 路径先 resolve external resource，再命中 persistent image/cache 有关；但这不能改变 internal `IMAGE_565` 已经远超 `<10%` 阈值的事实。

### 结论

- 这条路径满足“代码量大，值得做用户宏隔离”的第一条件。
- 但它完全不满足“关闭后性能回退 `<10%`”的第二条件。
- 因此：
  - 保留 `EGUI_CONFIG_IMAGE_STD_FAST_DRAW_ENABLE`
  - 默认值必须保持开启
  - 仅作为 size-first / A/B 实验开关，不建议在 shipped `HelloPerformance` 配置中默认关闭

## 本轮继续拆分 A/B: `std image fast resize`

### 测试环境

- 日期：`2026-04-03`
- 提交基线：`dabeeef`
- `HelloPerformance` code size A/B
  - `make -j APP=HelloPerformance PORT=qemu CPU_ARCH=cortex-m0plus APP_OBJ_SUFFIX=hpresizeon`
  - `make -j APP=HelloPerformance PORT=qemu CPU_ARCH=cortex-m0plus APP_OBJ_SUFFIX=hpresizeoff USER_CFLAGS=-DEGUI_CONFIG_IMAGE_STD_FAST_RESIZE_ENABLE=0`
- `HelloPerformance` perf A/B
  - `python scripts/perf_analysis/code_perf_check.py --profile cortex-m3 --clean --filter RECTANGLE,CIRCLE,ROUND_RECTANGLE,TEXT,IMAGE_565,EXTERN_IMAGE_565,IMAGE_RESIZE_565,IMAGE_RESIZE_565_1,IMAGE_RESIZE_565_2,IMAGE_RESIZE_565_4,IMAGE_RESIZE_565_8,EXTERN_IMAGE_RESIZE_565,EXTERN_IMAGE_RESIZE_565_1,EXTERN_IMAGE_RESIZE_565_2,EXTERN_IMAGE_RESIZE_565_4,EXTERN_IMAGE_RESIZE_565_8`
  - `python scripts/perf_analysis/code_perf_check.py --profile cortex-m3 --clean --filter RECTANGLE,CIRCLE,ROUND_RECTANGLE,TEXT,IMAGE_565,EXTERN_IMAGE_565,IMAGE_RESIZE_565,IMAGE_RESIZE_565_1,IMAGE_RESIZE_565_2,IMAGE_RESIZE_565_4,IMAGE_RESIZE_565_8,EXTERN_IMAGE_RESIZE_565,EXTERN_IMAGE_RESIZE_565_1,EXTERN_IMAGE_RESIZE_565_2,EXTERN_IMAGE_RESIZE_565_4,EXTERN_IMAGE_RESIZE_565_8 --extra-cflags=-DEGUI_CONFIG_IMAGE_STD_FAST_RESIZE_ENABLE=0`
- 运行/单测验证
  - `python scripts/code_runtime_check.py --app HelloPerformance --timeout 10`
  - `make -j APP=HelloUnitTest PORT=pc_test APP_OBJ_SUFFIX=utimgresize USER_CFLAGS=-DEGUI_CONFIG_IMAGE_STD_FAST_RESIZE_ENABLE=0`
  - `output/main.exe`

### 代码量证据

- `HelloPerformance` 链接汇总：
  - 默认开启：`text=2056200 data=72 bss=3832`
  - 关闭 resize fast path：`text=2037128 data=72 bss=3832`
- 结论：
  - `text -19072B`
  - `data/bss` 不变
- 对象级证据也一致：
  - `egui_image_std.c.o 61087B -> 41967B`
  - 主要被裁掉的是 resize specialized helpers，包括：
    - `egui_image_std_set_image_resize_rgb565`
    - `egui_image_std_set_image_resize_rgb565_8_common`
    - `egui_image_std_set_image_resize_rgb565_{1,2,4}`
    - `egui_image_std_draw_image_resize_external_alpha`

### 性能证据

| 场景 | 默认开启 | 关闭 resize fast path | 变化 |
| --- | ---: | ---: | ---: |
| `RECTANGLE` | `0.179ms` | `0.179ms` | `+0.0%` |
| `CIRCLE` | `0.810ms` | `0.810ms` | `+0.0%` |
| `ROUND_RECTANGLE` | `0.812ms` | `0.812ms` | `+0.0%` |
| `TEXT` | `1.026ms` | `1.026ms` | `+0.0%` |
| `IMAGE_565` | `0.239ms` | `0.238ms` | `-0.4%` |
| `EXTERN_IMAGE_565` | `0.417ms` | `0.413ms` | `-1.0%` |
| `IMAGE_RESIZE_565` | `0.459ms` | `2.957ms` | `+544.2%` |
| `IMAGE_RESIZE_565_1` | `0.612ms` | `2.861ms` | `+367.5%` |
| `IMAGE_RESIZE_565_8` | `0.635ms` | `2.523ms` | `+297.3%` |
| `EXTERN_IMAGE_RESIZE_565` | `0.506ms` | `0.090ms` | `-82.2%` |
| `EXTERN_IMAGE_RESIZE_565_8` | `0.698ms` | `0.090ms` | `-87.1%` |

- 这说明该宏对当前优先的基础直绘路径基本无副作用。
- 但只要项目依赖 internal `RGB565` resize，关闭后就会立刻退回到 `get_pixel` generic path，回退量级是 `+297% ~ +544%`，远超 `<10%` 阈值。

### 结论

- 这条拆分满足代码量收益目标，但不满足 resize 热点的性能阈值。
- 因此：
  - 保留 `EGUI_CONFIG_IMAGE_STD_FAST_RESIZE_ENABLE`
  - 默认值仍应保持开启
  - 它适合作为比 `EGUI_CONFIG_IMAGE_STD_FAST_DRAW_ENABLE` 更局部的实验型开关，但不适合作为 shipped 默认关闭项

## 本轮继续细拆 A/B: `std image fast resize alpha`

### 测试环境

- 日期：`2026-04-03`
- 提交基线：`dabeeef`
- `HelloPerformance` code size A/B
  - `make -j APP=HelloPerformance PORT=qemu CPU_ARCH=cortex-m0plus APP_OBJ_SUFFIX=hpresizealphaon`
  - `make -j APP=HelloPerformance PORT=qemu CPU_ARCH=cortex-m0plus APP_OBJ_SUFFIX=hpresizealphaoff USER_CFLAGS=-DEGUI_CONFIG_IMAGE_STD_FAST_RESIZE_ALPHA_ENABLE=0`
- `HelloPerformance` perf A/B
  - `python scripts/perf_analysis/code_perf_check.py --profile cortex-m3 --clean --filter RECTANGLE,CIRCLE,ROUND_RECTANGLE,TEXT,IMAGE_565,EXTERN_IMAGE_565,IMAGE_RESIZE_565,EXTERN_IMAGE_RESIZE_565,IMAGE_RESIZE_565_1,IMAGE_RESIZE_565_2,IMAGE_RESIZE_565_4,IMAGE_RESIZE_565_8,EXTERN_IMAGE_RESIZE_565_1,EXTERN_IMAGE_RESIZE_565_2,EXTERN_IMAGE_RESIZE_565_4,EXTERN_IMAGE_RESIZE_565_8`
  - `python scripts/perf_analysis/code_perf_check.py --profile cortex-m3 --clean --filter RECTANGLE,CIRCLE,ROUND_RECTANGLE,TEXT,IMAGE_565,EXTERN_IMAGE_565,IMAGE_RESIZE_565,EXTERN_IMAGE_RESIZE_565,IMAGE_RESIZE_565_1,IMAGE_RESIZE_565_2,IMAGE_RESIZE_565_4,IMAGE_RESIZE_565_8,EXTERN_IMAGE_RESIZE_565_1,EXTERN_IMAGE_RESIZE_565_2,EXTERN_IMAGE_RESIZE_565_4,EXTERN_IMAGE_RESIZE_565_8 --extra-cflags=-DEGUI_CONFIG_IMAGE_STD_FAST_RESIZE_ALPHA_ENABLE=0`
- 运行/单测验证
  - `python scripts/code_runtime_check.py --app HelloPerformance --timeout 10`
  - `make -j APP=HelloUnitTest PORT=pc_test APP_OBJ_SUFFIX=utimgresizealpha USER_CFLAGS=-DEGUI_CONFIG_IMAGE_STD_FAST_RESIZE_ALPHA_ENABLE=0`
  - `output/main.exe`

### 代码量证据

- `HelloPerformance` 链接汇总：
  - 默认开启：`text=2056200 data=72 bss=3832`
  - 关闭 alpha resize fast path：`text=2044948 data=72 bss=3832`
- 结论：
  - `text -11252B`
  - `data/bss` 不变
- 对象级证据：
  - `egui_image_std.c.o 61087B -> 49787B`
  - 主要回收的是 alpha resize specialized helpers，包括：
    - `egui_image_std_set_image_resize_rgb565_8_common`
    - `egui_image_std_set_image_resize_rgb565_{1,2,4}`
    - `egui_image_std_draw_image_resize_external_alpha`

### 性能证据

| 场景 | 默认开启 | 关闭 alpha resize fast path | 变化 |
| --- | ---: | ---: | ---: |
| `RECTANGLE` | `0.179ms` | `0.179ms` | `+0.0%` |
| `CIRCLE` | `0.810ms` | `0.810ms` | `+0.0%` |
| `ROUND_RECTANGLE` | `0.812ms` | `0.812ms` | `+0.0%` |
| `TEXT` | `1.026ms` | `1.026ms` | `+0.0%` |
| `IMAGE_565` | `0.239ms` | `0.238ms` | `-0.4%` |
| `EXTERN_IMAGE_565` | `0.417ms` | `0.413ms` | `-1.0%` |
| `IMAGE_RESIZE_565` | `0.459ms` | `0.458ms` | `-0.2%` |
| `EXTERN_IMAGE_RESIZE_565` | `0.506ms` | `0.507ms` | `+0.2%` |
| `IMAGE_RESIZE_565_1` | `0.612ms` | `2.876ms` | `+369.9%` |
| `IMAGE_RESIZE_565_4` | `0.788ms` | `3.101ms` | `+293.5%` |
| `IMAGE_RESIZE_565_8` | `0.635ms` | `2.540ms` | `+300.0%` |
| `EXTERN_IMAGE_RESIZE_565_1` | `0.678ms` | `0.108ms` | `-84.1%` |
| `EXTERN_IMAGE_RESIZE_565_8` | `0.698ms` | `0.110ms` | `-84.2%` |

- 这说明更细的 alpha 宏已经把 raw `RGB565` resize 主路径保住了：
  - `IMAGE_RESIZE_565 -0.2%`
  - `EXTERN_IMAGE_RESIZE_565 +0.2%`
- 但 alpha resize 系列仍然会明显回退，说明这次回收的代码确实集中在 `RGB565_1/2/4/8` 的 specialized path。

### 结论

- 这条拆分已经满足当前“基础 raw `RGB565` 直绘/缩放路径不退化”的目标。
- 但它不适用于依赖 alpha resize 热点的项目。
- 因此：
  - 保留 `EGUI_CONFIG_IMAGE_STD_FAST_RESIZE_ALPHA_ENABLE`
  - 默认值仍应保持开启
  - 它比整条 `std image fast resize` 更适合作为 size-first 实验型开关，前提是项目确认不会把 `RGB565_1/2/4/8` resize 当成热点

## 本轮继续细拆 A/B: `std image fast draw alpha`

### 测试环境

- 日期：`2026-04-03`
- 提交基线：`dabeeef`
- `HelloPerformance` code size A/B
  - `make -j APP=HelloPerformance PORT=qemu CPU_ARCH=cortex-m0plus APP_OBJ_SUFFIX=hpimgdrawalphaon`
  - `make -j APP=HelloPerformance PORT=qemu CPU_ARCH=cortex-m0plus APP_OBJ_SUFFIX=hpimgdrawalphaoff USER_CFLAGS=-DEGUI_CONFIG_IMAGE_STD_FAST_DRAW_ALPHA_ENABLE=0`
- `HelloPerformance` perf A/B
  - `python scripts/perf_analysis/code_perf_check.py --profile cortex-m3 --clean --filter RECTANGLE,CIRCLE,ROUND_RECTANGLE,TEXT,EXTERN_TEXT,IMAGE_565,IMAGE_565_1,IMAGE_565_2,IMAGE_565_4,IMAGE_565_8,EXTERN_IMAGE_565,EXTERN_IMAGE_565_1,EXTERN_IMAGE_565_2,EXTERN_IMAGE_565_4,EXTERN_IMAGE_565_8`
  - `python scripts/perf_analysis/code_perf_check.py --profile cortex-m3 --clean --filter RECTANGLE,CIRCLE,ROUND_RECTANGLE,TEXT,EXTERN_TEXT,IMAGE_565,IMAGE_565_1,IMAGE_565_2,IMAGE_565_4,IMAGE_565_8,EXTERN_IMAGE_565,EXTERN_IMAGE_565_1,EXTERN_IMAGE_565_2,EXTERN_IMAGE_565_4,EXTERN_IMAGE_565_8 --extra-cflags=-DEGUI_CONFIG_IMAGE_STD_FAST_DRAW_ALPHA_ENABLE=0`
- 运行/单测验证
  - `python scripts/code_runtime_check.py --app HelloPerformance --timeout 10`
  - `USER_CFLAGS=-DEGUI_CONFIG_IMAGE_STD_FAST_DRAW_ALPHA_ENABLE=0 python scripts/code_runtime_check.py --app HelloPerformance --timeout 10`
  - `make -j APP=HelloUnitTest PORT=pc_test APP_OBJ_SUFFIX=utimgdrawalpha USER_CFLAGS=-DEGUI_CONFIG_IMAGE_STD_FAST_DRAW_ALPHA_ENABLE=0`
  - `output/main.exe`

### 代码量证据

- `HelloPerformance` 链接汇总：
  - 默认开启：`text=2056200 data=72 bss=3832`
  - 关闭 alpha direct-draw fast path：`text=2048184 data=72 bss=3832`
- 结论：
  - `text -8016B`
  - `data/bss` 不变
- 对象级证据：
  - `output/obj/hpimgdrawalphaon_qemu/src/image/egui_image_std.c.o = 691788B`
  - `output/obj/hpimgdrawalphaoff_qemu/src/image/egui_image_std.c.o = 604520B`
  - 主要回收的是 `RGB565_1/2/4/8` 的 direct-draw specialized path，包括：
    - `egui_image_std_set_image_rgb565_8`
    - `egui_image_std_set_image_rgb565_{1,2,4}`
    - 以及只服务这几条直绘路径的局部 blend helpers

### 性能证据

| 场景 | 默认开启 | 关闭 alpha direct-draw fast path | 变化 |
| --- | ---: | ---: | ---: |
| `RECTANGLE` | `0.179ms` | `0.179ms` | `+0.0%` |
| `CIRCLE` | `0.810ms` | `0.810ms` | `+0.0%` |
| `ROUND_RECTANGLE` | `0.812ms` | `0.812ms` | `+0.0%` |
| `TEXT` | `1.026ms` | `1.026ms` | `+0.0%` |
| `EXTERN_TEXT` | `1.065ms` | `1.065ms` | `+0.0%` |
| `IMAGE_565` | `0.239ms` | `0.239ms` | `+0.0%` |
| `EXTERN_IMAGE_565` | `0.417ms` | `0.415ms` | `-0.5%` |
| `IMAGE_565_1` | `0.418ms` | `2.757ms` | `+559.6%` |
| `IMAGE_565_2` | `0.510ms` | `2.943ms` | `+477.1%` |
| `IMAGE_565_4` | `0.693ms` | `2.958ms` | `+326.8%` |
| `IMAGE_565_8` | `0.533ms` | `2.390ms` | `+348.4%` |
| `EXTERN_IMAGE_565_1` | `0.587ms` | `0.096ms` | `-83.6%` |
| `EXTERN_IMAGE_565_2` | `0.680ms` | `0.096ms` | `-85.9%` |
| `EXTERN_IMAGE_565_4` | `0.862ms` | `0.096ms` | `-88.9%` |
| `EXTERN_IMAGE_565_8` | `0.704ms` | `0.097ms` | `-86.2%` |

- 这说明 direct-draw alpha 的 specialized 分支已经被独立拆开：
  - 基础优先的 `IMAGE_565` / `EXTERN_IMAGE_565` 主路径保持不变
  - 回退集中在 `RGB565_1/2/4/8` 的 direct draw
- `EXTERN_IMAGE_565_1/2/4/8` 当前结果反而更快，和此前 resize / fast draw A/B 一样，说明 generic path 在 external resource 上会先 resolve persistent image，再走统一 `get_pixel` 通路。

### 结论

- 这条拆分满足当前优先目标：
  - 代码量收益 `>1KB`
  - 基础优先场景 `RECTANGLE / CIRCLE / ROUND_RECTANGLE / TEXT / IMAGE_565 / EXTERN_IMAGE_565` 性能回退 `<10%`
- 因此：
  - 保留 `EGUI_CONFIG_IMAGE_STD_FAST_DRAW_ALPHA_ENABLE`
  - 默认值仍应保持开启
  - 它比整条 `EGUI_CONFIG_IMAGE_STD_FAST_DRAW_ENABLE` 更适合作为 size-first 实验型开关，前提是项目确认不会把 `RGB565_1/2/4/8` direct draw 当成热点

## 本轮继续细拆 A/B: `std image alpha8 direct-draw fast path`

### 测试环境

- 日期：`2026-04-03`
- 提交基线：`dabeeef`
- `HelloPerformance` code size A/B
  - `make -B -j APP=HelloPerformance PORT=qemu CPU_ARCH=cortex-m0plus APP_OBJ_SUFFIX=hpalpha8drawon`
  - `make -B -j APP=HelloPerformance PORT=qemu CPU_ARCH=cortex-m0plus APP_OBJ_SUFFIX=hpalpha8drawoff USER_CFLAGS=-DEGUI_CONFIG_IMAGE_STD_FAST_DRAW_ALPHA8_ENABLE=0`
- `HelloPerformance` perf A/B
  - `python scripts/perf_analysis/code_perf_check.py --profile cortex-m3 --threshold 1000 --timeout 900 --clean --filter RECTANGLE,CIRCLE,ROUND_RECTANGLE,TEXT,IMAGE_565,EXTERN_IMAGE_565,IMAGE_565_8,EXTERN_IMAGE_565_8,IMAGE_565_1,IMAGE_565_2,IMAGE_565_4`
  - `python scripts/perf_analysis/code_perf_check.py --profile cortex-m3 --threshold 1000 --timeout 900 --clean --filter RECTANGLE,CIRCLE,ROUND_RECTANGLE,TEXT,IMAGE_565,EXTERN_IMAGE_565,IMAGE_565_8,EXTERN_IMAGE_565_8,IMAGE_565_1,IMAGE_565_2,IMAGE_565_4 --extra-cflags=-DEGUI_CONFIG_IMAGE_STD_FAST_DRAW_ALPHA8_ENABLE=0`
- 运行/单测验证
  - `python scripts/code_runtime_check.py --app HelloPerformance --timeout 10 --keep-screenshots`
  - `USER_CFLAGS=-DEGUI_CONFIG_IMAGE_STD_FAST_DRAW_ALPHA8_ENABLE=0 python scripts/code_runtime_check.py --app HelloPerformance --timeout 10 --keep-screenshots`
  - `make -j APP=HelloUnitTest PORT=pc_test APP_OBJ_SUFFIX=utalpha8drawon`
  - `make -j APP=HelloUnitTest PORT=pc_test APP_OBJ_SUFFIX=utalpha8drawoff USER_CFLAGS=-DEGUI_CONFIG_IMAGE_STD_FAST_DRAW_ALPHA8_ENABLE=0`
  - `output/main.exe`

### 代码量证据

- `HelloPerformance` 链接汇总：
  - 默认开启：`text=2055224 data=72 bss=3832`
  - 关闭 alpha8 direct-draw fast path：`text=2053476 data=72 bss=3832`
- 结论：
  - `text -1748B`
  - `data/bss` 不变
  - 被回收的主要是 `RGB565_8` direct-draw specialized helpers

### 性能证据

| 场景 | 默认开启 | 关闭 alpha8 direct-draw fast path | 变化 |
| --- | ---: | ---: | ---: |
| `RECTANGLE` | `0.179ms` | `0.179ms` | `+0.0%` |
| `CIRCLE` | `0.810ms` | `0.810ms` | `+0.0%` |
| `ROUND_RECTANGLE` | `0.812ms` | `0.812ms` | `+0.0%` |
| `TEXT` | `1.027ms` | `1.027ms` | `+0.0%` |
| `IMAGE_565` | `0.239ms` | `0.239ms` | `+0.0%` |
| `EXTERN_IMAGE_565` | `0.417ms` | `0.417ms` | `+0.0%` |
| `IMAGE_565_8` | `0.533ms` | `2.390ms` | `+348.4%` |
| `EXTERN_IMAGE_565_8` | `0.704ms` | `0.097ms` | `-86.2%` |
| `IMAGE_565_1` | `0.418ms` | `0.418ms` | `+0.0%` |
| `IMAGE_565_2` | `0.511ms` | `0.511ms` | `+0.0%` |
| `IMAGE_565_4` | `0.707ms` | `0.707ms` | `+0.0%` |

- 这说明：
  - 基础优先保留的 `RECTANGLE / CIRCLE / ROUND_RECTANGLE / TEXT / IMAGE_565 / EXTERN_IMAGE_565` 完全不动
  - 代价集中在 internal `RGB565_8 direct draw`
  - `RGB565_1/2/4` direct draw 不受影响

### 运行/单测

- `HelloPerformance` runtime
  - 默认开启：`241 frames`，`ALL PASSED`
  - 宏关闭：`241 frames`，`ALL PASSED`
- `HelloUnitTest`
  - 默认开启：`688/688 passed`
  - 宏关闭：`688/688 passed`

### 结论

- 这颗宏满足：
  - 代码量收益 `>1KB`
  - 基础 `RECTANGLE / CIRCLE / ROUND_RECTANGLE / TEXT / IMAGE_565 / EXTERN_IMAGE_565` 不退化
- 因此：
  - 保留 `EGUI_CONFIG_IMAGE_STD_FAST_DRAW_ALPHA8_ENABLE`
  - 默认值仍应保持开启
  - 它比 `EGUI_CONFIG_IMAGE_STD_FAST_DRAW_ALPHA_ENABLE` 更适合作为“只牺牲 internal RGB565_8 direct draw”的 size-first 实验开关

## 本轮继续细拆 A/B: `std image packed-alpha direct-draw fast path`

### 测试环境

- 日期：`2026-04-03`
- 提交基线：`dabeeef`
- `HelloPerformance` code size A/B
  - `make -B -j APP=HelloPerformance PORT=qemu CPU_ARCH=cortex-m0plus APP_OBJ_SUFFIX=hppackeddrawon`
  - `make -B -j APP=HelloPerformance PORT=qemu CPU_ARCH=cortex-m0plus APP_OBJ_SUFFIX=hppackeddrawoff USER_CFLAGS=-DEGUI_CONFIG_IMAGE_STD_FAST_DRAW_PACKED_ALPHA_ENABLE=0`
- `HelloPerformance` perf A/B
  - `python scripts/perf_analysis/code_perf_check.py --profile cortex-m3 --threshold 1000 --timeout 900 --clean --filter RECTANGLE,CIRCLE,ROUND_RECTANGLE,TEXT,IMAGE_565,EXTERN_IMAGE_565,IMAGE_565_1,IMAGE_565_2,IMAGE_565_4,IMAGE_565_8,EXTERN_IMAGE_565_1,EXTERN_IMAGE_565_2,EXTERN_IMAGE_565_4,EXTERN_IMAGE_565_8`
  - `python scripts/perf_analysis/code_perf_check.py --profile cortex-m3 --threshold 1000 --timeout 900 --clean --filter RECTANGLE,CIRCLE,ROUND_RECTANGLE,TEXT,IMAGE_565,EXTERN_IMAGE_565,IMAGE_565_1,IMAGE_565_2,IMAGE_565_4,IMAGE_565_8,EXTERN_IMAGE_565_1,EXTERN_IMAGE_565_2,EXTERN_IMAGE_565_4,EXTERN_IMAGE_565_8 --extra-cflags=-DEGUI_CONFIG_IMAGE_STD_FAST_DRAW_PACKED_ALPHA_ENABLE=0`
- 运行/单测验证
  - `python scripts/code_runtime_check.py --app HelloPerformance --timeout 10 --keep-screenshots`
  - `USER_CFLAGS=-DEGUI_CONFIG_IMAGE_STD_FAST_DRAW_PACKED_ALPHA_ENABLE=0 python scripts/code_runtime_check.py --app HelloPerformance --timeout 10 --keep-screenshots`
  - `make -j APP=HelloUnitTest PORT=pc_test APP_OBJ_SUFFIX=utpackeddrawon`
  - `make -j APP=HelloUnitTest PORT=pc_test APP_OBJ_SUFFIX=utpackeddrawoff USER_CFLAGS=-DEGUI_CONFIG_IMAGE_STD_FAST_DRAW_PACKED_ALPHA_ENABLE=0`
  - `output/main.exe`

### 代码量证据

- `HelloPerformance` 链接汇总：
  - 默认开启：`text=2055224 data=72 bss=3832`
  - 关闭 packed-alpha direct-draw fast path：`text=2049676 data=72 bss=3832`
- 结论：
  - `text -5548B`
  - `data/bss` 不变
  - 被回收的主要是 internal `RGB565_1/2/4` direct-draw specialized helpers

### 性能证据

| 场景 | 默认开启 | 关闭 packed-alpha direct-draw fast path | 变化 |
| --- | ---: | ---: | ---: |
| `RECTANGLE` | `0.179ms` | `0.179ms` | `+0.0%` |
| `CIRCLE` | `0.810ms` | `0.810ms` | `+0.0%` |
| `ROUND_RECTANGLE` | `0.812ms` | `0.812ms` | `+0.0%` |
| `TEXT` | `1.027ms` | `1.027ms` | `+0.0%` |
| `IMAGE_565` | `0.239ms` | `0.239ms` | `+0.0%` |
| `EXTERN_IMAGE_565` | `0.417ms` | `0.415ms` | `-0.5%` |
| `IMAGE_565_1` | `0.418ms` | `2.757ms` | `+559.6%` |
| `IMAGE_565_2` | `0.511ms` | `2.943ms` | `+475.9%` |
| `IMAGE_565_4` | `0.707ms` | `2.958ms` | `+318.4%` |
| `IMAGE_565_8` | `0.533ms` | `0.531ms` | `-0.4%` |
| `EXTERN_IMAGE_565_1` | `0.587ms` | `0.096ms` | `-83.6%` |
| `EXTERN_IMAGE_565_2` | `0.680ms` | `0.096ms` | `-85.9%` |
| `EXTERN_IMAGE_565_4` | `0.876ms` | `0.097ms` | `-88.9%` |
| `EXTERN_IMAGE_565_8` | `0.704ms` | `0.702ms` | `-0.3%` |

- 这说明这轮拆掉的主要是 internal packed-alpha direct draw：
  - 基础优先保留的 `RECTANGLE / CIRCLE / ROUND_RECTANGLE / TEXT / IMAGE_565 / EXTERN_IMAGE_565` 全部在噪声范围
  - `RGB565_8` direct draw 也几乎不动
  - 明显代价集中在 internal `RGB565_1/2/4`

### 运行/单测

- `HelloPerformance` runtime
  - 默认开启：`241 frames`，`ALL PASSED`
  - 宏关闭：`241 frames`，`ALL PASSED`
- `HelloUnitTest`
  - 默认开启：`688/688 passed`
  - 宏关闭：`688/688 passed`

### 结论

- 这颗宏满足：
  - 代码量收益 `>1KB`
  - 基础 `RECTANGLE / CIRCLE / ROUND_RECTANGLE / TEXT / IMAGE_565 / EXTERN_IMAGE_565` 不退化
  - `RGB565_8` direct draw 也不退化
- 因此：
  - 保留 `EGUI_CONFIG_IMAGE_STD_FAST_DRAW_PACKED_ALPHA_ENABLE`
  - 默认值仍应保持开启
  - 它比 `EGUI_CONFIG_IMAGE_STD_FAST_DRAW_ALPHA_ENABLE` 更适合作为“只牺牲 internal RGB565_1/2/4 direct draw”的 size-first 实验开关

## 本轮继续细拆 A/B: `std image opaque-source promotion`

### 测试环境

- 日期：`2026-04-03`
- 提交基线：`dabeeef`
- `HelloPerformance` code size A/B
  - `make -B -j APP=HelloPerformance PORT=qemu CPU_ARCH=cortex-m0plus APP_OBJ_SUFFIX=hpopaqueon`
  - `make -B -j APP=HelloPerformance PORT=qemu CPU_ARCH=cortex-m0plus APP_OBJ_SUFFIX=hpopaqueoff USER_CFLAGS=-DEGUI_CONFIG_IMAGE_STD_RGB565_OPAQUE_SOURCE_CHECK_ENABLE=0`
- `HelloPerformance` perf A/B
  - `python scripts/perf_analysis/code_perf_check.py --profile cortex-m3 --clean --filter RECTANGLE,CIRCLE,ROUND_RECTANGLE,TEXT,IMAGE_565,IMAGE_565_1,IMAGE_565_2,IMAGE_565_4,IMAGE_565_8,EXTERN_IMAGE_565,EXTERN_IMAGE_565_1,EXTERN_IMAGE_565_2,EXTERN_IMAGE_565_4,EXTERN_IMAGE_565_8,IMAGE_RESIZE_565,IMAGE_RESIZE_565_1,IMAGE_RESIZE_565_2,IMAGE_RESIZE_565_4,IMAGE_RESIZE_565_8,EXTERN_IMAGE_RESIZE_565,EXTERN_IMAGE_RESIZE_565_1,EXTERN_IMAGE_RESIZE_565_2,EXTERN_IMAGE_RESIZE_565_4,EXTERN_IMAGE_RESIZE_565_8`
  - `python scripts/perf_analysis/code_perf_check.py --profile cortex-m3 --clean --filter RECTANGLE,CIRCLE,ROUND_RECTANGLE,TEXT,IMAGE_565,IMAGE_565_1,IMAGE_565_2,IMAGE_565_4,IMAGE_565_8,EXTERN_IMAGE_565,EXTERN_IMAGE_565_1,EXTERN_IMAGE_565_2,EXTERN_IMAGE_565_4,EXTERN_IMAGE_565_8,IMAGE_RESIZE_565,IMAGE_RESIZE_565_1,IMAGE_RESIZE_565_2,IMAGE_RESIZE_565_4,IMAGE_RESIZE_565_8,EXTERN_IMAGE_RESIZE_565,EXTERN_IMAGE_RESIZE_565_1,EXTERN_IMAGE_RESIZE_565_2,EXTERN_IMAGE_RESIZE_565_4,EXTERN_IMAGE_RESIZE_565_8 --extra-cflags=-DEGUI_CONFIG_IMAGE_STD_RGB565_OPAQUE_SOURCE_CHECK_ENABLE=0`
- 运行/单测验证
  - `python scripts/code_runtime_check.py --app HelloPerformance --timeout 10 --keep-screenshots`
  - `USER_CFLAGS=-DEGUI_CONFIG_IMAGE_STD_RGB565_OPAQUE_SOURCE_CHECK_ENABLE=0 python scripts/code_runtime_check.py --app HelloPerformance --timeout 10 --keep-screenshots`
  - `make -j APP=HelloUnitTest PORT=pc_test APP_OBJ_SUFFIX=utopaque USER_CFLAGS=-DEGUI_CONFIG_IMAGE_STD_RGB565_OPAQUE_SOURCE_CHECK_ENABLE=0`
  - `output/main.exe`

### 代码量证据

- `HelloPerformance` 链接汇总：
  - 默认开启：`text=2056208 data=72 bss=3832`
  - 关闭 opaque-source promotion：`text=2054952 data=72 bss=3792`
- 结论：
  - `text -1256B`
  - `bss -40B`
- 对象级证据：
  - `egui_image_std.c.o` 可链接段合计 `61182B -> 59893B`
  - 被裁掉的是 `RGB565` “alpha 存在但全不透明”检测与缓存/提升逻辑，包括：
    - `egui_image_std_rgb565_alpha_is_all_opaque`
    - `egui_image_std_rgb565_external_alpha_is_all_opaque`
    - `egui_image_std_rgb565_can_use_opaque_draw_fast_path`
    - `egui_image_std_rgb565_can_use_opaque_resize_fast_path`
    - `egui_image_std_rgb565_is_opaque_source.part.0`
    - `g_egui_image_std_alpha_opaque_cache*`
- 这轮 A/B 需要注意：
  - `APP_OBJ_SUFFIX` 只隔离对象目录，不隔离最终 `output/main.elf` / `output/main.map`
  - 为避免被旧产物误导，必须 `make -B` 强制重链，并把两份 `main.elf/main.map` 分开保存

### 性能证据

| 场景 | 默认开启 | 关闭 opaque-source promotion | 变化 |
| --- | ---: | ---: | ---: |
| `RECTANGLE` | `0.179ms` | `0.179ms` | `+0.0%` |
| `CIRCLE` | `0.810ms` | `0.810ms` | `+0.0%` |
| `ROUND_RECTANGLE` | `0.812ms` | `0.812ms` | `+0.0%` |
| `TEXT` | `1.026ms` | `1.026ms` | `+0.0%` |
| `IMAGE_565` | `0.239ms` | `0.238ms` | `-0.4%` |
| `EXTERN_IMAGE_565` | `0.417ms` | `0.416ms` | `-0.2%` |
| `IMAGE_565_1` | `0.418ms` | `0.416ms` | `-0.5%` |
| `IMAGE_565_4` | `0.693ms` | `0.703ms` | `+1.4%` |
| `IMAGE_RESIZE_565` | `0.459ms` | `0.459ms` | `+0.0%` |
| `IMAGE_RESIZE_565_1` | `0.612ms` | `0.610ms` | `-0.3%` |
| `EXTERN_IMAGE_RESIZE_565` | `0.506ms` | `0.506ms` | `+0.0%` |
| `EXTERN_IMAGE_RESIZE_565_4` | `0.854ms` | `0.850ms` | `-0.5%` |

- 当前 `HelloPerformance` 基础优先路径没有可见退化。
- `RGB565_1/2/4/8` 直绘/缩放场景也基本都在测量噪声范围内，说明现有 perf 资源并不依赖这条 opaque-source promotion。
- 关闭后真正失去的是：
  - circle / round-rectangle masked draw/resize 分支里的 opaque-source promotion
  - 以及这条优化所需的 alpha opaque cache 元数据

### 运行/截图/单测

- `HelloPerformance` runtime：
  - 默认开启：`241 frames`，`ALL PASSED`
  - 宏关闭：`241 frames`，`ALL PASSED`
- 截图抽查：
  - `frame_0000.png`：对角绿色矩形正常
  - `frame_0080.png`：星形图片正常
  - `frame_0200.png`：平铺图片正常
- `HelloUnitTest`：
  - `688/688 passed`

### 结论

- 这条拆分满足当前优先目标：
  - 代码量收益 `>1KB`
  - `RECTANGLE / CIRCLE / ROUND_RECTANGLE / TEXT / IMAGE_565 / EXTERN_IMAGE_565` 性能回退 `<10%`
- 因此：
  - 保留 `EGUI_CONFIG_IMAGE_STD_RGB565_OPAQUE_SOURCE_CHECK_ENABLE`
  - 默认值仍应保持开启
  - 它适合作为比 `FAST_DRAW_ALPHA` / `FAST_RESIZE_ALPHA` 更保守的 size-first 实验开关
  - 仅在项目确认不会依赖“alpha 源图全不透明时自动回推 raw RGB565 fast path”时再关闭

## 本轮继续细拆 A/B: `std image row overlay fast path`

### 测试环境

- 日期：`2026-04-03`
- 提交基线：`dabeeef`
- `HelloPerformance` code size A/B
  - `make -B -j APP=HelloPerformance PORT=qemu CPU_ARCH=cortex-m0plus APP_OBJ_SUFFIX=hpoverlayon`
  - `make -B -j APP=HelloPerformance PORT=qemu CPU_ARCH=cortex-m0plus APP_OBJ_SUFFIX=hpoverlayoff USER_CFLAGS=-DEGUI_CONFIG_IMAGE_STD_ROW_OVERLAY_FAST_PATH_ENABLE=0`
- `HelloPerformance` perf A/B
  - `python scripts/perf_analysis/code_perf_check.py --profile cortex-m3 --clean --filter RECTANGLE,CIRCLE,ROUND_RECTANGLE,TEXT,IMAGE_565,IMAGE_565_1,IMAGE_565_8,EXTERN_IMAGE_565,EXTERN_IMAGE_565_1,EXTERN_IMAGE_565_8,IMAGE_RESIZE_565,IMAGE_RESIZE_565_1,IMAGE_RESIZE_565_8,EXTERN_IMAGE_RESIZE_565,EXTERN_IMAGE_RESIZE_565_1,EXTERN_IMAGE_RESIZE_565_8,TEXT_GRADIENT,TEXT_RECT_GRADIENT,TEXT_ROTATE_GRADIENT,IMAGE_GRADIENT_OVERLAY,MASK_GRADIENT_IMAGE,MASK_GRADIENT_IMAGE_ROTATE`
  - `python scripts/perf_analysis/code_perf_check.py --profile cortex-m3 --clean --filter RECTANGLE,CIRCLE,ROUND_RECTANGLE,TEXT,IMAGE_565,IMAGE_565_1,IMAGE_565_8,EXTERN_IMAGE_565,EXTERN_IMAGE_565_1,EXTERN_IMAGE_565_8,IMAGE_RESIZE_565,IMAGE_RESIZE_565_1,IMAGE_RESIZE_565_8,EXTERN_IMAGE_RESIZE_565,EXTERN_IMAGE_RESIZE_565_1,EXTERN_IMAGE_RESIZE_565_8,TEXT_GRADIENT,TEXT_RECT_GRADIENT,TEXT_ROTATE_GRADIENT,IMAGE_GRADIENT_OVERLAY,MASK_GRADIENT_IMAGE,MASK_GRADIENT_IMAGE_ROTATE --extra-cflags=-DEGUI_CONFIG_IMAGE_STD_ROW_OVERLAY_FAST_PATH_ENABLE=0`
- 运行/单测验证
  - `python scripts/code_runtime_check.py --app HelloPerformance --timeout 10 --keep-screenshots`
  - `USER_CFLAGS=-DEGUI_CONFIG_IMAGE_STD_ROW_OVERLAY_FAST_PATH_ENABLE=0 python scripts/code_runtime_check.py --app HelloPerformance --timeout 10 --keep-screenshots`
  - `make -j APP=HelloUnitTest PORT=pc_test APP_OBJ_SUFFIX=utoverlay USER_CFLAGS=-DEGUI_CONFIG_IMAGE_STD_ROW_OVERLAY_FAST_PATH_ENABLE=0`
  - `output/main.exe`

### 代码量证据

- `HelloPerformance` 链接汇总：
  - 默认开启：`text=2056204 data=72 bss=3832`
  - 关闭 row-overlay fast path：`text=2053460 data=72 bss=3832`
- 结论：
  - `text -2744B`
  - `data/bss` 不变
- 对象级证据：
  - `egui_image_std.c.o` 可链接段合计 `61178B -> 58434B`
  - 被裁掉的是 `mask_get_row_overlay()` 相关的 std-image specialized 分支，包括：
    - generic draw / resize 宏里的 row-overlay 分支
    - raw `RGB565` direct draw / resize 的 row-overlay fast path
    - `RGB565_8` alpha direct draw / resize 的 row-overlay fast path

### 性能证据

| 场景 | 默认开启 | 关闭 row-overlay fast path | 变化 |
| --- | ---: | ---: | ---: |
| `RECTANGLE` | `0.179ms` | `0.179ms` | `+0.0%` |
| `CIRCLE` | `0.810ms` | `0.810ms` | `+0.0%` |
| `ROUND_RECTANGLE` | `0.812ms` | `0.812ms` | `+0.0%` |
| `TEXT` | `1.026ms` | `1.026ms` | `+0.0%` |
| `IMAGE_565` | `0.239ms` | `0.238ms` | `-0.4%` |
| `EXTERN_IMAGE_565` | `0.417ms` | `0.415ms` | `-0.5%` |
| `IMAGE_RESIZE_565` | `0.459ms` | `0.459ms` | `+0.0%` |
| `EXTERN_IMAGE_RESIZE_565` | `0.506ms` | `0.507ms` | `+0.2%` |
| `TEXT_GRADIENT` | `0.233ms` | `0.233ms` | `+0.0%` |
| `TEXT_RECT_GRADIENT` | `0.761ms` | `0.760ms` | `-0.1%` |
| `MASK_GRADIENT_IMAGE` | `1.344ms` | `1.344ms` | `+0.0%` |
| `MASK_GRADIENT_IMAGE_ROTATE` | `1.863ms` | `1.863ms` | `+0.0%` |
| `IMAGE_GRADIENT_OVERLAY` | `0.910ms` | `5.745ms` | `+531.3%` |

- 这说明该宏确实只是在回收 gradient image overlay 专用快路径：
  - 当前优先基础路径没有可见回退
  - 连带的 text-gradient / gradient-mask image 场景也没有明显变化
  - 真正失去的是 `egui_canvas_draw_image_gradient_overlay()` 这类 row-uniform overlay 优化

### 运行/截图/单测

- `HelloPerformance` runtime：
  - 默认开启：`241 frames`，`ALL PASSED`
  - 宏关闭：`241 frames`，`ALL PASSED`
- 截图抽查：
  - `frame_0000.png`：对角绿色矩形正常
  - `frame_0120.png`：平铺星形图片正常
- `HelloUnitTest`：
  - `688/688 passed`

### 结论

- 这条拆分满足当前优先目标：
  - 代码量收益 `>1KB`
  - `RECTANGLE / CIRCLE / ROUND_RECTANGLE / TEXT / IMAGE_565 / EXTERN_IMAGE_565` 性能回退 `<10%`
- 但它不适用于把 `IMAGE_GRADIENT_OVERLAY` 当热点的项目。
- 因此：
  - 保留 `EGUI_CONFIG_IMAGE_STD_ROW_OVERLAY_FAST_PATH_ENABLE`
  - 默认值仍应保持开启
  - 它适合作为 gradient-image overlay 明确不是热点时的 size-first 实验开关

## 本轮继续细拆 A/B: `std image row-overlay non-rgb565 fast path`

### 测试环境

- 日期：`2026-04-04`
- 提交基线：`f484133`（基于当前 working tree 继续）
- `HelloPerformance` code size A/B
  - `make -B -j APP=HelloPerformance PORT=qemu CPU_ARCH=cortex-m0plus`
  - `make -B -j APP=HelloPerformance PORT=qemu CPU_ARCH=cortex-m0plus USER_CFLAGS=-DEGUI_CONFIG_IMAGE_STD_ROW_OVERLAY_NON_RGB565_FAST_PATH_ENABLE=0`
- `HelloPerformance` perf A/B
  - `python scripts/perf_analysis/code_perf_check.py --profile cortex-m3 --clean --filter RECTANGLE,CIRCLE,ROUND_RECTANGLE,TEXT,IMAGE_565,IMAGE_565_1,IMAGE_565_8,EXTERN_IMAGE_565,EXTERN_IMAGE_565_1,EXTERN_IMAGE_565_8,IMAGE_RESIZE_565,IMAGE_RESIZE_565_1,IMAGE_RESIZE_565_8,EXTERN_IMAGE_RESIZE_565,EXTERN_IMAGE_RESIZE_565_1,EXTERN_IMAGE_RESIZE_565_8,TEXT_GRADIENT,TEXT_RECT_GRADIENT,TEXT_ROTATE_GRADIENT,IMAGE_GRADIENT_OVERLAY,MASK_GRADIENT_IMAGE,MASK_GRADIENT_IMAGE_ROTATE`
  - `python scripts/perf_analysis/code_perf_check.py --profile cortex-m3 --clean --filter RECTANGLE,CIRCLE,ROUND_RECTANGLE,TEXT,IMAGE_565,IMAGE_565_1,IMAGE_565_8,EXTERN_IMAGE_565,EXTERN_IMAGE_565_1,EXTERN_IMAGE_565_8,IMAGE_RESIZE_565,IMAGE_RESIZE_565_1,IMAGE_RESIZE_565_8,EXTERN_IMAGE_RESIZE_565,EXTERN_IMAGE_RESIZE_565_1,EXTERN_IMAGE_RESIZE_565_8,TEXT_GRADIENT,TEXT_RECT_GRADIENT,TEXT_ROTATE_GRADIENT,IMAGE_GRADIENT_OVERLAY,MASK_GRADIENT_IMAGE,MASK_GRADIENT_IMAGE_ROTATE --extra-cflags=-DEGUI_CONFIG_IMAGE_STD_ROW_OVERLAY_NON_RGB565_FAST_PATH_ENABLE=0`
- 运行/单测验证
  - baseline / off runtime 使用 `scripts.code_runtime_check.compile_app()` + `run_app()` 分别录制到 `runtime_check_output/HelloPerformance/rowoverlaymask_base` / `rowoverlaymask_off`
  - `make -j APP=HelloUnitTest PORT=pc_test APP_OBJ_SUFFIX=utrowoverlaymaskbase`
  - `make -j APP=HelloUnitTest PORT=pc_test APP_OBJ_SUFFIX=utrowoverlaymaskoff USER_CFLAGS=-DEGUI_CONFIG_IMAGE_STD_ROW_OVERLAY_NON_RGB565_FAST_PATH_ENABLE=0`
  - `output/main.exe`

### 代码量证据

- `HelloPerformance` 最终链接汇总：
  - baseline：`text=2056264 data=72 bss=3832`
  - 关闭 non-rgb565 row-overlay fast path：`text=2054788 data=72 bss=3832`
- 结论：
  - `text -1476B`
  - `data/bss` 不变
- 从当前实现归属看，这颗 child 只裁掉 non-raw RGB565 的 row-overlay 分支：
  - generic `get_pixel` draw / resize 宏里的 row-overlay 分支
  - `RGB565_8` alpha direct draw / resize 的 row-overlay 分支
  - raw `RGB565` direct draw / resize 的 row-overlay fast path 仍留在父宏 `EGUI_CONFIG_IMAGE_STD_ROW_OVERLAY_FAST_PATH_ENABLE` 下

### 性能证据

| 场景 | baseline | 关闭 non-rgb565 row-overlay fast path | 变化 |
| --- | ---: | ---: | ---: |
| `RECTANGLE` | `0.179ms` | `0.179ms` | `+0.0%` |
| `CIRCLE` | `0.810ms` | `0.810ms` | `+0.0%` |
| `ROUND_RECTANGLE` | `0.812ms` | `0.812ms` | `+0.0%` |
| `TEXT` | `1.027ms` | `1.027ms` | `+0.0%` |
| `IMAGE_565` | `0.239ms` | `0.237ms` | `-0.8%` |
| `EXTERN_IMAGE_565` | `0.417ms` | `0.414ms` | `-0.7%` |
| `IMAGE_RESIZE_565` | `0.459ms` | `0.460ms` | `+0.2%` |
| `EXTERN_IMAGE_RESIZE_565` | `0.507ms` | `0.507ms` | `+0.0%` |
| `IMAGE_565_8` | `0.524ms` | `0.524ms` | `+0.0%` |
| `EXTERN_IMAGE_565_8` | `0.693ms` | `0.693ms` | `+0.0%` |
| `IMAGE_RESIZE_565_8` | `0.635ms` | `0.635ms` | `+0.0%` |
| `EXTERN_IMAGE_RESIZE_565_8` | `0.698ms` | `0.698ms` | `+0.0%` |
| `TEXT_GRADIENT` | `0.232ms` | `0.232ms` | `+0.0%` |
| `TEXT_RECT_GRADIENT` | `0.761ms` | `0.761ms` | `+0.0%` |
| `TEXT_ROTATE_GRADIENT` | `2.552ms` | `2.552ms` | `+0.0%` |
| `IMAGE_GRADIENT_OVERLAY` | `0.910ms` | `0.911ms` | `+0.1%` |
| `MASK_GRADIENT_IMAGE` | `1.344ms` | `1.344ms` | `+0.0%` |
| `MASK_GRADIENT_IMAGE_ROTATE` | `1.863ms` | `1.863ms` | `+0.0%` |

- 这说明继续保留 raw RGB565 row-overlay fast path 后，当前真正被打到的 `IMAGE_GRADIENT_OVERLAY` 热点没有跟着 umbrella 一起退化。
- 现有 perf 抽样里，generic / `RGB565_8` row-overlay 分支也没有暴露出独立热点。

### 运行/单测

- `HelloPerformance` runtime
  - baseline：`241 frames`
  - 宏关闭：`241 frames`
  - baseline / off 的 `frame_0000.png` 和 `frame_0240.png` SHA256 完全一致
- `HelloUnitTest`
  - baseline：`688/688 passed`
  - 宏关闭：`688/688 passed`

### 结论

- 这颗 child 宏满足当前优先目标：
  - `text -1476B`
  - 基础 `RECTANGLE / CIRCLE / ROUND_RECTANGLE / TEXT / IMAGE_565 / EXTERN_IMAGE_565 / IMAGE_RESIZE_565 / EXTERN_IMAGE_RESIZE_565` 路径不退化
  - 当前 overlay 抽样热点 `IMAGE_GRADIENT_OVERLAY / MASK_GRADIENT_IMAGE / MASK_GRADIENT_IMAGE_ROTATE` 也都维持在噪声范围
- 因此：
  - 保留 `EGUI_CONFIG_IMAGE_STD_ROW_OVERLAY_NON_RGB565_FAST_PATH_ENABLE`
  - 默认值继续保持开启
  - 如果项目想优先回收 row-overlay 相关代码、但又不想牺牲 raw `IMAGE_GRADIENT_OVERLAY` 热点，应先试这颗 child，再考虑更激进的 `EGUI_CONFIG_IMAGE_STD_ROW_OVERLAY_FAST_PATH_ENABLE`

## 本轮继续细拆 A/B: `std image row-overlay rgb565 fast path`

### 测试环境

- 日期：`2026-04-04`
- 提交基线：`4ba0e37`
- `HelloPerformance` code size A/B
  - `make -B -j APP=HelloPerformance PORT=qemu CPU_ARCH=cortex-m0plus`
  - `make -B -j APP=HelloPerformance PORT=qemu CPU_ARCH=cortex-m0plus USER_CFLAGS=-DEGUI_CONFIG_IMAGE_STD_ROW_OVERLAY_RGB565_FAST_PATH_ENABLE=0`
- `HelloPerformance` perf A/B
  - `python scripts/perf_analysis/code_perf_check.py --profile cortex-m3 --clean --filter RECTANGLE,CIRCLE,ROUND_RECTANGLE,TEXT,IMAGE_565,IMAGE_565_1,IMAGE_565_8,EXTERN_IMAGE_565,EXTERN_IMAGE_565_1,EXTERN_IMAGE_565_8,IMAGE_RESIZE_565,IMAGE_RESIZE_565_1,IMAGE_RESIZE_565_8,EXTERN_IMAGE_RESIZE_565,EXTERN_IMAGE_RESIZE_565_1,EXTERN_IMAGE_RESIZE_565_8,TEXT_GRADIENT,TEXT_RECT_GRADIENT,TEXT_ROTATE_GRADIENT,IMAGE_GRADIENT_OVERLAY,MASK_GRADIENT_IMAGE,MASK_GRADIENT_IMAGE_ROTATE`
  - `python scripts/perf_analysis/code_perf_check.py --profile cortex-m3 --clean --filter RECTANGLE,CIRCLE,ROUND_RECTANGLE,TEXT,IMAGE_565,IMAGE_565_1,IMAGE_565_8,EXTERN_IMAGE_565,EXTERN_IMAGE_565_1,EXTERN_IMAGE_565_8,IMAGE_RESIZE_565,IMAGE_RESIZE_565_1,IMAGE_RESIZE_565_8,EXTERN_IMAGE_RESIZE_565,EXTERN_IMAGE_RESIZE_565_1,EXTERN_IMAGE_RESIZE_565_8,TEXT_GRADIENT,TEXT_RECT_GRADIENT,TEXT_ROTATE_GRADIENT,IMAGE_GRADIENT_OVERLAY,MASK_GRADIENT_IMAGE,MASK_GRADIENT_IMAGE_ROTATE --extra-cflags=-DEGUI_CONFIG_IMAGE_STD_ROW_OVERLAY_RGB565_FAST_PATH_ENABLE=0`
- 运行/单测验证
  - baseline / off runtime 使用 `scripts.code_runtime_check.compile_app()` + `run_app()` 分别录制到 `runtime_check_output/HelloPerformance/rowoverlayrgb565_base` / `rowoverlayrgb565_off`
  - `make -j APP=HelloUnitTest PORT=pc_test APP_OBJ_SUFFIX=utrowoverlayrgb565base`
  - `make -j APP=HelloUnitTest PORT=pc_test APP_OBJ_SUFFIX=utrowoverlayrgb565off USER_CFLAGS=-DEGUI_CONFIG_IMAGE_STD_ROW_OVERLAY_RGB565_FAST_PATH_ENABLE=0`
  - `output/main.exe`

### 代码量证据

- `HelloPerformance` 最终链接汇总：
  - baseline：`text=2056256 data=72 bss=3832`
  - 关闭 raw RGB565 row-overlay fast path：`text=2054944 data=72 bss=3832`
- 结论：
  - `text -1312B`
  - `data/bss` 不变
- 从当前实现归属看，这颗 child 只裁掉 raw `RGB565` 的 row-overlay 分支：
  - `egui_image_std_set_image_rgb565()` 里的 row-overlay fast path
  - `egui_image_std_set_image_resize_rgb565()` 里的 row-overlay fast path
  - generic `get_pixel` / `RGB565_8` row-overlay 分支仍留在 `EGUI_CONFIG_IMAGE_STD_ROW_OVERLAY_NON_RGB565_FAST_PATH_ENABLE`

### 性能证据

| 场景 | baseline | 关闭 raw RGB565 row-overlay fast path | 变化 |
| --- | ---: | ---: | ---: |
| `RECTANGLE` | `0.179ms` | `0.179ms` | `+0.0%` |
| `CIRCLE` | `0.810ms` | `0.810ms` | `+0.0%` |
| `ROUND_RECTANGLE` | `0.812ms` | `0.812ms` | `+0.0%` |
| `TEXT` | `1.027ms` | `1.027ms` | `+0.0%` |
| `IMAGE_565` | `0.239ms` | `0.238ms` | `-0.4%` |
| `IMAGE_565_1` | `0.418ms` | `0.418ms` | `+0.0%` |
| `IMAGE_565_8` | `0.524ms` | `0.527ms` | `+0.6%` |
| `EXTERN_IMAGE_565` | `0.417ms` | `0.415ms` | `-0.5%` |
| `EXTERN_IMAGE_565_1` | `0.587ms` | `0.587ms` | `+0.0%` |
| `EXTERN_IMAGE_565_8` | `0.693ms` | `0.695ms` | `+0.3%` |
| `IMAGE_RESIZE_565` | `0.459ms` | `0.459ms` | `+0.0%` |
| `IMAGE_RESIZE_565_1` | `0.611ms` | `0.611ms` | `+0.0%` |
| `IMAGE_RESIZE_565_8` | `0.635ms` | `0.636ms` | `+0.2%` |
| `EXTERN_IMAGE_RESIZE_565` | `0.507ms` | `0.507ms` | `+0.0%` |
| `EXTERN_IMAGE_RESIZE_565_1` | `0.678ms` | `0.678ms` | `+0.0%` |
| `EXTERN_IMAGE_RESIZE_565_8` | `0.698ms` | `0.698ms` | `+0.0%` |
| `TEXT_GRADIENT` | `0.232ms` | `0.233ms` | `+0.4%` |
| `TEXT_RECT_GRADIENT` | `0.761ms` | `0.761ms` | `+0.0%` |
| `TEXT_ROTATE_GRADIENT` | `2.552ms` | `2.551ms` | `-0.0%` |
| `IMAGE_GRADIENT_OVERLAY` | `0.910ms` | `2.417ms` | `+165.6%` |
| `MASK_GRADIENT_IMAGE` | `1.344ms` | `1.344ms` | `+0.0%` |
| `MASK_GRADIENT_IMAGE_ROTATE` | `1.863ms` | `1.863ms` | `+0.0%` |

- 这说明 raw RGB565 child 的代价已经被隔离到 raw `IMAGE_GRADIENT_OVERLAY` 热点：
  - 基础 `RECTANGLE / CIRCLE / ROUND_RECTANGLE / TEXT / IMAGE_565 / EXTERN_IMAGE_565 / IMAGE_RESIZE_565 / EXTERN_IMAGE_RESIZE_565` 全都在噪声范围
  - `IMAGE_565_1 / IMAGE_565_8 / EXTERN_IMAGE_565_1 / EXTERN_IMAGE_565_8 / IMAGE_RESIZE_565_1 / IMAGE_RESIZE_565_8 / EXTERN_IMAGE_RESIZE_565_1 / EXTERN_IMAGE_RESIZE_565_8` 也都维持在噪声范围
  - non-raw overlay 对照组 `MASK_GRADIENT_IMAGE / MASK_GRADIENT_IMAGE_ROTATE` 维持不变

### 运行/单测

- `HelloPerformance` runtime
  - baseline：`241 frames`，`ALL PASSED`
  - `raw RGB565 row-overlay child off`：`241 frames`，`ALL PASSED`
  - 截图哈希一致：`frame_0000.png` / `frame_0240.png`
- `HelloUnitTest`
  - baseline：`688/688 passed`
  - `raw RGB565 row-overlay child off`：`688/688 passed`

### 结论

- 这颗 child 宏成立：
  - `text -1312B`
  - 基础主路径和当前 non-raw overlay 样本不退化
  - perf 代价只落在 raw `IMAGE_GRADIENT_OVERLAY`
- 因此：
  - 保留 `EGUI_CONFIG_IMAGE_STD_ROW_OVERLAY_RGB565_FAST_PATH_ENABLE`
  - 默认值继续保持开启
  - 如果项目已经先保住了 `NON_RGB565` child、但可以接受 raw `IMAGE_GRADIENT_OVERLAY` 回退，应再试这颗 child；两颗 child 一起关闭时效果接近 `EGUI_CONFIG_IMAGE_STD_ROW_OVERLAY_FAST_PATH_ENABLE`

## 本轮继续细拆 A/B: `std image row-overlay alpha8 fast path`

### 测试环境

- 日期：`2026-04-04`
- 提交基线：`04c84fa`
- `HelloPerformance` code size A/B
  - `make -j APP=HelloPerformance PORT=qemu CPU_ARCH=cortex-m0plus APP_OBJ_SUFFIX=hprowoverlayalpha8on`
  - `make -j APP=HelloPerformance PORT=qemu CPU_ARCH=cortex-m0plus APP_OBJ_SUFFIX=hprowoverlayalpha8off USER_CFLAGS=-DEGUI_CONFIG_IMAGE_STD_ROW_OVERLAY_ALPHA8_FAST_PATH_ENABLE=0`
- `HelloPerformance` perf A/B
  - `python scripts/perf_analysis/code_perf_check.py --profile cortex-m3 --clean --threshold 1000 --timeout 900 --filter RECTANGLE,CIRCLE,ROUND_RECTANGLE,TEXT,IMAGE_565,IMAGE_565_1,IMAGE_565_8,EXTERN_IMAGE_565,EXTERN_IMAGE_565_1,EXTERN_IMAGE_565_8,IMAGE_RESIZE_565,IMAGE_RESIZE_565_1,IMAGE_RESIZE_565_8,EXTERN_IMAGE_RESIZE_565,EXTERN_IMAGE_RESIZE_565_1,EXTERN_IMAGE_RESIZE_565_8,TEXT_GRADIENT,TEXT_RECT_GRADIENT,TEXT_ROTATE_GRADIENT,IMAGE_GRADIENT_OVERLAY,MASK_GRADIENT_IMAGE,MASK_GRADIENT_IMAGE_ROTATE`
  - `python scripts/perf_analysis/code_perf_check.py --profile cortex-m3 --clean --threshold 1000 --timeout 900 --filter RECTANGLE,CIRCLE,ROUND_RECTANGLE,TEXT,IMAGE_565,IMAGE_565_1,IMAGE_565_8,EXTERN_IMAGE_565,EXTERN_IMAGE_565_1,EXTERN_IMAGE_565_8,IMAGE_RESIZE_565,IMAGE_RESIZE_565_1,IMAGE_RESIZE_565_8,EXTERN_IMAGE_RESIZE_565,EXTERN_IMAGE_RESIZE_565_1,EXTERN_IMAGE_RESIZE_565_8,TEXT_GRADIENT,TEXT_RECT_GRADIENT,TEXT_ROTATE_GRADIENT,IMAGE_GRADIENT_OVERLAY,MASK_GRADIENT_IMAGE,MASK_GRADIENT_IMAGE_ROTATE --extra-cflags=-DEGUI_CONFIG_IMAGE_STD_ROW_OVERLAY_ALPHA8_FAST_PATH_ENABLE=0`
- 运行/单测验证
  - baseline / off runtime 使用 `scripts.code_runtime_check.compile_app()` + `run_app()` 分别录制到 `runtime_check_output/HelloPerformance/rowoverlayalpha8_on` / `runtime_check_output/HelloPerformance/rowoverlayalpha8_off`
  - `make -j APP=HelloUnitTest PORT=pc_test APP_OBJ_SUFFIX=utrowoverlayalpha8on`
  - `make -j APP=HelloUnitTest PORT=pc_test APP_OBJ_SUFFIX=utrowoverlayalpha8off USER_CFLAGS=-DEGUI_CONFIG_IMAGE_STD_ROW_OVERLAY_ALPHA8_FAST_PATH_ENABLE=0`
  - `output/main.exe`

### 代码量证据

- `HelloPerformance` 最终链接汇总：
  - baseline：`text=2056256 data=72 bss=3832`
  - 关闭 alpha8 row-overlay fast path：`text=2054992 data=72 bss=3832`
- 结论：
  - `text -1264B`
  - `data/bss` 不变
- 从当前实现归属看，这颗 child 只裁掉 `RGB565_8` alpha draw / resize 的 row-overlay 分支：
  - `egui_image_std_set_image_rgb565_8()` 里的 row-overlay fast path
  - `egui_image_std_set_image_resize_rgb565_8_common()` 里的 row-overlay fast path
  - generic `get_pixel` row-overlay 继续留在 `EGUI_CONFIG_IMAGE_STD_ROW_OVERLAY_NON_RGB565_FAST_PATH_ENABLE`
- 同一轮验证里继续细拆 generic `get_pixel` row-overlay 只回收 `188B`，不足 `1KB`，不保留额外 child

### 性能证据

| 场景 | baseline | 关闭 alpha8 row-overlay fast path | 变化 |
| --- | ---: | ---: | ---: |
| `RECTANGLE` | `0.179ms` | `0.179ms` | `+0.0%` |
| `CIRCLE` | `0.810ms` | `0.810ms` | `+0.0%` |
| `ROUND_RECTANGLE` | `0.812ms` | `0.812ms` | `+0.0%` |
| `TEXT` | `1.027ms` | `1.027ms` | `+0.0%` |
| `IMAGE_565` | `0.239ms` | `0.239ms` | `+0.0%` |
| `IMAGE_565_1` | `0.418ms` | `0.418ms` | `+0.0%` |
| `IMAGE_565_8` | `0.524ms` | `0.529ms` | `+1.0%` |
| `IMAGE_565_8_DOUBLE` | `0.087ms` | `0.421ms` | `+383.9%` |
| `EXTERN_IMAGE_565` | `0.417ms` | `0.417ms` | `+0.0%` |
| `EXTERN_IMAGE_565_1` | `0.587ms` | `0.587ms` | `+0.0%` |
| `EXTERN_IMAGE_565_8` | `0.693ms` | `0.697ms` | `+0.6%` |
| `IMAGE_RESIZE_565` | `0.459ms` | `0.459ms` | `+0.0%` |
| `IMAGE_RESIZE_565_1` | `0.611ms` | `0.611ms` | `+0.0%` |
| `IMAGE_RESIZE_565_8` | `0.635ms` | `0.635ms` | `+0.0%` |
| `EXTERN_IMAGE_RESIZE_565` | `0.507ms` | `0.507ms` | `+0.0%` |
| `EXTERN_IMAGE_RESIZE_565_1` | `0.678ms` | `0.678ms` | `+0.0%` |
| `EXTERN_IMAGE_RESIZE_565_8` | `0.698ms` | `0.698ms` | `+0.0%` |
| `TEXT_GRADIENT` | `0.232ms` | `0.232ms` | `+0.0%` |
| `TEXT_RECT_GRADIENT` | `0.761ms` | `0.761ms` | `+0.0%` |
| `TEXT_ROTATE_GRADIENT` | `2.552ms` | `2.552ms` | `+0.0%` |
| `IMAGE_GRADIENT_OVERLAY` | `0.910ms` | `0.910ms` | `+0.0%` |
| `MASK_GRADIENT_IMAGE` | `1.344ms` | `1.344ms` | `+0.0%` |
| `MASK_GRADIENT_IMAGE_ROTATE` | `1.863ms` | `1.863ms` | `+0.0%` |

- 这说明 alpha8 child 的边界已经比较清楚：
  - 基础主路径 `RECTANGLE / CIRCLE / ROUND_RECTANGLE / TEXT / IMAGE_565 / EXTERN_IMAGE_565 / IMAGE_RESIZE_565 / EXTERN_IMAGE_RESIZE_565` 全都在噪声范围
  - 当前 overlay 对照组 `IMAGE_GRADIENT_OVERLAY / MASK_GRADIENT_IMAGE / MASK_GRADIENT_IMAGE_ROTATE` 维持不变
  - perf 代价主要集中在 `RGB565_8` overlay 热点，尤其是 `IMAGE_565_8_DOUBLE +383.9%`

### 运行/单测

- `HelloPerformance` runtime
  - baseline：`241 frames captured`
  - `alpha8 row-overlay child off`：`241 frames captured`
  - 全量 `241` 帧 PNG：hash mismatch `0`
  - 抽查哈希一致：`frame_0000.png` / `frame_0120.png` / `frame_0240.png`
- `HelloUnitTest`
  - baseline：`688/688 passed`
  - `alpha8 row-overlay child off`：`688/688 passed`

### 结论

- 这颗 child 宏成立：
  - `text -1264B`
  - 基础主路径和当前 gradient / overlay 对照样本不退化
  - perf 代价主要集中在 `IMAGE_565_8_DOUBLE` 这类 `RGB565_8` overlay 热点
- 因此：
  - 保留 `EGUI_CONFIG_IMAGE_STD_ROW_OVERLAY_ALPHA8_FAST_PATH_ENABLE`
  - 默认值继续保持开启
  - 如果项目想保住 generic `get_pixel` row-overlay、又不想动 raw `RGB565` row-overlay，但可以接受 `RGB565_8` overlay 热点回退，应先试这颗 child，再考虑父宏 `EGUI_CONFIG_IMAGE_STD_ROW_OVERLAY_NON_RGB565_FAST_PATH_ENABLE`

## 本轮继续细拆 A/B: `std image external alpha fast path`

### 测试环境

- 日期：`2026-04-03`
- 提交基线：`dabeeef`
- `HelloPerformance` code size A/B
  - `make -B -j APP=HelloPerformance PORT=qemu CPU_ARCH=cortex-m0plus APP_OBJ_SUFFIX=hpextalphaon`
  - `make -B -j APP=HelloPerformance PORT=qemu CPU_ARCH=cortex-m0plus APP_OBJ_SUFFIX=hpextalphaoff USER_CFLAGS=-DEGUI_CONFIG_IMAGE_STD_EXTERNAL_ALPHA_FAST_PATH_ENABLE=0`
- `HelloPerformance` perf A/B
  - `python scripts/perf_analysis/code_perf_check.py --profile cortex-m3 --clean --filter RECTANGLE,CIRCLE,ROUND_RECTANGLE,TEXT,IMAGE_565,IMAGE_RESIZE_565,EXTERN_IMAGE_565,EXTERN_IMAGE_RESIZE_565,EXTERN_IMAGE_565_1,EXTERN_IMAGE_565_2,EXTERN_IMAGE_565_4,EXTERN_IMAGE_565_8,EXTERN_IMAGE_RESIZE_565_1,EXTERN_IMAGE_RESIZE_565_2,EXTERN_IMAGE_RESIZE_565_4,EXTERN_IMAGE_RESIZE_565_8`
  - `python scripts/perf_analysis/code_perf_check.py --profile cortex-m3 --clean --filter RECTANGLE,CIRCLE,ROUND_RECTANGLE,TEXT,IMAGE_565,IMAGE_RESIZE_565,EXTERN_IMAGE_565,EXTERN_IMAGE_RESIZE_565,EXTERN_IMAGE_565_1,EXTERN_IMAGE_565_2,EXTERN_IMAGE_565_4,EXTERN_IMAGE_565_8,EXTERN_IMAGE_RESIZE_565_1,EXTERN_IMAGE_RESIZE_565_2,EXTERN_IMAGE_RESIZE_565_4,EXTERN_IMAGE_RESIZE_565_8 --extra-cflags=-DEGUI_CONFIG_IMAGE_STD_EXTERNAL_ALPHA_FAST_PATH_ENABLE=0`
- 运行/单测验证
  - `python scripts/code_runtime_check.py --app HelloPerformance --timeout 10 --keep-screenshots`
  - `USER_CFLAGS=-DEGUI_CONFIG_IMAGE_STD_EXTERNAL_ALPHA_FAST_PATH_ENABLE=0 python scripts/code_runtime_check.py --app HelloPerformance --timeout 10 --keep-screenshots`
  - `make -j APP=HelloUnitTest PORT=pc_test APP_OBJ_SUFFIX=utextalphaon`
  - `make -j APP=HelloUnitTest PORT=pc_test APP_OBJ_SUFFIX=utextalphaoff USER_CFLAGS=-DEGUI_CONFIG_IMAGE_STD_EXTERNAL_ALPHA_FAST_PATH_ENABLE=0`
  - `output/main.exe`

### 代码量证据

- `HelloPerformance` 链接汇总：
  - 默认开启：`text=2056208 data=72 bss=3832`
  - 关闭 external alpha fast path：`text=2052152 data=72 bss=3832`
- 结论：
  - `text -4056B`
  - `data/bss` 不变
- 对象/符号证据：
  - `output/obj/hpextalphaon_qemu/src/image/egui_image_std.c.o = 691812B`
  - `output/obj/hpextalphaoff_qemu/src/image/egui_image_std.c.o = 646468B`
  - 完整消失的 external-alpha 专用 helper 包括：
    - `egui_image_std_draw_image_resize_external_alpha.isra.0`：`0x924` = `2340B`
    - `egui_image_std_load_external_alpha_row_persistent_cache.constprop.0`：`0x1ec` = `492B`
    - `egui_image_std_prepare_external_alpha_row_persistent_cache_min_rows.constprop.0.isra.0`：`0xf8` = `248B`
  - 保留下来的 internal alpha fast path 也略有收缩：
    - `egui_image_std_set_image_rgb565_8`：`0x8b8 -> 0x864`
    - `egui_image_std_set_image_rgb565_4`：`0x56c -> 0x51c`
    - `egui_image_std_set_image_rgb565_2`：`0x568 -> 0x518`
    - `egui_image_std_set_image_rgb565_1`：`0x564 -> 0x504`

### 性能证据

| 场景 | 默认开启 | 关闭 external alpha fast path | 变化 |
| --- | ---: | ---: | ---: |
| `RECTANGLE` | `0.179ms` | `0.179ms` | `+0.0%` |
| `CIRCLE` | `0.810ms` | `0.810ms` | `+0.0%` |
| `ROUND_RECTANGLE` | `0.812ms` | `0.812ms` | `+0.0%` |
| `TEXT` | `1.026ms` | `1.026ms` | `+0.0%` |
| `IMAGE_565` | `0.239ms` | `0.239ms` | `+0.0%` |
| `IMAGE_RESIZE_565` | `0.459ms` | `0.459ms` | `+0.0%` |
| `EXTERN_IMAGE_565` | `0.417ms` | `0.414ms` | `-0.7%` |
| `EXTERN_IMAGE_RESIZE_565` | `0.506ms` | `0.507ms` | `+0.2%` |
| `EXTERN_IMAGE_565_1` | `0.587ms` | `0.095ms` | `-83.8%` |
| `EXTERN_IMAGE_565_2` | `0.680ms` | `0.096ms` | `-85.9%` |
| `EXTERN_IMAGE_565_4` | `0.862ms` | `0.096ms` | `-88.9%` |
| `EXTERN_IMAGE_565_8` | `0.704ms` | `0.097ms` | `-86.2%` |
| `EXTERN_IMAGE_RESIZE_565_1` | `0.678ms` | `0.108ms` | `-84.1%` |
| `EXTERN_IMAGE_RESIZE_565_2` | `0.840ms` | `0.108ms` | `-87.1%` |
| `EXTERN_IMAGE_RESIZE_565_4` | `0.854ms` | `0.109ms` | `-87.2%` |
| `EXTERN_IMAGE_RESIZE_565_8` | `0.698ms` | `0.109ms` | `-84.4%` |

- 这说明本轮宏切掉的确实是 external alpha 的 specialized path：
  - 当前优先保护的基础路径没有可见退化
  - `EXTERN_IMAGE_565` / `EXTERN_IMAGE_RESIZE_565` raw 主路径也仍在噪声范围
  - 真正回退的是 external `RGB565_1/2/4/8` draw/resize
- `EXTERN_IMAGE_565_1/2/4/8` 与 `EXTERN_IMAGE_RESIZE_565_1/2/4/8` 反而明显变快，和前几轮整条 alpha draw/resize 开关的结果一致，说明 generic fallback 在 external resource 上会先 resolve persistent image，再走统一 `get_pixel` 通路。

### 运行/截图/单测

- `HelloPerformance` runtime：
  - 默认开启：`241 frames`，`ALL PASSED`
  - 宏关闭：`241 frames`，`ALL PASSED`
- 截图抽查：
  - `frame_0000.png`：对角绿色矩形正常
  - `frame_0120.png`：平铺星形图片正常
- `HelloUnitTest`：
  - 默认开启：`688/688 passed`
  - 宏关闭：`688/688 passed`

### 结论

- 这条拆分满足当前优先目标：
  - 代码量收益 `>1KB`
  - `RECTANGLE / CIRCLE / ROUND_RECTANGLE / TEXT / IMAGE_565 / EXTERN_IMAGE_565` 性能回退 `<10%`
  - `IMAGE_RESIZE_565 / EXTERN_IMAGE_RESIZE_565` raw resize 主路径也没有可见退化
- 因此：
  - 保留 `EGUI_CONFIG_IMAGE_STD_EXTERNAL_ALPHA_FAST_PATH_ENABLE`
  - 默认值仍应保持开启
  - 它比整条 `FAST_DRAW_ALPHA` / `FAST_RESIZE_ALPHA` 更适合作为“只牺牲 external alpha specialized path”的 size-first 实验开关

## 本轮继续细拆 A/B: `std image external alpha8 fast path`

### 测试环境

- 日期：`2026-04-03`
- 提交基线：`dabeeef`
- `HelloPerformance` code size A/B
  - `make -B -j APP=HelloPerformance PORT=qemu CPU_ARCH=cortex-m0plus APP_OBJ_SUFFIX=hpextalpha8on`
  - `make -B -j APP=HelloPerformance PORT=qemu CPU_ARCH=cortex-m0plus APP_OBJ_SUFFIX=hpextalpha8off USER_CFLAGS=-DEGUI_CONFIG_IMAGE_STD_EXTERNAL_ALPHA8_FAST_PATH_ENABLE=0`
- `HelloPerformance` perf A/B
  - `python scripts/perf_analysis/code_perf_check.py --profile cortex-m3 --threshold 1000 --timeout 900 --clean --filter RECTANGLE,CIRCLE,ROUND_RECTANGLE,TEXT,IMAGE_565,IMAGE_RESIZE_565,EXTERN_IMAGE_565,EXTERN_IMAGE_RESIZE_565,EXTERN_IMAGE_565_8,EXTERN_IMAGE_RESIZE_565_8,EXTERN_IMAGE_565_1,EXTERN_IMAGE_565_2,EXTERN_IMAGE_565_4,EXTERN_IMAGE_RESIZE_565_1,EXTERN_IMAGE_RESIZE_565_2,EXTERN_IMAGE_RESIZE_565_4`
  - `python scripts/perf_analysis/code_perf_check.py --profile cortex-m3 --threshold 1000 --timeout 900 --clean --filter RECTANGLE,CIRCLE,ROUND_RECTANGLE,TEXT,IMAGE_565,IMAGE_RESIZE_565,EXTERN_IMAGE_565,EXTERN_IMAGE_RESIZE_565,EXTERN_IMAGE_565_8,EXTERN_IMAGE_RESIZE_565_8,EXTERN_IMAGE_565_1,EXTERN_IMAGE_565_2,EXTERN_IMAGE_565_4,EXTERN_IMAGE_RESIZE_565_1,EXTERN_IMAGE_RESIZE_565_2,EXTERN_IMAGE_RESIZE_565_4 --extra-cflags=-DEGUI_CONFIG_IMAGE_STD_EXTERNAL_ALPHA8_FAST_PATH_ENABLE=0`
- 运行/单测验证
  - `python scripts/code_runtime_check.py --app HelloPerformance --timeout 10 --keep-screenshots`
  - `USER_CFLAGS=-DEGUI_CONFIG_IMAGE_STD_EXTERNAL_ALPHA8_FAST_PATH_ENABLE=0 python scripts/code_runtime_check.py --app HelloPerformance --timeout 10 --keep-screenshots`
  - `make -j APP=HelloUnitTest PORT=pc_test APP_OBJ_SUFFIX=utextalpha8on`
  - `make -j APP=HelloUnitTest PORT=pc_test APP_OBJ_SUFFIX=utextalpha8off USER_CFLAGS=-DEGUI_CONFIG_IMAGE_STD_EXTERNAL_ALPHA8_FAST_PATH_ENABLE=0`
  - `output/main.exe`

### 代码量证据

- `HelloPerformance` 链接汇总：
  - 默认开启：`text=2055216 data=72 bss=3832`
  - 关闭 external alpha8 fast path：`text=2054628 data=72 bss=3832`
- 结论：
  - `text -588B`
  - `data/bss` 不变

### 性能证据

| 场景 | 默认开启 | 关闭 external alpha8 fast path | 变化 |
| --- | ---: | ---: | ---: |
| `RECTANGLE` | `0.179ms` | `0.179ms` | `+0.0%` |
| `CIRCLE` | `0.810ms` | `0.810ms` | `+0.0%` |
| `ROUND_RECTANGLE` | `0.812ms` | `0.812ms` | `+0.0%` |
| `TEXT` | `1.027ms` | `1.027ms` | `+0.0%` |
| `IMAGE_565` | `0.239ms` | `0.239ms` | `+0.0%` |
| `EXTERN_IMAGE_565` | `0.417ms` | `0.414ms` | `-0.7%` |
| `IMAGE_RESIZE_565` | `0.459ms` | `0.459ms` | `+0.0%` |
| `EXTERN_IMAGE_RESIZE_565` | `0.507ms` | `0.506ms` | `-0.2%` |
| `EXTERN_IMAGE_565_8` | `0.704ms` | `0.097ms` | `-86.2%` |
| `EXTERN_IMAGE_RESIZE_565_8` | `0.698ms` | `0.110ms` | `-84.2%` |
| `EXTERN_IMAGE_565_1` | `0.587ms` | `0.588ms` | `+0.2%` |
| `EXTERN_IMAGE_565_2` | `0.680ms` | `0.680ms` | `+0.0%` |
| `EXTERN_IMAGE_565_4` | `0.876ms` | `0.877ms` | `+0.1%` |
| `EXTERN_IMAGE_RESIZE_565_1` | `0.678ms` | `0.679ms` | `+0.1%` |
| `EXTERN_IMAGE_RESIZE_565_2` | `0.840ms` | `0.840ms` | `+0.0%` |
| `EXTERN_IMAGE_RESIZE_565_4` | `0.854ms` | `0.854ms` | `+0.0%` |

- 这说明：
  - 基础优先保护的 `RECTANGLE / CIRCLE / ROUND_RECTANGLE / TEXT / IMAGE_565 / EXTERN_IMAGE_565 / IMAGE_RESIZE_565 / EXTERN_IMAGE_RESIZE_565` 都没有可见退化
  - 代价只集中在 external `RGB565_8` draw / resize
  - external `RGB565_1/2/4` draw / resize 基本不受影响

### 运行/单测

- `HelloPerformance` runtime
  - 默认开启：`241 frames`，`ALL PASSED`
  - 宏关闭：`241 frames`，`ALL PASSED`
- `HelloUnitTest`
  - 默认开启：`688/688 passed`
  - 宏关闭：`688/688 passed`

### 结论

- 这条细拆在性能隔离上是成立的：
  - 基础优先路径没有可见回退
  - 代价只集中在 external `RGB565_8` draw / resize
- 但它不满足当前保留门槛：
  - code size 收益只有 `588B`，低于 `>1KB`
- 因此：
  - 这条细拆记录保留，但不进入当前优先保留的实验宏序列
  - `EGUI_CONFIG_IMAGE_STD_EXTERNAL_ALPHA8_FAST_PATH_ENABLE` 暂不作为 HelloPerformance 推荐实验开关

## 本轮继续细拆 A/B: `std image external packed-alpha fast path`

### 测试环境

- 日期：`2026-04-03`
- 提交基线：`dabeeef`
- `HelloPerformance` code size A/B
  - `make -B -j APP=HelloPerformance PORT=qemu CPU_ARCH=cortex-m0plus APP_OBJ_SUFFIX=hpextpackedon`
  - `make -B -j APP=HelloPerformance PORT=qemu CPU_ARCH=cortex-m0plus APP_OBJ_SUFFIX=hpextpackedoff USER_CFLAGS=-DEGUI_CONFIG_IMAGE_STD_EXTERNAL_PACKED_ALPHA_FAST_PATH_ENABLE=0`
- `HelloPerformance` perf A/B
  - `python scripts/perf_analysis/code_perf_check.py --profile cortex-m3 --threshold 1000 --timeout 900 --clean --filter RECTANGLE,CIRCLE,ROUND_RECTANGLE,TEXT,IMAGE_565,IMAGE_RESIZE_565,EXTERN_IMAGE_565,EXTERN_IMAGE_RESIZE_565,EXTERN_IMAGE_565_1,EXTERN_IMAGE_565_2,EXTERN_IMAGE_565_4,EXTERN_IMAGE_565_8,EXTERN_IMAGE_RESIZE_565_1,EXTERN_IMAGE_RESIZE_565_2,EXTERN_IMAGE_RESIZE_565_4,EXTERN_IMAGE_RESIZE_565_8`
  - `python scripts/perf_analysis/code_perf_check.py --profile cortex-m3 --threshold 1000 --timeout 900 --clean --filter RECTANGLE,CIRCLE,ROUND_RECTANGLE,TEXT,IMAGE_565,IMAGE_RESIZE_565,EXTERN_IMAGE_565,EXTERN_IMAGE_RESIZE_565,EXTERN_IMAGE_565_1,EXTERN_IMAGE_565_2,EXTERN_IMAGE_565_4,EXTERN_IMAGE_565_8,EXTERN_IMAGE_RESIZE_565_1,EXTERN_IMAGE_RESIZE_565_2,EXTERN_IMAGE_RESIZE_565_4,EXTERN_IMAGE_RESIZE_565_8 --extra-cflags=-DEGUI_CONFIG_IMAGE_STD_EXTERNAL_PACKED_ALPHA_FAST_PATH_ENABLE=0`
- 运行/单测验证
  - `python scripts/code_runtime_check.py --app HelloPerformance --timeout 10 --keep-screenshots`
  - `USER_CFLAGS=-DEGUI_CONFIG_IMAGE_STD_EXTERNAL_PACKED_ALPHA_FAST_PATH_ENABLE=0 python scripts/code_runtime_check.py --app HelloPerformance --timeout 10 --keep-screenshots`
  - `make -j APP=HelloUnitTest PORT=pc_test APP_OBJ_SUFFIX=utextpackedoff USER_CFLAGS=-DEGUI_CONFIG_IMAGE_STD_EXTERNAL_PACKED_ALPHA_FAST_PATH_ENABLE=0`
  - `output/main.exe`

### 代码量证据

- `HelloPerformance` 链接汇总：
  - 默认开启：`text=2055216 data=72 bss=3832`
  - 关闭 external packed fast path：`text=2052400 data=72 bss=3832`
- 结论：
  - `text -2816B`
  - `data/bss` 不变

### 性能证据

| 场景 | 默认开启 | 关闭 external packed fast path | 变化 |
| --- | ---: | ---: | ---: |
| `RECTANGLE` | `0.179ms` | `0.179ms` | `+0.0%` |
| `CIRCLE` | `0.810ms` | `0.810ms` | `+0.0%` |
| `ROUND_RECTANGLE` | `0.812ms` | `0.812ms` | `+0.0%` |
| `TEXT` | `1.027ms` | `1.027ms` | `+0.0%` |
| `IMAGE_565` | `0.239ms` | `0.239ms` | `+0.0%` |
| `EXTERN_IMAGE_565` | `0.417ms` | `0.417ms` | `+0.0%` |
| `IMAGE_RESIZE_565` | `0.459ms` | `0.459ms` | `+0.0%` |
| `EXTERN_IMAGE_RESIZE_565` | `0.507ms` | `0.506ms` | `-0.2%` |
| `EXTERN_IMAGE_565_1` | `0.587ms` | `0.096ms` | `-83.6%` |
| `EXTERN_IMAGE_565_2` | `0.680ms` | `0.096ms` | `-85.9%` |
| `EXTERN_IMAGE_565_4` | `0.876ms` | `0.097ms` | `-88.9%` |
| `EXTERN_IMAGE_565_8` | `0.704ms` | `0.704ms` | `+0.0%` |
| `EXTERN_IMAGE_RESIZE_565_1` | `0.678ms` | `0.108ms` | `-84.1%` |
| `EXTERN_IMAGE_RESIZE_565_2` | `0.840ms` | `0.109ms` | `-87.0%` |
| `EXTERN_IMAGE_RESIZE_565_4` | `0.854ms` | `0.109ms` | `-87.2%` |
| `EXTERN_IMAGE_RESIZE_565_8` | `0.698ms` | `0.696ms` | `-0.3%` |

- 这说明：
  - 基础优先保护的 `RECTANGLE / CIRCLE / ROUND_RECTANGLE / TEXT / IMAGE_565 / EXTERN_IMAGE_565 / IMAGE_RESIZE_565 / EXTERN_IMAGE_RESIZE_565` 都在噪声范围
  - external `RGB565_8` draw / resize 也保持不动
  - 真正被回收到 generic path 的只有 external `RGB565_1/2/4` draw / resize

### 运行/单测

- `HelloPerformance` runtime
  - 默认开启：`241 frames`，`ALL PASSED`
  - 宏关闭：`241 frames`，`ALL PASSED`
- `HelloUnitTest`
  - 默认开启：`688/688 passed`
  - 宏关闭：`688/688 passed`

### 结论

- 这条细拆满足当前优先目标：
  - 代码量收益 `>1KB`
  - 基础优先路径性能回退 `<10%`
  - raw external `RGB565` / external `RGB565_8` draw / resize 主路径都没有可见退化
- 因此：
  - 保留 `EGUI_CONFIG_IMAGE_STD_EXTERNAL_PACKED_ALPHA_FAST_PATH_ENABLE`
  - 它比 umbrella `EGUI_CONFIG_IMAGE_STD_EXTERNAL_ALPHA_FAST_PATH_ENABLE` 更适合作为“只牺牲 external packed-alpha specialized path”的细粒度实验开关

## 本轮继续细拆 A/B: `std image external packed-alpha resize fast path`

### 测试环境

- 日期：`2026-04-04`
- 提交基线：`cfd723e`
- `HelloPerformance` code size A/B
  - `make -B -j APP=HelloPerformance PORT=qemu CPU_ARCH=cortex-m0plus APP_OBJ_SUFFIX=hpextpackbase`
  - `make -B -j APP=HelloPerformance PORT=qemu CPU_ARCH=cortex-m0plus APP_OBJ_SUFFIX=hpextpackresizeoff USER_CFLAGS=-DEGUI_CONFIG_IMAGE_STD_EXTERNAL_PACKED_ALPHA_RESIZE_FAST_PATH_ENABLE=0`
- `HelloPerformance` perf A/B
  - `python scripts/perf_analysis/code_perf_check.py --profile cortex-m3 --threshold 1000 --clean --filter RECTANGLE,CIRCLE,ROUND_RECTANGLE,TEXT,IMAGE_565,EXTERN_IMAGE_565,IMAGE_RESIZE_565,EXTERN_IMAGE_RESIZE_565,EXTERN_IMAGE_565_1,EXTERN_IMAGE_565_2,EXTERN_IMAGE_565_4,EXTERN_IMAGE_565_8,EXTERN_IMAGE_RESIZE_565_1,EXTERN_IMAGE_RESIZE_565_2,EXTERN_IMAGE_RESIZE_565_4,EXTERN_IMAGE_RESIZE_565_8`
  - `python scripts/perf_analysis/code_perf_check.py --profile cortex-m3 --threshold 1000 --clean --filter RECTANGLE,CIRCLE,ROUND_RECTANGLE,TEXT,IMAGE_565,EXTERN_IMAGE_565,IMAGE_RESIZE_565,EXTERN_IMAGE_RESIZE_565,EXTERN_IMAGE_565_1,EXTERN_IMAGE_565_2,EXTERN_IMAGE_565_4,EXTERN_IMAGE_565_8,EXTERN_IMAGE_RESIZE_565_1,EXTERN_IMAGE_RESIZE_565_2,EXTERN_IMAGE_RESIZE_565_4,EXTERN_IMAGE_RESIZE_565_8 --extra-cflags=-DEGUI_CONFIG_IMAGE_STD_EXTERNAL_PACKED_ALPHA_RESIZE_FAST_PATH_ENABLE=0`
- 运行/单测验证
  - `python scripts/code_runtime_check.py --app HelloPerformance --timeout 10`
  - `USER_CFLAGS=-DEGUI_CONFIG_IMAGE_STD_EXTERNAL_PACKED_ALPHA_RESIZE_FAST_PATH_ENABLE=0 python scripts/code_runtime_check.py --app HelloPerformance --timeout 10`
  - `make -j APP=HelloUnitTest PORT=pc_test APP_OBJ_SUFFIX=utextpackedfinalon`
  - `make -j APP=HelloUnitTest PORT=pc_test APP_OBJ_SUFFIX=utextpackedfinaloff USER_CFLAGS=-DEGUI_CONFIG_IMAGE_STD_EXTERNAL_PACKED_ALPHA_RESIZE_FAST_PATH_ENABLE=0`
  - `output/main.exe`

### 代码量证据

- `HelloPerformance` 链接汇总：
  - baseline：`text=2056264 data=72 bss=3832`
  - 关闭 packed-alpha resize child：`text=2053468 data=72 bss=3832`
- 结论：
  - `text -2796B`
  - `data/bss` 不变
- 同轮补做了 draw-only 试拆：
  - 临时态 `text=2056084 data=72 bss=3832`
  - 只回收 `180B`，不值得保留子宏，最终已回滚

### 性能证据

| 场景 | baseline | 关闭 packed-alpha resize child | 变化 |
| --- | ---: | ---: | ---: |
| `RECTANGLE` | `0.179ms` | `0.179ms` | `+0.0%` |
| `CIRCLE` | `0.810ms` | `0.810ms` | `+0.0%` |
| `ROUND_RECTANGLE` | `0.812ms` | `0.812ms` | `+0.0%` |
| `TEXT` | `1.027ms` | `1.027ms` | `+0.0%` |
| `IMAGE_565` | `0.239ms` | `0.239ms` | `+0.0%` |
| `EXTERN_IMAGE_565` | `0.417ms` | `0.417ms` | `+0.0%` |
| `IMAGE_RESIZE_565` | `0.459ms` | `0.459ms` | `+0.0%` |
| `EXTERN_IMAGE_RESIZE_565` | `0.507ms` | `0.507ms` | `+0.0%` |
| `EXTERN_IMAGE_565_1` | `0.587ms` | `0.587ms` | `+0.0%` |
| `EXTERN_IMAGE_565_2` | `0.680ms` | `0.680ms` | `+0.0%` |
| `EXTERN_IMAGE_565_4` | `0.862ms` | `0.876ms` | `+1.6%` |
| `EXTERN_IMAGE_565_8` | `0.704ms` | `0.704ms` | `+0.0%` |
| `EXTERN_IMAGE_RESIZE_565_1` | `0.678ms` | `0.108ms` | `-84.1%` |
| `EXTERN_IMAGE_RESIZE_565_2` | `0.840ms` | `0.109ms` | `-87.0%` |
| `EXTERN_IMAGE_RESIZE_565_4` | `0.854ms` | `0.109ms` | `-87.2%` |
| `EXTERN_IMAGE_RESIZE_565_8` | `0.698ms` | `0.698ms` | `+0.0%` |

- 这说明新的 child 宏已经把代价进一步收敛到 external `RGB565_1/2/4 resize`：
  - raw `EXTERN_IMAGE_565 / EXTERN_IMAGE_RESIZE_565` 主路径不动
  - external `RGB565_1/2/4` direct draw 也基本不动
  - 回退只集中在 external `RGB565_1/2/4 resize`
- 和此前 external alpha / packed-alpha A/B 一样，external resize 关闭 specialized path 后当前样本反而更快，说明 generic fallback 通过 resolved persistent image 命中了更短路径。

### 运行/单测

- `HelloPerformance` runtime
  - baseline：`241 frames`，`ALL PASSED`
  - packed-alpha resize child off：`241 frames`，`ALL PASSED`
- `HelloUnitTest`
  - baseline：`688/688 passed`
  - packed-alpha resize child off：`688/688 passed`

### 结论

- 这颗 child 宏成立：
  - 代码量收益 `>1KB`
  - 基础 `RECTANGLE / CIRCLE / ROUND_RECTANGLE / TEXT / IMAGE_565 / EXTERN_IMAGE_565 / IMAGE_RESIZE_565 / EXTERN_IMAGE_RESIZE_565` 不退化
  - external `RGB565_1/2/4` direct draw 也基本保住
- 因此：
  - 保留 `EGUI_CONFIG_IMAGE_STD_EXTERNAL_PACKED_ALPHA_RESIZE_FAST_PATH_ENABLE`
  - 默认值仍应保持开启
  - 它比 umbrella `EGUI_CONFIG_IMAGE_STD_EXTERNAL_PACKED_ALPHA_FAST_PATH_ENABLE` 更适合作为“只牺牲 external RGB565_1/2/4 resize”的更细粒度实验开关
  - draw-only child 只回收 `180B`，明确拒绝保留

## 本轮继续细拆 A/B: `std image alpha-color fast path`

### 测试环境

- 日期：`2026-04-03`
- 提交基线：`dabeeef`
- `HelloPerformance` code size A/B
  - `make -B -j APP=HelloPerformance PORT=qemu CPU_ARCH=cortex-m0plus APP_OBJ_SUFFIX=hpimgcolorfaston`
  - `make -B -j APP=HelloPerformance PORT=qemu CPU_ARCH=cortex-m0plus APP_OBJ_SUFFIX=hpimgcolorfastoff USER_CFLAGS=-DEGUI_CONFIG_IMAGE_STD_ALPHA_COLOR_FAST_PATH_ENABLE=0`
- `HelloPerformance` perf A/B
  - `python scripts/perf_analysis/code_perf_check.py --profile cortex-m3 --threshold 1000 --timeout 900 --filter RECTANGLE,CIRCLE,ROUND_RECTANGLE,TEXT,IMAGE_565,EXTERN_IMAGE_565`
  - `python scripts/perf_analysis/code_perf_check.py --profile cortex-m3 --threshold 1000 --timeout 900 --filter RECTANGLE,CIRCLE,ROUND_RECTANGLE,TEXT,IMAGE_565,EXTERN_IMAGE_565 --extra-cflags=-DEGUI_CONFIG_IMAGE_STD_ALPHA_COLOR_FAST_PATH_ENABLE=0`
- alpha-color 热点补充
  - `python scripts/perf_analysis/code_perf_check.py --profile cortex-m3 --threshold 1000 --timeout 180 --clean --filter IMAGE_COLOR --extra-cflags=\"-DEGUI_TEST_CONFIG_SINGLE_TEST=EGUI_VIEW_TEST_PERFORMANCE_TYPE_IMAGE_COLOR\"`
  - `python scripts/perf_analysis/code_perf_check.py --profile cortex-m3 --threshold 1000 --timeout 180 --clean --filter IMAGE_COLOR --extra-cflags=\"-DEGUI_TEST_CONFIG_SINGLE_TEST=EGUI_VIEW_TEST_PERFORMANCE_TYPE_IMAGE_COLOR -DEGUI_CONFIG_IMAGE_STD_ALPHA_COLOR_FAST_PATH_ENABLE=0\"`
  - `python scripts/perf_analysis/code_perf_check.py --profile cortex-m3 --threshold 1000 --timeout 180 --clean --filter IMAGE_RESIZE_COLOR --extra-cflags=\"-DEGUI_TEST_CONFIG_SINGLE_TEST=EGUI_VIEW_TEST_PERFORMANCE_TYPE_IMAGE_RESIZE_COLOR\"`
  - `python scripts/perf_analysis/code_perf_check.py --profile cortex-m3 --threshold 1000 --timeout 180 --clean --filter IMAGE_RESIZE_COLOR --extra-cflags=\"-DEGUI_TEST_CONFIG_SINGLE_TEST=EGUI_VIEW_TEST_PERFORMANCE_TYPE_IMAGE_RESIZE_COLOR -DEGUI_CONFIG_IMAGE_STD_ALPHA_COLOR_FAST_PATH_ENABLE=0\"`
- 运行/单测验证
  - `python scripts/code_runtime_check.py --app HelloPerformance --timeout 10 --keep-screenshots`
  - `USER_CFLAGS=-DEGUI_CONFIG_IMAGE_STD_ALPHA_COLOR_FAST_PATH_ENABLE=0 python scripts/code_runtime_check.py --app HelloPerformance --timeout 10 --keep-screenshots`
  - `make -j APP=HelloUnitTest PORT=pc_test APP_OBJ_SUFFIX=utimgcolorfaston`
  - `make -j APP=HelloUnitTest PORT=pc_test APP_OBJ_SUFFIX=utimgcolorfastoff USER_CFLAGS=-DEGUI_CONFIG_IMAGE_STD_ALPHA_COLOR_FAST_PATH_ENABLE=0`
  - `output/main.exe`

### 代码量证据

- `HelloPerformance` 链接汇总：
  - 默认开启：`text=2055224 data=72 bss=3832`
  - 关闭 alpha-color fast path：`text=2053976 data=72 bss=3832`
- 结论：
  - `text -1248B`
  - `data/bss` 不变
  - 回收的是 alpha-only tinted image draw/resize specialized helpers

### 性能证据

| 场景 | 默认开启 | 关闭 alpha-color fast path | 变化 |
| --- | ---: | ---: | ---: |
| `RECTANGLE` | `0.179ms` | `0.179ms` | `+0.0%` |
| `CIRCLE` | `0.810ms` | `0.810ms` | `+0.0%` |
| `ROUND_RECTANGLE` | `0.812ms` | `0.812ms` | `+0.0%` |
| `TEXT` | `1.027ms` | `1.027ms` | `+0.0%` |
| `IMAGE_565` | `0.239ms` | `0.239ms` | `+0.0%` |
| `EXTERN_IMAGE_565` | `0.417ms` | `0.417ms` | `+0.0%` |
| `IMAGE_COLOR` | `0.798ms` | `3.694ms` | `+362.9%` |
| `IMAGE_RESIZE_COLOR` | `0.743ms` | `3.683ms` | `+395.7%` |

- 这说明：
  - 基础优先保留的 `RECTANGLE / CIRCLE / ROUND_RECTANGLE / TEXT / IMAGE_565 / EXTERN_IMAGE_565` 完全不动
  - 代价集中在 alpha-only image 的 tinted draw / resize 热点

### 运行/单测

- `HelloPerformance` runtime
  - 默认开启：`241 frames`，`ALL PASSED`
  - 宏关闭：`241 frames`，`ALL PASSED`
- `HelloUnitTest`
  - 默认开启：`688/688 passed`
  - 宏关闭：`688/688 passed`

### 结论

- 这颗宏满足：
  - 代码量收益 `>1KB`
  - 基础 `RECTANGLE / CIRCLE / ROUND_RECTANGLE / TEXT / IMAGE_565 / EXTERN_IMAGE_565` 不退化
- 但它会明显牺牲：
  - `IMAGE_COLOR`
  - `IMAGE_RESIZE_COLOR`
- 因此：
  - 保留 `EGUI_CONFIG_IMAGE_STD_ALPHA_COLOR_FAST_PATH_ENABLE`
  - 默认值仍应保持开启
  - 它适合定位为“项目不用 alpha-only tinted image 热点”时的条件实验宏，不进入无条件优先关闭序列

## 本轮继续细拆 A/B: `std image alpha-color child fast path`

### 测试环境

- 日期：`2026-04-04`
- 提交基线：`dabeeef`
- `HelloPerformance` code size A/B
  - `make -B -j APP=HelloPerformance PORT=qemu CPU_ARCH=cortex-m0plus APP_OBJ_SUFFIX=hpalphacolorbase`
  - `make -B -j APP=HelloPerformance PORT=qemu CPU_ARCH=cortex-m0plus APP_OBJ_SUFFIX=hpalphacolordrawoff USER_CFLAGS=-DEGUI_CONFIG_IMAGE_STD_ALPHA_COLOR_DRAW_FAST_PATH_ENABLE=0`
  - `make -B -j APP=HelloPerformance PORT=qemu CPU_ARCH=cortex-m0plus APP_OBJ_SUFFIX=hpalphacolorresizeoff USER_CFLAGS=-DEGUI_CONFIG_IMAGE_STD_ALPHA_COLOR_RESIZE_FAST_PATH_ENABLE=0`
- `HelloPerformance` perf A/B
  - `python scripts/perf_analysis/code_perf_check.py --profile cortex-m3 --threshold 1000 --timeout 600 --clean --filter RECTANGLE,CIRCLE,ROUND_RECTANGLE,TEXT,IMAGE_565,EXTERN_IMAGE_565,IMAGE_COLOR,IMAGE_RESIZE_COLOR`
  - `python scripts/perf_analysis/code_perf_check.py --profile cortex-m3 --threshold 1000 --timeout 600 --clean --filter RECTANGLE,CIRCLE,ROUND_RECTANGLE,TEXT,IMAGE_565,EXTERN_IMAGE_565,IMAGE_COLOR,IMAGE_RESIZE_COLOR --extra-cflags=-DEGUI_CONFIG_IMAGE_STD_ALPHA_COLOR_DRAW_FAST_PATH_ENABLE=0`
  - `python scripts/perf_analysis/code_perf_check.py --profile cortex-m3 --threshold 1000 --timeout 600 --clean --filter RECTANGLE,CIRCLE,ROUND_RECTANGLE,TEXT,IMAGE_565,EXTERN_IMAGE_565,IMAGE_COLOR,IMAGE_RESIZE_COLOR --extra-cflags=-DEGUI_CONFIG_IMAGE_STD_ALPHA_COLOR_RESIZE_FAST_PATH_ENABLE=0`
- 运行/单测验证
  - `python scripts/code_runtime_check.py --app HelloPerformance --timeout 10 --keep-screenshots`
  - `USER_CFLAGS=-DEGUI_CONFIG_IMAGE_STD_ALPHA_COLOR_DRAW_FAST_PATH_ENABLE=0 python scripts/code_runtime_check.py --app HelloPerformance --timeout 10 --keep-screenshots`
  - `USER_CFLAGS=-DEGUI_CONFIG_IMAGE_STD_ALPHA_COLOR_RESIZE_FAST_PATH_ENABLE=0 python scripts/code_runtime_check.py --app HelloPerformance --timeout 10 --keep-screenshots`
  - `make -B -j APP=HelloUnitTest PORT=pc_test APP_OBJ_SUFFIX=utalphacolorbase`
  - `make -B -j APP=HelloUnitTest PORT=pc_test APP_OBJ_SUFFIX=utalphacolordrawoff USER_CFLAGS=-DEGUI_CONFIG_IMAGE_STD_ALPHA_COLOR_DRAW_FAST_PATH_ENABLE=0`
  - `make -B -j APP=HelloUnitTest PORT=pc_test APP_OBJ_SUFFIX=utalphacolorresizeoff USER_CFLAGS=-DEGUI_CONFIG_IMAGE_STD_ALPHA_COLOR_RESIZE_FAST_PATH_ENABLE=0`
  - `output/main.exe`

### 代码量证据

- `HelloPerformance` 链接汇总：
  - baseline：`text=2056272 data=72 bss=3832`
  - `DRAW` child off：`text=2056220 data=72 bss=3832`
  - `RESIZE` child off：`text=2056088 data=72 bss=3832`
- 结论：
  - `DRAW child -52B`
  - `RESIZE child -184B`
  - 两边都远低于 `1KB` 保留阈值

### 性能证据

| 场景 | baseline | `DRAW` off | 变化 | `RESIZE` off | 变化 |
| --- | ---: | ---: | ---: | ---: | ---: |
| `RECTANGLE` | `0.179ms` | `0.179ms` | `+0.0%` | `0.179ms` | `+0.0%` |
| `CIRCLE` | `0.810ms` | `0.810ms` | `+0.0%` | `0.810ms` | `+0.0%` |
| `ROUND_RECTANGLE` | `0.812ms` | `0.812ms` | `+0.0%` | `0.812ms` | `+0.0%` |
| `TEXT` | `1.027ms` | `1.027ms` | `+0.0%` | `1.027ms` | `+0.0%` |
| `IMAGE_565` | `0.239ms` | `0.239ms` | `+0.0%` | `0.239ms` | `+0.0%` |
| `EXTERN_IMAGE_565` | `0.417ms` | `0.417ms` | `+0.0%` | `0.417ms` | `+0.0%` |
| `IMAGE_COLOR` | `0.798ms` | `3.224ms` | `+304.0%` | `0.798ms` | `+0.0%` |
| `IMAGE_RESIZE_COLOR` | `0.771ms` | `0.771ms` | `+0.0%` | `3.386ms` | `+339.2%` |

- 这说明：
  - child split 的 perf 影响已经按语义成功隔离
  - `DRAW` child 只伤 `IMAGE_COLOR`
  - `RESIZE` child 只伤 `IMAGE_RESIZE_COLOR`
  - 但对应代码量回收都太小，不值得为此保留长期子宏

### 运行/单测

- `HelloPerformance` runtime
  - baseline：`241 frames`，`ALL PASSED`
  - `DRAW` child off：`241 frames`，`ALL PASSED`
  - `RESIZE` child off：`241 frames`，`ALL PASSED`
- `HelloUnitTest`
  - baseline：`688/688 passed`
  - `DRAW` child off：`688/688 passed`
  - `RESIZE` child off：`688/688 passed`

### 结论

- 这轮 child split 证明了 `alpha-color` 可以被干净拆成 draw / resize 两半。
- 但最终收益只有：
  - `DRAW child -52B`
  - `RESIZE child -184B`
- 因此：
  - 不保留 `EGUI_CONFIG_IMAGE_STD_ALPHA_COLOR_DRAW_FAST_PATH_ENABLE`
  - 不保留 `EGUI_CONFIG_IMAGE_STD_ALPHA_COLOR_RESIZE_FAST_PATH_ENABLE`
  - 代码已回退为只保留 umbrella `EGUI_CONFIG_IMAGE_STD_ALPHA_COLOR_FAST_PATH_ENABLE`
  - 这一项的最终定位不变：保留 umbrella 宏，作为“项目不用 alpha-only tinted image 热点”时的条件实验宏

## 本轮继续细拆 A/B: `std image alpha8 resize fast path`

### 测试环境

- 日期：`2026-04-03`
- 提交基线：`dabeeef`
- `HelloPerformance` code size A/B
  - `make -B -j APP=HelloPerformance PORT=qemu CPU_ARCH=cortex-m0plus APP_OBJ_SUFFIX=hpalpha8resizeon`
  - `make -B -j APP=HelloPerformance PORT=qemu CPU_ARCH=cortex-m0plus APP_OBJ_SUFFIX=hpalpha8resizeoff USER_CFLAGS=-DEGUI_CONFIG_IMAGE_STD_FAST_RESIZE_ALPHA8_ENABLE=0`
- `HelloPerformance` perf A/B
  - `python scripts/perf_analysis/code_perf_check.py --profile cortex-m3 --threshold 1000 --timeout 900 --clean --filter RECTANGLE,CIRCLE,ROUND_RECTANGLE,TEXT,IMAGE_565,EXTERN_IMAGE_565,IMAGE_RESIZE_565,EXTERN_IMAGE_RESIZE_565,IMAGE_RESIZE_565_8,EXTERN_IMAGE_RESIZE_565_8,IMAGE_RESIZE_565_1,IMAGE_RESIZE_565_2,IMAGE_RESIZE_565_4`
  - `python scripts/perf_analysis/code_perf_check.py --profile cortex-m3 --threshold 1000 --timeout 900 --clean --filter RECTANGLE,CIRCLE,ROUND_RECTANGLE,TEXT,IMAGE_565,EXTERN_IMAGE_565,IMAGE_RESIZE_565,EXTERN_IMAGE_RESIZE_565,IMAGE_RESIZE_565_8,EXTERN_IMAGE_RESIZE_565_8,IMAGE_RESIZE_565_1,IMAGE_RESIZE_565_2,IMAGE_RESIZE_565_4 --extra-cflags=-DEGUI_CONFIG_IMAGE_STD_FAST_RESIZE_ALPHA8_ENABLE=0`
- 运行/单测验证
  - `python scripts/code_runtime_check.py --app HelloPerformance --timeout 10 --keep-screenshots`
  - `USER_CFLAGS=-DEGUI_CONFIG_IMAGE_STD_FAST_RESIZE_ALPHA8_ENABLE=0 python scripts/code_runtime_check.py --app HelloPerformance --timeout 10 --keep-screenshots`
  - `make -j APP=HelloUnitTest PORT=pc_test APP_OBJ_SUFFIX=utalpha8resizeon`
  - `make -j APP=HelloUnitTest PORT=pc_test APP_OBJ_SUFFIX=utalpha8resizeoff USER_CFLAGS=-DEGUI_CONFIG_IMAGE_STD_FAST_RESIZE_ALPHA8_ENABLE=0`
  - `output/main.exe`

### 代码量证据

- `HelloPerformance` 链接汇总：
  - 默认开启：`text=2055224 data=72 bss=3832`
  - 关闭 alpha8 resize fast path：`text=2051468 data=72 bss=3832`
- 结论：
  - `text -3756B`
  - `data/bss` 不变
  - 被回收的主要是 `RGB565_8` resize specialized helpers

### 性能证据

| 场景 | 默认开启 | 关闭 alpha8 resize fast path | 变化 |
| --- | ---: | ---: | ---: |
| `RECTANGLE` | `0.179ms` | `0.179ms` | `+0.0%` |
| `CIRCLE` | `0.810ms` | `0.810ms` | `+0.0%` |
| `ROUND_RECTANGLE` | `0.812ms` | `0.812ms` | `+0.0%` |
| `TEXT` | `1.027ms` | `1.027ms` | `+0.0%` |
| `IMAGE_565` | `0.239ms` | `0.239ms` | `+0.0%` |
| `EXTERN_IMAGE_565` | `0.417ms` | `0.417ms` | `+0.0%` |
| `IMAGE_RESIZE_565` | `0.459ms` | `0.460ms` | `+0.2%` |
| `EXTERN_IMAGE_RESIZE_565` | `0.507ms` | `0.507ms` | `+0.0%` |
| `IMAGE_RESIZE_565_8` | `0.635ms` | `2.540ms` | `+300.0%` |
| `EXTERN_IMAGE_RESIZE_565_8` | `0.698ms` | `0.110ms` | `-84.2%` |
| `IMAGE_RESIZE_565_1` | `0.612ms` | `0.613ms` | `+0.2%` |
| `IMAGE_RESIZE_565_2` | `0.774ms` | `0.775ms` | `+0.1%` |
| `IMAGE_RESIZE_565_4` | `0.788ms` | `0.788ms` | `+0.0%` |

- 这说明：
  - 基础优先保留的 `RECTANGLE / CIRCLE / ROUND_RECTANGLE / TEXT / IMAGE_565 / EXTERN_IMAGE_565` 完全不动
  - raw `IMAGE_RESIZE_565 / EXTERN_IMAGE_RESIZE_565` 也不动
  - 代价集中在 internal `RGB565_8 resize`
  - `RGB565_1/2/4 resize` 不受影响

### 运行/单测

- `HelloPerformance` runtime
  - 默认开启：`241 frames`，`ALL PASSED`
  - 宏关闭：`241 frames`，`ALL PASSED`
- `HelloUnitTest`
  - 默认开启：`688/688 passed`
  - 宏关闭：`688/688 passed`

### 结论

- 这颗宏满足：
  - 代码量收益 `>1KB`
  - 基础 `RECTANGLE / CIRCLE / ROUND_RECTANGLE / TEXT / IMAGE_565 / EXTERN_IMAGE_565` 不退化
  - raw `IMAGE_RESIZE_565 / EXTERN_IMAGE_RESIZE_565` 不退化
- 因此：
  - 保留 `EGUI_CONFIG_IMAGE_STD_FAST_RESIZE_ALPHA8_ENABLE`
  - 默认值仍应保持开启
  - 它比 `EGUI_CONFIG_IMAGE_STD_FAST_RESIZE_ALPHA_ENABLE` 更适合作为“只牺牲 internal RGB565_8 resize”的 size-first 实验开关

## 本轮继续细拆 A/B: `std image packed-alpha resize fast path`

### 测试环境

- 日期：`2026-04-03`
- 提交基线：`dabeeef`
- `HelloPerformance` code size A/B
  - `make -B -j APP=HelloPerformance PORT=qemu CPU_ARCH=cortex-m0plus APP_OBJ_SUFFIX=hppackedresizeon`
  - `make -B -j APP=HelloPerformance PORT=qemu CPU_ARCH=cortex-m0plus APP_OBJ_SUFFIX=hppackedresizeoff USER_CFLAGS=-DEGUI_CONFIG_IMAGE_STD_FAST_RESIZE_PACKED_ALPHA_ENABLE=0`
- `HelloPerformance` perf A/B
  - `python scripts/perf_analysis/code_perf_check.py --profile cortex-m3 --threshold 1000 --timeout 900 --clean --filter RECTANGLE,CIRCLE,ROUND_RECTANGLE,TEXT,IMAGE_565,EXTERN_IMAGE_565,IMAGE_RESIZE_565,IMAGE_RESIZE_565_1,IMAGE_RESIZE_565_2,IMAGE_RESIZE_565_4,IMAGE_RESIZE_565_8,EXTERN_IMAGE_RESIZE_565,EXTERN_IMAGE_RESIZE_565_1,EXTERN_IMAGE_RESIZE_565_2,EXTERN_IMAGE_RESIZE_565_4,EXTERN_IMAGE_RESIZE_565_8`
  - `python scripts/perf_analysis/code_perf_check.py --profile cortex-m3 --threshold 1000 --timeout 900 --clean --filter RECTANGLE,CIRCLE,ROUND_RECTANGLE,TEXT,IMAGE_565,EXTERN_IMAGE_565,IMAGE_RESIZE_565,IMAGE_RESIZE_565_1,IMAGE_RESIZE_565_2,IMAGE_RESIZE_565_4,IMAGE_RESIZE_565_8,EXTERN_IMAGE_RESIZE_565,EXTERN_IMAGE_RESIZE_565_1,EXTERN_IMAGE_RESIZE_565_2,EXTERN_IMAGE_RESIZE_565_4,EXTERN_IMAGE_RESIZE_565_8 --extra-cflags=-DEGUI_CONFIG_IMAGE_STD_FAST_RESIZE_PACKED_ALPHA_ENABLE=0`
- 运行/单测验证
  - `python scripts/code_runtime_check.py --app HelloPerformance --timeout 10 --keep-screenshots`
  - `USER_CFLAGS=-DEGUI_CONFIG_IMAGE_STD_FAST_RESIZE_PACKED_ALPHA_ENABLE=0 python scripts/code_runtime_check.py --app HelloPerformance --timeout 10 --keep-screenshots`
  - `make -j APP=HelloUnitTest PORT=pc_test APP_OBJ_SUFFIX=utpackedresizeon`
  - `make -j APP=HelloUnitTest PORT=pc_test APP_OBJ_SUFFIX=utpackedresizeoff USER_CFLAGS=-DEGUI_CONFIG_IMAGE_STD_FAST_RESIZE_PACKED_ALPHA_ENABLE=0`
  - `output/main.exe`

### 代码量证据

- `HelloPerformance` 链接汇总：
  - 默认开启：`text=2055224 data=72 bss=3832`
  - 关闭 packed-alpha resize fast path：`text=2047712 data=72 bss=3832`
- 结论：
  - `text -7512B`
  - `data/bss` 不变
  - 被回收的主要是 `RGB565_1/2/4` resize specialized helpers

### 性能证据

| 场景 | 默认开启 | 关闭 packed-alpha resize fast path | 变化 |
| --- | ---: | ---: | ---: |
| `RECTANGLE` | `0.179ms` | `0.179ms` | `+0.0%` |
| `CIRCLE` | `0.810ms` | `0.810ms` | `+0.0%` |
| `ROUND_RECTANGLE` | `0.812ms` | `0.812ms` | `+0.0%` |
| `TEXT` | `1.027ms` | `1.027ms` | `+0.0%` |
| `IMAGE_565` | `0.239ms` | `0.238ms` | `-0.4%` |
| `EXTERN_IMAGE_565` | `0.417ms` | `0.413ms` | `-1.0%` |
| `IMAGE_RESIZE_565` | `0.459ms` | `0.459ms` | `+0.0%` |
| `IMAGE_RESIZE_565_1` | `0.612ms` | `2.879ms` | `+370.4%` |
| `IMAGE_RESIZE_565_2` | `0.774ms` | `3.077ms` | `+297.5%` |
| `IMAGE_RESIZE_565_4` | `0.788ms` | `3.104ms` | `+293.9%` |
| `IMAGE_RESIZE_565_8` | `0.635ms` | `0.625ms` | `-1.6%` |
| `EXTERN_IMAGE_RESIZE_565` | `0.507ms` | `0.507ms` | `+0.0%` |
| `EXTERN_IMAGE_RESIZE_565_1` | `0.678ms` | `0.109ms` | `-83.9%` |
| `EXTERN_IMAGE_RESIZE_565_2` | `0.840ms` | `0.109ms` | `-87.0%` |
| `EXTERN_IMAGE_RESIZE_565_4` | `0.854ms` | `0.110ms` | `-87.1%` |
| `EXTERN_IMAGE_RESIZE_565_8` | `0.698ms` | `0.685ms` | `-1.9%` |

- 这说明：
  - 基础优先保留的 `RECTANGLE / CIRCLE / ROUND_RECTANGLE / TEXT / IMAGE_565 / EXTERN_IMAGE_565` 完全不动
  - raw `IMAGE_RESIZE_565 / EXTERN_IMAGE_RESIZE_565` 也不动
  - 代价集中在 internal `RGB565_1/2/4 resize`
  - `RGB565_8 resize` 仍保持原有 fast path

### 运行/单测

- `HelloPerformance` runtime
  - 默认开启：`241 frames`，`ALL PASSED`
  - 宏关闭：`241 frames`，`ALL PASSED`
- `HelloUnitTest`
  - 默认开启：`688/688 passed`
  - 宏关闭：`688/688 passed`

### 结论

- 这颗宏满足：
  - 代码量收益 `>1KB`
  - 基础 `RECTANGLE / CIRCLE / ROUND_RECTANGLE / TEXT / IMAGE_565 / EXTERN_IMAGE_565` 不退化
  - raw `IMAGE_RESIZE_565 / EXTERN_IMAGE_RESIZE_565` 不退化
  - `RGB565_8 resize` 也不退化
- 因此：
  - 保留 `EGUI_CONFIG_IMAGE_STD_FAST_RESIZE_PACKED_ALPHA_ENABLE`
  - 默认值仍应保持开启
  - 它比 `EGUI_CONFIG_IMAGE_STD_FAST_RESIZE_ALPHA_ENABLE` 更适合作为“只牺牲 internal RGB565_1/2/4 resize”的 size-first 实验开关

## 本轮继续细拆 A/B: `std image mask-shape fast path`

### 测试环境

- 日期：`2026-04-03`
- 提交基线：`dabeeef`
- `HelloPerformance` code size A/B
  - `make -B -j APP=HelloPerformance PORT=qemu CPU_ARCH=cortex-m0plus APP_OBJ_SUFFIX=hpmaskshapeon`
  - `make -B -j APP=HelloPerformance PORT=qemu CPU_ARCH=cortex-m0plus APP_OBJ_SUFFIX=hpmaskshapeoff USER_CFLAGS=-DEGUI_CONFIG_IMAGE_STD_MASK_SHAPE_FAST_PATH_ENABLE=0`
- `HelloPerformance` perf A/B
  - `python scripts/perf_analysis/code_perf_check.py --profile cortex-m3 --threshold 1000 --filter RECTANGLE,CIRCLE,ROUND_RECTANGLE,TEXT,IMAGE_565,EXTERN_IMAGE_565,IMAGE_RESIZE_565,EXTERN_IMAGE_RESIZE_565,MASK_IMAGE_ROUND_RECT,MASK_IMAGE_CIRCLE,MASK_IMAGE_TEST_PERF_ROUND_RECT,MASK_IMAGE_TEST_PERF_CIRCLE,MASK_IMAGE_QOI_ROUND_RECT,MASK_IMAGE_QOI_CIRCLE,MASK_IMAGE_RLE_ROUND_RECT,MASK_IMAGE_RLE_CIRCLE,EXTERN_MASK_IMAGE_TEST_PERF_ROUND_RECT,EXTERN_MASK_IMAGE_TEST_PERF_CIRCLE,EXTERN_MASK_IMAGE_QOI_ROUND_RECT,EXTERN_MASK_IMAGE_QOI_CIRCLE,EXTERN_MASK_IMAGE_RLE_ROUND_RECT,EXTERN_MASK_IMAGE_RLE_CIRCLE`
  - `python scripts/perf_analysis/code_perf_check.py --profile cortex-m3 --threshold 1000 --filter RECTANGLE,CIRCLE,ROUND_RECTANGLE,TEXT,IMAGE_565,EXTERN_IMAGE_565,IMAGE_RESIZE_565,EXTERN_IMAGE_RESIZE_565,MASK_IMAGE_ROUND_RECT,MASK_IMAGE_CIRCLE,MASK_IMAGE_TEST_PERF_ROUND_RECT,MASK_IMAGE_TEST_PERF_CIRCLE,MASK_IMAGE_QOI_ROUND_RECT,MASK_IMAGE_QOI_CIRCLE,MASK_IMAGE_RLE_ROUND_RECT,MASK_IMAGE_RLE_CIRCLE,EXTERN_MASK_IMAGE_TEST_PERF_ROUND_RECT,EXTERN_MASK_IMAGE_TEST_PERF_CIRCLE,EXTERN_MASK_IMAGE_QOI_ROUND_RECT,EXTERN_MASK_IMAGE_QOI_CIRCLE,EXTERN_MASK_IMAGE_RLE_ROUND_RECT,EXTERN_MASK_IMAGE_RLE_CIRCLE --extra-cflags=-DEGUI_CONFIG_IMAGE_STD_MASK_SHAPE_FAST_PATH_ENABLE=0`
- 运行/单测验证
  - `python scripts/code_runtime_check.py --app HelloPerformance --timeout 10 --keep-screenshots`
  - `USER_CFLAGS=-DEGUI_CONFIG_IMAGE_STD_MASK_SHAPE_FAST_PATH_ENABLE=0 python scripts/code_runtime_check.py --app HelloPerformance --timeout 10 --keep-screenshots`
  - `make -j APP=HelloUnitTest PORT=pc_test APP_OBJ_SUFFIX=utmaskshapeon`
  - `make -j APP=HelloUnitTest PORT=pc_test APP_OBJ_SUFFIX=utmaskshapeoff USER_CFLAGS=-DEGUI_CONFIG_IMAGE_STD_MASK_SHAPE_FAST_PATH_ENABLE=0`
  - `output/main.exe`

### 代码量证据

- `HelloPerformance` 链接汇总：
  - 默认开启：`text=2056296 data=72 bss=3832`
  - 关闭 mask-shape fast path：`text=2043264 data=72 bss=3832`
- 结论：
  - `text -13032B`
  - `data/bss` 不变
- 符号证据：
  - 默认开启时能看到的大 helper 包括：
    - `egui_image_std_blend_rgb565_alpha8_round_rect_masked_row_fast_with_cache`：`0x0d20`
    - `egui_image_std_blend_rgb565_round_rect_masked_row_fast_with_cache`：`0x08a0`
    - `egui_image_std_blend_rgb565_circle_masked_row_fast`：`0x063e`
    - `egui_image_std_blend_rgb565_circle_masked_mapped_segment_fixed_row`：`0x041a`
    - `egui_image_std_blend_rgb565_alpha8_circle_masked_segment_fixed_row_direct`：`0x03cc`
    - `egui_image_std_blend_rgb565_alpha8_circle_masked_row_fast`：`0x0238`
  - 关闭宏后，上述 shape fast helper 从 `main_hpmaskshapeoff.elf` 中全部消失

### 性能证据

| 场景 | 默认开启 | 关闭 mask-shape fast path | 变化 |
| --- | ---: | ---: | ---: |
| `RECTANGLE` | `0.179ms` | `0.179ms` | `+0.0%` |
| `CIRCLE` | `0.810ms` | `0.810ms` | `+0.0%` |
| `ROUND_RECTANGLE` | `0.812ms` | `0.812ms` | `+0.0%` |
| `TEXT` | `1.026ms` | `1.026ms` | `+0.0%` |
| `IMAGE_565` | `0.239ms` | `0.238ms` | `-0.4%` |
| `EXTERN_IMAGE_565` | `0.417ms` | `0.413ms` | `-1.0%` |
| `IMAGE_RESIZE_565` | `0.459ms` | `0.458ms` | `-0.2%` |
| `EXTERN_IMAGE_RESIZE_565` | `0.506ms` | `0.506ms` | `+0.0%` |
| `MASK_IMAGE_ROUND_RECT` | `0.969ms` | `1.045ms` | `+7.8%` |
| `MASK_IMAGE_CIRCLE` | `1.425ms` | `1.570ms` | `+10.2%` |
| `MASK_IMAGE_TEST_PERF_ROUND_RECT` | `0.586ms` | `0.628ms` | `+7.2%` |
| `MASK_IMAGE_TEST_PERF_CIRCLE` | `0.908ms` | `1.222ms` | `+34.6%` |
| `MASK_IMAGE_QOI_ROUND_RECT` | `6.468ms` | `7.118ms` | `+10.0%` |
| `MASK_IMAGE_QOI_CIRCLE` | `6.614ms` | `12.567ms` | `+90.0%` |
| `MASK_IMAGE_QOI_8_CIRCLE` | `2.449ms` | `8.134ms` | `+232.1%` |
| `MASK_IMAGE_RLE_ROUND_RECT` | `2.715ms` | `3.365ms` | `+23.9%` |
| `MASK_IMAGE_RLE_CIRCLE` | `2.861ms` | `8.813ms` | `+208.0%` |
| `MASK_IMAGE_RLE_8_CIRCLE` | `1.719ms` | `7.404ms` | `+330.7%` |
| `EXTERN_MASK_IMAGE_QOI_ROUND_RECT` | `16.699ms` | `17.349ms` | `+3.9%` |
| `EXTERN_MASK_IMAGE_QOI_CIRCLE` | `16.844ms` | `22.797ms` | `+35.3%` |
| `EXTERN_MASK_IMAGE_RLE_ROUND_RECT` | `5.224ms` | `5.874ms` | `+12.4%` |
| `EXTERN_MASK_IMAGE_RLE_CIRCLE` | `5.370ms` | `11.322ms` | `+110.8%` |

- 这说明这轮拆掉的是 image 内部针对 `CIRCLE / ROUND_RECTANGLE` mask 的 specialized row / resize helper：
  - 当前优先保护的 raw `IMAGE_565 / EXTERN_IMAGE_565 / IMAGE_RESIZE_565 / EXTERN_IMAGE_RESIZE_565` 主路径都在噪声范围
  - round-rect masked image 多数是轻到中等回退
  - circle masked image，尤其 `QOI / RLE / alpha8` 组合，会明显回退到 generic path

### 运行/单测

- `HelloPerformance` runtime：
  - 默认开启：`241 frames`，`ALL PASSED`
  - 宏关闭：`241 frames`，`ALL PASSED`
- `HelloUnitTest`：
  - 默认开启：`688/688 passed`
  - 宏关闭：`688/688 passed`

### 结论

- 这条拆分满足当前优先目标：
  - 代码量收益 `>1KB`
  - `RECTANGLE / CIRCLE / ROUND_RECTANGLE / TEXT / IMAGE_565 / EXTERN_IMAGE_565 / IMAGE_RESIZE_565 / EXTERN_IMAGE_RESIZE_565` 性能回退 `<10%`
- 因此：
  - 保留 `EGUI_CONFIG_IMAGE_STD_MASK_SHAPE_FAST_PATH_ENABLE`
  - 默认值仍应保持开启
  - 它适合作为“只牺牲 circle / round-rectangle masked-image specialized path”的 size-first 实验开关

## 本轮继续细拆 A/B: `std image mask-shape child fast path`

### 测试环境

- 日期：`2026-04-03`
- 提交基线：`dabeeef`
- `HelloPerformance` code size A/B
  - `make -B -j APP=HelloPerformance PORT=qemu CPU_ARCH=cortex-m0plus APP_OBJ_SUFFIX=hpmaskspliton`
  - `make -B -j APP=HelloPerformance PORT=qemu CPU_ARCH=cortex-m0plus APP_OBJ_SUFFIX=hpmaskcircleoff USER_CFLAGS=-DEGUI_CONFIG_IMAGE_STD_MASK_CIRCLE_FAST_PATH_ENABLE=0`
  - `make -B -j APP=HelloPerformance PORT=qemu CPU_ARCH=cortex-m0plus APP_OBJ_SUFFIX=hpmaskroundrectoff USER_CFLAGS=-DEGUI_CONFIG_IMAGE_STD_MASK_ROUND_RECT_FAST_PATH_ENABLE=0`
- `HelloPerformance` perf A/B
  - `python scripts/perf_analysis/code_perf_check.py --profile cortex-m3 --threshold 1000 --filter RECTANGLE,CIRCLE,ROUND_RECTANGLE,TEXT,IMAGE_565,EXTERN_IMAGE_565,IMAGE_RESIZE_565,EXTERN_IMAGE_RESIZE_565,MASK_IMAGE_ROUND_RECT,MASK_IMAGE_CIRCLE,MASK_IMAGE_TEST_PERF_ROUND_RECT,MASK_IMAGE_TEST_PERF_CIRCLE,MASK_IMAGE_QOI_ROUND_RECT,MASK_IMAGE_QOI_CIRCLE,MASK_IMAGE_RLE_ROUND_RECT,MASK_IMAGE_RLE_CIRCLE,EXTERN_MASK_IMAGE_TEST_PERF_ROUND_RECT,EXTERN_MASK_IMAGE_TEST_PERF_CIRCLE,EXTERN_MASK_IMAGE_QOI_ROUND_RECT,EXTERN_MASK_IMAGE_QOI_CIRCLE,EXTERN_MASK_IMAGE_RLE_ROUND_RECT,EXTERN_MASK_IMAGE_RLE_CIRCLE`
  - `python scripts/perf_analysis/code_perf_check.py --profile cortex-m3 --threshold 1000 --filter RECTANGLE,CIRCLE,ROUND_RECTANGLE,TEXT,IMAGE_565,EXTERN_IMAGE_565,IMAGE_RESIZE_565,EXTERN_IMAGE_RESIZE_565,MASK_IMAGE_ROUND_RECT,MASK_IMAGE_CIRCLE,MASK_IMAGE_TEST_PERF_ROUND_RECT,MASK_IMAGE_TEST_PERF_CIRCLE,MASK_IMAGE_QOI_ROUND_RECT,MASK_IMAGE_QOI_CIRCLE,MASK_IMAGE_RLE_ROUND_RECT,MASK_IMAGE_RLE_CIRCLE,EXTERN_MASK_IMAGE_TEST_PERF_ROUND_RECT,EXTERN_MASK_IMAGE_TEST_PERF_CIRCLE,EXTERN_MASK_IMAGE_QOI_ROUND_RECT,EXTERN_MASK_IMAGE_QOI_CIRCLE,EXTERN_MASK_IMAGE_RLE_ROUND_RECT,EXTERN_MASK_IMAGE_RLE_CIRCLE --extra-cflags=-DEGUI_CONFIG_IMAGE_STD_MASK_CIRCLE_FAST_PATH_ENABLE=0`
  - `python scripts/perf_analysis/code_perf_check.py --profile cortex-m3 --threshold 1000 --filter RECTANGLE,CIRCLE,ROUND_RECTANGLE,TEXT,IMAGE_565,EXTERN_IMAGE_565,IMAGE_RESIZE_565,EXTERN_IMAGE_RESIZE_565,MASK_IMAGE_ROUND_RECT,MASK_IMAGE_CIRCLE,MASK_IMAGE_TEST_PERF_ROUND_RECT,MASK_IMAGE_TEST_PERF_CIRCLE,MASK_IMAGE_QOI_ROUND_RECT,MASK_IMAGE_QOI_CIRCLE,MASK_IMAGE_RLE_ROUND_RECT,MASK_IMAGE_RLE_CIRCLE,EXTERN_MASK_IMAGE_TEST_PERF_ROUND_RECT,EXTERN_MASK_IMAGE_TEST_PERF_CIRCLE,EXTERN_MASK_IMAGE_QOI_ROUND_RECT,EXTERN_MASK_IMAGE_QOI_CIRCLE,EXTERN_MASK_IMAGE_RLE_ROUND_RECT,EXTERN_MASK_IMAGE_RLE_CIRCLE --extra-cflags=-DEGUI_CONFIG_IMAGE_STD_MASK_ROUND_RECT_FAST_PATH_ENABLE=0`
- 运行/单测验证
  - `python scripts/code_runtime_check.py --app HelloPerformance --timeout 10 --keep-screenshots`
  - `USER_CFLAGS=-DEGUI_CONFIG_IMAGE_STD_MASK_CIRCLE_FAST_PATH_ENABLE=0 python scripts/code_runtime_check.py --app HelloPerformance --timeout 10 --keep-screenshots`
  - `USER_CFLAGS=-DEGUI_CONFIG_IMAGE_STD_MASK_ROUND_RECT_FAST_PATH_ENABLE=0 python scripts/code_runtime_check.py --app HelloPerformance --timeout 10 --keep-screenshots`
  - `make -j APP=HelloUnitTest PORT=pc_test APP_OBJ_SUFFIX=utmaskcircleoff USER_CFLAGS=-DEGUI_CONFIG_IMAGE_STD_MASK_CIRCLE_FAST_PATH_ENABLE=0`
  - `make -j APP=HelloUnitTest PORT=pc_test APP_OBJ_SUFFIX=utmaskroundrectoff USER_CFLAGS=-DEGUI_CONFIG_IMAGE_STD_MASK_ROUND_RECT_FAST_PATH_ENABLE=0`
  - `output/main.exe`

### 代码量证据

- `HelloPerformance` 链接汇总：
  - 默认开启：`text=2056296 data=72 bss=3832`
  - 关闭 `MASK_CIRCLE`：`text=2048928 data=72 bss=3832`
  - 关闭 `MASK_ROUND_RECT`：`text=2046756 data=72 bss=3832`
- 结论：
  - `EGUI_CONFIG_IMAGE_STD_MASK_CIRCLE_FAST_PATH_ENABLE=0`：`text -7368B`
  - `EGUI_CONFIG_IMAGE_STD_MASK_ROUND_RECT_FAST_PATH_ENABLE=0`：`text -9540B`
  - `data/bss` 不变

### 性能证据

| 场景 | 默认开启 | 关闭 `MASK_CIRCLE` | 变化 | 关闭 `MASK_ROUND_RECT` | 变化 |
| --- | ---: | ---: | ---: | ---: | ---: |
| `RECTANGLE` | `0.179ms` | `0.179ms` | `+0.0%` | `0.179ms` | `+0.0%` |
| `CIRCLE` | `0.810ms` | `0.810ms` | `+0.0%` | `0.810ms` | `+0.0%` |
| `ROUND_RECTANGLE` | `0.812ms` | `0.812ms` | `+0.0%` | `0.812ms` | `+0.0%` |
| `TEXT` | `1.026ms` | `1.026ms` | `+0.0%` | `1.026ms` | `+0.0%` |
| `IMAGE_565` | `0.239ms` | `0.239ms` | `+0.0%` | `0.239ms` | `+0.0%` |
| `EXTERN_IMAGE_565` | `0.417ms` | `0.417ms` | `+0.0%` | `0.417ms` | `+0.0%` |
| `IMAGE_RESIZE_565` | `0.459ms` | `0.459ms` | `+0.0%` | `0.458ms` | `-0.2%` |
| `EXTERN_IMAGE_RESIZE_565` | `0.506ms` | `0.507ms` | `+0.2%` | `0.507ms` | `+0.2%` |
| `MASK_IMAGE_ROUND_RECT` | `0.969ms` | `0.969ms` | `+0.0%` | `1.074ms` | `+10.8%` |
| `MASK_IMAGE_CIRCLE` | `1.425ms` | `1.588ms` | `+11.4%` | `1.426ms` | `+0.1%` |
| `MASK_IMAGE_TEST_PERF_ROUND_RECT` | `0.586ms` | `0.586ms` | `+0.0%` | `0.636ms` | `+8.5%` |
| `MASK_IMAGE_TEST_PERF_CIRCLE` | `0.908ms` | `1.228ms` | `+35.2%` | `0.905ms` | `-0.3%` |
| `MASK_IMAGE_QOI_ROUND_RECT` | `6.468ms` | `6.467ms` | `-0.0%` | `7.139ms` | `+10.4%` |
| `MASK_IMAGE_QOI_CIRCLE` | `6.614ms` | `12.587ms` | `+90.3%` | `6.611ms` | `-0.0%` |
| `MASK_IMAGE_QOI_8_CIRCLE` | `2.449ms` | `8.141ms` | `+232.4%` | `2.449ms` | `+0.0%` |
| `MASK_IMAGE_RLE_ROUND_RECT` | `2.715ms` | `2.713ms` | `-0.1%` | `3.386ms` | `+24.7%` |
| `MASK_IMAGE_RLE_CIRCLE` | `2.861ms` | `8.833ms` | `+208.7%` | `2.857ms` | `-0.1%` |
| `MASK_IMAGE_RLE_8_CIRCLE` | `1.719ms` | `7.411ms` | `+331.1%` | `1.719ms` | `+0.0%` |
| `EXTERN_MASK_IMAGE_QOI_ROUND_RECT` | `16.699ms` | `16.697ms` | `-0.0%` | `17.370ms` | `+4.0%` |
| `EXTERN_MASK_IMAGE_QOI_CIRCLE` | `16.844ms` | `22.817ms` | `+35.5%` | `16.841ms` | `-0.0%` |
| `EXTERN_MASK_IMAGE_QOI_8_CIRCLE` | `3.005ms` | `8.698ms` | `+189.5%` | `3.005ms` | `+0.0%` |
| `EXTERN_MASK_IMAGE_RLE_ROUND_RECT` | `5.224ms` | `5.223ms` | `-0.0%` | `5.895ms` | `+12.8%` |
| `EXTERN_MASK_IMAGE_RLE_CIRCLE` | `5.370ms` | `11.342ms` | `+111.2%` | `5.366ms` | `-0.1%` |
| `EXTERN_MASK_IMAGE_RLE_8_CIRCLE` | `2.512ms` | `8.204ms` | `+226.6%` | `2.512ms` | `+0.0%` |

- 这说明：
  - `MASK_CIRCLE` 和 `MASK_ROUND_RECT` 都没有伤到当前优先保护的基础主路径
  - `MASK_CIRCLE` 的代价集中在 circle masked-image，尤其 `QOI / RLE / alpha8 / external` 组合
  - `MASK_ROUND_RECT` 的代价集中在 round-rect masked-image，幅度明显小于 circle 分支

### 运行/单测

- `HelloPerformance` runtime：
  - 默认开启：`241 frames`，`ALL PASSED`
  - 关闭 `MASK_CIRCLE`：`241 frames`，`ALL PASSED`
  - 关闭 `MASK_ROUND_RECT`：`241 frames`，`ALL PASSED`
- `HelloUnitTest`：
  - 默认开启：`688/688 passed`
  - 关闭 `MASK_CIRCLE`：`688/688 passed`
  - 关闭 `MASK_ROUND_RECT`：`688/688 passed`

### 结论

- 保留粗粒度的 `EGUI_CONFIG_IMAGE_STD_MASK_SHAPE_FAST_PATH_ENABLE`，继续兼容“一键关掉全部 shape fast path”的老用法
- 继续保留两颗更细粒度子宏：
  - `EGUI_CONFIG_IMAGE_STD_MASK_CIRCLE_FAST_PATH_ENABLE`
  - `EGUI_CONFIG_IMAGE_STD_MASK_ROUND_RECT_FAST_PATH_ENABLE`
- 对 low-code size 项目：
  - 优先建议先尝试关闭 `EGUI_CONFIG_IMAGE_STD_MASK_ROUND_RECT_FAST_PATH_ENABLE`
  - 只有在 circle masked-image 不是热点时，再考虑关闭 `EGUI_CONFIG_IMAGE_STD_MASK_CIRCLE_FAST_PATH_ENABLE`

## 本轮继续细拆 A/B: `std image mask round-rect resize fast path`

### 测试环境

- 日期：`2026-04-04`
- 提交基线：`cb37711`
- `HelloPerformance` code size A/B
  - `make -B -j APP=HelloPerformance PORT=qemu CPU_ARCH=cortex-m0plus`
  - `make -B -j APP=HelloPerformance PORT=qemu CPU_ARCH=cortex-m0plus USER_CFLAGS=-DEGUI_CONFIG_IMAGE_STD_MASK_ROUND_RECT_RESIZE_FAST_PATH_ENABLE=0`
- `HelloPerformance` perf A/B
  - `python scripts/perf_analysis/code_perf_check.py --profile cortex-m3 --clean --filter RECTANGLE,CIRCLE,ROUND_RECTANGLE,TEXT,IMAGE_565,EXTERN_IMAGE_565,IMAGE_RESIZE_565,EXTERN_IMAGE_RESIZE_565,MASK_IMAGE_ROUND_RECT,MASK_IMAGE_ROUND_RECT_QUARTER,MASK_IMAGE_ROUND_RECT_DOUBLE,MASK_IMAGE_CIRCLE,MASK_IMAGE_CIRCLE_QUARTER,MASK_IMAGE_CIRCLE_DOUBLE,MASK_IMAGE_TEST_PERF_ROUND_RECT,MASK_IMAGE_QOI_ROUND_RECT,MASK_IMAGE_RLE_ROUND_RECT,EXTERN_MASK_IMAGE_TEST_PERF_ROUND_RECT,EXTERN_MASK_IMAGE_QOI_ROUND_RECT,EXTERN_MASK_IMAGE_RLE_ROUND_RECT`
  - `python scripts/perf_analysis/code_perf_check.py --profile cortex-m3 --clean --filter RECTANGLE,CIRCLE,ROUND_RECTANGLE,TEXT,IMAGE_565,EXTERN_IMAGE_565,IMAGE_RESIZE_565,EXTERN_IMAGE_RESIZE_565,MASK_IMAGE_ROUND_RECT,MASK_IMAGE_ROUND_RECT_QUARTER,MASK_IMAGE_ROUND_RECT_DOUBLE,MASK_IMAGE_CIRCLE,MASK_IMAGE_CIRCLE_QUARTER,MASK_IMAGE_CIRCLE_DOUBLE,MASK_IMAGE_TEST_PERF_ROUND_RECT,MASK_IMAGE_QOI_ROUND_RECT,MASK_IMAGE_RLE_ROUND_RECT,EXTERN_MASK_IMAGE_TEST_PERF_ROUND_RECT,EXTERN_MASK_IMAGE_QOI_ROUND_RECT,EXTERN_MASK_IMAGE_RLE_ROUND_RECT --extra-cflags=-DEGUI_CONFIG_IMAGE_STD_MASK_ROUND_RECT_RESIZE_FAST_PATH_ENABLE=0`
- 运行/单测验证
  - `python scripts/code_runtime_check.py --app HelloPerformance --timeout 10 --keep-screenshots`
  - `USER_CFLAGS=-DEGUI_CONFIG_IMAGE_STD_MASK_ROUND_RECT_RESIZE_FAST_PATH_ENABLE=0 python scripts/code_runtime_check.py --app HelloPerformance --timeout 10 --keep-screenshots`
  - `make -j APP=HelloUnitTest PORT=pc_test APP_OBJ_SUFFIX=utmaskrrresizebase`
  - `make -j APP=HelloUnitTest PORT=pc_test APP_OBJ_SUFFIX=utmaskrrresizeoff USER_CFLAGS=-DEGUI_CONFIG_IMAGE_STD_MASK_ROUND_RECT_RESIZE_FAST_PATH_ENABLE=0`
  - `output/main.exe`

### 代码量证据

- `HelloPerformance` 最终链接汇总：
  - baseline：`text=2056264 data=72 bss=3832`
  - 关闭 round-rect resize child：`text=2052500 data=72 bss=3832`
- 结论：
  - `text -3764B`
  - `data/bss` 不变
- 从当前实现归属看，这颗 child 只裁掉 internal round-rect masked-image resize 分支：
  - internal `RGB565` round-rect resize fast path
  - internal `RGB565_8` round-rect resize fast path
  - draw / QOI / RLE / external round-rect masked-image路径仍留在父宏 `EGUI_CONFIG_IMAGE_STD_MASK_ROUND_RECT_FAST_PATH_ENABLE` 下

### 性能证据

| 场景 | baseline | 关闭 round-rect resize child | 变化 |
| --- | ---: | ---: | ---: |
| `RECTANGLE` | `0.179ms` | `0.179ms` | `+0.0%` |
| `CIRCLE` | `0.810ms` | `0.810ms` | `+0.0%` |
| `ROUND_RECTANGLE` | `0.812ms` | `0.812ms` | `+0.0%` |
| `TEXT` | `1.027ms` | `1.027ms` | `+0.0%` |
| `IMAGE_565` | `0.239ms` | `0.239ms` | `+0.0%` |
| `EXTERN_IMAGE_565` | `0.417ms` | `0.417ms` | `+0.0%` |
| `IMAGE_RESIZE_565` | `0.459ms` | `0.458ms` | `-0.2%` |
| `EXTERN_IMAGE_RESIZE_565` | `0.507ms` | `0.507ms` | `+0.0%` |
| `MASK_IMAGE_ROUND_RECT` | `0.969ms` | `1.074ms` | `+10.8%` |
| `MASK_IMAGE_ROUND_RECT_QUARTER` | `0.326ms` | `0.351ms` | `+7.7%` |
| `MASK_IMAGE_ROUND_RECT_DOUBLE` | `0.943ms` | `1.041ms` | `+10.4%` |
| `MASK_IMAGE_TEST_PERF_ROUND_RECT` | `0.586ms` | `0.643ms` | `+9.7%` |
| `MASK_IMAGE_QOI_ROUND_RECT` | `6.468ms` | `6.468ms` | `+0.0%` |
| `MASK_IMAGE_RLE_ROUND_RECT` | `2.715ms` | `2.715ms` | `+0.0%` |
| `EXTERN_MASK_IMAGE_TEST_PERF_ROUND_RECT` | `0.684ms` | `0.684ms` | `+0.0%` |
| `EXTERN_MASK_IMAGE_QOI_ROUND_RECT` | `16.699ms` | `16.699ms` | `+0.0%` |
| `EXTERN_MASK_IMAGE_RLE_ROUND_RECT` | `5.224ms` | `5.224ms` | `+0.0%` |

- 这说明新的 child 宏已经把代价进一步收敛到 internal round-rect masked-image resize 场景：
  - 基础 `RECTANGLE / CIRCLE / ROUND_RECTANGLE / TEXT / IMAGE_565 / EXTERN_IMAGE_565 / IMAGE_RESIZE_565 / EXTERN_IMAGE_RESIZE_565` 都保持在噪声范围
  - `MASK_IMAGE_ROUND_RECT` / `MASK_IMAGE_ROUND_RECT_QUARTER` / `MASK_IMAGE_ROUND_RECT_DOUBLE` / `MASK_IMAGE_TEST_PERF_ROUND_RECT` 都是 resize 场景，回退符合 child 语义
  - round-rect 的 `QOI / RLE / external` 当前样本保持不动，说明 direct-draw 与外部资源路径没有被这颗 child 一起打掉

### 运行/单测

- `HelloPerformance` runtime
  - baseline：`241 frames`，`ALL PASSED`
  - 宏关闭：`241 frames`，`ALL PASSED`
  - baseline / off 的 `frame_0000.png` 和 `frame_0240.png` SHA256 完全一致
- `HelloUnitTest`
  - baseline：`688/688 passed`
  - 宏关闭：`688/688 passed`

### 结论

- 这颗 child 宏成立：
  - `text -3764B`
  - 基础主路径不退化
  - perf 代价只落在 internal round-rect masked-image resize 热点
- 因此：
  - 保留 `EGUI_CONFIG_IMAGE_STD_MASK_ROUND_RECT_RESIZE_FAST_PATH_ENABLE`
  - 默认值继续保持开启
  - 如果项目想优先回收 round-rect masked-image 相关代码、但又不想一起牺牲 round-rect 的 draw / QOI / RLE / external 热点，应先试这颗 child，再考虑更激进的 `EGUI_CONFIG_IMAGE_STD_MASK_ROUND_RECT_FAST_PATH_ENABLE`

## 本轮继续细拆 A/B: `std image mask circle resize fast path`

### 测试环境

- 日期：`2026-04-04`
- 提交基线：`27c1be5`
- `HelloPerformance` code size A/B
  - `make -B -j APP=HelloPerformance PORT=qemu CPU_ARCH=cortex-m0plus`
  - `make -B -j APP=HelloPerformance PORT=qemu CPU_ARCH=cortex-m0plus USER_CFLAGS=-DEGUI_CONFIG_IMAGE_STD_MASK_CIRCLE_RESIZE_FAST_PATH_ENABLE=0`
- `HelloPerformance` perf A/B
  - `python scripts/perf_analysis/code_perf_check.py --profile cortex-m3 --clean --filter RECTANGLE,CIRCLE,ROUND_RECTANGLE,TEXT,IMAGE_565,EXTERN_IMAGE_565,IMAGE_RESIZE_565,EXTERN_IMAGE_RESIZE_565,MASK_IMAGE_CIRCLE,MASK_IMAGE_CIRCLE_QUARTER,MASK_IMAGE_CIRCLE_DOUBLE,MASK_IMAGE_ROUND_RECT,MASK_IMAGE_ROUND_RECT_QUARTER,MASK_IMAGE_ROUND_RECT_DOUBLE,MASK_IMAGE_TEST_PERF_CIRCLE,MASK_IMAGE_QOI_CIRCLE,MASK_IMAGE_RLE_CIRCLE,EXTERN_MASK_IMAGE_TEST_PERF_CIRCLE,EXTERN_MASK_IMAGE_QOI_CIRCLE,EXTERN_MASK_IMAGE_RLE_CIRCLE`
  - `python scripts/perf_analysis/code_perf_check.py --profile cortex-m3 --clean --filter RECTANGLE,CIRCLE,ROUND_RECTANGLE,TEXT,IMAGE_565,EXTERN_IMAGE_565,IMAGE_RESIZE_565,EXTERN_IMAGE_RESIZE_565,MASK_IMAGE_CIRCLE,MASK_IMAGE_CIRCLE_QUARTER,MASK_IMAGE_CIRCLE_DOUBLE,MASK_IMAGE_ROUND_RECT,MASK_IMAGE_ROUND_RECT_QUARTER,MASK_IMAGE_ROUND_RECT_DOUBLE,MASK_IMAGE_TEST_PERF_CIRCLE,MASK_IMAGE_QOI_CIRCLE,MASK_IMAGE_RLE_CIRCLE,EXTERN_MASK_IMAGE_TEST_PERF_CIRCLE,EXTERN_MASK_IMAGE_QOI_CIRCLE,EXTERN_MASK_IMAGE_RLE_CIRCLE --extra-cflags=-DEGUI_CONFIG_IMAGE_STD_MASK_CIRCLE_RESIZE_FAST_PATH_ENABLE=0`
- 运行/单测验证
  - `python scripts/code_runtime_check.py --app HelloPerformance --timeout 10 --keep-screenshots`
  - `USER_CFLAGS=-DEGUI_CONFIG_IMAGE_STD_MASK_CIRCLE_RESIZE_FAST_PATH_ENABLE=0 python scripts/code_runtime_check.py --app HelloPerformance --timeout 10 --keep-screenshots`
  - `make -j APP=HelloUnitTest PORT=pc_test APP_OBJ_SUFFIX=utmaskcircleresizebase`
  - `make -j APP=HelloUnitTest PORT=pc_test APP_OBJ_SUFFIX=utmaskcircleresizeoff USER_CFLAGS=-DEGUI_CONFIG_IMAGE_STD_MASK_CIRCLE_RESIZE_FAST_PATH_ENABLE=0`
  - `output/main.exe`

### 代码量证据

- `HelloPerformance` 最终链接汇总：
  - baseline：`text=2056264 data=72 bss=3832`
  - 关闭 circle resize child：`text=2054280 data=72 bss=3832`
- 结论：
  - `text -1984B`
  - `data/bss` 不变
- 从当前实现归属看，这颗 child 只裁掉 internal circle masked-image resize 分支：
  - internal `RGB565` circle resize fast path
  - internal `RGB565_8` circle resize fast path
  - draw / QOI / RLE / external circle masked-image 路径仍留在父宏 `EGUI_CONFIG_IMAGE_STD_MASK_CIRCLE_FAST_PATH_ENABLE` 下

### 性能证据

| 场景 | baseline | 关闭 circle resize child | 变化 |
| --- | ---: | ---: | ---: |
| `RECTANGLE` | `0.179ms` | `0.179ms` | `+0.0%` |
| `CIRCLE` | `0.810ms` | `0.810ms` | `+0.0%` |
| `ROUND_RECTANGLE` | `0.812ms` | `0.812ms` | `+0.0%` |
| `TEXT` | `1.027ms` | `1.027ms` | `+0.0%` |
| `IMAGE_565` | `0.239ms` | `0.239ms` | `+0.0%` |
| `EXTERN_IMAGE_565` | `0.417ms` | `0.417ms` | `+0.0%` |
| `IMAGE_RESIZE_565` | `0.459ms` | `0.459ms` | `+0.0%` |
| `EXTERN_IMAGE_RESIZE_565` | `0.507ms` | `0.507ms` | `+0.0%` |
| `MASK_IMAGE_CIRCLE` | `1.431ms` | `1.609ms` | `+12.4%` |
| `MASK_IMAGE_CIRCLE_QUARTER` | `0.453ms` | `0.498ms` | `+9.9%` |
| `MASK_IMAGE_CIRCLE_DOUBLE` | `1.316ms` | `1.446ms` | `+9.9%` |
| `MASK_IMAGE_TEST_PERF_CIRCLE` | `0.904ms` | `1.231ms` | `+36.2%` |
| `MASK_IMAGE_ROUND_RECT` | `0.969ms` | `0.969ms` | `+0.0%` |
| `MASK_IMAGE_QOI_CIRCLE` | `6.614ms` | `6.614ms` | `+0.0%` |
| `MASK_IMAGE_RLE_CIRCLE` | `2.861ms` | `2.861ms` | `+0.0%` |
| `EXTERN_MASK_IMAGE_TEST_PERF_CIRCLE` | `1.286ms` | `1.287ms` | `+0.1%` |
| `EXTERN_MASK_IMAGE_QOI_CIRCLE` | `16.844ms` | `16.844ms` | `+0.0%` |
| `EXTERN_MASK_IMAGE_RLE_CIRCLE` | `5.370ms` | `5.370ms` | `+0.0%` |

- 这说明 circle resize child 的代价已经被隔离到 internal circle masked-image resize 热点：
  - 基础 `RECTANGLE / CIRCLE / ROUND_RECTANGLE / TEXT / IMAGE_565 / EXTERN_IMAGE_565 / IMAGE_RESIZE_565 / EXTERN_IMAGE_RESIZE_565` 全都在噪声范围
  - round-rect masked-image 路径没有被带伤
  - circle `QOI / RLE / external` 当前样本也没有被带伤

### 运行/单测

- `HelloPerformance` runtime
  - baseline：`241 frames`，`ALL PASSED`
  - `circle resize child off`：`241 frames`，`ALL PASSED`
  - 截图哈希一致：`frame_0000.png` / `frame_0240.png`
- `HelloUnitTest`
  - baseline：`688/688 passed`
  - `circle resize child off`：`688/688 passed`

### 结论

- 这颗 child 宏成立：
  - `text -1984B`
  - 基础主路径不退化
  - perf 代价只落在 internal circle masked-image resize 热点
- 因此：
  - 保留 `EGUI_CONFIG_IMAGE_STD_MASK_CIRCLE_RESIZE_FAST_PATH_ENABLE`
  - 默认值继续保持开启
  - 如果项目想优先回收 circle masked-image 相关代码、但又不想一起牺牲 circle 的 draw / QOI / RLE / external 热点，应先试这颗 child，再考虑更激进的 `EGUI_CONFIG_IMAGE_STD_MASK_CIRCLE_FAST_PATH_ENABLE`

## 本轮继续细拆 A/B: `std image mask circle resize alpha8 fast path`

### 测试环境

- 日期：`2026-04-04`
- 提交基线：`eb503a3`
- `HelloPerformance` code size A/B
  - `make -B -j APP=HelloPerformance PORT=qemu CPU_ARCH=cortex-m0plus USER_CFLAGS=-DEGUI_CONFIG_IMAGE_STD_MASK_CIRCLE_RESIZE_ALPHA8_FAST_PATH_ENABLE=1`
  - `make -B -j APP=HelloPerformance PORT=qemu CPU_ARCH=cortex-m0plus USER_CFLAGS=-DEGUI_CONFIG_IMAGE_STD_MASK_CIRCLE_RESIZE_ALPHA8_FAST_PATH_ENABLE=0`
- `HelloPerformance` perf A/B
  - `python scripts/perf_analysis/code_perf_check.py --profile cortex-m3 --threshold 1000 --timeout 300 --filter RECTANGLE,CIRCLE,ROUND_RECTANGLE,TEXT,IMAGE_565,EXTERN_IMAGE_565,IMAGE_RESIZE_565,EXTERN_IMAGE_RESIZE_565,MASK_IMAGE_CIRCLE,EXTERN_MASK_IMAGE_CIRCLE,MASK_IMAGE_CIRCLE_QUARTER,MASK_IMAGE_CIRCLE_DOUBLE,MASK_IMAGE_TEST_PERF_CIRCLE,EXTERN_MASK_IMAGE_TEST_PERF_CIRCLE,MASK_IMAGE_QOI_CIRCLE,MASK_IMAGE_QOI_8_CIRCLE,EXTERN_MASK_IMAGE_QOI_CIRCLE,EXTERN_MASK_IMAGE_QOI_8_CIRCLE,MASK_IMAGE_RLE_CIRCLE,MASK_IMAGE_RLE_8_CIRCLE,EXTERN_MASK_IMAGE_RLE_CIRCLE,EXTERN_MASK_IMAGE_RLE_8_CIRCLE,MASK_IMAGE_ROUND_RECT,EXTERN_MASK_IMAGE_ROUND_RECT,MASK_IMAGE_TEST_PERF_ROUND_RECT,EXTERN_MASK_IMAGE_TEST_PERF_ROUND_RECT,MASK_IMAGE_QOI_ROUND_RECT,MASK_IMAGE_QOI_8_ROUND_RECT,EXTERN_MASK_IMAGE_QOI_ROUND_RECT,EXTERN_MASK_IMAGE_QOI_8_ROUND_RECT,MASK_IMAGE_RLE_ROUND_RECT,MASK_IMAGE_RLE_8_ROUND_RECT,EXTERN_MASK_IMAGE_RLE_ROUND_RECT,EXTERN_MASK_IMAGE_RLE_8_ROUND_RECT --extra-cflags=-DEGUI_CONFIG_IMAGE_STD_MASK_CIRCLE_RESIZE_ALPHA8_FAST_PATH_ENABLE=1`
  - `python scripts/perf_analysis/code_perf_check.py --profile cortex-m3 --threshold 1000 --timeout 300 --filter RECTANGLE,CIRCLE,ROUND_RECTANGLE,TEXT,IMAGE_565,EXTERN_IMAGE_565,IMAGE_RESIZE_565,EXTERN_IMAGE_RESIZE_565,MASK_IMAGE_CIRCLE,EXTERN_MASK_IMAGE_CIRCLE,MASK_IMAGE_CIRCLE_QUARTER,MASK_IMAGE_CIRCLE_DOUBLE,MASK_IMAGE_TEST_PERF_CIRCLE,EXTERN_MASK_IMAGE_TEST_PERF_CIRCLE,MASK_IMAGE_QOI_CIRCLE,MASK_IMAGE_QOI_8_CIRCLE,EXTERN_MASK_IMAGE_QOI_CIRCLE,EXTERN_MASK_IMAGE_QOI_8_CIRCLE,MASK_IMAGE_RLE_CIRCLE,MASK_IMAGE_RLE_8_CIRCLE,EXTERN_MASK_IMAGE_RLE_CIRCLE,EXTERN_MASK_IMAGE_RLE_8_CIRCLE,MASK_IMAGE_ROUND_RECT,EXTERN_MASK_IMAGE_ROUND_RECT,MASK_IMAGE_TEST_PERF_ROUND_RECT,EXTERN_MASK_IMAGE_TEST_PERF_ROUND_RECT,MASK_IMAGE_QOI_ROUND_RECT,MASK_IMAGE_QOI_8_ROUND_RECT,EXTERN_MASK_IMAGE_QOI_ROUND_RECT,EXTERN_MASK_IMAGE_QOI_8_ROUND_RECT,MASK_IMAGE_RLE_ROUND_RECT,MASK_IMAGE_RLE_8_ROUND_RECT,EXTERN_MASK_IMAGE_RLE_ROUND_RECT,EXTERN_MASK_IMAGE_RLE_8_ROUND_RECT --extra-cflags=-DEGUI_CONFIG_IMAGE_STD_MASK_CIRCLE_RESIZE_ALPHA8_FAST_PATH_ENABLE=0`
- 运行/单测验证
  - `HelloPerformance` baseline / off 通过 `scripts.code_runtime_check.compile_app()` + `run_app()` 分别注入 `-DEGUI_CONFIG_IMAGE_STD_MASK_CIRCLE_RESIZE_ALPHA8_FAST_PATH_ENABLE=1/0`，录制目录为 `runtime_check_output/HelloPerformance/maskcircleresizea8_on` / `maskcircleresizea8_off`
  - `make -j APP=HelloUnitTest PORT=pc_test USER_CFLAGS=-DEGUI_CONFIG_IMAGE_STD_MASK_CIRCLE_RESIZE_ALPHA8_FAST_PATH_ENABLE=1`
  - `make -j APP=HelloUnitTest PORT=pc_test USER_CFLAGS=-DEGUI_CONFIG_IMAGE_STD_MASK_CIRCLE_RESIZE_ALPHA8_FAST_PATH_ENABLE=0`
  - `output/main.exe`

### 代码量证据

- `HelloPerformance` 最终链接汇总：
  - baseline：`text=2056264 data=72 bss=3832`
  - 关闭 `CIRCLE_RESIZE_ALPHA8` child：`text=2055724 data=72 bss=3832`
- 结论：
  - `text -540B`
  - `data/bss` 不变
- 从当前实现归属看，这颗 child 只裁掉 circle masked-image resize 里的 `RGB565_8` specialized path：
  - `egui_image_std_set_image_resize_rgb565_8_common()` 里的 circle alpha8 resize fast branch
  - `egui_image_std_blend_rgb565_alpha8_masked_mapped_segment()` 里 `src_x_map != NULL` 的 circle alpha8 mapped-segment fast branch
  - raw `RGB565` circle resize 继续留在父宏 `EGUI_CONFIG_IMAGE_STD_MASK_CIRCLE_RESIZE_FAST_PATH_ENABLE`

### 性能证据

| 场景 | baseline | 关闭 `CIRCLE_RESIZE_ALPHA8` child | 变化 |
| --- | ---: | ---: | ---: |
| `RECTANGLE` | `0.179ms` | `0.179ms` | `+0.0%` |
| `CIRCLE` | `0.810ms` | `0.810ms` | `+0.0%` |
| `ROUND_RECTANGLE` | `0.812ms` | `0.812ms` | `+0.0%` |
| `TEXT` | `1.027ms` | `1.027ms` | `+0.0%` |
| `IMAGE_565` | `0.239ms` | `0.239ms` | `+0.0%` |
| `EXTERN_IMAGE_565` | `0.417ms` | `0.417ms` | `+0.0%` |
| `IMAGE_RESIZE_565` | `0.459ms` | `0.459ms` | `+0.0%` |
| `EXTERN_IMAGE_RESIZE_565` | `0.507ms` | `0.507ms` | `+0.0%` |
| `MASK_IMAGE_CIRCLE` | `1.431ms` | `1.594ms` | `+11.4%` |
| `EXTERN_MASK_IMAGE_CIRCLE` | `1.494ms` | `1.658ms` | `+11.0%` |
| `MASK_IMAGE_CIRCLE_QUARTER` | `0.453ms` | `0.485ms` | `+7.1%` |
| `MASK_IMAGE_CIRCLE_DOUBLE` | `1.316ms` | `1.439ms` | `+9.3%` |
| `MASK_IMAGE_TEST_PERF_CIRCLE` | `0.904ms` | `0.904ms` | `+0.0%` |
| `EXTERN_MASK_IMAGE_TEST_PERF_CIRCLE` | `1.286ms` | `1.286ms` | `+0.0%` |
| `MASK_IMAGE_QOI_CIRCLE` | `6.614ms` | `6.614ms` | `+0.0%` |
| `MASK_IMAGE_QOI_8_CIRCLE` | `2.449ms` | `2.449ms` | `+0.0%` |
| `EXTERN_MASK_IMAGE_QOI_CIRCLE` | `16.844ms` | `16.844ms` | `+0.0%` |
| `EXTERN_MASK_IMAGE_QOI_8_CIRCLE` | `3.005ms` | `3.005ms` | `+0.0%` |
| `MASK_IMAGE_RLE_CIRCLE` | `2.861ms` | `2.861ms` | `+0.0%` |
| `MASK_IMAGE_RLE_8_CIRCLE` | `1.719ms` | `1.719ms` | `+0.0%` |
| `EXTERN_MASK_IMAGE_RLE_CIRCLE` | `5.370ms` | `5.370ms` | `+0.0%` |
| `EXTERN_MASK_IMAGE_RLE_8_CIRCLE` | `2.512ms` | `2.512ms` | `+0.0%` |
| `MASK_IMAGE_ROUND_RECT` | `0.969ms` | `0.969ms` | `+0.0%` |
| `EXTERN_MASK_IMAGE_ROUND_RECT` | `1.035ms` | `1.036ms` | `+0.1%` |
| `MASK_IMAGE_TEST_PERF_ROUND_RECT` | `0.586ms` | `0.586ms` | `+0.0%` |
| `EXTERN_MASK_IMAGE_TEST_PERF_ROUND_RECT` | `0.684ms` | `0.684ms` | `+0.0%` |

- 这说明新的 child 语义是成立的：
  - 基础 `RECTANGLE / CIRCLE / ROUND_RECTANGLE / TEXT / IMAGE_565 / EXTERN_IMAGE_565 / IMAGE_RESIZE_565 / EXTERN_IMAGE_RESIZE_565` 全都在噪声范围
  - raw `RGB565` circle resize 样本 `MASK_IMAGE_TEST_PERF_CIRCLE / EXTERN_MASK_IMAGE_TEST_PERF_CIRCLE` 维持不变
  - circle draw、`QOI / RLE / external` 对照组也都维持不变
  - perf 代价只落在 circle 的 alpha8 masked-image resize 热点

### 运行/单测

- `HelloPerformance` runtime
  - baseline / off 都是 `241 frames`
  - 文件 hash mismatch `4` 帧：`frame_0168.png` / `frame_0172.png` / `frame_0178.png` / `frame_0179.png`
  - 全量像素级对比 `pixel mismatch 0`
- `HelloUnitTest`
  - baseline：`688/688 passed`
  - `CIRCLE_RESIZE_ALPHA8 child off`：`688/688 passed`

### 结论

- 这条继续细拆在语义隔离上成立：
  - 基础主路径不退化
  - raw circle resize 继续保留
  - perf 代价只落在 circle 的 alpha8 masked-image resize 热点
- 但它不进入当前优先保留宏序列：
  - code size 收益只有 `540B`，低于 `1KB`
- 因此：
  - 这条细拆记录保留，但当前只作为低优先级的条件实验宏
  - `EGUI_CONFIG_IMAGE_STD_MASK_CIRCLE_RESIZE_ALPHA8_FAST_PATH_ENABLE` 默认值仍保持开启
  - 如果项目极度想保住 raw circle resize，可先试 `CIRCLE_RESIZE_ALPHA8`；否则当前更优先的仍是父宏 `EGUI_CONFIG_IMAGE_STD_MASK_CIRCLE_RESIZE_FAST_PATH_ENABLE`

## 本轮继续细拆 A/B: `std image mask circle resize rgb565 fast path`

### 测试环境

- 日期：`2026-04-05`
- 提交基线：`56c45cc`
- 新增 child split
  - `EGUI_CONFIG_IMAGE_STD_MASK_CIRCLE_RESIZE_RGB565_FAST_PATH_ENABLE`
- `HelloPerformance` perf A/B
  - on 结果：`.claude/perf_mask_circle_resize_rgb565_on_results.json`
  - off 结果：`.claude/perf_mask_circle_resize_rgb565_off_results.json`
- 运行 / 单测验证
  - `HelloPerformance` runtime 录制输出：
    - `runtime_check_output/HelloPerformance/mask_circle_resize_rgb565_on`
    - `runtime_check_output/HelloPerformance/mask_circle_resize_rgb565_off`
  - `HelloUnitTest` on / off 都已跑通

### 代码量证据

- `HelloPerformance` 最终链接汇总：
  - on：`text=2055380 data=72 bss=3832`
  - off：`text=2053924 data=72 bss=3832`
- 结论：
  - `text -1456B`
  - `data/bss` 不变
- 从当前实现归属看，这颗 child 只裁掉 raw `RGB565` circle masked-image resize specialized helpers：
  - `egui_image_std_blend_rgb565_circle_masked_left_segment_fixed_row()`
  - `egui_image_std_blend_rgb565_circle_masked_right_segment_fixed_row()`
  - `egui_image_std_blend_rgb565_circle_masked_mapped_segment_fixed_row()`
  - raw `RGB565` resize主路径里依赖 `circle_row_ready` 的 circle-edge fast branch
  - `RGB565_8` circle resize helper 继续留在 `EGUI_CONFIG_IMAGE_STD_MASK_CIRCLE_RESIZE_ALPHA8_FAST_PATH_ENABLE`

### 性能证据

- `HelloPerformance` 完整 `239` 场景对比
  - `>=10%` 回退：`1`
  - `<=-10%` 改善：`0`
- 基础主路径维持在噪声范围：
  - `RECTANGLE +0.0%`
  - `CIRCLE +0.0%`
  - `ROUND_RECTANGLE +0.0%`
  - `TEXT +0.0%`
  - `IMAGE_565 +0.0%`
  - `EXTERN_IMAGE_565 +0.0%`
  - `IMAGE_RESIZE_565 +0.0%`
  - `EXTERN_IMAGE_RESIZE_565 +0.0%`
- alpha8 circle resize 与 circle draw / codec / external 对照组继续稳定：
  - `MASK_IMAGE_CIRCLE -0.1%`
  - `EXTERN_MASK_IMAGE_CIRCLE -0.3%`
  - `MASK_IMAGE_CIRCLE_QUARTER +0.0%`
  - `MASK_IMAGE_CIRCLE_DOUBLE -0.1%`
  - `EXTERN_MASK_IMAGE_TEST_PERF_CIRCLE +0.0%`
  - `MASK_IMAGE_QOI_8_CIRCLE +0.0%`
  - `EXTERN_MASK_IMAGE_QOI_8_CIRCLE +0.0%`
  - `MASK_IMAGE_RLE_8_CIRCLE +0.0%`
  - `EXTERN_MASK_IMAGE_RLE_8_CIRCLE +0.0%`
- 但 raw sampled circle resize hotspot 仍明显回退：
  - `MASK_IMAGE_TEST_PERF_CIRCLE +36.2%`

### 运行 / 单测

- `HelloPerformance` runtime
  - on：`241 frames`，`ALL PASSED`
  - off：`241 frames`，`ALL PASSED`
  - on / off 全量 `241` 帧 PNG hash mismatch `1`
  - on / off 全量 `241` 帧 pixel mismatch `1`
  - 差异帧：`frame_0190.png`
- `HelloUnitTest`
  - on：`688/688 passed`
  - off：`688/688 passed`

### 结论

- 这颗 child 宏仍有独立回收价值：
  - `text -1456B`
  - 基础主路径不退化
  - alpha8 circle resize 与 circle draw / codec / external 对照组继续稳定
- 但它不进入当前优先保留序列：
  - 完整 `239` 场景里仍有 `MASK_IMAGE_TEST_PERF_CIRCLE +36.2%`
  - runtime on / off 存在 `frame_0190.png` 的 `1` 帧真实像素差
- 因此：
  - 保留 `EGUI_CONFIG_IMAGE_STD_MASK_CIRCLE_RESIZE_RGB565_FAST_PATH_ENABLE` 的实验记录
  - 默认值继续保持开启
  - 如果项目更想保住 alpha8 circle resize，并且明确不依赖 raw sampled circle resize hotspot，且能接受这 `1` 帧像素差，可在父宏之前单独试这颗 child
  - circle resize 这条线现在也已经有 `alpha8` / `rgb565` 两颗互补 child，但 `rgb565` 这颗当前只适合作为实验记录，不进入主推荐序列

## 本轮继续细拆 A/B: `std image mask circle draw fast path`

### 测试环境

- 日期：`2026-04-04`
- 提交基线：`0d84a99`
- `HelloPerformance` code size A/B
  - `make -B -j APP=HelloPerformance PORT=qemu CPU_ARCH=cortex-m0plus`
  - `make -B -j APP=HelloPerformance PORT=qemu CPU_ARCH=cortex-m0plus USER_CFLAGS=-DEGUI_CONFIG_IMAGE_STD_MASK_CIRCLE_DRAW_FAST_PATH_ENABLE=0`
- `HelloPerformance` perf A/B
  - `python scripts/perf_analysis/code_perf_check.py --profile cortex-m3 --clean --filter RECTANGLE,CIRCLE,ROUND_RECTANGLE,TEXT,IMAGE_565,EXTERN_IMAGE_565,IMAGE_RESIZE_565,EXTERN_IMAGE_RESIZE_565,MASK_IMAGE_CIRCLE,MASK_IMAGE_CIRCLE_QUARTER,MASK_IMAGE_CIRCLE_DOUBLE,MASK_IMAGE_TEST_PERF_CIRCLE,MASK_IMAGE_QOI_CIRCLE,MASK_IMAGE_RLE_CIRCLE,EXTERN_MASK_IMAGE_TEST_PERF_CIRCLE,EXTERN_MASK_IMAGE_QOI_CIRCLE,EXTERN_MASK_IMAGE_RLE_CIRCLE,MASK_IMAGE_ROUND_RECT,MASK_IMAGE_ROUND_RECT_QUARTER,MASK_IMAGE_ROUND_RECT_DOUBLE,MASK_IMAGE_TEST_PERF_ROUND_RECT,MASK_IMAGE_QOI_ROUND_RECT,MASK_IMAGE_RLE_ROUND_RECT,EXTERN_MASK_IMAGE_TEST_PERF_ROUND_RECT,EXTERN_MASK_IMAGE_QOI_ROUND_RECT,EXTERN_MASK_IMAGE_RLE_ROUND_RECT`
  - `python scripts/perf_analysis/code_perf_check.py --profile cortex-m3 --clean --filter RECTANGLE,CIRCLE,ROUND_RECTANGLE,TEXT,IMAGE_565,EXTERN_IMAGE_565,IMAGE_RESIZE_565,EXTERN_IMAGE_RESIZE_565,MASK_IMAGE_CIRCLE,MASK_IMAGE_CIRCLE_QUARTER,MASK_IMAGE_CIRCLE_DOUBLE,MASK_IMAGE_TEST_PERF_CIRCLE,MASK_IMAGE_QOI_CIRCLE,MASK_IMAGE_RLE_CIRCLE,EXTERN_MASK_IMAGE_TEST_PERF_CIRCLE,EXTERN_MASK_IMAGE_QOI_CIRCLE,EXTERN_MASK_IMAGE_RLE_CIRCLE,MASK_IMAGE_ROUND_RECT,MASK_IMAGE_ROUND_RECT_QUARTER,MASK_IMAGE_ROUND_RECT_DOUBLE,MASK_IMAGE_TEST_PERF_ROUND_RECT,MASK_IMAGE_QOI_ROUND_RECT,MASK_IMAGE_RLE_ROUND_RECT,EXTERN_MASK_IMAGE_TEST_PERF_ROUND_RECT,EXTERN_MASK_IMAGE_QOI_ROUND_RECT,EXTERN_MASK_IMAGE_RLE_ROUND_RECT --extra-cflags=-DEGUI_CONFIG_IMAGE_STD_MASK_CIRCLE_DRAW_FAST_PATH_ENABLE=0`
- 运行/单测验证
  - `python scripts/code_runtime_check.py --app HelloPerformance --timeout 10 --keep-screenshots`
  - `USER_CFLAGS=-DEGUI_CONFIG_IMAGE_STD_MASK_CIRCLE_DRAW_FAST_PATH_ENABLE=0 python scripts/code_runtime_check.py --app HelloPerformance --timeout 10 --keep-screenshots`
  - `make -j APP=HelloUnitTest PORT=pc_test APP_OBJ_SUFFIX=utmaskcircledrawbase`
  - `make -j APP=HelloUnitTest PORT=pc_test APP_OBJ_SUFFIX=utmaskcircledrawoff USER_CFLAGS=-DEGUI_CONFIG_IMAGE_STD_MASK_CIRCLE_DRAW_FAST_PATH_ENABLE=0`
  - `output/main.exe`

### 代码量证据

- `HelloPerformance` 最终链接汇总：
  - baseline：`text=2056264 data=72 bss=3832`
  - 关闭 circle draw child：`text=2053236 data=72 bss=3832`
- 结论：
  - `text -3028B`
  - `data/bss` 不变
- 从当前实现归属看，这颗 child 只裁掉 circle masked-image direct-draw 分支：
  - `egui_image_std_blend_rgb565_masked_row()` / `row_block()` 对 circle 的 specialized path
  - `egui_image_std_blend_rgb565_alpha8_masked_row()` / `row_block()` 对 circle 的 specialized path
  - circle masked-image resize helpers 仍留在 `EGUI_CONFIG_IMAGE_STD_MASK_CIRCLE_RESIZE_FAST_PATH_ENABLE`

### 性能证据

| 场景 | baseline | 关闭 circle draw child | 变化 |
| --- | ---: | ---: | ---: |
| `RECTANGLE` | `0.179ms` | `0.179ms` | `+0.0%` |
| `CIRCLE` | `0.810ms` | `0.810ms` | `+0.0%` |
| `ROUND_RECTANGLE` | `0.812ms` | `0.812ms` | `+0.0%` |
| `TEXT` | `1.027ms` | `1.027ms` | `+0.0%` |
| `IMAGE_565` | `0.239ms` | `0.239ms` | `+0.0%` |
| `EXTERN_IMAGE_565` | `0.417ms` | `0.417ms` | `+0.0%` |
| `IMAGE_RESIZE_565` | `0.459ms` | `0.458ms` | `-0.2%` |
| `EXTERN_IMAGE_RESIZE_565` | `0.507ms` | `0.507ms` | `+0.0%` |
| `MASK_IMAGE_CIRCLE` | `1.431ms` | `1.432ms` | `+0.1%` |
| `MASK_IMAGE_CIRCLE_QUARTER` | `0.453ms` | `0.452ms` | `-0.2%` |
| `MASK_IMAGE_CIRCLE_DOUBLE` | `1.316ms` | `1.317ms` | `+0.1%` |
| `MASK_IMAGE_TEST_PERF_CIRCLE` | `0.904ms` | `0.909ms` | `+0.6%` |
| `MASK_IMAGE_QOI_CIRCLE` | `6.614ms` | `12.587ms` | `+90.3%` |
| `MASK_IMAGE_RLE_CIRCLE` | `2.861ms` | `8.833ms` | `+208.7%` |
| `EXTERN_MASK_IMAGE_TEST_PERF_CIRCLE` | `1.286ms` | `1.286ms` | `+0.0%` |
| `EXTERN_MASK_IMAGE_QOI_CIRCLE` | `16.844ms` | `22.817ms` | `+35.5%` |
| `EXTERN_MASK_IMAGE_RLE_CIRCLE` | `5.370ms` | `11.342ms` | `+111.2%` |
| `MASK_IMAGE_ROUND_RECT` | `0.969ms` | `0.969ms` | `+0.0%` |
| `MASK_IMAGE_TEST_PERF_ROUND_RECT` | `0.586ms` | `0.587ms` | `+0.2%` |
| `MASK_IMAGE_QOI_ROUND_RECT` | `6.468ms` | `6.467ms` | `+0.0%` |
| `MASK_IMAGE_RLE_ROUND_RECT` | `2.715ms` | `2.713ms` | `-0.1%` |
| `EXTERN_MASK_IMAGE_TEST_PERF_ROUND_RECT` | `0.684ms` | `0.684ms` | `+0.0%` |
| `EXTERN_MASK_IMAGE_QOI_ROUND_RECT` | `16.699ms` | `16.697ms` | `+0.0%` |
| `EXTERN_MASK_IMAGE_RLE_ROUND_RECT` | `5.224ms` | `5.223ms` | `+0.0%` |

- 这说明 circle draw child 的代价已经被隔离到 circle 的 codec / external draw 热点：
  - 基础 `RECTANGLE / CIRCLE / ROUND_RECTANGLE / TEXT / IMAGE_565 / EXTERN_IMAGE_565 / IMAGE_RESIZE_565 / EXTERN_IMAGE_RESIZE_565` 全都在噪声范围
  - internal circle masked-image resize 样本维持不变
  - round-rect 对照组维持不变

### 运行/单测

- `HelloPerformance` runtime
  - baseline：`241 frames`，`ALL PASSED`
  - `circle draw child off`：`241 frames`，`ALL PASSED`
  - 截图哈希一致：`frame_0000.png` / `frame_0240.png`
- `HelloUnitTest`
  - baseline：`688/688 passed`
  - `circle draw child off`：`688/688 passed`

### 结论

- 这颗 child 宏成立：
  - `text -3028B`
  - 基础主路径不退化
  - perf 代价只落在 circle 的 codec / external masked-image draw 热点
- 因此：
  - 保留 `EGUI_CONFIG_IMAGE_STD_MASK_CIRCLE_DRAW_FAST_PATH_ENABLE`
  - 默认值继续保持开启
  - 如果项目想保住 internal circle masked-image resize，但可以接受牺牲 circle 的 QOI / RLE / external draw 热点，应先试这颗 child，再考虑更激进的 `EGUI_CONFIG_IMAGE_STD_MASK_CIRCLE_FAST_PATH_ENABLE`

## 本轮继续细拆 A/B: `std image mask circle draw alpha8 fast path`

### 测试环境

- 日期：`2026-04-04`
- 提交基线：`9a35a49`
- `HelloPerformance` perf A/B
  - baseline 结果：`.claude/perf_mask_circle_draw_alpha8_base_results.json`
  - `CIRCLE_DRAW_ALPHA8 child off`：`.claude/perf_mask_circle_draw_alpha8_off_results.json`
- 运行 / 单测验证
  - `HelloPerformance` runtime 录制输出：
    - `runtime_check_output/HelloPerformance/maskcircledrawa8_on`
    - `runtime_check_output/HelloPerformance/maskcircledrawa8_off`
  - `HelloUnitTest` baseline / off 都已跑通

### 代码量证据

- `HelloPerformance` 最终链接汇总：
  - baseline：`text=2056264 data=72 bss=3832`
  - 关闭 `CIRCLE_DRAW_ALPHA8` child：`text=2054388 data=72 bss=3832`
- 结论：
  - `text -1876B`
  - `data/bss` 不变
- 从当前实现归属看，这颗 child 只裁掉 circle masked-image direct-draw 里的 `RGB565_8` / alpha8 specialized path：
  - `egui_image_std_blend_rgb565_alpha8_circle_masked_row_fast()`
  - alpha8 row / row_block 对 circle 的 direct-draw dispatch
  - `egui_image_std_blend_rgb565_alpha8_masked_mapped_segment()` 里 `src_x_map == NULL` 的 circle fast branch
  - raw `RGB565` circle direct-draw helper 和 circle resize alpha8 helper 继续留在原父宏 / resize 宏上

### 性能证据

| 场景 | baseline | 关闭 `CIRCLE_DRAW_ALPHA8` child | 变化 |
| --- | ---: | ---: | ---: |
| `RECTANGLE` | `0.179ms` | `0.179ms` | `+0.0%` |
| `CIRCLE` | `0.810ms` | `0.810ms` | `+0.0%` |
| `ROUND_RECTANGLE` | `0.812ms` | `0.812ms` | `+0.0%` |
| `TEXT` | `1.027ms` | `1.027ms` | `+0.0%` |
| `IMAGE_565` | `0.239ms` | `0.239ms` | `+0.0%` |
| `EXTERN_IMAGE_565` | `0.417ms` | `0.417ms` | `+0.0%` |
| `IMAGE_RESIZE_565` | `0.459ms` | `0.459ms` | `+0.0%` |
| `EXTERN_IMAGE_RESIZE_565` | `0.507ms` | `0.507ms` | `+0.0%` |
| `MASK_IMAGE_CIRCLE` | `1.431ms` | `1.433ms` | `+0.1%` |
| `EXTERN_MASK_IMAGE_CIRCLE` | `1.494ms` | `1.496ms` | `+0.1%` |
| `MASK_IMAGE_CIRCLE_QUARTER` | `0.453ms` | `0.454ms` | `+0.2%` |
| `MASK_IMAGE_CIRCLE_DOUBLE` | `1.316ms` | `1.317ms` | `+0.1%` |
| `MASK_IMAGE_TEST_PERF_CIRCLE` | `0.904ms` | `0.904ms` | `+0.0%` |
| `EXTERN_MASK_IMAGE_TEST_PERF_CIRCLE` | `1.286ms` | `1.286ms` | `+0.0%` |
| `MASK_IMAGE_QOI_CIRCLE` | `6.614ms` | `6.614ms` | `+0.0%` |
| `MASK_IMAGE_QOI_8_CIRCLE` | `2.449ms` | `8.142ms` | `+232.5%` |
| `EXTERN_MASK_IMAGE_QOI_CIRCLE` | `16.844ms` | `16.844ms` | `+0.0%` |
| `EXTERN_MASK_IMAGE_QOI_8_CIRCLE` | `3.005ms` | `8.698ms` | `+189.5%` |
| `MASK_IMAGE_RLE_CIRCLE` | `2.861ms` | `2.861ms` | `+0.0%` |
| `MASK_IMAGE_RLE_8_CIRCLE` | `1.719ms` | `7.411ms` | `+331.1%` |
| `EXTERN_MASK_IMAGE_RLE_CIRCLE` | `5.370ms` | `5.370ms` | `+0.0%` |
| `EXTERN_MASK_IMAGE_RLE_8_CIRCLE` | `2.512ms` | `8.204ms` | `+226.6%` |
| `MASK_IMAGE_ROUND_RECT` | `0.969ms` | `0.969ms` | `+0.0%` |
| `EXTERN_MASK_IMAGE_ROUND_RECT` | `1.035ms` | `1.036ms` | `+0.1%` |
| `MASK_IMAGE_TEST_PERF_ROUND_RECT` | `0.586ms` | `0.586ms` | `+0.0%` |
| `EXTERN_MASK_IMAGE_TEST_PERF_ROUND_RECT` | `0.684ms` | `0.684ms` | `+0.0%` |
| `MASK_IMAGE_QOI_8_ROUND_RECT` | `2.356ms` | `2.355ms` | `-0.0%` |
| `EXTERN_MASK_IMAGE_QOI_8_ROUND_RECT` | `2.912ms` | `2.911ms` | `-0.0%` |
| `MASK_IMAGE_RLE_8_ROUND_RECT` | `1.626ms` | `1.625ms` | `-0.1%` |
| `EXTERN_MASK_IMAGE_RLE_8_ROUND_RECT` | `2.419ms` | `2.418ms` | `-0.0%` |

- 这说明 `CIRCLE_DRAW` 还能继续切出一颗更保守的 alpha8 child：
  - 基础 `RECTANGLE / CIRCLE / ROUND_RECTANGLE / TEXT / IMAGE_565 / EXTERN_IMAGE_565 / IMAGE_RESIZE_565 / EXTERN_IMAGE_RESIZE_565` 全都在噪声范围
  - raw circle draw / resize 与 non-alpha8 `QOI / RLE / external` 样本维持不变
  - round-rect 对照组维持不变
  - 代价只落在 circle 的 alpha8 masked-image direct-draw 热点

### 运行 / 单测

- `HelloPerformance` runtime
  - baseline：`241 frames`，录制通过
  - `CIRCLE_DRAW_ALPHA8 child off`：`241 frames`，录制通过
  - baseline / off 全量 `241` 帧像素级比对 `pixel mismatch 0`
  - 文件级 PNG hash 有 `4` 帧不一致：`frame_0208.png` / `frame_0216.png` / `frame_0230.png` / `frame_0238.png`
  - 上述 `4` 帧做像素比对后仍完全一致，可视为 PNG 文件级抖动而非渲染差异
  - 抽查 `frame_0000.png` / `frame_0120.png` / `frame_0240.png` hash 一致
- `HelloUnitTest`
  - baseline：`688/688 passed`
  - `CIRCLE_DRAW_ALPHA8 child off`：`688/688 passed`

### 结论

- 这颗 child 宏成立：
  - `text -1876B`
  - 基础主路径不退化
  - perf 代价只落在 circle 的 alpha8 masked-image draw 热点
- 因此：
  - 保留 `EGUI_CONFIG_IMAGE_STD_MASK_CIRCLE_DRAW_ALPHA8_FAST_PATH_ENABLE`
  - 默认值继续保持开启
  - 如果项目要保住 raw circle masked-image draw，应优先试这颗 child，再考虑父宏 `EGUI_CONFIG_IMAGE_STD_MASK_CIRCLE_DRAW_FAST_PATH_ENABLE`

## 本轮继续细拆 A/B: `std image mask circle draw rgb565 fast path`

### 测试环境

- 日期：`2026-04-05`
- 提交基线：`c1431cc`
- 新增 child split
  - `EGUI_CONFIG_IMAGE_STD_MASK_CIRCLE_DRAW_RGB565_FAST_PATH_ENABLE`
- `HelloPerformance` perf A/B
  - on 结果：`.claude/perf_mask_circle_draw_rgb565_on_results.json`
  - off 结果：`.claude/perf_mask_circle_draw_rgb565_off_results.json`
- 运行 / 单测验证
  - `HelloPerformance` runtime 录制输出：
    - `runtime_check_output/HelloPerformance/mask_circle_draw_rgb565_on`
    - `runtime_check_output/HelloPerformance/mask_circle_draw_rgb565_off`
  - `HelloUnitTest` on / off 都已跑通

### 代码量证据

- `HelloPerformance` 最终链接汇总：
  - on：`text=2055380 data=72 bss=3832`
  - off：`text=2054180 data=72 bss=3832`
- 结论：
  - `text -1200B`
  - `data/bss` 不变
- 从当前实现归属看，这颗 child 只裁掉 circle masked-image direct-draw 里的 non-alpha8 specialized helpers：
  - `egui_image_std_blend_rgb565_circle_masked_row_fast()` / `row_block()` 对 circle 的 specialized path
  - raw/non-alpha8 circle direct-draw 的 left/right fixed-row helpers
  - `RGB565_8` circle direct-draw / row-block specialized helpers 继续留在 `EGUI_CONFIG_IMAGE_STD_MASK_CIRCLE_DRAW_ALPHA8_FAST_PATH_ENABLE`

### 性能证据

- `HelloPerformance` 完整 `239` 场景对比
  - `>=10%` 回退：`0`
  - `<=-10%` 改善：`0`
- 基础主路径维持在噪声范围：
  - `RECTANGLE +0.0%`
  - `CIRCLE +0.0%`
  - `ROUND_RECTANGLE +0.0%`
  - `TEXT +0.0%`
  - `IMAGE_565 +0.0%`
  - `EXTERN_IMAGE_565 +0.0%`
  - `IMAGE_RESIZE_565 +0.0%`
  - `EXTERN_IMAGE_RESIZE_565 +0.0%`
- circle non-alpha8 draw 热点仍低于 `10%`：
  - `MASK_IMAGE_QOI_CIRCLE +2.8%`
  - `MASK_IMAGE_RLE_CIRCLE +6.5%`
  - `EXTERN_MASK_IMAGE_QOI_CIRCLE +1.1%`
  - `EXTERN_MASK_IMAGE_RLE_CIRCLE +3.5%`
- alpha8 circle draw 与 raw circle / resize 对照组继续稳定：
  - `MASK_IMAGE_QOI_8_CIRCLE -0.3%`
  - `MASK_IMAGE_RLE_8_CIRCLE -0.4%`
  - `EXTERN_MASK_IMAGE_QOI_8_CIRCLE -0.2%`
  - `EXTERN_MASK_IMAGE_RLE_8_CIRCLE -0.3%`
  - `MASK_IMAGE_CIRCLE -0.1%`
  - `EXTERN_MASK_IMAGE_CIRCLE -0.3%`
  - `MASK_IMAGE_TEST_PERF_CIRCLE +0.6%`
  - `EXTERN_MASK_IMAGE_TEST_PERF_CIRCLE +0.0%`

### 运行 / 单测

- `HelloPerformance` runtime
  - on：`241 frames`，`ALL PASSED`
  - off：`241 frames`，`ALL PASSED`
  - on / off 全量 `241` 帧 PNG hash mismatch `0`
  - on / off 全量 `241` 帧 pixel mismatch `0`
  - 抽查 `frame_0000.png` / `frame_0120.png` / `frame_0240.png` hash 一致
- `HelloUnitTest`
  - on：`688/688 passed`
  - off：`688/688 passed`

### 结论

- 这颗 child 宏成立：
  - `text -1200B`
  - 完整 `239` 场景 perf 没有任何 `>=10%` 回退
  - runtime 渲染与 unit 都保持严格等价
- 因此：
  - 保留 `EGUI_CONFIG_IMAGE_STD_MASK_CIRCLE_DRAW_RGB565_FAST_PATH_ENABLE`
  - 默认值继续保持开启
  - 如果项目要保住 circle 的 alpha8 masked-image draw 热点，应优先试这颗 child，再考虑父宏 `EGUI_CONFIG_IMAGE_STD_MASK_CIRCLE_DRAW_FAST_PATH_ENABLE`
  - 如果项目要保住 raw / non-alpha8 circle draw，则仍应优先试 `EGUI_CONFIG_IMAGE_STD_MASK_CIRCLE_DRAW_ALPHA8_FAST_PATH_ENABLE`

## 本轮继续细拆 A/B: `std image mask round-rect draw fast path`

### 测试环境

- 日期：`2026-04-04`
- 提交基线：`ae256e2`
- `HelloPerformance` code size A/B
  - `make -B -j APP=HelloPerformance PORT=qemu CPU_ARCH=cortex-m0plus`
  - `make -B -j APP=HelloPerformance PORT=qemu CPU_ARCH=cortex-m0plus USER_CFLAGS=-DEGUI_CONFIG_IMAGE_STD_MASK_ROUND_RECT_DRAW_FAST_PATH_ENABLE=0`
- `HelloPerformance` perf A/B
  - `python scripts/perf_analysis/code_perf_check.py --profile cortex-m3 --clean --filter RECTANGLE,CIRCLE,ROUND_RECTANGLE,TEXT,IMAGE_565,EXTERN_IMAGE_565,IMAGE_RESIZE_565,EXTERN_IMAGE_RESIZE_565,MASK_IMAGE_ROUND_RECT,MASK_IMAGE_ROUND_RECT_QUARTER,MASK_IMAGE_ROUND_RECT_DOUBLE,MASK_IMAGE_TEST_PERF_ROUND_RECT,MASK_IMAGE_QOI_ROUND_RECT,MASK_IMAGE_RLE_ROUND_RECT,EXTERN_MASK_IMAGE_TEST_PERF_ROUND_RECT,EXTERN_MASK_IMAGE_QOI_ROUND_RECT,EXTERN_MASK_IMAGE_RLE_ROUND_RECT,MASK_IMAGE_CIRCLE,MASK_IMAGE_CIRCLE_QUARTER,MASK_IMAGE_CIRCLE_DOUBLE,MASK_IMAGE_TEST_PERF_CIRCLE,MASK_IMAGE_QOI_CIRCLE,MASK_IMAGE_RLE_CIRCLE,EXTERN_MASK_IMAGE_TEST_PERF_CIRCLE,EXTERN_MASK_IMAGE_QOI_CIRCLE,EXTERN_MASK_IMAGE_RLE_CIRCLE`
  - `python scripts/perf_analysis/code_perf_check.py --profile cortex-m3 --clean --filter RECTANGLE,CIRCLE,ROUND_RECTANGLE,TEXT,IMAGE_565,EXTERN_IMAGE_565,IMAGE_RESIZE_565,EXTERN_IMAGE_RESIZE_565,MASK_IMAGE_ROUND_RECT,MASK_IMAGE_ROUND_RECT_QUARTER,MASK_IMAGE_ROUND_RECT_DOUBLE,MASK_IMAGE_TEST_PERF_ROUND_RECT,MASK_IMAGE_QOI_ROUND_RECT,MASK_IMAGE_RLE_ROUND_RECT,EXTERN_MASK_IMAGE_TEST_PERF_ROUND_RECT,EXTERN_MASK_IMAGE_QOI_ROUND_RECT,EXTERN_MASK_IMAGE_RLE_ROUND_RECT,MASK_IMAGE_CIRCLE,MASK_IMAGE_CIRCLE_QUARTER,MASK_IMAGE_CIRCLE_DOUBLE,MASK_IMAGE_TEST_PERF_CIRCLE,MASK_IMAGE_QOI_CIRCLE,MASK_IMAGE_RLE_CIRCLE,EXTERN_MASK_IMAGE_TEST_PERF_CIRCLE,EXTERN_MASK_IMAGE_QOI_CIRCLE,EXTERN_MASK_IMAGE_RLE_CIRCLE --extra-cflags=-DEGUI_CONFIG_IMAGE_STD_MASK_ROUND_RECT_DRAW_FAST_PATH_ENABLE=0`
- 运行/单测验证
  - `python scripts/code_runtime_check.py --app HelloPerformance --timeout 10 --keep-screenshots`
  - `USER_CFLAGS=-DEGUI_CONFIG_IMAGE_STD_MASK_ROUND_RECT_DRAW_FAST_PATH_ENABLE=0 python scripts/code_runtime_check.py --app HelloPerformance --timeout 10 --keep-screenshots`
  - `make -j APP=HelloUnitTest PORT=pc_test APP_OBJ_SUFFIX=utmaskrrdrawbase`
  - `make -j APP=HelloUnitTest PORT=pc_test APP_OBJ_SUFFIX=utmaskrrdrawoff USER_CFLAGS=-DEGUI_CONFIG_IMAGE_STD_MASK_ROUND_RECT_DRAW_FAST_PATH_ENABLE=0`
  - `output/main.exe`

### 代码量证据

- `HelloPerformance` 最终链接汇总：
  - baseline：`text=2056256 data=72 bss=3832`
  - 关闭 round-rect draw child：`text=2050140 data=72 bss=3832`
- 结论：
  - `text -6116B`
  - `data/bss` 不变
- 从当前实现归属看，这颗 child 只裁掉 round-rect masked-image direct-draw 分支：
  - `egui_image_std_blend_rgb565_masked_row()` / `row_block()` 对 round-rect 的 specialized path
  - `egui_image_std_blend_rgb565_alpha8_masked_row()` / `row_block()` 对 round-rect 的 specialized path
  - round-rect masked-image resize helpers 仍留在 `EGUI_CONFIG_IMAGE_STD_MASK_ROUND_RECT_RESIZE_FAST_PATH_ENABLE`

### 性能证据

| 场景 | baseline | 关闭 round-rect draw child | 变化 |
| --- | ---: | ---: | ---: |
| `RECTANGLE` | `0.179ms` | `0.179ms` | `+0.0%` |
| `CIRCLE` | `0.810ms` | `0.810ms` | `+0.0%` |
| `ROUND_RECTANGLE` | `0.812ms` | `0.812ms` | `+0.0%` |
| `TEXT` | `1.027ms` | `1.027ms` | `+0.0%` |
| `IMAGE_565` | `0.239ms` | `0.239ms` | `+0.0%` |
| `EXTERN_IMAGE_565` | `0.417ms` | `0.417ms` | `+0.0%` |
| `IMAGE_RESIZE_565` | `0.459ms` | `0.458ms` | `-0.2%` |
| `EXTERN_IMAGE_RESIZE_565` | `0.507ms` | `0.507ms` | `+0.0%` |
| `MASK_IMAGE_ROUND_RECT` | `0.969ms` | `0.969ms` | `+0.0%` |
| `MASK_IMAGE_ROUND_RECT_QUARTER` | `0.326ms` | `0.326ms` | `+0.0%` |
| `MASK_IMAGE_ROUND_RECT_DOUBLE` | `0.943ms` | `0.943ms` | `+0.0%` |
| `MASK_IMAGE_TEST_PERF_ROUND_RECT` | `0.586ms` | `0.587ms` | `+0.2%` |
| `MASK_IMAGE_QOI_ROUND_RECT` | `6.468ms` | `7.139ms` | `+10.4%` |
| `MASK_IMAGE_RLE_ROUND_RECT` | `2.715ms` | `3.386ms` | `+24.7%` |
| `EXTERN_MASK_IMAGE_TEST_PERF_ROUND_RECT` | `0.684ms` | `0.684ms` | `+0.0%` |
| `EXTERN_MASK_IMAGE_QOI_ROUND_RECT` | `16.699ms` | `17.370ms` | `+4.0%` |
| `EXTERN_MASK_IMAGE_RLE_ROUND_RECT` | `5.224ms` | `5.895ms` | `+12.8%` |
| `MASK_IMAGE_CIRCLE` | `1.431ms` | `1.429ms` | `-0.1%` |
| `MASK_IMAGE_CIRCLE_QUARTER` | `0.453ms` | `0.453ms` | `+0.0%` |
| `MASK_IMAGE_CIRCLE_DOUBLE` | `1.316ms` | `1.315ms` | `-0.1%` |
| `MASK_IMAGE_TEST_PERF_CIRCLE` | `0.904ms` | `0.909ms` | `+0.6%` |
| `MASK_IMAGE_QOI_CIRCLE` | `6.614ms` | `6.611ms` | `+0.0%` |
| `MASK_IMAGE_RLE_CIRCLE` | `2.861ms` | `2.857ms` | `-0.1%` |
| `EXTERN_MASK_IMAGE_TEST_PERF_CIRCLE` | `1.286ms` | `1.286ms` | `+0.0%` |
| `EXTERN_MASK_IMAGE_QOI_CIRCLE` | `16.844ms` | `16.841ms` | `+0.0%` |
| `EXTERN_MASK_IMAGE_RLE_CIRCLE` | `5.370ms` | `5.366ms` | `-0.1%` |

- 这说明 round-rect draw child 的代价已经被隔离到 round-rect 的 codec / external draw 热点：
  - 基础 `RECTANGLE / CIRCLE / ROUND_RECTANGLE / TEXT / IMAGE_565 / EXTERN_IMAGE_565 / IMAGE_RESIZE_565 / EXTERN_IMAGE_RESIZE_565` 全都在噪声范围
  - internal round-rect masked-image resize 样本维持不变
  - circle 对照组维持不变

### 运行/单测

- `HelloPerformance` runtime
  - baseline：`241 frames`，`ALL PASSED`
  - `round-rect draw child off`：`241 frames`，`ALL PASSED`
  - 截图哈希一致：`frame_0000.png` / `frame_0240.png`
- `HelloUnitTest`
  - baseline：`688/688 passed`
  - `round-rect draw child off`：`688/688 passed`

### 结论

- 这颗 child 宏成立：
  - `text -6116B`
  - 基础主路径不退化
  - perf 代价只落在 round-rect 的 codec / external masked-image draw 热点
- 因此：
  - 保留 `EGUI_CONFIG_IMAGE_STD_MASK_ROUND_RECT_DRAW_FAST_PATH_ENABLE`
  - 默认值继续保持开启
  - 如果项目想保住 internal round-rect masked-image resize，但可以接受牺牲 round-rect 的 QOI / RLE / external draw 热点，应先试这颗 child，再考虑更激进的 `EGUI_CONFIG_IMAGE_STD_MASK_ROUND_RECT_FAST_PATH_ENABLE`

## 本轮继续细拆 A/B: `std image mask round-rect draw alpha8 fast path`

### 测试环境

- 日期：`2026-04-04`
- 提交基线：`c5a9341`
- `HelloPerformance` perf A/B
  - baseline 结果：`.claude/perf_mask_round_rect_draw_alpha8_base_results.json`
  - `ROUND_RECT_DRAW_ALPHA8 child off`：`.claude/perf_mask_round_rect_draw_alpha8_off_results.json`
- 运行 / 单测验证
  - `HelloPerformance` runtime 录制输出：
    - `runtime_check_output/HelloPerformance/maskrrdrawalpha8_on`
    - `runtime_check_output/HelloPerformance/maskrrdrawalpha8_off`
  - `HelloUnitTest` baseline / off 都已跑通

### 代码量证据

- `HelloPerformance` 最终链接汇总：
  - baseline：`text=2056256 data=72 bss=3832`
  - 关闭 `ROUND_RECT_DRAW_ALPHA8` child：`text=2052296 data=72 bss=3832`
- 结论：
  - `text -3960B`
  - `data/bss` 不变
- 从当前实现归属看，这颗 child 只裁掉 round-rect masked-image direct-draw 里的 `RGB565_8` / alpha8 specialized helpers：
  - `egui_image_std_blend_rgb565_alpha8_masked_row()` / `row_block()` 对 round-rect 的 specialized path
  - alpha8 round-rect direct-draw 的 left/right fixed-row helpers
  - raw `RGB565` round-rect direct-draw helper 继续留在 `EGUI_CONFIG_IMAGE_STD_MASK_ROUND_RECT_DRAW_FAST_PATH_ENABLE`

### 性能证据

| 场景 | baseline | 关闭 `ROUND_RECT_DRAW_ALPHA8` child | 变化 |
| --- | ---: | ---: | ---: |
| `RECTANGLE` | `0.179ms` | `0.179ms` | `+0.0%` |
| `CIRCLE` | `0.810ms` | `0.810ms` | `+0.0%` |
| `ROUND_RECTANGLE` | `0.812ms` | `0.812ms` | `+0.0%` |
| `TEXT` | `1.027ms` | `1.027ms` | `+0.0%` |
| `IMAGE_565` | `0.239ms` | `0.239ms` | `+0.0%` |
| `EXTERN_IMAGE_565` | `0.417ms` | `0.417ms` | `+0.0%` |
| `IMAGE_RESIZE_565` | `0.459ms` | `0.458ms` | `-0.2%` |
| `EXTERN_IMAGE_RESIZE_565` | `0.507ms` | `0.507ms` | `+0.0%` |
| `MASK_IMAGE_ROUND_RECT` | `0.969ms` | `0.969ms` | `+0.0%` |
| `MASK_IMAGE_ROUND_RECT_QUARTER` | `0.326ms` | `0.326ms` | `+0.0%` |
| `MASK_IMAGE_ROUND_RECT_DOUBLE` | `0.943ms` | `0.943ms` | `+0.0%` |
| `MASK_IMAGE_TEST_PERF_ROUND_RECT` | `0.586ms` | `0.587ms` | `+0.2%` |
| `MASK_IMAGE_QOI_ROUND_RECT` | `6.468ms` | `6.442ms` | `-0.4%` |
| `MASK_IMAGE_RLE_ROUND_RECT` | `2.715ms` | `2.688ms` | `-1.0%` |
| `MASK_IMAGE_QOI_8_ROUND_RECT` | `2.356ms` | `2.579ms` | `+9.5%` |
| `MASK_IMAGE_RLE_8_ROUND_RECT` | `1.626ms` | `1.849ms` | `+13.7%` |
| `EXTERN_MASK_IMAGE_TEST_PERF_ROUND_RECT` | `0.684ms` | `0.684ms` | `+0.0%` |
| `EXTERN_MASK_IMAGE_QOI_ROUND_RECT` | `16.699ms` | `16.672ms` | `-0.2%` |
| `EXTERN_MASK_IMAGE_RLE_ROUND_RECT` | `5.224ms` | `5.197ms` | `-0.5%` |
| `EXTERN_MASK_IMAGE_QOI_8_ROUND_RECT` | `2.912ms` | `3.135ms` | `+7.7%` |
| `EXTERN_MASK_IMAGE_RLE_8_ROUND_RECT` | `2.419ms` | `2.642ms` | `+9.2%` |
| `MASK_IMAGE_TEST_PERF_CIRCLE` | `0.904ms` | `0.909ms` | `+0.6%` |
| `EXTERN_MASK_IMAGE_TEST_PERF_CIRCLE` | `1.286ms` | `1.286ms` | `+0.0%` |

- 这说明 `ROUND_RECT_DRAW` 还能继续切出一颗更保守的 alpha8 child：
  - 基础 `RECTANGLE / CIRCLE / ROUND_RECTANGLE / TEXT / IMAGE_565 / EXTERN_IMAGE_565 / IMAGE_RESIZE_565 / EXTERN_IMAGE_RESIZE_565` 全都在噪声范围
  - raw round-rect draw / resize 与 non-alpha8 `QOI / RLE / external` 样本维持不变
  - 代价只落在 round-rect 的 alpha8 direct-draw 热点

### 运行 / 单测

- `HelloPerformance` runtime
  - baseline：`241 frames`，`ALL PASSED`
  - `ROUND_RECT_DRAW_ALPHA8 child off`：`241 frames`，`ALL PASSED`
  - baseline / off 全量 `241` 帧 PNG hash mismatch `0`
  - 抽查 `frame_0000.png` / `frame_0120.png` / `frame_0240.png` hash 一致
- `HelloUnitTest`
  - baseline：`688/688 passed`
  - `ROUND_RECT_DRAW_ALPHA8 child off`：`688/688 passed`

### 结论

- 这颗 child 宏成立：
  - `text -3960B`
  - 基础主路径不退化
  - perf 代价只落在 round-rect 的 alpha8 masked-image draw 热点
- 因此：
  - 保留 `EGUI_CONFIG_IMAGE_STD_MASK_ROUND_RECT_DRAW_ALPHA8_FAST_PATH_ENABLE`
  - 默认值继续保持开启
  - 如果项目要保住 raw round-rect masked-image draw，应优先试这颗 child，再考虑父宏 `EGUI_CONFIG_IMAGE_STD_MASK_ROUND_RECT_DRAW_FAST_PATH_ENABLE`

## 本轮继续细拆 A/B: `std image mask round-rect draw alpha8 row-fast path`

### 测试环境

- 日期：`2026-04-04`
- 提交基线：`466f472`
- 临时 child split
  - `EGUI_CONFIG_IMAGE_STD_MASK_ROUND_RECT_DRAW_ALPHA8_ROW_FAST_PATH_ENABLE`
- `HelloPerformance` perf A/B
  - baseline 结果：`.claude/perf_round_rect_draw_alpha8_row_fast_on_results.json`
  - `ROW_FAST child off`：`.claude/perf_round_rect_draw_alpha8_row_fast_off_results.json`
- 运行 / 单测验证
  - `HelloPerformance` runtime 录制输出：
    - `runtime_check_output/HelloPerformance/rrdrawa8rowfast_on`
    - `runtime_check_output/HelloPerformance/rrdrawa8rowfast_off`
  - `HelloUnitTest` baseline / off 都已跑通

### 代码量证据

- `HelloPerformance` 最终链接汇总：
  - baseline：`text=2056264 data=72 bss=3832`
  - 关闭 `ROW_FAST child`：`text=2052304 data=72 bss=3832`
- 结论：
  - `text -3960B`
  - `data/bss` 不变
- 这与现有 `EGUI_CONFIG_IMAGE_STD_MASK_ROUND_RECT_DRAW_ALPHA8_FAST_PATH_ENABLE` 的历史回收量完全重合，说明这颗临时 child 没有形成新的更细层级。

### 性能证据

| 场景 | baseline | 关闭 `ROW_FAST child` | 变化 |
| --- | ---: | ---: | ---: |
| `RECTANGLE` | `0.179ms` | `0.179ms` | `+0.0%` |
| `CIRCLE` | `0.810ms` | `0.810ms` | `+0.0%` |
| `ROUND_RECTANGLE` | `0.812ms` | `0.812ms` | `+0.0%` |
| `TEXT` | `1.027ms` | `1.027ms` | `+0.0%` |
| `IMAGE_565` | `0.239ms` | `0.239ms` | `+0.0%` |
| `EXTERN_IMAGE_565` | `0.417ms` | `0.417ms` | `+0.0%` |
| `IMAGE_RESIZE_565` | `0.459ms` | `0.459ms` | `+0.0%` |
| `EXTERN_IMAGE_RESIZE_565` | `0.507ms` | `0.507ms` | `+0.0%` |
| `MASK_IMAGE_QOI_8_ROUND_RECT` | `2.356ms` | `2.579ms` | `+9.5%` |
| `MASK_IMAGE_RLE_8_ROUND_RECT` | `1.626ms` | `1.849ms` | `+13.7%` |
| `EXTERN_MASK_IMAGE_QOI_8_ROUND_RECT` | `2.912ms` | `3.135ms` | `+7.7%` |
| `EXTERN_MASK_IMAGE_RLE_8_ROUND_RECT` | `2.419ms` | `2.642ms` | `+9.2%` |

- 完整 `239` 场景 perf 的热点分布仍然与现有 `ROUND_RECT_DRAW_ALPHA8` child 重合：
  - 基础 `RECTANGLE / CIRCLE / ROUND_RECTANGLE / TEXT / IMAGE_565 / EXTERN_IMAGE_565 / IMAGE_RESIZE_565 / EXTERN_IMAGE_RESIZE_565` 都在噪声范围
  - 代价仍只落在同一批 round-rect alpha8 `QOI / RLE / external` masked-image draw 热点
- 因此它不是新的更细粒度 child，而只是把现有 `ROUND_RECT_DRAW_ALPHA8` 叶子宏重新表达了一次。

### 运行 / 单测

- `HelloPerformance` runtime
  - baseline：`241 frames`，`ALL PASSED`
  - `ROW_FAST child off`：`241 frames`，`ALL PASSED`
  - baseline / off 全量 `241` 帧 PNG hash mismatch `0`
  - baseline / off 全量 `241` 帧 pixel mismatch `0`
  - 抽查 `frame_0000.png` / `frame_0120.png` / `frame_0240.png` hash 一致
- `HelloUnitTest`
  - baseline：`688/688 passed`
  - `ROW_FAST child off`：`688/688 passed`

### 结论

- `EGUI_CONFIG_IMAGE_STD_MASK_ROUND_RECT_DRAW_ALPHA8_ROW_FAST_PATH_ENABLE` 不保留：
  - `text -3960B` 虽然达到门槛，但与现有 `EGUI_CONFIG_IMAGE_STD_MASK_ROUND_RECT_DRAW_ALPHA8_FAST_PATH_ENABLE` 的 size / perf 覆盖范围实质等价
  - 它没有把 `ROUND_RECT_DRAW_ALPHA8` 再拆成更细的新层级，只是重复同一层开关
- 因此：
  - 代码已回退，不引入这颗临时 child
  - `EGUI_CONFIG_IMAGE_STD_MASK_ROUND_RECT_DRAW_ALPHA8_FAST_PATH_ENABLE` 仍视为当前可保留的叶子宏
  - 后续不再沿这条 `row-fast` 线继续细拆

## 本轮继续细拆 A/B: `std image mask round-rect draw rgb565 fast path`

### 测试环境

- 日期：`2026-04-05`
- 提交基线：`299e65c`
- 新增 child split
  - `EGUI_CONFIG_IMAGE_STD_MASK_ROUND_RECT_DRAW_RGB565_FAST_PATH_ENABLE`
- `HelloPerformance` perf A/B
  - on 结果：`.claude/perf_mask_round_rect_draw_rgb565_on_results.json`
  - off 结果：`.claude/perf_mask_round_rect_draw_rgb565_off_results.json`
- 运行 / 单测验证
  - `HelloPerformance` runtime 录制输出：
    - `runtime_check_output/HelloPerformance/mask_round_rect_draw_rgb565_on`
    - `runtime_check_output/HelloPerformance/mask_round_rect_draw_rgb565_off`
  - `HelloUnitTest` on / off 都已跑通

### 代码量证据

- `HelloPerformance` 最终链接汇总：
  - on：`text=2055380 data=72 bss=3832`
  - off：`text=2053164 data=72 bss=3832`
- 结论：
  - `text -2216B`
  - `data/bss` 不变
- 从当前实现归属看，这颗 child 只裁掉 round-rect masked-image direct-draw 里的 non-alpha8 specialized helpers：
  - `egui_image_std_blend_rgb565_round_rect_masked_row_fast()` / `row_block()` 对 round-rect 的 specialized path
  - raw/non-alpha8 round-rect direct-draw 的 left/right fixed-row helpers
  - `RGB565_8` round-rect direct-draw / row-block specialized helpers 继续留在 `EGUI_CONFIG_IMAGE_STD_MASK_ROUND_RECT_DRAW_ALPHA8_FAST_PATH_ENABLE`

### 性能证据

- `HelloPerformance` 完整 `239` 场景对比
  - `>=10%` 回退：`0`
  - `<=-10%` 改善：`0`
- 基础主路径维持在噪声范围：
  - `RECTANGLE +0.0%`
  - `CIRCLE +0.0%`
  - `ROUND_RECTANGLE +0.0%`
  - `TEXT +0.0%`
  - `IMAGE_565 +0.0%`
  - `EXTERN_IMAGE_565 +0.0%`
  - `IMAGE_RESIZE_565 -0.2%`
  - `EXTERN_IMAGE_RESIZE_565 +0.0%`
- round-rect non-alpha8 draw 热点仍低于 `10%`：
  - `MASK_IMAGE_QOI_ROUND_RECT +3.2%`
  - `MASK_IMAGE_RLE_ROUND_RECT +7.7%`
  - `EXTERN_MASK_IMAGE_QOI_ROUND_RECT +1.3%`
  - `EXTERN_MASK_IMAGE_RLE_ROUND_RECT +4.0%`
- alpha8 round-rect draw 与 raw round-rect / resize 对照组继续稳定：
  - `MASK_IMAGE_QOI_8_ROUND_RECT +0.0%`
  - `MASK_IMAGE_RLE_8_ROUND_RECT +0.0%`
  - `EXTERN_MASK_IMAGE_QOI_8_ROUND_RECT +0.0%`
  - `EXTERN_MASK_IMAGE_RLE_8_ROUND_RECT +0.0%`
  - `MASK_IMAGE_ROUND_RECT +0.0%`
  - `EXTERN_MASK_IMAGE_ROUND_RECT +0.0%`
  - `MASK_IMAGE_TEST_PERF_ROUND_RECT +0.2%`
  - `EXTERN_MASK_IMAGE_TEST_PERF_ROUND_RECT +0.0%`

### 运行 / 单测

- `HelloPerformance` runtime
  - on：`241 frames`，`ALL PASSED`
  - off：`241 frames`，`ALL PASSED`
  - on / off 全量 `241` 帧 PNG hash mismatch `0`
  - on / off 全量 `241` 帧 pixel mismatch `0`
  - 抽查 `frame_0000.png` / `frame_0120.png` / `frame_0240.png` hash 一致
- `HelloUnitTest`
  - on：`688/688 passed`
  - off：`688/688 passed`

### 结论

- 这颗 child 宏成立：
  - `text -2216B`
  - 完整 `239` 场景 perf 没有任何 `>=10%` 回退
  - runtime 渲染与 unit 都保持严格等价
- 因此：
  - 保留 `EGUI_CONFIG_IMAGE_STD_MASK_ROUND_RECT_DRAW_RGB565_FAST_PATH_ENABLE`
  - 默认值继续保持开启
  - 如果项目要保住 round-rect 的 alpha8 masked-image draw 热点，应优先试这颗 child，再考虑父宏 `EGUI_CONFIG_IMAGE_STD_MASK_ROUND_RECT_DRAW_FAST_PATH_ENABLE`
  - 如果项目要保住 raw / non-alpha8 round-rect draw，则仍应优先试 `EGUI_CONFIG_IMAGE_STD_MASK_ROUND_RECT_DRAW_ALPHA8_FAST_PATH_ENABLE`

## 本轮继续细拆 A/B: `std image mask round-rect resize alpha8 fast path`

### 测试环境

- 日期：`2026-04-04`
- 提交基线：`71b9317`
- `HelloPerformance` perf A/B
  - baseline 结果：`.claude/perf_mask_round_rect_resize_alpha8_base_results.json`
  - `ROUND_RECT_RESIZE_ALPHA8 child off`：`.claude/perf_mask_round_rect_resize_alpha8_off_results.json`
- 运行 / 单测验证
  - `HelloPerformance` runtime 录制输出：
    - `runtime_check_output/HelloPerformance/maskrrresizea8_on`
    - `runtime_check_output/HelloPerformance/maskrrresizea8_off`
  - `HelloUnitTest` baseline / off 都已跑通

### 代码量证据

- `HelloPerformance` 最终链接汇总：
  - baseline：`text=2056256 data=72 bss=3832`
  - 关闭 `ROUND_RECT_RESIZE_ALPHA8` child：`text=2055120 data=72 bss=3832`
- 结论：
  - `text -1136B`
  - `data/bss` 不变
- 从当前实现归属看，这颗 child 只裁掉 round-rect masked-image resize 里的 `RGB565_8` specialized helper：
  - `egui_image_std_set_image_resize_rgb565_8_round_rect_fast()`
  - raw `RGB565` round-rect resize helper 继续留在 `EGUI_CONFIG_IMAGE_STD_MASK_ROUND_RECT_RESIZE_FAST_PATH_ENABLE`

### 性能证据

| 场景 | baseline | 关闭 `ROUND_RECT_RESIZE_ALPHA8` child | 变化 |
| --- | ---: | ---: | ---: |
| `RECTANGLE` | `0.179ms` | `0.179ms` | `+0.0%` |
| `CIRCLE` | `0.810ms` | `0.810ms` | `+0.0%` |
| `ROUND_RECTANGLE` | `0.812ms` | `0.812ms` | `+0.0%` |
| `TEXT` | `1.027ms` | `1.027ms` | `+0.0%` |
| `IMAGE_565` | `0.239ms` | `0.239ms` | `+0.0%` |
| `EXTERN_IMAGE_565` | `0.417ms` | `0.417ms` | `+0.0%` |
| `IMAGE_RESIZE_565` | `0.459ms` | `0.459ms` | `+0.0%` |
| `EXTERN_IMAGE_RESIZE_565` | `0.507ms` | `0.507ms` | `+0.0%` |
| `MASK_IMAGE_ROUND_RECT` | `0.969ms` | `1.074ms` | `+10.8%` |
| `EXTERN_MASK_IMAGE_ROUND_RECT` | `1.035ms` | `1.136ms` | `+9.8%` |
| `MASK_IMAGE_ROUND_RECT_QUARTER` | `0.326ms` | `0.352ms` | `+8.0%` |
| `MASK_IMAGE_ROUND_RECT_DOUBLE` | `0.943ms` | `1.041ms` | `+10.4%` |
| `MASK_IMAGE_TEST_PERF_ROUND_RECT` | `0.586ms` | `0.586ms` | `+0.0%` |
| `EXTERN_MASK_IMAGE_TEST_PERF_ROUND_RECT` | `0.684ms` | `0.684ms` | `+0.0%` |
| `MASK_IMAGE_QOI_8_ROUND_RECT` | `2.356ms` | `2.347ms` | `-0.4%` |
| `MASK_IMAGE_RLE_8_ROUND_RECT` | `1.626ms` | `1.616ms` | `-0.6%` |
| `EXTERN_MASK_IMAGE_QOI_8_ROUND_RECT` | `2.912ms` | `2.904ms` | `-0.3%` |
| `EXTERN_MASK_IMAGE_RLE_8_ROUND_RECT` | `2.419ms` | `2.409ms` | `-0.4%` |
| `MASK_IMAGE_TEST_PERF_CIRCLE` | `0.904ms` | `0.904ms` | `+0.0%` |
| `EXTERN_MASK_IMAGE_TEST_PERF_CIRCLE` | `1.286ms` | `1.286ms` | `+0.0%` |

- 这说明 `ROUND_RECT_RESIZE` 还能继续切出一颗更保守的 alpha8 child：
  - 基础 `RECTANGLE / CIRCLE / ROUND_RECTANGLE / TEXT / IMAGE_565 / EXTERN_IMAGE_565 / IMAGE_RESIZE_565 / EXTERN_IMAGE_RESIZE_565` 全都在噪声范围
  - raw round-rect resize 样本维持不变
  - draw / codec 对照组维持不变
  - 代价只落在 round-rect 的 alpha8 masked-image resize 热点

### 运行 / 单测

- `HelloPerformance` runtime
  - baseline：`241 frames`，`ALL PASSED`
  - `ROUND_RECT_RESIZE_ALPHA8 child off`：`241 frames`，`ALL PASSED`
  - baseline / off 全量 `241` 帧 PNG hash mismatch `0`
  - 抽查 `frame_0000.png` / `frame_0120.png` / `frame_0240.png` hash 一致
- `HelloUnitTest`
  - baseline：`688/688 passed`
  - `ROUND_RECT_RESIZE_ALPHA8 child off`：`688/688 passed`

### 结论

- 这颗 child 宏成立：
  - `text -1136B`
  - 基础主路径不退化
  - perf 代价只落在 round-rect 的 alpha8 masked-image resize 热点
- 因此：
  - 保留 `EGUI_CONFIG_IMAGE_STD_MASK_ROUND_RECT_RESIZE_ALPHA8_FAST_PATH_ENABLE`
  - 默认值继续保持开启
  - 如果项目要保住 raw round-rect masked-image resize，应优先试这颗 child，再考虑父宏 `EGUI_CONFIG_IMAGE_STD_MASK_ROUND_RECT_RESIZE_FAST_PATH_ENABLE`

## 本轮继续细拆 A/B: `std image mask round-rect resize rgb565 fast path`

### 测试环境

- 日期：`2026-04-05`
- 提交基线：`e377792`
- 新增 child split
  - `EGUI_CONFIG_IMAGE_STD_MASK_ROUND_RECT_RESIZE_RGB565_FAST_PATH_ENABLE`
- `HelloPerformance` perf A/B
  - on 结果：`.claude/perf_mask_round_rect_resize_rgb565_on_results.json`
  - off 结果：`.claude/perf_mask_round_rect_resize_rgb565_off_results.json`
- 运行 / 单测验证
  - `HelloPerformance` runtime 录制输出：
    - `runtime_check_output/HelloPerformance/mask_round_rect_resize_rgb565_on`
    - `runtime_check_output/HelloPerformance/mask_round_rect_resize_rgb565_off`
  - `HelloUnitTest` on / off 都已跑通

### 代码量证据

- `HelloPerformance` 最终链接汇总：
  - on：`text=2055376 data=72 bss=3832`
  - off：`text=2052904 data=72 bss=3832`
- 结论：
  - `text -2472B`
  - `data/bss` 不变
- 从当前实现归属看，这颗 child 只裁掉 raw `RGB565` round-rect masked-image resize specialized helpers：
  - `egui_image_std_set_image_resize_rgb565_round_rect_fast()`
  - `egui_image_std_blend_rgb565_round_rect_masked_left_segment_fixed_row()`
  - `egui_image_std_blend_rgb565_round_rect_masked_right_segment_fixed_row()`
  - `RGB565_8` round-rect resize helper 继续留在 `EGUI_CONFIG_IMAGE_STD_MASK_ROUND_RECT_RESIZE_ALPHA8_FAST_PATH_ENABLE`

### 性能证据

- `HelloPerformance` 完整 `239` 场景对比
  - `>=10%` 回退：`0`
  - `<=-10%` 改善：`0`
- 基础主路径维持在噪声范围：
  - `RECTANGLE +0.0%`
  - `CIRCLE +0.0%`
  - `ROUND_RECTANGLE +0.0%`
  - `TEXT +0.0%`
  - `IMAGE_565 +0.0%`
  - `EXTERN_IMAGE_565 +0.0%`
  - `IMAGE_RESIZE_565 -0.2%`
  - `EXTERN_IMAGE_RESIZE_565 +0.0%`
- raw round-rect resize / sampled round-rect 对照组仍低于阈值：
  - `MASK_IMAGE_ROUND_RECT +0.0%`
  - `MASK_IMAGE_ROUND_RECT_QUARTER +0.3%`
  - `MASK_IMAGE_ROUND_RECT_DOUBLE +0.0%`
  - `MASK_IMAGE_TEST_PERF_ROUND_RECT +9.7%`
- alpha8 round-rect resize 与 draw / codec / external 对照组继续稳定：
  - `EXTERN_MASK_IMAGE_ROUND_RECT +0.0%`
  - `EXTERN_MASK_IMAGE_TEST_PERF_ROUND_RECT +0.0%`
  - `MASK_IMAGE_QOI_8_ROUND_RECT +0.0%`
  - `MASK_IMAGE_RLE_8_ROUND_RECT +0.0%`
  - `EXTERN_MASK_IMAGE_QOI_8_ROUND_RECT +0.0%`
  - `EXTERN_MASK_IMAGE_RLE_8_ROUND_RECT +0.0%`
  - `MASK_IMAGE_QOI_ROUND_RECT -0.0%`
  - `MASK_IMAGE_RLE_ROUND_RECT -0.1%`

### 运行 / 单测

- `HelloPerformance` runtime
  - on：`241 frames`，`ALL PASSED`
  - off：`241 frames`，`ALL PASSED`
  - on / off 全量 `241` 帧 PNG hash mismatch `0`
  - on / off 全量 `241` 帧 pixel mismatch `0`
  - 抽查 `frame_0000.png` / `frame_0120.png` / `frame_0240.png` hash 一致
- `HelloUnitTest`
  - on：`688/688 passed`
  - off：`688/688 passed`

### 结论

- 这颗 child 宏成立：
  - `text -2472B`
  - 完整 `239` 场景 perf 没有任何 `>=10%` 回退
  - runtime 渲染与 unit 都保持严格等价
- 因此：
  - 保留 `EGUI_CONFIG_IMAGE_STD_MASK_ROUND_RECT_RESIZE_RGB565_FAST_PATH_ENABLE`
  - 默认值继续保持开启
  - 如果项目要保住 alpha8 round-rect masked-image resize，应优先试这颗 child，再考虑父宏 `EGUI_CONFIG_IMAGE_STD_MASK_ROUND_RECT_RESIZE_FAST_PATH_ENABLE`
  - 如果项目要保住 raw round-rect masked-image resize，则仍应优先试 `EGUI_CONFIG_IMAGE_STD_MASK_ROUND_RECT_RESIZE_ALPHA8_FAST_PATH_ENABLE`
  - round-rect resize 这条线现在已经有 `alpha8` / `rgb565` 两颗互补 child；除非后续新增 perf 场景还能再单独隔离更窄的 hotspot，否则不再优先沿同一层 `std.c` 继续细拆

## 本轮继续细拆 A/B: `canvas masked-fill fast path group`

### 代码量证据

- `HelloPerformance` baseline：
  - `text=2055224 data=72 bss=3832`
- shape child 宏：
  - `EGUI_CONFIG_CANVAS_MASK_FILL_CIRCLE_FAST_PATH_ENABLE=0`：`text=2051576 data=72 bss=3832`，收益 `-3648B`
  - `EGUI_CONFIG_CANVAS_MASK_FILL_ROUND_RECT_FAST_PATH_ENABLE=0`：`text=2054348 data=72 bss=3832`，收益 `-876B`
- auxiliary 宏：
  - `EGUI_CONFIG_CANVAS_MASK_FILL_ROW_BLEND_FAST_PATH_ENABLE=0`：`text=2052276 data=72 bss=3832`，收益 `-2948B`
  - `EGUI_CONFIG_CANVAS_MASK_FILL_IMAGE_FAST_PATH_ENABLE=0`：`text=2053364 data=72 bss=3832`，收益 `-1860B`
  - `EGUI_CONFIG_CANVAS_MASK_FILL_ROW_RANGE_FAST_PATH_ENABLE=0`：`text=2047196 data=72 bss=3832`，收益 `-8028B`
  - `EGUI_CONFIG_CANVAS_MASK_FILL_ROW_PARTIAL_FAST_PATH_ENABLE=0`：`text=2052296 data=72 bss=3832`，收益 `-2928B`
  - `EGUI_CONFIG_CANVAS_MASK_FILL_ROW_INSIDE_FAST_PATH_ENABLE=0`：`text=2053560 data=72 bss=3832`，收益 `-1664B`

### 性能证据

- 这一组宏在当前优先保护的基础主路径上都保持在噪声范围：
  - `RECTANGLE 0.179 -> 0.179 / 0.179 / 0.178 / 0.179`
  - `CIRCLE 0.810 -> 0.809 / 0.809 / 0.809 / 0.809`
  - `ROUND_RECTANGLE 0.812 -> 0.811 / 0.811 / 0.811 / 0.811`
  - `TEXT 1.027 -> 1.026 / 1.026 / 1.027 / 1.026`
  - `IMAGE_565 0.239 -> 0.239 / 0.239 / 0.239 / 0.239`
  - `EXTERN_IMAGE_565 0.417 -> 0.417 / 0.417 / 0.417 / 0.417`
- 关键代价集中在各自专属热点：
  - `MASK_FILL_CIRCLE=0`：`MASK_RECT_FILL_CIRCLE 0.692 -> 0.951 (+37.4%)`
  - `MASK_FILL_ROUND_RECT=0`：`MASK_RECT_FILL_ROUND_RECT 0.307 -> 0.345 (+12.4%)`，`MASK_ROUND_RECT_FILL_WITH_MASK 0.306 -> 0.345 (+12.7%)`
  - `ROW_BLEND=0`：`MASK_GRADIENT_RECT_FILL 0.339 -> 3.927 (+1058.4%)`
  - `IMAGE=0`：`MASK_RECT_FILL_IMAGE 0.277 -> 0.987 (+256.3%)`，`MASK_RECT_FILL_IMAGE_DOUBLE 0.173 -> 0.426 (+146.2%)`
  - `ROW_RANGE=0`：`2026-04-05` current-mainline 完整 `239` 场景同样没有任何 `>=10%` 波动，`MASK_GRADIENT_RECT_FILL`、`MASK_RECT_FILL_IMAGE`、`MASK_RECT_FILL_ROUND_RECT`、`MASK_RECT_FILL_CIRCLE`、`MASK_ROUND_RECT_FILL_WITH_MASK` 都在 `+0.3%` 以内
  - `ROW_PARTIAL=0`：`MASK_GRADIENT_RECT_FILL`、`MASK_RECT_FILL_IMAGE`、`MASK_RECT_FILL_ROUND_RECT`、`MASK_RECT_FILL_CIRCLE`、`MASK_ROUND_RECT_FILL_WITH_MASK` 当前都在 `+0.7%` 以内
  - `ROW_INSIDE=0`：基础 `RECTANGLE / CIRCLE / ROUND_RECTANGLE / TEXT / IMAGE_565 / EXTERN_IMAGE_565` 和当前抽样到的 canvas masked-fill 场景都在噪声范围

### 运行/单测

- `ROW_RANGE` 默认开启 / 宏关闭：
  - `HelloPerformance` runtime：`241 frames`，`hash mismatch 0`、`pixel mismatch 0`
  - `HelloUnitTest`：`688/688 passed`
- `ROW_PARTIAL` 默认开启 / 宏关闭：
  - `HelloPerformance` runtime：`241 frames`，`ALL PASSED`
  - `HelloUnitTest`：`688/688 passed`
- `ROW_INSIDE` 默认开启 / 宏关闭：
  - `HelloPerformance` runtime：`241 frames`，`ALL PASSED`
  - `HelloUnitTest`：`688/688 passed`
- 之前已经完成的 `CIRCLE / ROUND_RECT / ROW_BLEND / IMAGE` 这几轮 canvas masked-fill A/B 也都已通过 runtime / unit。

### 结论

- 这一组 `canvas masked-fill` 宏现在可以分成四档：
  - 第一档：`EGUI_CONFIG_CANVAS_MASK_FILL_ROW_RANGE_FAST_PATH_ENABLE`
    - 当前收益最大，`text -8028B`
    - `2026-04-05` current-mainline 已补齐完整 `239` 场景 perf、`241` 帧 runtime 和 `688` 项 unit，全量等价
    - 当前基础路径和抽样 canvas masked-fill 都安全
  - 第二档：`EGUI_CONFIG_CANVAS_MASK_FILL_CIRCLE_SEGMENT_FAST_PATH_ENABLE`
    - `2026-04-05` current-mainline `text -3328B`
    - 它是保留整行 `egui_mask_circle_fill_row_segment()`、只拿掉 `egui_canvas_fill_masked_row_segment()` 里 circle partial-row segment helper 的更保守档位
    - 当前完整 `239` 场景 perf、`241` 帧 runtime 和 `688` 项 unit 也都全量等价
  - 第三档：`EGUI_CONFIG_CANVAS_MASK_FILL_ROW_PARTIAL_FAST_PATH_ENABLE`
    - `text -2928B`
    - 它是保留 `OUTSIDE / INSIDE` 行级优化、只拿掉 `PARTIAL` 分支加速的中间档位
  - 第四档：`EGUI_CONFIG_CANVAS_MASK_FILL_ROW_INSIDE_FAST_PATH_ENABLE`
    - `text -1664B`
    - 它是只拿掉 `EGUI_MASK_ROW_INSIDE` 直填分支、继续保留 `OUTSIDE` 跳过和可选 `PARTIAL` 加速的更保守档位
  - 条件档：`EGUI_CONFIG_CANVAS_MASK_FILL_CIRCLE_FAST_PATH_ENABLE`、`EGUI_CONFIG_CANVAS_MASK_FILL_IMAGE_FAST_PATH_ENABLE`、`EGUI_CONFIG_CANVAS_MASK_FILL_ROW_BLEND_FAST_PATH_ENABLE`
    - 都满足基础主路径不退化
    - 但会分别伤到 circle masked-fill、image masked-fill、gradient masked-fill 热点
- `EGUI_CONFIG_CANVAS_MASK_FILL_ROUND_RECT_FAST_PATH_ENABLE` 继续保留没有问题，但当前代码量收益只有 `876B`，已经低于这轮 `>1KB` 的优先门槛，不再作为优先推荐的 size-first 关闭项。
- `canvas masked-fill circle` 这条线已经补出 `EGUI_CONFIG_CANVAS_MASK_FILL_CIRCLE_SEGMENT_FAST_PATH_ENABLE` 这颗更保守的 child；除非后续新增 perf 场景能单独隔离 partial-row circle masked-fill segment hotspot，否则不再优先沿同层继续细拆。

## 本轮继续细拆 A/B: `canvas masked-fill circle segment fast path`

### 测试环境

- 日期：`2026-04-05`
- 提交基线：`b783c7a`
- 新增 child split
  - `EGUI_CONFIG_CANVAS_MASK_FILL_CIRCLE_SEGMENT_FAST_PATH_ENABLE`
- `HelloPerformance` perf A/B
  - on 结果：`.claude/perf_canvas_mask_fill_circle_segment_on_results.json`
  - off 结果：`.claude/perf_canvas_mask_fill_circle_segment_off_results.json`
- 运行 / 单测验证
  - `HelloPerformance` runtime 录制输出：
    - `runtime_check_output/HelloPerformance/canvas_mask_fill_circle_segment_on`
    - `runtime_check_output/HelloPerformance/canvas_mask_fill_circle_segment_off`
  - `HelloUnitTest` on / off 都已跑通

### 代码量证据

- `HelloPerformance` 最终链接汇总：
  - on：`text=2055380 data=72 bss=3832`
  - off：`text=2052052 data=72 bss=3832`
- 结论：
  - `text -3328B`
  - `data/bss` 不变
- 从当前实现归属看，这颗 child 只裁掉 `egui_canvas_fill_masked_row_segment()` 里的 circle partial-row segment fast path：
  - circle `point_cached_row_index` / `point_cached_row_valid` 的 partial-row 复用逻辑
  - segment 内按列计算 circle coverage 并直接 blend 的专用循环
  - 整行 `egui_mask_circle_fill_row_segment()` 仍继续留在 `EGUI_CONFIG_CANVAS_MASK_FILL_CIRCLE_FAST_PATH_ENABLE`

### 性能证据

- `HelloPerformance` 完整 `239` 场景对比
  - `>=10%` 回退：`0`
  - `<=-10%` 改善：`0`
- 基础主路径维持在噪声范围：
  - `RECTANGLE +0.0%`
  - `CIRCLE +0.0%`
  - `ROUND_RECTANGLE +0.0%`
  - `TEXT +0.0%`
  - `IMAGE_565 +0.0%`
  - `EXTERN_IMAGE_565 +0.0%`
  - `IMAGE_RESIZE_565 +0.0%`
  - `EXTERN_IMAGE_RESIZE_565 +0.0%`
- 当前 sampled canvas masked-fill 对照组也继续稳定：
  - `MASK_RECT_FILL_CIRCLE +0.0%`
  - `MASK_RECT_FILL_ROUND_RECT +0.0%`
  - `MASK_RECT_FILL_IMAGE +0.0%`
  - `MASK_GRADIENT_RECT_FILL +0.0%`
  - `MASK_ROUND_RECT_FILL_WITH_MASK +0.0%`

### 运行 / 单测

- `HelloPerformance` runtime
  - on：`241 frames`，`ALL PASSED`
  - off：`241 frames`，`ALL PASSED`
  - on / off 全量 `241` 帧 PNG hash mismatch `0`
  - on / off 全量 `241` 帧 pixel mismatch `0`
  - 抽查 `frame_0000.png` / `frame_0120.png` / `frame_0240.png` hash 一致
- `HelloUnitTest`
  - on：`688/688 passed`
  - off：`688/688 passed`

### 结论

- 这颗 child 宏成立：
  - `text -3328B`
  - 完整 `239` 场景 perf 没有任何 `>=10%` 回退
  - runtime 渲染与 unit 都保持严格等价
- 因此：
  - 保留 `EGUI_CONFIG_CANVAS_MASK_FILL_CIRCLE_SEGMENT_FAST_PATH_ENABLE`
  - 默认值继续保持开启
  - 如果项目要保住当前 circle masked-fill 主路径，应优先试这颗 child，再考虑父宏 `EGUI_CONFIG_CANVAS_MASK_FILL_CIRCLE_FAST_PATH_ENABLE`
  - 当前 perf 集还没有单独隔离 partial-row circle masked-fill segment hotspot，因此它更适合作为条件实验宏，而不适合默认关

## 本轮补齐 A/B: `canvas masked-fill shape umbrella fast path`

### 代码量证据

- 先前带 `APP_OBJ_SUFFIX` 的对象级对比能看到 `egui_canvas.c.o` 等对象变小，但当前树最终还是要以无 suffix 的 `output/main.elf` 为准：
  - baseline：`text=2053748 data=72 bss=3832`
  - `EGUI_CONFIG_CANVAS_MASK_SHAPE_FILL_FAST_PATH_ENABLE=0`：`text=2053748 data=72 bss=3832`
- 结论：
  - 最终链接 `text +0B`
  - 这颗 umbrella 在当前树上没有稳定的最终 ELF 收益

### 性能证据

- 基础优先路径维持在噪声范围：
  - `RECTANGLE 0.179 -> 0.179 (+0.0%)`
  - `CIRCLE 0.810 -> 0.810 (+0.0%)`
  - `ROUND_RECTANGLE 0.812 -> 0.812 (+0.0%)`
  - `TEXT 1.027 -> 1.027 (+0.0%)`
  - `IMAGE_565 0.239 -> 0.239 (+0.0%)`
  - `EXTERN_IMAGE_565 0.417 -> 0.417 (+0.0%)`
- 代价仍然集中在 shape masked-fill 热点：
  - `MASK_GRADIENT_RECT_FILL 0.339 -> 0.342 (+0.9%)`
  - `MASK_RECT_FILL_IMAGE 0.276 -> 0.277 (+0.4%)`
  - `MASK_RECT_FILL_ROUND_RECT 0.307 -> 0.347 (+13.0%)`
  - `MASK_RECT_FILL_CIRCLE 0.692 -> 0.948 (+37.0%)`
  - `MASK_ROUND_RECT_FILL_WITH_MASK 0.307 -> 0.347 (+13.0%)`

### 运行/单测

- `HelloPerformance` runtime
  - baseline：`241 frames`，`ALL PASSED`
  - 宏关闭：`241 frames`，`ALL PASSED`
  - baseline / off 的 `frame_0000.png` 和 `frame_0240.png` SHA256 完全一致
- `HelloUnitTest`
  - baseline：`688/688 passed`
  - 宏关闭：`688/688 passed`

### 结论

- 这颗 umbrella 没有换来任何最终 ELF 收益，却会把 circle / round-rect masked-fill 热点重新打回 generic path。
- 因此：
  - 直接拒绝 `EGUI_CONFIG_CANVAS_MASK_SHAPE_FILL_FAST_PATH_ENABLE`
  - 不再把它作为长期保留的实验宏
  - 继续只保留已经证明有独立价值的 child 宏：`EGUI_CONFIG_CANVAS_MASK_FILL_CIRCLE_FAST_PATH_ENABLE` / `EGUI_CONFIG_CANVAS_MASK_FILL_ROUND_RECT_FAST_PATH_ENABLE`

## 本轮继续细拆 A/B: `canvas masked-fill image scale fast path`

### 测试环境

- 日期：`2026-04-04`
- 提交基线：`53bedd2`
- `HelloPerformance` code size A/B
  - `make -B -j APP=HelloPerformance PORT=qemu CPU_ARCH=cortex-m0plus APP_OBJ_SUFFIX=hpcanvasfillimagescaleon USER_CFLAGS=-DEGUI_CONFIG_CANVAS_MASK_FILL_IMAGE_SCALE_FAST_PATH_ENABLE=1`
  - `make -B -j APP=HelloPerformance PORT=qemu CPU_ARCH=cortex-m0plus APP_OBJ_SUFFIX=hpcanvasfillimagescaleoff USER_CFLAGS=-DEGUI_CONFIG_CANVAS_MASK_FILL_IMAGE_SCALE_FAST_PATH_ENABLE=0`
- `HelloPerformance` perf A/B
  - `python scripts/perf_analysis/code_perf_check.py --profile cortex-m3 --clean --threshold 1000 --timeout 600 --extra-cflags=-DEGUI_CONFIG_CANVAS_MASK_FILL_IMAGE_SCALE_FAST_PATH_ENABLE=1`
  - `python scripts/perf_analysis/code_perf_check.py --profile cortex-m3 --clean --threshold 1000 --timeout 600 --extra-cflags=-DEGUI_CONFIG_CANVAS_MASK_FILL_IMAGE_SCALE_FAST_PATH_ENABLE=0`
- 运行/单测验证
  - `HelloPerformance` baseline / off 通过 `scripts.code_runtime_check.compile_app()` + `run_app(timeout=35)` 注入 `-DEGUI_CONFIG_CANVAS_MASK_FILL_IMAGE_SCALE_FAST_PATH_ENABLE=1/0`
  - `make -j APP=HelloUnitTest PORT=pc_test APP_OBJ_SUFFIX=utcanvasfillimagescaleon USER_CFLAGS=-DEGUI_CONFIG_CANVAS_MASK_FILL_IMAGE_SCALE_FAST_PATH_ENABLE=1`
  - `make -j APP=HelloUnitTest PORT=pc_test APP_OBJ_SUFFIX=utcanvasfillimagescaleoff USER_CFLAGS=-DEGUI_CONFIG_CANVAS_MASK_FILL_IMAGE_SCALE_FAST_PATH_ENABLE=0`
  - `output/main.exe`

### 代码量证据

- `HelloPerformance` 链接汇总：
  - 默认开启：`text=2056264 data=72 bss=3832`
  - 关闭 scaled child：`text=2056068 data=72 bss=3832`
- 结论：
  - `text -196B`
  - `data/bss` 不变

### 性能证据

- `HelloPerformance` 全量 `239` 场景 on / off 都在噪声内
- 当前没有任何场景超过 `10%`
- 当前波动最大的样本也只有：
  - `MASK_RECT_FILL_IMAGE_DOUBLE -0.58%`
  - `MASK_RECT_FILL_IMAGE -0.36%`
  - `MASK_IMAGE_IMAGE +0.34%`
  - `IMAGE_TILED_RLE_565_8 +0.08%`

### 运行与渲染验证

- `HelloPerformance` baseline / off 都是 `241 frames captured`
- `frame_0000.png` / `frame_0120.png` / `frame_0240.png` hash 一致
- 全量 `241` 帧 `hash mismatch 0`
- 全量 `241` 帧 `pixel mismatch 0`

### 单元测试

- `HelloUnitTest` baseline：`688/688 passed`
- `HelloUnitTest` off：`688/688 passed`

### 结论

- `EGUI_CONFIG_CANVAS_MASK_FILL_IMAGE_SCALE_FAST_PATH_ENABLE` 只覆盖 `egui_mask_image_fill_row_segment()` 的 non-identity-scale 分支。
- 它证明这层代码几乎已经没有独立回收价值：
  - `HelloPerformance text -196B`
  - perf / runtime / unit 全量等价
- 因此不保留这颗 child，代码已回退。
- `canvas masked-fill image` 这条线当前仍只保留父宏 `EGUI_CONFIG_CANVAS_MASK_FILL_IMAGE_FAST_PATH_ENABLE`；如果后续再继续压缩，应优先回到更上层的父宏或别的 mask/image 线，而不是继续追这个 scaled child。

## 本轮继续细拆 A/B: `mask-image identity-scale fast path`

### 代码量证据

- `HelloPerformance` baseline：
  - `text=2055224 data=72 bss=3832`
- `EGUI_CONFIG_MASK_IMAGE_IDENTITY_SCALE_FAST_PATH_ENABLE=0`：
  - `text=2051952 data=72 bss=3832`
  - 收益 `-3272B`

### 性能证据

- 基础优先路径维持在噪声范围：
  - `RECTANGLE 0.179 -> 0.179 (+0.0%)`
  - `CIRCLE 0.810 -> 0.810 (+0.0%)`
  - `ROUND_RECTANGLE 0.812 -> 0.812 (+0.0%)`
  - `TEXT 1.027 -> 1.027 (+0.0%)`
  - `IMAGE_565 0.239 -> 0.239 (+0.0%)`
  - `EXTERN_IMAGE_565 0.417 -> 0.417 (+0.0%)`
- 代价集中在 identity-scale image-mask / image-masked solid-fill 热点：
  - `MASK_RECT_FILL_IMAGE 0.277 -> 0.421 (+52.0%)`
  - `MASK_RECT_FILL_IMAGE_DOUBLE 0.173 -> 0.225 (+30.1%)`
  - `MASK_IMAGE_IMAGE 0.299 -> 0.456 (+52.5%)`
  - `MASK_IMAGE_QOI_IMAGE 1.401 -> 1.540 (+9.9%)`
  - `MASK_IMAGE_RLE_IMAGE 0.595 -> 0.733 (+23.2%)`
  - `EXTERN_MASK_IMAGE_QOI_IMAGE 3.378 -> 3.516 (+4.1%)`
  - `EXTERN_MASK_IMAGE_RLE_IMAGE 0.928 -> 1.067 (+15.0%)`

### 运行/单测

- `HelloPerformance` runtime：
  - 默认开启：`241 frames`，`ALL PASSED`
  - 宏关闭：`241 frames`，`ALL PASSED`
- `HelloUnitTest`：
  - 默认开启：`688/688 passed`
  - 宏关闭：`688/688 passed`

### 结论

- 这颗宏满足：
  - 代码量收益 `>1KB`
  - 当前基础 `RECTANGLE / CIRCLE / ROUND_RECTANGLE / TEXT / IMAGE_565 / EXTERN_IMAGE_565` 路径不退化
- 但它会明确牺牲 identity-scale image-mask / image-masked solid-fill 热点。
- 因此：
  - 保留 `EGUI_CONFIG_MASK_IMAGE_IDENTITY_SCALE_FAST_PATH_ENABLE`
  - 默认值继续保持开启
  - 作为“项目不用 image-mask identity-scale 热点时”的条件实验宏是成立的
  - 当前不进入无条件优先关闭序列

## 本轮继续细拆 A/B: `mask-image identity-scale child fast path`

### 测试环境

- 日期：`2026-04-04`
- 提交基线：`dabeeef`
- 临时 child split：
  - `EGUI_CONFIG_MASK_IMAGE_IDENTITY_SCALE_FILL_FAST_PATH_ENABLE`
  - `EGUI_CONFIG_MASK_IMAGE_IDENTITY_SCALE_BLEND_FAST_PATH_ENABLE`
- 额外验证过但最终拒绝的更碎临时拆分：
  - `SEGMENT child`
  - `ROW_BLOCK child`

### 代码量证据

- baseline
  - `text=2056264 data=72 bss=3832`
- `FILL child off`
  - `text=2055604 data=72 bss=3832`
- `BLEND child off`
  - `text=2053652 data=72 bss=3832`
- 临时更碎 child：
  - `SEGMENT child off`：`text=2055996 data=72 bss=3832`
  - `ROW_BLOCK child off`：`text=2055524 data=72 bss=3832`
- 结论：
  - `FILL child -660B`
  - `BLEND child -2612B`
  - `SEGMENT child -268B`
  - `ROW_BLOCK child -740B`

### 性能证据

| 场景 | baseline | `FILL child off` | 变化 | `BLEND child off` | 变化 |
| --- | ---: | ---: | ---: | ---: | ---: |
| `RECTANGLE` | `0.179ms` | `0.179ms` | `+0.0%` | `0.179ms` | `+0.0%` |
| `CIRCLE` | `0.810ms` | `0.810ms` | `+0.0%` | `0.810ms` | `+0.0%` |
| `ROUND_RECTANGLE` | `0.812ms` | `0.812ms` | `+0.0%` | `0.812ms` | `+0.0%` |
| `TEXT` | `1.027ms` | `1.027ms` | `+0.0%` | `1.027ms` | `+0.0%` |
| `IMAGE_565` | `0.239ms` | `0.239ms` | `+0.0%` | `0.239ms` | `+0.0%` |
| `EXTERN_IMAGE_565` | `0.417ms` | `0.417ms` | `+0.0%` | `0.417ms` | `+0.0%` |
| `MASK_RECT_FILL_IMAGE` | `0.276ms` | `0.421ms` | `+52.5%` | `0.276ms` | `+0.0%` |
| `MASK_RECT_FILL_IMAGE_DOUBLE` | `0.173ms` | `0.225ms` | `+30.1%` | `0.173ms` | `+0.0%` |
| `MASK_IMAGE_IMAGE` | `0.296ms` | `0.297ms` | `+0.3%` | `0.454ms` | `+53.4%` |
| `EXTERN_MASK_IMAGE_IMAGE` | `0.331ms` | `0.330ms` | `-0.3%` | `0.488ms` | `+47.4%` |
| `MASK_IMAGE_QOI_IMAGE` | `1.401ms` | `1.401ms` | `+0.0%` | `1.539ms` | `+9.9%` |
| `MASK_IMAGE_RLE_IMAGE` | `0.595ms` | `0.595ms` | `+0.0%` | `0.734ms` | `+23.4%` |
| `EXTERN_MASK_IMAGE_QOI_IMAGE` | `3.377ms` | `3.377ms` | `+0.0%` | `3.516ms` | `+4.1%` |
| `EXTERN_MASK_IMAGE_RLE_IMAGE` | `0.928ms` | `0.928ms` | `+0.0%` | `1.067ms` | `+15.0%` |

- 这轮 child split 的 perf 影响是干净隔离的：
  - `FILL child` 只伤 identity-scale image-mask solid-fill 热点
  - `BLEND child` 只伤 identity-scale masked-image blend 热点
- 但更碎的 `SEGMENT` / `ROW_BLOCK` 单独拆分收益都不到 `1KB`，不值得长期保留。

### 运行/单测

- `HelloPerformance` runtime
  - baseline：`241 frames`，`ALL PASSED`
  - `FILL child off`：`241 frames`，`ALL PASSED`
  - `BLEND child off`：`241 frames`，`ALL PASSED`
- `HelloUnitTest`
  - baseline：`688/688 passed`
  - `FILL child off`：`688/688 passed`
  - `BLEND child off`：`688/688 passed`

### 结论

- 这轮 child split 证明 `mask-image identity-scale` 可以按语义拆成：
  - `FILL`：只牺牲 identity-scale image-mask solid-fill 热点
  - `BLEND`：只牺牲 identity-scale masked-image blend 热点
- 但代码量收益不对称：
  - `FILL child -660B`，低于当前 `>1KB` 门槛，不保留
  - `BLEND child -2612B`，满足保留门槛，且 perf 代价只落在对应热点
- 因此最终保留项收敛为：
  - 保留 umbrella `EGUI_CONFIG_MASK_IMAGE_IDENTITY_SCALE_FAST_PATH_ENABLE`
  - 保留 `EGUI_CONFIG_MASK_IMAGE_IDENTITY_SCALE_BLEND_FAST_PATH_ENABLE`
  - 不保留 `FILL child`
  - 不保留更碎的 `SEGMENT child` / `ROW_BLOCK child`

## 本轮继续细拆 A/B: `std image mask visible-range fast path`

### 代码量证据

- baseline
  - `text=2055220 data=72 bss=3832`
- `EGUI_CONFIG_IMAGE_STD_MASK_VISIBLE_RANGE_FAST_PATH_ENABLE=0`
  - `text=2053804 data=72 bss=3832`
  - 收益：`text -1416B`

### 性能证据

- 基础优先路径维持在噪声范围：
  - `RECTANGLE 0.179 -> 0.179 (+0.0%)`
  - `CIRCLE 0.810 -> 0.810 (+0.0%)`
  - `ROUND_RECTANGLE 0.812 -> 0.812 (+0.0%)`
  - `TEXT 1.027 -> 1.027 (+0.0%)`
  - `IMAGE_565 0.239 -> 0.239 (+0.0%)`
  - `EXTERN_IMAGE_565 0.417 -> 0.417 (+0.0%)`
- 当前抽样的主要 masked-image 场景基本不动：
  - `MASK_IMAGE_CIRCLE 1.425 -> 1.420 (-0.4%)`
  - `EXTERN_MASK_IMAGE_CIRCLE 1.489 -> 1.483 (-0.4%)`
  - `MASK_IMAGE_IMAGE 0.299 -> 0.299 (+0.0%)`
  - `EXTERN_MASK_IMAGE_IMAGE 0.333 -> 0.334 (+0.3%)`
  - `MASK_IMAGE_QOI_IMAGE 1.401 -> 1.401 (+0.0%)`
  - `EXTERN_MASK_IMAGE_QOI_IMAGE 3.378 -> 3.378 (+0.0%)`
  - `MASK_IMAGE_RLE_IMAGE 0.595 -> 0.595 (+0.0%)`
  - `EXTERN_MASK_IMAGE_RLE_IMAGE 0.928 -> 0.928 (+0.0%)`
- 明显代价集中在一个 external masked-image 专项场景：
  - `EXTERN_MASK_IMAGE_TEST_PERF_CIRCLE 1.276 -> 2.354 (+84.5%)`

### 运行/单测

- `HelloPerformance` runtime
  - 默认开启：`241 frames`，`ALL PASSED`
  - 宏关闭：`241 frames`，`ALL PASSED`
- `HelloUnitTest`
  - 默认开启：`688/688 passed`
  - 宏关闭：`688/688 passed`

### 结论

- 这颗宏满足：
  - 代码量收益 `>1KB`
  - 当前基础 `RECTANGLE / CIRCLE / ROUND_RECTANGLE / TEXT / IMAGE_565 / EXTERN_IMAGE_565` 路径不退化
- 但它不是“无代价回收”，因为 external masked-image 的专项场景 `EXTERN_MASK_IMAGE_TEST_PERF_CIRCLE` 会明显回退。
- 因此：
  - 保留 `EGUI_CONFIG_IMAGE_STD_MASK_VISIBLE_RANGE_FAST_PATH_ENABLE`
  - 默认值继续保持开启
  - 作为“项目不用 partial-row external masked-image 热点”时的条件实验宏是成立的
  - 当前不进入无条件优先关闭序列

## 本轮继续细拆 A/B: `std image mask visible-range resize fast path`

### 代码量证据

- `HelloPerformance` 最终链接汇总：
  - baseline：`text=2056264 data=72 bss=3832`
  - `EGUI_CONFIG_IMAGE_STD_MASK_VISIBLE_RANGE_RESIZE_FAST_PATH_ENABLE=0`：`text=2055444 data=72 bss=3832`
- 结论：
  - `text -820B`
  - `data/bss` 不变

### 性能证据

| 场景 | baseline | 关闭 visible-range resize child | 变化 |
| --- | ---: | ---: | ---: |
| `RECTANGLE` | `0.179ms` | `0.179ms` | `+0.0%` |
| `CIRCLE` | `0.810ms` | `0.810ms` | `+0.0%` |
| `ROUND_RECTANGLE` | `0.812ms` | `0.812ms` | `+0.0%` |
| `TEXT` | `1.027ms` | `1.027ms` | `+0.0%` |
| `IMAGE_565` | `0.239ms` | `0.239ms` | `+0.0%` |
| `EXTERN_IMAGE_565` | `0.417ms` | `0.417ms` | `+0.0%` |
| `IMAGE_RESIZE_565` | `0.459ms` | `0.459ms` | `+0.0%` |
| `EXTERN_IMAGE_RESIZE_565` | `0.507ms` | `0.507ms` | `+0.0%` |
| `MASK_IMAGE_ROUND_RECT` | `0.969ms` | `0.969ms` | `+0.0%` |
| `MASK_IMAGE_CIRCLE` | `1.431ms` | `1.422ms` | `-0.6%` |
| `MASK_IMAGE_IMAGE` | `0.296ms` | `0.297ms` | `+0.3%` |
| `EXTERN_MASK_IMAGE_ROUND_RECT` | `1.035ms` | `1.035ms` | `+0.0%` |
| `EXTERN_MASK_IMAGE_CIRCLE` | `1.494ms` | `1.483ms` | `-0.7%` |
| `EXTERN_MASK_IMAGE_IMAGE` | `0.331ms` | `0.331ms` | `+0.0%` |
| `MASK_IMAGE_TEST_PERF_ROUND_RECT` | `0.586ms` | `0.586ms` | `+0.0%` |
| `MASK_IMAGE_TEST_PERF_CIRCLE` | `0.904ms` | `0.903ms` | `-0.1%` |
| `MASK_IMAGE_TEST_PERF_IMAGE` | `1.661ms` | `1.662ms` | `+0.1%` |
| `EXTERN_MASK_IMAGE_TEST_PERF_ROUND_RECT` | `0.684ms` | `0.708ms` | `+3.5%` |
| `EXTERN_MASK_IMAGE_TEST_PERF_CIRCLE` | `1.286ms` | `2.529ms` | `+96.7%` |
| `EXTERN_MASK_IMAGE_TEST_PERF_IMAGE` | `1.092ms` | `1.735ms` | `+58.9%` |

- 这说明 resize child 的语义虽然干净，但代价已经集中到 external sampled masked-image resize 热点：
  - 基础 `RECTANGLE / CIRCLE / ROUND_RECTANGLE / TEXT / IMAGE_565 / EXTERN_IMAGE_565 / IMAGE_RESIZE_565 / EXTERN_IMAGE_RESIZE_565` 全都不退化
  - non-test masked-image resize 样本基本都在噪声范围
  - 但 `EXTERN_MASK_IMAGE_TEST_PERF_CIRCLE` 和 `EXTERN_MASK_IMAGE_TEST_PERF_IMAGE` 回退过大

### 运行 / 单测

- `HelloPerformance` runtime
  - baseline：`241 frames`，`ALL PASSED`
  - visible-range resize child off：`241 frames`，`ALL PASSED`
  - baseline / off 全量 `241` 帧 PNG hash mismatch `0`
  - 抽查 `frame_0000.png` / `frame_0120.png` / `frame_0240.png` hash 一致
- `HelloUnitTest`
  - baseline：`688/688 passed`
  - visible-range resize child off：`688/688 passed`

### 结论

- 这颗 child 宏不保留：
  - 代码量收益只有 `text -820B`
  - perf 代价集中在 external sampled masked-image resize 热点
- 因此：
  - 拒绝保留 `EGUI_CONFIG_IMAGE_STD_MASK_VISIBLE_RANGE_RESIZE_FAST_PATH_ENABLE`
  - 源码已回退
  - `VISIBLE_RANGE` 这条线当前停在父宏 `EGUI_CONFIG_IMAGE_STD_MASK_VISIBLE_RANGE_FAST_PATH_ENABLE`

## 本轮补齐 A/B: `std image mask row-range fast path`

### 代码量证据

- `HelloPerformance` 最终链接汇总：
  - baseline：`text=2056264 data=72 bss=3832`
  - `EGUI_CONFIG_IMAGE_STD_MASK_ROW_RANGE_FAST_PATH_ENABLE=0`：`text=2046016 data=72 bss=3832`
- 结论：
  - `text -10248B`
  - `data/bss` 不变

### 性能证据

| 场景 | baseline | 关闭 row-range fast path | 变化 |
| --- | ---: | ---: | ---: |
| `RECTANGLE` | `0.179ms` | `0.179ms` | `+0.0%` |
| `CIRCLE` | `0.810ms` | `0.810ms` | `+0.0%` |
| `ROUND_RECTANGLE` | `0.812ms` | `0.812ms` | `+0.0%` |
| `TEXT` | `1.027ms` | `1.027ms` | `+0.0%` |
| `IMAGE_565` | `0.239ms` | `0.239ms` | `+0.0%` |
| `EXTERN_IMAGE_565` | `0.417ms` | `0.417ms` | `+0.0%` |
| `IMAGE_RESIZE_565` | `0.459ms` | `0.459ms` | `+0.0%` |
| `EXTERN_IMAGE_RESIZE_565` | `0.507ms` | `0.506ms` | `-0.2%` |
| `MASK_IMAGE_TEST_PERF_CIRCLE` | `0.904ms` | `7.249ms` | `+701.9%` |
| `EXTERN_MASK_IMAGE_TEST_PERF_CIRCLE` | `1.286ms` | `7.304ms` | `+468.0%` |
| `MASK_IMAGE_TEST_PERF_ROUND_RECT` | `0.586ms` | `0.587ms` | `+0.2%` |
| `EXTERN_MASK_IMAGE_TEST_PERF_ROUND_RECT` | `0.684ms` | `7.125ms` | `+941.7%` |

- 这说明 `ROW_RANGE` umbrella 虽然保持了基础主路径，但会把 `PARTIAL + INSIDE` 两类 masked-image 热点一起打回 generic path。
- 相比只关 `ROW_PARTIAL`，它多换来约 `2.6KB` 的代码收益，但 external round-rect 的 perf 代价会急剧放大。

### 运行/单测

- `HelloPerformance` runtime
  - baseline：`241 frames`，`ALL PASSED`
  - 宏关闭：`241 frames`，`ALL PASSED`
  - baseline / off 的 `frame_0000.png` 和 `frame_0240.png` SHA256 完全一致
- `HelloUnitTest`
  - baseline：`688/688 passed`
  - 宏关闭：`688/688 passed`

### 结论

- 这颗宏满足：
  - 代码量收益显著，`text -10248B`
  - 基础 `RECTANGLE / CIRCLE / ROUND_RECTANGLE / TEXT / IMAGE_565 / EXTERN_IMAGE_565 / IMAGE_RESIZE_565 / EXTERN_IMAGE_RESIZE_565` 路径不退化
- 但它是这一簇里更激进的 umbrella，perf 代价明显高于只关 `ROW_PARTIAL`。
- 因此：
  - 保留 `EGUI_CONFIG_IMAGE_STD_MASK_ROW_RANGE_FAST_PATH_ENABLE`
  - 默认值继续保持开启
  - 它只适合作为“项目不用 row-range masked-image 热点”时的一次性激进实验宏
  - 如果想先试更可控的关法，顺序仍然优先 `ROW_PARTIAL`，再考虑 `ROW_RANGE`，最后才是 `ROW_INSIDE`

## 本轮继续细拆 A/B: `std image mask row-partial fast path`

### 测试环境

- 日期：`2026-04-03`
- 提交基线：`dabeeef`
- `HelloPerformance` code size A/B
  - `make clean APP=HelloPerformance PORT=qemu`
  - `make all APP=HelloPerformance PORT=qemu`
  - `make clean APP=HelloPerformance PORT=qemu`
  - `make all APP=HelloPerformance PORT=qemu USER_CFLAGS="-DEGUI_CONFIG_IMAGE_STD_MASK_ROW_PARTIAL_FAST_PATH_ENABLE=0"`
- `HelloPerformance` perf A/B
  - `python scripts/perf_analysis/code_perf_check.py --profile cortex-m3 --clean --threshold 1000 --filter RECTANGLE,CIRCLE,ROUND_RECTANGLE,TEXT,IMAGE_565,EXTERN_IMAGE_565,IMAGE_RESIZE_565,EXTERN_IMAGE_RESIZE_565,MASK_IMAGE_TEST_PERF_CIRCLE,EXTERN_MASK_IMAGE_TEST_PERF_CIRCLE,MASK_IMAGE_TEST_PERF_ROUND_RECT,EXTERN_MASK_IMAGE_TEST_PERF_ROUND_RECT`
  - `python scripts/perf_analysis/code_perf_check.py --profile cortex-m3 --clean --threshold 1000 --filter RECTANGLE,CIRCLE,ROUND_RECTANGLE,TEXT,IMAGE_565,EXTERN_IMAGE_565,IMAGE_RESIZE_565,EXTERN_IMAGE_RESIZE_565,MASK_IMAGE_TEST_PERF_CIRCLE,EXTERN_MASK_IMAGE_TEST_PERF_CIRCLE,MASK_IMAGE_TEST_PERF_ROUND_RECT,EXTERN_MASK_IMAGE_TEST_PERF_ROUND_RECT --extra-cflags=-DEGUI_CONFIG_IMAGE_STD_MASK_ROW_PARTIAL_FAST_PATH_ENABLE=0`
- 运行/单测验证
  - `python scripts/code_runtime_check.py --app HelloPerformance --timeout 10 --keep-screenshots`
  - `mask row partial off` 通过 `scripts/code_runtime_check.py` 的脚本接口注入 `USER_CFLAGS=-DEGUI_CONFIG_IMAGE_STD_MASK_ROW_PARTIAL_FAST_PATH_ENABLE=0`
  - `make -j APP=HelloUnitTest PORT=pc_test`
  - `make -j APP=HelloUnitTest PORT=pc_test USER_CFLAGS="-DEGUI_CONFIG_IMAGE_STD_MASK_ROW_PARTIAL_FAST_PATH_ENABLE=0"`
  - `output/main.exe`

### 代码量证据

- `HelloPerformance` 链接汇总：
  - 默认开启：`text=2036712 data=72 bss=3840`
  - 关闭 row-partial fast path：`text=2029112 data=72 bss=3840`
- 结论：
  - `text -7600B`
  - `data/bss` 不变

### 性能证据

| 场景 | 默认开启 | 关闭 row-partial fast path | 变化 |
| --- | ---: | ---: | ---: |
| `RECTANGLE` | `0.179ms` | `0.179ms` | `+0.0%` |
| `CIRCLE` | `0.810ms` | `0.810ms` | `+0.0%` |
| `ROUND_RECTANGLE` | `0.812ms` | `0.812ms` | `+0.0%` |
| `TEXT` | `1.027ms` | `1.027ms` | `+0.0%` |
| `IMAGE_565` | `0.239ms` | `0.239ms` | `+0.0%` |
| `EXTERN_IMAGE_565` | `0.417ms` | `0.417ms` | `+0.0%` |
| `IMAGE_RESIZE_565` | `0.459ms` | `0.460ms` | `+0.2%` |
| `EXTERN_IMAGE_RESIZE_565` | `0.507ms` | `0.507ms` | `+0.0%` |
| `MASK_IMAGE_TEST_PERF_CIRCLE` | `0.904ms` | `3.904ms` | `+331.9%` |
| `EXTERN_MASK_IMAGE_TEST_PERF_CIRCLE` | `1.286ms` | `3.938ms` | `+206.2%` |
| `MASK_IMAGE_TEST_PERF_ROUND_RECT` | `0.586ms` | `0.586ms` | `+0.0%` |
| `EXTERN_MASK_IMAGE_TEST_PERF_ROUND_RECT` | `0.684ms` | `1.495ms` | `+118.6%` |

- 这说明代价集中在依赖 `EGUI_MASK_ROW_PARTIAL` 的 masked-image 热点。
- 当前抽样里 internal round-rect 测点还在噪声内，但 circle 和 external round-rect 已经明显回退。

### 运行/单测

- `HelloPerformance` runtime
  - 默认开启：`241 frames`，`ALL PASSED`
  - 宏关闭：`241 frames`，`ALL PASSED`
- `HelloUnitTest`
  - 默认开启：`688/688 passed`
  - 宏关闭：`688/688 passed`

### 结论

- 这颗宏满足：
  - 代码量收益 `>1KB`
  - 基础 `RECTANGLE / CIRCLE / ROUND_RECTANGLE / TEXT / IMAGE_565 / EXTERN_IMAGE_565 / IMAGE_RESIZE_565 / EXTERN_IMAGE_RESIZE_565` 路径不退化
- 但它不是“无代价回收”，因为 partial-row masked-image 热点会明显回退。
- 因此：
  - 保留 `EGUI_CONFIG_IMAGE_STD_MASK_ROW_PARTIAL_FAST_PATH_ENABLE`
  - 默认值继续保持开启
  - 作为“项目不用 masked-image partial-row 热点”时的条件实验宏是成立的
  - 在这一簇里它的 size 收益明显大于 `ROW_INSIDE`，但仍不进入无条件优先关闭序列

## 本轮继续细拆 A/B: `std image mask row-partial draw fast path`

### 测试环境

- 日期：`2026-04-04`
- 提交基线：`a7d94d1`
- `HelloPerformance` code size A/B
  - `make -j APP=HelloPerformance PORT=qemu CPU_ARCH=cortex-m0plus APP_OBJ_SUFFIX=hpmaskpartialdrawon`
  - `make -j APP=HelloPerformance PORT=qemu CPU_ARCH=cortex-m0plus APP_OBJ_SUFFIX=hpmaskpartialdrawoff USER_CFLAGS=-DEGUI_CONFIG_IMAGE_STD_MASK_ROW_PARTIAL_DRAW_FAST_PATH_ENABLE=0`
- `HelloPerformance` perf A/B
  - `python scripts/perf_analysis/code_perf_check.py --profile cortex-m3 --clean --threshold 1000 --timeout 600`
  - `python scripts/perf_analysis/code_perf_check.py --profile cortex-m3 --clean --threshold 1000 --timeout 600 --extra-cflags=-DEGUI_CONFIG_IMAGE_STD_MASK_ROW_PARTIAL_DRAW_FAST_PATH_ENABLE=0`
- 运行/单测验证
  - `HelloPerformance` baseline / off 通过 `scripts.code_runtime_check.compile_app()` + `run_app()` 注入 `-DEGUI_CONFIG_IMAGE_STD_MASK_ROW_PARTIAL_DRAW_FAST_PATH_ENABLE=0`
  - `make -j APP=HelloUnitTest PORT=pc_test APP_OBJ_SUFFIX=utmaskpartialdrawon`
  - `make -j APP=HelloUnitTest PORT=pc_test APP_OBJ_SUFFIX=utmaskpartialdrawoff USER_CFLAGS=-DEGUI_CONFIG_IMAGE_STD_MASK_ROW_PARTIAL_DRAW_FAST_PATH_ENABLE=0`
  - `output/main.exe`

### 代码量证据

- `HelloPerformance` 链接汇总：
  - 默认开启：`text=2056260 data=72 bss=3832`
  - 关闭 row-partial draw child：`text=2053900 data=72 bss=3832`
- 结论：
  - `text -2360B`
  - `data/bss` 不变

### 性能证据

| 场景 | 默认开启 | 关闭 row-partial draw child | 变化 |
| --- | ---: | ---: | ---: |
| `RECTANGLE` | `0.179ms` | `0.179ms` | `+0.0%` |
| `CIRCLE` | `0.810ms` | `0.810ms` | `+0.0%` |
| `ROUND_RECTANGLE` | `0.812ms` | `0.812ms` | `+0.0%` |
| `TEXT` | `1.027ms` | `1.027ms` | `+0.0%` |
| `IMAGE_565` | `0.239ms` | `0.239ms` | `+0.0%` |
| `EXTERN_IMAGE_565` | `0.417ms` | `0.417ms` | `+0.0%` |
| `IMAGE_RESIZE_565` | `0.459ms` | `0.458ms` | `-0.2%` |
| `EXTERN_IMAGE_RESIZE_565` | `0.507ms` | `0.507ms` | `+0.0%` |
| `IMAGE_565_1` | `0.418ms` | `0.410ms` | `-1.9%` |
| `IMAGE_565_2` | `0.511ms` | `0.502ms` | `-1.8%` |
| `IMAGE_565_4` | `0.693ms` | `0.682ms` | `-1.6%` |
| `IMAGE_565_8` | `0.524ms` | `0.516ms` | `-1.5%` |
| `EXTERN_IMAGE_565_1` | `0.587ms` | `0.576ms` | `-1.9%` |
| `EXTERN_IMAGE_565_2` | `0.680ms` | `0.669ms` | `-1.6%` |
| `EXTERN_IMAGE_565_4` | `0.862ms` | `0.849ms` | `-1.5%` |
| `EXTERN_IMAGE_565_8` | `0.693ms` | `0.689ms` | `-0.6%` |
| `MASK_IMAGE_TEST_PERF_CIRCLE` | `0.904ms` | `0.909ms` | `+0.6%` |
| `EXTERN_MASK_IMAGE_TEST_PERF_CIRCLE` | `1.286ms` | `1.286ms` | `+0.0%` |
| `EXTERN_MASK_IMAGE_TEST_PERF_ROUND_RECT` | `0.684ms` | `0.684ms` | `+0.0%` |
| `EXTERN_MASK_IMAGE_TEST_PERF_IMAGE` | `1.092ms` | `1.092ms` | `+0.0%` |

- 当前 full 239-scene perf run 没有出现 `>=10%` 的回退；最大绝对波动是 `IMAGE_RESIZE_TILED_565_8 / EXTERN_IMAGE_RESIZE_TILED_565_8 -4.5%`，仍在当前噪声带内。
- 基础主路径、当前 raw alpha draw 对照组（`IMAGE_565_1/2/4/8`、`EXTERN_IMAGE_565_1/2/4/8`）以及 sampled masked-image 对照组（`MASK_IMAGE_TEST_PERF_*` / `EXTERN_MASK_IMAGE_TEST_PERF_*`）都没有出现可归因于这颗 child 的明显回退。
- 但当前 perf 集还没有单独隔离 “raw alpha masked direct-draw partial-row” 热点，因此这个结论只对当前 239 场景成立。

### 运行/单测

- `HelloPerformance` runtime
  - baseline / off 都是 `241 frames`
  - `frame_0000.png`、`frame_0120.png`、`frame_0240.png` hash 一致
  - 全量 `241` 帧 `hash mismatch 0`
  - 全量 `241` 帧 `pixel mismatch 0`
- `HelloUnitTest`
  - baseline：`688/688 passed`
  - off：`688/688 passed`

### 结论

- `EGUI_CONFIG_IMAGE_STD_MASK_ROW_PARTIAL_DRAW_FAST_PATH_ENABLE` 当前满足性能隔离目标：
  - 能再独立回收 `2360B`
  - 当前 239 场景 perf 回归里没有出现 `>=10%` 的退化
  - runtime baseline / off 严格渲染等价
- 它比父宏 `ROW_PARTIAL` 更保守：
  - 保住了 partial-row resize 路径
  - 也保住了当前 sampled partial-row 对照组
- 但当前还不适合默认关：
  - 现有 perf 集对 raw alpha masked direct-draw partial-row 热点覆盖仍有限
- 因此：
  - 保留这颗 child 作为条件实验宏记录
  - 如果项目想先保住 partial-row resize / sampled partial-row 场景，应优先试 `ROW_PARTIAL_DRAW`
  - 再考虑 `ROW_PARTIAL_RESIZE`
  - 最后才考虑父宏 `EGUI_CONFIG_IMAGE_STD_MASK_ROW_PARTIAL_FAST_PATH_ENABLE`

## 本轮继续细拆 A/B: `std image mask row-partial draw alpha8 fast path`

### 测试环境

- 日期：`2026-04-04`
- 提交基线：`ec87248`
- `HelloPerformance` code size A/B
  - `make -j APP=HelloPerformance PORT=qemu CPU_ARCH=cortex-m0plus APP_OBJ_SUFFIX=hpmaskpartialdrawa8on`
  - `make -j APP=HelloPerformance PORT=qemu CPU_ARCH=cortex-m0plus APP_OBJ_SUFFIX=hpmaskpartialdrawa8off USER_CFLAGS=-DEGUI_CONFIG_IMAGE_STD_MASK_ROW_PARTIAL_DRAW_ALPHA8_FAST_PATH_ENABLE=0`
- `HelloPerformance` perf A/B
  - `python scripts/perf_analysis/code_perf_check.py --profile cortex-m3 --clean --threshold 1000 --timeout 600`
  - `python scripts/perf_analysis/code_perf_check.py --profile cortex-m3 --clean --threshold 1000 --timeout 600 --extra-cflags=-DEGUI_CONFIG_IMAGE_STD_MASK_ROW_PARTIAL_DRAW_ALPHA8_FAST_PATH_ENABLE=0`
- 运行/单测验证
  - `HelloPerformance` baseline / off 通过 `scripts.code_runtime_check.compile_app()` + `run_app()` 注入 `-DEGUI_CONFIG_IMAGE_STD_MASK_ROW_PARTIAL_DRAW_ALPHA8_FAST_PATH_ENABLE=0`
  - `make -j APP=HelloUnitTest PORT=pc_test APP_OBJ_SUFFIX=utmaskpartialdrawa8on`
  - `make -j APP=HelloUnitTest PORT=pc_test APP_OBJ_SUFFIX=utmaskpartialdrawa8off USER_CFLAGS=-DEGUI_CONFIG_IMAGE_STD_MASK_ROW_PARTIAL_DRAW_ALPHA8_FAST_PATH_ENABLE=0`
  - `output/main.exe`

### 代码量证据

- `HelloPerformance` 链接汇总：
  - 默认开启：`text=2056264 data=72 bss=3832`
  - 关闭 row-partial draw alpha8 child：`text=2055168 data=72 bss=3832`
- 结论：
  - `text -1096B`
  - `data/bss` 不变

### 性能证据

| 场景 | 默认开启 | 关闭 row-partial draw alpha8 child | 变化 |
| --- | ---: | ---: | ---: |
| `RECTANGLE` | `0.179ms` | `0.179ms` | `+0.0%` |
| `CIRCLE` | `0.810ms` | `0.810ms` | `+0.0%` |
| `ROUND_RECTANGLE` | `0.812ms` | `0.812ms` | `+0.0%` |
| `TEXT` | `1.027ms` | `1.027ms` | `+0.0%` |
| `IMAGE_565` | `0.239ms` | `0.239ms` | `+0.0%` |
| `EXTERN_IMAGE_565` | `0.417ms` | `0.417ms` | `+0.0%` |
| `IMAGE_RESIZE_565` | `0.459ms` | `0.459ms` | `+0.0%` |
| `EXTERN_IMAGE_RESIZE_565` | `0.507ms` | `0.507ms` | `+0.0%` |
| `IMAGE_565_8` | `0.524ms` | `0.525ms` | `+0.2%` |
| `EXTERN_IMAGE_565_8` | `0.693ms` | `0.698ms` | `+0.7%` |
| `MASK_IMAGE_TEST_PERF_CIRCLE` | `0.904ms` | `0.904ms` | `+0.0%` |
| `EXTERN_MASK_IMAGE_TEST_PERF_CIRCLE` | `1.286ms` | `1.286ms` | `+0.0%` |
| `EXTERN_MASK_IMAGE_TEST_PERF_ROUND_RECT` | `0.684ms` | `0.684ms` | `+0.0%` |
| `EXTERN_MASK_IMAGE_TEST_PERF_IMAGE` | `1.092ms` | `1.092ms` | `+0.0%` |
| `MASK_IMAGE_QOI_8_CIRCLE` | `2.449ms` | `2.449ms` | `+0.0%` |
| `EXTERN_MASK_IMAGE_QOI_8_CIRCLE` | `3.005ms` | `3.005ms` | `+0.0%` |
| `MASK_IMAGE_RLE_8_CIRCLE` | `1.719ms` | `1.719ms` | `+0.0%` |
| `EXTERN_MASK_IMAGE_RLE_8_CIRCLE` | `2.512ms` | `2.512ms` | `+0.0%` |

- 当前 full 239-scene perf run 仍然没有出现 `>=10%` 的回退；最大绝对波动是 `IMAGE_TILED_565_4 / IMAGE_TILED_STAR_565_4 +4.5%`。
- 基础主路径、当前 alpha8 raw draw 对照组（`IMAGE_565_8`、`EXTERN_IMAGE_565_8`）以及当前 sampled alpha8 masked-image draw 对照组（`MASK_IMAGE_QOI_8_CIRCLE`、`MASK_IMAGE_RLE_8_CIRCLE`、`EXTERN_MASK_IMAGE_QOI_8_CIRCLE`、`EXTERN_MASK_IMAGE_RLE_8_CIRCLE`）都没有出现可归因于这颗 child 的明显回退。
- 但当前 perf 集仍然没有单独隔离 raw alpha8 masked direct-draw partial-row 热点，因此结论依然只对当前 239 场景成立。

### 运行/单测

- `HelloPerformance` runtime
  - baseline / off 都是 `241 frames`
  - `frame_0000.png`、`frame_0120.png`、`frame_0240.png` hash 一致
  - 全量 `241` 帧 `hash mismatch 0`
  - 全量 `241` 帧 `pixel mismatch 0`
- `HelloUnitTest`
  - baseline：`688/688 passed`
  - off：`688/688 passed`

### 结论

- `EGUI_CONFIG_IMAGE_STD_MASK_ROW_PARTIAL_DRAW_ALPHA8_FAST_PATH_ENABLE` 当前满足性能隔离目标：
  - 能再独立回收 `1096B`
  - 当前 239 场景 perf 回归里没有出现 `>=10%` 的退化
  - runtime baseline / off 严格渲染等价
- 它比父宏 `ROW_PARTIAL_DRAW` 更保守：
  - 只牺牲 `RGB565_8` alpha8 direct-draw partial-row
  - packed 4/2/1 partial-row draw 继续保留
- 但当前仍不适合默认关：
  - 现有 perf 集对 raw alpha8 masked direct-draw partial-row 热点覆盖仍有限
- 因此：
  - 保留这颗 child 作为条件实验宏记录
  - 如果项目想先保住 packed-alpha partial-row draw，应优先试 `ROW_PARTIAL_DRAW_ALPHA8`
  - 再试 `ROW_PARTIAL_DRAW`
  - 然后再考虑 `ROW_PARTIAL_RESIZE`
  - 最后才考虑父宏 `EGUI_CONFIG_IMAGE_STD_MASK_ROW_PARTIAL_FAST_PATH_ENABLE`

## 本轮继续细拆 A/B: `std image mask row-partial draw packed-alpha fast path`

### 测试环境

- 日期：`2026-04-04`
- 提交基线：`7831025`
- `HelloPerformance` code size A/B
  - `make -j APP=HelloPerformance PORT=qemu CPU_ARCH=cortex-m0plus APP_OBJ_SUFFIX=hpmaskpartialdrawpackon`
  - `make -j APP=HelloPerformance PORT=qemu CPU_ARCH=cortex-m0plus APP_OBJ_SUFFIX=hpmaskpartialdrawpackoff USER_CFLAGS=-DEGUI_CONFIG_IMAGE_STD_MASK_ROW_PARTIAL_DRAW_PACKED_ALPHA_FAST_PATH_ENABLE=0`
- `HelloPerformance` perf A/B
  - `python scripts/perf_analysis/code_perf_check.py --profile cortex-m3 --clean --threshold 1000 --timeout 600`
  - `python scripts/perf_analysis/code_perf_check.py --profile cortex-m3 --clean --threshold 1000 --timeout 600 --extra-cflags=-DEGUI_CONFIG_IMAGE_STD_MASK_ROW_PARTIAL_DRAW_PACKED_ALPHA_FAST_PATH_ENABLE=0`
- 运行/单测验证
  - `HelloPerformance` baseline / off 通过 `scripts.code_runtime_check.compile_app()` + `run_app()` 注入 `-DEGUI_CONFIG_IMAGE_STD_MASK_ROW_PARTIAL_DRAW_PACKED_ALPHA_FAST_PATH_ENABLE=0`
  - `make -j APP=HelloUnitTest PORT=pc_test APP_OBJ_SUFFIX=utmaskpartialdrawpackon`
  - `make -j APP=HelloUnitTest PORT=pc_test APP_OBJ_SUFFIX=utmaskpartialdrawpackoff USER_CFLAGS=-DEGUI_CONFIG_IMAGE_STD_MASK_ROW_PARTIAL_DRAW_PACKED_ALPHA_FAST_PATH_ENABLE=0`
  - `output/main.exe`

### 代码量证据

- `HelloPerformance` 链接汇总：
  - 默认开启：`text=2056264 data=72 bss=3832`
  - 关闭 row-partial draw packed child：`text=2054904 data=72 bss=3832`
- 结论：
  - `text -1360B`
  - `data/bss` 不变

### 性能证据

| 场景 | 默认开启 | 关闭 row-partial draw packed child | 变化 |
| --- | ---: | ---: | ---: |
| `RECTANGLE` | `0.179ms` | `0.179ms` | `+0.0%` |
| `CIRCLE` | `0.810ms` | `0.810ms` | `+0.0%` |
| `ROUND_RECTANGLE` | `0.812ms` | `0.812ms` | `+0.0%` |
| `TEXT` | `1.027ms` | `1.027ms` | `+0.0%` |
| `IMAGE_565` | `0.239ms` | `0.239ms` | `+0.0%` |
| `EXTERN_IMAGE_565` | `0.417ms` | `0.417ms` | `+0.0%` |
| `IMAGE_RESIZE_565` | `0.459ms` | `0.458ms` | `-0.2%` |
| `EXTERN_IMAGE_RESIZE_565` | `0.507ms` | `0.507ms` | `+0.0%` |
| `IMAGE_565_1` | `0.418ms` | `0.410ms` | `-1.9%` |
| `IMAGE_565_2` | `0.511ms` | `0.502ms` | `-1.8%` |
| `IMAGE_565_4` | `0.693ms` | `0.682ms` | `-1.6%` |
| `EXTERN_IMAGE_565_1` | `0.587ms` | `0.577ms` | `-1.7%` |
| `EXTERN_IMAGE_565_2` | `0.680ms` | `0.669ms` | `-1.6%` |
| `EXTERN_IMAGE_565_4` | `0.862ms` | `0.849ms` | `-1.5%` |
| `MASK_IMAGE_QOI_CIRCLE` | `6.614ms` | `6.614ms` | `+0.0%` |
| `MASK_IMAGE_QOI_8_CIRCLE` | `2.449ms` | `2.442ms` | `-0.3%` |
| `MASK_IMAGE_RLE_CIRCLE` | `2.861ms` | `2.861ms` | `+0.0%` |
| `MASK_IMAGE_RLE_8_CIRCLE` | `1.719ms` | `1.712ms` | `-0.4%` |
| `EXTERN_MASK_IMAGE_QOI_CIRCLE` | `16.844ms` | `16.845ms` | `+0.0%` |
| `EXTERN_MASK_IMAGE_QOI_8_CIRCLE` | `3.005ms` | `2.998ms` | `-0.2%` |
| `EXTERN_MASK_IMAGE_RLE_CIRCLE` | `5.370ms` | `5.370ms` | `+0.0%` |
| `EXTERN_MASK_IMAGE_RLE_8_CIRCLE` | `2.512ms` | `2.505ms` | `-0.3%` |
| `MASK_IMAGE_TEST_PERF_CIRCLE` | `0.904ms` | `0.909ms` | `+0.6%` |
| `EXTERN_MASK_IMAGE_TEST_PERF_CIRCLE` | `1.286ms` | `1.286ms` | `+0.0%` |
| `EXTERN_MASK_IMAGE_TEST_PERF_ROUND_RECT` | `0.684ms` | `0.684ms` | `+0.0%` |
| `EXTERN_MASK_IMAGE_TEST_PERF_IMAGE` | `1.092ms` | `1.092ms` | `+0.0%` |

- 当前 full 239-scene perf run 仍然没有出现 `>=10%` 的回退。
- 基础主路径、当前 packed raw draw 对照组（`IMAGE_565_1/2/4`、`EXTERN_IMAGE_565_1/2/4`）以及当前 sampled packed masked-image draw 对照组（`MASK_IMAGE_QOI_CIRCLE / MASK_IMAGE_RLE_CIRCLE / EXTERN_MASK_IMAGE_QOI_CIRCLE / EXTERN_MASK_IMAGE_RLE_CIRCLE`，含 alpha8 对照项）都没有出现可归因于这颗 child 的明显回退。
- 但当前 perf 集仍然没有单独隔离 raw packed-alpha masked direct-draw partial-row 热点，因此结论依然只对当前 239 场景成立。

### 运行/单测

- `HelloPerformance` runtime
  - baseline / off 都是 `241 frames`
  - `frame_0000.png`、`frame_0120.png`、`frame_0240.png` hash 一致
  - 全量 `241` 帧 `hash mismatch 0`
  - 全量 `241` 帧 `pixel mismatch 0`
- `HelloUnitTest`
  - baseline：`688/688 passed`
  - off：`688/688 passed`

### 结论

- `EGUI_CONFIG_IMAGE_STD_MASK_ROW_PARTIAL_DRAW_PACKED_ALPHA_FAST_PATH_ENABLE` 当前满足性能隔离目标：
  - 能再独立回收 `1360B`
  - 当前 239 场景 perf 回归里没有出现 `>=10%` 的退化
  - runtime baseline / off 严格渲染等价
- 它比父宏 `ROW_PARTIAL_DRAW` 更保守：
  - 只牺牲 packed `RGB565_4/2/1` direct-draw partial-row
  - `RGB565_8` alpha8 direct-draw partial-row 继续保留
- 但当前仍不适合默认关：
  - 现有 perf 集对 raw packed-alpha masked direct-draw partial-row 热点覆盖仍有限
- 因此：
  - 保留这颗 child 作为条件实验宏记录
  - 如果项目想先保住 alpha8 partial-row draw，应优先试 `ROW_PARTIAL_DRAW_PACKED_ALPHA`
  - 再试 `ROW_PARTIAL_DRAW`
  - 然后再考虑 `ROW_PARTIAL_RESIZE`
  - 最后才考虑父宏 `EGUI_CONFIG_IMAGE_STD_MASK_ROW_PARTIAL_FAST_PATH_ENABLE`

## 本轮继续细拆 A/B: `std image mask row-partial resize fast path`

### 测试环境

- 日期：`2026-04-04`
- 提交基线：`6b8a296`
- `HelloPerformance` code size A/B
  - `make clean`
  - `make -j APP=HelloPerformance PORT=qemu CPU_ARCH=cortex-m0plus APP_OBJ_SUFFIX=hpmaskpartialresizeon`
  - `make -j APP=HelloPerformance PORT=qemu CPU_ARCH=cortex-m0plus APP_OBJ_SUFFIX=hpmaskpartialresizeoff USER_CFLAGS=-DEGUI_CONFIG_IMAGE_STD_MASK_ROW_PARTIAL_RESIZE_FAST_PATH_ENABLE=0`
- `HelloPerformance` perf A/B
  - `python scripts/perf_analysis/code_perf_check.py --profile cortex-m3 --clean --threshold 1000 --filter RECTANGLE,CIRCLE,ROUND_RECTANGLE,TEXT,IMAGE_565,EXTERN_IMAGE_565,IMAGE_RESIZE_565,EXTERN_IMAGE_RESIZE_565,MASK_IMAGE_NO_MASK,MASK_IMAGE_ROUND_RECT,MASK_IMAGE_CIRCLE,MASK_IMAGE_IMAGE,MASK_IMAGE_CIRCLE_QUARTER,MASK_IMAGE_CIRCLE_DOUBLE,MASK_IMAGE_ROUND_RECT_QUARTER,MASK_IMAGE_ROUND_RECT_DOUBLE,EXTERN_MASK_IMAGE_NO_MASK,EXTERN_MASK_IMAGE_ROUND_RECT,EXTERN_MASK_IMAGE_CIRCLE,EXTERN_MASK_IMAGE_IMAGE,MASK_IMAGE_TEST_PERF_NO_MASK,MASK_IMAGE_TEST_PERF_ROUND_RECT,MASK_IMAGE_TEST_PERF_CIRCLE,MASK_IMAGE_TEST_PERF_IMAGE,EXTERN_MASK_IMAGE_TEST_PERF_NO_MASK,EXTERN_MASK_IMAGE_TEST_PERF_ROUND_RECT,EXTERN_MASK_IMAGE_TEST_PERF_CIRCLE,EXTERN_MASK_IMAGE_TEST_PERF_IMAGE`
  - `python scripts/perf_analysis/code_perf_check.py --profile cortex-m3 --clean --threshold 1000 --filter RECTANGLE,CIRCLE,ROUND_RECTANGLE,TEXT,IMAGE_565,EXTERN_IMAGE_565,IMAGE_RESIZE_565,EXTERN_IMAGE_RESIZE_565,MASK_IMAGE_NO_MASK,MASK_IMAGE_ROUND_RECT,MASK_IMAGE_CIRCLE,MASK_IMAGE_IMAGE,MASK_IMAGE_CIRCLE_QUARTER,MASK_IMAGE_CIRCLE_DOUBLE,MASK_IMAGE_ROUND_RECT_QUARTER,MASK_IMAGE_ROUND_RECT_DOUBLE,EXTERN_MASK_IMAGE_NO_MASK,EXTERN_MASK_IMAGE_ROUND_RECT,EXTERN_MASK_IMAGE_CIRCLE,EXTERN_MASK_IMAGE_IMAGE,MASK_IMAGE_TEST_PERF_NO_MASK,MASK_IMAGE_TEST_PERF_ROUND_RECT,MASK_IMAGE_TEST_PERF_CIRCLE,MASK_IMAGE_TEST_PERF_IMAGE,EXTERN_MASK_IMAGE_TEST_PERF_NO_MASK,EXTERN_MASK_IMAGE_TEST_PERF_ROUND_RECT,EXTERN_MASK_IMAGE_TEST_PERF_CIRCLE,EXTERN_MASK_IMAGE_TEST_PERF_IMAGE --extra-cflags=-DEGUI_CONFIG_IMAGE_STD_MASK_ROW_PARTIAL_RESIZE_FAST_PATH_ENABLE=0`
- 运行/单测验证
  - `HelloPerformance` baseline / off 通过 `scripts.code_runtime_check.compile_app()` + `run_app()` 注入默认配置和 `-DEGUI_CONFIG_IMAGE_STD_MASK_ROW_PARTIAL_RESIZE_FAST_PATH_ENABLE=0`
  - `make clean`
  - `make -j APP=HelloUnitTest PORT=pc_test APP_OBJ_SUFFIX=utmaskrowpartialresize USER_CFLAGS=-DEGUI_CONFIG_IMAGE_STD_MASK_ROW_PARTIAL_RESIZE_FAST_PATH_ENABLE=0`
  - `output/main.exe`

### 代码量证据

- `HelloPerformance` 链接汇总：
  - 默认开启：`text=2056256 data=72 bss=3832`
  - 关闭 row-partial resize child：`text=2049980 data=72 bss=3832`
- 结论：
  - `text -6276B`
  - `data/bss` 不变

### 性能证据

| 场景 | 默认开启 | 关闭 row-partial resize child | 变化 |
| --- | ---: | ---: | ---: |
| `RECTANGLE` | `0.179ms` | `0.179ms` | `+0.0%` |
| `CIRCLE` | `0.810ms` | `0.810ms` | `+0.0%` |
| `ROUND_RECTANGLE` | `0.812ms` | `0.812ms` | `+0.0%` |
| `TEXT` | `1.027ms` | `1.027ms` | `+0.0%` |
| `IMAGE_565` | `0.239ms` | `0.239ms` | `+0.0%` |
| `EXTERN_IMAGE_565` | `0.417ms` | `0.417ms` | `+0.0%` |
| `IMAGE_RESIZE_565` | `0.459ms` | `0.459ms` | `+0.0%` |
| `EXTERN_IMAGE_RESIZE_565` | `0.507ms` | `0.506ms` | `-0.2%` |
| `MASK_IMAGE_NO_MASK` | `0.634ms` | `0.634ms` | `+0.0%` |
| `MASK_IMAGE_ROUND_RECT` | `0.969ms` | `0.967ms` | `-0.2%` |
| `MASK_IMAGE_CIRCLE` | `1.431ms` | `3.549ms` | `+148.0%` |
| `MASK_IMAGE_IMAGE` | `0.296ms` | `0.297ms` | `+0.3%` |
| `MASK_IMAGE_CIRCLE_QUARTER` | `0.453ms` | `0.981ms` | `+116.6%` |
| `MASK_IMAGE_CIRCLE_DOUBLE` | `1.316ms` | `2.374ms` | `+80.4%` |
| `MASK_IMAGE_ROUND_RECT_QUARTER` | `0.326ms` | `0.326ms` | `+0.0%` |
| `MASK_IMAGE_ROUND_RECT_DOUBLE` | `0.943ms` | `0.942ms` | `-0.1%` |
| `EXTERN_MASK_IMAGE_NO_MASK` | `0.697ms` | `0.697ms` | `+0.0%` |
| `EXTERN_MASK_IMAGE_ROUND_RECT` | `1.035ms` | `1.034ms` | `-0.1%` |
| `EXTERN_MASK_IMAGE_CIRCLE` | `1.494ms` | `3.610ms` | `+141.6%` |
| `EXTERN_MASK_IMAGE_IMAGE` | `0.331ms` | `0.331ms` | `+0.0%` |
| `MASK_IMAGE_TEST_PERF_NO_MASK` | `0.459ms` | `0.459ms` | `+0.0%` |
| `MASK_IMAGE_TEST_PERF_ROUND_RECT` | `0.586ms` | `0.586ms` | `+0.0%` |
| `MASK_IMAGE_TEST_PERF_CIRCLE` | `0.904ms` | `3.904ms` | `+331.9%` |
| `MASK_IMAGE_TEST_PERF_IMAGE` | `1.661ms` | `1.662ms` | `+0.1%` |
| `EXTERN_MASK_IMAGE_TEST_PERF_NO_MASK` | `0.507ms` | `0.506ms` | `-0.2%` |
| `EXTERN_MASK_IMAGE_TEST_PERF_ROUND_RECT` | `0.684ms` | `1.495ms` | `+118.6%` |
| `EXTERN_MASK_IMAGE_TEST_PERF_CIRCLE` | `1.286ms` | `3.938ms` | `+206.2%` |
| `EXTERN_MASK_IMAGE_TEST_PERF_IMAGE` | `1.092ms` | `2.072ms` | `+89.7%` |

- 这说明新 child 基本只打掉了 masked-image resize 里的 `EGUI_MASK_ROW_PARTIAL` 分支。
- 基础 draw / resize 主路径不退化，普通 no-mask / round-rect / image masked resize 样本也都保持在噪声内。
- 代价集中在 circle 与 external partial-row masked resize 热点。

### 运行/单测

- `HelloPerformance` runtime
  - baseline / off 都是 `241 frames`
  - `frame_0000.png`、`frame_0120.png`、`frame_0240.png` hash 一致
  - 全量只有 `5` 帧 PNG 文件 hash 不同，但逐帧像素 diff 都是 `0`
- `HelloUnitTest`
  - off：`688/688 passed`

### 结论

- `EGUI_CONFIG_IMAGE_STD_MASK_ROW_PARTIAL_RESIZE_FAST_PATH_ENABLE` 满足：
  - 代码量收益 `>1KB`
  - 基础 `RECTANGLE / CIRCLE / ROUND_RECTANGLE / TEXT / IMAGE_565 / EXTERN_IMAGE_565 / IMAGE_RESIZE_565 / EXTERN_IMAGE_RESIZE_565` 路径不退化
  - 代价比父宏 `ROW_PARTIAL` 更集中，只落在 partial-row masked-image resize 热点
- 因此：
  - 保留 `EGUI_CONFIG_IMAGE_STD_MASK_ROW_PARTIAL_RESIZE_FAST_PATH_ENABLE`
  - 默认值继续保持开启
  - 若项目要保住 masked-image draw，但可以牺牲 circle / external partial-row resize 热点，应先试这颗 child，再考虑更激进的 `EGUI_CONFIG_IMAGE_STD_MASK_ROW_PARTIAL_FAST_PATH_ENABLE`

## 本轮继续细拆 A/B: `std image mask row-partial resize alpha8 fast path`

### 测试环境

- 日期：`2026-04-04`
- 提交基线：`4ffb8bf`
- `HelloPerformance` code size A/B
  - `make -B -j APP=HelloPerformance PORT=qemu CPU_ARCH=cortex-m0plus APP_OBJ_SUFFIX=hpmaskpartialresizea8on USER_CFLAGS=-DEGUI_CONFIG_IMAGE_STD_MASK_ROW_PARTIAL_RESIZE_ALPHA8_FAST_PATH_ENABLE=1`
  - `make -B -j APP=HelloPerformance PORT=qemu CPU_ARCH=cortex-m0plus APP_OBJ_SUFFIX=hpmaskpartialresizea8off USER_CFLAGS=-DEGUI_CONFIG_IMAGE_STD_MASK_ROW_PARTIAL_RESIZE_ALPHA8_FAST_PATH_ENABLE=0`
- `HelloPerformance` perf A/B
  - `python scripts/perf_analysis/code_perf_check.py --profile cortex-m3 --clean --threshold 1000 --timeout 600 --filter RECTANGLE,CIRCLE,ROUND_RECTANGLE,TEXT,IMAGE_565,EXTERN_IMAGE_565,IMAGE_RESIZE_565,EXTERN_IMAGE_RESIZE_565,MASK_IMAGE_NO_MASK,MASK_IMAGE_ROUND_RECT,MASK_IMAGE_CIRCLE,MASK_IMAGE_IMAGE,MASK_IMAGE_CIRCLE_QUARTER,MASK_IMAGE_CIRCLE_DOUBLE,MASK_IMAGE_ROUND_RECT_QUARTER,MASK_IMAGE_ROUND_RECT_DOUBLE,EXTERN_MASK_IMAGE_NO_MASK,EXTERN_MASK_IMAGE_ROUND_RECT,EXTERN_MASK_IMAGE_CIRCLE,EXTERN_MASK_IMAGE_IMAGE,MASK_IMAGE_TEST_PERF_NO_MASK,MASK_IMAGE_TEST_PERF_ROUND_RECT,MASK_IMAGE_TEST_PERF_CIRCLE,MASK_IMAGE_TEST_PERF_IMAGE,EXTERN_MASK_IMAGE_TEST_PERF_NO_MASK,EXTERN_MASK_IMAGE_TEST_PERF_ROUND_RECT,EXTERN_MASK_IMAGE_TEST_PERF_CIRCLE,EXTERN_MASK_IMAGE_TEST_PERF_IMAGE --extra-cflags=-DEGUI_CONFIG_IMAGE_STD_MASK_ROW_PARTIAL_RESIZE_ALPHA8_FAST_PATH_ENABLE=1`
  - `python scripts/perf_analysis/code_perf_check.py --profile cortex-m3 --clean --threshold 1000 --timeout 600 --filter RECTANGLE,CIRCLE,ROUND_RECTANGLE,TEXT,IMAGE_565,EXTERN_IMAGE_565,IMAGE_RESIZE_565,EXTERN_IMAGE_RESIZE_565,MASK_IMAGE_NO_MASK,MASK_IMAGE_ROUND_RECT,MASK_IMAGE_CIRCLE,MASK_IMAGE_IMAGE,MASK_IMAGE_CIRCLE_QUARTER,MASK_IMAGE_CIRCLE_DOUBLE,MASK_IMAGE_ROUND_RECT_QUARTER,MASK_IMAGE_ROUND_RECT_DOUBLE,EXTERN_MASK_IMAGE_NO_MASK,EXTERN_MASK_IMAGE_ROUND_RECT,EXTERN_MASK_IMAGE_CIRCLE,EXTERN_MASK_IMAGE_IMAGE,MASK_IMAGE_TEST_PERF_NO_MASK,MASK_IMAGE_TEST_PERF_ROUND_RECT,MASK_IMAGE_TEST_PERF_CIRCLE,MASK_IMAGE_TEST_PERF_IMAGE,EXTERN_MASK_IMAGE_TEST_PERF_NO_MASK,EXTERN_MASK_IMAGE_TEST_PERF_ROUND_RECT,EXTERN_MASK_IMAGE_TEST_PERF_CIRCLE,EXTERN_MASK_IMAGE_TEST_PERF_IMAGE --extra-cflags=-DEGUI_CONFIG_IMAGE_STD_MASK_ROW_PARTIAL_RESIZE_ALPHA8_FAST_PATH_ENABLE=0`
- 运行/单测验证
  - `HelloPerformance` baseline / off 通过 `scripts.code_runtime_check.compile_app()` + `run_app()` 分别注入 `-DEGUI_CONFIG_IMAGE_STD_MASK_ROW_PARTIAL_RESIZE_ALPHA8_FAST_PATH_ENABLE=1/0`
  - `make -j APP=HelloUnitTest PORT=pc_test APP_OBJ_SUFFIX=utmaskrowpartialresizea8on USER_CFLAGS=-DEGUI_CONFIG_IMAGE_STD_MASK_ROW_PARTIAL_RESIZE_ALPHA8_FAST_PATH_ENABLE=1`
  - `make -j APP=HelloUnitTest PORT=pc_test APP_OBJ_SUFFIX=utmaskrowpartialresizea8off USER_CFLAGS=-DEGUI_CONFIG_IMAGE_STD_MASK_ROW_PARTIAL_RESIZE_ALPHA8_FAST_PATH_ENABLE=0`
  - `output/main.exe`

### 代码量证据

- `HelloPerformance` 链接汇总：
  - 默认开启：`text=2056264 data=72 bss=3832`
  - 关闭 row-partial resize alpha8 child：`text=2055264 data=72 bss=3832`
- 结论：
  - `text -1000B`
  - `data/bss` 不变

### 性能证据

| 场景 | 默认开启 | 关闭 row-partial resize alpha8 child | 变化 |
| --- | ---: | ---: | ---: |
| `RECTANGLE` | `0.179ms` | `0.179ms` | `+0.0%` |
| `CIRCLE` | `0.810ms` | `0.810ms` | `+0.0%` |
| `ROUND_RECTANGLE` | `0.812ms` | `0.812ms` | `+0.0%` |
| `TEXT` | `1.027ms` | `1.027ms` | `+0.0%` |
| `IMAGE_565` | `0.239ms` | `0.239ms` | `+0.0%` |
| `EXTERN_IMAGE_565` | `0.417ms` | `0.417ms` | `+0.0%` |
| `IMAGE_RESIZE_565` | `0.459ms` | `0.459ms` | `+0.0%` |
| `EXTERN_IMAGE_RESIZE_565` | `0.507ms` | `0.507ms` | `+0.0%` |
| `MASK_IMAGE_NO_MASK` | `0.634ms` | `0.634ms` | `+0.0%` |
| `MASK_IMAGE_ROUND_RECT` | `0.969ms` | `0.967ms` | `-0.2%` |
| `MASK_IMAGE_CIRCLE` | `1.431ms` | `3.549ms` | `+148.0%` |
| `MASK_IMAGE_IMAGE` | `0.296ms` | `0.297ms` | `+0.3%` |
| `MASK_IMAGE_CIRCLE_QUARTER` | `0.453ms` | `0.981ms` | `+116.6%` |
| `MASK_IMAGE_CIRCLE_DOUBLE` | `1.316ms` | `2.374ms` | `+80.4%` |
| `MASK_IMAGE_ROUND_RECT_QUARTER` | `0.326ms` | `0.326ms` | `+0.0%` |
| `MASK_IMAGE_ROUND_RECT_DOUBLE` | `0.943ms` | `0.942ms` | `-0.1%` |
| `EXTERN_MASK_IMAGE_NO_MASK` | `0.697ms` | `0.697ms` | `+0.0%` |
| `EXTERN_MASK_IMAGE_ROUND_RECT` | `1.035ms` | `1.034ms` | `-0.1%` |
| `EXTERN_MASK_IMAGE_CIRCLE` | `1.494ms` | `3.610ms` | `+141.6%` |
| `EXTERN_MASK_IMAGE_IMAGE` | `0.331ms` | `0.331ms` | `+0.0%` |
| `MASK_IMAGE_TEST_PERF_NO_MASK` | `0.459ms` | `0.459ms` | `+0.0%` |
| `MASK_IMAGE_TEST_PERF_ROUND_RECT` | `0.586ms` | `0.586ms` | `+0.0%` |
| `MASK_IMAGE_TEST_PERF_CIRCLE` | `0.904ms` | `0.904ms` | `+0.0%` |
| `MASK_IMAGE_TEST_PERF_IMAGE` | `1.661ms` | `1.662ms` | `+0.1%` |
| `EXTERN_MASK_IMAGE_TEST_PERF_NO_MASK` | `0.507ms` | `0.507ms` | `+0.0%` |
| `EXTERN_MASK_IMAGE_TEST_PERF_ROUND_RECT` | `0.684ms` | `0.684ms` | `+0.0%` |
| `EXTERN_MASK_IMAGE_TEST_PERF_CIRCLE` | `1.286ms` | `1.286ms` | `+0.0%` |
| `EXTERN_MASK_IMAGE_TEST_PERF_IMAGE` | `1.092ms` | `1.092ms` | `+0.0%` |

- 这说明新 child 只把代价继续收敛到 `RGB565_8` alpha8 circle masked-image resize 热点：
  - 基础 `RECTANGLE / CIRCLE / ROUND_RECTANGLE / TEXT / IMAGE_565 / EXTERN_IMAGE_565 / IMAGE_RESIZE_565 / EXTERN_IMAGE_RESIZE_565` 不退化
  - raw sampled masked-image resize 对照组不退化：`MASK_IMAGE_TEST_PERF_CIRCLE / EXTERN_MASK_IMAGE_TEST_PERF_CIRCLE / EXTERN_MASK_IMAGE_TEST_PERF_ROUND_RECT`
  - 代价集中在 alpha8 circle resize 样本：`MASK_IMAGE_CIRCLE`、`EXTERN_MASK_IMAGE_CIRCLE`、`MASK_IMAGE_CIRCLE_QUARTER`、`MASK_IMAGE_CIRCLE_DOUBLE`

### 运行/单测

- `HelloPerformance` runtime
  - baseline / off 都是 `241 frames`
  - `frame_0000.png`、`frame_0120.png`、`frame_0240.png` hash 一致
  - `hash mismatch 4` 帧：`frame_0168.png` / `frame_0172.png` / `frame_0178.png` / `frame_0179.png`
  - `pixel mismatch 4`
  - 差异集中在 circle 边缘的细小抗锯齿像素，不是整块渲染错位
- `HelloUnitTest`
  - baseline：`688/688 passed`
  - off：`688/688 passed`

### 结论

- `EGUI_CONFIG_IMAGE_STD_MASK_ROW_PARTIAL_RESIZE_ALPHA8_FAST_PATH_ENABLE` 在性能隔离上成立：
  - 能再独立回收 `1000B`
  - raw sampled masked-image resize 热点继续保住
  - perf 代价只落在 alpha8 circle masked-image resize 热点
- 但它与 baseline 不是严格渲染等价：
  - runtime baseline / off 存在 `4` 帧真实像素差异
  - 差异位于 circle 边缘抗锯齿线
- 因此：
  - 保留这颗 child 作为条件实验宏
  - 不进入当前优先保留序列，优先级低于父宏 `ROW_PARTIAL_RESIZE`
  - 如果项目要求 on/off 渲染完全等价，当前不建议默认关这颗宏

## 本轮继续细拆 A/B: `std image mask row-partial resize packed-alpha fast path`

### 测试环境

- 日期：`2026-04-04`
- 提交基线：`56f59f8`
- `HelloPerformance` code size A/B
  - `make -B -j APP=HelloPerformance PORT=qemu CPU_ARCH=cortex-m0plus APP_OBJ_SUFFIX=hpmaskpartialresizepackon USER_CFLAGS=-DEGUI_CONFIG_IMAGE_STD_MASK_ROW_PARTIAL_RESIZE_PACKED_ALPHA_FAST_PATH_ENABLE=1`
  - `make -B -j APP=HelloPerformance PORT=qemu CPU_ARCH=cortex-m0plus APP_OBJ_SUFFIX=hpmaskpartialresizepackoff USER_CFLAGS=-DEGUI_CONFIG_IMAGE_STD_MASK_ROW_PARTIAL_RESIZE_PACKED_ALPHA_FAST_PATH_ENABLE=0`
- `HelloPerformance` perf A/B
  - `python scripts/perf_analysis/code_perf_check.py --profile cortex-m3 --clean --threshold 1000 --timeout 600 --extra-cflags=-DEGUI_CONFIG_IMAGE_STD_MASK_ROW_PARTIAL_RESIZE_PACKED_ALPHA_FAST_PATH_ENABLE=1`
  - `python scripts/perf_analysis/code_perf_check.py --profile cortex-m3 --clean --threshold 1000 --timeout 600 --extra-cflags=-DEGUI_CONFIG_IMAGE_STD_MASK_ROW_PARTIAL_RESIZE_PACKED_ALPHA_FAST_PATH_ENABLE=0`
- 运行/单测验证
  - `HelloPerformance` baseline / off 通过 `scripts.code_runtime_check.compile_app()` + `run_app(timeout=35)` 注入 `-DEGUI_CONFIG_IMAGE_STD_MASK_ROW_PARTIAL_RESIZE_PACKED_ALPHA_FAST_PATH_ENABLE=1/0`
  - `make -j APP=HelloUnitTest PORT=pc_test APP_OBJ_SUFFIX=utmaskrowpartialresizepackon USER_CFLAGS=-DEGUI_CONFIG_IMAGE_STD_MASK_ROW_PARTIAL_RESIZE_PACKED_ALPHA_FAST_PATH_ENABLE=1`
  - `make -j APP=HelloUnitTest PORT=pc_test APP_OBJ_SUFFIX=utmaskrowpartialresizepackoff USER_CFLAGS=-DEGUI_CONFIG_IMAGE_STD_MASK_ROW_PARTIAL_RESIZE_PACKED_ALPHA_FAST_PATH_ENABLE=0`
  - `output/main.exe`

### 代码量证据

- `HelloPerformance` 链接汇总：
  - 默认开启：`text=2056264 data=72 bss=3832`
  - 关闭 row-partial resize packed child：`text=2053828 data=72 bss=3832`
- 结论：
  - `text -2436B`
  - `data/bss` 不变

### 性能证据

- 基础主路径维持在噪声范围：
  - `RECTANGLE 0.179 -> 0.179 (+0.0%)`
  - `CIRCLE 0.810 -> 0.810 (+0.0%)`
  - `ROUND_RECTANGLE 0.812 -> 0.812 (+0.0%)`
  - `TEXT 1.027 -> 1.027 (+0.0%)`
  - `IMAGE_565 0.239 -> 0.239 (+0.0%)`
  - `EXTERN_IMAGE_565 0.417 -> 0.417 (+0.0%)`
  - `IMAGE_RESIZE_565 0.459 -> 0.459 (+0.0%)`
  - `EXTERN_IMAGE_RESIZE_565 0.507 -> 0.507 (+0.0%)`
- raw packed resize 对照组维持在噪声范围：
  - `IMAGE_RESIZE_565_1 0.611 -> 0.619 (+1.3%)`
  - `IMAGE_RESIZE_565_2 0.775 -> 0.774 (-0.1%)`
  - `IMAGE_RESIZE_565_4 0.788 -> 0.788 (+0.0%)`
  - `EXTERN_IMAGE_RESIZE_565_1 0.678 -> 0.678 (+0.0%)`
  - `EXTERN_IMAGE_RESIZE_565_2 0.840 -> 0.840 (+0.0%)`
  - `EXTERN_IMAGE_RESIZE_565_4 0.854 -> 0.853 (-0.1%)`
- 父宏级 sampled masked-image resize 回退已收敛回噪声范围：
  - `MASK_IMAGE_CIRCLE 1.431 -> 1.429 (-0.1%)`
  - `EXTERN_MASK_IMAGE_CIRCLE 1.494 -> 1.490 (-0.3%)`
  - `MASK_IMAGE_TEST_PERF_CIRCLE 0.904 -> 0.904 (+0.0%)`
  - `EXTERN_MASK_IMAGE_TEST_PERF_CIRCLE 1.286 -> 1.286 (+0.0%)`
  - `EXTERN_MASK_IMAGE_TEST_PERF_ROUND_RECT 0.684 -> 0.683 (-0.1%)`
- 当前 `239` 场景 full perf run 无 `>=10%` 回退
  - 最大绝对波动：`IMAGE_RESIZE_565_1 / IMAGE_RESIZE_STAR_565_1 +1.3%`

### 运行与渲染验证

- `HelloPerformance` baseline / off 都是 `241 frames captured`
- `frame_0000.png` / `frame_0120.png` / `frame_0240.png` hash 一致
- 全量 `241` 帧 `hash mismatch 0`
- 全量 `241` 帧 `pixel mismatch 0`

### 单元测试

- `HelloUnitTest` baseline：`688/688 passed`
- `HelloUnitTest` off：`688/688 passed`

### 结论

- `EGUI_CONFIG_IMAGE_STD_MASK_ROW_PARTIAL_RESIZE_PACKED_ALPHA_FAST_PATH_ENABLE` 当前满足性能隔离目标：
  - 能再独立回收 `2436B`
  - raw sampled resize 对照组与当前 sampled masked-image circle / test-perf 主路径都继续保住
  - runtime baseline / off 严格等价
- 它比父宏 `ROW_PARTIAL_RESIZE` 更保守：
  - 只牺牲 packed `RGB565_4/2/1` sampled resize partial-row
  - `RGB565_8` alpha8 sampled resize partial-row 继续保留
- 但当前还不适合默认关：
  - 现有 perf 集对 raw packed-alpha masked sampled resize partial-row 热点仍缺专项覆盖
- 当前建议：
  - 保留为条件实验宏记录
  - 如果项目要优先保住 alpha8 / raw sampled partial-row resize，应先试 `ROW_PARTIAL_RESIZE_PACKED_ALPHA`
  - 若更想保住 packed sampled resize，且能接受 alpha8 circle 的 `4` 帧细微差异，再试 `ROW_PARTIAL_RESIZE_ALPHA8`
  - 然后再考虑父宏 `EGUI_CONFIG_IMAGE_STD_MASK_ROW_PARTIAL_RESIZE_FAST_PATH_ENABLE`
  - 最后才考虑父宏 `EGUI_CONFIG_IMAGE_STD_MASK_ROW_PARTIAL_FAST_PATH_ENABLE`

## 本轮继续细拆 A/B: `std image mask row-partial resize rgb565 fast path`

### 测试环境

- 日期：`2026-04-04`
- 提交基线：`6b67f8a`
- `HelloPerformance` code size A/B
  - `make -B -j APP=HelloPerformance PORT=qemu CPU_ARCH=cortex-m0plus APP_OBJ_SUFFIX=hpmaskpartialresizergb565on USER_CFLAGS=-DEGUI_CONFIG_IMAGE_STD_MASK_ROW_PARTIAL_RESIZE_RGB565_FAST_PATH_ENABLE=1`
  - `make -B -j APP=HelloPerformance PORT=qemu CPU_ARCH=cortex-m0plus APP_OBJ_SUFFIX=hpmaskpartialresizergb565off USER_CFLAGS=-DEGUI_CONFIG_IMAGE_STD_MASK_ROW_PARTIAL_RESIZE_RGB565_FAST_PATH_ENABLE=0`
- `HelloPerformance` perf A/B
  - `python scripts/perf_analysis/code_perf_check.py --profile cortex-m3 --clean --threshold 1000 --timeout 600 --extra-cflags=-DEGUI_CONFIG_IMAGE_STD_MASK_ROW_PARTIAL_RESIZE_RGB565_FAST_PATH_ENABLE=1`
  - `python scripts/perf_analysis/code_perf_check.py --profile cortex-m3 --clean --threshold 1000 --timeout 600 --extra-cflags=-DEGUI_CONFIG_IMAGE_STD_MASK_ROW_PARTIAL_RESIZE_RGB565_FAST_PATH_ENABLE=0`
- 运行/单测验证
  - `HelloPerformance` baseline / off 通过 `scripts.code_runtime_check.compile_app()` + `run_app(timeout=35)` 注入 `-DEGUI_CONFIG_IMAGE_STD_MASK_ROW_PARTIAL_RESIZE_RGB565_FAST_PATH_ENABLE=1/0`
  - `make -j APP=HelloUnitTest PORT=pc_test APP_OBJ_SUFFIX=utmaskrowpartialresizergb565on USER_CFLAGS=-DEGUI_CONFIG_IMAGE_STD_MASK_ROW_PARTIAL_RESIZE_RGB565_FAST_PATH_ENABLE=1`
  - `make -j APP=HelloUnitTest PORT=pc_test APP_OBJ_SUFFIX=utmaskrowpartialresizergb565off USER_CFLAGS=-DEGUI_CONFIG_IMAGE_STD_MASK_ROW_PARTIAL_RESIZE_RGB565_FAST_PATH_ENABLE=0`
  - `output/main.exe`

### 代码量证据

- `HelloPerformance` 链接汇总：
  - 默认开启：`text=2056264 data=72 bss=3832`
  - 关闭 row-partial resize rgb565 child：`text=2053328 data=72 bss=3832`
- 结论：
  - `text -2936B`
  - `data/bss` 不变

### 性能证据

- 基础主路径维持在噪声范围：
  - `RECTANGLE 0.179 -> 0.179 (+0.0%)`
  - `CIRCLE 0.810 -> 0.810 (+0.0%)`
  - `ROUND_RECTANGLE 0.812 -> 0.812 (+0.0%)`
  - `TEXT 1.027 -> 1.027 (+0.0%)`
  - `IMAGE_565 0.239 -> 0.239 (+0.0%)`
  - `EXTERN_IMAGE_565 0.417 -> 0.417 (+0.0%)`
  - `IMAGE_RESIZE_565 0.459 -> 0.459 (+0.0%)`
  - `EXTERN_IMAGE_RESIZE_565 0.507 -> 0.507 (+0.0%)`
- raw / packed sampled resize 对照组维持在噪声范围：
  - `IMAGE_RESIZE_565_1 0.611 -> 0.611 (+0.0%)`
  - `IMAGE_RESIZE_565_2 0.775 -> 0.774 (-0.1%)`
  - `IMAGE_RESIZE_565_4 0.788 -> 0.788 (+0.0%)`
  - `EXTERN_IMAGE_RESIZE_565_1 0.678 -> 0.678 (+0.0%)`
  - `EXTERN_IMAGE_RESIZE_565_2 0.840 -> 0.840 (+0.0%)`
  - `EXTERN_IMAGE_RESIZE_565_4 0.854 -> 0.853 (-0.1%)`
- `MASK_IMAGE_CIRCLE / EXTERN_MASK_IMAGE_CIRCLE` 也维持在噪声范围：
  - `MASK_IMAGE_CIRCLE 1.431 -> 1.429 (-0.1%)`
  - `EXTERN_MASK_IMAGE_CIRCLE 1.494 -> 1.490 (-0.3%)`
- 当前 `239` 场景里明显回退的热点集中在 raw sampled masked-image `TEST_PERF_*`：
  - `MASK_IMAGE_TEST_PERF_CIRCLE 0.904 -> 3.905 (+332.0%)`
  - `EXTERN_MASK_IMAGE_TEST_PERF_CIRCLE 1.286 -> 3.938 (+206.2%)`
  - `EXTERN_MASK_IMAGE_TEST_PERF_ROUND_RECT 0.684 -> 1.495 (+118.6%)`
  - `EXTERN_MASK_IMAGE_TEST_PERF_IMAGE 1.092 -> 2.072 (+89.7%)`

### 运行与渲染验证

- `HelloPerformance` baseline / off 都是 `241 frames captured`
- `frame_0000.png` / `frame_0120.png` / `frame_0240.png` hash 一致
- 全量 `241` 帧 `hash mismatch 1`
- 全量 `241` 帧 `pixel mismatch 1`
- 当前差异帧是 `frame_0190.png`

### 单元测试

- `HelloUnitTest` baseline：`688/688 passed`
- `HelloUnitTest` off：`688/688 passed`

### 结论

- `EGUI_CONFIG_IMAGE_STD_MASK_ROW_PARTIAL_RESIZE_RGB565_FAST_PATH_ENABLE` 能再独立回收 `2936B`
- 但它没有像 `ROW_PARTIAL_RESIZE_PACKED_ALPHA` 那样把 perf 代价收敛到噪声范围：
  - 当前仍会明显打到 raw sampled masked-image `TEST_PERF_*` 热点
  - 因此不进入当前优先保留序列
- 它与 baseline 也不是严格渲染等价：
  - runtime baseline / off 当前有 `1` 帧真实像素差异
- 因此：
  - 只保留这颗 child 的实验记录
  - 如果项目明确不依赖 raw sampled masked-image `TEST_PERF_*` 热点，且可以接受 `1` 帧像素差，可再单独试它
  - 默认不建议优先于 `EGUI_CONFIG_IMAGE_STD_MASK_ROW_PARTIAL_RESIZE_PACKED_ALPHA_FAST_PATH_ENABLE`

## 本轮继续细拆 A/B: `std image mask row-partial resize generic fast path`

### 测试环境

- 日期：`2026-04-04`
- 提交基线：`82c19fc`
- `HelloPerformance` code size A/B
  - `make -B -j APP=HelloPerformance PORT=qemu CPU_ARCH=cortex-m0plus APP_OBJ_SUFFIX=hpmaskpartialresizegenericon USER_CFLAGS=-DEGUI_CONFIG_IMAGE_STD_MASK_ROW_PARTIAL_RESIZE_GENERIC_FAST_PATH_ENABLE=1`
  - `make -B -j APP=HelloPerformance PORT=qemu CPU_ARCH=cortex-m0plus APP_OBJ_SUFFIX=hpmaskpartialresizegenericoff USER_CFLAGS=-DEGUI_CONFIG_IMAGE_STD_MASK_ROW_PARTIAL_RESIZE_GENERIC_FAST_PATH_ENABLE=0`
- `HelloPerformance` perf A/B
  - `python scripts/perf_analysis/code_perf_check.py --profile cortex-m3 --clean --threshold 1000 --timeout 600 --extra-cflags=-DEGUI_CONFIG_IMAGE_STD_MASK_ROW_PARTIAL_RESIZE_GENERIC_FAST_PATH_ENABLE=1`
  - `python scripts/perf_analysis/code_perf_check.py --profile cortex-m3 --clean --threshold 1000 --timeout 600 --extra-cflags=-DEGUI_CONFIG_IMAGE_STD_MASK_ROW_PARTIAL_RESIZE_GENERIC_FAST_PATH_ENABLE=0`
- 运行/单测验证
  - `HelloPerformance` baseline / off 通过 `scripts.code_runtime_check.compile_app()` + `run_app(timeout=35)` 注入 `-DEGUI_CONFIG_IMAGE_STD_MASK_ROW_PARTIAL_RESIZE_GENERIC_FAST_PATH_ENABLE=1/0`
  - `make -j APP=HelloUnitTest PORT=pc_test APP_OBJ_SUFFIX=utmaskrowpartialresizegenericon USER_CFLAGS=-DEGUI_CONFIG_IMAGE_STD_MASK_ROW_PARTIAL_RESIZE_GENERIC_FAST_PATH_ENABLE=1`
  - `make -j APP=HelloUnitTest PORT=pc_test APP_OBJ_SUFFIX=utmaskrowpartialresizegenericoff USER_CFLAGS=-DEGUI_CONFIG_IMAGE_STD_MASK_ROW_PARTIAL_RESIZE_GENERIC_FAST_PATH_ENABLE=0`
  - `output/main.exe`

### 代码量证据

- `HelloPerformance` 链接汇总：
  - 默认开启：`text=2056264 data=72 bss=3832`
  - 关闭 row-partial resize generic child：`text=2056264 data=72 bss=3832`
- 结论：
  - `text 0B`
  - `data/bss` 不变

### 性能证据

- `HelloPerformance` 全量 `239` 场景 on / off 结果一致
- 基础主路径、sampled masked-image 热点和各类 resize 对照组都没有任何变化

### 运行与渲染验证

- `HelloPerformance` baseline / off 都是 `241 frames captured`
- `frame_0000.png` / `frame_0120.png` / `frame_0240.png` hash 一致
- 全量 `241` 帧 `hash mismatch 0`
- 全量 `241` 帧 `pixel mismatch 0`

### 单元测试

- `HelloUnitTest` baseline：`688/688 passed`
- `HelloUnitTest` off：`688/688 passed`

### 结论

- `EGUI_CONFIG_IMAGE_STD_MASK_ROW_PARTIAL_RESIZE_GENERIC_FAST_PATH_ENABLE` 被当前链接结果证伪：
  - `HelloPerformance text 0B`
  - perf / runtime / unit 全量一致
- 因此不保留这颗 generic child 宏：
  - 源码里已回退对应配置与条件分支改动
  - 只保留实验记录

## 本轮继续细拆 A/B: `std image mask row-inside fast path`

### 测试环境

- 日期：`2026-04-03`
- 提交基线：`dabeeef`
- `HelloPerformance` code size A/B
  - `make clean APP=HelloPerformance PORT=qemu`
  - `make all APP=HelloPerformance PORT=qemu`
  - `make clean APP=HelloPerformance PORT=qemu`
  - `make all APP=HelloPerformance PORT=qemu USER_CFLAGS="-DEGUI_CONFIG_IMAGE_STD_MASK_ROW_INSIDE_FAST_PATH_ENABLE=0"`
- `HelloPerformance` perf A/B
  - `python scripts/perf_analysis/code_perf_check.py --profile cortex-m3 --clean --threshold 1000 --filter RECTANGLE,CIRCLE,ROUND_RECTANGLE,TEXT,IMAGE_565,EXTERN_IMAGE_565,IMAGE_RESIZE_565,EXTERN_IMAGE_RESIZE_565,MASK_IMAGE_TEST_PERF_CIRCLE,EXTERN_MASK_IMAGE_TEST_PERF_CIRCLE,MASK_IMAGE_TEST_PERF_ROUND_RECT,EXTERN_MASK_IMAGE_TEST_PERF_ROUND_RECT`
  - `python scripts/perf_analysis/code_perf_check.py --profile cortex-m3 --clean --threshold 1000 --filter RECTANGLE,CIRCLE,ROUND_RECTANGLE,TEXT,IMAGE_565,EXTERN_IMAGE_565,IMAGE_RESIZE_565,EXTERN_IMAGE_RESIZE_565,MASK_IMAGE_TEST_PERF_CIRCLE,EXTERN_MASK_IMAGE_TEST_PERF_CIRCLE,MASK_IMAGE_TEST_PERF_ROUND_RECT,EXTERN_MASK_IMAGE_TEST_PERF_ROUND_RECT --extra-cflags=-DEGUI_CONFIG_IMAGE_STD_MASK_ROW_INSIDE_FAST_PATH_ENABLE=0`
- 运行/单测验证
  - `python scripts/code_runtime_check.py --app HelloPerformance --timeout 10 --keep-screenshots`
  - `mask row inside off` 通过 `scripts/code_runtime_check.py` 的脚本接口注入 `USER_CFLAGS=-DEGUI_CONFIG_IMAGE_STD_MASK_ROW_INSIDE_FAST_PATH_ENABLE=0`
  - `make -j APP=HelloUnitTest PORT=pc_test`
  - `make -j APP=HelloUnitTest PORT=pc_test USER_CFLAGS="-DEGUI_CONFIG_IMAGE_STD_MASK_ROW_INSIDE_FAST_PATH_ENABLE=0"`
  - `output/main.exe`

### 代码量证据

- `HelloPerformance` 链接汇总：
  - 默认开启：`text=2036712 data=72 bss=3840`
  - 关闭 row-inside fast path：`text=2035372 data=72 bss=3840`
- 结论：
  - `text -1340B`
  - `data/bss` 不变

### 性能证据

| 场景 | 默认开启 | 关闭 row-inside fast path | 变化 |
| --- | ---: | ---: | ---: |
| `RECTANGLE` | `0.179ms` | `0.179ms` | `+0.0%` |
| `CIRCLE` | `0.810ms` | `0.810ms` | `+0.0%` |
| `ROUND_RECTANGLE` | `0.812ms` | `0.812ms` | `+0.0%` |
| `TEXT` | `1.027ms` | `1.027ms` | `+0.0%` |
| `IMAGE_565` | `0.239ms` | `0.239ms` | `+0.0%` |
| `EXTERN_IMAGE_565` | `0.417ms` | `0.417ms` | `+0.0%` |
| `IMAGE_RESIZE_565` | `0.459ms` | `0.459ms` | `+0.0%` |
| `EXTERN_IMAGE_RESIZE_565` | `0.507ms` | `0.507ms` | `+0.0%` |
| `MASK_IMAGE_TEST_PERF_CIRCLE` | `0.904ms` | `4.512ms` | `+399.1%` |
| `EXTERN_MASK_IMAGE_TEST_PERF_CIRCLE` | `1.286ms` | `4.566ms` | `+255.1%` |
| `MASK_IMAGE_TEST_PERF_ROUND_RECT` | `0.586ms` | `0.586ms` | `+0.0%` |
| `EXTERN_MASK_IMAGE_TEST_PERF_ROUND_RECT` | `0.684ms` | `6.249ms` | `+813.6%` |

- 这说明 `EGUI_MASK_ROW_INSIDE` 对 sampled masked-image 热点仍是核心路径，尤其 external round-rect。

### 运行/单测

- `HelloPerformance` runtime
  - 默认开启：`241 frames`，`ALL PASSED`
  - 宏关闭：`241 frames`，`ALL PASSED`
- `HelloUnitTest`
  - 默认开启：`688/688 passed`
  - 宏关闭：`688/688 passed`

### 结论

- 这颗宏满足：
  - 代码量收益略高于 `1KB`
  - 基础 `RECTANGLE / CIRCLE / ROUND_RECTANGLE / TEXT / IMAGE_565 / EXTERN_IMAGE_565 / IMAGE_RESIZE_565 / EXTERN_IMAGE_RESIZE_565` 路径不退化
- 但它的 perf 代价比 `ROW_PARTIAL` 更尖锐，而 size 收益只有 `1340B`。
- 因此：
  - 保留 `EGUI_CONFIG_IMAGE_STD_MASK_ROW_INSIDE_FAST_PATH_ENABLE`
  - 默认值继续保持开启
  - 作为“项目不用 masked-image inside-row 热点”时的条件实验宏是成立的
  - 优先级低于 `EGUI_CONFIG_IMAGE_STD_MASK_ROW_PARTIAL_FAST_PATH_ENABLE`

## 本轮继续细拆 A/B: `std image mask row-inside resize fast path`

### 测试环境

- 日期：`2026-04-04`
- 提交基线：`dd4900c`
- `HelloPerformance` code size A/B
  - `make clean APP=HelloPerformance PORT=qemu CPU_ARCH=cortex-m0plus APP_OBJ_SUFFIX=hpmaskinsideresizeon`
  - `make -j APP=HelloPerformance PORT=qemu CPU_ARCH=cortex-m0plus APP_OBJ_SUFFIX=hpmaskinsideresizeon`
  - `make clean APP=HelloPerformance PORT=qemu CPU_ARCH=cortex-m0plus APP_OBJ_SUFFIX=hpmaskinsideresizeoff`
  - `make -j APP=HelloPerformance PORT=qemu CPU_ARCH=cortex-m0plus APP_OBJ_SUFFIX=hpmaskinsideresizeoff USER_CFLAGS=-DEGUI_CONFIG_IMAGE_STD_MASK_ROW_INSIDE_RESIZE_FAST_PATH_ENABLE=0`
- `HelloPerformance` perf A/B
  - `python scripts/perf_analysis/code_perf_check.py --profile cortex-m3 --clean --threshold 1000 --timeout 900 --filter RECTANGLE,CIRCLE,ROUND_RECTANGLE,TEXT,IMAGE_565,EXTERN_IMAGE_565,IMAGE_RESIZE_565,EXTERN_IMAGE_RESIZE_565,MASK_IMAGE_NO_MASK,MASK_IMAGE_ROUND_RECT,MASK_IMAGE_CIRCLE,MASK_IMAGE_IMAGE,MASK_IMAGE_CIRCLE_QUARTER,MASK_IMAGE_CIRCLE_DOUBLE,MASK_IMAGE_ROUND_RECT_QUARTER,MASK_IMAGE_ROUND_RECT_DOUBLE,EXTERN_MASK_IMAGE_NO_MASK,EXTERN_MASK_IMAGE_ROUND_RECT,EXTERN_MASK_IMAGE_CIRCLE,EXTERN_MASK_IMAGE_IMAGE,MASK_IMAGE_TEST_PERF_NO_MASK,MASK_IMAGE_TEST_PERF_ROUND_RECT,MASK_IMAGE_TEST_PERF_CIRCLE,MASK_IMAGE_TEST_PERF_IMAGE,EXTERN_MASK_IMAGE_TEST_PERF_NO_MASK,EXTERN_MASK_IMAGE_TEST_PERF_ROUND_RECT,EXTERN_MASK_IMAGE_TEST_PERF_CIRCLE,EXTERN_MASK_IMAGE_TEST_PERF_IMAGE`
  - `python scripts/perf_analysis/code_perf_check.py --profile cortex-m3 --clean --threshold 1000 --timeout 900 --filter RECTANGLE,CIRCLE,ROUND_RECTANGLE,TEXT,IMAGE_565,EXTERN_IMAGE_565,IMAGE_RESIZE_565,EXTERN_IMAGE_RESIZE_565,MASK_IMAGE_NO_MASK,MASK_IMAGE_ROUND_RECT,MASK_IMAGE_CIRCLE,MASK_IMAGE_IMAGE,MASK_IMAGE_CIRCLE_QUARTER,MASK_IMAGE_CIRCLE_DOUBLE,MASK_IMAGE_ROUND_RECT_QUARTER,MASK_IMAGE_ROUND_RECT_DOUBLE,EXTERN_MASK_IMAGE_NO_MASK,EXTERN_MASK_IMAGE_ROUND_RECT,EXTERN_MASK_IMAGE_CIRCLE,EXTERN_MASK_IMAGE_IMAGE,MASK_IMAGE_TEST_PERF_NO_MASK,MASK_IMAGE_TEST_PERF_ROUND_RECT,MASK_IMAGE_TEST_PERF_CIRCLE,MASK_IMAGE_TEST_PERF_IMAGE,EXTERN_MASK_IMAGE_TEST_PERF_NO_MASK,EXTERN_MASK_IMAGE_TEST_PERF_ROUND_RECT,EXTERN_MASK_IMAGE_TEST_PERF_CIRCLE,EXTERN_MASK_IMAGE_TEST_PERF_IMAGE --extra-cflags=-DEGUI_CONFIG_IMAGE_STD_MASK_ROW_INSIDE_RESIZE_FAST_PATH_ENABLE=0`
- 运行/单测验证
  - `HelloPerformance` baseline / off 通过 `scripts.code_runtime_check.compile_app()` + `run_app()` 注入默认配置和 `-DEGUI_CONFIG_IMAGE_STD_MASK_ROW_INSIDE_RESIZE_FAST_PATH_ENABLE=0`
  - `make -j APP=HelloUnitTest PORT=pc_test APP_OBJ_SUFFIX=utmaskinsideresizeon`
  - `make -j APP=HelloUnitTest PORT=pc_test APP_OBJ_SUFFIX=utmaskinsideresizeoff USER_CFLAGS=-DEGUI_CONFIG_IMAGE_STD_MASK_ROW_INSIDE_RESIZE_FAST_PATH_ENABLE=0`
  - `output/main.exe`

### 代码量证据

- `HelloPerformance` 链接汇总：
  - 默认开启：`text=2056256 data=72 bss=3832`
  - 关闭 row-inside resize fast path：`text=2055328 data=72 bss=3832`
- 结论：
  - `text -928B`
  - `data/bss` 不变

### 性能证据

| 场景 | 默认开启 | 关闭 row-inside resize fast path | 变化 |
| --- | ---: | ---: | ---: |
| `RECTANGLE` | `0.179ms` | `0.179ms` | `+0.0%` |
| `CIRCLE` | `0.810ms` | `0.810ms` | `+0.0%` |
| `ROUND_RECTANGLE` | `0.812ms` | `0.812ms` | `+0.0%` |
| `TEXT` | `1.027ms` | `1.027ms` | `+0.0%` |
| `IMAGE_565` | `0.239ms` | `0.239ms` | `+0.0%` |
| `EXTERN_IMAGE_565` | `0.417ms` | `0.417ms` | `+0.0%` |
| `IMAGE_RESIZE_565` | `0.459ms` | `0.459ms` | `+0.0%` |
| `EXTERN_IMAGE_RESIZE_565` | `0.507ms` | `0.507ms` | `+0.0%` |
| `MASK_IMAGE_NO_MASK` | `0.634ms` | `0.634ms` | `+0.0%` |
| `MASK_IMAGE_ROUND_RECT` | `0.969ms` | `0.969ms` | `+0.0%` |
| `MASK_IMAGE_CIRCLE` | `1.431ms` | `1.430ms` | `-0.1%` |
| `MASK_IMAGE_IMAGE` | `0.296ms` | `0.297ms` | `+0.3%` |
| `MASK_IMAGE_CIRCLE_QUARTER` | `0.453ms` | `0.453ms` | `+0.0%` |
| `MASK_IMAGE_CIRCLE_DOUBLE` | `1.316ms` | `1.316ms` | `+0.0%` |
| `MASK_IMAGE_ROUND_RECT_QUARTER` | `0.326ms` | `0.327ms` | `+0.3%` |
| `MASK_IMAGE_ROUND_RECT_DOUBLE` | `0.943ms` | `0.943ms` | `+0.0%` |
| `EXTERN_MASK_IMAGE_NO_MASK` | `0.697ms` | `0.697ms` | `+0.0%` |
| `EXTERN_MASK_IMAGE_ROUND_RECT` | `1.035ms` | `1.035ms` | `+0.0%` |
| `EXTERN_MASK_IMAGE_CIRCLE` | `1.494ms` | `1.494ms` | `+0.0%` |
| `EXTERN_MASK_IMAGE_IMAGE` | `0.331ms` | `0.331ms` | `+0.0%` |
| `MASK_IMAGE_TEST_PERF_NO_MASK` | `0.459ms` | `0.459ms` | `+0.0%` |
| `MASK_IMAGE_TEST_PERF_ROUND_RECT` | `0.586ms` | `0.586ms` | `+0.0%` |
| `MASK_IMAGE_TEST_PERF_CIRCLE` | `0.904ms` | `4.512ms` | `+399.1%` |
| `MASK_IMAGE_TEST_PERF_IMAGE` | `1.661ms` | `1.662ms` | `+0.1%` |
| `EXTERN_MASK_IMAGE_TEST_PERF_NO_MASK` | `0.507ms` | `0.507ms` | `+0.0%` |
| `EXTERN_MASK_IMAGE_TEST_PERF_ROUND_RECT` | `0.684ms` | `6.249ms` | `+813.6%` |
| `EXTERN_MASK_IMAGE_TEST_PERF_CIRCLE` | `1.286ms` | `4.562ms` | `+254.7%` |
| `EXTERN_MASK_IMAGE_TEST_PERF_IMAGE` | `1.092ms` | `1.250ms` | `+14.5%` |

- 这说明代价已经进一步收敛到 inside-row masked-image resize hotspot。
- 当前抽样到的 non-test masked-image resize 场景都维持在噪声范围内。

### 运行/单测

- `HelloPerformance` runtime
  - baseline / off 通过 `scripts.code_runtime_check.compile_app()` + `run_app()` 录制
  - baseline：`166 frames captured`
  - 宏关闭：`166 frames captured`
  - `frame_0000.png` / `frame_0082.png` / `frame_0165.png` SHA256 一致
  - 全量 `166` 帧 PNG 文件 hash mismatch `0`，像素 diff `0`
- `HelloUnitTest`
  - baseline：`688/688 passed`
  - 宏关闭：`688/688 passed`

### 结论

- 这颗 child 满足：
  - 代码量有稳定收益：`text -928B`
  - 基础 `RECTANGLE / CIRCLE / ROUND_RECTANGLE / TEXT / IMAGE_565 / EXTERN_IMAGE_565 / IMAGE_RESIZE_565 / EXTERN_IMAGE_RESIZE_565` 路径不退化
  - sampled non-test masked-image resize 场景也维持在噪声范围
- 代价仍集中在 inside-row sampled resize hotspot，尤其 `MASK_IMAGE_TEST_PERF_CIRCLE`、`EXTERN_MASK_IMAGE_TEST_PERF_CIRCLE`、`EXTERN_MASK_IMAGE_TEST_PERF_ROUND_RECT`
- 因此：
  - 保留 `EGUI_CONFIG_IMAGE_STD_MASK_ROW_INSIDE_RESIZE_FAST_PATH_ENABLE`
  - 默认值继续保持开启
  - 如果项目想保住 masked-image draw 和绝大多数 resize 样本，但可以牺牲 inside-row sampled resize 热点，应先试这颗 child，再考虑父宏 `EGUI_CONFIG_IMAGE_STD_MASK_ROW_INSIDE_FAST_PATH_ENABLE`

## 本轮继续细拆 A/B: `std image mask row-inside resize alpha8 fast path`

### 测试环境

- 日期：`2026-04-04`
- 提交基线：`8221b16`
- `HelloPerformance` code size A/B
  - `make -B -j APP=HelloPerformance PORT=qemu CPU_ARCH=cortex-m0plus APP_OBJ_SUFFIX=hpmaskinsideresizea8on USER_CFLAGS=-DEGUI_CONFIG_IMAGE_STD_MASK_ROW_INSIDE_RESIZE_ALPHA8_FAST_PATH_ENABLE=1`
  - `make -B -j APP=HelloPerformance PORT=qemu CPU_ARCH=cortex-m0plus APP_OBJ_SUFFIX=hpmaskinsideresizea8off USER_CFLAGS=-DEGUI_CONFIG_IMAGE_STD_MASK_ROW_INSIDE_RESIZE_ALPHA8_FAST_PATH_ENABLE=0`
- `HelloPerformance` perf A/B
  - `python scripts/perf_analysis/code_perf_check.py --profile cortex-m3 --clean --threshold 1000 --timeout 600 --filter RECTANGLE,CIRCLE,ROUND_RECTANGLE,TEXT,IMAGE_565,EXTERN_IMAGE_565,IMAGE_RESIZE_565,EXTERN_IMAGE_RESIZE_565,MASK_IMAGE_NO_MASK,MASK_IMAGE_ROUND_RECT,MASK_IMAGE_CIRCLE,MASK_IMAGE_IMAGE,MASK_IMAGE_CIRCLE_QUARTER,MASK_IMAGE_CIRCLE_DOUBLE,MASK_IMAGE_ROUND_RECT_QUARTER,MASK_IMAGE_ROUND_RECT_DOUBLE,EXTERN_MASK_IMAGE_NO_MASK,EXTERN_MASK_IMAGE_ROUND_RECT,EXTERN_MASK_IMAGE_CIRCLE,EXTERN_MASK_IMAGE_IMAGE,MASK_IMAGE_TEST_PERF_NO_MASK,MASK_IMAGE_TEST_PERF_ROUND_RECT,MASK_IMAGE_TEST_PERF_CIRCLE,MASK_IMAGE_TEST_PERF_IMAGE,EXTERN_MASK_IMAGE_TEST_PERF_NO_MASK,EXTERN_MASK_IMAGE_TEST_PERF_ROUND_RECT,EXTERN_MASK_IMAGE_TEST_PERF_CIRCLE,EXTERN_MASK_IMAGE_TEST_PERF_IMAGE --extra-cflags=-DEGUI_CONFIG_IMAGE_STD_MASK_ROW_INSIDE_RESIZE_ALPHA8_FAST_PATH_ENABLE=1`
  - `python scripts/perf_analysis/code_perf_check.py --profile cortex-m3 --clean --threshold 1000 --timeout 600 --filter RECTANGLE,CIRCLE,ROUND_RECTANGLE,TEXT,IMAGE_565,EXTERN_IMAGE_565,IMAGE_RESIZE_565,EXTERN_IMAGE_RESIZE_565,MASK_IMAGE_NO_MASK,MASK_IMAGE_ROUND_RECT,MASK_IMAGE_CIRCLE,MASK_IMAGE_IMAGE,MASK_IMAGE_CIRCLE_QUARTER,MASK_IMAGE_CIRCLE_DOUBLE,MASK_IMAGE_ROUND_RECT_QUARTER,MASK_IMAGE_ROUND_RECT_DOUBLE,EXTERN_MASK_IMAGE_NO_MASK,EXTERN_MASK_IMAGE_ROUND_RECT,EXTERN_MASK_IMAGE_CIRCLE,EXTERN_MASK_IMAGE_IMAGE,MASK_IMAGE_TEST_PERF_NO_MASK,MASK_IMAGE_TEST_PERF_ROUND_RECT,MASK_IMAGE_TEST_PERF_CIRCLE,MASK_IMAGE_TEST_PERF_IMAGE,EXTERN_MASK_IMAGE_TEST_PERF_NO_MASK,EXTERN_MASK_IMAGE_TEST_PERF_ROUND_RECT,EXTERN_MASK_IMAGE_TEST_PERF_CIRCLE,EXTERN_MASK_IMAGE_TEST_PERF_IMAGE --extra-cflags=-DEGUI_CONFIG_IMAGE_STD_MASK_ROW_INSIDE_RESIZE_ALPHA8_FAST_PATH_ENABLE=0`
- 运行/单测验证
  - `HelloPerformance` baseline / off 通过 `scripts.code_runtime_check.compile_app()` + `run_app()` 注入 `-DEGUI_CONFIG_IMAGE_STD_MASK_ROW_INSIDE_RESIZE_ALPHA8_FAST_PATH_ENABLE=1/0`
  - `make -j APP=HelloUnitTest PORT=pc_test APP_OBJ_SUFFIX=utmaskinsideresizea8on USER_CFLAGS=-DEGUI_CONFIG_IMAGE_STD_MASK_ROW_INSIDE_RESIZE_ALPHA8_FAST_PATH_ENABLE=1`
  - `make -j APP=HelloUnitTest PORT=pc_test APP_OBJ_SUFFIX=utmaskinsideresizea8off USER_CFLAGS=-DEGUI_CONFIG_IMAGE_STD_MASK_ROW_INSIDE_RESIZE_ALPHA8_FAST_PATH_ENABLE=0`
  - `output/main.exe`

### 代码量证据

- `HelloPerformance` 链接汇总：
  - 默认开启：`text=2056264 data=72 bss=3832`
  - 关闭 row-inside resize alpha8 child：`text=2055552 data=72 bss=3832`
- 结论：
  - `text -712B`
  - `data/bss` 不变

### 性能证据

| 场景 | 默认开启 | 关闭 row-inside resize alpha8 child | 变化 |
| --- | ---: | ---: | ---: |
| `RECTANGLE` | `0.179ms` | `0.179ms` | `+0.0%` |
| `CIRCLE` | `0.810ms` | `0.810ms` | `+0.0%` |
| `ROUND_RECTANGLE` | `0.812ms` | `0.812ms` | `+0.0%` |
| `TEXT` | `1.027ms` | `1.027ms` | `+0.0%` |
| `IMAGE_565` | `0.239ms` | `0.239ms` | `+0.0%` |
| `EXTERN_IMAGE_565` | `0.417ms` | `0.417ms` | `+0.0%` |
| `IMAGE_RESIZE_565` | `0.459ms` | `0.459ms` | `+0.0%` |
| `EXTERN_IMAGE_RESIZE_565` | `0.507ms` | `0.507ms` | `+0.0%` |
| `MASK_IMAGE_ROUND_RECT` | `0.969ms` | `0.965ms` | `-0.4%` |
| `MASK_IMAGE_CIRCLE` | `1.431ms` | `4.412ms` | `+208.3%` |
| `EXTERN_MASK_IMAGE_ROUND_RECT` | `1.035ms` | `1.031ms` | `-0.4%` |
| `EXTERN_MASK_IMAGE_CIRCLE` | `1.494ms` | `4.475ms` | `+199.5%` |
| `MASK_IMAGE_CIRCLE_QUARTER` | `0.453ms` | `1.166ms` | `+157.4%` |
| `MASK_IMAGE_CIRCLE_DOUBLE` | `1.316ms` | `5.087ms` | `+286.6%` |
| `MASK_IMAGE_TEST_PERF_CIRCLE` | `0.904ms` | `0.904ms` | `+0.0%` |
| `EXTERN_MASK_IMAGE_TEST_PERF_CIRCLE` | `1.286ms` | `1.286ms` | `+0.0%` |
| `EXTERN_MASK_IMAGE_TEST_PERF_ROUND_RECT` | `0.684ms` | `0.684ms` | `+0.0%` |
| `EXTERN_MASK_IMAGE_TEST_PERF_IMAGE` | `1.092ms` | `1.092ms` | `+0.0%` |

- 这说明新 child 只把代价继续收敛到 `RGB565_8` alpha8 inside-row circle masked-image resize 热点：
  - 基础 `RECTANGLE / CIRCLE / ROUND_RECTANGLE / TEXT / IMAGE_565 / EXTERN_IMAGE_565 / IMAGE_RESIZE_565 / EXTERN_IMAGE_RESIZE_565` 不退化
  - raw sampled inside-row resize 对照组不退化：`MASK_IMAGE_TEST_PERF_CIRCLE / EXTERN_MASK_IMAGE_TEST_PERF_CIRCLE / EXTERN_MASK_IMAGE_TEST_PERF_ROUND_RECT / EXTERN_MASK_IMAGE_TEST_PERF_IMAGE`
  - 代价集中在 alpha8 circle resize 样本：`MASK_IMAGE_CIRCLE`、`EXTERN_MASK_IMAGE_CIRCLE`、`MASK_IMAGE_CIRCLE_QUARTER`、`MASK_IMAGE_CIRCLE_DOUBLE`

### 运行/单测

- `HelloPerformance` runtime
  - baseline / off 都是 `241 frames`
  - `frame_0000.png`、`frame_0120.png`、`frame_0240.png` hash 一致
  - 全量 `241` 帧 hash mismatch `0`
  - 全量 `241` 帧 pixel mismatch `0`
- `HelloUnitTest`
  - baseline：`688/688 passed`
  - off：`688/688 passed`

### 结论

- `EGUI_CONFIG_IMAGE_STD_MASK_ROW_INSIDE_RESIZE_ALPHA8_FAST_PATH_ENABLE` 在性能隔离上成立：
  - 能再独立回收 `712B`
  - raw sampled inside-row resize 热点继续保住
  - runtime baseline / off 严格渲染等价
- 它比父宏 `ROW_INSIDE_RESIZE` 更保守：
  - 父宏会打到 sampled inside-row resize 对照组
  - 这颗 child 的代价只落在 alpha8 circle masked-image resize 热点
- 但当前不进入主推荐序列：
  - size 收益低于 `1KB`
- 因此：
  - 保留这颗 child 作为条件实验宏记录
  - 如果项目要尽量保住 raw inside-row resize，应先试这颗宏，再考虑父宏 `EGUI_CONFIG_IMAGE_STD_MASK_ROW_INSIDE_RESIZE_FAST_PATH_ENABLE`

## 当前 fast path 结论矩阵

| 路径 | 代码/资源证据 | HelloPerformance 证据 | 当前结论 |
| --- | --- | --- | --- |
| `rectangle basic canvas path` | 当前未发现值得长期维护的独立大 helper，也没有现成能带来 `>1KB` 收益的关闭候选 | 本轮 `std image fast draw` A/B 下 `RECTANGLE +0.0%`，说明这次能回收的代码不在 rectangle 主路径 | 暂不新增宏 |
| `circle / round-rectangle basic fill fast path` | 当前只保留 `EGUI_CONFIG_CIRCLE_FILL_BASIC` 这颗 HelloPerformance 示例局部 override，不再写入共享默认头 | 历史结论已确认这是 HelloPerformance 需要保留的加速路径；本轮 image 宏 A/B 下 `CIRCLE/ROUND_RECTANGLE` 也均为 `+0.0%` | 维持现状，继续作为 HelloPerformance 专用 override |
| `canvas masked-fill shape child fast path` | `MASK_FILL_CIRCLE text -3648B`；继续细拆 `CIRCLE_SEGMENT child -3328B`；`MASK_FILL_ROUND_RECT text -876B` | 基础路径都在噪声范围；`MASK_FILL_CIRCLE` 主要拖慢 circle masked-fill 热点；`CIRCLE_SEGMENT child` 当前完整 `239` 场景 perf / runtime / unit 全量等价；`MASK_FILL_ROUND_RECT` 主要拖慢 round-rect masked-fill 热点 | 保留 `EGUI_CONFIG_CANVAS_MASK_FILL_CIRCLE_FAST_PATH_ENABLE` 作为条件实验宏，并优先保留更保守的 `CIRCLE_SEGMENT` child；`ROUND_RECT` 当前收益低于 `1KB`，降级为低优先级 |
| `canvas masked-fill circle segment fast path` | `2026-04-05` current-mainline `HelloPerformance text -3328B`；主要是 `egui_canvas_fill_masked_row_segment()` 里的 circle partial-row segment helper | 基础 `RECTANGLE / CIRCLE / ROUND_RECTANGLE / TEXT / IMAGE_565 / EXTERN_IMAGE_565 / IMAGE_RESIZE_565 / EXTERN_IMAGE_RESIZE_565` 不退化；完整 `239` 场景没有任何 `>=10%` 回退；`MASK_RECT_FILL_CIRCLE / MASK_RECT_FILL_ROUND_RECT / MASK_RECT_FILL_IMAGE / MASK_GRADIENT_RECT_FILL / MASK_ROUND_RECT_FILL_WITH_MASK` 也都不退化；runtime `241` 帧 hash / pixel mismatch `0`；`HelloUnitTest 688/688 passed` | 保留 `EGUI_CONFIG_CANVAS_MASK_FILL_CIRCLE_SEGMENT_FAST_PATH_ENABLE`，作为比 `MASK_FILL_CIRCLE` 更保守的 child；若项目要保住当前 circle masked-fill 主路径，应优先试这颗宏 |
| `canvas masked-fill shape umbrella fast path` | 当前树最终 `HelloPerformance text +0B`；对象级 shrink 不能转化为最终 ELF 收益 | 基础 `RECTANGLE / CIRCLE / ROUND_RECTANGLE / TEXT / IMAGE_565 / EXTERN_IMAGE_565` 不退化；但 `MASK_RECT_FILL_CIRCLE +37.0%`，`MASK_RECT_FILL_ROUND_RECT` / `MASK_ROUND_RECT_FILL_WITH_MASK +13.0%` | 直接拒绝，不保留 `EGUI_CONFIG_CANVAS_MASK_SHAPE_FILL_FAST_PATH_ENABLE` |
| `canvas masked-fill auxiliary fast path` | `ROW_BLEND text -2948B`；`IMAGE text -1860B`；继续细拆 `IMAGE_SCALE child -196B`；`ROW_RANGE text -8028B`；`ROW_PARTIAL text -2928B`；`ROW_INSIDE text -1664B` | 基础路径都在噪声范围；`ROW_BLEND` 明显伤 gradient masked-fill；`IMAGE` 明显伤 image masked-fill；`IMAGE_SCALE child` 的 `239` 场景 perf / runtime / unit 全量等价；`2026-04-05` current-mainline 复验继续确认 `ROW_RANGE / ROW_PARTIAL / ROW_INSIDE` 的完整 `239` 场景都没有任何 `>=10%` 波动，基础主路径和当前 sampled canvas masked-fill 对照组都在噪声范围 | 保留 `ROW_BLEND / IMAGE / ROW_RANGE / ROW_PARTIAL / ROW_INSIDE` 父宏；不保留 `EGUI_CONFIG_CANVAS_MASK_FILL_IMAGE_SCALE_FAST_PATH_ENABLE`；当前优先级 `ROW_RANGE > ROW_PARTIAL > ROW_INSIDE > IMAGE > ROW_BLEND` |
| `canvas masked-fill visible-range call-site split` | `HelloPerformance text +1552B` | 基础 `RECTANGLE / CIRCLE / ROUND_RECTANGLE / TEXT / IMAGE_565 / EXTERN_IMAGE_565` 与当前抽样 `MASK_*` 场景基本都在噪声范围，但代码量反向增长 | 直接拒绝，不保留 `EGUI_CONFIG_CANVAS_MASK_FILL_VISIBLE_RANGE_FAST_PATH_ENABLE` |
| `mask-image identity-scale fast path` | umbrella `HelloPerformance text -3272B`；`2026-04-05` current-mainline 复验的 `BLEND child` 为 `text -2612B`，`FILL child -660B`，更碎的 `SEGMENT -268B` / `ROW_BLOCK -740B` 已拒绝 | 基础 `RECTANGLE / CIRCLE / ROUND_RECTANGLE / TEXT / IMAGE_565 / EXTERN_IMAGE_565 / IMAGE_RESIZE_565 / EXTERN_IMAGE_RESIZE_565` 不退化；`FILL` 只伤 `MASK_RECT_FILL_IMAGE`，`BLEND child` 完整 `239` 场景里 `9` 个 `>=10%` 回退都集中在 identity-scale masked-image blend 热点，如 `MASK_IMAGE_IMAGE +53.4%`、`EXTERN_MASK_IMAGE_IMAGE +47.4%`、`MASK_IMAGE_RLE_IMAGE +23.2%`、`EXTERN_MASK_IMAGE_RLE_IMAGE +15.0%` | 保留 `EGUI_CONFIG_MASK_IMAGE_IDENTITY_SCALE_FAST_PATH_ENABLE`，并保留 `EGUI_CONFIG_MASK_IMAGE_IDENTITY_SCALE_BLEND_FAST_PATH_ENABLE` 作为更细粒度实验宏；`FILL` child 因收益太小被拒绝 |
| `std image mask visible-range fast path` | `HelloPerformance text -1416B`；继续细拆后 `VISIBLE_RANGE_RESIZE child -820B` | 基础 `RECTANGLE / CIRCLE / ROUND_RECTANGLE / TEXT / IMAGE_565 / EXTERN_IMAGE_565` 不退化；大多数 `masked-image` 场景不动，但 `EXTERN_MASK_IMAGE_TEST_PERF_CIRCLE +84.5%`；继续细拆后 `EXTERN_MASK_IMAGE_TEST_PERF_CIRCLE +96.7%`、`EXTERN_MASK_IMAGE_TEST_PERF_IMAGE +58.9%` | 保留 `EGUI_CONFIG_IMAGE_STD_MASK_VISIBLE_RANGE_FAST_PATH_ENABLE`，作为“项目不用 partial-row external masked-image 热点”时的条件实验宏；`VISIBLE_RANGE_RESIZE` child 因收益不足被拒绝 |
| `std image mask row-range fast path` | `HelloPerformance text -10248B` | 基础 `RECTANGLE / CIRCLE / ROUND_RECTANGLE / TEXT / IMAGE_565 / EXTERN_IMAGE_565 / IMAGE_RESIZE_565 / EXTERN_IMAGE_RESIZE_565` 不退化；但 `MASK_IMAGE_TEST_PERF_CIRCLE +701.9%`、`EXTERN_MASK_IMAGE_TEST_PERF_CIRCLE +468.0%`、`EXTERN_MASK_IMAGE_TEST_PERF_ROUND_RECT +941.7%` | 保留 `EGUI_CONFIG_IMAGE_STD_MASK_ROW_RANGE_FAST_PATH_ENABLE`，作为更激进的 umbrella 条件实验宏；若要先试更可控的关法，优先 `ROW_PARTIAL_DRAW / ROW_PARTIAL_RESIZE`，再考虑 `ROW_PARTIAL` |
| `std image mask row-partial fast path` | `HelloPerformance text -7600B` | 基础 `RECTANGLE / CIRCLE / ROUND_RECTANGLE / TEXT / IMAGE_565 / EXTERN_IMAGE_565 / IMAGE_RESIZE_565 / EXTERN_IMAGE_RESIZE_565` 不退化；但 `MASK_IMAGE_TEST_PERF_CIRCLE +331.9%`、`EXTERN_MASK_IMAGE_TEST_PERF_CIRCLE +206.2%`、`EXTERN_MASK_IMAGE_TEST_PERF_ROUND_RECT +118.6%` | 保留 `EGUI_CONFIG_IMAGE_STD_MASK_ROW_PARTIAL_FAST_PATH_ENABLE`，作为“项目不用 masked-image partial-row 热点”时的条件实验宏；若要先试更保守的 child，优先 `ROW_PARTIAL_DRAW / ROW_PARTIAL_RESIZE` |
| `std image mask row-partial draw fast path` | `HelloPerformance text -2360B` | 基础 `RECTANGLE / CIRCLE / ROUND_RECTANGLE / TEXT / IMAGE_565 / EXTERN_IMAGE_565 / IMAGE_RESIZE_565 / EXTERN_IMAGE_RESIZE_565` 不退化；当前 239 场景 perf run 无 `>=10%` 回退，`IMAGE_565_1/2/4/8` 与 `MASK_IMAGE_TEST_PERF_*` 维持在噪声范围；但 raw alpha masked direct-draw partial-row 热点覆盖仍有限 | 保留 `EGUI_CONFIG_IMAGE_STD_MASK_ROW_PARTIAL_DRAW_FAST_PATH_ENABLE`，作为比 `ROW_PARTIAL` 更保守的 draw-side child 记录；若项目要先保住 partial-row resize / sampled partial-row 场景，应优先试它 |
| `std image mask row-partial draw alpha8 fast path` | `HelloPerformance text -1096B` | 基础 `RECTANGLE / CIRCLE / ROUND_RECTANGLE / TEXT / IMAGE_565 / EXTERN_IMAGE_565 / IMAGE_RESIZE_565 / EXTERN_IMAGE_RESIZE_565` 不退化；`IMAGE_565_8 / EXTERN_IMAGE_565_8` 与 `MASK_IMAGE_QOI_8_CIRCLE / MASK_IMAGE_RLE_8_CIRCLE / EXTERN_MASK_IMAGE_QOI_8_CIRCLE / EXTERN_MASK_IMAGE_RLE_8_CIRCLE` 都维持在噪声范围；但 raw alpha8 masked direct-draw partial-row 热点覆盖仍有限 | 保留 `EGUI_CONFIG_IMAGE_STD_MASK_ROW_PARTIAL_DRAW_ALPHA8_FAST_PATH_ENABLE`，作为比 `ROW_PARTIAL_DRAW` 更保守的 alpha8 child 记录；若项目要先保住 packed-alpha partial-row draw，应优先试它 |
| `std image mask row-partial draw packed-alpha fast path` | `HelloPerformance text -1360B` | 基础 `RECTANGLE / CIRCLE / ROUND_RECTANGLE / TEXT / IMAGE_565 / EXTERN_IMAGE_565 / IMAGE_RESIZE_565 / EXTERN_IMAGE_RESIZE_565` 不退化；`IMAGE_565_1/2/4 / EXTERN_IMAGE_565_1/2/4` 与 `MASK_IMAGE_QOI_CIRCLE / MASK_IMAGE_RLE_CIRCLE / EXTERN_MASK_IMAGE_QOI_CIRCLE / EXTERN_MASK_IMAGE_RLE_CIRCLE` 都维持在噪声范围；但 raw packed-alpha masked direct-draw partial-row 热点覆盖仍有限 | 保留 `EGUI_CONFIG_IMAGE_STD_MASK_ROW_PARTIAL_DRAW_PACKED_ALPHA_FAST_PATH_ENABLE`，作为比 `ROW_PARTIAL_DRAW` 更保守的 packed child 记录；若项目要先保住 alpha8 partial-row draw，应优先试它 |
| `std image mask row-partial resize fast path` | `HelloPerformance text -6276B` | 基础 `RECTANGLE / CIRCLE / ROUND_RECTANGLE / TEXT / IMAGE_565 / EXTERN_IMAGE_565 / IMAGE_RESIZE_565 / EXTERN_IMAGE_RESIZE_565` 不退化；代价集中在 `MASK_IMAGE_CIRCLE +148.0%`、`EXTERN_MASK_IMAGE_CIRCLE +141.6%`、`MASK_IMAGE_TEST_PERF_CIRCLE +331.9%`、`EXTERN_MASK_IMAGE_TEST_PERF_CIRCLE +206.2%`、`EXTERN_MASK_IMAGE_TEST_PERF_ROUND_RECT +118.6%` | 保留 `EGUI_CONFIG_IMAGE_STD_MASK_ROW_PARTIAL_RESIZE_FAST_PATH_ENABLE`，作为比 `ROW_PARTIAL` 更保守的 masked-image resize 条件实验宏 |
| `std image mask row-partial resize packed-alpha fast path` | `HelloPerformance text -2436B` | 基础 `RECTANGLE / CIRCLE / ROUND_RECTANGLE / TEXT / IMAGE_565 / EXTERN_IMAGE_565 / IMAGE_RESIZE_565 / EXTERN_IMAGE_RESIZE_565` 不退化；`IMAGE_RESIZE_565_1/2/4 / EXTERN_IMAGE_RESIZE_565_1/2/4` 与 `MASK_IMAGE_CIRCLE / EXTERN_MASK_IMAGE_CIRCLE / MASK_IMAGE_TEST_PERF_CIRCLE / EXTERN_MASK_IMAGE_TEST_PERF_CIRCLE / EXTERN_MASK_IMAGE_TEST_PERF_ROUND_RECT` 都维持在噪声范围；当前 `239` 场景最大绝对波动仅 `+1.3%` | 保留 `EGUI_CONFIG_IMAGE_STD_MASK_ROW_PARTIAL_RESIZE_PACKED_ALPHA_FAST_PATH_ENABLE`，作为比 `ROW_PARTIAL_RESIZE` 更保守的 packed child 记录；若项目要先保住 alpha8 / raw sampled partial-row resize，应优先试它 |
| `std image mask row-partial resize rgb565 fast path` | `HelloPerformance text -2936B` | 基础 `RECTANGLE / CIRCLE / ROUND_RECTANGLE / TEXT / IMAGE_565 / EXTERN_IMAGE_565 / IMAGE_RESIZE_565 / EXTERN_IMAGE_RESIZE_565`、`IMAGE_RESIZE_565_1/2/4 / EXTERN_IMAGE_RESIZE_565_1/2/4` 与 `MASK_IMAGE_CIRCLE / EXTERN_MASK_IMAGE_CIRCLE` 不退化；但 `MASK_IMAGE_TEST_PERF_CIRCLE +332.0%`、`EXTERN_MASK_IMAGE_TEST_PERF_CIRCLE +206.2%`、`EXTERN_MASK_IMAGE_TEST_PERF_ROUND_RECT +118.6%`、`EXTERN_MASK_IMAGE_TEST_PERF_IMAGE +89.7%`，且 runtime baseline / off 有 `1` 帧像素差 | 仅保留 `EGUI_CONFIG_IMAGE_STD_MASK_ROW_PARTIAL_RESIZE_RGB565_FAST_PATH_ENABLE` 的实验记录；如果项目明确不依赖 raw sampled masked-image `TEST_PERF_*` 热点，且可接受 `1` 帧像素差，可再单独试它；默认不建议优先于 `ROW_PARTIAL_RESIZE_PACKED_ALPHA` |
| `std image mask row-partial resize alpha8 fast path` | `HelloPerformance text -1000B` | 基础 `RECTANGLE / CIRCLE / ROUND_RECTANGLE / TEXT / IMAGE_565 / EXTERN_IMAGE_565 / IMAGE_RESIZE_565 / EXTERN_IMAGE_RESIZE_565` 与 raw sampled resize 对照组不退化；代价继续收敛到 `MASK_IMAGE_CIRCLE +148.0%`、`EXTERN_MASK_IMAGE_CIRCLE +141.6%`、`MASK_IMAGE_CIRCLE_QUARTER +116.6%`、`MASK_IMAGE_CIRCLE_DOUBLE +80.4%`；但 runtime baseline / off 有 `4` 帧细微 circle 边缘像素差 | 保留 `EGUI_CONFIG_IMAGE_STD_MASK_ROW_PARTIAL_RESIZE_ALPHA8_FAST_PATH_ENABLE` 作为条件实验宏；不进入当前优先序列，若要求 on/off 渲染完全等价则不建议默认关 |
| `std image mask row-inside resize fast path` | `HelloPerformance text -928B` | 基础 `RECTANGLE / CIRCLE / ROUND_RECTANGLE / TEXT / IMAGE_565 / EXTERN_IMAGE_565 / IMAGE_RESIZE_565 / EXTERN_IMAGE_RESIZE_565` 不退化；sampled non-test masked-image resize 场景维持在噪声范围；但 `MASK_IMAGE_TEST_PERF_CIRCLE +399.1%`、`EXTERN_MASK_IMAGE_TEST_PERF_CIRCLE +254.7%`、`EXTERN_MASK_IMAGE_TEST_PERF_ROUND_RECT +813.6%`、`EXTERN_MASK_IMAGE_TEST_PERF_IMAGE +14.5%` | 保留 `EGUI_CONFIG_IMAGE_STD_MASK_ROW_INSIDE_RESIZE_FAST_PATH_ENABLE`，作为比 `ROW_INSIDE` 更保守的 inside-row resize 条件实验宏 |
| `std image mask row-inside resize alpha8 fast path` | `HelloPerformance text -712B` | 基础 `RECTANGLE / CIRCLE / ROUND_RECTANGLE / TEXT / IMAGE_565 / EXTERN_IMAGE_565 / IMAGE_RESIZE_565 / EXTERN_IMAGE_RESIZE_565` 与 raw sampled inside-row resize 对照组不退化；`MASK_IMAGE_TEST_PERF_CIRCLE / EXTERN_MASK_IMAGE_TEST_PERF_CIRCLE / EXTERN_MASK_IMAGE_TEST_PERF_ROUND_RECT / EXTERN_MASK_IMAGE_TEST_PERF_IMAGE` 全部 `+0.0%`；但 `MASK_IMAGE_CIRCLE +208.3%`、`EXTERN_MASK_IMAGE_CIRCLE +199.5%`、`MASK_IMAGE_CIRCLE_QUARTER +157.4%`、`MASK_IMAGE_CIRCLE_DOUBLE +286.6%`；runtime hash / pixel mismatch `0` | 保留 `EGUI_CONFIG_IMAGE_STD_MASK_ROW_INSIDE_RESIZE_ALPHA8_FAST_PATH_ENABLE` 作为比 `ROW_INSIDE_RESIZE` 更保守的 child 记录；收益低于 `1KB`，不进入当前主推荐序列 |
| `std image mask row-inside fast path` | `HelloPerformance text -1340B` | 基础 `RECTANGLE / CIRCLE / ROUND_RECTANGLE / TEXT / IMAGE_565 / EXTERN_IMAGE_565 / IMAGE_RESIZE_565 / EXTERN_IMAGE_RESIZE_565` 不退化；但 `MASK_IMAGE_TEST_PERF_CIRCLE +399.1%`、`EXTERN_MASK_IMAGE_TEST_PERF_CIRCLE +255.1%`、`EXTERN_MASK_IMAGE_TEST_PERF_ROUND_RECT +813.6%` | 保留 `EGUI_CONFIG_IMAGE_STD_MASK_ROW_INSIDE_FAST_PATH_ENABLE`，作为“项目不用 masked-image inside-row 热点”时的条件实验宏；优先级低于 `ROW_PARTIAL` |
| `font std fast draw` | `HelloSimple __code_size -8328B`；主 helper 合计约 `6KB+` | 关闭后 `TEXT +27.3%`、`TEXT_RECT +316.6%`、`TEXT_RECT_GRADIENT +464.8%` | 保留，且不要新增长期宏 |
| `font std fast mask draw` | `HelloPerformance text -2576B`；主要是 `egui_font_std_draw_fast_mask` / `egui_font_std_draw_string_fast_4_mask` | 基础场景 `TEXT -0.3%`、`EXTERN_TEXT -0.5%`，其余优先路径 `+0.0%`；但当前未覆盖 masked-text 专项场景 | 保留 `EGUI_CONFIG_FONT_STD_FAST_MASK_DRAW_ENABLE`，作为更细粒度实验型开关 |
| `font std mask row-blend fast path` | `HelloPerformance text -1632B`；主要是 `egui_font_std_draw_fast_mask` | 基础 `RECTANGLE / CIRCLE / ROUND_RECTANGLE / TEXT / EXTERN_TEXT / IMAGE_565 / EXTERN_IMAGE_565` 不退化；但 `TEXT_GRADIENT +227.2%`、`TEXT_RECT_GRADIENT +467.9%`、`TEXT_ROTATE_GRADIENT +0.0%` | 保留 `EGUI_CONFIG_FONT_STD_MASK_ROW_BLEND_FAST_PATH_ENABLE`，作为比 `FONT_STD_FAST_MASK_DRAW` 更保守的 child；若项目只愿意牺牲 gradient masked-text 热点，优先试这颗宏 |
| `std image fast draw main switch` | `HelloPerformance text -38228B`、`bss -40B`；关闭后回落到 generic `get_pixel` draw/resize 路径 | `IMAGE_565 +1078.7%`、`IMAGE_565_1 +558.6%`、`IMAGE_565_8 +347.1%`；`EXTERN_IMAGE_565` 虽变快，但不足以改变结论 | 保留 `EGUI_CONFIG_IMAGE_STD_FAST_DRAW_ENABLE`，默认开启，仅保留实验型开关 |
| `std image fast draw alpha fast path` | `HelloPerformance text -8016B`；主要是 `RGB565_1/2/4/8` direct-draw specialized helpers | 基础场景 `IMAGE_565 +0.0%`、`EXTERN_IMAGE_565 -0.5%`；但 `IMAGE_565_1/2/4/8 +326.8% ~ +559.6%` | 保留 `EGUI_CONFIG_IMAGE_STD_FAST_DRAW_ALPHA_ENABLE`，作为更细粒度实验型开关 |
| `std image alpha8 direct-draw fast path` | `HelloPerformance text -1748B`；主要是 `RGB565_8` direct-draw helpers | 基础场景 `IMAGE_565 +0.0%`、`EXTERN_IMAGE_565 +0.0%`；但 internal `IMAGE_565_8 +348.4%` | 保留 `EGUI_CONFIG_IMAGE_STD_FAST_DRAW_ALPHA8_ENABLE`，作为“只牺牲 internal RGB565_8 direct draw”的实验型开关 |
| `std image packed-alpha direct-draw fast path` | `HelloPerformance text -5548B`；主要是 internal `RGB565_1/2/4` direct-draw helpers | 基础场景 `IMAGE_565 +0.0%`、`EXTERN_IMAGE_565 -0.5%`、`IMAGE_565_8 -0.4%`；但 internal `IMAGE_565_1/2/4 +318.4% ~ +559.6%` | 保留 `EGUI_CONFIG_IMAGE_STD_FAST_DRAW_PACKED_ALPHA_ENABLE`，作为“只牺牲 internal RGB565_1/2/4 direct draw”的实验型开关 |
| `std image alpha-color fast path` | `HelloPerformance text -1248B`；主要是 alpha-only tinted image draw/resize specialized helpers；继续细拆后 `DRAW -52B`、`RESIZE -184B` | 基础场景 `RECTANGLE / CIRCLE / ROUND_RECTANGLE / TEXT / IMAGE_565 / EXTERN_IMAGE_565 +0.0%`；umbrella 关闭会让 `IMAGE_COLOR +362.9%`、`IMAGE_RESIZE_COLOR +395.7%`；child split 也分别只把代价隔离到对应热点 | 保留 `EGUI_CONFIG_IMAGE_STD_ALPHA_COLOR_FAST_PATH_ENABLE`，作为项目不用 alpha-only tinted image 热点时的条件实验宏；`DRAW/RESIZE` child 因收益太小被拒绝 |
| `std image external alpha fast path` | `HelloPerformance text -4056B`；主要是 external `RGB565_1/2/4/8` draw/resize specialized helpers | 基础场景 `IMAGE_565 +0.0%`、`EXTERN_IMAGE_565 -0.7%`、`IMAGE_RESIZE_565 +0.0%`、`EXTERN_IMAGE_RESIZE_565 +0.2%` | 保留 `EGUI_CONFIG_IMAGE_STD_EXTERNAL_ALPHA_FAST_PATH_ENABLE`，作为“只牺牲 external alpha specialized path”的实验型开关 |
| `std image external alpha8 fast path` | `HelloPerformance text -588B`；主要是 external `RGB565_8` draw/resize specialized helpers | 基础场景 `IMAGE_565 +0.0%`、`EXTERN_IMAGE_565 -0.7%`、`IMAGE_RESIZE_565 +0.0%`、`EXTERN_IMAGE_RESIZE_565 -0.2%`；但 external `EXTERN_IMAGE_565_8 -86.2%`、`EXTERN_IMAGE_RESIZE_565_8 -84.2%` | 不进入当前优先保留宏序列；收益低于 `1KB`，仅保留细拆记录 |
| `std image external packed-alpha fast path` | `HelloPerformance text -2816B`；主要是 external `RGB565_1/2/4` draw/resize specialized helpers | 基础场景 `IMAGE_565 +0.0%`、`EXTERN_IMAGE_565 +0.0%`、`IMAGE_RESIZE_565 +0.0%`、`EXTERN_IMAGE_RESIZE_565 -0.2%`、`EXTERN_IMAGE_565_8 +0.0%`、`EXTERN_IMAGE_RESIZE_565_8 -0.3%`；但 external `RGB565_1/2/4` draw/resize 热点 `-83.6% ~ -88.9%` | 保留 `EGUI_CONFIG_IMAGE_STD_EXTERNAL_PACKED_ALPHA_FAST_PATH_ENABLE`，作为“只牺牲 external packed-alpha specialized path”的细粒度实验开关 |
| `std image external packed-alpha resize fast path` | `HelloPerformance text -2796B`；主要是 external `RGB565_1/2/4` resize specialized helpers | 基础场景 `IMAGE_565 +0.0%`、`EXTERN_IMAGE_565 +0.0%`、`IMAGE_RESIZE_565 +0.0%`、`EXTERN_IMAGE_RESIZE_565 +0.0%`、`EXTERN_IMAGE_565_1/2/4 +0.0% ~ +1.6%`；但 external `EXTERN_IMAGE_RESIZE_565_1/2/4 -84.1% ~ -87.2%` | 保留 `EGUI_CONFIG_IMAGE_STD_EXTERNAL_PACKED_ALPHA_RESIZE_FAST_PATH_ENABLE`，作为“只牺牲 external RGB565_1/2/4 resize”的更细粒度实验开关；draw-only split `-180B` 已拒绝 |
| `std image opaque-source promotion` | `HelloPerformance text -1256B`、`bss -40B`；主要是 opaque-source 检测/缓存/回推逻辑 | 基础场景 `IMAGE_565 -0.4%`、`EXTERN_IMAGE_565 -0.2%`、`IMAGE_RESIZE_565 +0.0%`；当前扩展到 `RGB565_1/2/4/8` 也无明显退化 | 保留 `EGUI_CONFIG_IMAGE_STD_RGB565_OPAQUE_SOURCE_CHECK_ENABLE`，作为更保守的实验型开关 |
| `std image row overlay fast path` | `HelloPerformance text -2744B`；主要是 `mask_get_row_overlay()` 相关的 std-image specialized branches | 基础场景 `IMAGE_565 -0.4%`、`EXTERN_IMAGE_565 -0.5%`、`IMAGE_RESIZE_565 +0.0%`；但 `IMAGE_GRADIENT_OVERLAY +531.3%` | 保留 `EGUI_CONFIG_IMAGE_STD_ROW_OVERLAY_FAST_PATH_ENABLE`，仅在 gradient-image overlay 不是热点时按需关闭 |
| `std image row-overlay non-rgb565 fast path` | `HelloPerformance text -1476B`；继续细拆后 `ALPHA8 child -1264B`、generic `GET_PIXEL child -188B` 已拒绝；raw `RGB565` row-overlay 仍留在父宏里 | `2026-04-05` current-mainline 复验的完整 `239` 场景没有任何 `>=10%` 回退；基础 `IMAGE_565 -0.8%`、`EXTERN_IMAGE_565 -0.7%`、`IMAGE_RESIZE_565 +0.2%`、`EXTERN_IMAGE_RESIZE_565 +0.0%`；`IMAGE_GRADIENT_OVERLAY +0.1%`、`MASK_GRADIENT_IMAGE +0.0%`、`MASK_GRADIENT_IMAGE_ROTATE +0.0%` | 保留 `EGUI_CONFIG_IMAGE_STD_ROW_OVERLAY_NON_RGB565_FAST_PATH_ENABLE`，作为比 umbrella 更保守的 row-overlay 实验宏；如果项目还想保住 generic `get_pixel` overlay，应先试 `ROW_OVERLAY_ALPHA8` child，再考虑这颗父宏 |
| `std image row-overlay alpha8 fast path` | `HelloPerformance text -1264B`；只覆盖 `RGB565_8` draw / resize 的 row-overlay fast path | 基础 `IMAGE_565 / EXTERN_IMAGE_565 / IMAGE_RESIZE_565 / EXTERN_IMAGE_RESIZE_565` 与当前 `IMAGE_GRADIENT_OVERLAY / MASK_GRADIENT_IMAGE / MASK_GRADIENT_IMAGE_ROTATE` 不退化；但 `IMAGE_565_8_DOUBLE +383.9%` | 保留 `EGUI_CONFIG_IMAGE_STD_ROW_OVERLAY_ALPHA8_FAST_PATH_ENABLE`，作为比 `ROW_OVERLAY_NON_RGB565` 更保守的 child；若项目要保住 generic `get_pixel` overlay、但可以牺牲 `RGB565_8` overlay 热点，应先试这颗 child |
| `std image row-overlay rgb565 fast path` | `HelloPerformance text -1312B`；主要是 raw `RGB565` direct draw / resize 的 row-overlay fast path | 基础场景 `IMAGE_565 -0.4%`、`EXTERN_IMAGE_565 -0.5%`、`IMAGE_RESIZE_565 +0.0%`、`EXTERN_IMAGE_RESIZE_565 +0.0%`；当前 `MASK_GRADIENT_IMAGE +0.0%`、`MASK_GRADIENT_IMAGE_ROTATE +0.0%`；但 raw `IMAGE_GRADIENT_OVERLAY +165.6%` | 保留 `EGUI_CONFIG_IMAGE_STD_ROW_OVERLAY_RGB565_FAST_PATH_ENABLE`，作为与 `NON_RGB565` / `ROW_OVERLAY_ALPHA8` 互补的 row-overlay child；若项目已经保住 non-raw overlay，只剩 raw gradient-image overlay 可以牺牲，再试这颗 child |
| `std image fast resize fast path` | `HelloPerformance text -19072B`；关闭后回落到 generic resize 路径 | 基础场景 `IMAGE_565 -0.4%`、`EXTERN_IMAGE_565 -1.0%`；但 `IMAGE_RESIZE_565 +544.2%` | 保留 `EGUI_CONFIG_IMAGE_STD_FAST_RESIZE_ENABLE`，默认开启，仅保留实验型开关 |
| `std image fast resize alpha fast path` | `HelloPerformance text -11252B`；主要是 `RGB565_1/2/4/8` resize specialized helpers | 基础场景 `IMAGE_RESIZE_565 -0.2%`、`EXTERN_IMAGE_RESIZE_565 +0.2%`；但 alpha resize 热点 `+293.5% ~ +369.9%` | 保留 `EGUI_CONFIG_IMAGE_STD_FAST_RESIZE_ALPHA_ENABLE`，作为更细粒度实验型开关 |
| `std image alpha8 resize fast path` | `HelloPerformance text -3756B`；主要是 `RGB565_8` resize helpers | 基础场景 `IMAGE_565 +0.0%`、`EXTERN_IMAGE_565 +0.0%`、`IMAGE_RESIZE_565 +0.2%`、`EXTERN_IMAGE_RESIZE_565 +0.0%`；但 internal `IMAGE_RESIZE_565_8 +300.0%` | 保留 `EGUI_CONFIG_IMAGE_STD_FAST_RESIZE_ALPHA8_ENABLE`，作为“只牺牲 internal RGB565_8 resize”的实验型开关 |
| `std image packed-alpha resize fast path` | `HelloPerformance text -7512B`；主要是 internal `RGB565_1/2/4` resize helpers | 基础场景 `IMAGE_565 -0.4%`、`EXTERN_IMAGE_565 -1.0%`、`IMAGE_RESIZE_565 +0.0%`、`EXTERN_IMAGE_RESIZE_565 +0.0%`、`IMAGE_RESIZE_565_8 -1.6%`；但 internal `IMAGE_RESIZE_565_1/2/4 +293.9% ~ +370.4%` | 保留 `EGUI_CONFIG_IMAGE_STD_FAST_RESIZE_PACKED_ALPHA_ENABLE`，作为“只牺牲 internal RGB565_1/2/4 resize”的实验型开关 |
| `image mask circle / round-rect fast path` | 粗粒度整体关闭时 `HelloBasic(mask) __code_size -5164B`；`HelloPerformance` 下关闭总开关 `text -13032B`；继续细拆后 `MASK_CIRCLE -7368B`、`MASK_ROUND_RECT -9540B` | 整条路径不能回收；但继续细拆成 `EGUI_CONFIG_IMAGE_STD_MASK_CIRCLE_FAST_PATH_ENABLE` / `EGUI_CONFIG_IMAGE_STD_MASK_ROUND_RECT_FAST_PATH_ENABLE` 后，基础 raw image / resize 主路径仍在 `<10%`，且 perf 代价能按 shape 分摊 | 保留总开关，并继续保留两个子宏作为更细粒度实验开关；默认更建议优先关闭 `MASK_ROUND_RECT` 子宏 |
| `std image mask round-rect resize fast path` | `2026-04-05` current-mainline `HelloPerformance text -3764B`；继续细拆后 `ROUND_RECT_RESIZE_ALPHA8 child -1136B`、`ROUND_RECT_RESIZE_RGB565 child -2472B` | 基础 `RECTANGLE / CIRCLE / ROUND_RECTANGLE / TEXT / IMAGE_565 / EXTERN_IMAGE_565 / IMAGE_RESIZE_565 / EXTERN_IMAGE_RESIZE_565` 不退化；父宏的 `>=10%` 回退集中在 internal round-rect resize：`MASK_IMAGE_ROUND_RECT +10.8%`、`MASK_IMAGE_ROUND_RECT_DOUBLE +10.4%`；`ROUND_RECT_RESIZE_ALPHA8` 把代价收敛到 alpha8 round-rect resize 热点；`ROUND_RECT_RESIZE_RGB565` 当前完整 `239` 场景没有任何 `>=10%` 回退，raw round-rect resize 样本里最高也只有 `MASK_IMAGE_TEST_PERF_ROUND_RECT +9.7%`，同时 alpha8 round-rect resize 与 draw / codec / external 对照组不退化 | 保留 `EGUI_CONFIG_IMAGE_STD_MASK_ROUND_RECT_RESIZE_FAST_PATH_ENABLE`，作为比 `MASK_ROUND_RECT` 更保守的实验宏；若项目要保住 raw round-rect resize，优先试 `ROUND_RECT_RESIZE_ALPHA8`；若项目要保住 alpha8 round-rect resize，优先试 `ROUND_RECT_RESIZE_RGB565`；只有两边都能牺牲时再考虑父宏 |
| `std image mask round-rect resize alpha8 fast path` | `2026-04-05` current-mainline `HelloPerformance text -1136B`；主要是 round-rect `RGB565_8` masked-image resize specialized helper | 基础 `RECTANGLE / CIRCLE / ROUND_RECTANGLE / TEXT / IMAGE_565 / EXTERN_IMAGE_565 / IMAGE_RESIZE_565 / EXTERN_IMAGE_RESIZE_565` 不退化；`2026-04-05` 完整 `239` 场景里 `2` 个 `>=10%` 回退仍只落在 round-rect alpha8 resize 热点：`MASK_IMAGE_ROUND_RECT +10.8%`、`MASK_IMAGE_ROUND_RECT_DOUBLE +10.4%`；`EXTERN_MASK_IMAGE_ROUND_RECT +9.8%`、`MASK_IMAGE_ROUND_RECT_QUARTER +8.0%` 仍低于阈值；raw round-rect resize 与 draw / codec 对照组不退化 | 保留 `EGUI_CONFIG_IMAGE_STD_MASK_ROUND_RECT_RESIZE_ALPHA8_FAST_PATH_ENABLE`，作为比 `ROUND_RECT_RESIZE` 更保守的 child；若项目要保住 raw round-rect resize，应优先试这颗宏 |
| `std image mask round-rect resize rgb565 fast path` | `2026-04-05` current-mainline `HelloPerformance text -2472B`；主要是 round-rect raw `RGB565` masked-image resize specialized helpers | 基础 `RECTANGLE / CIRCLE / ROUND_RECTANGLE / TEXT / IMAGE_565 / EXTERN_IMAGE_565 / IMAGE_RESIZE_565 / EXTERN_IMAGE_RESIZE_565` 不退化；完整 `239` 场景没有任何 `>=10%` 回退；raw round-rect resize 样本里最高也只有 `MASK_IMAGE_TEST_PERF_ROUND_RECT +9.7%`，`MASK_IMAGE_ROUND_RECT / MASK_IMAGE_ROUND_RECT_DOUBLE / EXTERN_MASK_IMAGE_ROUND_RECT` 都不退化；alpha8 round-rect resize 与 draw / codec / external 对照组也不退化 | 保留 `EGUI_CONFIG_IMAGE_STD_MASK_ROUND_RECT_RESIZE_RGB565_FAST_PATH_ENABLE`，作为与 `ROUND_RECT_RESIZE_ALPHA8` 互补的 child；若项目要保住 alpha8 round-rect resize，应优先试这颗宏 |
| `std image mask round-rect draw fast path` | `2026-04-05` current-mainline `HelloPerformance text -5804B`；继续细拆后 `ROUND_RECT_DRAW_ALPHA8 child -3960B`、`ROUND_RECT_DRAW_RGB565 child -2216B` | 基础 `RECTANGLE / CIRCLE / ROUND_RECTANGLE / TEXT / IMAGE_565 / EXTERN_IMAGE_565 / IMAGE_RESIZE_565 / EXTERN_IMAGE_RESIZE_565` 不退化；internal round-rect resize 样本 `+0.2%` 以内；`2026-04-05` 完整 `239` 场景里 `4` 个 `>=10%` 回退都集中在 round-rect codec draw 热点：`MASK_IMAGE_QOI_ROUND_RECT +10.4%`、`MASK_IMAGE_RLE_ROUND_RECT +24.7%`、`MASK_IMAGE_RLE_8_ROUND_RECT +13.7%`、`EXTERN_MASK_IMAGE_RLE_ROUND_RECT +12.8%` | 保留 `EGUI_CONFIG_IMAGE_STD_MASK_ROUND_RECT_DRAW_FAST_PATH_ENABLE`，作为与 `ROUND_RECT_RESIZE` 互补的实验宏；若项目要保住 raw / non-alpha8 round-rect draw，应优先 `ROUND_RECT_DRAW_ALPHA8`；若项目要保住 alpha8 round-rect draw，应优先 `ROUND_RECT_DRAW_RGB565`；只有两边都能牺牲时再考虑父宏 |
| `std image mask round-rect draw alpha8 fast path` | `HelloPerformance text -3960B`；主要是 round-rect `RGB565_8` masked-image direct-draw / row-block specialized helpers；继续细拆 `ROW_FAST child -3960B`，但已证实与本行叶子宏等价 | 基础 `RECTANGLE / CIRCLE / ROUND_RECTANGLE / TEXT / IMAGE_565 / EXTERN_IMAGE_565 / IMAGE_RESIZE_565 / EXTERN_IMAGE_RESIZE_565` 不退化；raw round-rect draw / resize 与 non-alpha8 `QOI / RLE / external` 样本不退化；但 `MASK_IMAGE_QOI_8_ROUND_RECT +9.5%`、`MASK_IMAGE_RLE_8_ROUND_RECT +13.7%`、`EXTERN_MASK_IMAGE_QOI_8_ROUND_RECT +7.7%`、`EXTERN_MASK_IMAGE_RLE_8_ROUND_RECT +9.2%` | 保留 `EGUI_CONFIG_IMAGE_STD_MASK_ROUND_RECT_DRAW_ALPHA8_FAST_PATH_ENABLE`，作为比 `ROUND_RECT_DRAW` 更保守的 child；`ROW_FAST child` 因与本叶子宏覆盖范围重合而拒绝；若项目要保住 raw round-rect draw，应优先试这颗宏 |
| `std image mask round-rect draw rgb565 fast path` | `2026-04-05` current-mainline `HelloPerformance text -2216B`；主要是 round-rect non-alpha8 masked-image direct-draw / row-block specialized helpers | 基础 `RECTANGLE / CIRCLE / ROUND_RECTANGLE / TEXT / IMAGE_565 / EXTERN_IMAGE_565 / IMAGE_RESIZE_565 / EXTERN_IMAGE_RESIZE_565` 不退化；完整 `239` 场景没有任何 `>=10%` 回退；non-alpha8 round-rect draw 热点仍低于阈值：`MASK_IMAGE_QOI_ROUND_RECT +3.2%`、`MASK_IMAGE_RLE_ROUND_RECT +7.7%`、`EXTERN_MASK_IMAGE_QOI_ROUND_RECT +1.3%`、`EXTERN_MASK_IMAGE_RLE_ROUND_RECT +4.0%`；alpha8 round-rect draw 与 raw round-rect / resize 对照组不退化 | 保留 `EGUI_CONFIG_IMAGE_STD_MASK_ROUND_RECT_DRAW_RGB565_FAST_PATH_ENABLE`，作为与 `ROUND_RECT_DRAW_ALPHA8` 互补的 child；若项目要保住 alpha8 round-rect draw，应优先试这颗宏 |
| `std image mask circle resize fast path` | `HelloPerformance text -1984B`；继续细拆后 `CIRCLE_RESIZE_ALPHA8 child -540B`、`CIRCLE_RESIZE_RGB565 child -1456B` | 基础 `RECTANGLE / CIRCLE / ROUND_RECTANGLE / TEXT / IMAGE_565 / EXTERN_IMAGE_565 / IMAGE_RESIZE_565 / EXTERN_IMAGE_RESIZE_565` 不退化；父宏会伤 internal circle raw + alpha8 resize 热点；`CIRCLE_RESIZE_ALPHA8` 只伤 alpha8 circle resize 热点；`CIRCLE_RESIZE_RGB565` 则继续保住 alpha8 circle resize 与 circle draw / codec / external 对照组，但仍有 `MASK_IMAGE_TEST_PERF_CIRCLE +36.2%`，且 runtime 有 `frame_0190.png` 的 `1` 帧像素差 | 保留 `EGUI_CONFIG_IMAGE_STD_MASK_CIRCLE_RESIZE_FAST_PATH_ENABLE`，作为比 `MASK_CIRCLE` 更保守的实验宏；若项目极度想保住 raw circle resize，可先试 `CIRCLE_RESIZE_ALPHA8`；若项目更想保住 alpha8 circle resize，且能接受 sampled raw circle hotspot / `1` 帧像素差，可再试 `CIRCLE_RESIZE_RGB565`；否则仍以父宏为主 |
| `std image mask circle resize alpha8 fast path` | `HelloPerformance text -540B`；主要是 circle `RGB565_8` masked-image resize specialized helper | 基础 `RECTANGLE / CIRCLE / ROUND_RECTANGLE / TEXT / IMAGE_565 / EXTERN_IMAGE_565 / IMAGE_RESIZE_565 / EXTERN_IMAGE_RESIZE_565` 不退化；raw `MASK_IMAGE_TEST_PERF_CIRCLE / EXTERN_MASK_IMAGE_TEST_PERF_CIRCLE` 与 circle draw / codec 对照组不退化；但 `MASK_IMAGE_CIRCLE +11.4%`、`EXTERN_MASK_IMAGE_CIRCLE +11.0%`、`MASK_IMAGE_CIRCLE_QUARTER +7.1%`、`MASK_IMAGE_CIRCLE_DOUBLE +9.3%` | 不进入当前优先保留宏序列；收益低于 `1KB`，仅保留细拆记录；若项目极度想保住 raw circle resize，可先试这颗宏 |
| `std image mask circle resize rgb565 fast path` | `2026-04-05` current-mainline `HelloPerformance text -1456B`；主要是 circle raw `RGB565` masked-image resize specialized helpers | 基础 `RECTANGLE / CIRCLE / ROUND_RECTANGLE / TEXT / IMAGE_565 / EXTERN_IMAGE_565 / IMAGE_RESIZE_565 / EXTERN_IMAGE_RESIZE_565` 不退化；`MASK_IMAGE_CIRCLE / EXTERN_MASK_IMAGE_CIRCLE / MASK_IMAGE_CIRCLE_QUARTER / MASK_IMAGE_CIRCLE_DOUBLE` 与 alpha8 circle resize 对照组不退化；但 `MASK_IMAGE_TEST_PERF_CIRCLE +36.2%`，runtime on / off 也有 `frame_0190.png` 的 `1` 帧真实像素差 | 仅保留 `EGUI_CONFIG_IMAGE_STD_MASK_CIRCLE_RESIZE_RGB565_FAST_PATH_ENABLE` 的实验记录；如果项目更想保住 alpha8 circle resize，且明确不依赖 raw sampled circle resize hotspot、并能接受 `1` 帧像素差，可在父宏之前单独试它 |
| `std image mask circle draw fast path` | `HelloPerformance text -3028B`；继续细拆后 `CIRCLE_DRAW_ALPHA8 child -1876B`、`CIRCLE_DRAW_RGB565 child -1200B` | 基础 `RECTANGLE / CIRCLE / ROUND_RECTANGLE / TEXT / IMAGE_565 / EXTERN_IMAGE_565 / IMAGE_RESIZE_565 / EXTERN_IMAGE_RESIZE_565` 不退化；internal circle resize 样本仍不退化；父宏会伤 circle 的 raw + alpha8 draw 热点，而继续细拆后的 `CIRCLE_DRAW_ALPHA8` child 只伤 `MASK_IMAGE_QOI_8_CIRCLE +232.5%`、`MASK_IMAGE_RLE_8_CIRCLE +331.1%`、`EXTERN_MASK_IMAGE_QOI_8_CIRCLE +189.5%`、`EXTERN_MASK_IMAGE_RLE_8_CIRCLE +226.6%`；`CIRCLE_DRAW_RGB565` child 则保住 alpha8 circle draw，当前完整 `239` 场景没有任何 `>=10%` 回退，non-alpha8 circle draw 热点最大 `MASK_IMAGE_RLE_CIRCLE +6.5%` | 保留 `EGUI_CONFIG_IMAGE_STD_MASK_CIRCLE_DRAW_FAST_PATH_ENABLE`，作为与 `CIRCLE_RESIZE` 互补的实验宏；若项目要保住 raw / non-alpha8 circle draw，应优先试 `CIRCLE_DRAW_ALPHA8`；若项目要保住 alpha8 circle draw，应优先试 `CIRCLE_DRAW_RGB565`；只有两边都能牺牲时再考虑 `CIRCLE_DRAW` |
| `std image mask circle draw alpha8 fast path` | `HelloPerformance text -1876B`；主要是 circle `RGB565_8` masked-image direct-draw / row-block specialized helpers | 基础 `RECTANGLE / CIRCLE / ROUND_RECTANGLE / TEXT / IMAGE_565 / EXTERN_IMAGE_565 / IMAGE_RESIZE_565 / EXTERN_IMAGE_RESIZE_565` 不退化；raw circle draw / resize 与 non-alpha8 `QOI / RLE / external` 样本不退化；但 `MASK_IMAGE_QOI_8_CIRCLE +232.5%`、`MASK_IMAGE_RLE_8_CIRCLE +331.1%`、`EXTERN_MASK_IMAGE_QOI_8_CIRCLE +189.5%`、`EXTERN_MASK_IMAGE_RLE_8_CIRCLE +226.6%` | 保留 `EGUI_CONFIG_IMAGE_STD_MASK_CIRCLE_DRAW_ALPHA8_FAST_PATH_ENABLE`，作为比 `CIRCLE_DRAW` 更保守的 child；若项目要保住 raw circle draw，应优先试这颗宏 |
| `std image mask circle draw rgb565 fast path` | `2026-04-05` current-mainline `HelloPerformance text -1200B`；主要是 circle non-alpha8 masked-image direct-draw / row-block specialized helpers | 基础 `RECTANGLE / CIRCLE / ROUND_RECTANGLE / TEXT / IMAGE_565 / EXTERN_IMAGE_565 / IMAGE_RESIZE_565 / EXTERN_IMAGE_RESIZE_565` 不退化；完整 `239` 场景没有任何 `>=10%` 回退；non-alpha8 circle draw 热点仍低于阈值：`MASK_IMAGE_QOI_CIRCLE +2.8%`、`MASK_IMAGE_RLE_CIRCLE +6.5%`、`EXTERN_MASK_IMAGE_QOI_CIRCLE +1.1%`、`EXTERN_MASK_IMAGE_RLE_CIRCLE +3.5%`；alpha8 circle draw 与 raw circle / resize 对照组不退化 | 保留 `EGUI_CONFIG_IMAGE_STD_MASK_CIRCLE_DRAW_RGB565_FAST_PATH_ENABLE`，作为与 `CIRCLE_DRAW_ALPHA8` 互补的 child；若项目要保住 alpha8 circle draw，应优先试这颗宏 |
| `image codec mask-image row-block fast path` | `HelloPerformance text -856B`；主要是 `QOI/RLE` image-mask decode 调用侧 `row_block` 入口里的 `use_image_mask` 分支 | `HelloPerformance` 全量 `239` 场景 on / off 都在噪声内，没有场景超过 `10%`；`MASK_IMAGE_QOI/RLE_*_IMAGE` 与 `EXTERN_MASK_IMAGE_QOI/RLE_*_IMAGE` 当前样本保持 `+0.0%`；runtime `241` 帧 hash / pixel mismatch `0`；`HelloUnitTest 688/688 passed` | 保留 `EGUI_CONFIG_IMAGE_CODEC_MASK_IMAGE_ROW_BLOCK_FAST_PATH_ENABLE` 作为低优先级条件实验宏；它比继续拆更细的 `QOI` dead child 更有效，但收益仍低于当前主推荐门槛 |
| `image rle mask-image row-block fast path` | `HelloPerformance text -108B`；只覆盖 `RLE` image-mask decode 调用侧 `row_block` 入口 | `HelloPerformance` 全量 `239` 场景 on / off 都在噪声内，没有场景超过 `10%`；`MASK_IMAGE_RLE_*_IMAGE` 与 `EXTERN_MASK_IMAGE_RLE_*_IMAGE` 当前样本保持 `+0.0%`；runtime `241` 帧 hash / pixel mismatch `0`；`HelloUnitTest 688/688 passed` | 不保留 `EGUI_CONFIG_IMAGE_RLE_MASK_IMAGE_ROW_BLOCK_FAST_PATH_ENABLE`；独立收益只有 `108B`，代码已回退 |
| `image qoi masked alpha8 split fast path` | `HelloPerformance text 0B`；只覆盖 QOI tail-row-cache 的 masked `RGB565_8` visible-span split-decode 直连分支 | `HelloPerformance` 全量 `239` 场景 on / off 一致，没有场景超过 `10%`；runtime `241` 帧 hash / pixel mismatch `0`；`HelloUnitTest 688/688 passed` | 不保留 `EGUI_CONFIG_IMAGE_QOI_MASKED_ALPHA8_SPLIT_FAST_PATH_ENABLE`；这层没有独立 code-size 回收价值，代码已回退 |
| `image qoi direct alpha8 split fast path` | `HelloPerformance text 0B`；只覆盖 QOI tail-row-cache 的 direct `RGB565_8` visible-span split-decode 直连 blend 分支 | `HelloPerformance` 全量 `239` 场景 on / off 一致，没有场景超过 `10%`；基础 `RECTANGLE / CIRCLE / ROUND_RECTANGLE / TEXT / IMAGE_565 / EXTERN_IMAGE_565 / IMAGE_RESIZE_565 / EXTERN_IMAGE_RESIZE_565` 与 `IMAGE_QOI_565 / EXTERN_IMAGE_QOI_565 / MASK_IMAGE_QOI_8_* / EXTERN_MASK_IMAGE_QOI_8_*` 都保持 `+0.0%`；runtime `241` 帧 hash / pixel mismatch `0`；`HelloUnitTest 688/688 passed` | 不保留 `EGUI_CONFIG_IMAGE_QOI_DIRECT_ALPHA8_SPLIT_FAST_PATH_ENABLE`；这层是 `0B` dead child，代码已回退 |
| `image qoi direct copy split fast path` | `HelloPerformance text 0B`；只覆盖 QOI tail-row-cache 的 direct `RGB565` visible-span split-decode 直连 copy 分支 | `HelloPerformance` 全量 `239` 场景 on / off 一致，没有场景超过 `10%`；基础 `RECTANGLE / CIRCLE / ROUND_RECTANGLE / TEXT / IMAGE_565 / EXTERN_IMAGE_565 / IMAGE_RESIZE_565 / EXTERN_IMAGE_RESIZE_565` 与 `IMAGE_QOI_565 / EXTERN_IMAGE_QOI_565 / MASK_IMAGE_QOI_* / EXTERN_MASK_IMAGE_QOI_*` 都保持 `+0.0%`；runtime `241` 帧 hash / pixel mismatch `0`；`HelloUnitTest 688/688 passed` | 不保留 `EGUI_CONFIG_IMAGE_QOI_DIRECT_COPY_SPLIT_FAST_PATH_ENABLE`；这层也是 `0B` dead child，代码已回退 |
| `image rle checkpoint` | `HelloPerformance text -92B, bss -16B`；只覆盖 RLE row-band checkpoint 的 save/restore 状态 | `HelloPerformance` 全量 `239` 场景里有 `20` 个 `>=10%` 回退；基础 `RECTANGLE / CIRCLE / ROUND_RECTANGLE / TEXT / IMAGE_565 / EXTERN_IMAGE_565 / IMAGE_RESIZE_565 / EXTERN_IMAGE_RESIZE_565` 不退化，但 `IMAGE_RLE_565 +238.4%`、`EXTERN_IMAGE_RLE_565 +452.0%`、`MASK_IMAGE_RLE_NO_MASK +238.4%`、`EXTERN_MASK_IMAGE_RLE_NO_MASK +452.0%`、`IMAGE_RLE_565_8 +109.5%`、`EXTERN_IMAGE_RLE_565_8 +282.1%`；runtime `241` 帧 hash / pixel mismatch `0`；`HelloUnitTest 688/688 passed` | 不建议关闭 `EGUI_CONFIG_IMAGE_RLE_CHECKPOINT_ENABLE`；独立收益极小且会打穿整条 RLE draw 主路径，当前不再沿 `RLE checkpoint` 继续细拆 |
| `image rle direct copy split fast path` | `HelloPerformance text 0B`；只覆盖 RLE tail-row-cache 的 direct `RGB565` visible-span split-decode 直连 copy 分支 | `HelloPerformance` 全量 `239` 场景 on / off 一致，没有场景超过 `10%`；基础 `RECTANGLE / CIRCLE / ROUND_RECTANGLE / TEXT / IMAGE_565 / EXTERN_IMAGE_565 / IMAGE_RESIZE_565 / EXTERN_IMAGE_RESIZE_565` 与 `IMAGE_RLE_565 / EXTERN_IMAGE_RLE_565 / MASK_IMAGE_RLE_* / EXTERN_MASK_IMAGE_RLE_*` 都保持 `+0.0%`；runtime `241` 帧 hash / pixel mismatch `0`；`HelloUnitTest 688/688 passed` | 不保留 `EGUI_CONFIG_IMAGE_RLE_DIRECT_COPY_SPLIT_FAST_PATH_ENABLE`；这层也是 `0B` dead child，代码已回退 |
| `image rle external window persistent cache` | `HelloPerformance text +816B, bss -1040B`；把 external RLE read window 从 persistent BSS 切到 transient frame heap | 完整 `239` 场景没有任何 `>=10%` 波动；基础主路径不退化；`IMAGE_TILED_RLE_565_0 +6.2%`，external RLE 主路径与 masked 变体约 `+2.7% ~ +5.1%`；runtime `241` 帧 hash / pixel mismatch `0`；`HelloUnitTest 688/688 passed` | 可保留 `EGUI_CONFIG_IMAGE_RLE_EXTERNAL_WINDOW_PERSISTENT_CACHE_ENABLE` 作为 low-RAM 条件实验宏；current shipped/default 继续保持 `1`，不进入 text-first split 主线 |
| `std image external row persistent cache` | `HelloPerformance text +20B, bss -40B`；只影响 std external image 的跨帧 row persistent cache storage | `HelloPerformance` 全量 `239` 场景 on / off 都在噪声内，没有场景超过 `10%`；基础 `RECTANGLE / CIRCLE / ROUND_RECTANGLE / TEXT / IMAGE_565 / EXTERN_IMAGE_565 / IMAGE_RESIZE_565 / EXTERN_IMAGE_RESIZE_565` 保持在 `0% ~ +1.7%`，`EXTERN_IMAGE_QOI_565 / EXTERN_IMAGE_RLE_565 / EXTERN_MASK_IMAGE_QOI_IMAGE / EXTERN_MASK_IMAGE_RLE_IMAGE` 也保持 `+0.0%`；runtime `241` 帧 hash / pixel mismatch `0`；`HelloUnitTest 688/688 passed` | 可保留为低优先级 low-RAM 条件实验宏 `EGUI_CONFIG_IMAGE_STD_EXTERNAL_ROW_PERSISTENT_CACHE_ENABLE`；它不属于当前 text-first split 主线，但可以用 `text +20B` 换 `bss -40B` |
| `transform external row persistent cache` | `HelloPerformance text -24B, bss -56B`；只影响 transform/external source 的跨帧 row persistent cache storage | `HelloPerformance` 全量 `239` 场景 on / off 都在噪声内，没有场景超过 `10%`；基础 `RECTANGLE / CIRCLE / ROUND_RECTANGLE / TEXT / IMAGE_565 / EXTERN_IMAGE_565 / IMAGE_RESIZE_565 / EXTERN_IMAGE_RESIZE_565` 保持在 `-0.1% ~ +0.0%`，旋转/变换场景最大也只有 `+0.8%`；runtime `241` 帧 hash / pixel mismatch `0`；`HelloUnitTest 688/688 passed` | 可保留为低优先级 low-RAM 条件实验宏 `EGUI_CONFIG_IMAGE_TRANSFORM_EXTERNAL_ROW_PERSISTENT_CACHE_ENABLE`；它不属于当前 text-first split 主线，但比 std external row persistent 更像“顺手可留”的 RAM 宏 |
| `decode opaque alpha row use row cache` | `HelloPerformance text -132B, bss -8B`；让 masked opaque fallback row 复用 codec row-cache alpha backing store，而不是单独保留 decode-row alpha buffer | `HelloPerformance` 全量 `239` 场景 `ge10=0 / le10=0`，最大绝对波动仅 `0.431%`；基础 `RECTANGLE / CIRCLE / ROUND_RECTANGLE / TEXT / IMAGE_565 / EXTERN_IMAGE_565 / IMAGE_RESIZE_565 / EXTERN_IMAGE_RESIZE_565` 全部 `+0.0%`；runtime `241` 帧 hash / pixel mismatch `0`；`HelloUnitTest 688/688 passed` | current shipped/default 继续保持 `0`；由于收益低于 1KB 门槛，这颗宏已从 public 配置面内收，只保留历史 A/B 记录 |
| `qoi index rgb565 cache` | `HelloPerformance text -64B`；关闭时不再在 QOI index table 里缓存 RGB565 预转换结果，每个活动实例少约 `128B heap` | `HelloPerformance` 全量 `239` 场景 `ge10=0 / le10=0`；基础 `RECTANGLE / CIRCLE / ROUND_RECTANGLE / TEXT / IMAGE_565 / EXTERN_IMAGE_565 / IMAGE_RESIZE_565 / EXTERN_IMAGE_RESIZE_565` 全部 `+0.0%`；QOI 场景最大绝对波动也只在 `+2.8% / -1.7%` 噪声内；runtime `241` 帧 hash / pixel mismatch `0`；`HelloUnitTest 688/688 passed` | current shipped/default 继续保持 `1`；由于收益低于 1KB 门槛，这颗宏也已从 public 配置面内收，只保留历史 A/B 记录 |
| `rle external cache window size` | `HelloPerformance text +96B, bss -1024B`；把 external RLE read window 从默认 `1024B` 缩到 `64B` | 基础 `RECTANGLE / CIRCLE / ROUND_RECTANGLE / TEXT / IMAGE_565 / EXTERN_IMAGE_565 / IMAGE_RESIZE_565 / EXTERN_IMAGE_RESIZE_565` 不退化，但完整 `239` 场景里有 `5` 个 `>=10%` 回退，全部集中在 external RLE 主路径，范围约 `+17.5% ~ +21.8%`；`EXTERN_IMAGE_RLE_565_8 / EXTERN_MASK_IMAGE_RLE_8_*` 也稳定回退 `+9.4%`；runtime `241` 帧 hash / pixel mismatch `0`；`HelloUnitTest 688/688 passed` | current shipped/default 继续保持 `1024`；`64` 仍可保留为高收益 low-RAM 条件实验值，但只适合明确不关心 external RLE 热点的项目 |
| `qoi checkpoint count` | `HelloPerformance text -276B, bss -8B`；把 QOI decoder checkpoints 从默认 `2` 降到 `0` | 基础 `RECTANGLE / CIRCLE / ROUND_RECTANGLE / TEXT / IMAGE_565 / EXTERN_IMAGE_565 / IMAGE_RESIZE_565 / EXTERN_IMAGE_RESIZE_565` 不退化，但完整 `239` 场景里有 `20` 个 `>=10%` 回退，全部集中在 QOI draw / mask 主路径，范围约 `+110.0% ~ +911.4%`；runtime `241` 帧 hash / pixel mismatch `0`；`HelloUnitTest 688/688 passed` | 不建议把 `EGUI_CONFIG_IMAGE_QOI_CHECKPOINT_COUNT` 设为 `0`；current shipped/default 继续保持 `2`，历史 low-RAM 值只保留为外部条件实验入口 |
| `image qoi checkpoint` | `HelloPerformance text -276B, bss -8B`；只覆盖 QOI row-band checkpoint 的 save/restore 与 checkpoint table 管理 | `HelloPerformance` 全量 `239` 场景里有 `20` 个 `>=10%` 回退；基础 `RECTANGLE / CIRCLE / ROUND_RECTANGLE / TEXT / IMAGE_565 / EXTERN_IMAGE_565 / IMAGE_RESIZE_565 / EXTERN_IMAGE_RESIZE_565` 不退化，但 `IMAGE_QOI_565 +819.2%`、`EXTERN_IMAGE_QOI_565 +911.4%`、`MASK_IMAGE_QOI_NO_MASK +819.2%`、`EXTERN_MASK_IMAGE_QOI_NO_MASK +911.4%`、`MASK_IMAGE_QOI_8_IMAGE +110.0%`；runtime `241` 帧 hash / pixel mismatch `0`；`HelloUnitTest 688/688 passed` | 不保留临时 `EGUI_CONFIG_IMAGE_QOI_CHECKPOINT_ENABLE`；独立收益太小且会打穿整条 QOI draw 主路径，代码已回退 |
| rotated-text visible alpha8 fast path | `text_transform_draw_visible_alpha8_tile_layout` 当前约 `8128B` | 历史 A/B 显示 `5120` 相比 `4096` 时 `TEXT_ROTATE_BUFFERED -25.7%`、`EXTERN_TEXT_ROTATE_BUFFERED -28.2%`；`2560B` 已被拒绝 | 保留 fast path，本体不拆；仅把 low-RAM heap ceiling 继续放尾部按需块 |
| `shadow d_sq -> alpha LUT` 策略 | `egui_shadow_draw_corner` 当前约 `1980B`；本质是 stack/LUT 上限策略，不是常规小项目 code-size 宏 | `256 -> 64` 时 `SHADOW_ROUND 6.306 -> 4.949 (-21.5%)`，其它锚点基本不动 | 继续放 `app_egui_config.h` 尾部 `#if 0` low-RAM 块，默认不打开 |
| round-rect / circle `PFB_HEIGHT` row cache | 已有历史结论：关闭后 `text -708B`、`static RAM -16B` | 关闭后无 `>5%` 回退，且 `MASK_IMAGE_QOI_8_ROUND_RECT`、`MASK_IMAGE_RLE_8_ROUND_RECT` 还变快 | 已处理完，默认关闭，不再作为 active override |
| `alpha opaque slots` | `HelloPerformance text -156B, bss -40B`；关闭后不再缓存 RGB565+alpha source 是否全 opaque 的探测结果 | 完整 `239` 场景 `ge10=0 / le10=1`，只有 `EXTERN_IMAGE_RESIZE_TILED_565_8 -11.4%` 这一处 `<=-10%` 改善；但 runtime `4 vs 0` 仍有 `8` 帧真实像素差，`4 vs 4 repeat` 控制组 hash / pixel mismatch `0`；`HelloUnitTest 688/688 passed` | current shipped/default 继续保持实现私有默认 `4`；由于 `4 -> 0` 只回收 `text -156B, bss -40B`，远低于 1KB 门槛，而且 `0` 路径不再 render-equivalent，这颗宏已从 public 配置面内收，只保留历史 A/B 记录 |
| `IMAGE_CODEC_ROW_CACHE_ENABLE` | 主要影响 heap，不是 small-project code-size toggle | `row-cache off` 会把 QOI/RLE 热点打到 `+1800% ~ +3600%` 量级 | 保留 |

### 矩阵补充

- `canvas masked-fill shape child fast path`
  - 代码证据：`MASK_FILL_CIRCLE text -3648B`；继续细拆 `CIRCLE_SEGMENT child -3328B`；`MASK_FILL_ROUND_RECT text -876B`
  - HelloPerformance 证据：基础 `RECTANGLE / CIRCLE / ROUND_RECTANGLE / TEXT / IMAGE_565 / EXTERN_IMAGE_565` 都维持在噪声范围；`MASK_FILL_CIRCLE` 主要拖慢 circle masked-fill；`CIRCLE_SEGMENT child` 当前完整 `239` 场景 perf / runtime / unit 全量等价；`MASK_FILL_ROUND_RECT` 主要拖慢 round-rect masked-fill
  - 结论：保留 `EGUI_CONFIG_CANVAS_MASK_FILL_CIRCLE_FAST_PATH_ENABLE`，并优先保留更保守的 `EGUI_CONFIG_CANVAS_MASK_FILL_CIRCLE_SEGMENT_FAST_PATH_ENABLE`；`EGUI_CONFIG_CANVAS_MASK_FILL_ROUND_RECT_FAST_PATH_ENABLE` 的推荐优先级继续下调，因为它当前不足 `1KB`
- `canvas masked-fill circle segment fast path`
  - 代码证据：`2026-04-05` current-mainline `HelloPerformance text -3328B`
  - HelloPerformance 证据：基础 `RECTANGLE / CIRCLE / ROUND_RECTANGLE / TEXT / IMAGE_565 / EXTERN_IMAGE_565 / IMAGE_RESIZE_565 / EXTERN_IMAGE_RESIZE_565` 与当前 `MASK_RECT_FILL_CIRCLE / MASK_RECT_FILL_ROUND_RECT / MASK_RECT_FILL_IMAGE / MASK_GRADIENT_RECT_FILL / MASK_ROUND_RECT_FILL_WITH_MASK` 都在噪声范围；完整 `239` 场景 perf、runtime `241` 帧和 `HelloUnitTest 688/688` 全量等价
  - 结论：保留 `EGUI_CONFIG_CANVAS_MASK_FILL_CIRCLE_SEGMENT_FAST_PATH_ENABLE`，作为比 `MASK_FILL_CIRCLE` 更保守的 child；若项目要保住当前 circle masked-fill 主路径，应优先试它
- `canvas masked-fill shape umbrella fast path`
  - 代码量证据：当前树最终 `HelloPerformance text +0B`；带 suffix 的对象级 shrink 不能转化成最终 ELF 收益
  - HelloPerformance 证据：基础 `RECTANGLE / CIRCLE / ROUND_RECTANGLE / TEXT / IMAGE_565 / EXTERN_IMAGE_565` 不动；但 `MASK_RECT_FILL_CIRCLE +37.0%`，`MASK_RECT_FILL_ROUND_RECT` / `MASK_ROUND_RECT_FILL_WITH_MASK +13.0%`
  - 结论：直接拒绝 `EGUI_CONFIG_CANVAS_MASK_SHAPE_FILL_FAST_PATH_ENABLE`，继续只保留 child 宏
- `canvas masked-fill auxiliary fast path`
  - 代码证据：`ROW_BLEND text -2948B`；`IMAGE text -1860B`；继续细拆 `IMAGE_SCALE child -196B`；`ROW_RANGE text -8028B`；`ROW_PARTIAL text -2928B`；`ROW_INSIDE text -1664B`
  - HelloPerformance 证据：基础路径都在噪声范围；`ROW_BLEND` 明显伤 gradient masked-fill；`IMAGE` 明显伤 image masked-fill；`IMAGE_SCALE child` 的 `239` 场景 perf / runtime / unit 全量等价；`2026-04-05` current-mainline 复验继续确认 `ROW_RANGE / ROW_PARTIAL / ROW_INSIDE` 的完整 `239` 场景都没有任何 `>=10%` 波动，基础 `RECTANGLE / CIRCLE / ROUND_RECTANGLE / TEXT / IMAGE_565 / EXTERN_IMAGE_565 / IMAGE_RESIZE_565 / EXTERN_IMAGE_RESIZE_565` 与 `MASK_GRADIENT_RECT_FILL / MASK_RECT_FILL_IMAGE / MASK_RECT_FILL_ROUND_RECT / MASK_RECT_FILL_CIRCLE / MASK_ROUND_RECT_FILL_WITH_MASK` 都在噪声范围
  - 运行/单测：`ROW_RANGE`、`ROW_PARTIAL` 与 `ROW_INSIDE` 的 on / off 都是 `241` 帧，`hash mismatch 0`、`pixel mismatch 0`；HelloUnitTest on / off 都是 `688/688 passed`
  - 结论：保留 `EGUI_CONFIG_CANVAS_MASK_FILL_ROW_BLEND_FAST_PATH_ENABLE` / `EGUI_CONFIG_CANVAS_MASK_FILL_IMAGE_FAST_PATH_ENABLE` / `EGUI_CONFIG_CANVAS_MASK_FILL_ROW_RANGE_FAST_PATH_ENABLE` / `EGUI_CONFIG_CANVAS_MASK_FILL_ROW_PARTIAL_FAST_PATH_ENABLE` / `EGUI_CONFIG_CANVAS_MASK_FILL_ROW_INSIDE_FAST_PATH_ENABLE`；不保留 `EGUI_CONFIG_CANVAS_MASK_FILL_IMAGE_SCALE_FAST_PATH_ENABLE`，其中 `ROW_RANGE`、`ROW_PARTIAL`、`ROW_INSIDE` 仍是当前更值得优先尝试的 size-first 子宏
- `canvas masked-fill visible-range call-site split`
  - 临时 split：`EGUI_CONFIG_CANVAS_MASK_FILL_VISIBLE_RANGE_FAST_PATH_ENABLE`
  - 代码量证据：`HelloPerformance text +1552B`
  - HelloPerformance 证据：基础 `RECTANGLE / CIRCLE / ROUND_RECTANGLE / TEXT / IMAGE_565 / EXTERN_IMAGE_565` 与当前抽样 `MASK_GRADIENT_RECT_FILL / MASK_RECT_FILL_ROUND_RECT / MASK_ROUND_RECT_FILL_WITH_MASK / MASK_RECT_FILL_CIRCLE / MASK_RECT_FILL_IMAGE` 基本都在噪声范围
  - 运行/单测：HelloPerformance baseline / off 都 `ALL PASSED`；HelloUnitTest baseline / off 都 `688/688 passed`
  - 结论：这是负收益 split，直接拒绝，不保留这颗调用侧宏
- `mask-image identity-scale fast path`
  - 代码证据：umbrella `HelloPerformance text -3272B`；`2026-04-05` current-mainline 复验里 `BLEND child` 仍是 `text -2612B`，`FILL child -660B`
  - HelloPerformance 证据：完整 `239` 场景里基础 `RECTANGLE / CIRCLE / ROUND_RECTANGLE / TEXT / IMAGE_565 / EXTERN_IMAGE_565 / IMAGE_RESIZE_565 / EXTERN_IMAGE_RESIZE_565` 不动；`BLEND child` 的 `9` 个 `>=10%` 回退全部集中在 identity-scale masked-image blend 热点，`FILL` 仍只伤 `MASK_RECT_FILL_IMAGE`
  - 运行/单测：HelloPerformance on / off 都是 `241` 帧，`hash mismatch 0`、`pixel mismatch 0`；HelloUnitTest on / off 都是 `688/688 passed`
  - 结论：保留 `EGUI_CONFIG_MASK_IMAGE_IDENTITY_SCALE_FAST_PATH_ENABLE`，并继续保留 `EGUI_CONFIG_MASK_IMAGE_IDENTITY_SCALE_BLEND_FAST_PATH_ENABLE`；`FILL` child 因只有 `660B` 被拒绝
- `std image mask visible-range fast path`
  - 代码量证据：`HelloPerformance text -1416B`
  - HelloPerformance 证据：基础 `RECTANGLE / CIRCLE / ROUND_RECTANGLE / TEXT / IMAGE_565 / EXTERN_IMAGE_565` 不动；大多数 `masked-image` 场景不动，但 `EXTERN_MASK_IMAGE_TEST_PERF_CIRCLE` 明显回退；继续细拆后的 `VISIBLE_RANGE_RESIZE` child 只有 `-820B`，且会让 `EXTERN_MASK_IMAGE_TEST_PERF_CIRCLE +96.7%`、`EXTERN_MASK_IMAGE_TEST_PERF_IMAGE +58.9%`
  - 结论：保留 `EGUI_CONFIG_IMAGE_STD_MASK_VISIBLE_RANGE_FAST_PATH_ENABLE`，作为 partial-row external masked-image 热点可接受时的条件实验宏；`VISIBLE_RANGE_RESIZE` child 已拒绝
- `std image mask row-range fast path`
  - 代码量证据：`HelloPerformance text -10248B`
  - HelloPerformance 证据：基础 `RECTANGLE / CIRCLE / ROUND_RECTANGLE / TEXT / IMAGE_565 / EXTERN_IMAGE_565 / IMAGE_RESIZE_565 / EXTERN_IMAGE_RESIZE_565` 不动；但 `MASK_IMAGE_TEST_PERF_CIRCLE +701.9%`、`EXTERN_MASK_IMAGE_TEST_PERF_CIRCLE +468.0%`、`EXTERN_MASK_IMAGE_TEST_PERF_ROUND_RECT +941.7%`
  - 结论：保留 `EGUI_CONFIG_IMAGE_STD_MASK_ROW_RANGE_FAST_PATH_ENABLE`，作为比 child 更激进的 umbrella 条件实验宏；若要先试更可控的关法，优先 `ROW_PARTIAL_DRAW / ROW_PARTIAL_RESIZE`，再考虑 `ROW_PARTIAL`
- `std image mask row-partial fast path`
  - 代码量证据：`HelloPerformance text -7600B`
  - HelloPerformance 证据：基础 `RECTANGLE / CIRCLE / ROUND_RECTANGLE / TEXT / IMAGE_565 / EXTERN_IMAGE_565 / IMAGE_RESIZE_565 / EXTERN_IMAGE_RESIZE_565` 不动；但 `MASK_IMAGE_TEST_PERF_CIRCLE`、`EXTERN_MASK_IMAGE_TEST_PERF_CIRCLE`、`EXTERN_MASK_IMAGE_TEST_PERF_ROUND_RECT` 明显回退
  - 结论：保留 `EGUI_CONFIG_IMAGE_STD_MASK_ROW_PARTIAL_FAST_PATH_ENABLE`，作为“只牺牲 partial-row masked-image 热点”的条件实验宏；若要先试更保守的 child，优先 `ROW_PARTIAL_DRAW / ROW_PARTIAL_RESIZE`
- `std image mask row-partial draw fast path`
  - 代码量证据：`HelloPerformance text -2360B`
  - HelloPerformance 证据：基础主路径、`IMAGE_565_1/2/4/8` / `EXTERN_IMAGE_565_1/2/4/8` 与 `MASK_IMAGE_TEST_PERF_*` 对照组都维持在噪声范围；当前 239 场景没有出现 `>=10%` 回退
  - 运行证据：baseline / off `241` 帧，hash / pixel mismatch `0`
  - 结论：保留 `EGUI_CONFIG_IMAGE_STD_MASK_ROW_PARTIAL_DRAW_FAST_PATH_ENABLE`，作为比 `ROW_PARTIAL` 更保守的 draw-side child；如果项目要先保住 partial-row resize / sampled partial-row 场景，应先试它，但 raw alpha masked direct-draw partial-row 热点仍建议补专项 perf
- `std image mask row-partial draw alpha8 fast path`
  - 代码量证据：`HelloPerformance text -1096B`
  - HelloPerformance 证据：基础主路径、`IMAGE_565_8 / EXTERN_IMAGE_565_8` 与 `MASK_IMAGE_QOI_8_CIRCLE / MASK_IMAGE_RLE_8_CIRCLE / EXTERN_MASK_IMAGE_QOI_8_CIRCLE / EXTERN_MASK_IMAGE_RLE_8_CIRCLE` 都维持在噪声范围；当前 239 场景没有出现 `>=10%` 回退
  - 运行证据：baseline / off `241` 帧，hash / pixel mismatch `0`
  - 结论：保留 `EGUI_CONFIG_IMAGE_STD_MASK_ROW_PARTIAL_DRAW_ALPHA8_FAST_PATH_ENABLE`，作为比 `ROW_PARTIAL_DRAW` 更保守的 alpha8 child；如果项目要先保住 packed-alpha partial-row draw，应先试它，但 raw alpha8 masked direct-draw partial-row 热点仍建议补专项 perf
- `std image mask row-partial draw packed-alpha fast path`
  - 代码量证据：`HelloPerformance text -1360B`
  - HelloPerformance 证据：基础主路径、`IMAGE_565_1/2/4 / EXTERN_IMAGE_565_1/2/4` 与 `MASK_IMAGE_QOI_CIRCLE / MASK_IMAGE_RLE_CIRCLE / EXTERN_MASK_IMAGE_QOI_CIRCLE / EXTERN_MASK_IMAGE_RLE_CIRCLE` 都维持在噪声范围；当前 239 场景没有出现 `>=10%` 回退
  - 运行证据：baseline / off `241` 帧，hash / pixel mismatch `0`
  - 结论：保留 `EGUI_CONFIG_IMAGE_STD_MASK_ROW_PARTIAL_DRAW_PACKED_ALPHA_FAST_PATH_ENABLE`，作为比 `ROW_PARTIAL_DRAW` 更保守的 packed child；如果项目要先保住 alpha8 partial-row draw，应先试它，但 raw packed-alpha masked direct-draw partial-row 热点仍建议补专项 perf
- `std image mask row-partial resize packed-alpha fast path`
  - 代码量证据：`HelloPerformance text -2436B`
  - HelloPerformance 证据：基础主路径、`IMAGE_RESIZE_565_1/2/4 / EXTERN_IMAGE_RESIZE_565_1/2/4` 与父宏里显著回退的 `MASK_IMAGE_CIRCLE / EXTERN_MASK_IMAGE_CIRCLE / MASK_IMAGE_TEST_PERF_CIRCLE / EXTERN_MASK_IMAGE_TEST_PERF_CIRCLE / EXTERN_MASK_IMAGE_TEST_PERF_ROUND_RECT` 都回到噪声范围；当前 239 场景最大绝对波动只有 `+1.3%`
  - 运行证据：baseline / off `241` 帧，hash / pixel mismatch `0`
  - 结论：保留 `EGUI_CONFIG_IMAGE_STD_MASK_ROW_PARTIAL_RESIZE_PACKED_ALPHA_FAST_PATH_ENABLE`，作为比 `ROW_PARTIAL_RESIZE` 更保守的 packed child；如果项目要先保住 alpha8 / raw sampled partial-row resize，应先试它，但 raw packed-alpha masked sampled resize partial-row 热点仍建议补专项 perf
- `std image mask row-partial resize rgb565 fast path`
  - 代码量证据：`HelloPerformance text -2936B`
  - HelloPerformance 证据：基础主路径、`IMAGE_RESIZE_565_1/2/4 / EXTERN_IMAGE_RESIZE_565_1/2/4` 与 `MASK_IMAGE_CIRCLE / EXTERN_MASK_IMAGE_CIRCLE` 不动；但 raw sampled masked-image `TEST_PERF_*` 热点明显回退
  - 运行证据：baseline / off `241` 帧，但 `frame_0190` 有 `1` 帧真实像素差异
  - 结论：仅保留 `EGUI_CONFIG_IMAGE_STD_MASK_ROW_PARTIAL_RESIZE_RGB565_FAST_PATH_ENABLE` 的实验记录；如果项目明确不依赖 raw sampled masked-image `TEST_PERF_*` 热点，且能接受 `1` 帧像素差，可再单独试它；默认不建议优先于 `ROW_PARTIAL_RESIZE_PACKED_ALPHA`
- `std image mask row-partial resize alpha8 fast path`
  - 代码量证据：`HelloPerformance text -1000B`
  - HelloPerformance 证据：基础 `RECTANGLE / CIRCLE / ROUND_RECTANGLE / TEXT / IMAGE_565 / EXTERN_IMAGE_565 / IMAGE_RESIZE_565 / EXTERN_IMAGE_RESIZE_565` 与 raw sampled resize 对照组不动；代价只落在 alpha8 circle resize 热点
  - 运行证据：baseline / off 有 `4` 帧细微 circle 边缘像素差
  - 结论：仅保留 `EGUI_CONFIG_IMAGE_STD_MASK_ROW_PARTIAL_RESIZE_ALPHA8_FAST_PATH_ENABLE` 的实验记录；如果项目更想保住 packed sampled resize，且能接受 alpha8 circle 的 `4` 帧细微差异，可在 `ROW_PARTIAL_RESIZE_PACKED_ALPHA` 之后再试它
- `std image mask row-partial resize generic fast path`
  - 代码量证据：`HelloPerformance text 0B`
  - HelloPerformance 证据：`239` 场景 perf on / off 全量一致
  - 运行证据：baseline / off `241` 帧，hash / pixel mismatch `0`
  - 结论：证明 generic sampled resize partial-row 这层没有独立回收价值，不保留 `EGUI_CONFIG_IMAGE_STD_MASK_ROW_PARTIAL_RESIZE_GENERIC_FAST_PATH_ENABLE`
- `std image mask row-inside resize fast path`
  - 代码量证据：`HelloPerformance text -928B`
  - HelloPerformance 证据：基础 `RECTANGLE / CIRCLE / ROUND_RECTANGLE / TEXT / IMAGE_565 / EXTERN_IMAGE_565 / IMAGE_RESIZE_565 / EXTERN_IMAGE_RESIZE_565` 不动；sampled non-test masked-image resize 场景维持在噪声范围；但 sampled inside-row resize 热点明显回退
  - 结论：保留 `EGUI_CONFIG_IMAGE_STD_MASK_ROW_INSIDE_RESIZE_FAST_PATH_ENABLE`，作为比 `ROW_INSIDE` 更保守的条件实验宏；如果项目要保住 raw sampled inside-row resize，应先试它的 alpha8 child
- `std image mask row-inside resize alpha8 fast path`
  - 代码量证据：`HelloPerformance text -712B`
  - HelloPerformance 证据：基础主路径与 raw sampled inside-row resize 对照组不动；代价只落在 alpha8 circle resize 热点
  - 运行证据：baseline / off `241` 帧，hash / pixel mismatch `0`
  - 结论：保留 `EGUI_CONFIG_IMAGE_STD_MASK_ROW_INSIDE_RESIZE_ALPHA8_FAST_PATH_ENABLE` 的实验记录；如果项目要尽量保住 raw inside-row resize，可先试它，再试 `ROW_INSIDE_RESIZE`
- `std image mask row-inside fast path`
  - 代码量证据：`HelloPerformance text -1340B`
  - HelloPerformance 证据：基础 `RECTANGLE / CIRCLE / ROUND_RECTANGLE / TEXT / IMAGE_565 / EXTERN_IMAGE_565 / IMAGE_RESIZE_565 / EXTERN_IMAGE_RESIZE_565` 不动；但 `MASK_IMAGE_TEST_PERF_CIRCLE`、`EXTERN_MASK_IMAGE_TEST_PERF_CIRCLE`、`EXTERN_MASK_IMAGE_TEST_PERF_ROUND_RECT` 明显回退，且 `EXTERN_MASK_IMAGE_TEST_PERF_ROUND_RECT` 代价尤其大
  - 结论：保留 `EGUI_CONFIG_IMAGE_STD_MASK_ROW_INSIDE_FAST_PATH_ENABLE`，作为更保守但优先级更低的条件实验宏
- `std image external alpha fast path`
  - 代码证据：`HelloPerformance text -4056B`
  - HelloPerformance 证据：`IMAGE_565 +0.0%`、`EXTERN_IMAGE_565 -0.7%`、`IMAGE_RESIZE_565 +0.0%`、`EXTERN_IMAGE_RESIZE_565 +0.2%`
  - 结论：保留 `EGUI_CONFIG_IMAGE_STD_EXTERNAL_ALPHA_FAST_PATH_ENABLE`，作为“只牺牲 external alpha specialized path”的实验型开关
- `std image row-overlay non-rgb565 fast path`
  - 代码证据：`HelloPerformance text -1476B`；继续细拆后 `ALPHA8 child -1264B`，generic `GET_PIXEL child` 只有 `-188B`
  - HelloPerformance 证据：`2026-04-05` current-mainline 完整 `239` 场景里没有任何 `>=10%` 回退；基础主路径和当前 overlay 对照组都在噪声范围，`IMAGE_GRADIENT_OVERLAY +0.1%`
  - 运行/单测：HelloPerformance on / off 都是 `241` 帧，`hash mismatch 0`、`pixel mismatch 0`；HelloUnitTest on / off 都是 `688/688 passed`
  - 结论：保留 `EGUI_CONFIG_IMAGE_STD_ROW_OVERLAY_NON_RGB565_FAST_PATH_ENABLE`，同时继续保留更保守的 `ROW_OVERLAY_ALPHA8` child；generic `GET_PIXEL` child 不保留
- `std image row-overlay alpha8 fast path`
  - 代码证据：`HelloPerformance text -1264B`
  - HelloPerformance 证据：基础主路径和当前 `IMAGE_GRADIENT_OVERLAY / MASK_GRADIENT_IMAGE / MASK_GRADIENT_IMAGE_ROTATE` 不动；代价主要落在 `IMAGE_565_8_DOUBLE`
  - 结论：保留 `EGUI_CONFIG_IMAGE_STD_ROW_OVERLAY_ALPHA8_FAST_PATH_ENABLE`，作为比 `ROW_OVERLAY_NON_RGB565` 更保守的 child；若项目要保住 generic `get_pixel` overlay，应先试这颗宏
- `std image row-overlay rgb565 fast path`
  - 代码证据：`HelloPerformance text -1312B`
  - HelloPerformance 证据：基础主路径和 non-raw overlay 对照组都在噪声范围；代价只落在 raw `IMAGE_GRADIENT_OVERLAY`
  - 结论：保留 `EGUI_CONFIG_IMAGE_STD_ROW_OVERLAY_RGB565_FAST_PATH_ENABLE`，作为与 `ROW_OVERLAY_NON_RGB565` / `ROW_OVERLAY_ALPHA8` 互补的 child；如果项目已经先保住 non-raw overlay，只剩 raw gradient-image overlay 可以牺牲，再试这颗 child
- `std image external alpha8 fast path`
  - 代码证据：`HelloPerformance text -588B`
  - HelloPerformance 证据：基础 `IMAGE_565 / EXTERN_IMAGE_565 / IMAGE_RESIZE_565 / EXTERN_IMAGE_RESIZE_565` 不动；但 `EXTERN_IMAGE_565_8 -86.2%`、`EXTERN_IMAGE_RESIZE_565_8 -84.2%`
  - 结论：不进入当前优先保留宏序列；收益低于 `1KB`
- `std image external packed-alpha fast path`
  - 代码证据：`HelloPerformance text -2816B`
  - HelloPerformance 证据：`IMAGE_565 +0.0%`、`EXTERN_IMAGE_565 +0.0%`、`IMAGE_RESIZE_565 +0.0%`、`EXTERN_IMAGE_RESIZE_565 -0.2%`、`EXTERN_IMAGE_565_8 +0.0%`
  - 结论：保留 `EGUI_CONFIG_IMAGE_STD_EXTERNAL_PACKED_ALPHA_FAST_PATH_ENABLE`，作为“只牺牲 external packed-alpha specialized path”的细粒度实验型开关
- `std image external packed-alpha resize fast path`
  - 代码证据：`HelloPerformance text -2796B`；draw-only split 只有 `-180B`
  - HelloPerformance 证据：`IMAGE_565 +0.0%`、`EXTERN_IMAGE_565 +0.0%`、`IMAGE_RESIZE_565 +0.0%`、`EXTERN_IMAGE_RESIZE_565 +0.0%`、`EXTERN_IMAGE_565_1/2/4 +0.0% ~ +1.6%`；但 `EXTERN_IMAGE_RESIZE_565_1/2/4 -84.1% ~ -87.2%`
  - 结论：保留 `EGUI_CONFIG_IMAGE_STD_EXTERNAL_PACKED_ALPHA_RESIZE_FAST_PATH_ENABLE`，作为“只牺牲 external RGB565_1/2/4 resize”的更细粒度实验型开关；draw-only child 不保留
- `std image packed-alpha direct-draw fast path`
  - 代码证据：`HelloPerformance text -5548B`
  - HelloPerformance 证据：`IMAGE_565 +0.0%`、`EXTERN_IMAGE_565 -0.5%`、`IMAGE_565_8 -0.4%`；但 internal `IMAGE_565_1/2/4` 分别回退 `+559.6%`、`+475.9%`、`+318.4%`
  - 结论：保留 `EGUI_CONFIG_IMAGE_STD_FAST_DRAW_PACKED_ALPHA_ENABLE`，作为“只牺牲 internal RGB565_1/2/4 direct draw”的实验型开关
- `std image alpha8 direct-draw fast path`
  - 代码证据：`HelloPerformance text -1748B`
  - HelloPerformance 证据：`IMAGE_565 +0.0%`、`EXTERN_IMAGE_565 +0.0%`；但 internal `IMAGE_565_8 +348.4%`
  - 结论：保留 `EGUI_CONFIG_IMAGE_STD_FAST_DRAW_ALPHA8_ENABLE`，作为“只牺牲 internal RGB565_8 direct draw”的实验型开关
- `std image alpha-color fast path`
  - 代码证据：umbrella `HelloPerformance text -1248B`；继续细拆后 `DRAW -52B`、`RESIZE -184B`
  - HelloPerformance 证据：基础 `RECTANGLE / CIRCLE / ROUND_RECTANGLE / TEXT / IMAGE_565 / EXTERN_IMAGE_565` 不动；umbrella 关闭时 `IMAGE_COLOR +362.9%`、`IMAGE_RESIZE_COLOR +395.7%`；继续细拆后 `DRAW` 只伤 `IMAGE_COLOR`，`RESIZE` 只伤 `IMAGE_RESIZE_COLOR`
  - 结论：保留 `EGUI_CONFIG_IMAGE_STD_ALPHA_COLOR_FAST_PATH_ENABLE`，作为 alpha-only tinted image 热点可接受时的条件实验宏；`DRAW/RESIZE` child 因为只有 `52B/184B` 被拒绝
- `std image packed-alpha resize fast path`
  - 代码证据：`HelloPerformance text -7512B`
  - HelloPerformance 证据：`IMAGE_565 -0.4%`、`EXTERN_IMAGE_565 -1.0%`、`IMAGE_RESIZE_565 +0.0%`、`EXTERN_IMAGE_RESIZE_565 +0.0%`、`IMAGE_RESIZE_565_8 -1.6%`；但 internal `IMAGE_RESIZE_565_1/2/4` 分别回退 `+370.4%`、`+297.5%`、`+293.9%`
  - 结论：保留 `EGUI_CONFIG_IMAGE_STD_FAST_RESIZE_PACKED_ALPHA_ENABLE`，作为“只牺牲 internal RGB565_1/2/4 resize”的实验型开关
- `std image alpha8 resize fast path`
  - 代码证据：`HelloPerformance text -3756B`
  - HelloPerformance 证据：`IMAGE_565 +0.0%`、`EXTERN_IMAGE_565 +0.0%`、`IMAGE_RESIZE_565 +0.2%`、`EXTERN_IMAGE_RESIZE_565 +0.0%`；但 internal `IMAGE_RESIZE_565_8 +300.0%`
  - 结论：保留 `EGUI_CONFIG_IMAGE_STD_FAST_RESIZE_ALPHA8_ENABLE`，作为“只牺牲 internal RGB565_8 resize”的实验型开关
- `std image mask-shape fast path`
  - 代码证据：`HelloPerformance text -13032B`
  - HelloPerformance 证据：`IMAGE_565 -0.4%`、`EXTERN_IMAGE_565 -1.0%`、`IMAGE_RESIZE_565 -0.2%`、`EXTERN_IMAGE_RESIZE_565 +0.0%`
  - 结论：保留 `EGUI_CONFIG_IMAGE_STD_MASK_SHAPE_FAST_PATH_ENABLE`，作为“只牺牲 circle / round-rectangle masked-image specialized path”的实验型开关
- `std image mask-shape child fast path`
  - 代码证据：`MASK_CIRCLE text -7368B`；`MASK_ROUND_RECT text -9540B`
  - HelloPerformance 证据：基础 `RECTANGLE / CIRCLE / ROUND_RECTANGLE / TEXT / IMAGE_565 / EXTERN_IMAGE_565 / IMAGE_RESIZE_565 / EXTERN_IMAGE_RESIZE_565` 都维持在噪声范围；`MASK_CIRCLE` 主要拖慢 circle masked-image，`MASK_ROUND_RECT` 主要拖慢 round-rect masked-image
  - 结论：保留 `EGUI_CONFIG_IMAGE_STD_MASK_CIRCLE_FAST_PATH_ENABLE` / `EGUI_CONFIG_IMAGE_STD_MASK_ROUND_RECT_FAST_PATH_ENABLE`，其中 `MASK_ROUND_RECT` 更适合作为优先尝试关闭的 size-first 子宏
- `std image mask-shape resize child fast path`
  - 代码证据：`MASK_ROUND_RECT_RESIZE text -3764B`；`MASK_CIRCLE_RESIZE text -1984B`
  - HelloPerformance 证据：基础主路径都在噪声范围；`2026-04-05` current-mainline 复验继续确认 `ROUND_RECT_RESIZE` 完整 `239` 场景里只有 `MASK_IMAGE_ROUND_RECT +10.8%`、`MASK_IMAGE_ROUND_RECT_DOUBLE +10.4%` 两个 `>=10%` 回退，`QOI / RLE / external` round-rect 对照组维持不变；`CIRCLE_RESIZE` 历史结论仍是只伤 internal circle masked-image resize
  - 结论：保留 `EGUI_CONFIG_IMAGE_STD_MASK_ROUND_RECT_RESIZE_FAST_PATH_ENABLE` / `EGUI_CONFIG_IMAGE_STD_MASK_CIRCLE_RESIZE_FAST_PATH_ENABLE`，其中 round-rect resize 这条线继续保留互补的 `ROUND_RECT_RESIZE_ALPHA8` / `ROUND_RECT_RESIZE_RGB565` 两颗 child；当项目要保住对应 shape 的 draw / codec / external 热点时，优先试这些 resize child，再考虑关闭父宏
- `std image mask circle resize rgb565 fast path`
  - 代码证据：`2026-04-05` current-mainline `HelloPerformance text -1456B`
  - HelloPerformance 证据：基础 `RECTANGLE / CIRCLE / ROUND_RECTANGLE / TEXT / IMAGE_565 / EXTERN_IMAGE_565 / IMAGE_RESIZE_565 / EXTERN_IMAGE_RESIZE_565` 不退化；`MASK_IMAGE_CIRCLE / EXTERN_MASK_IMAGE_CIRCLE / MASK_IMAGE_CIRCLE_QUARTER / MASK_IMAGE_CIRCLE_DOUBLE` 与 alpha8 circle resize 对照组不退化；但 `MASK_IMAGE_TEST_PERF_CIRCLE +36.2%`
  - 结论：只保留 `EGUI_CONFIG_IMAGE_STD_MASK_CIRCLE_RESIZE_RGB565_FAST_PATH_ENABLE` 的实验记录；如果项目更想保住 alpha8 circle resize，且明确不依赖 raw sampled circle resize hotspot、并能接受 `frame_0190.png` 的 `1` 帧像素差，可在父宏之前单独试它
- `std image mask round-rect resize rgb565 fast path`
  - 代码证据：`2026-04-05` current-mainline `HelloPerformance text -2472B`
  - HelloPerformance 证据：基础 `RECTANGLE / CIRCLE / ROUND_RECTANGLE / TEXT / IMAGE_565 / EXTERN_IMAGE_565 / IMAGE_RESIZE_565 / EXTERN_IMAGE_RESIZE_565` 不退化；完整 `239` 场景没有任何 `>=10%` 回退；raw round-rect resize 样本里最高也只有 `MASK_IMAGE_TEST_PERF_ROUND_RECT +9.7%`，同时 alpha8 round-rect resize 与 draw / codec / external 对照组继续稳定
  - 结论：保留 `EGUI_CONFIG_IMAGE_STD_MASK_ROUND_RECT_RESIZE_RGB565_FAST_PATH_ENABLE`，作为与 `ROUND_RECT_RESIZE_ALPHA8` 互补的 child；若项目要保住 alpha8 round-rect resize，应优先试它
- `std image mask round-rect draw child fast path`
  - 代码证据：`2026-04-05` current-mainline `MASK_ROUND_RECT_DRAW text -5804B`；继续细拆后 `ROUND_RECT_DRAW_ALPHA8 child -3960B`、`ROUND_RECT_DRAW_RGB565 child -2216B`
  - HelloPerformance 证据：基础主路径、internal round-rect resize 样本和 circle 对照组都在噪声范围；`2026-04-05` current-mainline 完整 `239` 场景里 `4` 个 `>=10%` 回退都落在 round-rect 的 `QOI / RLE` draw 热点：`MASK_IMAGE_QOI_ROUND_RECT +10.4%`、`MASK_IMAGE_RLE_ROUND_RECT +24.7%`、`MASK_IMAGE_RLE_8_ROUND_RECT +13.7%`、`EXTERN_MASK_IMAGE_RLE_ROUND_RECT +12.8%`；继续细拆后 alpha8 child 把代价收敛到 `QOI_8 / RLE_8 / external 8-bit` draw 热点，而 rgb565 child 在当前完整 `239` 场景里没有任何 `>=10%` 回退
  - 结论：保留 `EGUI_CONFIG_IMAGE_STD_MASK_ROUND_RECT_DRAW_FAST_PATH_ENABLE`，同时继续保留 `EGUI_CONFIG_IMAGE_STD_MASK_ROUND_RECT_DRAW_ALPHA8_FAST_PATH_ENABLE` / `EGUI_CONFIG_IMAGE_STD_MASK_ROUND_RECT_DRAW_RGB565_FAST_PATH_ENABLE`；若项目要保住 raw / non-alpha8 round-rect draw，应先试 alpha8 child；若要保住 alpha8 round-rect draw，应先试 rgb565 child；只有两边都能牺牲时再考虑父宏
- `std image mask round-rect draw alpha8 fast path`
  - 代码证据：`HelloPerformance text -3960B`
  - HelloPerformance 证据：基础主路径与 raw round-rect draw / resize、不带 alpha8 的 `QOI / RLE / external` 样本都在噪声范围；代价只落在 round-rect 的 alpha8 masked-image draw 热点
  - 结论：保留 `EGUI_CONFIG_IMAGE_STD_MASK_ROUND_RECT_DRAW_ALPHA8_FAST_PATH_ENABLE`，作为比 `MASK_ROUND_RECT_DRAW` 更保守的 child
- `std image mask round-rect draw rgb565 fast path`
  - 代码证据：`2026-04-05` current-mainline `HelloPerformance text -2216B`
  - HelloPerformance 证据：基础主路径、alpha8 round-rect draw 对照组和 raw round-rect / resize 对照组都在噪声范围；完整 `239` 场景没有任何 `>=10%` 回退，non-alpha8 round-rect draw 热点里最大也只有 `MASK_IMAGE_RLE_ROUND_RECT +7.7%`
  - 结论：保留 `EGUI_CONFIG_IMAGE_STD_MASK_ROUND_RECT_DRAW_RGB565_FAST_PATH_ENABLE`，作为与 `ROUND_RECT_DRAW_ALPHA8` 互补的 child；若项目要保住 alpha8 round-rect draw，应优先试它
- `std image mask circle draw child fast path`
  - 代码证据：`MASK_CIRCLE_DRAW text -3028B`；继续细拆后 `CIRCLE_DRAW_ALPHA8 child -1876B`、`CIRCLE_DRAW_RGB565 child -1200B`
  - HelloPerformance 证据：基础主路径、internal circle resize 样本和 round-rect 对照组都在噪声范围；父宏代价落在 circle 的 `QOI / RLE / external` draw 热点，继续细拆后的 alpha8 child 只伤 alpha8 draw 热点，rgb565 child 则在当前完整 `239` 场景里保持 `0` 个 `>=10%` 回退
  - 结论：保留 `EGUI_CONFIG_IMAGE_STD_MASK_CIRCLE_DRAW_FAST_PATH_ENABLE`，作为与 `MASK_CIRCLE_RESIZE` 互补的 child；若项目要保住 raw / non-alpha8 circle draw，应先试 `CIRCLE_DRAW_ALPHA8`；若要保住 alpha8 circle draw，应先试 `CIRCLE_DRAW_RGB565`；只有两边都能牺牲时再考虑父宏
- `std image mask circle draw alpha8 fast path`
  - 代码证据：`HelloPerformance text -1876B`
  - HelloPerformance 证据：基础主路径与 raw circle draw / resize、不带 alpha8 的 `QOI / RLE / external` 样本都在噪声范围；代价只落在 circle 的 alpha8 masked-image draw 热点
  - 结论：保留 `EGUI_CONFIG_IMAGE_STD_MASK_CIRCLE_DRAW_ALPHA8_FAST_PATH_ENABLE`，作为比 `MASK_CIRCLE_DRAW` 更保守的 child
- `std image mask circle draw rgb565 fast path`
  - 代码证据：`2026-04-05` current-mainline `HelloPerformance text -1200B`
  - HelloPerformance 证据：基础主路径、alpha8 circle draw 对照组和 raw circle / resize 对照组都在噪声范围；完整 `239` 场景没有任何 `>=10%` 回退，non-alpha8 circle draw 热点里最大也只有 `MASK_IMAGE_RLE_CIRCLE +6.5%`
  - 结论：保留 `EGUI_CONFIG_IMAGE_STD_MASK_CIRCLE_DRAW_RGB565_FAST_PATH_ENABLE`，作为与 `CIRCLE_DRAW_ALPHA8` 互补的 child；若项目要保住 alpha8 circle draw，应优先试它

## 当前收敛结论

- 围绕本轮优先路径：
  - `RECTANGLE`：暂未发现值得单独宏化的独立大 fast path，先不新增宏。
  - `CIRCLE` / `ROUND_RECTANGLE`：继续保留 `EGUI_CONFIG_CIRCLE_FILL_BASIC` 这颗示例局部 override，只给 `HelloPerformance` 打开，不再写入共享默认头。
  - `canvas masked-fill`：当前最值得暴露给 size-first 项目的顺序已经比较清楚，优先是 `EGUI_CONFIG_CANVAS_MASK_FILL_ROW_RANGE_FAST_PATH_ENABLE`，其次是 `EGUI_CONFIG_CANVAS_MASK_FILL_CIRCLE_SEGMENT_FAST_PATH_ENABLE`，第三档是 `EGUI_CONFIG_CANVAS_MASK_FILL_ROW_PARTIAL_FAST_PATH_ENABLE`，第四档是更保守的 `EGUI_CONFIG_CANVAS_MASK_FILL_ROW_INSIDE_FAST_PATH_ENABLE`；其中 `ROW_RANGE`、`CIRCLE_SEGMENT`、`ROW_PARTIAL` 与 `ROW_INSIDE` 都已在 `2026-04-05` current-mainline 下补齐完整 `239` 场景 perf、`241` 帧 runtime 和 `688` 项 unit，全量保持等价。再往后才是 `MASK_FILL_IMAGE`、`ROW_BLEND`，以及会明确牺牲 circle masked-fill 热点的父宏 `MASK_FILL_CIRCLE`。`visible-range` 调用侧 split 已证伪，`text` 不减反增 `+1552B`；`shape umbrella` 也已证伪，当前树最终 ELF `+0B`；`MASK_FILL_IMAGE` 往下再拆出来的 `IMAGE_SCALE child` 也只剩 `-196B`。这三条都不再继续保留新宏。
  - `canvas masked-fill round-rect child`：继续保留宏即可，但当前收益只有 `876B`，已经不属于这轮优先回收对象。
  - `TEXT`：`EGUI_CONFIG_FONT_STD_FAST_DRAW_ENABLE` 仍作为示例局部 override 存在，但关闭后的性能回退远超 `<10%`，不能作为 size-first 默认关闭候选。
  - `TEXT` 继续细拆后，`EGUI_CONFIG_FONT_STD_FAST_MASK_DRAW_ENABLE` 已经满足基础优先场景的阈值，可作为更局部的 size-first 实验开关。
  - `IMAGE_565` / `EXTERN_IMAGE_565`：`EGUI_CONFIG_IMAGE_STD_FAST_DRAW_ENABLE` 太粗，只适合作为实验型 size-first 开关，不能默认关闭。
  - `IMAGE_565` / `EXTERN_IMAGE_565` 继续细拆后，`EGUI_CONFIG_IMAGE_STD_FAST_DRAW_ALPHA_ENABLE` 已经满足基础优先场景阈值，可作为“只牺牲 alpha direct draw”的更局部实验开关。
  - `IMAGE_565` / `EXTERN_IMAGE_565` 继续细拆后，`EGUI_CONFIG_IMAGE_STD_FAST_DRAW_ALPHA8_ENABLE` 已把代价收敛到 internal `RGB565_8 direct draw`，适合作为比 `FAST_DRAW_ALPHA_ENABLE` 更局部的 size-first 实验开关。
  - `IMAGE_565` / `EXTERN_IMAGE_565` 继续细拆后，`EGUI_CONFIG_IMAGE_STD_FAST_DRAW_PACKED_ALPHA_ENABLE` 已进一步把代价收敛到 internal `RGB565_1/2/4` direct draw，适合作为比 `FAST_DRAW_ALPHA_ENABLE` 更局部的 size-first 实验开关。
  - `IMAGE_COLOR` / `IMAGE_RESIZE_COLOR` 继续细拆后，umbrella `EGUI_CONFIG_IMAGE_STD_ALPHA_COLOR_FAST_PATH_ENABLE` 仍满足基础优先场景阈值，但 draw / resize child 只回收 `52B / 184B`，不值得保留长期子宏；因此最终仍只保留 umbrella，定位也仍是条件实验宏。
  - `row-overlay`：`EGUI_CONFIG_IMAGE_STD_ROW_OVERLAY_FAST_PATH_ENABLE` 仍只适合作为 umbrella 实验宏；`2026-04-05` current-mainline 复验继续确认其 non-raw 侧保留 `EGUI_CONFIG_IMAGE_STD_ROW_OVERLAY_NON_RGB565_FAST_PATH_ENABLE` 是成立的，它能单独回收 `1476B`，并在完整 `239` 场景里保持 `IMAGE_GRADIENT_OVERLAY +0.1%`、其余基础路径都在噪声范围。其下继续细拆出的 `EGUI_CONFIG_IMAGE_STD_ROW_OVERLAY_ALPHA8_FAST_PATH_ENABLE` 仍是更保守的 child。如果项目要保住 generic `get_pixel` overlay，应先试 `ROW_OVERLAY_ALPHA8`，再考虑 `ROW_OVERLAY_NON_RGB565`；generic `GET_PIXEL` child 因只有 `188B` 已拒绝。若项目要保住 non-raw overlay、只剩 raw gradient overlay 可以牺牲，再试 `ROW_OVERLAY_RGB565`。
  - `image-mask`：`EGUI_CONFIG_MASK_IMAGE_IDENTITY_SCALE_FAST_PATH_ENABLE` 已经能稳定回收 `3272B`；`2026-04-05` current-mainline 复验继续确认只需要保留 `EGUI_CONFIG_MASK_IMAGE_IDENTITY_SCALE_BLEND_FAST_PATH_ENABLE`，它能单独回收 `2612B`，并把完整 `239` 场景里的 `9` 个 `>=10%` 回退都收敛到 identity-scale masked-image blend 热点；`FILL child` 只有 `660B`，已拒绝。
  - `masked-image partial row visible-range`：`EGUI_CONFIG_IMAGE_STD_MASK_VISIBLE_RANGE_FAST_PATH_ENABLE` 已经能稳定回收 `1416B`，基础主路径不动，但 `EXTERN_MASK_IMAGE_TEST_PERF_CIRCLE` 会明显回退，所以当前定位仍是条件实验宏；继续细拆出的 `VISIBLE_RANGE_RESIZE` child 只有 `820B`，并且 external sampled masked-image resize 热点退化明显，已拒绝。
- `masked-image row-range / row-partial / row-partial-draw / row-partial-resize / row-inside / row-inside-resize`：`EGUI_CONFIG_IMAGE_STD_MASK_ROW_RANGE_FAST_PATH_ENABLE` / `EGUI_CONFIG_IMAGE_STD_MASK_ROW_PARTIAL_FAST_PATH_ENABLE` / `EGUI_CONFIG_IMAGE_STD_MASK_ROW_PARTIAL_DRAW_FAST_PATH_ENABLE` / `EGUI_CONFIG_IMAGE_STD_MASK_ROW_PARTIAL_RESIZE_FAST_PATH_ENABLE` / `EGUI_CONFIG_IMAGE_STD_MASK_ROW_INSIDE_FAST_PATH_ENABLE` / `EGUI_CONFIG_IMAGE_STD_MASK_ROW_INSIDE_RESIZE_FAST_PATH_ENABLE` 都已经满足“基础主路径不退化”的条件，但代价集中在 sampled masked-image 热点；其中 `ROW_RANGE` 回收 `10248B`、`ROW_PARTIAL` 回收 `7600B`、`ROW_PARTIAL_RESIZE` 回收 `6276B`、`ROW_PARTIAL_DRAW` 回收 `2360B`、`ROW_INSIDE` 回收 `1340B`、`ROW_INSIDE_RESIZE` 回收 `928B`。如果只看总代码量，顺序仍应是 `ROW_PARTIAL_RESIZE > ROW_PARTIAL > ROW_RANGE > ROW_PARTIAL_DRAW > ROW_INSIDE_RESIZE > ROW_INSIDE`；如果只关心 partial-row 这条线的更保守隔离，则 draw 侧应按要保住的格式选择：想保住 packed draw 时优先 `ROW_PARTIAL_DRAW_ALPHA8`，想保住 alpha8 draw 时优先 `ROW_PARTIAL_DRAW_PACKED_ALPHA`，如果两边都还能继续牺牲，再试 `ROW_PARTIAL_DRAW`；resize 侧当前只有 `ROW_PARTIAL_RESIZE_PACKED_ALPHA` 同时满足热点收敛和 runtime 严格等价。若项目明确能接受更有针对性的 raw / alpha8 sampled resize 退化或少量像素差，可再按要保住的格式分别试 `ROW_PARTIAL_RESIZE_RGB565`、`ROW_PARTIAL_RESIZE_ALPHA8`；generic child 已证伪为 `0B`，不再保留；然后才是 `ROW_PARTIAL_RESIZE > ROW_PARTIAL`；如果只关心 inside-row 这条线，则仍应先试 `ROW_INSIDE_RESIZE`，再考虑父宏 `ROW_INSIDE`。
  - `masked-image row-partial draw alpha8 child`：`EGUI_CONFIG_IMAGE_STD_MASK_ROW_PARTIAL_DRAW_ALPHA8_FAST_PATH_ENABLE` 还能再独立回收 `1096B`，并继续保住 `IMAGE_565_8 / EXTERN_IMAGE_565_8` 与当前 sampled alpha8 masked-image draw 对照组，且 runtime baseline / off 严格等价。由于现有 perf 集仍未单独隔离 raw alpha8 masked direct-draw partial-row 热点，它仍只适合作为条件实验宏记录；如果项目要尽量保住 packed-alpha partial-row draw，应先试这颗 child，再试 `ROW_PARTIAL_DRAW`。
  - `masked-image row-partial draw packed child`：`EGUI_CONFIG_IMAGE_STD_MASK_ROW_PARTIAL_DRAW_PACKED_ALPHA_FAST_PATH_ENABLE` 还能再独立回收 `1360B`，并继续保住 `IMAGE_565_1/2/4 / EXTERN_IMAGE_565_1/2/4` 与当前 sampled packed masked-image draw 对照组，且 runtime baseline / off 严格等价。由于现有 perf 集仍未单独隔离 raw packed-alpha masked direct-draw partial-row 热点，它也只适合作为条件实验宏记录；如果项目要尽量保住 alpha8 partial-row draw，应先试这颗 child，再试 `ROW_PARTIAL_DRAW`。
  - `masked-image row-partial resize packed child`：`EGUI_CONFIG_IMAGE_STD_MASK_ROW_PARTIAL_RESIZE_PACKED_ALPHA_FAST_PATH_ENABLE` 还能再独立回收 `2436B`，并继续保住 `IMAGE_RESIZE_565_1/2/4 / EXTERN_IMAGE_RESIZE_565_1/2/4` 与父宏下明显回退的 sampled masked-image circle / test-perf 主路径，且 runtime baseline / off 严格等价。由于现有 perf 集仍未单独隔离 raw packed-alpha masked sampled resize partial-row 热点，它也只适合作为条件实验宏记录；如果项目要尽量保住 alpha8 / raw sampled partial-row resize，应先试这颗 child，再试 `ROW_PARTIAL_RESIZE`。
  - `masked-image row-partial resize rgb565 child`：`EGUI_CONFIG_IMAGE_STD_MASK_ROW_PARTIAL_RESIZE_RGB565_FAST_PATH_ENABLE` 还能再独立回收 `2936B`，并继续保住基础主路径、`IMAGE_RESIZE_565_1/2/4 / EXTERN_IMAGE_RESIZE_565_1/2/4` 与 `MASK_IMAGE_CIRCLE / EXTERN_MASK_IMAGE_CIRCLE`；但当前 perf 仍会明显打到 raw sampled masked-image `TEST_PERF_*` 热点，runtime baseline / off 也有 `1` 帧像素差。因此它只适合作为实验记录，不进入当前优先序列；如果项目明确不依赖 raw sampled masked-image `TEST_PERF_*` 热点，且可接受少量像素差，可在 `ROW_PARTIAL_RESIZE_PACKED_ALPHA` 之后再单独试它。
  - `masked-image row-partial resize alpha8 child`：`EGUI_CONFIG_IMAGE_STD_MASK_ROW_PARTIAL_RESIZE_ALPHA8_FAST_PATH_ENABLE` 还能再独立回收 `1000B`，并把代价继续收敛到 alpha8 circle masked-image resize 热点，同时保住 raw sampled resize 对照组；但 runtime baseline / off 存在 `4` 帧细微 circle 边缘像素差。因此它只适合作为条件实验宏，不进入当前优先序列。
  - `masked-image row-partial resize generic child`：尝试把 generic `get_pixel` 与 generic fallback sampled resize partial-row 两处一起拆成 `EGUI_CONFIG_IMAGE_STD_MASK_ROW_PARTIAL_RESIZE_GENERIC_FAST_PATH_ENABLE` 后，`HelloPerformance text` 仍然是 `0B`，且 perf / runtime / unit 全量一致，说明这层没有独立回收价值，因此直接拒绝，不保留宏。
  - `masked-image row-inside resize alpha8 child`：`EGUI_CONFIG_IMAGE_STD_MASK_ROW_INSIDE_RESIZE_ALPHA8_FAST_PATH_ENABLE` 还能再独立回收 `712B`，并把代价继续收敛到 alpha8 circle masked-image resize 热点，同时保住 raw sampled inside-row resize 对照组，且 runtime baseline / off 严格等价。由于收益低于 `1KB`，它也不进入当前优先序列；但如果项目要尽量保住 raw inside-row resize，应先试这颗 child，再试 `ROW_INSIDE_RESIZE`。
  - `masked-image circle resize child`：`EGUI_CONFIG_IMAGE_STD_MASK_CIRCLE_RESIZE_FAST_PATH_ENABLE` 已经把代价收敛到 internal circle masked-image resize 热点，能单独回收 `1984B`；继续细拆后 `EGUI_CONFIG_IMAGE_STD_MASK_CIRCLE_RESIZE_ALPHA8_FAST_PATH_ENABLE` 还能再把代价收敛到 circle 的 alpha8 masked-image resize 热点，但只额外回收 `540B`，不进入当前优先序列；本轮补齐的 `EGUI_CONFIG_IMAGE_STD_MASK_CIRCLE_RESIZE_RGB565_FAST_PATH_ENABLE` 也能再独立回收 `1456B`，并继续保住 alpha8 circle resize 与 circle draw / codec / external 对照组，但仍有 `MASK_IMAGE_TEST_PERF_CIRCLE +36.2%`，runtime on / off 也存在 `frame_0190.png` 的 `1` 帧真实像素差。因此当前主推荐仍是 `CIRCLE_RESIZE`；如果项目极度想保住 raw circle resize，再先试 `CIRCLE_RESIZE_ALPHA8`；如果项目更想保住 alpha8 circle resize，且明确不依赖 raw sampled circle resize hotspot、并能接受这 `1` 帧像素差，可在父宏之前再试 `CIRCLE_RESIZE_RGB565`。
  - `masked-image round-rect resize child`：`EGUI_CONFIG_IMAGE_STD_MASK_ROUND_RECT_RESIZE_FAST_PATH_ENABLE` 已经把代价收敛到 internal round-rect masked-image resize 热点，`2026-04-05` current-mainline 下能单独回收 `3764B`；完整 `239` 场景里只有 `MASK_IMAGE_ROUND_RECT +10.8%`、`MASK_IMAGE_ROUND_RECT_DOUBLE +10.4%` 两个 `>=10%` 回退，`EXTERN_MASK_IMAGE_ROUND_RECT +9.6%` 与 `MASK_IMAGE_TEST_PERF_ROUND_RECT +9.7%` 仍低于阈值；继续细拆后 `EGUI_CONFIG_IMAGE_STD_MASK_ROUND_RECT_RESIZE_ALPHA8_FAST_PATH_ENABLE` 已在 `2026-04-05` current-mainline 下补齐完整 `239` 场景 perf、`241` 帧 runtime 和 `688` 项 unit，能再独立回收 `1136B`，并把代价继续收敛到 round-rect 的 alpha8 masked-image resize 热点；本轮新补齐的 `EGUI_CONFIG_IMAGE_STD_MASK_ROUND_RECT_RESIZE_RGB565_FAST_PATH_ENABLE` 也能再独立回收 `2472B`，并在完整 `239` 场景里保持 `0` 个 `>=10%` 回退，同时保住 alpha8 round-rect resize 与 draw / codec / external 对照组。因此如果项目要保住 raw round-rect resize，顺序应优先 `ROUND_RECT_RESIZE_ALPHA8`；如果项目要保住 alpha8 round-rect resize，顺序应优先 `ROUND_RECT_RESIZE_RGB565`；只有两边都能牺牲时再考虑 `ROUND_RECT_RESIZE`。round-rect resize 这条线现在已经有互补的 `alpha8` / `rgb565` child，后续不再优先沿同一层 `std.c` 继续细拆。
  - `masked-image round-rect draw child`：`EGUI_CONFIG_IMAGE_STD_MASK_ROUND_RECT_DRAW_FAST_PATH_ENABLE` 已经把代价进一步收敛到 round-rect 的 codec / external masked-image draw 热点，`2026-04-05` current-mainline 下能单独回收 `5804B`，同时不伤 internal round-rect resize 样本；继续细拆后 `EGUI_CONFIG_IMAGE_STD_MASK_ROUND_RECT_DRAW_ALPHA8_FAST_PATH_ENABLE` 还能再独立回收 `3960B`，并把代价继续收敛到 round-rect 的 alpha8 masked-image draw 热点；新补齐的 `EGUI_CONFIG_IMAGE_STD_MASK_ROUND_RECT_DRAW_RGB565_FAST_PATH_ENABLE` 也能再独立回收 `2216B`，并在完整 `239` 场景里保持 `0` 个 `>=10%` 回退，同时保住 alpha8 round-rect draw 热点。再往下拆的 `ROW_FAST child` 与 `ROUND_RECT_DRAW_ALPHA8` 实质重合，已证伪为重复宏。因此如果项目要保住 raw / non-alpha8 round-rect draw，顺序应优先 `ROUND_RECT_DRAW_ALPHA8`；如果项目要保住 alpha8 round-rect draw，顺序应优先 `ROUND_RECT_DRAW_RGB565`；只有两边都能牺牲时再考虑 `ROUND_RECT_DRAW`；不再继续追 `ROW_FAST child`。
  - `masked-image circle draw child`：`EGUI_CONFIG_IMAGE_STD_MASK_CIRCLE_DRAW_FAST_PATH_ENABLE` 已经把代价进一步收敛到 circle 的 codec / external masked-image draw 热点，能单独回收 `3028B`，同时不伤 internal circle resize 样本；继续细拆后 `EGUI_CONFIG_IMAGE_STD_MASK_CIRCLE_DRAW_ALPHA8_FAST_PATH_ENABLE` 还能再独立回收 `1876B`，并把代价继续收敛到 circle 的 alpha8 masked-image draw 热点；新补齐的 `EGUI_CONFIG_IMAGE_STD_MASK_CIRCLE_DRAW_RGB565_FAST_PATH_ENABLE` 也能再独立回收 `1200B`，并在完整 `239` 场景里保持 `0` 个 `>=10%` 回退，同时保住 alpha8 circle draw 热点。因此如果项目要保住 raw / non-alpha8 circle draw，顺序应优先 `CIRCLE_DRAW_ALPHA8`；如果项目要保住 alpha8 circle draw，顺序应优先 `CIRCLE_DRAW_RGB565`；只有两边都能牺牲时再考虑 `CIRCLE_DRAW`。
  - `image codec mask-image row-block child`：继续往 decode/provider 侧下钻后，把 `QOI/RLE` image-mask decode 调用侧 `row_block` 入口里的 `use_image_mask` 分支单独挂成 `EGUI_CONFIG_IMAGE_CODEC_MASK_IMAGE_ROW_BLOCK_FAST_PATH_ENABLE`。结果仍能再独立回收 `856B`，且 `HelloPerformance` 全量 `239` 场景 perf、runtime `241` 帧和 `HelloUnitTest 688/688` 都保持等价，说明这层 image-mask 调度仍有独立回收价值；但收益仍低于当前 `1KB` 主推荐门槛，因此只保留为更低优先级条件实验宏。
  - `image rle mask-image row-block child`：继续沿 image-mask decode provider 侧细拆，只把 `RLE` 调用侧 `row_block` 入口单独挂成 `EGUI_CONFIG_IMAGE_RLE_MASK_IMAGE_ROW_BLOCK_FAST_PATH_ENABLE`。结果 `HelloPerformance text` 只再回收 `108B`，且完整 `239` 场景 perf、runtime `241` 帧和 `HelloUnitTest 688/688` 仍都严格等价，说明这层 provider child 没有值得长期保留的独立收益，因此直接拒绝并回退代码。
  - `image qoi masked alpha8 split child`：继续往 decode/provider 侧下钻后，尝试把 QOI tail-row-cache 里的 masked `RGB565_8` visible-span split-decode 直连路径单独挂成 `EGUI_CONFIG_IMAGE_QOI_MASKED_ALPHA8_SPLIT_FAST_PATH_ENABLE`。结果 `HelloPerformance text` 仍然是 `0B`，且完整 `239` 场景 perf、runtime `241` 帧和 `HelloUnitTest 688/688` 全都严格等价，说明这层没有独立回收价值，因此直接拒绝，不保留宏。
  - `image qoi direct alpha8 split child`：沿着 checkpoint 之后更窄的 tail-row-cache / direct-path selector 继续细拆，只把 direct `RGB565_8` visible-span split-decode 直连 blend 分支单独挂成 `EGUI_CONFIG_IMAGE_QOI_DIRECT_ALPHA8_SPLIT_FAST_PATH_ENABLE`。结果 `HelloPerformance text` 仍然是 `0B`，完整 `239` 场景 perf 继续全量等价，runtime `241` 帧 hash / pixel mismatch 仍然是 `0`，`HelloUnitTest` 也是 `688/688 passed`，说明这层同样是没有独立体积回收价值的 dead child，因此直接拒绝并回退代码。
  - `image qoi direct copy split child`：沿同一层 tail-row-cache / direct-path selector 继续把无 alpha 的 direct `RGB565` visible-span split-decode 直连 copy 分支单独挂成 `EGUI_CONFIG_IMAGE_QOI_DIRECT_COPY_SPLIT_FAST_PATH_ENABLE`。结果 `HelloPerformance text` 仍然是 `0B`，完整 `239` 场景 perf 依旧全量等价，runtime `241` 帧 hash / pixel mismatch 仍是 `0`，`HelloUnitTest` 继续 `688/688 passed`，说明这颗 sibling 也是没有独立体积回收价值的 dead child，因此直接拒绝并回退代码。
  - `image rle checkpoint child`：转去同层现成宏 `EGUI_CONFIG_IMAGE_RLE_CHECKPOINT_ENABLE` 做完整 A/B。结果只拿到 `HelloPerformance text -92B, bss -16B`，但完整 `239` 场景里同样有 `20` 个 `>=10%` 回退，直接打穿整条 RLE draw 主路径，其中 `IMAGE_RLE_565 +238.4%`、`EXTERN_IMAGE_RLE_565 +452.0%`、`MASK_IMAGE_RLE_NO_MASK +238.4%`、`EXTERN_MASK_IMAGE_RLE_NO_MASK +452.0%`、`IMAGE_RLE_565_8 +109.5%`、`EXTERN_IMAGE_RLE_565_8 +282.1%`。runtime `241` 帧和 `HelloUnitTest 688/688` 仍严格等价，说明这层和 `QOI checkpoint` 一样，也是功能等价但性能关键的主路径块，因此当前不再沿 `RLE checkpoint` 继续细拆。
  - `image rle direct copy split child`：继续转回 `RLE` tail-row-cache / direct-path selector，只把 direct `RGB565` visible-span split-decode 直连 copy 分支单独挂成 `EGUI_CONFIG_IMAGE_RLE_DIRECT_COPY_SPLIT_FAST_PATH_ENABLE`。结果 `HelloPerformance text` 仍然是 `0B`，完整 `239` 场景 perf 继续全量等价，runtime `241` 帧 hash / pixel mismatch 仍然是 `0`，`HelloUnitTest` 也继续 `688/688 passed`，说明这颗 sibling 和 `QOI direct copy split` 一样，也是没有独立体积回收价值的 dead child，因此直接拒绝并回退代码。
  - `std image external row persistent cache`：转到 image/external 侧现成宏 `EGUI_CONFIG_IMAGE_STD_EXTERNAL_ROW_PERSISTENT_CACHE_ENABLE` 做完整 A/B。结果不是 text-first split，而是 low-RAM tradeoff：`HelloPerformance text +20B, bss -40B`，完整 `239` 场景 perf 没有任何 `>=10%` 回退，runtime `241` 帧 hash / pixel mismatch 仍然是 `0`，`HelloUnitTest` 继续 `688/688 passed`。说明这颗宏可以保留为低优先级 low-RAM 条件实验宏，但不进入当前 text-first split 主线。
  - `transform external row persistent cache`：继续沿 image/external 的现成宏做 A/B，把 `EGUI_CONFIG_IMAGE_TRANSFORM_EXTERNAL_ROW_PERSISTENT_CACHE_ENABLE` 单独拉出来。结果比 std external row persistent 更干净：`HelloPerformance text -24B, bss -56B`，完整 `239` 场景 perf 没有任何 `>=10%` 回退，runtime `241` 帧 hash / pixel mismatch 仍然是 `0`，`HelloUnitTest` 继续 `688/688 passed`。说明这颗宏可以保留为低优先级 low-RAM 条件实验宏；它仍不属于当前 text-first split 主线，但如果项目想先榨一点 RAM，这颗比 std external row persistent 更值得先试。
  - `image qoi checkpoint child`：继续往 decode/provider 侧尝试把 QOI row-band checkpoint 的 save/restore 与 checkpoint table 管理单独挂成临时 `EGUI_CONFIG_IMAGE_QOI_CHECKPOINT_ENABLE`。结果只拿到 `HelloPerformance text -276B, bss -8B`，但完整 `239` 场景里有 `20` 个 `>=10%` 回退，直接打穿整条 QOI draw 主路径，其中 `IMAGE_QOI_565 +819.2%`、`EXTERN_IMAGE_QOI_565 +911.4%`、`MASK_IMAGE_QOI_NO_MASK +819.2%`、`EXTERN_MASK_IMAGE_QOI_NO_MASK +911.4%`。runtime `241` 帧和 `HelloUnitTest 688/688` 仍严格等价，说明这层是功能等价但性能关键的主路径块，因此直接拒绝并回退代码。
  - `IMAGE_RESIZE_565` / `EXTERN_IMAGE_RESIZE_565`：`EGUI_CONFIG_IMAGE_STD_FAST_RESIZE_ENABLE` 太粗，不满足 resize 热点 `<10%` 阈值。
  - `IMAGE_RESIZE_565` / `EXTERN_IMAGE_RESIZE_565` 继续细拆后，`EGUI_CONFIG_IMAGE_STD_FAST_RESIZE_ALPHA_ENABLE` 更接近目标，但前提是项目没有 alpha resize 热点。
  - `IMAGE_RESIZE_565` / `EXTERN_IMAGE_RESIZE_565` 继续细拆后，`EGUI_CONFIG_IMAGE_STD_FAST_RESIZE_ALPHA8_ENABLE` 已把代价收敛到 internal `RGB565_8 resize`，适合作为比 `FAST_RESIZE_ALPHA_ENABLE` 更局部的 size-first 实验开关。
  - `IMAGE_RESIZE_565` / `EXTERN_IMAGE_RESIZE_565` 继续细拆后，`EGUI_CONFIG_IMAGE_STD_FAST_RESIZE_PACKED_ALPHA_ENABLE` 已进一步把代价收敛到 internal `RGB565_1/2/4 resize`，适合作为比 `FAST_RESIZE_ALPHA_ENABLE` 更局部的 size-first 实验开关。
  - `EXTERN_IMAGE_RESIZE_565` 继续细拆后，`EGUI_CONFIG_IMAGE_STD_EXTERNAL_PACKED_ALPHA_RESIZE_FAST_PATH_ENABLE` 已把代价进一步收敛到 external `RGB565_1/2/4 resize`；与之对应的 draw-only split 只有 `180B`，不再保留。
- 已有的 low-RAM policy 项里：
  - `shadow dsq LUT`
  - `text transform` 的 stack/transient-heap 策略
  - 都已经归到 `app_egui_config.h` 尾部的 `#if 0` 可选块，不再污染默认构建面。
- 已经证明确实“不值得默认开”的快路径缓存项里：
  - 历史名字 `EGUI_CONFIG_IMAGE_STD_ROUND_RECT_FAST_ROW_CACHE_ENABLE`
  - 历史名字 `EGUI_CONFIG_MASK_CIRCLE_FRAME_ROW_CACHE_ENABLE`
  - 这两项已经处理完，现已收回实现私有并保持默认关闭，不再作为 active override。
- 已经证明确实“不能回收”的快路径项里：
  - `font std fast draw`
  - `std image fast draw main switch`
  - `std image fast resize fast path`
  - `image mask circle / round-rect fast path（整条）`
  - `IMAGE_CODEC_ROW_CACHE_ENABLE`
  - 都需要保留。
- 其中 `alpha opaque slots` 已按同一规则内收到实现私有默认 `4`：它不是值得继续保留 public 宏名的项，只是 shipped 行为仍保持不变。
- 已经证明确实“可以继续细拆成实验宏”的项里：
  - `font std fast mask draw`
  - `std image fast draw alpha fast path`
  - `std image alpha8 direct-draw fast path`
  - `std image packed-alpha direct-draw fast path`
  - `std image alpha-color fast path`
  - `std image fast resize alpha fast path`
  - `std image alpha8 resize fast path`
  - `std image packed-alpha resize fast path`
  - `std image external alpha fast path`
  - `std image row-overlay non-rgb565 fast path`
  - `std image row-overlay rgb565 fast path`
  - `std image external packed-alpha fast path`
  - `std image external packed-alpha resize fast path`
  - `std image mask circle fast path`
  - `std image mask round-rect fast path`
  - `std image mask round-rect draw fast path`
  - `std image mask round-rect draw alpha8 fast path`
  - `std image mask round-rect draw rgb565 fast path`
  - `std image mask circle draw fast path`
  - `std image mask circle draw alpha8 fast path`
  - `std image mask circle draw rgb565 fast path`
  - `std image mask round-rect resize fast path`
  - `std image mask round-rect resize alpha8 fast path`
  - `std image mask circle resize fast path`
  - `std image mask visible-range fast path`
  - `std image mask row-range fast path`
  - `std image mask row-partial fast path`
  - `std image mask row-partial draw fast path`
  - `std image mask row-partial draw alpha8 fast path`
  - `std image mask row-partial draw packed-alpha fast path`
  - `std image mask row-partial resize fast path`
  - `std image mask row-partial resize packed-alpha fast path`
  - `std image mask row-partial resize alpha8 fast path`
  - `std image mask row-inside resize fast path`
  - `std image mask row-inside resize alpha8 fast path`
  - `std image mask row-inside fast path`
  - `mask-image identity-scale fast path`
  - `canvas mask fill row-range fast path`
  - `canvas mask fill row-partial fast path`
  - `canvas mask fill row-inside fast path`
  - `canvas mask fill circle fast path`
  - `canvas mask fill image fast path`
  - `canvas mask fill row-blend fast path`
  - 这类拆法优先保留基础直绘快路径，只把 mask / specialized 分支挪成按需开关。

## 本轮继续细拆 A/B: `image codec mask-image row-block fast path`

### 测试环境

- 日期：`2026-04-04`
- 提交基线：`8e62b26`
- `HelloPerformance` code size A/B
  - `make -B -j APP=HelloPerformance PORT=qemu CPU_ARCH=cortex-m0plus APP_OBJ_SUFFIX=hpcodecimgmaskrowblockon USER_CFLAGS=-DEGUI_CONFIG_IMAGE_CODEC_MASK_IMAGE_ROW_BLOCK_FAST_PATH_ENABLE=1`
  - `make -B -j APP=HelloPerformance PORT=qemu CPU_ARCH=cortex-m0plus APP_OBJ_SUFFIX=hpcodecimgmaskrowblockoff USER_CFLAGS=-DEGUI_CONFIG_IMAGE_CODEC_MASK_IMAGE_ROW_BLOCK_FAST_PATH_ENABLE=0`
- `HelloPerformance` perf A/B
  - `python scripts/perf_analysis/code_perf_check.py --profile cortex-m3 --clean --threshold 1000 --timeout 600 --extra-cflags=-DEGUI_CONFIG_IMAGE_CODEC_MASK_IMAGE_ROW_BLOCK_FAST_PATH_ENABLE=1`
  - `python scripts/perf_analysis/code_perf_check.py --profile cortex-m3 --clean --threshold 1000 --timeout 600 --extra-cflags=-DEGUI_CONFIG_IMAGE_CODEC_MASK_IMAGE_ROW_BLOCK_FAST_PATH_ENABLE=0`
- 运行/单测验证
  - `HelloPerformance` baseline / off 通过 `scripts.code_runtime_check.compile_app()` + `run_app(timeout=35)` 注入 `-DEGUI_CONFIG_IMAGE_CODEC_MASK_IMAGE_ROW_BLOCK_FAST_PATH_ENABLE=1/0`
  - `make -j APP=HelloUnitTest PORT=pc_test APP_OBJ_SUFFIX=utcodecimgmaskrowblockon USER_CFLAGS=-DEGUI_CONFIG_IMAGE_CODEC_MASK_IMAGE_ROW_BLOCK_FAST_PATH_ENABLE=1`
  - `make -j APP=HelloUnitTest PORT=pc_test APP_OBJ_SUFFIX=utcodecimgmaskrowblockoff USER_CFLAGS=-DEGUI_CONFIG_IMAGE_CODEC_MASK_IMAGE_ROW_BLOCK_FAST_PATH_ENABLE=0`
  - `output/main.exe`

### 代码量证据

- `HelloPerformance` 链接汇总：
  - 默认开启：`text=2056264 data=72 bss=3832`
  - 关闭 codec mask-image row-block child：`text=2055408 data=72 bss=3832`
- 结论：
  - `text -856B`
  - `data/bss` 不变

### 性能证据

- `HelloPerformance` 全量 `239` 场景 on / off 结果都在噪声内
- 当前没有任何场景超过 `10%`
- 关键场景全部保持 `+0.0%`：
  - `RECTANGLE / CIRCLE / ROUND_RECTANGLE / TEXT`
  - `IMAGE_565 / EXTERN_IMAGE_565 / IMAGE_RESIZE_565 / EXTERN_IMAGE_RESIZE_565`
  - `MASK_IMAGE_QOI_IMAGE / EXTERN_MASK_IMAGE_QOI_IMAGE`
  - `MASK_IMAGE_RLE_IMAGE / EXTERN_MASK_IMAGE_RLE_IMAGE`
  - `MASK_IMAGE_QOI_8_IMAGE / EXTERN_MASK_IMAGE_QOI_8_IMAGE`
  - `MASK_IMAGE_RLE_8_IMAGE / EXTERN_MASK_IMAGE_RLE_8_IMAGE`

### 运行与渲染验证

- `HelloPerformance` baseline / off 都是 `241 frames captured`
- `frame_0000.png` / `frame_0120.png` / `frame_0240.png` hash 一致
- 全量 `241` 帧 `hash mismatch 0`
- 全量 `241` 帧 `pixel mismatch 0`

### 单元测试

- `HelloUnitTest` baseline：`688/688 passed`
- `HelloUnitTest` off：`688/688 passed`

### 结论

- `EGUI_CONFIG_IMAGE_CODEC_MASK_IMAGE_ROW_BLOCK_FAST_PATH_ENABLE` 证明 `QOI/RLE` image-mask decode 调用侧 `row_block` 的 `use_image_mask` 分支有独立回收价值：
  - `HelloPerformance text -856B`
  - perf / runtime / unit 全量通过
- 但收益仍低于当前 `1KB` 经验门槛，因此不进入当前主推荐序列。
- 这颗宏适合作为低优先级条件实验宏保留：
  - 如果项目想继续压缩 `QOI/RLE` image-mask row-block 调度代码，可以单独试它
  - 如果后续还要继续沿 image-mask decode 这条线细拆，优先级应放在别的 decode/provider 层，而不是继续追 `std.c` 或同一层 `row_block child`

## 本轮继续细拆 A/B: `image rle mask-image row-block fast path`

### 测试环境

- 日期：`2026-04-04`
- 提交基线：`cf2617b`
- 临时 provider child
  - `EGUI_CONFIG_IMAGE_RLE_MASK_IMAGE_ROW_BLOCK_FAST_PATH_ENABLE`
- `HelloPerformance` code size A/B
  - `make -B -j APP=HelloPerformance PORT=qemu CPU_ARCH=cortex-m0plus APP_OBJ_SUFFIX=hprlemaskimgrowblockon USER_CFLAGS=-DEGUI_CONFIG_IMAGE_RLE_MASK_IMAGE_ROW_BLOCK_FAST_PATH_ENABLE=1`
  - `make -B -j APP=HelloPerformance PORT=qemu CPU_ARCH=cortex-m0plus APP_OBJ_SUFFIX=hprlemaskimgrowblockoff USER_CFLAGS=-DEGUI_CONFIG_IMAGE_RLE_MASK_IMAGE_ROW_BLOCK_FAST_PATH_ENABLE=0`
- `HelloPerformance` perf A/B
  - `python scripts/perf_analysis/code_perf_check.py --profile cortex-m3 --clean --threshold 1000 --timeout 600 --extra-cflags=-DEGUI_CONFIG_IMAGE_RLE_MASK_IMAGE_ROW_BLOCK_FAST_PATH_ENABLE=1`
  - `python scripts/perf_analysis/code_perf_check.py --profile cortex-m3 --clean --threshold 1000 --timeout 600 --extra-cflags=-DEGUI_CONFIG_IMAGE_RLE_MASK_IMAGE_ROW_BLOCK_FAST_PATH_ENABLE=0`
- 运行 / 单测验证
  - `USER_CFLAGS=-DEGUI_CONFIG_IMAGE_RLE_MASK_IMAGE_ROW_BLOCK_FAST_PATH_ENABLE=1 python scripts/code_runtime_check.py --app HelloPerformance --timeout 35 --keep-screenshots`
  - `USER_CFLAGS=-DEGUI_CONFIG_IMAGE_RLE_MASK_IMAGE_ROW_BLOCK_FAST_PATH_ENABLE=0 python scripts/code_runtime_check.py --app HelloPerformance --timeout 35 --keep-screenshots`
  - `make -j APP=HelloUnitTest PORT=pc_test APP_OBJ_SUFFIX=utrlemaskimgrowblockon USER_CFLAGS=-DEGUI_CONFIG_IMAGE_RLE_MASK_IMAGE_ROW_BLOCK_FAST_PATH_ENABLE=1`
  - `make -j APP=HelloUnitTest PORT=pc_test APP_OBJ_SUFFIX=utrlemaskimgrowblockoff USER_CFLAGS=-DEGUI_CONFIG_IMAGE_RLE_MASK_IMAGE_ROW_BLOCK_FAST_PATH_ENABLE=0`
  - `output/main.exe`

### 代码量证据

- `HelloPerformance` 链接汇总：
  - on：`text=2055380 data=72 bss=3832`
  - off：`text=2055272 data=72 bss=3832`
- 结论：
  - `text -108B`
  - `data/bss` 不变
- 说明这颗 provider child 只有很薄的一层 `RLE` image-mask row-block 调度壳。

### 性能证据

- `HelloPerformance` 全量 `239` 场景 on / off 都在噪声内
- 当前没有任何场景超过 `10%`
- 关键场景保持 `+0.0%`：
  - `RECTANGLE / CIRCLE / ROUND_RECTANGLE / TEXT`
  - `IMAGE_565 / EXTERN_IMAGE_565 / IMAGE_RESIZE_565 / EXTERN_IMAGE_RESIZE_565`
  - `MASK_IMAGE_RLE_IMAGE / EXTERN_MASK_IMAGE_RLE_IMAGE`
  - `MASK_IMAGE_RLE_8_IMAGE / EXTERN_MASK_IMAGE_RLE_8_IMAGE`
- 最大波动也只有：
  - `IMAGE_TILED_RLE_565_0 -0.21%`
  - `IMAGE_TILED_RLE_565_8 -0.08%`

### 运行与渲染验证

- `HelloPerformance` on / off 都是 `241 frames`
- 全量 `241` 帧 `hash mismatch 0`
- 全量 `241` 帧 `pixel mismatch 0`
- 抽查 `frame_0000.png` / `frame_0120.png` / `frame_0240.png` hash 一致

### 单元测试

- `HelloUnitTest` on：`688/688 passed`
- `HelloUnitTest` off：`688/688 passed`

### 结论

- `EGUI_CONFIG_IMAGE_RLE_MASK_IMAGE_ROW_BLOCK_FAST_PATH_ENABLE` 不保留：
  - `HelloPerformance text -108B`
  - perf / runtime / unit 全量严格等价
  - 说明这层 `RLE` provider child 没有值得长期保留的独立收益
- 因此：
  - 代码已回退，不引入这颗临时 child
  - 当前不再沿这颗 `RLE` image-mask row-block child 继续细拆

## 本轮继续细拆 A/B: `image qoi masked alpha8 split fast path`

### 测试环境

- 日期：`2026-04-04`
- 提交基线：`7865d0e`
- 临时 child split
  - `EGUI_CONFIG_IMAGE_QOI_MASKED_ALPHA8_SPLIT_FAST_PATH_ENABLE`
- `HelloPerformance` perf A/B
  - baseline 结果：`.claude/perf_qoi_masked_alpha8_split_on_results.json`
  - `QOI masked alpha8 split child off`：`.claude/perf_qoi_masked_alpha8_split_off_results.json`
- 运行 / 单测验证
  - `HelloPerformance` runtime 录制输出：
    - `runtime_check_output/HelloPerformance/qoimaska8split_on`
    - `runtime_check_output/HelloPerformance/qoimaska8split_off`
  - `HelloUnitTest` baseline / off 都已跑通

### 代码量证据

- `HelloPerformance` 链接汇总：
  - baseline：`text=2056264 data=72 bss=3832`
  - 关闭 `QOI masked alpha8 split child`：`text=2056264 data=72 bss=3832`
- 结论：
  - `text 0B`
  - `data/bss` 不变
- 这说明 QOI tail-row-cache 里这层 masked alpha8 visible-span split-decode 直连分支没有独立 code-size 回收价值。

### 性能证据

- `HelloPerformance` 完整 `239` 场景 perf on / off 全量一致
- 当前没有任何场景超过 `10%`
- 关键场景全部保持 `+0.0%`：
  - `RECTANGLE / CIRCLE / ROUND_RECTANGLE / TEXT`
  - `IMAGE_565 / EXTERN_IMAGE_565 / IMAGE_RESIZE_565 / EXTERN_IMAGE_RESIZE_565`
  - `MASK_IMAGE_QOI_8_NO_MASK / ROUND_RECT / CIRCLE / IMAGE`
  - `EXTERN_MASK_IMAGE_QOI_8_NO_MASK / ROUND_RECT / CIRCLE / IMAGE`

### 运行与渲染验证

- `HelloPerformance` runtime
  - baseline：`241 frames`
  - `QOI masked alpha8 split child off`：`241 frames`
  - baseline / off 全量 `241` 帧 PNG hash mismatch `0`
  - baseline / off 全量 `241` 帧 pixel mismatch `0`
  - 抽查 `frame_0000.png` / `frame_0120.png` / `frame_0240.png` hash 一致

### 单元测试

- `HelloUnitTest`
  - baseline：`688/688 passed`
  - `QOI masked alpha8 split child off`：`688/688 passed`

### 结论

- `EGUI_CONFIG_IMAGE_QOI_MASKED_ALPHA8_SPLIT_FAST_PATH_ENABLE` 不保留：
  - `HelloPerformance text 0B`
  - perf / runtime / unit 全量严格等价
  - 证明这层 QOI decode child 本身没有独立回收价值
- 因此：
  - 代码已回退，不引入这颗临时 child
  - 当前不再沿这条 QOI masked alpha8 split 线继续细拆
  - 如果后续还要继续压缩 draw-alpha8 相关代码，优先回到别的 decode/provider 层找真正有体积的块

## 本轮继续细拆 A/B: `image qoi checkpoint`

### 测试环境

- 日期：`2026-04-04`
- 提交基线：`7c52653`
- 临时 child
  - `EGUI_CONFIG_IMAGE_QOI_CHECKPOINT_ENABLE`
- `HelloPerformance` code size A/B
  - `make -B -j APP=HelloPerformance PORT=qemu CPU_ARCH=cortex-m0plus APP_OBJ_SUFFIX=hpqoickpton USER_CFLAGS=-DEGUI_CONFIG_IMAGE_QOI_CHECKPOINT_ENABLE=1`
  - `make -B -j APP=HelloPerformance PORT=qemu CPU_ARCH=cortex-m0plus APP_OBJ_SUFFIX=hpqoickptoff USER_CFLAGS=-DEGUI_CONFIG_IMAGE_QOI_CHECKPOINT_ENABLE=0`
- `HelloPerformance` perf A/B
  - `python scripts/perf_analysis/code_perf_check.py --profile cortex-m3 --clean --threshold 1000 --timeout 600 --extra-cflags=-DEGUI_CONFIG_IMAGE_QOI_CHECKPOINT_ENABLE=1`
  - `python scripts/perf_analysis/code_perf_check.py --profile cortex-m3 --clean --threshold 1000 --timeout 600 --extra-cflags=-DEGUI_CONFIG_IMAGE_QOI_CHECKPOINT_ENABLE=0`
- 运行 / 单测验证
  - runtime 输出目录：
    - `runtime_check_output/HelloPerformance/qoickpt_on`
    - `runtime_check_output/HelloPerformance/qoickpt_off`
  - `make -j APP=HelloUnitTest PORT=pc_test APP_OBJ_SUFFIX=utqoickpton USER_CFLAGS=-DEGUI_CONFIG_IMAGE_QOI_CHECKPOINT_ENABLE=1`
  - `make -j APP=HelloUnitTest PORT=pc_test APP_OBJ_SUFFIX=utqoickptoff USER_CFLAGS=-DEGUI_CONFIG_IMAGE_QOI_CHECKPOINT_ENABLE=0`
  - `output/main.exe`

### 代码量证据

- `HelloPerformance` 链接汇总：
  - on：`text=2055380 data=72 bss=3832`
  - off：`text=2055104 data=72 bss=3824`
- 结论：
  - `text -276B`
  - `bss -8B`
- 说明这颗 child 只覆盖 QOI row-band checkpoint 的 save/restore 和 checkpoint table 管理，独立体积很薄。

### 性能证据

- `HelloPerformance` 完整 `239` 场景对比里，有 `20` 个场景回退达到 `>=10%`
- 基础主路径保持不变：
  - `RECTANGLE / CIRCLE / ROUND_RECTANGLE / TEXT`
  - `IMAGE_565 / EXTERN_IMAGE_565 / IMAGE_RESIZE_565 / EXTERN_IMAGE_RESIZE_565`
- 但整条 QOI draw 主路径都会明显退化：
  - `IMAGE_QOI_565 / MASK_IMAGE_QOI_NO_MASK +819.2%`
  - `MASK_IMAGE_QOI_ROUND_RECT +789.6%`
  - `MASK_IMAGE_QOI_CIRCLE +772.1%`
  - `EXTERN_IMAGE_QOI_565 / EXTERN_MASK_IMAGE_QOI_NO_MASK +911.4%`
  - `EXTERN_MASK_IMAGE_QOI_ROUND_RECT +898.6%`
  - `EXTERN_MASK_IMAGE_QOI_CIRCLE +890.8%`
  - `IMAGE_QOI_565_8 / MASK_IMAGE_QOI_8_NO_MASK +374.7% ~ +375.0%`
  - `EXTERN_IMAGE_QOI_565_8 / EXTERN_MASK_IMAGE_QOI_8_NO_MASK +484.2%`
  - `MASK_IMAGE_QOI_IMAGE +250.6%`
  - `EXTERN_MASK_IMAGE_QOI_IMAGE +315.3%`
- 这说明 checkpoint 不是可安全剥离的薄壳，而是 QOI 避免重复 decode 的关键主路径块。

### 运行与渲染验证

- `HelloPerformance` on / off 都是 `241 frames`
- 全量 `241` 帧：
  - `hash mismatch 0`
  - `pixel mismatch 0`
- 抽查 `frame_0000.png` / `frame_0120.png` / `frame_0240.png` hash 一致

### 单元测试

- `HelloUnitTest` on：`688/688 passed`
- `HelloUnitTest` off：`688/688 passed`

### 结论

- 临时 `EGUI_CONFIG_IMAGE_QOI_CHECKPOINT_ENABLE` 不保留：
  - `HelloPerformance text -276B, bss -8B`
  - 但 perf 会把整条 QOI draw 主路径打到 `+110% ~ +911%`
  - runtime / unit 只是证明功能等价，不能抵消这类主路径性能代价
- 因此：
  - 代码已回退，不引入这颗临时 child
  - 当前不再沿 `QOI checkpoint` 这条线继续细拆
  - 如果后续还要继续压缩 QOI 侧代码，应优先找比 checkpoint 更窄的 tail-row-cache / direct-path selector 分支

## 本轮继续细拆 A/B: `image qoi direct alpha8 split fast path`

### 测试环境

- 日期：`2026-04-04`
- 提交基线：`8c20d93`
- 临时 child
  - `EGUI_CONFIG_IMAGE_QOI_DIRECT_ALPHA8_SPLIT_FAST_PATH_ENABLE`
- `HelloPerformance` code size A/B
  - `make -B -j APP=HelloPerformance PORT=qemu CPU_ARCH=cortex-m0plus APP_OBJ_SUFFIX=hpqoidirecta8spliton USER_CFLAGS=-DEGUI_CONFIG_IMAGE_QOI_DIRECT_ALPHA8_SPLIT_FAST_PATH_ENABLE=1`
  - `make -B -j APP=HelloPerformance PORT=qemu CPU_ARCH=cortex-m0plus APP_OBJ_SUFFIX=hpqoidirecta8splitoff USER_CFLAGS=-DEGUI_CONFIG_IMAGE_QOI_DIRECT_ALPHA8_SPLIT_FAST_PATH_ENABLE=0`
- `HelloPerformance` perf A/B
  - `python scripts/perf_analysis/code_perf_check.py --profile cortex-m3 --clean --threshold 1000 --timeout 600 --extra-cflags=-DEGUI_CONFIG_IMAGE_QOI_DIRECT_ALPHA8_SPLIT_FAST_PATH_ENABLE=1`
  - `python scripts/perf_analysis/code_perf_check.py --profile cortex-m3 --clean --threshold 1000 --timeout 600 --extra-cflags=-DEGUI_CONFIG_IMAGE_QOI_DIRECT_ALPHA8_SPLIT_FAST_PATH_ENABLE=0`
- 运行 / 单测验证
  - runtime 输出目录：
    - `runtime_check_output/HelloPerformance/qoidirecta8split_on`
    - `runtime_check_output/HelloPerformance/qoidirecta8split_off`
  - `make -j APP=HelloUnitTest PORT=pc_test APP_OBJ_SUFFIX=utqoidirecta8spliton USER_CFLAGS=-DEGUI_CONFIG_IMAGE_QOI_DIRECT_ALPHA8_SPLIT_FAST_PATH_ENABLE=1`
  - `make -j APP=HelloUnitTest PORT=pc_test APP_OBJ_SUFFIX=utqoidirecta8splitoff USER_CFLAGS=-DEGUI_CONFIG_IMAGE_QOI_DIRECT_ALPHA8_SPLIT_FAST_PATH_ENABLE=0`
  - `output/main.exe`

### 代码量证据

- `HelloPerformance` 链接汇总：
  - on：`text=2055380 data=72 bss=3832`
  - off：`text=2055380 data=72 bss=3832`
- 结论：
  - `text 0B`
  - `data/bss 0B`
- 说明这颗 child 只覆盖 QOI tail-row-cache 里的 direct `RGB565_8` visible-span split-decode 直连 blend 分支，本身没有独立 code-size 回收价值。

### 性能证据

- `HelloPerformance` 完整 `239` 场景 on / off 全量一致，没有任何 `>=10%` 波动
- 基础主路径保持 `+0.0%`：
  - `RECTANGLE / CIRCLE / ROUND_RECTANGLE / TEXT`
  - `IMAGE_565 / EXTERN_IMAGE_565 / IMAGE_RESIZE_565 / EXTERN_IMAGE_RESIZE_565`
- QOI 主路径与 alpha8 对照组也保持 `+0.0%`：
  - `IMAGE_QOI_565 / EXTERN_IMAGE_QOI_565`
  - `MASK_IMAGE_QOI_8_NO_MASK / CIRCLE / ROUND_RECT / IMAGE`
  - `EXTERN_MASK_IMAGE_QOI_8_NO_MASK / CIRCLE / ROUND_RECT / IMAGE`
- 这说明 direct alpha8 split 只是更窄的一层 selector / helper 壳，既没有独立体积，也没有可观测的独立性能面。

### 运行与渲染验证

- `HelloPerformance` on / off 都是 `241 frames`
- 全量 `241` 帧：
  - `hash mismatch 0`
  - `pixel mismatch 0`
- 抽查 `frame_0000.png` / `frame_0120.png` / `frame_0240.png` hash 一致

### 单元测试

- `HelloUnitTest` on：`688/688 passed`
- `HelloUnitTest` off：`688/688 passed`

### 结论

- 临时 `EGUI_CONFIG_IMAGE_QOI_DIRECT_ALPHA8_SPLIT_FAST_PATH_ENABLE` 不保留：
  - `HelloPerformance text 0B`
  - perf / runtime / unit 全量严格等价
- 因此：
  - 代码已回退，不引入这颗临时 child
  - 当前不再沿 `QOI direct alpha8 split` 这条线继续细拆
  - 如果后续还要继续压缩 QOI 侧代码，需要离开这层 direct alpha8 split 壳，转去找别的非共享块

## 本轮继续细拆 A/B: `image qoi direct copy split fast path`

### 测试环境

- 日期：`2026-04-04`
- 提交基线：`bd51366`
- 临时 child
  - `EGUI_CONFIG_IMAGE_QOI_DIRECT_COPY_SPLIT_FAST_PATH_ENABLE`
- `HelloPerformance` code size A/B
  - `make -B -j APP=HelloPerformance PORT=qemu CPU_ARCH=cortex-m0plus APP_OBJ_SUFFIX=hpqoidirectcopyspliton USER_CFLAGS=-DEGUI_CONFIG_IMAGE_QOI_DIRECT_COPY_SPLIT_FAST_PATH_ENABLE=1`
  - `make -B -j APP=HelloPerformance PORT=qemu CPU_ARCH=cortex-m0plus APP_OBJ_SUFFIX=hpqoidirectcopysplitoff USER_CFLAGS=-DEGUI_CONFIG_IMAGE_QOI_DIRECT_COPY_SPLIT_FAST_PATH_ENABLE=0`
- `HelloPerformance` perf A/B
  - `python scripts/perf_analysis/code_perf_check.py --profile cortex-m3 --clean --threshold 1000 --timeout 600 --extra-cflags=-DEGUI_CONFIG_IMAGE_QOI_DIRECT_COPY_SPLIT_FAST_PATH_ENABLE=1`
  - `python scripts/perf_analysis/code_perf_check.py --profile cortex-m3 --clean --threshold 1000 --timeout 600 --extra-cflags=-DEGUI_CONFIG_IMAGE_QOI_DIRECT_COPY_SPLIT_FAST_PATH_ENABLE=0`
- 运行 / 单测验证
  - runtime 输出目录：
    - `runtime_check_output/HelloPerformance/qoidirectcopysplit_on`
    - `runtime_check_output/HelloPerformance/qoidirectcopysplit_off`
  - `make -j APP=HelloUnitTest PORT=pc_test APP_OBJ_SUFFIX=utqoidirectcopyspliton USER_CFLAGS=-DEGUI_CONFIG_IMAGE_QOI_DIRECT_COPY_SPLIT_FAST_PATH_ENABLE=1`
  - `make -j APP=HelloUnitTest PORT=pc_test APP_OBJ_SUFFIX=utqoidirectcopysplitoff USER_CFLAGS=-DEGUI_CONFIG_IMAGE_QOI_DIRECT_COPY_SPLIT_FAST_PATH_ENABLE=0`
  - `output/main.exe`

### 代码量证据

- `HelloPerformance` 链接汇总：
  - on：`text=2055380 data=72 bss=3832`
  - off：`text=2055380 data=72 bss=3832`
- 结论：
  - `text 0B`
  - `data/bss 0B`
- 说明这颗 child 只覆盖 QOI tail-row-cache 里的 direct `RGB565` visible-span split-decode 直连 copy 分支，而真正的 `egui_image_qoi_decode_row_rgb565_split()` 仍是共享 helper，因此这层没有独立 code-size 回收价值。

### 性能证据

- `HelloPerformance` 完整 `239` 场景 on / off 全量一致，没有任何 `>=10%` 波动
- 基础主路径保持 `+0.0%`：
  - `RECTANGLE / CIRCLE / ROUND_RECTANGLE / TEXT`
  - `IMAGE_565 / EXTERN_IMAGE_565 / IMAGE_RESIZE_565 / EXTERN_IMAGE_RESIZE_565`
- QOI 无 alpha 主路径与对照组也保持 `+0.0%`：
  - `IMAGE_QOI_565 / EXTERN_IMAGE_QOI_565`
  - `MASK_IMAGE_QOI_NO_MASK / CIRCLE / ROUND_RECT / IMAGE`
  - `EXTERN_MASK_IMAGE_QOI_NO_MASK / CIRCLE / ROUND_RECT / IMAGE`
- 这说明 direct copy split 和 direct alpha8 split 一样，都只是 selector 层的 dead split，不是值得保留的独立 QOI child。

### 运行与渲染验证

- `HelloPerformance` on / off 都是 `241 frames`
- 全量 `241` 帧：
  - `hash mismatch 0`
  - `pixel mismatch 0`
- 抽查 `frame_0000.png` / `frame_0120.png` / `frame_0240.png` hash 一致

### 单元测试

- `HelloUnitTest` on：`688/688 passed`
- `HelloUnitTest` off：`688/688 passed`

### 结论

- 临时 `EGUI_CONFIG_IMAGE_QOI_DIRECT_COPY_SPLIT_FAST_PATH_ENABLE` 不保留：
  - `HelloPerformance text 0B`
  - perf / runtime / unit 全量严格等价
- 因此：
  - 代码已回退，不引入这颗临时 child
  - 当前不再沿 `QOI direct copy split` 这条线继续细拆
  - 这组 tail-row-cache / direct-path selector sibling 至少 `masked alpha8 / direct alpha8 / direct copy` 三颗都已经验证为 dead child

## 本轮继续细拆 A/B: `image rle checkpoint`

### 测试环境

- 日期：`2026-04-04`
- 提交基线：`8012268`
- 现成 child
  - `EGUI_CONFIG_IMAGE_RLE_CHECKPOINT_ENABLE`
- `HelloPerformance` code size A/B
  - `make -B -j APP=HelloPerformance PORT=qemu CPU_ARCH=cortex-m0plus APP_OBJ_SUFFIX=hprleckpton USER_CFLAGS=-DEGUI_CONFIG_IMAGE_RLE_CHECKPOINT_ENABLE=1`
  - `make -B -j APP=HelloPerformance PORT=qemu CPU_ARCH=cortex-m0plus APP_OBJ_SUFFIX=hprleckptoff USER_CFLAGS=-DEGUI_CONFIG_IMAGE_RLE_CHECKPOINT_ENABLE=0`
- `HelloPerformance` perf A/B
  - `python scripts/perf_analysis/code_perf_check.py --profile cortex-m3 --clean --threshold 1000 --timeout 600 --extra-cflags=-DEGUI_CONFIG_IMAGE_RLE_CHECKPOINT_ENABLE=1`
  - `python scripts/perf_analysis/code_perf_check.py --profile cortex-m3 --clean --threshold 1000 --timeout 600 --extra-cflags=-DEGUI_CONFIG_IMAGE_RLE_CHECKPOINT_ENABLE=0`
- 运行 / 单测验证
  - runtime 输出目录：
    - `runtime_check_output/HelloPerformance/rleckpt_on`
    - `runtime_check_output/HelloPerformance/rleckpt_off`
  - `make -j APP=HelloUnitTest PORT=pc_test APP_OBJ_SUFFIX=utrleckpton USER_CFLAGS=-DEGUI_CONFIG_IMAGE_RLE_CHECKPOINT_ENABLE=1`
  - `make -j APP=HelloUnitTest PORT=pc_test APP_OBJ_SUFFIX=utrleckptoff USER_CFLAGS=-DEGUI_CONFIG_IMAGE_RLE_CHECKPOINT_ENABLE=0`
  - `output/main.exe`

### 代码量证据

- `HelloPerformance` 链接汇总：
  - on：`text=2055380 data=72 bss=3832`
  - off：`text=2055288 data=72 bss=3816`
- 结论：
  - `text -92B`
  - `bss -16B`
- 说明这颗 child 只覆盖 RLE row-band checkpoint 的 save/restore 状态，本身独立体积极薄。

### 性能证据

- `HelloPerformance` 完整 `239` 场景里有 `20` 个场景回退达到 `>=10%`
- 基础主路径保持不变：
  - `RECTANGLE / CIRCLE / ROUND_RECTANGLE / TEXT`
  - `IMAGE_565 / EXTERN_IMAGE_565 / IMAGE_RESIZE_565 / EXTERN_IMAGE_RESIZE_565`
- 但整条 RLE draw 主路径都会明显退化：
  - `IMAGE_RLE_565 / MASK_IMAGE_RLE_NO_MASK +238.4%`
  - `MASK_IMAGE_RLE_ROUND_RECT +217.8%`
  - `MASK_IMAGE_RLE_CIRCLE +206.7%`
  - `EXTERN_IMAGE_RLE_565 / EXTERN_MASK_IMAGE_RLE_NO_MASK +452.0%`
  - `EXTERN_MASK_IMAGE_RLE_ROUND_RECT +431.8%`
  - `EXTERN_MASK_IMAGE_RLE_CIRCLE +420.0%`
  - `IMAGE_RLE_565_8 / MASK_IMAGE_RLE_8_NO_MASK +109.5%`
  - `EXTERN_IMAGE_RLE_565_8 / EXTERN_MASK_IMAGE_RLE_8_NO_MASK +282.1%`
  - `MASK_IMAGE_RLE_IMAGE +48.1%`
  - `EXTERN_MASK_IMAGE_RLE_IMAGE +115.4%`
- 这说明 RLE checkpoint 也不是可安全剥离的薄壳，而是避免重复解码的关键主路径块。

### 运行与渲染验证

- `HelloPerformance` on / off 都是 `241 frames`
- 全量 `241` 帧：
  - `hash mismatch 0`
  - `pixel mismatch 0`
- 抽查 `frame_0000.png` / `frame_0120.png` / `frame_0240.png` hash 一致

### 单元测试

- `HelloUnitTest` on：`688/688 passed`
- `HelloUnitTest` off：`688/688 passed`

### 结论

- `EGUI_CONFIG_IMAGE_RLE_CHECKPOINT_ENABLE` 不建议关闭：
  - `HelloPerformance text -92B, bss -16B`
  - 但 perf 会把整条 RLE draw 主路径打到 `+32.1% ~ +452.0%`
- 因此：
  - 当前不再沿 `RLE checkpoint` 这条线继续细拆
  - 如果后续还要继续压缩 RLE 侧代码，应优先找比 checkpoint 更窄、且不是 decode 主路径块的分支

## 本轮继续细拆 A/B: `image rle direct copy split fast path`

### 测试环境

- 日期：`2026-04-04`
- 提交基线：`0b98eaa`
- 临时 child
  - `EGUI_CONFIG_IMAGE_RLE_DIRECT_COPY_SPLIT_FAST_PATH_ENABLE`
- `HelloPerformance` code size A/B
  - `make -B -j APP=HelloPerformance PORT=qemu CPU_ARCH=cortex-m0plus APP_OBJ_SUFFIX=hprledirectcopyspliton USER_CFLAGS=-DEGUI_CONFIG_IMAGE_RLE_DIRECT_COPY_SPLIT_FAST_PATH_ENABLE=1`
  - `make -B -j APP=HelloPerformance PORT=qemu CPU_ARCH=cortex-m0plus APP_OBJ_SUFFIX=hprledirectcopysplitoff USER_CFLAGS=-DEGUI_CONFIG_IMAGE_RLE_DIRECT_COPY_SPLIT_FAST_PATH_ENABLE=0`
- `HelloPerformance` perf A/B
  - `python scripts/perf_analysis/code_perf_check.py --profile cortex-m3 --clean --threshold 1000 --timeout 600 --extra-cflags=-DEGUI_CONFIG_IMAGE_RLE_DIRECT_COPY_SPLIT_FAST_PATH_ENABLE=1`
  - `python scripts/perf_analysis/code_perf_check.py --profile cortex-m3 --clean --threshold 1000 --timeout 600 --extra-cflags=-DEGUI_CONFIG_IMAGE_RLE_DIRECT_COPY_SPLIT_FAST_PATH_ENABLE=0`
- 运行 / 单测验证
  - runtime 输出目录：
    - `runtime_check_output/HelloPerformance/rledirectcopysplit_on`
    - `runtime_check_output/HelloPerformance/rledirectcopysplit_off`
  - `make -j APP=HelloUnitTest PORT=pc_test APP_OBJ_SUFFIX=utrledirectcopyspliton USER_CFLAGS=-DEGUI_CONFIG_IMAGE_RLE_DIRECT_COPY_SPLIT_FAST_PATH_ENABLE=1`
  - `make -j APP=HelloUnitTest PORT=pc_test APP_OBJ_SUFFIX=utrledirectcopysplitoff USER_CFLAGS=-DEGUI_CONFIG_IMAGE_RLE_DIRECT_COPY_SPLIT_FAST_PATH_ENABLE=0`
  - `output/main.exe`

### 代码量证据

- `HelloPerformance` 链接汇总：
  - on：`text=2055380 data=72 bss=3832`
  - off：`text=2055380 data=72 bss=3832`
- 结论：
  - `text 0B`
  - `data/bss 0B`
- 说明这颗 child 只覆盖 RLE tail-row-cache 里的 direct `RGB565` visible-span split-decode 直连 copy 分支，而真正的 `egui_image_rle_decompress_row_split()` 仍是共享 helper，因此这层没有独立 code-size 回收价值。

### 性能证据

- `HelloPerformance` 完整 `239` 场景 on / off 全量一致，没有任何 `>=10%` 波动
- 基础主路径保持 `+0.0%`：
  - `RECTANGLE / CIRCLE / ROUND_RECTANGLE / TEXT`
  - `IMAGE_565 / EXTERN_IMAGE_565 / IMAGE_RESIZE_565 / EXTERN_IMAGE_RESIZE_565`
- RLE 无 alpha 主路径与对照组也保持 `+0.0%`：
  - `IMAGE_RLE_565 / EXTERN_IMAGE_RLE_565`
  - `MASK_IMAGE_RLE_NO_MASK / CIRCLE / ROUND_RECT / IMAGE`
  - `EXTERN_MASK_IMAGE_RLE_NO_MASK / CIRCLE / ROUND_RECT / IMAGE`
- 这说明 direct copy split 在 `RLE` 侧也只是 selector 层的 dead split，不是值得保留的独立 child。

### 运行与渲染验证

- `HelloPerformance` on / off 都是 `241 frames`
- 全量 `241` 帧：
  - `hash mismatch 0`
  - `pixel mismatch 0`
- 抽查 `frame_0000.png` / `frame_0120.png` / `frame_0240.png` hash 一致

### 单元测试

- `HelloUnitTest` on：`688/688 passed`
- `HelloUnitTest` off：`688/688 passed`

### 结论

- 临时 `EGUI_CONFIG_IMAGE_RLE_DIRECT_COPY_SPLIT_FAST_PATH_ENABLE` 不保留：
  - `HelloPerformance text 0B`
  - perf / runtime / unit 全量严格等价
- 因此：
  - 代码已回退，不引入这颗临时 child
  - 当前不再沿 `RLE direct copy split` 这条线继续细拆
  - `RLE` 侧当前已经补齐 `checkpoint` 与 `direct copy split` 两层结论：前者是性能关键主路径块，后者是 dead child

## 本轮补齐 A/B: `decode opaque alpha row use row cache`

### 测试环境

- 日期：`2026-04-05`
- 提交基线：`c6b6b9c`
- 现成框架宏
  - `EGUI_CONFIG_IMAGE_DECODE_OPAQUE_ALPHA_ROW_USE_ROW_CACHE`
- `HelloPerformance` code size A/B
  - `make -B -j APP=HelloPerformance PORT=qemu CPU_ARCH=cortex-m0plus APP_OBJ_SUFFIX=hpopaquealpharowcacherecheckon USER_CFLAGS=-DEGUI_CONFIG_IMAGE_DECODE_OPAQUE_ALPHA_ROW_USE_ROW_CACHE=1`
  - `make -B -j APP=HelloPerformance PORT=qemu CPU_ARCH=cortex-m0plus APP_OBJ_SUFFIX=hpopaquealpharowcacherecheckoff USER_CFLAGS=-DEGUI_CONFIG_IMAGE_DECODE_OPAQUE_ALPHA_ROW_USE_ROW_CACHE=0`
- `HelloPerformance` perf A/B
  - `python scripts/perf_analysis/code_perf_check.py --profile cortex-m3 --clean --threshold 1000 --timeout 600 --extra-cflags=-DEGUI_CONFIG_IMAGE_DECODE_OPAQUE_ALPHA_ROW_USE_ROW_CACHE=1`
  - `python scripts/perf_analysis/code_perf_check.py --profile cortex-m3 --clean --threshold 1000 --timeout 600 --extra-cflags=-DEGUI_CONFIG_IMAGE_DECODE_OPAQUE_ALPHA_ROW_USE_ROW_CACHE=0`
- 运行 / 单测验证
  - runtime 输出目录：
    - `runtime_check_output/HelloPerformance/decode_opaque_alpha_row_use_row_cache_recheck_on`
    - `runtime_check_output/HelloPerformance/decode_opaque_alpha_row_use_row_cache_recheck_off`
  - `make -j APP=HelloUnitTest PORT=pc_test APP_OBJ_SUFFIX=utopaquealpharowcacherecheckon USER_CFLAGS=-DEGUI_CONFIG_IMAGE_DECODE_OPAQUE_ALPHA_ROW_USE_ROW_CACHE=1`
  - `make -j APP=HelloUnitTest PORT=pc_test APP_OBJ_SUFFIX=utopaquealpharowcacherecheckoff USER_CFLAGS=-DEGUI_CONFIG_IMAGE_DECODE_OPAQUE_ALPHA_ROW_USE_ROW_CACHE=0`
  - `output/main.exe`

### 代码量证据

- `HelloPerformance` 链接汇总：
  - on：`text=2055248 data=72 bss=3824`
  - off：`text=2055380 data=72 bss=3832`
- 结论：
  - `1 -> 0` 反向等价于 `text +132B, bss +8B`
  - 因而 `1` 相比 `0` 为 `text -132B, bss -8B`

### 性能证据

- `HelloPerformance` 完整 `239` 场景：
  - `ge10=0`
  - `le10=0`
- 最大绝对波动仅 `0.431%`
  - `TEXT_GRADIENT 0.232 -> 0.233 (+0.431%)`
  - `MASK_IMAGE_NO_MASK_QUARTER 0.233 -> 0.232 (-0.429%)`
  - `CIRCLE_FILL_QUARTER 0.256 -> 0.257 (+0.391%)`
- 基础主路径保持不变：
  - `RECTANGLE / CIRCLE / ROUND_RECTANGLE / TEXT`
  - `IMAGE_565 / EXTERN_IMAGE_565 / IMAGE_RESIZE_565 / EXTERN_IMAGE_RESIZE_565`
  - 上述场景全部 `+0.0%`

### 运行与渲染验证

- `HelloPerformance` on / off 都是 `241 frames`
- 全量 `241` 帧：
  - `hash mismatch 0`
  - `pixel mismatch 0`
- 抽查 `frame_0000.png` / `frame_0120.png` / `frame_0240.png` hash 一致

### 单元测试

- `HelloUnitTest` on：`688/688 passed`
- `HelloUnitTest` off：`688/688 passed`

### 结论

- `EGUI_CONFIG_IMAGE_DECODE_OPAQUE_ALPHA_ROW_USE_ROW_CACHE` 当前不该再写成 `HelloPerformance=1`
- 它的收益只有 `text -132B, bss -8B`
- 但完整 `239` 场景 perf、runtime `241/241` 和 `HelloUnitTest 688/688` 都保持等价
- 因此：
  - current shipped/default 继续保持 `0`
  - 同时保留外部覆盖入口，把 `1` 视为收益很小、行为稳定的 low-RAM 条件实验值

## 本轮补齐 A/B: `rle external cache window size`

### 测试环境

- 日期：`2026-04-05`
- 提交基线：`d1be566`
- 现成框架宏
  - `EGUI_CONFIG_IMAGE_RLE_EXTERNAL_CACHE_WINDOW_SIZE`
- `HelloPerformance` code size A/B
  - `make -B -j APP=HelloPerformance PORT=qemu CPU_ARCH=cortex-m0plus APP_OBJ_SUFFIX=hprlewindowrecheckon USER_CFLAGS=-DEGUI_CONFIG_IMAGE_RLE_EXTERNAL_CACHE_WINDOW_SIZE=1024`
  - `make -B -j APP=HelloPerformance PORT=qemu CPU_ARCH=cortex-m0plus APP_OBJ_SUFFIX=hprlewindowrecheckoff USER_CFLAGS=-DEGUI_CONFIG_IMAGE_RLE_EXTERNAL_CACHE_WINDOW_SIZE=64`
- `HelloPerformance` perf A/B
  - `python scripts/perf_analysis/code_perf_check.py --profile cortex-m3 --clean --threshold 1000 --timeout 600 --extra-cflags=-DEGUI_CONFIG_IMAGE_RLE_EXTERNAL_CACHE_WINDOW_SIZE=1024`
  - `python scripts/perf_analysis/code_perf_check.py --profile cortex-m3 --clean --threshold 1000 --timeout 600 --extra-cflags=-DEGUI_CONFIG_IMAGE_RLE_EXTERNAL_CACHE_WINDOW_SIZE=64`
- 运行 / 单测验证
  - runtime 输出目录：
    - `runtime_check_output/HelloPerformance/rle_external_cache_window_recheck_on`
    - `runtime_check_output/HelloPerformance/rle_external_cache_window_recheck_off`
  - `make -j APP=HelloUnitTest PORT=pc_test APP_OBJ_SUFFIX=utrlewindowrecheckon USER_CFLAGS=-DEGUI_CONFIG_IMAGE_RLE_EXTERNAL_CACHE_WINDOW_SIZE=1024`
  - `make -j APP=HelloUnitTest PORT=pc_test APP_OBJ_SUFFIX=utrlewindowrecheckoff USER_CFLAGS=-DEGUI_CONFIG_IMAGE_RLE_EXTERNAL_CACHE_WINDOW_SIZE=64`
  - `output/main.exe`

### 代码量证据

- `HelloPerformance` 链接汇总：
  - on：`text=2055380 data=72 bss=3832`
  - off：`text=2055476 data=72 bss=2808`
- 结论：
  - `1024 -> 64` 后是 `text +96B, bss -1024B`

### 性能证据

- `HelloPerformance` 完整 `239` 场景：
  - `ge10=5`
  - `le10=0`
- 基础主路径保持不变：
  - `RECTANGLE / CIRCLE / ROUND_RECTANGLE / TEXT`
  - `IMAGE_565 / EXTERN_IMAGE_565 / IMAGE_RESIZE_565 / EXTERN_IMAGE_RESIZE_565`
  - 上述场景全部 `+0.0%`
- 回退全部集中在 external RLE 主路径及其 masked 变体：
  - `EXTERN_IMAGE_RLE_565 / EXTERN_MASK_IMAGE_RLE_NO_MASK +21.8%`
  - `EXTERN_MASK_IMAGE_RLE_ROUND_RECT +20.8%`
  - `EXTERN_MASK_IMAGE_RLE_CIRCLE +20.3%`
  - `EXTERN_MASK_IMAGE_RLE_IMAGE +17.5%`
  - `EXTERN_IMAGE_RLE_565_8 / EXTERN_MASK_IMAGE_RLE_8_NO_MASK +9.4%`
- 这说明 `64` 仍是有效的 low-RAM 值，但不再适合作为当前 shipped/default。

### 运行与渲染验证

- `HelloPerformance` on / off 都是 `241 frames`
- 全量 `241` 帧：
  - `hash mismatch 0`
  - `pixel mismatch 0`
- 抽查 `frame_0000.png` / `frame_0120.png` / `frame_0240.png` hash 一致

### 单元测试

- `HelloUnitTest` on：`688/688 passed`
- `HelloUnitTest` off：`688/688 passed`

### 结论

- `EGUI_CONFIG_IMAGE_RLE_EXTERNAL_CACHE_WINDOW_SIZE` 当前不该再被写成 `HelloPerformance=64`
- `64` 仍然是一个有效的 low-RAM 条件实验值，因为它能稳定回收 `bss -1024B`
- 但它不再适合作为当前 shipped/default：external RLE 主路径会出现 `+17.5% ~ +21.8%` 的明显回退
- 因此：
  - current shipped/default 继续保持 `1024`
  - `64` 只在项目明确不关心 external RLE 热点时再考虑启用

## 本轮补齐 A/B: `qoi checkpoint count`

### 测试环境

- 日期：`2026-04-05`
- 提交基线：`387823d`
- 现成框架宏
  - `EGUI_CONFIG_IMAGE_QOI_CHECKPOINT_COUNT`
- `HelloPerformance` code size A/B
  - `make -B -j APP=HelloPerformance PORT=qemu CPU_ARCH=cortex-m0plus APP_OBJ_SUFFIX=hpqoicheckpointrecheckon USER_CFLAGS=-DEGUI_CONFIG_IMAGE_QOI_CHECKPOINT_COUNT=2`
  - `make -B -j APP=HelloPerformance PORT=qemu CPU_ARCH=cortex-m0plus APP_OBJ_SUFFIX=hpqoicheckpointrecheckoff USER_CFLAGS=-DEGUI_CONFIG_IMAGE_QOI_CHECKPOINT_COUNT=0`
- `HelloPerformance` perf A/B
  - `python scripts/perf_analysis/code_perf_check.py --profile cortex-m3 --clean --threshold 1000 --timeout 600 --extra-cflags=-DEGUI_CONFIG_IMAGE_QOI_CHECKPOINT_COUNT=2`
  - `python scripts/perf_analysis/code_perf_check.py --profile cortex-m3 --clean --threshold 1000 --timeout 600 --extra-cflags=-DEGUI_CONFIG_IMAGE_QOI_CHECKPOINT_COUNT=0`
- 运行 / 单测验证
  - runtime 输出目录：
    - `runtime_check_output/HelloPerformance/qoi_checkpoint_count_recheck_on`
    - `runtime_check_output/HelloPerformance/qoi_checkpoint_count_recheck_off`
  - `make -j APP=HelloUnitTest PORT=pc_test APP_OBJ_SUFFIX=utqoicheckpointrecheckon USER_CFLAGS=-DEGUI_CONFIG_IMAGE_QOI_CHECKPOINT_COUNT=2`
  - `make -j APP=HelloUnitTest PORT=pc_test APP_OBJ_SUFFIX=utqoicheckpointrecheckoff USER_CFLAGS=-DEGUI_CONFIG_IMAGE_QOI_CHECKPOINT_COUNT=0`
  - `output/main.exe`

### 代码量证据

- `HelloPerformance` 链接汇总：
  - on：`text=2055380 data=72 bss=3832`
  - off：`text=2055104 data=72 bss=3824`
- 结论：
  - `2 -> 0` 为 `text -276B`
  - `bss -8B`

### 性能证据

- `HelloPerformance` 完整 `239` 场景里有 `20` 个 `>=10%` 回退，没有任何 `<=-10%` 改善
- 基础主路径保持不变：
  - `RECTANGLE / CIRCLE / ROUND_RECTANGLE / TEXT`
  - `IMAGE_565 / EXTERN_IMAGE_565 / IMAGE_RESIZE_565 / EXTERN_IMAGE_RESIZE_565`
  - 上述场景全部 `+0.0%`
- 回退全部集中在 QOI draw 主路径及其 masked 变体：
  - `EXTERN_IMAGE_QOI_565 / EXTERN_MASK_IMAGE_QOI_NO_MASK +911.4%`
  - `EXTERN_MASK_IMAGE_QOI_ROUND_RECT +898.6%`
  - `EXTERN_MASK_IMAGE_QOI_CIRCLE +890.8%`
  - `IMAGE_QOI_565 / MASK_IMAGE_QOI_NO_MASK +819.2%`
  - `MASK_IMAGE_QOI_IMAGE +250.6%`
  - `MASK_IMAGE_QOI_8_IMAGE +110.0%`
- 这说明 `QOI_CHECKPOINT_COUNT=0` 虽然能回收少量 text / bss，但会直接打穿当前 QOI 主路径。

### 运行与渲染验证

- `HelloPerformance` on / off 都是 `241 frames`
- 全量 `241` 帧：
  - `hash mismatch 0`
  - `pixel mismatch 0`
- 抽查 `frame_0000.png` / `frame_0120.png` / `frame_0240.png` hash 一致

### 单元测试

- `HelloUnitTest` on：`688/688 passed`
- `HelloUnitTest` off：`688/688 passed`

### 结论

- `EGUI_CONFIG_IMAGE_QOI_CHECKPOINT_COUNT` 当前不应再被写成 `HelloPerformance=0`
- `2 -> 0` 虽然能回收 `HelloPerformance text -276B, bss -8B`
- 但它会让完整 `239` 场景里的 `20` 个 QOI 热点出现 `+110.0% ~ +911.4%` 回退
- 因此：
  - current shipped/default 继续保持 `2`
  - 历史实验值 `0` 只保留为外部条件实验入口，不再作为 `HelloPerformance` 的推荐 low-RAM 值

## 本轮继续细拆 A/B: `std image external row persistent cache`

### 测试环境

- 日期：`2026-04-05`
- 提交基线：`3104561`
- 现成 child
  - `EGUI_CONFIG_IMAGE_STD_EXTERNAL_ROW_PERSISTENT_CACHE_ENABLE`
- `HelloPerformance` code size A/B
  - `make -B -j APP=HelloPerformance PORT=qemu CPU_ARCH=cortex-m0plus APP_OBJ_SUFFIX=hpstdextrowpersistrecheckon USER_CFLAGS=-DEGUI_CONFIG_IMAGE_STD_EXTERNAL_ROW_PERSISTENT_CACHE_ENABLE=1`
  - `make -B -j APP=HelloPerformance PORT=qemu CPU_ARCH=cortex-m0plus APP_OBJ_SUFFIX=hpstdextrowpersistrecheckoff USER_CFLAGS=-DEGUI_CONFIG_IMAGE_STD_EXTERNAL_ROW_PERSISTENT_CACHE_ENABLE=0`
- `HelloPerformance` perf A/B
  - `python scripts/perf_analysis/code_perf_check.py --profile cortex-m3 --clean --threshold 1000 --timeout 600 --extra-cflags=-DEGUI_CONFIG_IMAGE_STD_EXTERNAL_ROW_PERSISTENT_CACHE_ENABLE=1`
  - `python scripts/perf_analysis/code_perf_check.py --profile cortex-m3 --clean --threshold 1000 --timeout 600 --extra-cflags=-DEGUI_CONFIG_IMAGE_STD_EXTERNAL_ROW_PERSISTENT_CACHE_ENABLE=0`
- 运行 / 单测验证
  - runtime 输出目录：
    - `runtime_check_output/HelloPerformance/std_external_row_persistent_recheck_on`
    - `runtime_check_output/HelloPerformance/std_external_row_persistent_recheck_off`
  - `make -j APP=HelloUnitTest PORT=pc_test APP_OBJ_SUFFIX=utstdextrowpersistrecheckon USER_CFLAGS=-DEGUI_CONFIG_IMAGE_STD_EXTERNAL_ROW_PERSISTENT_CACHE_ENABLE=1`
  - `make -j APP=HelloUnitTest PORT=pc_test APP_OBJ_SUFFIX=utstdextrowpersistrecheckoff USER_CFLAGS=-DEGUI_CONFIG_IMAGE_STD_EXTERNAL_ROW_PERSISTENT_CACHE_ENABLE=0`
  - `output/main.exe`

### 代码量证据

- `HelloPerformance` 链接汇总：
  - on：`text=2055380 data=72 bss=3832`
  - off：`text=2055400 data=72 bss=3792`
- 结论：
  - `text +20B`
  - `bss -40B`
- 说明这颗宏不是 text-first split，而是一个轻微增代码、换少量静态 RAM 的 low-RAM 条件宏。

### 性能证据

- `HelloPerformance` 完整 `239` 场景 on / off 全量都在噪声内，没有任何 `>=10%` 波动
- 基础主路径保持在 `0% ~ +1.7%`：
  - `RECTANGLE / CIRCLE / ROUND_RECTANGLE / TEXT`
  - `IMAGE_565 / EXTERN_IMAGE_565 / IMAGE_RESIZE_565 / EXTERN_IMAGE_RESIZE_565`
- external image tiled / resize 对照组最大绝对波动也只有 `2.467%`：
  - `EXTERN_IMAGE_RESIZE_TILED_565_0 +2.467%`
  - `EXTERN_IMAGE_TILED_565_1 +1.813%`
  - `EXTERN_IMAGE_RESIZE_TILED_565_1 +1.776%`
- external image / codec 对照组继续保持在噪声内：
  - `EXTERN_IMAGE_565_1 / EXTERN_IMAGE_565_8`
  - `EXTERN_IMAGE_RESIZE_565_1 / EXTERN_IMAGE_RESIZE_565_8`
  - `EXTERN_IMAGE_QOI_565 / EXTERN_IMAGE_RLE_565`
  - `EXTERN_MASK_IMAGE_QOI_IMAGE / EXTERN_MASK_IMAGE_RLE_IMAGE`
- 这说明关闭 persistent row cache 不会打穿当前 HelloPerformance 的 external image 主路径，但它的收益也主要是 `bss -40B`，不是 text 回收。

### 运行与渲染验证

- `HelloPerformance` on / off 都是 `241 frames`
- 全量 `241` 帧：
  - `hash mismatch 0`
  - `pixel mismatch 0`
- 抽查 `frame_0000.png` / `frame_0120.png` / `frame_0240.png` hash 一致

### 单元测试

- `HelloUnitTest` on：`688/688 passed`
- `HelloUnitTest` off：`688/688 passed`

### 结论

- `EGUI_CONFIG_IMAGE_STD_EXTERNAL_ROW_PERSISTENT_CACHE_ENABLE` 可保留为低优先级 low-RAM 条件实验宏：
  - `HelloPerformance text +20B, bss -40B`
  - perf / runtime / unit 全量通过
- 因此：
  - 它不进入当前 text-first split 主线
  - 如果项目的目标是再榨一点静态 RAM，而不是追求 text 回收，可以考虑关闭它
  - 但在 image/external 侧的 low-RAM 条件宏里，它的优先级仍低于 `TRANSFORM_EXTERNAL_ROW_PERSISTENT_CACHE_ENABLE`

## 本轮继续细拆 A/B: `transform external row persistent cache`

### 测试环境

- 日期：`2026-04-05`
- 提交基线：`cf0e1a3`
- 现成 child
  - `EGUI_CONFIG_IMAGE_TRANSFORM_EXTERNAL_ROW_PERSISTENT_CACHE_ENABLE`
- `HelloPerformance` code size A/B
  - `make -B -j APP=HelloPerformance PORT=qemu CPU_ARCH=cortex-m0plus APP_OBJ_SUFFIX=hptransformextrowpersistrecheckon USER_CFLAGS=-DEGUI_CONFIG_IMAGE_TRANSFORM_EXTERNAL_ROW_PERSISTENT_CACHE_ENABLE=1`
  - `make -B -j APP=HelloPerformance PORT=qemu CPU_ARCH=cortex-m0plus APP_OBJ_SUFFIX=hptransformextrowpersistrecheckoff USER_CFLAGS=-DEGUI_CONFIG_IMAGE_TRANSFORM_EXTERNAL_ROW_PERSISTENT_CACHE_ENABLE=0`
- `HelloPerformance` perf A/B
  - `python scripts/perf_analysis/code_perf_check.py --profile cortex-m3 --clean --threshold 1000 --timeout 600 --extra-cflags=-DEGUI_CONFIG_IMAGE_TRANSFORM_EXTERNAL_ROW_PERSISTENT_CACHE_ENABLE=1`
  - `python scripts/perf_analysis/code_perf_check.py --profile cortex-m3 --clean --threshold 1000 --timeout 600 --extra-cflags=-DEGUI_CONFIG_IMAGE_TRANSFORM_EXTERNAL_ROW_PERSISTENT_CACHE_ENABLE=0`
- 运行 / 单测验证
  - runtime 输出目录：
    - `runtime_check_output/HelloPerformance/transform_external_row_persistent_recheck_on`
    - `runtime_check_output/HelloPerformance/transform_external_row_persistent_recheck_off`
  - `make -j APP=HelloUnitTest PORT=pc_test APP_OBJ_SUFFIX=uttransformextrowpersistrecheckon USER_CFLAGS=-DEGUI_CONFIG_IMAGE_TRANSFORM_EXTERNAL_ROW_PERSISTENT_CACHE_ENABLE=1`
  - `make -j APP=HelloUnitTest PORT=pc_test APP_OBJ_SUFFIX=uttransformextrowpersistrecheckoff USER_CFLAGS=-DEGUI_CONFIG_IMAGE_TRANSFORM_EXTERNAL_ROW_PERSISTENT_CACHE_ENABLE=0`
  - `output/main.exe`

### 代码量证据

- `HelloPerformance` 链接汇总：
  - on：`text=2055380 data=72 bss=3832`
  - off：`text=2055356 data=72 bss=3776`
- 结论：
  - `text -24B`
  - `bss -56B`
- 说明这颗宏属于低 RAM 条件宏，而且和 `std external row persistent cache` 不同，它这次连 text 也略有回收。

### 性能证据

- `HelloPerformance` 完整 `239` 场景 on / off 全量都在噪声内，没有任何 `>=10%` 波动
- 基础主路径保持在 `+0.0% ~ +0.1%`：
  - `RECTANGLE / CIRCLE / ROUND_RECTANGLE / TEXT`
  - `IMAGE_565 / EXTERN_IMAGE_565 / IMAGE_RESIZE_565 / EXTERN_IMAGE_RESIZE_565`
- transform/external rotate 对照组最大绝对波动也只有 `1.316%`：
  - `IMAGE_ROTATE_TILED_565_0 +0.791%`
  - `IMAGE_ROTATE_TILED_565_8 +0.565%`
  - `EXTERN_IMAGE_ROTATE_565_1 -1.316%`
  - `EXTERN_IMAGE_ROTATE_TILED_565_8 -1.035%`
- 这说明关闭 transform external row persistent cache 不会打穿当前 transform/external 主路径。

### 运行与渲染验证

- `HelloPerformance` on / off 都是 `241 frames`
- 全量 `241` 帧：
  - `hash mismatch 0`
  - `pixel mismatch 0`
- 抽查 `frame_0000.png` / `frame_0120.png` / `frame_0240.png` hash 一致

### 单元测试

- `HelloUnitTest` on：`688/688 passed`
- `HelloUnitTest` off：`688/688 passed`

### 结论

- `EGUI_CONFIG_IMAGE_TRANSFORM_EXTERNAL_ROW_PERSISTENT_CACHE_ENABLE` 可保留为低优先级 low-RAM 条件实验宏：
  - `HelloPerformance text -24B, bss -56B`
  - perf / runtime / unit 全量通过
- 因此：
  - 它不进入当前 text-first split 主线
  - 如果项目要在 image/external 侧先榨一点静态 RAM，这颗宏应优先级高于 `STD_EXTERNAL_ROW_PERSISTENT_CACHE_ENABLE`

## 本轮补齐 A/B: `image rle external window persistent cache`

### 测试环境

- 日期：`2026-04-05`
- 提交基线：`1eec090`
- 现成宏
  - `EGUI_CONFIG_IMAGE_RLE_EXTERNAL_WINDOW_PERSISTENT_CACHE_ENABLE`
- `HelloPerformance` code size A/B
  - on：`make -B -j APP=HelloPerformance PORT=qemu CPU_ARCH=cortex-m0plus APP_OBJ_SUFFIX=hprlepersiston USER_CFLAGS=-DEGUI_CONFIG_IMAGE_RLE_EXTERNAL_WINDOW_PERSISTENT_CACHE_ENABLE=1`
  - off：`make -B -j APP=HelloPerformance PORT=qemu CPU_ARCH=cortex-m0plus APP_OBJ_SUFFIX=hprlepersistoff USER_CFLAGS=-DEGUI_CONFIG_IMAGE_RLE_EXTERNAL_WINDOW_PERSISTENT_CACHE_ENABLE=0`
- `HelloPerformance` perf A/B
  - on：`python scripts/perf_analysis/code_perf_check.py --profile cortex-m3 --clean --threshold 1000 --timeout 600 --extra-cflags=-DEGUI_CONFIG_IMAGE_RLE_EXTERNAL_WINDOW_PERSISTENT_CACHE_ENABLE=1`
  - off：`python scripts/perf_analysis/code_perf_check.py --profile cortex-m3 --clean --threshold 1000 --timeout 600 --extra-cflags=-DEGUI_CONFIG_IMAGE_RLE_EXTERNAL_WINDOW_PERSISTENT_CACHE_ENABLE=0`
- 运行 / 单测验证
  - runtime 输出目录：
    - `runtime_check_output/HelloPerformance/rle_external_window_persistent_cache_enable_on`
    - `runtime_check_output/HelloPerformance/rle_external_window_persistent_cache_enable_off`
  - `HelloUnitTest` on / off 都已跑通

### 代码 / 静态 RAM 证据

- `HelloPerformance` 链接汇总：
  - on：`text=2055380 data=72 bss=3832`
  - off：`text=2056196 data=72 bss=2792`
- 结论：
  - `1 -> 0` 会带来 `text +816B`
  - 同时回收 `bss -1040B`
- 这颗宏的本质不是 text-first split，而是把 external RLE read window 从 persistent BSS 挪到 transient frame heap。

### 性能证据

- `HelloPerformance` 完整 `239` 场景对比
  - `>=10%` 回退：`0`
  - `<=-10%` 改善：`0`
  - 最大绝对波动：`IMAGE_TILED_RLE_565_0 +6.183%`
- 基础主路径都在噪声范围：
  - `RECTANGLE / CIRCLE / ROUND_RECTANGLE / TEXT`
  - `IMAGE_565 / EXTERN_IMAGE_565 / IMAGE_RESIZE_565 / EXTERN_IMAGE_RESIZE_565`
- internal RLE 路径基本保持不变：
  - `IMAGE_RLE_565 / IMAGE_RLE_565_8 / MASK_IMAGE_RLE_*` 基本都在 `+0.4%` 内
  - `IMAGE_TILED_RLE_565_0 +6.183%`
  - `IMAGE_TILED_RLE_565_8 +2.834%`
- external RLE 主路径与 masked 变体出现局部小幅回退：
  - `EXTERN_IMAGE_RLE_565 +5.070%`
  - `EXTERN_MASK_IMAGE_RLE_NO_MASK +5.070%`
  - `EXTERN_MASK_IMAGE_RLE_ROUND_RECT +4.843%`
  - `EXTERN_MASK_IMAGE_RLE_CIRCLE +4.711%`
  - `EXTERN_MASK_IMAGE_RLE_IMAGE +3.664%`
  - `EXTERN_IMAGE_RLE_565_8 / EXTERN_MASK_IMAGE_RLE_8_NO_MASK +3.149%`
  - `EXTERN_MASK_IMAGE_RLE_8_ROUND_RECT +2.811%`
  - `EXTERN_MASK_IMAGE_RLE_8_IMAGE +2.710%`
  - `EXTERN_MASK_IMAGE_RLE_8_CIRCLE +2.707%`

### 运行与渲染验证

- `HelloPerformance` on / off 都是 `241 frames`
- 全量 `241` 帧：
  - `hash mismatch 0`
  - `pixel mismatch 0`
- 抽查 `frame_0000.png` / `frame_0120.png` / `frame_0240.png` hash 一致

### 单元测试

- `HelloUnitTest` on：`688/688 passed`
- `HelloUnitTest` off：`688/688 passed`

### 结论

- `EGUI_CONFIG_IMAGE_RLE_EXTERNAL_WINDOW_PERSISTENT_CACHE_ENABLE` 可保留为 low-RAM 条件实验宏：
  - `1 -> 0` 的收益是 `bss -1040B`
  - 但代价是 `text +816B`
  - tiled / external RLE 路径仍会出现 `+2.7% ~ +6.2%` 的局部回退
- 因此：
  - current shipped/default 继续保持 `1`
  - 它不进入当前 text-first split 主线
  - 如果项目目标是把 external RLE window 从 persistent BSS 切到 transient frame heap，且能接受这组局部回退，仍可按需试 `0`

## 后续候选

- `mask_get_row_visible_range()` 这条线已经拆出一颗调用侧宏：`EGUI_CONFIG_IMAGE_STD_MASK_VISIBLE_RANGE_FAST_PATH_ENABLE`；继续细拆出来的 `VISIBLE_RANGE_RESIZE` child 只有 `-820B` 且已证伪，当前先不继续往 provider 侧再细拆。
- `std image mask row-range` 这一簇目前已经补齐 `ROW_RANGE / VISIBLE_RANGE / ROW_PARTIAL / ROW_PARTIAL_RESIZE / ROW_PARTIAL_RESIZE_PACKED_ALPHA / ROW_PARTIAL_RESIZE_RGB565 / ROW_PARTIAL_RESIZE_ALPHA8 / ROW_INSIDE / ROW_INSIDE_RESIZE / ROW_INSIDE_RESIZE_ALPHA8` 十颗宏；其中 `ROW_PARTIAL_RESIZE_PACKED_ALPHA` 还能再独立回收 `2436B`，并保持 runtime 严格等价，仍然是 `ROW_PARTIAL_RESIZE` 下更优先的保守 child；`ROW_PARTIAL_RESIZE_RGB565` 虽然还能再回收 `2936B`，但当前 perf 仍会打到 raw sampled masked-image `TEST_PERF_*` 热点，且 runtime 有 `1` 帧像素差；`ROW_PARTIAL_RESIZE_ALPHA8` 虽然还能再回收 `1000B`，但会带来 `4` 帧细微 circle 边缘像素差；`ROW_INSIDE_RESIZE_ALPHA8` 虽然 runtime 严格等价，但也只回收 `712B`。generic sampled resize partial-row 合并 child 已经被验证为 `0B` 并直接拒绝，因此这条线当前没有更值得继续保留的新宏。
- `image-mask decode -> row_block` 这条线已经补齐调用侧 child：`EGUI_CONFIG_IMAGE_CODEC_MASK_IMAGE_ROW_BLOCK_FAST_PATH_ENABLE`；它能在 `QOI/RLE` image-mask decode 调用侧再独立回收 `856B`，且当前 `239` 场景 perf / runtime / unit 全量等价，但收益仍低于 `1KB`。这条线的 image-mask row-block 调用侧已经补齐；round-rect draw 和 circle draw 这两条 `std.c` 线当前也都已经补齐 `alpha8 / rgb565` 两颗互补 child，因此如果后续还要继续拆同层 `ROUND_RECT_DRAW_* / CIRCLE_DRAW_*`，优先级应转向别的 decode/provider 层，而不是继续追 `std.c` 或同一层 image-mask `row_block child`。
- `image-mask decode -> RLE row_block child` 这条线已经补过 provider 细拆：`EGUI_CONFIG_IMAGE_RLE_MASK_IMAGE_ROW_BLOCK_FAST_PATH_ENABLE` 只再独立回收 `108B`，且 `239` 场景 perf / runtime / unit 全量等价，已经明确不值得保留；当前不再沿这颗 `RLE` child 继续细拆。
- `image rle checkpoint` 这条线已经验证为“功能等价但性能关键”的主路径块：现成宏只回收 `text -92B, bss -16B`，却让完整 `239` 场景里的 `20` 个 RLE 场景退化到 `+32.1% ~ +452.0%`；当前不再沿 `RLE checkpoint` 继续细拆。
- `image rle direct copy split` 这条线已经验证为 `0B` dead child：`HelloPerformance` 完整 `239` 场景 perf、runtime `241` 帧和 `HelloUnitTest 688/688` 全都严格等价，代码已回退，当前不再沿这条线继续细拆。
- `image rle external window persistent cache` 这条线当前证明自己是 low-RAM 条件宏，而不是 text-first split：`1 -> 0` 会带来 `HelloPerformance text +816B, bss -1040B`，完整 `239` 场景 perf / runtime / unit 全量通过，但 tiled / external RLE 仍有 `+2.7% ~ +6.2%` 的局部回退；可保留，但不进入当前主推荐序列。
- `std image external row persistent cache` 这条线当前证明自己是 low-RAM 条件宏，而不是 text-first split：`2026-04-05` current-mainline 关闭后仍是 `HelloPerformance text +20B, bss -40B`，perf / runtime / unit 全量通过；可保留，但不进入当前主推荐序列。
- `transform external row persistent cache` 这条线当前同样证明自己是 low-RAM 条件宏，而不是 text-first split：关闭后 `HelloPerformance text -24B, bss -56B`，perf / runtime / unit 全量通过；可保留，而且如果目标是先榨一点静态 RAM，它比 `STD_EXTERNAL_ROW_PERSISTENT_CACHE_ENABLE` 更值得优先尝试。
- `decode opaque alpha row use row cache` 这条线当前证明自己是“收益很小但稳定”的 low-RAM 条件宏：`1` 相比 `0` 只回收 `HelloPerformance text -132B, bss -8B`，但完整 `239` 场景 perf / runtime / unit 仍全量等价；current shipped/default 继续保持 `0`，只保留外部条件实验入口。
- `rle external cache window size` 这条线当前证明自己是“收益很大但回退可见”的 low-RAM 条件值：`1024 -> 64` 能回收 `HelloPerformance bss -1024B`，代价是 external RLE 主路径 `+17.5% ~ +21.8%` 回退；current shipped/default 继续保持 `1024`，只有在项目明确不关心 external RLE 热点时才建议切到 `64`。
- `qoi checkpoint count` 这条线当前证明自己不是可接受的 low-RAM 推荐值：`2 -> 0` 只回收 `HelloPerformance text -276B, bss -8B`，但完整 `239` 场景里有 `20` 个 QOI 场景退化到 `+110.0% ~ +911.4%`；current shipped/default 继续保持 `2`。
- `image qoi checkpoint` 这条线已经验证为“功能等价但性能关键”的主路径块：临时 child 只回收 `text -276B, bss -8B`，却让完整 `239` 场景里的 `20` 个 QOI 场景退化到 `+110% ~ +911%`；当前不再沿 `QOI checkpoint` 继续细拆。
- `image qoi masked alpha8 split` 这条线已经验证为 `0B` dead child：`HelloPerformance` 完整 `239` 场景 perf、runtime `241` 帧和 `HelloUnitTest 688/688` 全都严格等价，代码已回退，当前不再沿这条线继续细拆。
- `image qoi direct alpha8 split` 这条线也已经验证为 `0B` dead child：`HelloPerformance` 完整 `239` 场景 perf、runtime `241` 帧和 `HelloUnitTest 688/688` 同样全都严格等价，代码已回退，当前不再沿这条线继续细拆。
- `image qoi direct copy split` 这条线同样已经验证为 `0B` dead child：`HelloPerformance` 完整 `239` 场景 perf、runtime `241` 帧和 `HelloUnitTest 688/688` 也全部严格等价，代码已回退，当前不再沿这条线继续细拆。
- `std image mask round-rect draw rgb565 child` 这条线已经补齐 complement sibling：`EGUI_CONFIG_IMAGE_STD_MASK_ROUND_RECT_DRAW_RGB565_FAST_PATH_ENABLE` 能在 current-mainline 下再独立回收 `2216B`，并保持完整 `239` 场景 perf 没有任何 `>=10%` 回退，runtime `241` 帧和 `HelloUnitTest 688/688` 也都严格等价。round-rect draw 这条线当前已经有 `alpha8` / `rgb565` 两颗互补 child，后续不再优先沿同一层 `std.c` 继续细拆。
- `std image mask round-rect resize rgb565 child` 这条线也已经补齐 complement sibling：`EGUI_CONFIG_IMAGE_STD_MASK_ROUND_RECT_RESIZE_RGB565_FAST_PATH_ENABLE` 能在 current-mainline 下再独立回收 `2472B`，并保持完整 `239` 场景 perf 没有任何 `>=10%` 回退，runtime `241` 帧和 `HelloUnitTest 688/688` 也都严格等价。round-rect resize 这条线当前同样已经有 `alpha8` / `rgb565` 两颗互补 child，后续不再优先沿同一层 `std.c` 继续细拆。
- `std image mask circle resize rgb565 child` 这条线也已经补齐 complement sibling：`EGUI_CONFIG_IMAGE_STD_MASK_CIRCLE_RESIZE_RGB565_FAST_PATH_ENABLE` 能在 current-mainline 下再独立回收 `1456B`，并继续保住 alpha8 circle resize 与 circle draw / codec / external 对照组，但完整 `239` 场景里仍有 `MASK_IMAGE_TEST_PERF_CIRCLE +36.2%`，runtime on / off 也存在 `frame_0190.png` 的 `1` 帧真实像素差。因此它当前只保留为实验记录，不进入主推荐序列；circle resize 这条线也不再优先沿同一层 `std.c` 继续细拆。
- `std image mask circle draw rgb565 child` 这条线也已经补齐 complement sibling：`EGUI_CONFIG_IMAGE_STD_MASK_CIRCLE_DRAW_RGB565_FAST_PATH_ENABLE` 能在 current-mainline 下再独立回收 `1200B`，并保持完整 `239` 场景 perf 没有任何 `>=10%` 回退，runtime `241` 帧和 `HelloUnitTest 688/688` 也都严格等价。circle draw 这条线当前也已经有 `alpha8` / `rgb565` 两颗互补 child，后续不再优先沿同一层 `std.c` 继续细拆。
- `std image mask round-rect draw alpha8` 往下拆出来的 `ROW_FAST child` 已验证为重复宏：`HelloPerformance text -3960B`，但 size / perf 覆盖范围与 `EGUI_CONFIG_IMAGE_STD_MASK_ROUND_RECT_DRAW_ALPHA8_FAST_PATH_ENABLE` 实质重合，代码已回退，当前不再沿这条线继续细拆。
- `canvas masked-fill visible-range` 调用侧 split 已验证为负收益：`HelloPerformance text +1552B`，当前不再沿这条线继续细拆。
- `canvas masked-fill image` 往下拆出来的 `IMAGE_SCALE child` 已验证只有 `HelloPerformance text -196B`，且 `239` 场景 perf / runtime / unit 全量等价，当前不再沿这条线继续细拆。
- `EGUI_CONFIG_CANVAS_MASK_FILL_ROUND_RECT_FAST_PATH_ENABLE` 暂时不再优先追，因为当前实测收益只有 `876B`。
- `ROW_BLEND` 本体也不建议再优先往下切了，因为已经证明它的主要价值就集中在 gradient masked-fill。
- `std image row overlay` 这一簇当前已经补齐 umbrella + `NON_RGB565` child + `RGB565` child：`NON_RGB565` 可以先回收 `1476B` 而不伤 raw `IMAGE_GRADIENT_OVERLAY`，`RGB565` 再补充回收 `1312B`，但会把 raw `IMAGE_GRADIENT_OVERLAY` 打回 generic path；如果项目只想保住 raw 热点，优先停在 `NON_RGB565`，如果 raw 热点也不是重点，再继续关 `RGB565` child，最后才需要直接动 umbrella。
- 其余方向仍建议优先回到 image 侧其它只影响 specialized hotspot 的剩余分支；如果后续收益开始继续碎片化，再转回 font 侧。
- 处理原则维持不变：
  - 先做只用于实验的编译期开关
  - 再补一个会真实下探的 code-size case
  - 最后用 `HelloPerformance` QEMU 跑完整 A/B 再决定是否回收
