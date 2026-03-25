# 2026-03-25 HelloPerformance Decode Buffer RAM Plan

## 背景

- 用户关注 canvas / image 相关全局或静态变量持续增加，导致所需 RAM 上升。
- 前一轮已清掉一批低收益缓存和死字段，并提交 `a9a64f4 perf: reduce image mask cache RAM`。
- 本轮继续检查 HelloPerformance 中剩余的大块 RAM 占用，优先找“静态 RAM 大、性能收益明确或无损”的项。

## 分析结论

- `example/HelloPerformance/app_egui_config.h` 开启的压缩图 row-band cache 仍然值得保留，因为它直接服务 QOI / RLE 热路径。
- 但其像素 scratch / row cache buffer 之前固定按 `4 bytes/pixel` 分配，属于按 RGB32 最坏情况预留。
- HelloPerformance 的压缩图资源实际全部是 `rgb565`，不需要为 `RGB32` 预留这块静态 RAM。
- 因此本轮不关闭 row cache，而是把“最大像素字节数”从固定 4 调整为可配置，并仅对 HelloPerformance 收紧到 2。

## 实施项

1. 在 `src/core/egui_config_default.h` 新增 `EGUI_CONFIG_IMAGE_DECODE_MAX_PIXEL_SIZE`
   - 默认值保持 `4`
   - 其他应用默认行为不变

2. 在 `src/image/egui_image_decode_utils.c/.h` 使用该宏分配压缩图 decode row buffer 和 row-band cache
   - `egui_image_decode_row_pixel_buf`
   - `egui_image_decode_row_cache_pixel`

3. 在 `src/image/egui_image_decode_utils.h` 增加 buffer 访问检查
   - `egui_image_decode_get_row_pixel_buf()`
   - `egui_image_decode_cache_pixel_row()` 中校验 `bytes_per_pixel`

4. 在 `example/HelloPerformance/app_egui_config.h` 覆盖
   - `EGUI_CONFIG_IMAGE_DECODE_MAX_PIXEL_SIZE 2`
   - 用 `#ifndef` 包裹，允许性能脚本用 `USER_CFLAGS` 做 A/B 覆盖

## 预期收益

- HelloPerformance 静态 RAM 减少约 `14,880B`
  - row-band pixel cache: `30 * 240 * (4 - 2) = 14,400B`
  - shared row pixel scratch: `240 * (4 - 2) = 480B`
- 算法路径不变，理论性能应保持一致。

## 验证

### 必跑回归

- `make clean APP=HelloUnitTest PORT=pc_test`
- `make all APP=HelloUnitTest PORT=pc_test`
- `output\main.exe`
  - 结果：`649/649 passed`

- `make clean APP=HelloPerformance PORT=pc`
- `make all APP=HelloPerformance PORT=pc`
- `python scripts/code_runtime_check.py --app HelloPerformance --timeout 120 --keep-screenshots`
  - 结果：`ALL PASSED`

### QEMU 性能

- `python scripts/code_perf_check.py --profile cortex-m3 --filter QOI,RLE`
  - 结果：`PASSED`

### A/B 对比

- 基线：`-DEGUI_CONFIG_IMAGE_DECODE_MAX_PIXEL_SIZE=4`
- 当前：`EGUI_CONFIG_IMAGE_DECODE_MAX_PIXEL_SIZE=2`
- 方法：通过 `scripts/code_perf_check.py` 的 `USER_CFLAGS` 机制做同 profile QEMU 对比
- 结论：所有 `QOI/RLE` 相关测试项 `delta_pct = 0.0%`

## 结论

- 本轮保留 row cache 机制，只移除其对 HelloPerformance 不必要的 RGB32 预留。
- 这是一次“静态 RAM 明显下降、QEMU 性能无回退”的低风险收敛。
- 后续若继续压 RAM，可再单独评估 `*_PERSISTENT_CACHE_MAX_BYTES` 两类运行期 heap 配置。
