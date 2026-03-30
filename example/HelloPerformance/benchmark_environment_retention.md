# HelloPerformance Benchmark Environment 保留记录

## 范围

- 本文记录当前仍保留的 benchmark environment 宏：
  - `EGUI_CONFIG_SCEEN_HEIGHT`
  - `EGUI_CONFIG_MAX_FPS`
  - `EGUI_CONFIG_CIRCLE_SUPPORT_RADIUS_BASIC_RANGE`
  - `EGUI_CONFIG_DEBUG_LOG_LEVEL`
- `EGUI_CONFIG_SCEEN_WIDTH` 已从 app override 移除，直接继承库默认 `240`。
- 这组宏定义的是 HelloPerformance benchmark harness 本身，不是为了省几十字节 SRAM 临时加出来的 small static-RAM toggle。

## 依据来源

- `example/HelloPerformance/app_egui_config.h`
- `example/HelloPerformance/uicode.c`
- `example/HelloPerformance/egui_view_test_performance.c`
- `scripts/code_perf_check.py`
- `src/core/egui_core.c`
- `src/core/egui_api.h`
- `src/core/egui_canvas.c`
- `src/core/egui_config_default.h`
- `src/core/egui_config_debug_default.h`
- `src/resource/egui_res_circle_info.c`
- `perf_output/perf_report.md`

## 本次支撑报告

- 历史 clean baseline：`c11cbcd`
- 本轮复跑时间：`2026-03-31`
- 报告头部提交：`eaeea78`
- 命令：
  - `python scripts/code_perf_check.py --profile cortex-m3 --threshold 5 --clean`
- 结果：
  - 删除 `EGUI_CONFIG_SCEEN_WIDTH` app override 后，`read_screen_size()` 仍返回 `(240, 240)`
  - `perf_output/perf_report.md` 当前输出 `222` 个 clean perf case
  - 报告覆盖 `Basic Shapes / Text / Image / Compress / Gradient / Shadow / Mask / Animation`
  - `scripts/code_perf_check.py` 现在按维度读取 `EGUI_CONFIG_SCEEN_WIDTH / HEIGHT`，缺省时分别回退到库默认值
  - 这次复跑发生在脏工作树上，报告头部仍记录当前 `HEAD`；但实际 benchmark 输出与此前 `240x240` clean harness 一致
  - 报告生成依赖 `PERF_RESULT` / `PERF_SKIP` / `PERF_COMPLETE` 结构化日志

## 宏结论

### `EGUI_CONFIG_SCEEN_WIDTH`

- 当前库默认宽度本来就是 `240`，与 HelloPerformance 原 app override 相同。
- `uicode.c` 和 `egui_view_test_performance.c` 继续通过 `EGUI_CONFIG_SCEEN_WIDTH` 使用同一套 `240px` 几何；去掉 app 侧重复定义不会改变编译后的 benchmark harness。
- 之前真正阻止删除的是 `scripts/code_perf_check.py`：旧逻辑要求 `app_egui_config.h` 同时显式写出宽高，否则会整体回退成 `240x320`。本轮已改成宽高分别回默认值，因此删除宽度 override 后，perf harness 仍会被正确识别为 `240x240`。
- 处理结论：删除 app override，直接继承库默认 `240`。

### `EGUI_CONFIG_SCEEN_HEIGHT`

- `uicode.c` 直接用它设置 benchmark root view 高度。
- `egui_view_test_performance.c` 的全文字、整屏图片、mask、gradient、circle 等场景都按 `SCEEN_HEIGHT` 推导几何。
- 当前 app 仍需显式固定 `240`，用来把 clean perf 报告中的 `TEXT_ROTATE_BUFFERED 11.498ms`、`EXTERN_IMAGE_ROTATE_565_8 18.990ms`、`MASK_RECT_FILL_CIRCLE_DOUBLE 1.656ms` 等结果锚定到同一套 `240x240` square harness。把 `HEIGHT` 回退到默认 `320` 会直接改变 benchmark 本身，而不是只减少一项 small-RAM 宏。
- 处理结论：保留，作为 benchmark 几何锚点。

### `EGUI_CONFIG_MAX_FPS`

- `egui_core.c` 用 `1000 / EGUI_CONFIG_MAX_FPS` 定义刷新节奏，并对统计中的 `fps` 做上限钳制。
- HelloPerformance 的场景推进走 timer/recording pipeline；当前 clean perf 报告是在 `MAX_FPS=1` 的节奏下采集的。
- 这不是 static-RAM 微调项，而是 benchmark 运行时环境约束。把它回退到默认 `60`，改变的是基准节奏，不是“默认打开后几乎无代价”的功能开关。
- 处理结论：保留。

### `EGUI_CONFIG_CIRCLE_SUPPORT_RADIUS_BASIC_RANGE`

- 当前 `241` 不是随意保留的 RAM 开关，而是“维持现有 benchmark 圆形 coverage 的最小值”。
- `egui_canvas_get_circle_item()` 使用严格的 `r < EGUI_CONFIG_CIRCLE_SUPPORT_RADIUS_BASIC_RANGE` 检查，所以要覆盖 `radius=240` 的场景，range 至少要 `241`。
- `egui_res_circle_info.c` 里的 built-in circle info 也是按 `>= N` 条件编译；HelloPerformance 的 `MASK_RECT_FILL_CIRCLE_DOUBLE` 和 `MASK_IMAGE_CIRCLE_DOUBLE` 还会直接把 `EGUI_CONFIG_CIRCLE_SUPPORT_RADIUS_BASIC_RANGE` 送进 `set_perf_circle_mask(...)`。
- 回退到默认 `50` 会让当前大圆/大圆角 benchmark 场景失去现有 coverage；这不属于 `<100B static RAM` 清理，而是 benchmark workload 定义变化。
- 处理结论：保留。

### `EGUI_CONFIG_DEBUG_LOG_LEVEL`

- `scripts/code_perf_check.py` 只认 `PERF_RESULT:` / `PERF_SKIP:` / `PERF_COMPLETE` 结构化输出。
- `uicode.c` 通过 `EGUI_LOG_INF` 发出这些标记；`egui_api.h` 在 log level 小于 `INF` 时会把 `EGUI_LOG_INF` 编译为空宏。
- 默认 `EGUI_CONFIG_DEBUG_LOG_LEVEL` 是 `EGUI_LOG_IMPL_LEVEL_NONE`。如果直接回默认，clean perf 报告就失去采样输出入口。
- 这不是 small static-RAM switch，而是 benchmark report harness 的 I/O 契约。
- 处理结论：保留 `EGUI_LOG_IMPL_LEVEL_INF`。

## 最终结论

- `EGUI_CONFIG_SCEEN_WIDTH` 已回退到库默认 `240`
- `EGUI_CONFIG_SCEEN_HEIGHT`
- `EGUI_CONFIG_MAX_FPS`
- `EGUI_CONFIG_CIRCLE_SUPPORT_RADIUS_BASIC_RANGE`
- `EGUI_CONFIG_DEBUG_LOG_LEVEL`

本轮已确认 `EGUI_CONFIG_SCEEN_WIDTH` 可以直接继承库默认值，不再需要 app 侧宏管理。剩余 4 项仍不应按“`<100B static RAM` 且 `perf <5%` 就默认打开”的规则处理；它们定义的是 HelloPerformance benchmark 的几何、节奏、圆形 coverage 和报告输出环境，去掉这些显式定义，改变的是 benchmark harness，而不是仅仅减少宏维护成本。
