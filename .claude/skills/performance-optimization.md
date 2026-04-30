---
name: performance-optimization
description: Use when optimizing performance for a specific EmbeddedGUI APP, page, interaction, widget state, dirty region behavior, PFB/SPI configuration, text/image rendering, or app-level performance regression; use doc/source/performance as reference material but build baselines around the target application scenario
---

# Performance Optimization Skill

用于**具体应用**的性能优化。默认目标不是 `HelloPerformance`，而是用户指定的 `APP` / `APP_SUB` / 页面 / 交互场景。`doc/source/performance` 和 `HelloPerformance` 只作为框架机制、工具命令和经验结论的参考。

## 先锁定应用场景

开始优化前必须明确：

- 目标：`APP`、可选 `APP_SUB`、页面名、控件或交互路径。
- 症状：卡顿、整屏刷新、动画掉帧、滚动慢、输入延迟、内存峰值高、真机 flush 慢等。
- 场景：idle、首次进入、页面切换、滚动、拖动、列表刷新、定时器更新、后端数据变化。
- 配置：屏幕尺寸、PFB 宽高、PFB buffer count、`PORT`、`USER_CFLAGS`、是否开启 debug overlay。
- 数据：PC mock 数据、录制动作、后端数据状态、资源版本。

如果用户只说“优化性能”，先从仓库中找到该 APP 的 `app_egui_config.h`、页面源码、recording 动作和资源配置，再决定测试场景。不要默认跑 `HelloPerformance` 作为应用基线。

## 参考资料

按问题类型读取 `doc/source/performance/`：

- 应用调优优先级：`perf_tuning.md`
- QEMU 测试框架背景：`perf_overview.md`
- 脏区接口：`dirty_region_interfaces.md`
- 细粒度脏区：`dirty_region_tuning.md`
- 结构容器透传：`dirty_passthrough_container.md`、`is_dirty_passthrough.md`
- Debug overlay：`debug_monitor.md`
- 低 RAM/缓存宏取舍：`low_ram_config_macros.md`
- 系统性回归案例：`nano_specs_memcpy_regression.md`
- 框架级参考报告：`perf_report.md`、`pfb_matrix_report.md`、`spi_matrix_report.md`

这些文档给出机制和经验，不替代目标 APP 的场景基线。

## 应用级基线

具体 APP 也要建基线，基线单位是“场景”，不是“整个框架”。

建议记录：

- `app`: `APP` / `APP_SUB`
- `scenario`: 例如 `player_idle`、`list_scroll`、`volume_drag`、`page_switch`
- `commit`: 当前 commit
- `config`: 屏幕尺寸、PFB、buffer count、关键宏、资源版本
- `recording`: 固定录制动作或手动复现步骤
- `mock_state`: PC 仿真的后端数据状态
- `outputs`: runtime 截图、dirty stats、可选 QEMU/真机日志

PC 基线适合：

- 截图和视觉正确性
- `DIRTY_REGION_STATS` 的 `dirty_area` / `pfb_tiles`
- 页面是否全屏 dirty、是否有残影/缺刷
- 固定 mock 数据下的交互路径稳定性

最终耗时基线根据场景选择：

- 可移植绘制热点：用 QEMU 或已有 perf harness。
- 真实 APP 页面：优先用 PC dirty stats + runtime 截图证明覆盖面，再用真机 perf/debug monitor/平台日志确认 render/flush。
- 涉及 SPI/DMA/外部资源/真实后端：必须用真机或目标端日志确认，PC/QEMU 只能作为辅助。

## 固定工作流

1. **建立场景基线**
   - 运行目标 APP：
     ```bash
     python scripts/code_runtime_check.py --app {APP} --keep-screenshots
     ```
   - 有子应用时：
     ```bash
     python scripts/code_runtime_check.py --app {APP} --app-sub {SUB} --keep-screenshots
     ```
   - 打开脏区统计构建，收集 `DIRTY_REGION_STATS`：
     ```bash
     make all APP={APP} APP_SUB={SUB} PORT=pc USER_CFLAGS="-DEGUI_CONFIG_FUNCTION_RECORDING_TEST=1 -DEGUI_CONFIG_DEBUG_DIRTY_REGION_STATS=1 -DEGUI_CONFIG_DEBUG_DIRTY_REGION_TRACE=1"
     ```
   - 记录截图目录、日志、场景步骤和配置。

2. **定位瓶颈**
   - 如果整屏或大面积蓝框频繁出现，优先查 dirty/invalidate/passthrough。
   - 如果 dirty area 很小但真机仍慢，优先查 SPI/flush、DMA 等待、外部资源读取。
   - 如果 CPU render 明显高，查文本测量、图片解码、渐变、阴影、mask、圆弧、cell 循环。
   - 如果多个无关应用或基础场景同时回退，再用 `HelloPerformance` / QEMU 报告判断框架级回归。
   - 如果 debug monitor 开启，先区分 overlay 自身刷新和业务刷新，不把调试开销当正式性能。

3. **选择优化手段**
   - **减少 dirty area**：只刷新变化控件，避免 root/page invalidate。
   - **局部 dirty**：使用 `egui_view_invalidate_region()`、`egui_view_invalidate_sub_region()`，旧状态和新状态都覆盖。
   - **绘制早裁剪**：在高成本 `on_draw()` 子区域前使用 `egui_canvas_is_region_active()`。
   - **结构容器透传**：纯布局容器开启 `EGUI_CONFIG_FUNCTION_SUPPORT_DIRTY_PASSTHROUGH` 和 `egui_view_set_dirty_passthrough()`。
   - **PFB**：按 APP 的 RAM 预算和内容类型选择；文本密集可评估更大或全宽条带，低 RAM 保守。
   - **双缓冲**：真机 SPI 屏优先双缓冲；三/四缓冲只有在数据证明有收益时再用。
   - **文本**：减少重复 set_text/格式化/测量；文本重排时回退整控件刷新。
   - **图片**：复杂静态图形优先切图/压缩资源；动态大图考虑外部资源、row cache、流式更新。
   - **绘图 API**：动画高频路径避免不必要 HQ；静态复杂效果可改为资源图。

4. **实现约束**
   - 改动必须围绕目标 APP 场景，不做无关框架重构。
   - 新 fast path 必须保留 fallback；不改变 public API，除非任务明确要求。
   - 局部 dirty 优化必须在布局、文本重排、尺寸变化、主题变化时回退到整控件 invalidate。
   - 新增或调整宏前先查 `low_ram_config_macros.md`，不要复活文档中已内收或不推荐的 public 宏。
   - 涉及内存搬运热点时使用 `egui_api_memcpy()` / `egui_api_memset()` 等平台抽象。

5. **验证闭环**
   - 重跑同一 APP 场景的 runtime_check，确认截图无残影、缺刷、错位。
   - 生成 dirty stats before/after，对比 `dirty_area`、`pfb_tiles`、best/worst partial frame。
   - 真机相关优化要提供 render/flush/FPS/队列/外设日志。
   - 框架公共代码改动或多个 APP 受影响时，额外跑：
     ```bash
     python scripts/perf_analysis/main.py --profile cortex-m3
     ```

## Dirty 优化细则

适用场景：

- 光标闪烁、按钮按压、选中态、badge、进度条局部变化。
- number picker、calendar、button matrix、slider、analog clock 等固定区域控件。
- virtual list/grid holder、scroll 内容层、page/activity root 等结构容器。

检查顺序：

1. 是否错误刷新了父容器、page root 或 root group。
2. 是否只有一个小区域变化，却调用了整控件 `egui_view_invalidate()`。
3. 是否旧状态区域没有失效，导致残影。
4. `on_draw()` 是否无条件遍历所有 cell/文本/图片。
5. `EGUI_CONFIG_DIRTY_AREA_COUNT` 是否不足导致小区域被合并成大区域。

常用工具：

- `egui_view_invalidate_region()`
- `egui_view_invalidate_sub_region()`
- `egui_canvas_is_region_active()`
- `egui_view_circle_dirty.h`
- `EGUI_CONFIG_DEBUG_DIRTY_REGION_REFRESH`
- `EGUI_CONFIG_DEBUG_DIRTY_REGION_STATS`
- `EGUI_CONFIG_DEBUG_DIRTY_REGION_TRACE`
- `scripts/perf_analysis/dirty_region_stats_report.py`

## Dirty Passthrough 判断

只对“自身不绘制像素”的结构容器开启：

- 适合：root group、page root、activity root、scroll 内容层、viewpage page、virtual holder、只负责布局的 linearlayout。
- 不适合：自身绘制背景、边框、阴影、mask、装饰线，或自身像素会随位置/尺寸变化的容器。
- 带 background 的稳定 scroll 内容移动可以例外，但必须用截图和 dirty stats 验证。

调试时看：

- `dirty_passthrough_swept`
- `dirty_passthrough_self`
- `layout`
- `subregion`
- `regions` / `dirty_area` / `pfb_tiles`

## 框架参考测试何时使用

不要把 `HelloPerformance` 当成具体 APP 的默认基线。只有这些情况才跑：

- 改到了 `src/` 公共绘制、字体、图片、canvas、dirty、PFB、porting/qemu 等框架代码。
- 多个 APP 都出现相同类别回退。
- 怀疑编译参数、libc、`memcpy`/`memset`、公共 fast path 导致系统性回归。
- 需要重新生成 `doc/source/performance` 性能图表。

命令：

```bash
python scripts/perf_analysis/main.py --profile cortex-m3
python scripts/perf_analysis/main.py --pfb-matrix
python scripts/perf_analysis/main.py --spi-matrix
python scripts/perf_analysis/main.py --full-check --threshold 10
```

更新文档图表时才运行：

```bash
python scripts/perf_analysis/main.py --full-check --doc
```

## 常见诊断

| 应用现象 | 优先判断 | 处理方向 |
| --- | --- | --- |
| 页面切换整屏蓝框 | page/root invalidate 或结构容器未透传 | 开启 passthrough，改为子控件 swept dirty |
| 列表滚动慢 | holder/layout dirty 过大或 cell 全量绘制 | virtual holder passthrough，cell 早裁剪 |
| 数值/时间更新卡顿 | 刷新父容器或文本重复测量 | 只刷新 label 区域，避免重复 set_text |
| 拖动 slider/volume 卡 | SET 高频和 dirty 过大 | 本地 coalesce，刷新 thumb/track 差异区 |
| PC 截图没问题但真机慢 | SPI/flush/外部资源 | 看真机 render/flush，减少 dirty 面积或调整双缓冲 |
| 打开 debug monitor 后多刷新 | overlay 自身标脏 | 正式基线关闭 monitor |
| 多个 APP 同时慢 | 框架级回归 | 再跑 HelloPerformance/QEMU 定位公共路径 |

## 交付要求

最终回复必须包含：

- 目标 APP/APP_SUB、页面/场景、mock 数据或复现步骤。
- 优化前后的配置、命令、截图目录和日志路径。
- 应用级指标变化：`dirty_area`、`pfb_tiles`、render/flush/FPS 或真机日志。
- 视觉正确性结果：runtime_check 截图、单测或人工检查结论。
- 是否额外跑了框架参考测试；没跑时说明为什么不需要。
