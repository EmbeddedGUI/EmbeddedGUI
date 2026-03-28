# Core RAM 使用分布与裁剪指南

## 文档范围

- 本文面向 `EmbeddedGUI core/framework` 层，说明当前 RAM 的主要组成、热点分布和裁剪规则。
- 当前示例数据来自 `HelloPerformance + PORT=qemu + CPU_ARCH=cortex-m3`，用于帮助定位 RAM 大头，不代表所有应用都完全一致。
- `PFB` 会单独列出，但它属于用户配置项，不计入框架 RAM 优化成果。
- 业务层自定义 view、应用资源、端口私有缓冲区，以及链接脚本保留的 stack/heap 区域，同样会占用同一片 SRAM；本文重点解释 core 侧的占用方式和裁剪顺序。

## RAM 三类

建议把 EmbeddedGUI 的 RAM 按下面三类来审视：

| 类别 | 典型位置 | 典型内容 | 规则 |
| --- | --- | --- | --- |
| 静态 RAM | `.bss` / `.data` | 核心状态机、定时器、少量固定元数据、`PFB` | 只保留与尺寸无关、上界稳定、收益明确的小对象 |
| 跟随尺寸走的 buffer | 运行时 scratch / cache | 跟随字体大小、图片大小、屏幕尺寸、`PFB` 尺寸变化的 buffer | 必须走 `heap`，不能为了回避 `heap` 改成宏固定 RAM、静态数组或大栈数组 |
| Heap | 单次绘制 / 单帧 / 跨帧缓存 | 图像解码 row cache、文字变换 scratch、外部字体 scratch、图片 resize scratch | 优先短生命周期；如需常驻，必须记录 owner、lifetime、bytes 和收益 |

额外需要单独评估的一类是 `stack`：

- `stack` 不属于上面三类，但它和 `.bss/.data/heap` 竞争同一片 SRAM。
- 大局部数组同样可能把系统栈顶爆。
- 任何新引入的大栈对象，都必须说明必要性，并用 `-fstack-usage` 复核。

额外的 HelloPerformance 硬约束：

- 不接受整个图像尺寸的 resident heap，即使它能换来明显性能收益。
- 尺寸相关 heap 的上限只接受两类：
  - 不超过 `1 * PFB` 字节；
  - 或最多 `2` 行 / `2` 列图像、屏幕尺寸相关 scratch/cache。
- 如果后续确实需要更高性能版本，也必须在这个上限内做“双版本”选择，不能靠 whole-image cache。

## 当前 HelloPerformance / QEMU 快照

当前 `HelloPerformance/QEMU` 的 RAM 观测值如下：

| 项目 | 当前值 | 说明 |
| --- | ---: | --- |
| `text` | `2161016B` | 代码和只读数据 |
| `data` | `52B` | 已初始化静态 RAM |
| `bss` | `2924B` | `llvm-size` 汇总值，包含链接脚本保留区 |
| `.bss` | `2152B` | 实际未初始化静态符号区 |
| `._user_heap_stack` | `772B` | 当前链接脚本保留区，不是 core 固定对象本身 |
| 固定静态 RAM 小计 | `2204B` | `.data + .bss`，不含链接脚本保留区 |
| Heap 峰值 | `9456B` | `QEMU_HEAP_MEASURE=1` 实测峰值 |
| Heap 空闲 current | `0B` | 当前默认低 RAM 方案下，尺寸相关 scratch 最终都会释放 |
| alloc/free | `8069 / 8069` | 完整录制流程结束后配平 |
| 编译期最大栈帧 | `1200B` | `egui_view_heart_rate_on_draw()`，不在当前录制热路径 |
| 当前活跃路径最大栈帧 | `752B` | `egui_canvas_draw_circle_fill_gradient()` |

说明：

- `llvm-size` 的 `bss=2924B` 里包含了 `._user_heap_stack=772B`，看总表时不要把它全部当成 core 固定静态 RAM。
- 从 core 固定对象角度看，当前真正长期常驻的 `.data + .bss` 只有 `2204B`。
- 其中 `egui_pfb=1536B` 是当前应用配置下的 `PFB`，这是用户自己选择的空间换时间项，不应被当成框架裁剪成果。
- `heap peak=9456B` 是运行时峰值，不是 idle 常驻占用；当前默认示例结束后 `current heap` 会回到 `0B`。
- 如需继续追峰值归因，可在测量构建里额外打开 `QEMU_HEAP_TRACE_ACTIONS=1`，让 QEMU 按录制 action 输出 `HEAP_ACTION:<idx>:current/peak/allocs/frees`，默认关闭时不会改变当前 RAM 口径。
- `HelloPerformance` 现在把 QEMU 链接脚本保留栈压到 `768B`，所以静态 RAM headline 已反映这一调整。
- 当前正常 QEMU 构建口径是 `text=2161016`、`data=52`、`bss=2924`、`static RAM=2976`；latest external transform row-cache chunk 对齐只增加 `80B text`，不增加静态 RAM / heap / stack。
- 当前默认 external raw-image shared row cache 也已收紧到 `2` 行上限：`EGUI_CONFIG_IMAGE_EXTERNAL_DATA_CACHE_MAX_BYTES=960`、`EGUI_CONFIG_IMAGE_EXTERNAL_ALPHA_CACHE_MAX_BYTES=480`。这不会改变 whole-run `9456B` headline，但会把 `240px` 外部 RGB565+alpha 场景的 scene-local heap 从旧的 `2880B` 压到 `1440B`，resize 场景则是 `1536B`。

## 静态 RAM 分布

下面是当前示例里最主要的固定静态符号：

| 符号 | 字节 | 分类 | 说明 |
| --- | ---: | --- | --- |
| `egui_pfb` | `1536B` | 用户配置项 | 当前 `PFB` 像素缓冲区 |
| `qoi_state` | `208B` | core 元数据 | QOI 解码器状态 |
| `egui_core` | `120B` | core 元数据 | 核心运行时状态 |
| `g_text_transform_prepare_cache` | `60B` | core 元数据 | 旋转文字小型 affine prepare cache，不是尺寸相关 scratch |
| `test_view` | `56B` | app 示例对象 | HelloPerformance 场景对象 |
| `canvas_data` | `32B` | core 元数据 | canvas 运行时状态 |
| `ui_timer` | `20B` | app 示例对象 | app 定时器 |
| `port_display_driver` | `16B` | port 元数据 | QEMU 显示驱动状态 |
| `g_egui_mask_circle_frame_row_cache` | `16B` | heap handle 元数据 | 只保留 owner 和句柄，真正 row cache 在 heap |
| `rle_state` | `16B` | core 元数据 | RLE 解码状态 |
| `g_font_std_code_lookup_cache` | `12B` | core 元数据 | 字体 code lookup 缓存元数据 |
| `egui_image_decode_cache_state` | `12B` | core 元数据 | 解码 cache 元数据，记录 tail-row cache 窗口信息 |
| `g_text_transform_visible_tile_cache` | `8B` | heap handle 元数据 | 只保存 heap 缓冲句柄/容量 |
| `s_qemu_resource_handle` | `4B` | port 元数据 | QEMU 资源句柄 |

判断原则：

- 可以留在静态区的，应该是与字体大小、图片大小、屏幕尺寸、`PFB` 尺寸无关的小型状态对象。
- 如果静态对象只是某个 heap buffer 的 `owner / pointer / capacity` 元数据，通常可以接受，因为真正的大块数据已经不再常驻 `.bss`。
- `PFB` 必须单独看。它通常是静态 RAM 里最大的一项，但这是应用配置，不是 core 算法残留。
- 最新 external text/image transform 优化仍遵守这条线：只新增了流式读取和 chunk 对齐控制代码，没有把任何 glyph 或 image 尺寸相关 scratch 落回静态区。

## 跟随尺寸走的 buffer

以下这类 buffer 不允许为了“heap 看起来更小”而回退成静态数组或大栈数组：

- 跟随字体大小变化
- 跟随图片大小变化
- 跟随屏幕尺寸变化
- 跟随 `PFB` 尺寸变化

当前 core 中典型的尺寸相关 buffer 如下：

| 模块 | owner | 尺寸驱动因素 | 当前放置 | 生命周期 |
| --- | --- | --- | --- | --- |
| 压缩图单行解码 scratch | `egui_image_decode_*` | 图片宽度、像素格式、alpha 行宽 | `heap` | 单次解码 / 当前帧 |
| 压缩图 row-band / tail-row cache | `egui_image_decode_*` | `row_count * image_width * bytes_per_pixel`，受 `PFB_HEIGHT` 和图片宽度影响 | `heap` | 当前 refresh walk |
| 旋转文字 visible alpha8 tile | `egui_canvas_draw_text_transform()` | 可见 glyph 变换后包围盒 | `heap` | 当前 refresh walk |
| rotated text layout scratch | `egui_canvas_draw_text_transform()` | glyph 数、line 数、布局结果 | `heap` | 单次绘制 |
| rotated text tile scratch | `egui_canvas_draw_text_transform()` | 可见 glyph 数、line 数 | `heap` | 单次绘制 |
| 图片 resize `src_x_map` | `egui_image_std_*` | 当前绘制宽度 | `heap` | 单次绘制 |
| round-rect 图片快速路径 row cache | `egui_image_std_*` | `min(PFB_HEIGHT, image_height)` | `heap` | 当前 refresh walk |
| circle mask row cache | `egui_mask_circle_*` | `3 * PFB_HEIGHT * sizeof(egui_dim_t)` | `heap` | 当前帧 |
| 外部字体 glyph / row scratch | `egui_font_std_*`、`egui_canvas_transform_*` | 字体大小、glyph bitmap 尺寸 | `heap` | 单次绘制 / 当前 refresh walk，visible-tile 外部旋转文字按 `14` 行 chunk 流式读取 |
关于宏的使用边界：

- 宏可以表达“是否启用某个能力”或“某个路径的预算上限”。
- 但不能用宏把尺寸相关 buffer 直接落成固定 `.bss`、静态局部数组或大栈数组。
- 宏可以决定“需不需要这块 buffer”或“允许用多少 heap”；真正的大块存储仍然必须走 `heap`。

## 当前 Heap 分布

当前默认示例的 heap 以短生命周期 scratch 为主，峰值 `9456B`，空闲 `current` 回到 `0B`。

### 默认低 RAM 路径的大头

| 类型 | 典型 owner | 特征 | 备注 |
| --- | --- | --- | --- |
| 压缩图 tail-row cache | `egui_image_qoi_draw_image()` / `egui_image_rle_draw_image()` | 当前默认 heap 峰值主因 | 首个 tile 只为可见段申请瞬时 scratch，后续只保留横向 tail 列 |
| rotated text scratch | `egui_canvas_draw_text_transform()` | 随文本布局结果变化 | 已改为按 glyph/line 数动态申请 |
| 图片 resize / round-rect scratch | `egui_image_std_*` | 随绘制宽度、图片高度、`PFB_HEIGHT` 变化 | 用完立即释放 |
| circle mask frame scratch | `egui_mask_circle_*` | 随 `PFB_HEIGHT` 变化 | 帧结束释放 |
| 外部字体 scratch | `egui_font_std_*` / `egui_canvas_transform_*` | 随 glyph bitmap 尺寸变化 | 用完或 frame 结束释放；visible-tile 外部旋转文字改为 `14` 行 chunk heap scratch |

### 默认路径的单场景 heap 热点

| 场景 | interaction total peak | 说明 |
| --- | ---: | --- |
| `IMAGE_QOI_565_8` | `9360B` | 低 RAM codec tail-row cache + 首个 tile 的可见段瞬时 scratch |
| `EXTERN_IMAGE_QOI_565_8` | `9360B` | 同上，外部 QOI alpha 场景 |
| `IMAGE_RLE_565_8` | `9360B` | 同上，内部 RLE alpha 场景 |
| `EXTERN_IMAGE_RLE_565_8` | `9360B` | 同上，外部 RLE alpha 场景 |
| `EXTERN_TEXT_ROTATE_BUFFERED` | `4194B` | external rotated-text visible alpha8 tile cache + `14` 行 external glyph chunk scratch + transient layout/tile scratch |
| `TEXT_ROTATE_BUFFERED` | `3984B` | rotated-text visible alpha8 tile cache + transient layout/tile scratch |
| `EXTERN_IMAGE_RESIZE_565_8` | `1536B` | `2` 行 external shared RGB565+alpha row cache + resize `src_x_map` |
| `EXTERN_IMAGE_565_8` | `1440B` | `2` 行 external shared RGB565+alpha row cache |
| `EXTERN_IMAGE_ROTATE_565_8` | `1440B` | `2` 行 external transform-side shared RGB565+alpha row cache |

补充说明：

- 当前 whole-run heap peak 仍然由压缩 alpha 图片场景决定，而不是 text 场景。
- 所有尺寸相关 scratch 最终都会回到 `0B` current heap；默认路径没有常驻 heap 大块缓存。
- 当前 `9360B` 的压缩图热点已经可以精确拆开：`192` 列 tail-row cache 乘以 `16` 行、乘以 `RGB565 2B + alpha8 1B`，得到 `9216B`；再加首个 `48px` tile 的可见段瞬时 scratch `144B`，就是默认压缩 alpha 场景的实际 heap floor。`MASK_IMAGE_*_ROUND_RECT` 再叠加 `96B` circle-mask row cache，所以整轮 headline 是 `9456B`。
- 2026-03-29 对 `EGUI_CONFIG_IMAGE_CODEC_TAIL_ROW_CACHE_MAX_COLS` 做过 A/B：`144` 和 `96` 都会让 QOI/RLE alpha 场景退化到数倍，说明在当前 `240px` 屏宽、`PFB_WIDTH=48`、横向逐 tile refresh walk 下，`192` 尾列已经是默认单次解码路径的硬下限，而不是随手还能继续压的余量。
- 2026-03-29 第二轮 A/B 又测了 `176` 和 `184` 两档 tail cap，并配合 `EGUI_CONFIG_IMAGE_QOI_CHECKPOINT_COUNT=1/2` 与 `EGUI_CONFIG_IMAGE_RLE_CHECKPOINT_ENABLE=1`。相对当前 `d3d37bf` 基线（`IMAGE_QOI_565_8 2.258`、`EXTERN_IMAGE_QOI_565_8 2.734`、`IMAGE_RLE_565_8 1.561`、`EXTERN_IMAGE_RLE_565_8 2.385`），四组组合仍然退化 `+45% ~ +66%`，因此不再继续补 heap 数据，直接判定默认路径拒绝。
- 这背后的几何约束已经比较清楚：在 `240px` 图宽、`48px` 水平 PFB walk 下，首个可见 tile 覆盖 `0..47`，后续横向邻居需要的列并集是 `48..239`，正好就是 `192` 列。只要单次 tail cache 小于 `192`，最后一个 tile 就必然 miss，进而触发额外 row-band 重解；checkpoint 只能把解码状态恢复到 row-band 起点，并不能改变这个覆盖条件。这里是根据当前 walk 顺序做的推导，上面的实测数据正好印证了这个结论。

| 配置 | `IMAGE_QOI_565_8` | `EXTERN_IMAGE_QOI_565_8` | `IMAGE_RLE_565_8` | `EXTERN_IMAGE_RLE_565_8` | 结论 |
| --- | ---: | ---: | ---: | ---: | --- |
| 基线（`d3d37bf`） | `2.258` | `2.734` | `1.561` | `2.385` | 当前默认 |
| `tail=176, qoi_cp=1, rle_cp=1` | `3.616 (+60.14%)` | `4.536 (+65.91%)` | `2.270 (+45.42%)` | `3.911 (+63.98%)` | 拒绝 |
| `tail=176, qoi_cp=2, rle_cp=1` | `3.617 (+60.19%)` | `4.537 (+65.95%)` | `2.270 (+45.42%)` | `3.911 (+63.98%)` | 拒绝 |
| `tail=184, qoi_cp=1, rle_cp=1` | `3.629 (+60.72%)` | `4.544 (+66.20%)` | `2.284 (+46.32%)` | `3.927 (+64.65%)` | 拒绝 |
| `tail=184, qoi_cp=2, rle_cp=1` | `3.630 (+60.76%)` | `4.544 (+66.20%)` | `2.284 (+46.32%)` | `3.927 (+64.65%)` | 拒绝 |

- 按同样的几何关系推算，`176` 和 `184` 其实也只是在 `192` 列 tail 上少了 `16` / `8` 列，checkpoint 开销扣除前的 raw tail 节省不过 `768B` / `384B`。在已经实测到 `+45% ~ +66%` 退化的前提下，这条线没有继续做默认路径 heap 复测的必要。
- buffered rotated-text 的可见 alpha8 ceiling 仍保持 `3072B`：此前从 `3648B` 压到 `3072B` 时，两条 text 热点都再降了 `576B`，同时仍满足 `>500B => 10%` 的性能线；再往下到 `2560B` 会越线。
- 在这个 `3072B` ceiling 基础上，latest external rotated-text 又把 visible-tile external glyph scratch 从整 glyph heap block 改成 `14` 行 chunk stream，令 `EXTERN_TEXT_ROTATE_BUFFERED 4254B -> 4194B`，同时性能只从 `12.713ms` 变到 `12.885ms (+1.35%)`，满足更严格的 `<=500B => 5%` 线。
- 当前 next-largest 的非 codec heap 热点仍是 `EXTERN_TEXT_ROTATE_BUFFERED`，但它现在只比内部路径多 `210B`，说明 external glyph scratch 的额外成本已经被压到较小尾差。
- 当前默认 external raw-image 路径也已经强制遵守 `<=2` 行 / 列尺寸相关 heap 约束：shared row cache 从旧的 `1920/960` 收紧到 `960/480`，因此代表性 scene-local heap 变为 `EXTERN_IMAGE_565_8 1440B`、`EXTERN_IMAGE_RESIZE_565_8 1536B`、`EXTERN_IMAGE_ROTATE_565_8 1440B`。
- 这一轮属于“按 RAM 规则收口”的默认低 RAM 路径，而不是 perf-neutral 优化：旧的 `4` 行 external raw-image 默认 cache 已经超出当前 HelloPerformance 上限，因此现在接受一部分外部原始图片场景变慢，最差实测为 `EXTERN_IMAGE_565_1 +13.11%`、`EXTERN_IMAGE_ROTATE_565_1 +12.56%`、`EXTERN_IMAGE_ROTATE_TILED_565_8 +12.26%`。
- 最新 external transform row-cache 对齐则是在同样 `2` 行 cache 上追回 rotate 性能，不改变 `1440B` / `1536B` scene-local heap：`EXTERN_IMAGE_ROTATE_565_1 14.696 -> 13.474 (-8.32%)`、`EXTERN_IMAGE_ROTATE_565_2 15.508 -> 14.106 (-9.04%)`、`EXTERN_IMAGE_ROTATE_TILED_565_0 11.707 -> 10.630 (-9.20%)`、`EXTERN_IMAGE_ROTATE_TILED_565_8 17.633 -> 16.029 (-9.10%)`，而 direct draw / resize 保持 `0.00%`。
- 只要收益不足阈值，就优先保 RAM：`<=500B` RAM 变化用 `5%` 线，`>500B` RAM 变化用 `10%` 线。
- 如果后续还要继续压缩这一段 heap，手段就不能再是“简单缩 tail 列数”，而需要新的解码/渲染架构，例如改变 PFB walk 顺序，或引入更细粒度的 decoder checkpoint。

### 不接受的方向

下列方案即使性能收益明显，也不再接受：

| 方向 | 原因 |
| --- | --- |
| whole-image resident cache | 直接按整图尺寸申请 heap，违反 HelloPerformance 的 RAM 上限规则 |
| 用宏上限把整图或整屏 scratch 固定进 `.bss` / `stack` | 本质仍是尺寸相关 buffer，只是把风险从 heap 转移到静态区或栈 |

因此，后续允许保留的高性能变体只能是：

- 行缓存、列缓存、`PFB` 级 scratch；
- 最多 `2` 行 / `2` 列图像、屏幕尺寸相关 heap；
- 且必须和默认低 RAM 方案分开记录。

## 栈使用评估

`stack` 不是本文的主分类，但必须同步审计。当前 `-fstack-usage` 结果中，较大的栈帧如下：

| 函数 | 栈帧 | 是否在当前 HelloPerformance 活跃路径 | 说明 |
| --- | ---: | --- | --- |
| `egui_view_heart_rate_on_draw()` | `1200B` | 否（仅编译进入，已被当前镜像 GC 丢弃） | 大局部数组热点；`output/main.map` 只在 `Discarded input sections` 中出现 |
| `egui_view_virtual_tree_walk_internal()` | `1112B` | 否（仅编译进入，已被当前镜像 GC 丢弃） | 大局部 `frames[EGUI_VIEW_VIRTUAL_TREE_MAX_DEPTH]`；仅出现在 `Discarded input sections` |
| `line_hq_draw_polyline_segment()` | `976B` | 否（仅编译进入，已被当前镜像 GC 丢弃） | polyline helper 没有链接进当前 HelloPerformance benchmark 镜像 |
| `egui_canvas_draw_circle_fill_gradient()` | `752B` | 是 | 当前最终链接镜像里的最大单函数栈帧 |
| `egui_canvas_draw_image_transform()` | `736B` | 是 | 外部 whole-image cache 探测已撤掉，保持默认低 RAM 路径 |
| `egui_canvas_draw_text_transform()` | `376B` | 是 | 函数级 `optimize("Os")` 后由 `760B` 降到 `376B` |
| `egui_canvas_draw_line_hq()` | `288B` | 是 | 函数级 `optimize("Os")` 后由 `824B` 降到 `288B`，不再是当前 linked 栈热点 |

栈侧规则：

- 新增局部 buffer 之前，先判断它是否属于尺寸相关 buffer；如果是，直接走 `heap`。
- 即使不是尺寸相关 buffer，只要局部数组达到几百字节，也应该评估是否会把热路径 stack 顶高。
- 提交前建议至少对热路径跑一次 `-fstack-usage`，确认没有新的异常膨胀。

- `Map` 交叉复核结果：`.su` 里的 `1200B`、`1112B`、`976B` 这三个大栈帧，在当前 HelloPerformance 镜像里都只出现在 `Discarded input sections`，并未进入最终链接结果。
- 当前最终链接镜像里的最大单函数栈帧是 `egui_canvas_draw_circle_fill_gradient (752B)`；`egui_canvas_draw_text_transform` 已降到 `376B`、`egui_canvas_draw_line_hq` 已降到 `288B`，最终镜像中仍然没有 `>=1KB` 的链接热点。
- HelloPerformance 现在使用 `__qemu_min_stack_size__=0x0300`，即 `768B` QEMU 预留栈；这是当前经过完整 `cortex-m3` perf、运行时截图和单元测试复核后的最小已验证值。

## 建议的裁剪顺序

当用户继续压缩 RAM 时，建议按下面顺序判断：

1. 先把 `PFB` 当作应用配置项单独评估，不要把减小 `PFB` 误记成 core 优化成果。
2. 先排查是否还有尺寸相关 buffer 被放在 `.bss` 或 `stack`。
3. 再看是否存在“只带来轻微性能收益”的常驻 heap 或静态 cache。
4. 最后再考虑压缩少量固定元数据，因为这类收益通常是几十字节级别。

通常最值得优先关注的是：

- 图像解码 row cache / single-row scratch
- 外部图像 row cache
- 文字变换 scratch
- 外部字体 glyph scratch
- 跟随 `PFB_HEIGHT` 的 mask / round-rect row cache
- 跟随绘制宽度变化的 resize map

## 建议的记录口径

后续所有 RAM 相关提交，建议至少同步记录以下数据：

| 维度 | 建议记录内容 |
| --- | --- |
| 静态 RAM | `llvm-size` 的 `data/bss`，以及 `llvm-size -A` 拆开的 `.bss`、链接脚本保留区 |
| 固定静态大头 | `llvm-nm -S --size-sort --print-size` 的主要符号 |
| Heap | peak/current、alloc/free、常驻 heap 的 owner/lifetime/bytes |
| Stack | `-fstack-usage` 的热点函数，区分“当前活跃路径”和“仅编译进入” |
| 结论 | 是否有新的尺寸相关 buffer 落回 `.bss` 或 `stack` |

建议复测命令：

```bash
make clean
make all APP=HelloPerformance PORT=qemu CPU_ARCH=cortex-m3
llvm-size output/main.elf
llvm-size -A output/main.elf
llvm-nm -S --size-sort --print-size output/main.elf
```

Heap 观测：

```bash
make clean
make all APP=HelloPerformance PORT=qemu CPU_ARCH=cortex-m3 USER_CFLAGS="-DQEMU_HEAP_MEASURE=1 -DQEMU_HEAP_ACTIONS_APP_RECORDING=1 -DEGUI_CONFIG_RECORDING_TEST=1"
```

按 action 追踪 heap 峰值归因：

```bash
make clean
make all APP=HelloPerformance PORT=qemu CPU_ARCH=cortex-m3 USER_CFLAGS="-DQEMU_HEAP_MEASURE=1 -DQEMU_HEAP_ACTIONS_APP_RECORDING=1 -DQEMU_HEAP_TRACE_ACTIONS=1 -DEGUI_CONFIG_RECORDING_TEST=1"
```

Stack 观测：

```bash
make clean
make all APP=HelloPerformance PORT=qemu CPU_ARCH=cortex-m3 USER_CFLAGS="-fstack-usage"
```

同时建议每次同步更新：

- `example/HelloPerformance/ram_tracking.md`

## 小结

当前 EmbeddedGUI core 的 RAM 使用可以概括为：

- 固定静态 RAM 已经比较小，当前示例真正常驻的 `.data + .bss` 为 `2204B`，其中 `PFB` 就占了 `1536B`。
- 需要跟随字体、图片、屏幕、`PFB` 尺寸变化的 buffer，必须继续保持 `heap` 化，不能为了追求“heap=0”回退到静态区或大栈。
- 当前默认 heap 峰值 `9456B` 主要来自运行时 scratch，空间可回到 `0B`；若后续引入常驻 heap，必须明确记录 owner、lifetime、bytes。
- 对于高性能变体，也只能在 `1 * PFB` 或 `2` 行 / 列尺寸相关 heap 的约束内做选择，不能回到 whole-image cache。
