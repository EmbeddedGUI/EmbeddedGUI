# Virtual Stage Showcase

## 这个例程是什么

`HelloVirtual/virtual_stage_showcase` 的目标不是做一个“风格接近 `HelloShowcase`”的新页面，
而是在视觉上尽量保持和 `HelloShowcase` 一致，同时把底层承载方式切换成真正的 `virtual_stage`。

用户看到的仍然是一整页 showcase 效果，但 SRAM 不再保存整棵 showcase 控件树。

## 当前实现方式

1. 页面被拆成多个 `virtual_stage` 节点，而不是把整个 `HelloShowcase` 根视图塞进 stage。
2. 大块静态区域使用 `render-only` 节点，由 stage 的 `draw_node` 直接绘制。
3. 真实可交互控件单独作为独立节点，按需 materialize，包括：
   - `Button`
   - `TextInput`
   - `Switch`
   - `Checkbox`
   - `ToggleButton`
   - `Slider`
   - `NumberPicker`
   - `Roller`
   - `Combobox`
   - `List`
   - 两个重叠 layer card
4. 固定节点数组直接通过 `EGUI_VIEW_VIRTUAL_STAGE_NODE_ARRAY_SCREEN_STATEFUL_BRIDGE_INIT_WITH_LIMIT(...)` 接到 stage，全屏 rich case 不再单独维护 `params / node_source / array_adapter / ops / setup`。
5. `save_state / restore_state / hit_test / should_keep_alive` 这组常见组合直接跟 bridge 一起声明，用户只需要关注回调本身，不需要再单独理解 `ops` 对象。
6. 默认 `live_slot_limit` 设为 `2`，只在 parity 录制路径下临时放宽到 `20`，页面运行时只保留少量活跃真实控件。
7. `Keyboard` 继续作为 stage 外的 root view 单独挂载，保持输入交互正常。
8. parity 录制路径下需要一次性保活多组真实控件时，直接用 `EGUI_VIEW_VIRTUAL_STAGE_PIN_IDS(...)` 批量 pin，不再逐个写一长串 `PIN(...)`。
9. 该例程单独把 `EGUI_CONFIG_DIRTY_AREA_COUNT` 提升到 `16`。目的不是扩大刷新范围，而是给语言切换、多块 panel 重绘和动画子节点局部刷新留出更稳定的 dirty 合并空间，尽量避免过早退化成 `fallback_union`。

如果后续基于这个例程继续加状态联动，少量节点的一次性刷新优先直接用 `EGUI_VIEW_VIRTUAL_STAGE_NOTIFY_IDS(...)` / `EGUI_VIEW_VIRTUAL_STAGE_NOTIFY_BOUNDS_IDS(...)`；只有在 `stable_id` 数组需要复用时，再继续用 `EGUI_VIEW_VIRTUAL_STAGE_NOTIFY_NODES(...)` / `EGUI_VIEW_VIRTUAL_STAGE_NOTIFY_NODES_BOUNDS(...)`。

## 这个例程验证了什么

- 视觉效果保持为 showcase 单页
- 真实控件按需创建、销毁、恢复状态
- `Combobox` 展开、`List` 滚动等状态可以跨 materialize 保留
- stage 的 live slot 数量被限制，不再常驻整页控件树

## 为什么看起来动效少了

`virtual_stage_showcase` 不是把 `HelloShowcase` 的整棵原生控件树直接挂到页面上，而是拆成了两类节点：

- 少量真实 live widget：负责交互，按需 materialize
- 大量 render-only 节点：由 `draw_node()` 直接重绘快照

所以“看起来更静”通常不是因为动画定时器停了，而是因为很多原来由真实控件内部自己驱动的动画，已经被改成了“外部 tick + render-only 重绘”。

当前实现里：

- 正常构建下 `showcase_anim_cb()` 会持续递增 `showcase_ctx.anim_tick`
- 当前不会再整页 `invalidate`，而是只对 7 个动画子节点调用 `EGUI_VIEW_VIRTUAL_STAGE_NOTIFY_IDS(...)`
- 这些子节点分别覆盖 `ProgressBar`、`CircularProgressBar`、`ActivityRing`、`Spinner`、`AnalogClock`、`DigitalClock/Stopwatch`、`HeartRate`
- 这样可以把高频动画 dirty 控制在更小范围内，但 render-only 路径仍不会保留原控件内部的完整自驱动画状态机，所以观感会比 `HelloShowcase` 更克制

如果目标是“动效也和 `HelloShowcase` 一模一样”，那就不能只保留现在这批 live widget，而要把对应动画控件也改成 live widget，并在动画期间 keepalive / pin。代价就是 live SRAM/heap 会继续上涨。

另外，parity 录制构建（`-DEGUI_SHOWCASE_PARITY_RECORDING=1`）会关闭两边的动画定时器，方便做静态逐帧对比；默认例程构建不是这个路径。

## 和 `HelloShowcase` 的取舍

这两个例程的目标不同：

- `HelloShowcase` 更适合直接展示完整控件页，开发和排查最直接，不需要拆节点、外置状态或维护 `stable_id`
- `HelloVirtual/virtual_stage_showcase` 更适合在视觉保持一致的前提下，把整页控件树改成“少量真实控件 + 大量 render-only 节点”

这里不再直接用整个 ELF 的 `data + bss` 做 SRAM 对比。那个口径会把 `PFB`、平台移植层、框架公共静态数据一起算进去，不适合衡量 `virtual_stage` 自身带来的收益。

新的对比口径是：

- 只统计示例自身对象文件的 `.data* + .bss*`
- `HelloShowcase` 统计目录：`output/obj/HelloShowcase_stm32g0/example/HelloShowcase/`
- `HelloVirtual/virtual_stage_showcase` 统计目录：`output/obj/HelloVirtual_virtual_stage_showcase_stm32g0/example/HelloVirtual/virtual_stage_showcase/`

以 `PORT=stm32g0`、`COMPILE_OPT_LEVEL=-Os` 为例：

| 示例 | 示例自身静态 SRAM |
| --- | ---: |
| `HelloShowcase` | 13248 B |
| `HelloVirtual/virtual_stage_showcase` | 7128 B |

结论：

- `virtual_stage_showcase` 比 `HelloShowcase` 少占 `6120` 字节示例自身静态 SRAM，约 `46.2%`
- `HelloShowcase` 的大头是整页真实控件树常驻，例如 `wg_keyboard`、`wg_list`、`wg_table`
- `virtual_stage_showcase` 的大头变成了 `showcase_stage_view`、`showcase_scratch`、`showcase_ctx` 和少量 live slot 运行所需状态

这个口径天然排除了：

- `PFB`
- 平台移植层自身 SRAM
- 框架公共静态 SRAM

但它也不包含运行时栈/堆。也就是说，这里衡量的是“示例自身静态保留了多少 SRAM”，而不是整机运行时总 SRAM 峰值。

## QEMU heap 实测

如果要看运行时 heap，要单独跑 QEMU：

```bash
python scripts/compare_virtual_showcase_heap_qemu.py --mode app-recording
```

`app-recording` 模式会直接复用 `HelloShowcase` 和 `virtual_stage_showcase` 各自的 `egui_port_get_recording_action()`，因此比统一假动作更接近页面真实交互。

`2026-03-22` 的 QEMU 实测结果：

| 示例 | idle current heap | interaction total peak heap |
| --- | ---: | ---: |
| `HelloShowcase` | 0 B | 0 B |
| `HelloVirtual/virtual_stage_showcase` | 2488 B | 3328 B |

这说明：

- `HelloShowcase` 这页主要是静态全局控件，页面自身几乎不依赖运行时 `malloc`
- `virtual_stage_showcase` 省掉了整页真实控件树的静态 SRAM，但会引入一部分运行时 heap
- 为了稳定保住多区域 partial refresh，本例程额外保留了更多 dirty slot；这部分只是小规模静态数组开销，和整页真实控件常驻相比仍然是次要成本

heap 主要来自三类来源：

- `virtual_stage` 自身的节点缓存和绘制顺序缓存，会在 `egui_view_virtual_stage_reload_cache()` 里分配 `node_cache` 和 `draw_order`
- 正常模式下默认保留的少量 live widget，尤其 `List` 节点带 `KEEPALIVE`，所以 idle 阶段就会有少量常驻 heap
- 交互时按需 materialize 的真实控件，`showcase_adapter_create_view()` 会为 `Button`、`TextInput`、`Slider`、`Combobox`、`List` 等控件动态 `egui_malloc()`

从这次实测看：

- idle 阶段有 `3` 次分配，主要是 stage 元数据缓存和少量默认常驻 live widget
- 交互阶段额外出现 `24` 次分配和 `24` 次释放，说明 heap 峰值主要来自交互过程中的 live widget 创建/释放

所以 `virtual_stage_showcase` 的内存 tradeoff 是：

- 静态 SRAM 明显下降
- 运行时 heap 小幅上升，而且这部分上升和 `live_slot_limit`、keepalive、控件类型直接相关

如果后续要继续优化 heap，优先看这几项：

- 继续减少默认 keepalive 节点，优先压 `List`、展开态 `Combobox`
- 严格控制 `SHOWCASE_LIVE_SLOT_LIMIT`
- 纯展示节点继续保持 render-only，不要升级成 live widget
- 对较重控件评估“展开时创建，结束后立即释放”，不要长期 keepalive
- 如果目标是峰值可控，可以把 stage cache / pin 表 / live view 进一步改成固定池或专用 arena

## 关键文件

- `example/HelloVirtual/virtual_stage_showcase/test.c`
- `example/HelloShowcase/uicode.c`
- `src/widget/egui_view_virtual_stage.h`
- `src/widget/egui_view_virtual_stage.c`

## 运行

构建：

```bash
make all APP=HelloVirtual APP_SUB=virtual_stage_showcase PORT=pc
```

运行时检查：

```bash
python scripts/code_runtime_check.py --app HelloVirtual --app-sub virtual_stage_showcase --keep-screenshots
python scripts/showcase_stage_parity_check.py --timeout 35 --bits64
```

HelloVirtual 渲染工作流：

```bash
python scripts/hello_basic_render_workflow.py --app HelloVirtual --widgets virtual_stage_showcase --skip-unit-tests --bits64
```

`stm32g0` 对比构建：

```bash
make -j1 all APP=HelloShowcase PORT=stm32g0
make -j1 all APP=HelloVirtual APP_SUB=virtual_stage_showcase PORT=stm32g0
python scripts/compare_virtual_showcase_ram.py --skip-build
python scripts/compare_virtual_showcase_heap_qemu.py --mode app-recording
```
