# HelloPerformance SVG PFB 优化记录

## 目标

- 参考 `egui_image_file`，在 `HelloPerformance` 增加 `egui_image_svg` 性能测试场景
- 重点优化 PFB 场景下的运行时 SVG 性能
- 性能数据全部基于 QEMU

## 本轮改动

- 在 `HelloPerformance` 新增两个 runtime SVG case
  - `IMAGE_SVG_PFB_TILED`
  - `IMAGE_SVG_PFB_TILED_RESIZE`
- 打开 `HelloPerformance` 的 runtime SVG 配置开关
- 修复 `uicode.c` 中新增枚举值的字符串映射与逻辑 PFB hint 分支
- 为 PlutoSVG / PlutoVG 接入 `egui_malloc/egui_free` allocator 包装，避免 QEMU 下标准库 heap 与 egui heap 混用导致的 SVG 输入缓冲被覆盖
- 给 `egui_image_svg` 补上 `get_size` API，保证 tiled direct 场景真正按 SVG 原始尺寸平铺
- 在 `egui_image_svg` 中加入 raster cache 透明边界裁剪
  - 渲染完成后扫描 alpha 包围盒
  - 仅缓存非透明区域的 RGB565 + A8 数据
  - 记录缓存偏移，在 `draw/get_point` 路径回补偏移
- 为 `IMAGE_SVG_PFB_TILED` / `RESIZE` 设置逻辑 PFB 宽度 hint 为 `96`
- 为性能场景使用内嵌 badge SVG，确保运行时检查和 QEMU benchmark 一致
- 在 `HelloUnitTest` 中补一条 `egui_image_get_size()` 的 SVG 用例

## 关键定位结论

- 初始解析失败的根因不是 SVG 文本本身，而是 QEMU 下 PlutoSVG/PlutoVG 继续使用标准 `malloc/calloc/realloc/free`
- runtime SVG 文本复制到 `egui_malloc` 堆后，会被第三方库标准堆分配过程破坏
- allocator 统一后，QEMU 单 case 恢复正常，`PERF_RESULT` 可以稳定输出

## QEMU 性能结果

### 单 case

- `IMAGE_SVG_PFB_TILED`: `0.657 ms`
- `IMAGE_SVG_PFB_TILED_RESIZE`: `0.533 ms`

对应报告：

- `perf_output/perf_report.md`

### PFB Matrix

- `small (15x15)`: `1.288 ms`
- `middle (30x30)`: `0.601 ms`
- `fullwidth (240x1)`: `1.093 ms`
- `fullheight (1x240)`: `1.068 ms`
- `fullscreen (240x240)`: `0.344 ms`

对应报告：

- `perf_output/pfb_matrix_report.md`

## 逻辑 PFB 宽度试探

本轮额外对 `IMAGE_SVG_PFB_TILED` 做了逻辑 PFB 宽度候选试探：

- 默认 `96x8`: `0.657 ms`
- 候选 `128x6`: `0.600 ms`
- 候选 `192x4`: `0.726 ms`

但 `128x6` 在 matrix 中没有整体优势：

- `middle`: `0.627 ms`，劣于 `96x8` 的 `0.601 ms`
- `fullheight`: `1.088 ms`，劣于 `96x8` 的 `1.068 ms`

因此保留 `96x8` 作为更稳妥的 shipped 默认值，优先保证窄条 PFB 场景整体收益。

## 运行验证

- `python scripts/perf_analysis/main.py --profile cortex-m3 --clean --timeout 300 --filter IMAGE_SVG_PFB_TILED --extra-cflags="-DEGUI_TEST_CONFIG_SINGLE_TEST=EGUI_VIEW_TEST_PERFORMANCE_TYPE_IMAGE_SVG_PFB_TILED"`：通过
- `python scripts/perf_analysis/main.py --profile cortex-m3 --clean --timeout 300 --filter IMAGE_SVG_PFB_TILED_RESIZE --extra-cflags="-DEGUI_TEST_CONFIG_SINGLE_TEST=EGUI_VIEW_TEST_PERFORMANCE_TYPE_IMAGE_SVG_PFB_TILED_RESIZE"`：通过
- `python scripts/perf_analysis/main.py --pfb-matrix --clean --profile cortex-m3 --timeout 300 --extra-cflags="-DEGUI_TEST_CONFIG_SINGLE_TEST=EGUI_VIEW_TEST_PERFORMANCE_TYPE_IMAGE_SVG_PFB_TILED"`：通过
- `python scripts/code_runtime_check.py --app HelloPerformance --timeout 10 --keep-screenshots`：`ALL PASSED`

截图确认：

- `runtime_check_output/HelloPerformance/default/frame_0256.png`: `IMAGE_SVG_PFB_TILED`
- `runtime_check_output/HelloPerformance/default/frame_0257.png`: `IMAGE_SVG_PFB_TILED_RESIZE`

两个 SVG 场景都正常平铺，无黑屏、无空白。

## 残留问题

- `make -j APP=HelloUnitTest PORT=pc_test && output/main.exe` 仍有 3 条 `image_svg` opacity 相关断言失败：
  - `test_image_svg_group_opacity_multiplies_fill_opacity`
  - `test_image_svg_rgba_function_fill_uses_alpha`
  - `test_image_svg_rect_stroke_opacity`
- 这些失败目前集中在 runtime SVG 的半透明颜色断言，不阻塞本轮 `HelloPerformance` SVG PFB benchmark 接入与性能优化，但需要后续单独收口

## 第二轮 PFB 优化（2026-04-17）

### 本轮改动

- `perf_draw_svg_tiled_direct()` 不再遍历整屏所有 tile，而是先读取 `egui_canvas_get_base_view_work_region()`，只迭代与当前 PFB work region 相交的 tile 范围
- `egui_image_svg` 在 raster cache 命中后复用 `egui_image_std_t` 包装对象，避免每次 draw/get_point 都重新初始化临时标准图片包装
- `HelloPerformance` 的 SVG 逻辑 PFB hint 改为分 case 配置：
  - `IMAGE_SVG_PFB_TILED`: `128`
  - `IMAGE_SVG_PFB_TILED_RESIZE`: `96`

### 关键分析

- 第一轮的 `96` hint 是在“整屏遍历 tile，再依赖底层裁剪”的旧路径上得到的
- 当 draw loop 改成只遍历当前 work region 后，`IMAGE_SVG_PFB_TILED` 的最优 hint 重新偏向更宽的 band
- 但 `IMAGE_SVG_PFB_TILED_RESIZE` 在 `128` 下会从 `0.431 ms` 退化到 `0.464 ms`，因此不能共用同一个默认 hint
- 额外试探结果：
  - `IMAGE_SVG_PFB_TILED` + `112`：`0.491 ms`，无收益
  - `IMAGE_SVG_PFB_TILED` + `128`：`0.457 ms`，显著优于 `96`
  - `IMAGE_SVG_PFB_TILED_RESIZE` + `128`：`0.464 ms`，劣于 `96`

### 第二轮 QEMU 结果

单 case：

- `IMAGE_SVG_PFB_TILED`: `0.457 ms`，相对第一轮 `0.657 ms` 再降约 `30.4%`
- `IMAGE_SVG_PFB_TILED_RESIZE`: `0.431 ms`，相对第一轮 `0.533 ms` 再降约 `19.1%`

PFB Matrix（`IMAGE_SVG_PFB_TILED`，默认 hint=`128`）：

- `small (15x15)`: `0.692 ms`，相对第一轮 `1.288 ms` 降约 `46.3%`
- `middle (30x30)`: `0.478 ms`，相对第一轮 `0.601 ms` 降约 `20.5%`
- `fullwidth (240x1)`: `0.707 ms`，相对第一轮 `1.093 ms` 降约 `35.3%`
- `fullheight (1x240)`: `0.660 ms`，相对第一轮 `1.068 ms` 降约 `38.2%`
- `fullscreen (240x240)`: `0.344 ms`，与第一轮 `0.344 ms` 基本持平

### 第二轮验证

- `python scripts/perf_analysis/main.py --profile cortex-m3 --clean --timeout 300 --filter IMAGE_SVG_PFB_TILED --extra-cflags="-DEGUI_TEST_CONFIG_SINGLE_TEST=EGUI_VIEW_TEST_PERFORMANCE_TYPE_IMAGE_SVG_PFB_TILED"`：通过
- `python scripts/perf_analysis/main.py --profile cortex-m3 --clean --timeout 300 --filter IMAGE_SVG_PFB_TILED_RESIZE --extra-cflags="-DEGUI_TEST_CONFIG_SINGLE_TEST=EGUI_VIEW_TEST_PERFORMANCE_TYPE_IMAGE_SVG_PFB_TILED_RESIZE"`：通过
- `python scripts/perf_analysis/main.py --pfb-matrix --clean --profile cortex-m3 --timeout 300 --extra-cflags="-DEGUI_TEST_CONFIG_SINGLE_TEST=EGUI_VIEW_TEST_PERFORMANCE_TYPE_IMAGE_SVG_PFB_TILED"`：通过
- `python scripts/code_runtime_check.py --app HelloPerformance --timeout 10 --keep-screenshots`：`ALL PASSED`
- `output/main.exe`（`HelloUnitTest`）：`461` total / `458` passed / `3` failed，失败集合与第一轮一致，未新增失败

截图确认：

- `runtime_check_output/HelloPerformance/default/frame_0256.png`：`IMAGE_SVG_PFB_TILED`
- `runtime_check_output/HelloPerformance/default/frame_0257.png`：`IMAGE_SVG_PFB_TILED_RESIZE`

两张截图均正常平铺，无黑屏、无空白、无明显裁剪异常。

## 第三轮 PFB 优化（2026-04-17）

### 本轮改动

- 在 `egui_image_std.h` 中补充可复用的 RGB565 / RGB565+A8 行级 fast helper：
  - `egui_image_std_copy_rgb565_row_fast()`
  - `egui_image_std_blend_rgb565_row_fast()`
  - `egui_image_std_blend_rgb565_alpha8_row_fast()`
- `egui_image_svg` 新增缓存栅格直写 fast path：
  - 先按 `base_view_work_region` 与当前 PFB 可见区域裁剪实际绘制矩形
  - 无 mask 时直接对当前 PFB 做行级 `alpha8` blend
  - 有 mask 时优先尝试 `egui_image_std_blend_rgb565_alpha8_masked_row_block()`
  - 仅在 fast path 不适用时回退到 `egui_image_std` 的通用 draw 分发

### 关键分析

- 第二轮仍然保留了 `svg -> std image api -> std draw_image` 这条完整分发链
- 在 `HelloPerformance` 的 tiled SVG 场景里，缓存命中后真正需要做的工作其实只是“把一块内部 RGB565+A8 栅格裁剪后混合进当前 PFB”
- 将这一步改成直接面向当前可见矩形的 row blend 后，可以继续压缩每个 tile 的调度开销
- 第二轮之后重新验证了 `IMAGE_SVG_PFB_TILED` 的 hint：
  - `hint=128`：`0.432 ms`
  - `hint=96`：`0.461 ms`
- 因此保持 `IMAGE_SVG_PFB_TILED=128 / IMAGE_SVG_PFB_TILED_RESIZE=96` 仍然是更优的 shipped 默认值

### 第三轮 QEMU 结果

单 case：

- `IMAGE_SVG_PFB_TILED`: `0.432 ms`
  - 相对第二轮 `0.457 ms` 再降约 `5.5%`
  - 相对第一轮 `0.657 ms` 累计下降约 `34.2%`
- `IMAGE_SVG_PFB_TILED_RESIZE`: `0.415 ms`
  - 相对第二轮 `0.431 ms` 再降约 `3.7%`
  - 相对第一轮 `0.533 ms` 累计下降约 `22.1%`

PFB Matrix（`IMAGE_SVG_PFB_TILED`，默认 hint=`128`）：

- `small (15x15)`: `0.673 ms`
  - 相对第二轮 `0.692 ms` 再降约 `2.7%`
  - 相对第一轮 `1.288 ms` 累计下降约 `47.7%`
- `middle (30x30)`: `0.448 ms`
  - 相对第二轮 `0.478 ms` 再降约 `6.3%`
  - 相对第一轮 `0.601 ms` 累计下降约 `25.5%`
- `fullwidth (240x1)`: `0.690 ms`
  - 相对第二轮 `0.707 ms` 再降约 `2.4%`
  - 相对第一轮 `1.093 ms` 累计下降约 `36.9%`
- `fullheight (1x240)`: `0.638 ms`
  - 相对第二轮 `0.660 ms` 再降约 `3.3%`
  - 相对第一轮 `1.068 ms` 累计下降约 `40.3%`
- `fullscreen (240x240)`: `0.325 ms`
  - 相对第二轮 `0.344 ms` 再降约 `5.5%`
  - 相对第一轮 `0.344 ms` 累计下降约 `5.5%`

### 第三轮验证

- `python scripts/perf_analysis/main.py --profile cortex-m3 --clean --timeout 300 --filter IMAGE_SVG_PFB_TILED --extra-cflags="-DEGUI_TEST_CONFIG_SINGLE_TEST=EGUI_VIEW_TEST_PERFORMANCE_TYPE_IMAGE_SVG_PFB_TILED"`：通过
- `python scripts/perf_analysis/main.py --profile cortex-m3 --clean --timeout 300 --filter IMAGE_SVG_PFB_TILED_RESIZE --extra-cflags="-DEGUI_TEST_CONFIG_SINGLE_TEST=EGUI_VIEW_TEST_PERFORMANCE_TYPE_IMAGE_SVG_PFB_TILED_RESIZE"`：通过
- `python scripts/perf_analysis/main.py --pfb-matrix --clean --profile cortex-m3 --timeout 300 --extra-cflags="-DEGUI_TEST_CONFIG_SINGLE_TEST=EGUI_VIEW_TEST_PERFORMANCE_TYPE_IMAGE_SVG_PFB_TILED"`：通过
- `make clean APP=HelloPerformance PORT=pc && make -j APP=HelloPerformance PORT=pc`：通过
- `make clean APP=HelloUnitTest PORT=pc_test && make -j APP=HelloUnitTest PORT=pc_test && output/main.exe`：`ALL PASSED`
- `python scripts/code_runtime_check.py --app HelloPerformance --timeout 10 --keep-screenshots`：`ALL PASSED`

截图确认：

- `runtime_check_output/HelloPerformance/default/frame_0256.png`：`IMAGE_SVG_PFB_TILED`
- `runtime_check_output/HelloPerformance/default/frame_0257.png`：`IMAGE_SVG_PFB_TILED_RESIZE`

两张截图仍然正常平铺，无裁剪缝隙、无黑屏、无空白。

### 残留状态更新

- 第二轮记录中提到的 3 条 `image_svg` opacity 相关单元测试失败，在第三轮 fast path 合入后已消失
- 当前 `HelloUnitTest` 为 `ALL PASSED`，不再存在 SVG 透明度相关的已知失败集合

## 第四轮 PFB 优化（2026-04-17）

### 本轮改动

- `egui_image_svg` 在 raster cache 构建阶段预计算每行 metadata：
  - `visible_start`
  - `visible_end`
  - `all_opaque`
- cached fast path 改为只在“行 metadata 真正有收益”时才启用：
  - 全透明行直接跳过
  - 有透明边缘的行只绘制有效像素段
  - 整行全不透明时直接走 `copy_rgb565_row_fast` / `blend_rgb565_row_fast`
- 增加 doc 级开关；如果整张缓存图不存在可利用的行 metadata，则直接回退到第三轮的通用 row fast path，避免 `resize` 场景被额外分支拖慢

### 关键分析

- 第三轮已经去掉了 `svg -> std image -> draw_image` 的分发成本，但对带透明边缘的 SVG 行，仍要在每次 draw 时重复做 alpha 扫描
- 这些行特征在 cache 命中后是静态的，前移到 cache build 阶段一次性预计算更合算
- QEMU 结果显示，这笔优化对 `IMAGE_SVG_PFB_TILED` 和各类 PFB strip/fullscreen 都有稳定收益，说明热点确实在 tile 内部的行级透明处理
- `IMAGE_SVG_PFB_TILED_RESIZE` 从 `0.415 ms` 变为 `0.417 ms`，只差 `+0.002 ms`（约 `+0.5%`），基本持平
  - 推测原因：resize 后的缓存行更常见“整行都有有效像素且包含混合 alpha”，metadata 的可利用空间小于纯 tiled 场景

### 第四轮 QEMU 结果

单 case：

- `IMAGE_SVG_PFB_TILED`: `0.387 ms`
  - 相对第三轮 `0.432 ms` 再降约 `10.4%`
  - 相对第二轮 `0.457 ms` 再降约 `15.3%`
  - 相对第一轮 `0.657 ms` 累计下降约 `41.1%`
- `IMAGE_SVG_PFB_TILED_RESIZE`: `0.417 ms`
  - 相对第三轮 `0.415 ms` 基本持平（`+0.5%`）
  - 相对第二轮 `0.431 ms` 仍有约 `3.2%` 收益
  - 相对第一轮 `0.533 ms` 累计下降约 `21.8%`

PFB Matrix（`IMAGE_SVG_PFB_TILED`，hint=`128`）：

- `small (15x15)`: `0.633 ms`
  - 相对第三轮 `0.673 ms` 再降约 `5.9%`
- `middle (30x30)`: `0.399 ms`
  - 相对第三轮 `0.448 ms` 再降约 `10.9%`
- `fullwidth (240x1)`: `0.654 ms`
  - 相对第三轮 `0.690 ms` 再降约 `5.2%`
- `fullheight (1x240)`: `0.595 ms`
  - 相对第三轮 `0.638 ms` 再降约 `6.7%`
- `fullscreen (240x240)`: `0.280 ms`
  - 相对第三轮 `0.325 ms` 再降约 `13.8%`

### 第四轮验证

- `python scripts/perf_analysis/main.py --profile cortex-m3 --clean --timeout 300 --filter IMAGE_SVG_PFB_TILED --extra-cflags="-DEGUI_TEST_CONFIG_SINGLE_TEST=EGUI_VIEW_TEST_PERFORMANCE_TYPE_IMAGE_SVG_PFB_TILED"`：通过
- `python scripts/perf_analysis/main.py --profile cortex-m3 --clean --timeout 300 --filter IMAGE_SVG_PFB_TILED_RESIZE --extra-cflags="-DEGUI_TEST_CONFIG_SINGLE_TEST=EGUI_VIEW_TEST_PERFORMANCE_TYPE_IMAGE_SVG_PFB_TILED_RESIZE"`：通过
- `python scripts/perf_analysis/main.py --pfb-matrix --clean --profile cortex-m3 --timeout 300 --extra-cflags="-DEGUI_TEST_CONFIG_SINGLE_TEST=EGUI_VIEW_TEST_PERFORMANCE_TYPE_IMAGE_SVG_PFB_TILED"`：通过
- `make clean APP=HelloPerformance PORT=pc && make -j APP=HelloPerformance PORT=pc`：通过
- `make clean APP=HelloUnitTest PORT=pc_test && make -j APP=HelloUnitTest PORT=pc_test && output/main.exe`：`ALL PASSED`
- `python scripts/code_runtime_check.py --app HelloPerformance --timeout 10 --keep-screenshots`：`ALL PASSED`

截图输出：

- `runtime_check_output/HelloPerformance/default/frame_0256.png`：`IMAGE_SVG_PFB_TILED`
- `runtime_check_output/HelloPerformance/default/frame_0257.png`：`IMAGE_SVG_PFB_TILED_RESIZE`

两张 SVG 截图已正常生成，与前三轮 runtime 表现一致，无黑屏、无空白。

## 第五轮 PFB 优化（2026-04-17）

### 本轮改动

- `egui_image_svg` 的 raster row metadata 继续扩展，新增：
  - `opaque_start`
  - `opaque_end`
- 在 cache build 阶段为每一行预计算“最长连续全不透明 span”
  - 只在连续 span 长度达到阈值时启用该 fast path，避免 metadata 本身拖慢无收益场景
- cached fast path 对部分透明行改成“三段式”绘制：
  - 左侧抗锯齿边缘继续走 `egui_image_std_blend_rgb565_alpha8_row_fast()`
  - 中间全不透明区间直接走 `copy_rgb565_row_fast()` / `blend_rgb565_row_fast()`
  - 右侧抗锯齿边缘继续走 `egui_image_std_blend_rgb565_alpha8_row_fast()`
- 如果当前可见段完全落在 `opaque span` 内，则整段直接走不透明 row fast path，不再进入整段 alpha8 blend

### 关键分析

- 第四轮的 row metadata 已经消除了“全透明行”和“透明边缘裁剪”的无效 work，但对“中间大片不透明、两侧少量 AA 边缘”的行，仍然会把整段送进 alpha8 blend
- `HelloPerformance` 里的 badge SVG 正好非常符合这种分布：主体区域大面积实心，只有轮廓和圆角附近需要 alpha 混合
- 把最长 opaque span 前移到 cache build 阶段后，cache hit 的每次 draw 只需要处理两侧窄边缘，显著减少了 `alpha8_row_fast()` 的重复扫描与混合成本
- 这一轮不仅改善 `IMAGE_SVG_PFB_TILED`，也把第四轮基本持平的 `IMAGE_SVG_PFB_TILED_RESIZE` 明显拉下来，说明 resize 后缓存行同样存在可利用的长不透明区间

### 第五轮 QEMU 结果

单 case：

- `IMAGE_SVG_PFB_TILED`: `0.370 ms`
  - 相对第四轮 `0.387 ms` 再降约 `4.4%`
  - 相对第一轮 `0.657 ms` 累计下降约 `43.7%`
- `IMAGE_SVG_PFB_TILED_RESIZE`: `0.365 ms`
  - 相对第四轮 `0.417 ms` 再降约 `12.5%`
  - 相对第一轮 `0.533 ms` 累计下降约 `31.5%`

PFB Matrix（`IMAGE_SVG_PFB_TILED`，hint=`128`）：

- `small (15x15)`: `0.615 ms`
  - 相对第四轮 `0.633 ms` 再降约 `2.8%`
- `middle (30x30)`: `0.383 ms`
  - 相对第四轮 `0.399 ms` 再降约 `4.0%`
- `fullwidth (240x1)`: `0.638 ms`
  - 相对第四轮 `0.654 ms` 再降约 `2.4%`
- `fullheight (1x240)`: `0.579 ms`
  - 相对第四轮 `0.595 ms` 再降约 `2.7%`
- `fullscreen (240x240)`: `0.263 ms`
  - 相对第四轮 `0.280 ms` 再降约 `6.1%`

对应报告：

- `perf_output/perf_report.md`
- `perf_output/pfb_matrix_report.md`

### 第五轮验证

- `python scripts/perf_analysis/main.py --profile cortex-m3 --clean --timeout 300 --filter IMAGE_SVG_PFB_TILED --extra-cflags="-DEGUI_TEST_CONFIG_SINGLE_TEST=EGUI_VIEW_TEST_PERFORMANCE_TYPE_IMAGE_SVG_PFB_TILED"`：通过
- `python scripts/perf_analysis/main.py --profile cortex-m3 --clean --timeout 300 --filter IMAGE_SVG_PFB_TILED_RESIZE --extra-cflags="-DEGUI_TEST_CONFIG_SINGLE_TEST=EGUI_VIEW_TEST_PERFORMANCE_TYPE_IMAGE_SVG_PFB_TILED_RESIZE"`：通过
- `python scripts/perf_analysis/main.py --pfb-matrix --clean --profile cortex-m3 --timeout 300 --extra-cflags="-DEGUI_TEST_CONFIG_SINGLE_TEST=EGUI_VIEW_TEST_PERFORMANCE_TYPE_IMAGE_SVG_PFB_TILED"`：通过
- `make clean APP=HelloPerformance PORT=pc`：通过
- `make -j APP=HelloPerformance PORT=pc`：通过
- `make clean APP=HelloUnitTest PORT=pc_test`：通过
- `make -j APP=HelloUnitTest PORT=pc_test`：通过
- `output/main.exe`（`HelloUnitTest`）：`461` total / `461` passed / `0` failed，`ALL PASSED`
- `python scripts/code_runtime_check.py --app HelloPerformance --timeout 10 --keep-screenshots`：`ALL PASSED`

截图确认：

- `runtime_check_output/HelloPerformance/default/frame_0256.png`：`IMAGE_SVG_PFB_TILED`
- `runtime_check_output/HelloPerformance/default/frame_0257.png`：`IMAGE_SVG_PFB_TILED_RESIZE`

两张截图已再次人工确认，平铺与缩放场景都正常，无黑屏、无空白、无明显裁剪异常。

## 第六轮 PFB 优化（2026-04-17）

### 本轮改动

- 在 `egui_image_std.h` 中新增 `egui_image_std_blend_rgb565_alpha8_row_full_alpha_fast()`
  - 专门处理 `canvas alpha == 100%` 的 RGB565 + A8 row blend
  - 直接复用原先 helper 中的 full-alpha 分支逻辑，避免每次调用都再判断 `canvas_alpha`
- `egui_svg_cached_raster_draw_fast()` 按 `canvas->alpha` 拆成两条循环：
  - `alpha == 100%` 时走专用 full-alpha row fast path
  - 其他透明度继续走原有 blend row fast path
- 在两条循环里都保留第五轮的 `opaque span` 三段式绘制，但去掉行内重复的 `copy/blend` 分支判断
- 将 `EGUI_SVG_ROW_META_MIN_OPAQUE_SPAN` 改成可覆写宏，便于后续 QEMU 调参实验

### 关键分析

- 第五轮之后，热点已经进一步收敛到“cached fast path 每段 row blend 的分支调度成本”
- `HelloPerformance` 的两个 SVG PFB case 都是标准不透明绘制，`canvas alpha` 长时间固定为 `100%`
- 之前即使进入了 opaque span fast path，边缘段和 fallback 段仍然会反复调用带 `canvas_alpha` 判定的通用 helper
- 把 full-alpha 情况抽成专用 helper 后：
  - 边缘 alpha8 blend 少一次外层透明度分支
  - 行级 opaque 段不再在循环里重复判断 copy / blend
  - `tiled` 与 `resize` 都继续下降，说明这一轮确实打中了当前剩余热点
- 额外做了 `opaque span` 阈值试探：
  - `EGUI_SVG_ROW_META_MIN_OPAQUE_SPAN=4`
  - `EGUI_SVG_ROW_META_MIN_OPAQUE_SPAN=12`
  - 两组结果都与当前默认值 `8` 持平，因此保留 `8` 作为 shipped 默认值

### 第六轮 QEMU 结果

单 case：

- `IMAGE_SVG_PFB_TILED`: `0.360 ms`
  - 相对第五轮 `0.370 ms` 再降约 `2.7%`
  - 相对第一轮 `0.657 ms` 累计下降约 `45.2%`
- `IMAGE_SVG_PFB_TILED_RESIZE`: `0.351 ms`
  - 相对第五轮 `0.365 ms` 再降约 `3.8%`
  - 相对第一轮 `0.533 ms` 累计下降约 `34.1%`

PFB Matrix（`IMAGE_SVG_PFB_TILED`，hint=`128`）：

- `small (15x15)`: `0.608 ms`
  - 相对第五轮 `0.615 ms` 再降约 `1.1%`
- `middle (30x30)`: `0.370 ms`
  - 相对第五轮 `0.383 ms` 再降约 `3.4%`
- `fullwidth (240x1)`: `0.632 ms`
  - 相对第五轮 `0.638 ms` 再降约 `0.9%`
- `fullheight (1x240)`: `0.569 ms`
  - 相对第五轮 `0.579 ms` 再降约 `1.7%`
- `fullscreen (240x240)`: `0.253 ms`
  - 相对第五轮 `0.263 ms` 再降约 `3.8%`

对应报告：

- `perf_output/perf_report.md`
- `perf_output/pfb_matrix_report.md`

### 第六轮验证

- `python scripts/perf_analysis/main.py --profile cortex-m3 --clean --timeout 300 --filter IMAGE_SVG_PFB_TILED --extra-cflags="-DEGUI_TEST_CONFIG_SINGLE_TEST=EGUI_VIEW_TEST_PERFORMANCE_TYPE_IMAGE_SVG_PFB_TILED"`：通过
- `python scripts/perf_analysis/main.py --profile cortex-m3 --clean --timeout 300 --filter IMAGE_SVG_PFB_TILED_RESIZE --extra-cflags="-DEGUI_TEST_CONFIG_SINGLE_TEST=EGUI_VIEW_TEST_PERFORMANCE_TYPE_IMAGE_SVG_PFB_TILED_RESIZE"`：通过
- `python scripts/perf_analysis/main.py --pfb-matrix --clean --profile cortex-m3 --timeout 300 --extra-cflags="-DEGUI_TEST_CONFIG_SINGLE_TEST=EGUI_VIEW_TEST_PERFORMANCE_TYPE_IMAGE_SVG_PFB_TILED"`：通过
- `python scripts/perf_analysis/main.py --profile cortex-m3 --clean --timeout 300 --filter IMAGE_SVG_PFB_TILED --extra-cflags="-DEGUI_SVG_ROW_META_MIN_OPAQUE_SPAN=4 -DEGUI_TEST_CONFIG_SINGLE_TEST=EGUI_VIEW_TEST_PERFORMANCE_TYPE_IMAGE_SVG_PFB_TILED"`：通过，结果持平
- `python scripts/perf_analysis/main.py --profile cortex-m3 --clean --timeout 300 --filter IMAGE_SVG_PFB_TILED_RESIZE --extra-cflags="-DEGUI_SVG_ROW_META_MIN_OPAQUE_SPAN=4 -DEGUI_TEST_CONFIG_SINGLE_TEST=EGUI_VIEW_TEST_PERFORMANCE_TYPE_IMAGE_SVG_PFB_TILED_RESIZE"`：通过，结果持平
- `python scripts/perf_analysis/main.py --profile cortex-m3 --clean --timeout 300 --filter IMAGE_SVG_PFB_TILED --extra-cflags="-DEGUI_SVG_ROW_META_MIN_OPAQUE_SPAN=12 -DEGUI_TEST_CONFIG_SINGLE_TEST=EGUI_VIEW_TEST_PERFORMANCE_TYPE_IMAGE_SVG_PFB_TILED"`：通过，结果持平
- `python scripts/perf_analysis/main.py --profile cortex-m3 --clean --timeout 300 --filter IMAGE_SVG_PFB_TILED_RESIZE --extra-cflags="-DEGUI_SVG_ROW_META_MIN_OPAQUE_SPAN=12 -DEGUI_TEST_CONFIG_SINGLE_TEST=EGUI_VIEW_TEST_PERFORMANCE_TYPE_IMAGE_SVG_PFB_TILED_RESIZE"`：通过，结果持平
- `make clean APP=HelloUnitTest PORT=pc_test`：通过
- `make -j APP=HelloUnitTest PORT=pc_test`：通过
- `output/main.exe`（`HelloUnitTest`）：`461` total / `461` passed / `0` failed，`ALL PASSED`
- `make -j APP=HelloPerformance PORT=pc`：通过
- `python scripts/code_runtime_check.py --app HelloPerformance --timeout 10 --keep-screenshots`：`ALL PASSED`

截图确认：

- `runtime_check_output/HelloPerformance/default/frame_0256.png`：`IMAGE_SVG_PFB_TILED`
- `runtime_check_output/HelloPerformance/default/frame_0257.png`：`IMAGE_SVG_PFB_TILED_RESIZE`

两张截图已再次确认，平铺与缩放场景均正常，无黑屏、无空白、无明显裁剪错误。
