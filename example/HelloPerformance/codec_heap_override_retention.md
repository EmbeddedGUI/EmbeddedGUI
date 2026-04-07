# HelloPerformance Codec Heap Override 保留记录

## 范围

- 本文记录当前仍保留为 shipped override 或 low-RAM 条件实验值的 codec/decode 宏：
  - `EGUI_CONFIG_IMAGE_CODEC_ROW_CACHE_ENABLE`
  - `EGUI_CONFIG_IMAGE_DECODE_MAX_PIXEL_SIZE`
  - `EGUI_CONFIG_IMAGE_CODEC_TAIL_ROW_CACHE_ENABLE`
- 这些宏控制的是瞬时 decode heap 形态和 codec 行为，不是本轮 small static-RAM cleanup 的 `<100B static RAM` 候选。
- 下文仍保留已经内收的 QOI / RLE checkpoint、decode-state 与 external-window policy 宏历史 A/B 记录，用来说明为什么它们不再暴露 public 配置入口。

## 证据来源

- `example/HelloPerformance/ram_tracking.md`
- `docs/low_ram_config_macros.md`
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
- `2026-04-05` 按当前主线重新做完整 A/B：
  - on：`EGUI_CONFIG_IMAGE_CODEC_ROW_CACHE_ENABLE=1`（当前 `HelloPerformance` shipped override）
  - off：`EGUI_CONFIG_IMAGE_CODEC_ROW_CACHE_ENABLE=0`（框架默认）
- size
  - `HelloPerformance` 链接结果：`text 2055380 -> 2050272`、`data 72 -> 60`、`bss 3832 -> 3824`
  - 即 `1 -> 0` 会带来 `text -5108B, data -12B, bss -8B`
- perf
  - 完整 `239` 场景里有 `4` 个 `>=10%` 回退，`<=-10%` 改善为 `0`
  - 基础 `RECTANGLE / CIRCLE / ROUND_RECTANGLE / TEXT / IMAGE_565 / EXTERN_IMAGE_565 / IMAGE_RESIZE_565 / EXTERN_IMAGE_RESIZE_565` 全部保持在噪声内
  - 回退集中在 internal tiled codec 热点：
    - `IMAGE_TILED_QOI_565_0 0.544 -> 3.991 (+633.640%)`
    - `IMAGE_TILED_RLE_565_0 0.469 -> 1.420 (+202.772%)`
    - `IMAGE_TILED_QOI_565_8 1.230 -> 3.392 (+175.772%)`
    - `IMAGE_TILED_RLE_565_8 1.235 -> 2.606 (+111.012%)`
  - 其它 codec 主路径也仍有可见但未过线的局部回退，例如：
    - `IMAGE_QOI_565 +7.555%`
    - `MASK_IMAGE_QOI_NO_MASK +7.555%`
    - `MASK_IMAGE_QOI_ROUND_RECT +7.282%`
    - `MASK_IMAGE_QOI_CIRCLE +7.121%`
- runtime / unit
  - `HelloPerformance` on / off 都是 `241 frames`
  - 全量 `241` 帧 `hash mismatch 0`、`pixel mismatch 0`
  - `HelloUnitTest` on / off 都是 `688/688 passed`
- 处理结论：
  - 当前主线下，`row cache off` 已不再像早期基线那样把整条 codec 主线打穿，但它仍会稳定打穿 internal tiled `QOI/RLE` 热点
  - 因此 `HelloPerformance` 继续保留 `EGUI_CONFIG_IMAGE_CODEC_ROW_CACHE_ENABLE=1`
  - 同时保留 `#ifndef` 外部覆盖入口，用于后续 codec policy A/B

### `EGUI_CONFIG_IMAGE_DECODE_MAX_PIXEL_SIZE`

- 先纠正当前 shipped 关系：
  - 框架默认值已经是 `2`
  - `HelloPerformance` 当前 shipped 配置也直接继承这个默认值
  - `example/HelloPerformance/app_egui_config.h` 里的 `2` 只保留在尾部 `#if 0` low-RAM 块中，用于保留历史低 RAM profile 的外部 A/B 入口
- 当前量级约为：
  - `2` 对应 `16 × 240 × 2 = 7680B` 的 decode heap 上界
  - 历史默认 `4` 在相同几何下对应 `16 × 240 × 4 = 15360B`
  - 差值约 `7680B`
- HelloPerformance 的压缩图资源当前是 RGB565-only，因此这个宏本质上是在约束 decode heap 上界，不是 fixed static RAM 开关。
- `2026-04-05` 按当前主线重新做完整 A/B：
  - base：`EGUI_CONFIG_IMAGE_DECODE_MAX_PIXEL_SIZE=2`（当前默认）
  - compare：`EGUI_CONFIG_IMAGE_DECODE_MAX_PIXEL_SIZE=4`（历史默认）
- size
  - `HelloPerformance` 链接结果：`text 2055380 -> 2055488`、`bss 3832 -> 3832`
  - 即 `2 -> 4` 为 `text +108B, bss +0`
- perf
  - 完整 `239` 场景没有任何 `>=10%` 回退或 `<=-10%` 改善
  - 最大绝对波动仅 `IMAGE_TILED_QOI_565_8 -0.325%`
  - 次大波动也只有 `IMAGE_TILED_RLE_565_0 -0.213%`、`IMAGE_TILED_RLE_565_8 -0.162%`
  - 基础 `RECTANGLE / CIRCLE / ROUND_RECTANGLE / TEXT / IMAGE_565 / EXTERN_IMAGE_565 / IMAGE_RESIZE_565 / EXTERN_IMAGE_RESIZE_565` 全部 `+0.0%`
- runtime / unit
  - `HelloPerformance` base / compare 都是 `241 frames`
  - 全量 `241` 帧 `hash mismatch 0`、`pixel mismatch 0`
  - `HelloUnitTest` base / compare 都是 `688/688 passed`
- 这说明它属于 decode heap 上界策略，而不是当前可继续下沉或清理掉的 app 侧小宏。
- 处理结论：
  - 不再把它写成 `HelloPerformance` 当前 active override
  - shipped/default 记为 `2`
  - `4` 继续保留为历史默认/外部 A/B 对比值；若未来重新引入 RGBA32 decode 需求，再做更广覆盖验证后评估是否恢复

### `EGUI_CONFIG_IMAGE_DECODE_OPAQUE_ALPHA_ROW_USE_ROW_CACHE`（已内收）

- `docs/low_ram_config_macros.md` 已说明此宏依赖 `IMAGE_CODEC_ROW_CACHE_ENABLE=1`，作用是复用现有 row-cache 的 alpha 缓冲头部，而不是再单独保留一份不透明 alpha 参考行。
- 先纠正当前 shipped 关系：
  - 框架默认值是 `0`
  - `HelloPerformance` 当前 shipped 配置也直接继承这个默认值
  - `example/HelloPerformance/app_egui_config.h` 里的 `1` 只保留在尾部 `#if 0` low-RAM 块中，属于历史实验值
- 当前量级约为：
  - `~240B` alpha 行数据
  - `+8B` 指针/容量元数据
  - 合计约 `248B`
- `2026-04-05` 按当前主线重新做完整 A/B，并补齐了 recheck 侧 perf compare / runtime compare / unit：
  - on：`EGUI_CONFIG_IMAGE_DECODE_OPAQUE_ALPHA_ROW_USE_ROW_CACHE=1`（历史实验值）
  - off：`EGUI_CONFIG_IMAGE_DECODE_OPAQUE_ALPHA_ROW_USE_ROW_CACHE=0`（当前默认）
- size
  - `HelloPerformance` 链接结果：`text 2055248 -> 2055380`、`bss 3824 -> 3832`
  - 即 `1` 相比 `0` 为 `text -132B, bss -8B`
- perf
  - 完整 `239` 场景没有任何 `>=10%` 回退或改善，compare 汇总为 `ge10=0`、`le10=0`
  - 最大绝对波动仅 `0.431%`
  - 基础 `RECTANGLE / CIRCLE / ROUND_RECTANGLE / TEXT / IMAGE_565 / EXTERN_IMAGE_565 / IMAGE_RESIZE_565 / EXTERN_IMAGE_RESIZE_565` 全部 `+0.0%`
- runtime / unit
  - `HelloPerformance` on / off 都是 `241 frames`
  - 全量 `241` 帧 `hash mismatch 0`、`pixel mismatch 0`
  - `HelloUnitTest` on / off 都是 `688/688 passed`
- 这属于 decode heap 组织方式，且与 row-cache A/B 绑定，不是可按 `<100B static RAM` 规则清掉的 app 小宏。
- 处理结论：
  - 不再把它写成 `HelloPerformance` 当前 active override
  - `2026-04-06` 起，框架 public 入口也已内收，当前实现私有默认值固定为 `0`
  - `1` 只保留为历史 low-RAM A/B 记录，不再作为当前 app-side override 或外部入口

### `EGUI_CONFIG_IMAGE_QOI_CHECKPOINT_COUNT`（已内收）

> `2026-04-06` 起，这个名字不再是当前框架 public 配置入口；当前主线已把它内收到 `src/image/egui_image_qoi.c` 的实现私有常量，默认行为固定为 `2`。下面保留的是历史 A/B 记录，用来说明为什么它不再值得继续保留成公共宏。

- 先纠正当前 shipped 关系：
  - 框架默认值是 `2`
  - `HelloPerformance` 当前 shipped 配置也直接继承这个默认值
  - `example/HelloPerformance/app_egui_config.h` 里的 `0` 只保留在尾部 `#if 0` low-RAM 块中，属于历史实验值
- `2026-04-05` 按当前主线重新做完整 recheck A/B，并补齐新的 perf compare / runtime compare / unit：
  - on：`EGUI_CONFIG_IMAGE_QOI_CHECKPOINT_COUNT=2`（当前默认）
  - off：`EGUI_CONFIG_IMAGE_QOI_CHECKPOINT_COUNT=0`（历史实验值）
- size
  - `HelloPerformance` 链接结果：`text 2055380 -> 2055104`、`bss 3832 -> 3824`
  - 即 `2 -> 0` 为 `text -276B, bss -8B`
- perf
  - 完整 `239` 场景里有 `20` 个 `>=10%` 回退，`<=-10%` 改善为 `0`
  - 基础 `RECTANGLE / CIRCLE / ROUND_RECTANGLE / TEXT / IMAGE_565 / EXTERN_IMAGE_565 / IMAGE_RESIZE_565 / EXTERN_IMAGE_RESIZE_565` 全部 `+0.0%`
  - 回退全部集中在 QOI draw 主路径及其 masked 变体：
    - `EXTERN_IMAGE_QOI_565 / EXTERN_MASK_IMAGE_QOI_NO_MASK +911.4%`
    - `EXTERN_MASK_IMAGE_QOI_ROUND_RECT +898.6%`
    - `EXTERN_MASK_IMAGE_QOI_CIRCLE +890.8%`
    - `IMAGE_QOI_565 / MASK_IMAGE_QOI_NO_MASK +819.2%`
    - `MASK_IMAGE_QOI_ROUND_RECT +789.6%`
    - `MASK_IMAGE_QOI_CIRCLE +772.1%`
    - `EXTERN_IMAGE_QOI_565_8 / EXTERN_MASK_IMAGE_QOI_8_NO_MASK +484.2%`
    - `IMAGE_QOI_565_8 / MASK_IMAGE_QOI_8_NO_MASK +374.7% ~ +375.0%`
    - `EXTERN_MASK_IMAGE_QOI_IMAGE +315.3%`
    - `MASK_IMAGE_QOI_IMAGE +250.6%`
    - `EXTERN_MASK_IMAGE_QOI_8_IMAGE +149.9%`
    - `MASK_IMAGE_QOI_8_IMAGE +110.0%`
- runtime / unit
  - `HelloPerformance` on / off 都是 `241 frames`
  - 全量 `241` 帧 `hash mismatch 0`、`pixel mismatch 0`
  - sample hash 仍一致：
    - `frame_0000.png A4337E1408AB6C17F77FC5F1D5BB6815E8EEAB83B934854E1EEBDAF37340D7A5`
    - `frame_0120.png 7CBEC8DCC7375B8F37C13019A017FC460CFC276A9764993516D0AC5EEF94F569`
    - `frame_0240.png 619EA7069DAC066ABA1A27113EAC475C9B13391BCCD078D03CBFBB113F576EA2`
  - `HelloUnitTest` on / off 都是 `688/688 passed`
- 这说明它是 QOI 恢复路径的 heap/perf 调参点，不是可安全压掉的低成本 decode 宏。
- 处理结论：
  - 不再把它写成 `HelloPerformance` 当前 active override
  - `2026-04-06` 起，框架 public 入口也已内收，当前实现私有默认值固定为 `2`
  - 历史实验值 `0` 当前只额外换来 `text -276B, bss -8B`，但会打穿整条 QOI draw 主路径；由于收益低于 1KB 门槛，它现在只保留为历史 A/B 记录，不再作为当前外部入口

### `EGUI_CONFIG_IMAGE_CODEC_TAIL_ROW_CACHE_ENABLE`

- `2026-03-28` 的历史基线已经验证过它确实能改写 codec heap 形态：
  - whole-run heap peak `11616B -> 10032B`，减少 `1584B`
  - `static RAM` 仅 `+8B`
  - 当时最坏已验证 perf 为 `IMAGE_RLE_565_8 +8.35%`
- 但到 `2026-04-05` 再按当前主线做完整 recheck A/B 时，需要先纠正一个前提：
  - 这颗宏现在并不是 `HelloPerformance` shipped 默认
  - `example/HelloPerformance/app_egui_config.h` 里的旧 low-RAM codec override 块仍在 `#if 0` 下面
  - 因此当前真实对比应是：
    - on：`EGUI_CONFIG_IMAGE_CODEC_TAIL_ROW_CACHE_ENABLE=1`（历史实验值）
    - off：`EGUI_CONFIG_IMAGE_CODEC_TAIL_ROW_CACHE_ENABLE=0`（当前默认）
- size
  - `HelloPerformance` 链接结果：`text 2055380 -> 2065356`，即开启后 `text +9976B`
  - 最终 ELF `data / bss` 不变
- perf
  - 当前默认 `0` 相比历史实验值 `1`，完整 `239` 场景里有 `40` 个 `>=10%` 回退，`<=-10%` 改善为 `0`
  - 基础 `RECTANGLE / CIRCLE / ROUND_RECTANGLE / TEXT / IMAGE_565 / EXTERN_IMAGE_565 / IMAGE_RESIZE_565 / EXTERN_IMAGE_RESIZE_565` 都是 `+0.0%`
  - 回退全部集中在 `QOI/RLE` 压缩图主路径及其 masked 变体：
    - `IMAGE_QOI_565 +168.2%`
    - `EXTERN_IMAGE_QOI_565 +189.9%`
    - `IMAGE_RLE_565 +121.3%`
    - `EXTERN_IMAGE_RLE_565 +141.3%`
    - `MASK_IMAGE_QOI_NO_MASK +168.2%`
    - `EXTERN_MASK_IMAGE_QOI_NO_MASK +189.9%`
    - `MASK_IMAGE_RLE_NO_MASK +121.3%`
    - `EXTERN_MASK_IMAGE_RLE_NO_MASK +141.3%`
- runtime / unit
  - `HelloPerformance` on / off 都是 `241 frames`
  - 全量 `241` 帧 `hash mismatch 0`、`pixel mismatch 0`
  - sample hash 仍一致：
    - `frame_0000.png A4337E1408AB6C17F77FC5F1D5BB6815E8EEAB83B934854E1EEBDAF37340D7A5`
    - `frame_0120.png 7CBEC8DCC7375B8F37C13019A017FC460CFC276A9764993516D0AC5EEF94F569`
    - `frame_0240.png 619EA7069DAC066ABA1A27113EAC475C9B13391BCCD078D03CBFBB113F576EA2`
  - `HelloUnitTest` on / off 都是 `688/688 passed`
- 处理结论：
  - 这颗宏不能再写成“当前默认就是 1”
  - 但它也不是已经失效的旧 low-RAM 思路：在当前实现里，`1` 仍是会显著改善 `QOI/RLE` 热点吞吐的高杠杆 codec policy 宏
  - 因此保持 `#ifndef` 外部覆盖入口，并把它从“当前默认”改写成“历史实验值 1（当前默认 0）”；如果未来要重新审 codec policy，优先级应高于已经被证明不划算的 `QOI_COMPACT_RGB565_INDEX`

### `EGUI_CONFIG_IMAGE_RLE_EXTERNAL_WINDOW_PERSISTENT_CACHE_ENABLE`（已内收）

> `2026-04-06` 起，这个名字不再是当前框架 public 配置入口；当前主线已把它内收到 `src/image/egui_image_rle.c` 的实现私有常量，默认行为固定为 `1`。下面保留的是历史 A/B 记录，用来说明为什么它不再值得继续保留成公共宏。

- 先纠正当前 shipped 关系：
  - 框架默认值是 `1`
  - `HelloPerformance` 当前 shipped 配置也直接继承这个默认值
  - `example/HelloPerformance/app_egui_config.h` 里的 `0` 只保留在尾部 `#if 0` low-RAM codec/decode 块中，属于历史实验值
- `2026-04-05` 按当前主线重新做完整 A/B：
  - on：`EGUI_CONFIG_IMAGE_RLE_EXTERNAL_WINDOW_PERSISTENT_CACHE_ENABLE=1`（当前默认）
  - off：`EGUI_CONFIG_IMAGE_RLE_EXTERNAL_WINDOW_PERSISTENT_CACHE_ENABLE=0`（历史实验值）
- size
  - `HelloPerformance` 链接结果：`text 2055380 -> 2056196`、`bss 3832 -> 2792`
  - 即 `1 -> 0` 为 `text +816B, bss -1040B`
- perf
  - 完整 `239` 场景里没有任何 `>=10%` 回退或改善
  - 最大绝对波动为 `IMAGE_TILED_RLE_565_0 +6.183%`
  - 基础 `RECTANGLE / CIRCLE / ROUND_RECTANGLE / TEXT / IMAGE_565 / EXTERN_IMAGE_565 / IMAGE_RESIZE_565 / EXTERN_IMAGE_RESIZE_565` 都在噪声内
  - internal `IMAGE_RLE_565 / IMAGE_RLE_565_8 / MASK_IMAGE_RLE_*` 基本都在 `+0.4%` 内
  - `IMAGE_TILED_RLE_565_0 +6.183%`、`IMAGE_TILED_RLE_565_8 +2.834%`
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
- runtime / unit
  - `HelloPerformance` on / off 都是 `241 frames`
  - 全量 `241` 帧 `hash mismatch 0`、`pixel mismatch 0`
  - `HelloUnitTest` on / off 都是 `688/688 passed`
- 处理结论：
  - `2026-04-06` 起，框架 public 入口也已内收，当前实现私有默认值固定为 `1`
  - 历史实验值 `0` 虽然能把 external RLE window 从 persistent BSS 切到 transient frame heap，并稳定回收 `bss -1040B`
  - 但它当前会带来 `text +816B`，也要接受 tiled / external RLE 的 `+2.7% ~ +6.2%` 局部回退；因此现在只保留为历史 A/B 记录，不再作为当前外部入口

### `EGUI_CONFIG_IMAGE_QOI_COMPACT_RGB565_INDEX_ENABLE`（已内收）

> `2026-04-06` 起，这个名字不再是当前框架 public 配置入口；当前主线已把它内收为 `src/image/egui_image_qoi.c` 私有默认 `0`。下面保留的是历史 A/B 记录，用来说明为什么它不再值得继续保留为公共宏。

- 先纠正当前 shipped 关系：
  - 框架默认值是 `0`
  - `HelloPerformance` 当前 shipped 配置也直接继承这个默认值
  - 历史实验值 `1` 现在只保留在外部 A/B 入口里，不再是当前 active override
- `2026-04-05` 按当前主线重新补齐完整 recheck A/B：
  - on：`EGUI_CONFIG_IMAGE_QOI_COMPACT_RGB565_INDEX_ENABLE=1`（历史实验值）
  - off：`EGUI_CONFIG_IMAGE_QOI_COMPACT_RGB565_INDEX_ENABLE=0`（当前默认）
- size / heap 量级
  - `HelloPerformance` 链接结果：`text 2055380 -> 2056160`，即开启后 `text +780B`
  - 最终 ELF `bss` 不变，收益不在静态 RAM
  - 按当前 `egui_image_qoi_decode_state_t` 字段布局，QOI decode state 从 `404B` 收缩到 `212B`，每个活动实例只节省 `192B heap`
- perf
  - 完整 `239` 场景里有 `10` 个 `>=10%` 回退，`<=-10%` 改善为 `0`
  - 基础 `RECTANGLE / CIRCLE / ROUND_RECTANGLE / TEXT / IMAGE_565 / EXTERN_IMAGE_565 / IMAGE_RESIZE_565 / EXTERN_IMAGE_RESIZE_565` 全部 `+0.0%`
  - 回退全部集中在 `QOI RGB565` 主路径：
    - `IMAGE_QOI_565 +52.2%`
    - `MASK_IMAGE_QOI_NO_MASK +52.2%`
    - `MASK_IMAGE_QOI_ROUND_RECT +50.3%`
    - `MASK_IMAGE_QOI_CIRCLE +49.2%`
    - `MASK_IMAGE_QOI_IMAGE +27.5%`
    - `EXTERN_IMAGE_QOI_565 +19.3%`
    - `EXTERN_MASK_IMAGE_QOI_NO_MASK +19.3%`
    - `EXTERN_MASK_IMAGE_QOI_ROUND_RECT +19.0%`
    - `EXTERN_MASK_IMAGE_QOI_CIRCLE +18.9%`
    - `EXTERN_MASK_IMAGE_QOI_IMAGE +10.4%`
- runtime / unit
  - `HelloPerformance` on / off 都是 `241 frames`
  - 全量 `241` 帧 `hash mismatch 0`、`pixel mismatch 0`
  - sample hash 仍一致：
    - `frame_0000.png A4337E1408AB6C17F77FC5F1D5BB6815E8EEAB83B934854E1EEBDAF37340D7A5`
    - `frame_0120.png 7CBEC8DCC7375B8F37C13019A017FC460CFC276A9764993516D0AC5EEF94F569`
    - `frame_0240.png 619EA7069DAC066ABA1A27113EAC475C9B13391BCCD078D03CBFBB113F576EA2`
  - `HelloUnitTest` on / off 都是 `688/688 passed`
- 处理结论：
  - 当前不应再把它写成 `HelloPerformance=1`
  - `2026-04-06` 起，框架 public 入口也已内收，当前实现私有默认值固定为 `0`
  - `1` 虽然还能换到约 `192B`/活动实例的 `QOI decode heap` 收益，但它已经不是“低代价堆压缩”，而是会稳定打穿 `QOI RGB565` 主路径性能的 codec 状态压缩开关，因此只保留为历史 A/B 记录，不再作为当前外部入口

### `EGUI_CONFIG_IMAGE_QOI_INDEX_RGB565_CACHE_ENABLE`（已内收）

- `2026-04-05` 用当前 `HelloPerformance` 基线重新做了完整 A/B，并补齐了 off 侧 `HelloUnitTest` 与 perf compare 汇总：
  - on：`EGUI_CONFIG_IMAGE_QOI_INDEX_RGB565_CACHE_ENABLE=1`（当前默认）
  - off：`EGUI_CONFIG_IMAGE_QOI_INDEX_RGB565_CACHE_ENABLE=0`
- size / heap 量级
  - `HelloPerformance` 链接结果：`text 2055380 -> 2055316`，即关闭后只回收 `text -64B`
  - 最终 ELF `bss` 不变
  - 关闭时每个活动 QOI decode 实例可再少掉约 `128B heap`
- perf
  - 完整 `239` 场景里没有任何 `>=10%` 回退或改善，compare 汇总为 `ge10=0`、`le10=0`
  - 基础 `RECTANGLE / CIRCLE / ROUND_RECTANGLE / TEXT / IMAGE_565 / EXTERN_IMAGE_565 / IMAGE_RESIZE_565 / EXTERN_IMAGE_RESIZE_565` 都是 `+0.0%`
  - 最大绝对波动仍在当前噪声内，集中在 QOI 路径：
    - `IMAGE_QOI_565 +2.8%`
    - `MASK_IMAGE_QOI_NO_MASK +2.8%`
    - `MASK_IMAGE_QOI_ROUND_RECT +2.7%`
    - `MASK_IMAGE_QOI_CIRCLE +2.6%`
    - `EXTERN_MASK_IMAGE_QOI_IMAGE -1.7%`
    - `EXTERN_IMAGE_QOI_565 -1.2%`
    - `EXTERN_MASK_IMAGE_QOI_NO_MASK -1.2%`
    - `EXTERN_MASK_IMAGE_QOI_ROUND_RECT -1.2%`
    - `EXTERN_MASK_IMAGE_QOI_CIRCLE -1.2%`
- runtime / unit
  - `HelloPerformance` on / off 都是 `241 frames`
  - 全量 `241` 帧 `hash mismatch 0`、`pixel mismatch 0`
  - `HelloUnitTest` on / off 都是 `688/688 passed`
- 处理结论：
  - 这颗宏当前不应再被写成 `HelloPerformance=0`
  - `2026-04-06` 起，框架 public 入口也已内收，当前实现私有默认值固定为 `1`
  - 关闭只换来 `text -64B` 和约 `128B`/活动实例的 QOI decode heap 回收，因此只保留历史 A/B 记录，不再作为当前外部入口

## 最终结论

- 这 8 个宏都属于 HelloPerformance 的 codec/decode heap 调参入口。
- 它们的主要影响面是 heap 峰值、codec 行缓存形态和压缩图热点性能，而不是 `<100B` 的固定 static RAM。
- 因此本轮不把这些项统一改成“默认打开后移除宏管理”；但对收益低于 1KB 门槛、且当前仓内没有真实 shipped 依赖的微型入口，继续按同一规则内收到实现私有常量。
- 额外补充：`2026-04-05` 当前主线重跑后，`EGUI_CONFIG_IMAGE_CODEC_TAIL_ROW_CACHE_ENABLE` 当前 shipped/default 仍是 `0`。虽然开启 `1` 会带来 `text +9976B`，但关闭到默认值 `0` 会让完整 `239` 场景中的 `40` 个 `QOI/RLE` 压缩图热点出现 `+16.2% ~ +189.9%` 回退，因此它仍应保留为高优先级 codec policy 候选，而不是被降格成“已失效的旧 low-RAM 宏”。
- 额外补充：`2026-04-06` 起，`EGUI_CONFIG_IMAGE_RLE_EXTERNAL_WINDOW_PERSISTENT_CACHE_ENABLE` 也已从 public 配置面内收。历史 `0` 虽然能稳定回收 `bss -1040B`，但会带来 `text +816B`，且 tiled / external RLE 路径存在 `+2.7% ~ +6.2%` 的局部回退，因此当前只保留历史 A/B 记录。
- 额外补充：`2026-04-06` 起，`EGUI_CONFIG_IMAGE_QOI_CHECKPOINT_COUNT` 也已从 public 配置面内收。历史 `0` 虽然还能回收 `text -276B, bss -8B`，但完整 `239` 场景会出现 `20` 个 `>=10%` 的 QOI 主路径回退，峰值 `+911.4%`，因此当前只保留历史 A/B 记录。
- 额外补充：`2026-04-06` 起，`EGUI_CONFIG_IMAGE_CODEC_TAIL_ROW_CACHE_MAX_COLS` 也已从 public 配置面内收。它没有任何 code-size 收益可言，历史更窄 cap `184/176/144/96` 反而会打破当前 `240px` 图宽、`PFB_WIDTH=48` 横向 walk 下的 tail 覆盖条件，让 QOI/RLE alpha 热点出现 `+45% ~ +66%` 乃至数倍回退，因此当前只保留历史 A/B 记录。
- 额外补充：`2026-04-06` 起，`EGUI_CONFIG_IMAGE_QOI_COMPACT_RGB565_INDEX_ENABLE` 也已从 public 配置面内收。历史 `1` 虽然还能压 `192B`/活动实例的 QOI decode heap，但会额外带来 `text +780B`，并在完整 `239` 场景中出现 `10` 个 `>=10%` 的 `QOI RGB565` 主路径回退，因此当前只保留历史 A/B 记录。
- 额外补充：`EGUI_CONFIG_IMAGE_QOI_INDEX_RGB565_CACHE_ENABLE` 当前 shipped 默认仍是 `1`。关闭后只多拿到 `text -64B` 和约 `128B`/活动实例 heap，当前完整 `239` 场景则全部维持在噪声内；由于收益低于 1KB 门槛，这颗宏也已从 public 配置面内收。
