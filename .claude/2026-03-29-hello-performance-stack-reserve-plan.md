# 2026-03-29 HelloPerformance 栈预留继续收敛 Plan

## 目标

- 在不偏离 `HelloPerformance` 的前提下，继续压缩当前最终链接镜像里的活跃栈热点。
- 如果活跃路径最大栈帧还能下降，就继续同步下压 QEMU 链接脚本保留栈，直接换回静态 RAM。
- 保持现有 heap 规则不变：尺寸相关 buffer 继续走 heap，不回退到 `.bss`、静态局部或大栈数组。

## 执行步骤

1. 复核当前 `HelloPerformance/QEMU` 的 `llvm-size`、`llvm-size -A`、`-fstack-usage`、`main.map`。
2. 只对当前最终链接镜像里的栈热点做函数级 `optimize("Os")` 压栈，不改尺寸相关 buffer 的归属。
3. 重新跑 heap measurement，确认 whole-run `heap peak/current/alloc/free` 不变。
4. 复核 `runtime`、`unit test`、`perf`，并同步更新 `ram_tracking.md` 和 `core_ram_guide.md`。

## 结果

- 本轮把 `example/HelloPerformance/build.mk` 的 `__qemu_min_stack_size__` 从 `0x0300` 继续压到 `0x0220`。
- 活跃路径最大 linked 栈帧从 `752B` 下降到 `536B`，当前最大值变为：
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
- 本轮函数级 `optimize("Os")` 不但没有带来 perf 回退，几项几何/渐变/阴影场景还有明显提升。

## 结论

- 这轮属于“继续压活跃栈帧，再把链接预留栈换回静态 RAM”的收敛，不涉及新的尺寸相关 buffer，也没有把 heap 压力转移回 `.bss` 或 `stack`。
- 当前 `__qemu_min_stack_size__=0x0220` 是最新已验证可用值，后续如果还想继续下压，必须继续以最终链接镜像里的活跃帧为准，不能看未链接进镜像的 `.su` 假热点。
