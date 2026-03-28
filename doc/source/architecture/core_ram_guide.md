# Core RAM 使用分布与裁剪指南

## 文档范围

- 本文面向 `EmbeddedGUI core/framework` 层，说明项目 RAM 的主要组成、当前典型大头，以及裁剪时的基本规则。
- 当前字节数示例来自 `HelloPerformance + PORT=qemu + CPU_ARCH=cortex-m3` 的现有构建，用于帮助定位 RAM 大头，不代表所有应用和所有端口都完全相同。
- `PFB` 会单独列出，但它本身是用户配置项，不计作框架 RAM 优化成果。
- 业务层自定义 view、应用资源、端口私有缓冲区、链接脚本保留的 stack/heap 空间，也会占用同一片 SRAM；本文重点解释 core 侧的占用方式和裁剪原则。

## RAM 三类

EmbeddedGUI 的 RAM 建议按下面三类来看：

| 类别 | 典型位置 | 典型内容 | 规则 |
| --- | --- | --- | --- |
| 静态 RAM | `.bss` / `.data` | 核心状态机、定时器、少量固定元数据、`PFB` | 只保留与尺寸无关、上界稳定、收益明确的小对象 |
| 跟随尺寸走的 buffer | 运行时 scratch/cache | 跟随字体大小、图片大小、屏幕尺寸、`PFB` 尺寸变化的 buffer | 必须走 `heap`，不能为了避免 heap 而改成宏固定 RAM、静态数组或大栈数组 |
| Heap | 每次绘制 / 每帧 / 跨帧缓存 | 图像解码 row cache、文字变换 scratch、外部字体 scratch、图片 resize scratch 等 | 优先短生命周期；若必须常驻，必须记录 owner、lifetime、bytes |

额外需要单独评估的一类是 `stack`：

- `stack` 不属于上面三类之一，但它和 `.bss/.data/heap` 竞争同一片 SRAM。
- 大局部数组同样可能把系统顶爆。
- 任何新引入的大栈对象，都必须说明必要性，并用 `-fstack-usage` 复核。

## 当前示例总览

当前 `HelloPerformance/QEMU` 构建的 RAM 观测值如下：

| 项目 | 当前值 | 说明 |
| --- | ---: | --- |
| `text` | `2160560B` | 代码和只读数据 |
| `data` | `48B` | 已初始化静态 RAM |
| `bss` | `10272B` | `llvm-size` 汇总值，包含链接脚本保留区 |
| `.bss` | `2080B` | 实际未初始化静态符号区 |
| `._user_heap_stack` | `8192B` | 当前链接脚本保留区，不是 core 固定对象本身 |
| 固定静态 RAM 小计 | `2128B` | `.data + .bss`，不含链接脚本保留区 |
| Heap 峰值 | `11616B` | `QEMU_HEAP_MEASURE=1` 实测峰值 |
| Heap 空闲 current | `0B` | 当前示例中的尺寸相关 scratch 最终会释放 |
| alloc/free | `7403 / 7403` | 交互完成后配平 |
| 当前活跃路径最大栈帧 | `976B` | `line_hq_draw_polyline_segment()` |

说明：

- `llvm-size` 的 `bss=10272B` 里包含了 `._user_heap_stack=8192B`，所以看总表时不要把它全部当成 core 固定静态 RAM。
- 从 core 固定对象角度看，当前真正长期常驻的 `.data + .bss` 只有 `2128B`。
- 其中 `egui_pfb=1536B` 是当前应用配置下的 `PFB`，它是用户自己选择的空间换时间项，不应被当作框架裁剪成果。
- `heap peak=11616B` 是运行时峰值，不是 idle 常驻占用；当前示例结束后 `current heap` 回到 `0B`。

## 静态 RAM 分布

下面是当前示例中最主要的固定静态符号：

| 符号 | 字节 | 分类 | 说明 |
| --- | ---: | --- | --- |
| `egui_pfb` | `1536B` | 用户配置项 | 当前 `PFB` 像素缓冲区 |
| `qoi_state` | `208B` | core 元数据 | QOI 解码器状态 |
| `egui_core` | `120B` | core 元数据 | 核心运行时状态 |
| `test_view` | `56B` | app 示例对象 | HelloPerformance 场景对象 |
| `canvas_data` | `32B` | core 元数据 | canvas 运行时状态 |
| `ui_timer` | `20B` | app 示例对象 | app 定时器 |
| `port_display_driver` | `16B` | port 元数据 | QEMU 端口显示驱动状态 |
| `g_egui_mask_circle_frame_row_cache` | `16B` | heap handle 元数据 | 只保存 owner 和指针，真正 row cache 在 heap |
| `rle_state` | `16B` | core 元数据 | RLE 解码状态 |
| `g_font_std_code_lookup_cache` | `12B` | core 元数据 | 字体 code lookup 缓存元数据 |
| `egui_image_decode_cache_state` | `8B` | core 元数据 | 图像解码缓存命中状态 |
| `g_text_transform_visible_tile_cache` | `8B` | heap handle 元数据 | 只保存 heap 缓冲句柄 |
| `s_qemu_resource_handle` | `4B` | port 元数据 | QEMU 资源句柄 |

上表合计 `2052B`，剩余约 `76B` 是零散小符号。

静态 RAM 的判断原则：

- 可以留在静态区的，应该是与字体大小、图片大小、屏幕尺寸、`PFB` 尺寸无关的小型状态对象。
- 如果静态对象只是某个 heap buffer 的 owner / capacity / pointer 元数据，通常可以接受，因为真正的大块数据已经不再常驻 `.bss`。
- `PFB` 必须单独看。它通常是静态 RAM 里最大的一项，但这是应用配置，不是 core 算法残留。

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
| 压缩图 row-band cache | `egui_image_decode_*` | `row_count * image_width * bytes_per_pixel`，通常受 `PFB_HEIGHT` 和图片宽度影响 | `heap` | 当前 refresh walk |
| 图片 resize `src_x_map` | `egui_image_std_*` | 当前绘制宽度 | `heap` | 单次绘制 |
| round-rect 图片快速路径 row cache | `egui_image_std_*` | `min(PFB_HEIGHT, image_height)` | `heap` | 当前 refresh walk |
| circle mask row cache | `egui_mask_circle_*` | `3 * PFB_HEIGHT * sizeof(egui_dim_t)` | `heap` | 当前帧 |
| rotated text layout scratch | `egui_canvas_draw_text_transform()` | glyph 数、line 数、字体布局结果 | `heap` | 单次绘制 |
| rotated text tile scratch | `egui_canvas_draw_text_transform()` | 可见 glyph 数、line 数 | `heap` | 单次绘制 |
| 外部字体 glyph / row scratch | `egui_font_std_*`、`egui_canvas_transform_*` | 字体大小、glyph bitmap 尺寸 | `heap` | 单次绘制 / 当前 refresh walk |
| 外部图像 full-image persistent cache（可选 HQ） | `egui_image_std_prepare_external_persistent_cache()` | 图片宽高、像素格式、alpha 格式 | `heap` | 跨帧常驻，默认关闭 |

这类 buffer 的共性是：

- 大小不是常数，而是由当前资源或当前配置决定。
- 同一套代码换个字体、换张图、换屏幕、换 `PFB` 后，RAM 需求会一起变。
- 因此它们必须动态申请，并在使用完成后释放，或者至少收敛到“当前 frame 内有效”的生命周期。

### 关于宏的使用边界

这里需要特别强调：

- 可以用宏表达“上限规则”或“是否启用某功能”。
- 但不能用宏把尺寸相关 buffer 直接落成固定 `.bss`、静态局部数组或大栈数组。
- 也就是说，宏可以决定是否需要一块 buffer，或者限制最大预算；真正的大块存储仍然必须走 `heap`。

例如：

- `EGUI_CONFIG_IMAGE_DECODE_ROW_BUF_WIDTH` 这类宏可以参与计算“需要多大”。
- 但真正的 decode row scratch、row-band cache、glyph scratch、resize map，不应因此变成固定全局数组。

## Heap 分布

当前示例的 heap 以短生命周期 scratch 为主，峰值 `11616B`，空闲回到 `0B`。

### 当前主要 heap 大头

| 类型 | 典型 owner | 特征 | 备注 |
| --- | --- | --- | --- |
| 压缩图 row-band cache | `egui_image_decode_cache_prepare_rows()` | 当前峰值主力 | 用空间换掉横向 tile 重复解码 |
| rotated text scratch | `egui_canvas_draw_text_transform()` | 随文本布局结果变化 | 已改成按 glyph/line 数动态申请 |
| 图片 resize / round-rect scratch | `egui_image_std_*` | 随绘制宽度、图片高度、`PFB_HEIGHT` 变化 | 使用完立即释放 |
| circle mask frame scratch | `egui_mask_circle_*` | 随 `PFB_HEIGHT` 变化 | 帧结束释放 |
| 外部字体 scratch | `egui_font_std_*` / `egui_canvas_transform_*` | 随 glyph bitmap 尺寸变化 | 使用完或 frame 结束释放 |

### 常驻 heap 的处理原则

如果某个 heap buffer 不是瞬时 scratch，而是要跨帧常驻，必须在文档或提交说明中记录：

- owner
- lifetime
- bytes
- 为什么不能改成短生命周期
- 带来的性能收益是否超过 `5%`

若性能收益不足 `5%`，优先选择更省 RAM 的方案，而不是为了边际性能保留大块常驻 heap。

补充一条针对性能/HQ 变体的规则：

- 如果某个“跟随字体大小、图片大小、屏幕尺寸、`PFB` 尺寸变化”的常驻 heap cache 能带来 `>=2x` 的性能收益，可以保留一条单独的性能/HQ 路径。
- 这条 HQ 路径应与默认低 RAM 路径分开，并明确记录 `owner / lifetime / bytes / speedup`，避免默认配置在无感收益下长期占用 SRAM。

### 当前代码里的常见 heap 生命周期

| 生命周期 | 典型释放点 | 适用场景 |
| --- | --- | --- |
| 单次绘制 | 函数 `cleanup` / `return` 前 | text transform、glyph scratch、resize map |
| 当前帧 | `egui_*_release_frame_cache()` | 需要跨多个 `PFB tile` 复用、但不需要跨帧保留 |
| 跨帧常驻 | 模块 deinit / cache reset | full-image persistent cache、少数长期缓存 |

当前 core 已经把一批“只需跨当前 refresh walk”的 buffer 收敛到了帧级释放，例如：

- `egui_canvas_transform_release_frame_cache()`
- `egui_image_std_release_frame_cache()`
- `egui_mask_circle_release_frame_cache()`
- `egui_font_std_release_frame_cache()`

这类 buffer 可以跨当前帧内的多个 `PFB tile` 复用，但不会无限期占住 SRAM。

## 栈使用评估

`stack` 不是本文的主分类，但必须同步审计。当前 `-fstack-usage` 结果里，较大的栈帧如下：

| 函数 | 栈帧 | 是否在当前 HelloPerformance 活跃路径 | 说明 |
| --- | ---: | --- | --- |
| `egui_view_heart_rate_on_draw()` | `1200B` | 否 | 框架已编入，但不是当前录制路径热点 |
| `egui_view_virtual_tree_walk_internal()` | `1112B` | 否 | 框架已编入，但不是当前录制路径热点 |
| `line_hq_draw_polyline_segment()` | `976B` | 是 | 当前活跃路径最大栈帧 |
| `egui_canvas_draw_line_hq()` | `824B` | 是 | 线段 HQ 路径 |
| `egui_canvas_draw_text_transform()` | `760B` | 是 | 旋转文本 scratch 已迁到 heap 后显著下降 |
| `egui_canvas_draw_image_transform()` | `736B` | 是 | 当前仍需持续关注 |

栈侧的裁剪规则：

- 新增局部 buffer 之前，先判断它是否属于“尺寸相关 buffer”；如果是，直接走 `heap`。
- 即使不是尺寸相关 buffer，只要局部数组达到几百字节，也应该评估是否会把热路径 stack 顶高。
- 提交前建议用 `-fstack-usage` 复核热点函数，至少确认当前实际路径上没有异常膨胀。

## 当前建议的裁剪顺序

当用户需要继续压缩 RAM 时，建议按下面顺序判断：

1. 先把 `PFB` 当作应用配置项单独评估，不要把减小 `PFB` 误记成 core 优化成果。
2. 先排查是否还有尺寸相关 buffer 被放在 `.bss` 或 `stack`。
3. 再看是否存在“只带来轻微性能收益”的常驻 heap 或静态 cache。
4. 最后再考虑压缩少量固定元数据，因为这类收益通常是几十字节级别。

通常最值得优先关注的是：

- 图片解码 row cache / single-row scratch
- 外部图片 row cache
- 文字变换 scratch
- 外部字体 glyph scratch
- 跟随 `PFB_HEIGHT` 的 mask / round-rect row cache
- 跟随绘制宽度变化的 resize map

## 例外与注意事项

### `PFB` 不是优化目标

- `PFB` 是用户自己在流畅度和 SRAM 之间做的配置取舍。
- 文档里要列出它，因为它通常是静态 RAM 大头。
- 但提交说明不应把“改小 `PFB`”写成 core RAM 优化成果。

### 现有 legacy 路径需要单独标注

目前源码中仍存在少数“功能关闭时不生效、开启时会带来尺寸相关 RAM”的路径，需要在启用时额外审查。例如：

- `EGUI_CONFIG_SOFTWARE_ROTATION=1` 时，`egui_core.c` 中的软件旋转 scratch 会跟随 `PFB_WIDTH * PFB_HEIGHT` 变化。

这类路径如果要在小 RAM 平台上启用，应优先评估 heap 化或明确记录其 RAM 成本，而不是默认忽略。

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

Stack 观测：

```bash
make clean
make all APP=HelloPerformance PORT=qemu CPU_ARCH=cortex-m3 USER_CFLAGS="-fstack-usage"
```

## 小结

当前 EmbeddedGUI core 的 RAM 使用可以概括为：

- 固定静态 RAM 已经较小，当前示例真正常驻的 `.data + .bss` 仅 `2128B`，其中 `PFB` 就占了 `1536B`。
- 需要跟随字体、图片、屏幕、`PFB` 变化的 buffer，必须继续保持 `heap` 化，不能为了追求“heap=0”而回退到静态区或大栈。
- 当前 heap 峰值 `11616B` 主要是运行时 scratch，空闲可回到 `0B`；后续若引入常驻 heap，必须明确记录 owner、lifetime、bytes。
- 后续 RAM 优化应继续优先盯住“尺寸相关 buffer 是否回退放错位置”，而不是只追几十字节的零散静态对象。
