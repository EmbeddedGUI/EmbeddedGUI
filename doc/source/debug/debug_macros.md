# 调试宏

调试宏主要分布在两个配置头里：

- `src/core/egui_config_debug_default.h`

- `src/core/egui_config_default.h`

推荐优先在 `app_egui_config.h` 或 `USER_CFLAGS` 里覆盖，不要直接改默认头文件。

## 画面可视化类

| 宏 | 默认值 | 作用 | 典型用法 | 注意事项 |
| --- | --- | --- | --- | --- |
| `EGUI_CONFIG_DEBUG_PFB_REFRESH` | `0` | 在每个 PFB tile 外画红色边框 | 看 tile 扫描路径、PFB 大小是否合适 | 建议配合 `EGUI_CONFIG_DEBUG_REFRESH_DELAY`，否则闪得太快 |
| `EGUI_CONFIG_DEBUG_DIRTY_REGION_REFRESH` | `0` | 在每个 dirty region 外画蓝色边框 | 看脏区是否过大、是否漏刷 | 常和 `PFB_REFRESH` 一起开 |
| `EGUI_CONFIG_DEBUG_PFB_DIRTY_REGION_CLEAR` | `1` | 控制调试可视化时是否先清掉上一次的脏区结果 | `0` 适合只看当前一轮，`1` 适合看累计扫描轨迹 | 只在 `PFB_REFRESH` 或 `DIRTY_REGION_REFRESH` 开启时有意义 |
| `EGUI_CONFIG_DEBUG_REFRESH_DELAY` | `0` | 调试可视化时，每个 tile flush 之间额外延时 | 设成 `20~50ms` 更容易肉眼观察 | 会显著拉低刷新速度 |
| `EGUI_CONFIG_DEBUG_PERF_MONITOR_SHOW` | `0` | 显示 `FPS / CPU / ms(render \| flush)` | 快速看刷新节奏和绘制耗时 | 统计口径是窗口平均，不是单帧瞬时值 |
| `EGUI_CONFIG_DEBUG_MEM_MONITOR_SHOW` | `0` | 显示 EGUI 自身 `malloc` 的 `current / peak` | 快速看框架自身 SRAM 占用变化 | 只统计 `egui_api_malloc/free` 路径，不代表系统总 heap |
| `EGUI_CONFIG_DEBUG_MONITOR_FONT` | `&egui_res_font_montserrat_12_4` | 设置屏幕 monitor 共用字体 | 觉得默认字太大或太小时覆盖字体资源 | 需传入字体对象地址，详细行为见 {doc}`../performance/debug_monitor` |
| `EGUI_CONFIG_DEBUG_TOUCH_TRACE` | `0` | 自动记录并绘制最新一条触摸轨迹 | 排查点击、拖动、坐标映射问题 | 现在位于 `egui_core`，不是 PC 端单独叠加 |
| `EGUI_CONFIG_DEBUG_TOUCH_TRACE_MAX_POINTS` | `256` | 触摸轨迹最多保留的点数 | 长拖动或高采样设备可调大 | 点数越多，占用的静态内存越多 |

## 日志与追踪类

| 宏 | 默认值 | 作用 | 典型用法 | 注意事项 |
| --- | --- | --- | --- | --- |
| `EGUI_CONFIG_DEBUG_VIEW_ID` | `0` | 给每个 view 分配唯一 `id` | 配合 `CLASS_NAME` 做日志定位 | 会给 `egui_view_t` 增加一个 `uint16_t id` 字段 |
| `EGUI_CONFIG_DEBUG_CLASS_NAME` | `0` | 给 view / activity / dialog 保留名字 | 打开 `DBG` 日志后看控件名称 | 会给 `egui_view_t` 增加一个 `const char *name` 字段 |
| `EGUI_CONFIG_DEBUG_LOG_LEVEL` | `EGUI_LOG_IMPL_LEVEL_NONE` | 控制日志等级 | `ERR` / `WRN` / `INF` / `DBG` 按需打开 | `DBG` 日志量很大，嵌入式平台慎用 |
| `EGUI_CONFIG_DEBUG_LOG_SIMPLE` | `1` | 控制日志格式 | `0` 时带等级、函数名和行号 | `1` 更短，`0` 更适合定位 |
| `EGUI_CONFIG_DEBUG_DIRTY_REGION_STATS` | `0` | 输出每帧脏区面积和 PFB tile 统计 | 做性能分析、生成报告 | 日志以 `DIRTY_REGION_STATS:` 开头 |
| `EGUI_CONFIG_DEBUG_DIRTY_REGION_DETAIL` | `0` | 输出每帧 dirty region 矩形明细 | 看一帧内有哪些脏区 | 日志量中等 |
| `EGUI_CONFIG_DEBUG_DIRTY_REGION_TRACE` | `0` | 输出脏区来源和合并过程 | 深挖“是谁让它变大了” | 日志量很大，建议短时开启 |

## 推荐组合

### 1. 看 tile 扫描

```bash
make all APP=HelloBasic APP_SUB=slider PORT=pc \
  USER_CFLAGS="-DEGUI_CONFIG_DEBUG_PFB_REFRESH=1 -DEGUI_CONFIG_DEBUG_REFRESH_DELAY=40"
```

### 2. 看脏区是否合理

```bash
make all APP=HelloBasic APP_SUB=slider PORT=pc \
  USER_CFLAGS="-DEGUI_CONFIG_DEBUG_DIRTY_REGION_REFRESH=1 -DEGUI_CONFIG_DEBUG_REFRESH_DELAY=40"
```

### 3. 看触摸路径

```bash
make all APP=HelloBasic APP_SUB=slider PORT=pc \
  USER_CFLAGS="-DEGUI_CONFIG_DEBUG_TOUCH_TRACE=1"
```

### 4. 查脏区收益

```bash
make all APP=HelloShowcase PORT=pc \
  USER_CFLAGS="-DEGUI_CONFIG_DEBUG_DIRTY_REGION_STATS=1"
```

### 5. 追日志到具体控件

```bash
make all APP=HelloShowcase PORT=pc \
  USER_CFLAGS="-DEGUI_CONFIG_DEBUG_CLASS_NAME=1 -DEGUI_CONFIG_DEBUG_VIEW_ID=1 -DEGUI_CONFIG_DEBUG_LOG_LEVEL=EGUI_LOG_IMPL_LEVEL_DBG -DEGUI_CONFIG_DEBUG_LOG_SIMPLE=0"
```

## 运行效果

### PFB 刷新可视化

下面这张图里红色网格就是 PFB tile 边界。为了把 tile 划分看得更清楚，截图时把 `PFB_WIDTH/PFB_HEIGHT` 临时调成了 `20x20`。

```{figure} images/pfb_refresh.png
:name: debug-pfb-refresh

`EGUI_CONFIG_DEBUG_PFB_REFRESH=1` 的效果。红色边框表示当前 PFB tile。
```

### 脏区可视化

```{figure} images/dirty_region_refresh.png
:name: debug-dirty-region-refresh

`EGUI_CONFIG_DEBUG_DIRTY_REGION_REFRESH=1` 的效果。蓝色边框表示 dirty region。
```

### 同时看 tile 和脏区

```{figure} images/pfb_dirty_refresh.png
:name: debug-pfb-dirty-refresh

`EGUI_CONFIG_DEBUG_PFB_REFRESH=1` 和 `EGUI_CONFIG_DEBUG_DIRTY_REGION_REFRESH=1` 同时开启。红色看 tile，蓝色看 dirty region。
```

### 实时运行信息

`EGUI_CONFIG_DEBUG_INFO_SHOW` 已移除，当前请直接使用 `EGUI_CONFIG_DEBUG_PERF_MONITOR_SHOW=1` 和 `EGUI_CONFIG_DEBUG_MEM_MONITOR_SHOW=1`。详细计算规则、显示格式、位置参数和运行截图见 {doc}`../performance/debug_monitor`。

```{figure} images/debug_info_show.png
:name: debug-info-show

旧 `EGUI_CONFIG_DEBUG_INFO_SHOW` 所覆盖的调试监视器效果。
```

### 触摸轨迹

轨迹线默认关闭。编译时打开 `EGUI_CONFIG_DEBUG_TOUCH_TRACE=1` 后，只保留最近一次从按下到松开的轨迹，自动用红色 1 像素线绘制。

```{figure} images/touch_trace_on.png
:name: debug-touch-trace-on

`EGUI_CONFIG_DEBUG_TOUCH_TRACE=1` 的效果。红色细线是最新一条触摸轨迹。
```

```{figure} images/touch_trace_off.png
:name: debug-touch-trace-off

关闭 `EGUI_CONFIG_DEBUG_TOUCH_TRACE` 后，同一场景不会显示轨迹线。
```
