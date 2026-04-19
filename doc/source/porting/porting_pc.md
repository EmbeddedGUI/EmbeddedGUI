# PC 模拟器移植

PC 模拟器是 EmbeddedGUI 的主要开发和调试平台，基于 SDL2 提供显示、输入、录制和截图能力。它的目标不是完全模拟 MCU 环境，而是提供一个高迭代效率、可自动化验证的本地运行平台。

## 构建与运行

```bash
# 构建
make all APP=HelloSimple PORT=pc

# 运行
make run

# 或直接运行
./output/main.exe
```

切换 `APP` 或 `PORT` 时，建议先执行一次 `make clean`，避免复用旧的 `output/main.exe`：

```bash
make clean
make all APP=HelloUnitTest PORT=pc_test
./output/main.exe
```

## 核心文件

- `porting/pc/main.c`
  PC 入口，负责主屏/副屏 descriptor 收集、线程启动和退出回收
- `porting/pc/sdl_port.c`
  SDL window 管理、事件路由、录制、截图和 runtime info
- `porting/pc/sdl_port.h`
  SDL 相关接口声明
- `porting/pc/egui_port_pc.c`
  display driver、platform driver 和触摸驱动注册

## 运行时模型

当前 PC 端采用下面的线程模型：

- SDL 主线程
  负责窗口事件分发、录制动作推进、截图输出和退出管理
- 每个 display/core 各自的 GUI 线程
  只轮询本屏幕的 `egui_polling_work(core)`

这意味着 PC 端已经不是早期“单 GUI 线程串行轮询所有屏幕”的模型，而是 per-core GUI thread。

## 多屏启动流程

`porting/pc/main.c` 当前流程可以概括为：

1. 初始化 SDL / platform
2. 构造主屏 descriptor
3. 调用 `egui_port_get_additional_display_descriptors()` 收集副屏 descriptor
4. 逐个创建 display driver、touch driver 和 `egui_core_t`
5. 调用 `egui_setup_display()`
6. 为每个 core 启动 GUI 线程
7. SDL 主线程进入事件循环
8. 退出时通知所有 GUI 线程停止，并回收窗口和线程资源

新增副屏时，优先补 descriptor，而不是继续在 `main.c` 写死示例逻辑。

## SDL 显示后端

PC display driver 会把 PFB 数据写入 SDL 纹理，再把纹理刷新到窗口。

典型流程：

- `draw_area`
  把局部像素块写入 SDL 后端缓冲
- `flush`
  提交当前帧并刷新窗口

窗口尺寸由应用配置决定，例如：

```c
#define EGUI_CONFIG_SCEEN_WIDTH  240
#define EGUI_CONFIG_SCEEN_HEIGHT 320
```

副屏则由 descriptor 中的 `screen_width` / `screen_height` 决定。

## 输入路由

SDL 鼠标事件会映射到 EGUI 触摸事件：

| SDL 事件 | EGUI 事件 |
|----------|-----------|
| `SDL_MOUSEBUTTONDOWN` | `EGUI_MOTION_EVENT_ACTION_DOWN` |
| `SDL_MOUSEMOTION` | `EGUI_MOTION_EVENT_ACTION_MOVE` |
| `SDL_MOUSEBUTTONUP` | `EGUI_MOTION_EVENT_ACTION_UP` |

多屏模式下，PC 端会按窗口把事件路由到对应 `display_id`：

- 主屏窗口进入 display 0 输入队列
- 副屏窗口进入对应副屏输入队列
- 不同屏幕的输入不会串屏

如果副屏 descriptor 注册了：

```c
descriptors[i].touch_register = egui_port_register_touch_driver;
```

那么该副屏就可以独立接收触摸输入。

## 键盘事件

SDL 键盘事件会映射为 EGUI 按键事件，例如：

| SDL 键 | EGUI 键 |
|--------|---------|
| `SDLK_RETURN` | `EGUI_KEY_CODE_ENTER` |
| `SDLK_ESCAPE` | `EGUI_KEY_CODE_BACK` |
| `SDLK_UP/DOWN/LEFT/RIGHT` | 方向键 |

启用键盘支持：

```c
#define EGUI_CONFIG_FUNCTION_SUPPORT_KEY 1
```

## 跨线程访问边界

PC 端最重要的约束是：SDL 主线程不能直接修改 foreign core 的 UI。

需要遵守：

- UI 树、动画、定时器、dirty region 只能在所属 GUI 线程中直接修改
- 跨线程操作必须先投递到目标 core

当前可用的辅助接口：

- `egui_port_post_core_task()`
  把任务投递到目标 core 的 GUI 线程执行
- `egui_port_get_display_runtime_info()`
  同步读取 display 运行时状态，避免 SDL 线程直接访问目标 core 内部数据

这两个接口是 PC 多屏线程安全模型的基础，后续 port 如需跟进，也建议沿用同样的职责边界。

## 新 Port 对齐建议

如果后续 MCU / RTOS port 也要跟进本地多屏并行线程方案，建议至少保持下面几条不变：

- display 的创建入口保持 descriptor 化，不再把“主屏 + 固定 display 1”写死在启动代码里
- 每个 display/core 对应自己的 GUI 线程或等价轮询上下文，不要回退到单线程串行轮询全部 core
- foreign core 的 UI 修改统一走“投递到目标 core”入口，不要让事件线程、录制线程或业务线程直接调用目标对象 API
- 读取运行时状态时优先提供快照/查询接口，避免外部线程直接探测目标 core 内部结构
- 退出顺序固定为“先停 GUI 线程，再销毁 display/window/driver，最后做 platform deinit”，这样更容易做自动化校验

PC 端当前已经按这个边界拆成了 `egui_port_get_additional_display_descriptors()`、`egui_port_post_core_task()`、`egui_port_get_display_runtime_info()` 和 shutdown 标记链路。新 port 如果复用相同职责划分，后续脚本和文档也更容易沿用。

## Platform Driver

PC platform driver 主要依赖标准库和 SDL：

| 接口 | PC 实现 |
|------|---------|
| 堆分配 | 默认 `malloc` / `free` |
| 日志 / 格式化 | 默认 `vprintf` / `vsprintf` |
| `get_tick_ms` | `sdl_get_system_timestamp_ms()` |
| `delay` | `sdl_port_sleep()` |
| 内存清零 / 拷贝 | 默认 `memset` / `memcpy` |
| `interrupt_disable/enable` | 由 PC port 提供最小同步能力，供输入等临界区使用 |

PC 端的 `interrupt_disable/enable` 并不是 MCU 意义上的真实中断屏蔽，而是为了给输入等共享队列提供统一的最小临界区包装。

## 录制与截图

PC 模拟器支持录制截图序列，用于 runtime check 和 GIF 生成：

```bash
./output/main.exe --record output_dir 30 10
./output/main.exe --record output_dir 30 10 --speed 2
```

启用录制测试：

```c
#define EGUI_CONFIG_RECORDING_TEST 1
```

应用可以实现：

```c
bool egui_port_get_recording_action(int action_index, egui_sim_action_t *p_action);
```

多屏录制推荐直接指定目标屏幕：

- 设置 `p_action->display_id`
- 或使用 `EGUI_SIM_SET_*_DISP()` 宏

例如：

```c
EGUI_SIM_SET_CLICK_VIEW_DISP(p_action, target_view, 600, 1);
```

## 运行时验证

代码修改后，推荐至少执行：

```bash
make clean
make all APP=HelloSimple PORT=pc
python scripts/code_runtime_check.py --app HelloSimple --timeout 10 --keep-screenshots
```

多屏示例建议额外验证：

```bash
python scripts/release_check.py --scope multi-display

python scripts/code_compile_check.py --scope multi-display --case-jobs 2
python scripts/code_runtime_check.py --scope multi-display --jobs 2 --timeout 10 --keep-screenshots

make clean
make all APP=HelloMultiDisplay PORT=pc
python scripts/code_runtime_check.py --app HelloMultiDisplay --timeout 10 --keep-screenshots

make clean
make all APP=HelloMultiDisplayHetero PORT=pc
python scripts/code_runtime_check.py --app HelloMultiDisplayHetero --timeout 10 --keep-screenshots
```

其中 `release_check.py --scope multi-display` 适合先做一轮一键多屏回归；命令开头会直接打印 scoped profile 摘要、`--only` 可用 step、compile/runtime/doc 的 drill-down 命令和关键产物目录，summary 尾部也会把失败步骤对应的 `python scripts/release_check.py --scope multi-display --only <step>`、底层命令与产物位置再列一遍。如果需要看具体双屏录制输出，再按需打开单个示例截图细查。

输出截图位于 `runtime_check_output/`。

当前多屏 runtime 摘要会带上：

- `checks=...`
- `stages=...`
- `shutdown=begin->threads:N->cleanup:1+M->deinit`

分别用于确认示例内建自检、录制阶段序列，以及线程退出和窗口销毁顺序。

当前多屏示例里，`HelloMultiDisplay` 会额外验证“主屏点击不会误推进副屏 activity”以及“主副屏 activity 动画在重叠窗口内同时处于运行态”；`HelloMultiDisplayHetero` 会验证“主屏连续拖动时副屏 tick 不停摆”与“副屏点击后 tick 归零”。

## 文件系统资源加载

PC 平台通过标准文件 I/O 加载外部资源。默认情况下会读取当前目录中的：

```text
app_egui_resource_merge.bin
```

也可以通过命令行指定资源文件路径：

```bash
./output/main.exe /path/to/app_egui_resource_merge.bin
```

## 调试能力

常用调试宏：

```c
#define EGUI_CONFIG_DEBUG_LOG_LEVEL EGUI_LOG_IMPL_LEVEL_DBG
#define EGUI_CONFIG_DEBUG_PFB_REFRESH 1
#define EGUI_CONFIG_DEBUG_DIRTY_REGION_REFRESH 1
#define EGUI_CONFIG_DEBUG_PERF_MONITOR_SHOW 1
#define EGUI_CONFIG_DEBUG_MEM_MONITOR_SHOW 1
```

触摸轨迹调试：

```bash
make all APP=HelloBasic APP_SUB=slider PORT=pc USER_CFLAGS="-DEGUI_CONFIG_DEBUG_TOUCH_TRACE=1"
```

只要触摸支持开启，窗口和截图中就会显示红色触摸轨迹线。

## 常见问题

- SDL2 未安装
  需要先安装 SDL2 开发库
- 窗口不显示
  检查 `EGUI_CONFIG_SCEEN_WIDTH/HEIGHT` 和 SDL 初始化是否成功
- 触摸无响应
  确认 `EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH=1`，并且目标 descriptor 已注册 `touch_register`
- 多屏录制没有命中副屏
  检查 `display_id` 是否设置正确，或是否使用了 `_DISP` 宏
- 运行了错误的 `output/main.exe`
  切换 `APP` / `PORT` 后先执行 `make clean`
