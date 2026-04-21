# PC 端调试流程

`PORT=pc` 是最推荐的第一现场。它的优势是：

- 构建快，改完代码可以立刻复现
- 可以直接录制截图，方便看渲染结果
- 支持 `--headless` 运行，便于脚本化

## 单次抓图

最直接的方式是：

1. 用 `USER_CFLAGS` 临时打开调试宏
2. 运行 `output/main.exe`
3. 用 `--record` 把帧抓到目录里

示例：抓一组脏区框截图

```bash
make clean
make all APP=HelloBasic APP_SUB=slider PORT=pc \
  USER_CFLAGS="-DEGUI_CONFIG_FUNCTION_RECORDING_TEST=1 -DEGUI_CONFIG_DEBUG_DIRTY_REGION_REFRESH=1 -DEGUI_CONFIG_DEBUG_REFRESH_DELAY=40"

output/main.exe output/app_egui_resource_merge.bin \
  --record runtime_check_output/debug_dirty_slider 10 8 --headless
```

关键点：

- `EGUI_CONFIG_FUNCTION_RECORDING_TEST=1` 让示例里的录制动作自动跑起来，方便稳定复现交互
- `--record <dir> <fps> <duration>` 会把 PNG 帧保存到目录里
- `--headless` 适合 CI、远程环境和批量抓图

## 可视化类调试的常用命令

### 看 PFB tile

```bash
make clean
make all APP=HelloBasic APP_SUB=slider PORT=pc \
  USER_CFLAGS="-DEGUI_CONFIG_FUNCTION_RECORDING_TEST=1 -DEGUI_CONFIG_DEBUG_PFB_REFRESH=1 -DEGUI_CONFIG_DEBUG_REFRESH_DELAY=40"
```

### 看脏区范围

```bash
make clean
make all APP=HelloBasic APP_SUB=slider PORT=pc \
  USER_CFLAGS="-DEGUI_CONFIG_FUNCTION_RECORDING_TEST=1 -DEGUI_CONFIG_DEBUG_DIRTY_REGION_REFRESH=1 -DEGUI_CONFIG_DEBUG_REFRESH_DELAY=40"
```

### 看触摸轨迹

```bash
make clean
make all APP=HelloBasic APP_SUB=slider PORT=pc \
  USER_CFLAGS="-DEGUI_CONFIG_FUNCTION_RECORDING_TEST=1 -DEGUI_CONFIG_DEBUG_TOUCH_TRACE=1"
```

### 看实时运行信息

```bash
make clean
make all APP=HelloTest PORT=pc
python scripts/code_runtime_check.py --app HelloTest --keep-screenshots
```

## 推荐排查顺序

### 渲染范围不对

先开：

```bash
-DEGUI_CONFIG_DEBUG_PFB_REFRESH=1
-DEGUI_CONFIG_DEBUG_DIRTY_REGION_REFRESH=1
-DEGUI_CONFIG_DEBUG_REFRESH_DELAY=40
```

先确认两件事：

- 蓝框是不是比预期大很多
- 红框是不是在做全屏扫描，或者 tile 尺寸明显不合适

### 交互不对

先开：

```bash
-DEGUI_CONFIG_DEBUG_TOUCH_TRACE=1
```

然后看：

- 红线是不是经过了你预期的控件
- 轨迹起点和终点有没有被缩放、旋转或坐标映射搞偏

### 性能或脏区收益不明确

先开：

```bash
-DEGUI_CONFIG_DEBUG_DIRTY_REGION_STATS=1
```

然后配合 `scripts/perf_analysis/dirty_region_stats_report.py` 出报告。

## 运行时验证

如果不是一次性抓图，而是要稳定回归，优先用脚本。

### 单个应用或子示例

```bash
python scripts/code_runtime_check.py --app HelloBasic --app-sub slider --keep-screenshots
python scripts/code_runtime_check.py --app HelloVirtual --app-sub virtual_stage_showcase --keep-screenshots
```

输出目录在 `runtime_check_output/` 下。

### 多屏专项回归

多屏相关改动后，优先补跑：

```bash
python scripts/release_check.py --scope multi-display
python scripts/code_compile_check.py --scope multi-display --case-jobs 2
python scripts/code_runtime_check.py --scope multi-display --jobs 2 --timeout 10 --keep-screenshots
```

其中 `release_check.py --scope multi-display` 会把 compile/runtime/doc 串起来；如果你正在看副屏截图细节，继续用下面两条拆开的命令更直接。

这会只覆盖：

- `HelloMultiDisplay`
- `HelloMultiDisplayHetero`

适合验证：

- descriptor 扩展
- per-core GUI 线程
- 副屏输入路由
- 多屏录制和截图输出

### 批量跑 `HelloBasic` / `HelloVirtual`

```bash
python scripts/checks/hello_basic_render_workflow.py --app HelloBasic --suite basic
python scripts/checks/hello_basic_render_workflow.py --app HelloVirtual --suite basic
```

这个脚本除了抓图，还会检查：

- 录制动作是否存在
- 截图是不是空白
- 交互前后是否真的发生了画面变化

## 调试结果该怎么看

- `frame_*.png`
  看最终渲染和调试叠加是否符合预期
- `runtime_check_output/<app>/...`
  看一次录制的完整输出
- 终端日志
  看 `DIRTY_REGION_STATS`、`DIRTY_REGION_TRACE` 或控件级别调试信息

如果你需要更系统地看每个脚本负责什么，继续看 {doc}`debug_scripts`。
