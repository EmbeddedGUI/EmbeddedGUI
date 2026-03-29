# 2026-03-29 HelloPerformance 栈预留收敛 Plan

## 目标

- 在不偏离 `HelloPerformance` 的前提下，继续压缩当前最终链接镜像里的活跃栈热点。
- 如果活跃路径最大栈帧还能继续下降，就同步下压 QEMU 链接脚本保留栈，把静态 RAM 换回来。
- 保持现有 heap 规则不变：尺寸相关 buffer 继续走 `heap`，不回退到 `.bss`、静态局部或大栈数组。

## 执行步骤

1. 复核当前 `HelloPerformance/QEMU` 的 `llvm-size`、`llvm-size -A`、`-fstack-usage`、`main.map`。
2. 只对当前最终链接镜像里的活跃栈热点做函数级压栈，不改尺寸相关 buffer 的归属。
3. 重新复核 `runtime`、`unit test`、`perf`，并同步更新 `ram_tracking.md` 和 `core_ram_guide.md`。

## 首轮结果

- 这一轮把 `example/HelloPerformance/build.mk` 的 `__qemu_min_stack_size__` 从 `0x0300` 压到 `0x0220`。
- 活跃路径最大 linked 栈帧从 `752B` 降到 `536B`，当时最大值变为：
  - `egui_canvas_draw_polygon_fill_gradient = 536B`
  - `egui_canvas_draw_polygon_fill = 536B`
- 正常 QEMU 构建口径变为：
  - `text=2140376`
  - `data=52`
  - `bss=2700`
  - `static RAM=2752`
- `llvm-size -A` 复核结果：
  - `.bss=2152`
  - `._user_heap_stack=548`
- heap measurement 复核结果保持不变：
  - `interaction_total_peak=9456`
  - `interaction_total_current=0`
  - `alloc/free=8069/8069`

## 结论

- 这轮属于“继续压活跃栈帧，再把链接预留栈换回静态 RAM”的收敛，不涉及新的尺寸相关 buffer，也没有把 heap 压力转移回 `.bss` 或 `stack`。
- 当时 `__qemu_min_stack_size__=0x0220` 是最新已验证可用值；后续如果还想继续下压，必须继续以最终链接镜像里的活跃帧为准，不能看未链接进镜像的 `.su` 假热点。

## 2026-03-30 Follow-up

- 后续又继续做过一次 `0x01b0 -> 0x0180` 的 follow-up A/B，思路是先把 `egui_canvas_draw_thick_line_scan()` 临时拆到 `384B`，再继续压 QEMU reserve。
- fresh clean `-fstack-usage` / `main.map` / `llvm-nm` 复核后，确认这条线不能直接提交。原因不是 thick-line 这一个函数，而是当前真正的 linked ceiling 会立刻转移到：
  - `egui_canvas_draw_circle_fill_gradient = 424B`
  - `egui_canvas_draw_rectangle_fill_gradient = 416B`
  - `egui_canvas_draw_polygon_fill_gradient = 408B`
  - `egui_canvas_draw_polygon_fill = 408B`
  - `egui_image_rle_draw_image = 392B`
- 结论是：当前 shipped 口径仍然保持 `__qemu_min_stack_size__=0x01b0`。
- 继续为了 `<=24B` 的静态 RAM headline 去追这条线，不符合“几个字节的不管、重点看大头”的规则。
- 后续如果还要继续做 stack reserve 方向，必须成组地下移这批 active linked hotspot，而不是只盯着单个 `432B` 函数。
