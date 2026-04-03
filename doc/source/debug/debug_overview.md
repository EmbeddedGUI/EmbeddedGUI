# 调试总览

EmbeddedGUI 的调试建议分成三层来用：

- 画面可视化：直接把 PFB、脏区、触摸轨迹和帧率信息画到屏幕上，先确认“画了什么”。

- 日志和统计：输出脏区来源、合并过程和每帧统计，回答“为什么会这样画”。

- 自动化脚本：把编译、录制、截图和差异检查串起来，避免修完以后回归。

## 推荐顺序

1. 先在 `PORT=pc` 上复现问题。

2. 先开画面类宏，看渲染范围、触摸路径和实时指标是否符合预期。

3. 再开日志类宏，定位是谁触发了脏区、脏区是怎么被合并的。

4. 最后接脚本回归，把问题固定成可重复检查的流程。

## 宏怎么开启

日常有两种方式：

- 临时调试：直接走 `USER_CFLAGS`，适合一次性排查。

- 应用常驻：写进对应示例或应用的 `app_egui_config.h`。

临时调试示例：

```bash
make all APP=HelloBasic APP_SUB=slider PORT=pc \
  USER_CFLAGS="-DEGUI_CONFIG_DEBUG_DIRTY_REGION_REFRESH=1 -DEGUI_CONFIG_DEBUG_REFRESH_DELAY=40"
```

应用内常驻示例：

```c
#define EGUI_CONFIG_DEBUG_DIRTY_REGION_REFRESH 1
#define EGUI_CONFIG_DEBUG_REFRESH_DELAY 40
```

## 常见症状对应工具

| 现象 | 先开什么 | 再看什么 |
| --- | --- | --- |
| 只刷新了部分区域、怀疑 tile 错位 | `EGUI_CONFIG_DEBUG_PFB_REFRESH` | `EGUI_CONFIG_DEBUG_DIRTY_REGION_REFRESH` |
| 脏区太大、不知道为什么整块刷新 | `EGUI_CONFIG_DEBUG_DIRTY_REGION_REFRESH` | `EGUI_CONFIG_DEBUG_DIRTY_REGION_STATS`、`EGUI_CONFIG_DEBUG_DIRTY_REGION_TRACE` |
| 点击、拖动位置不对 | `EGUI_CONFIG_DEBUG_TOUCH_TRACE` | `scripts/code_runtime_check.py` 或手工录制 |
| 想确认当前 FPS、CPU 占用和 LCD latency | `EGUI_CONFIG_DEBUG_PERF_MONITOR_SHOW` | 如需一起看 EGUI 自身 SRAM current / peak，再叠加 `EGUI_CONFIG_DEBUG_MEM_MONITOR_SHOW` |
| 想知道是哪个控件触发了日志 | `EGUI_CONFIG_DEBUG_CLASS_NAME` | `EGUI_CONFIG_DEBUG_VIEW_ID`、`EGUI_CONFIG_DEBUG_LOG_LEVEL=EGUI_LOG_IMPL_LEVEL_DBG` |
| 想批量检查示例渲染和交互 | `scripts/code_runtime_check.py` | `scripts/checks/hello_basic_render_workflow.py` |
| 想确认动画是否仍然是局部刷新 | `scripts/checks/code_dirty_animation_check.py` | `scripts/perf_analysis/dirty_region_stats_report.py` |

## 这组文档覆盖什么

- {doc}`debug_macros`：每个调试宏的作用、推荐组合和运行效果。

- {doc}`debug_pc_workflow`：PC 端最常用的抓图、录制、回归流程。

- {doc}`debug_scripts`：调试相关脚本的定位、命令和输出结果。

如果你现在关心的是 PFB 和脏区原理本身，可以同时参考 {doc}`../architecture/rendering_pipeline` 和 {doc}`../performance/dirty_region_tuning`。
