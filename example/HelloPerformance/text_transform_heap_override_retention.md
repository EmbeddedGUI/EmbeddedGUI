# HelloPerformance Text Transform Heap Override 记录

## 范围

- 本文只记录：
  - `EGUI_CONFIG_TEXT_TRANSFORM_VISIBLE_ALPHA8_MAX_BYTES`
- 这个宏控制的是 buffered rotated-text 的瞬时 heap ceiling，不是 fixed static RAM 开关。
- `2026-03-31` 起，这个宏已移动到 `example/HelloPerformance/app_egui_config.h` 尾部的 `#if 0` 可选 low-RAM 块，默认关闭。

## 证据来源

- `example/HelloPerformance/ram_tracking.md`
- 历史 QEMU 检查命令：
  - `python scripts/code_perf_check.py --profile cortex-m3 --threshold 5`
  - `python scripts/code_runtime_check.py --app HelloPerformance --keep-screenshots`

## 当前结论

### 1. 它不是本轮 `<100B static RAM` 小宏

- 多轮 A/B 里，这个宏变化时 `text/static RAM` 一直保持不变。
- 它改变的是 visible alpha8 tile cache 的 heap 上界，以及何时掉入 packed4 fallback。
- 所以这项判断的核心不是“能省多少 static RAM”，而是“heap cliff 和热点 perf 是否还能接受”。

### 2. `2560` 继续保留为 low-RAM 取值

- `2026-03-28`：
  - `5120` 相比 `4096`，`TEXT_ROTATE_BUFFERED 12.325 -> 9.159 (-25.7%)`
  - 但这说明更高 ceiling 只是换更多 heap 余量，不属于本轮 small static-RAM 清理
- 同日继续 A/B：
  - `5120 -> 4160` 后，历史结论是 `4096` 当时会超过 `5%` perf 门槛，而 `4160` 可以接受
  - 再到 `3648`、`3072`，都仍属于 “heap ceiling 继续下探，但 perf 仍可接受” 的阶段
- `2026-03-30` 在当时的 low-RAM shipped 基线上重新打开这项：
  - `3072 -> 2560`
  - `TEXT_ROTATE_BUFFERED 3846 -> 3334 (-512B)`
  - `EXTERN_TEXT_ROTATE_BUFFERED 4056 -> 3544 (-512B)`
  - 全量 perf 仍满足更严格的 `<=500B => 5%` 规则：
    - `TEXT_ROTATE_BUFFERED 11.358 -> 11.552 (+1.71%)`
    - `EXTERN_TEXT_ROTATE_BUFFERED 12.884 -> 13.072 (+1.46%)`
  - runtime 仍是 `223/223`
- `2026-03-31` 起默认构建关闭这条 low-RAM override，直接回到框架默认 `4096`。
  - 当前 clean perf 仍是 `222` 个 case
  - `TEXT_ROTATE_BUFFERED 6.411ms`
  - `EXTERN_TEXT_ROTATE_BUFFERED 6.792ms`
  - runtime check 仍是 `223/223`

### 3. `2304` 及以下仍然必须拒绝

- 同一轮 heap-cliff audit 已证明：
  - `2304B` 时 `TEXT_ROTATE_BUFFERED` 直接跳到 `5410B`
  - `2048B / 1792B / 1536B / 256B / 0B` 都停在 `5252B`
- 原因不是简单的 timing 抖动，而是路径切回 packed4 fallback 后，瞬时 scratch 反而变大。
- 所以当前限制已经不是 perf，而是 heap shape cliff。

## 处理结论

- `2560` 继续保留在 `app_egui_config.h` 尾部的可选 low-RAM 块中
- 默认构建关闭这条 override，直接继承框架默认 `4096`
- 保留外部可选入口，继续作为后续 rotated-text heap/perf A/B 的测量入口
- 不把它纳入本轮 “默认打开后移除宏管理” 的 `<100B static RAM` 清理范围
