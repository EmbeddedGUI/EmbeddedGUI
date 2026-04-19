# 多屏方案（Multi-Display）

## 概述

EmbeddedGUI 支持在同一进程内驱动多块屏幕。当前实现的基本原则是：

- 每块屏幕对应一个独立的 `egui_core_t`
- 每块屏幕拥有自己的分辨率、PFB、输入状态、定时器、动画和 UI 树
- `uicode_dispN_init(core)` 只负责在目标 `core` 上构建 UI，不负责创建 `core`、display driver 或线程

多屏场景的核心约束不是“先切 active core 再使用对象”，而是“对象从初始化开始就绑定到正确的 core 上”。

## 当前 PC 实现

PC 端已经把多屏启动入口收敛为 descriptor 流程，不再在 `porting/pc/main.c` 中写死“主屏 + 固定 display 1”的特殊逻辑。

当前启动过程是：

1. 初始化 SDL / platform
2. 构建主屏 descriptor
3. 通过 `egui_port_get_additional_display_descriptors()` 收集额外屏幕 descriptor
4. 为每个 descriptor 创建 `egui_core_t`、display driver 和 SDL window
5. 调用 `egui_setup_display()`
6. 为每个 core 启动独立 GUI 线程

这意味着：

- `porting/pc/main.c` 现在是通用的
- 应用层只需要提供副屏 descriptor
- 新增副屏时，优先扩展 descriptor，而不是继续往 PC 入口里加 if/else

## 职责划分

### `uicode_disp0_init()` / `uicode_disp1_init()`

这些函数只负责目标屏幕的 UI 初始化，例如：

- 初始化 view / activity / dialog / toast
- 把 view 挂到目标 `core`
- 启动属于该屏幕的定时器

这些函数不应该负责：

- 分配或保存 `egui_core_t`
- 创建 display driver
- 创建副屏窗口
- 注册 GUI 线程

### `porting` 层

`porting` 层负责：

- 维护多屏 `core` 实例
- 准备每块屏幕的 `egui_display_setup_t`
- 创建主屏和副屏 display driver
- 调用 `egui_setup_display()`
- 启动并管理 GUI 线程
- 路由 SDL 输入、录制和退出流程

## 推荐初始化接口

### `egui_setup_display()`

所有屏幕都推荐通过 `egui_setup_display()` 初始化。它会统一完成：

- `egui_core_t` 初始化
- display driver 注册
- 可选的 touch driver 注册
- 调用 `uicode_dispN_init(core)`
- 调用 `egui_screen_on(core)`

`egui_init_display()` 仍然是底层接口，但新代码优先使用 `egui_setup_display()`。

### `egui_port_get_additional_display_descriptors()`

PC 多屏应用推荐通过这个钩子返回额外屏幕描述。每个 descriptor 至少包含：

- `screen_width` / `screen_height`
- `pfb_width` / `pfb_height`
- `pfb_buffers` / `pfb_buffer_count`
- `touch_register`
- `uicode_init`

主屏仍由 PC 入口直接构造，副屏由 descriptor 扩展。

## 线程模型

当前 PC 端采用“1 个 SDL 主线程 + 每个 core 1 个 GUI 线程”的模型：

- SDL 主线程负责窗口事件、录制驱动、截图和退出管理
- 每个 display/core 拥有自己的 GUI 线程，只轮询自己的 `egui_polling_work(core)`

这比早期“单 GUI 线程串行轮询所有 core”更接近真实多屏运行模型。

## 跨线程访问约束

需要严格遵守下面的边界：

- UI 树、动画、定时器、dirty region 只能在所属 core 的 GUI 线程中直接修改
- SDL 主线程不能直接操作 foreign core 的 view/tree
- 跨线程 UI 修改必须先投递到目标 core

当前 PC 端已经提供两个辅助接口：

- `egui_port_post_core_task()`：把操作投递到目标 core
- `egui_port_get_display_runtime_info()`：安全读取 display 运行时信息，避免 SDL 线程直接访问目标 core 内部状态

后续跨屏逻辑应优先走“投递到目标 core”模型，而不是在外部线程直接调用目标屏幕对象 API。

## 配置

多屏默认配置分为两层：

- 单屏默认值位于 `src/core/egui_config_default.h`
- 多屏补充值位于 `src/core/egui_config_multi_default.h`

当前常用宏如下：

```c
#define EGUI_CONFIG_MAX_DISPLAY_COUNT 1

#define EGUI_CONFIG_SCEEN_1_WIDTH  EGUI_CONFIG_SCEEN_WIDTH
#define EGUI_CONFIG_SCEEN_1_HEIGHT EGUI_CONFIG_SCEEN_HEIGHT
#define EGUI_CONFIG_PFB_1_WIDTH    EGUI_CONFIG_PFB_WIDTH
#define EGUI_CONFIG_PFB_1_HEIGHT   EGUI_CONFIG_PFB_HEIGHT
```

注意：

- 没有 `EGUI_CONFIG_SCEEN_0_*` / `EGUI_CONFIG_PFB_0_*`
- display 0 直接使用 `EGUI_CONFIG_SCEEN_WIDTH`、`EGUI_CONFIG_SCEEN_HEIGHT`、`EGUI_CONFIG_PFB_WIDTH`、`EGUI_CONFIG_PFB_HEIGHT`
- display 1、display 2 默认 fallback 到主屏配置，可在 `app_egui_config.h` 中覆盖

启用双屏的最小配置：

```c
#define EGUI_CONFIG_MAX_DISPLAY_COUNT 2
```

异构副屏示例：

```c
#define EGUI_CONFIG_MAX_DISPLAY_COUNT 2

#define EGUI_CONFIG_SCEEN_1_WIDTH  128
#define EGUI_CONFIG_SCEEN_1_HEIGHT 64
#define EGUI_CONFIG_PFB_1_WIDTH    16
#define EGUI_CONFIG_PFB_1_HEIGHT   8
```

## 副屏输入

当前 PC 多屏已经支持副屏独立触摸输入。启用条件是副屏 descriptor 显式注册 `touch_register`：

```c
descriptors[0].touch_register = egui_port_register_touch_driver;
```

启用后：

- SDL 会按窗口把鼠标/触摸事件路由到对应 `display_id`
- 每个 display 维护自己的输入队列
- 副屏可以独立点击、拖动，而不会串到主屏

## 录制与运行时验证

PC 多屏录制已经支持按 `display_id` 路由动作。推荐使用带 `_DISP` 后缀的宏：

- `EGUI_SIM_SET_CLICK_VIEW_DISP()`
- `EGUI_SIM_SET_DRAG_VIEW_DISP()`
- `EGUI_SIM_SET_SWIPE_VIEW_DISP()`

也可以直接设置：

```c
p_action->display_id = 1;
```

当前仓库内已经补齐两个多屏示例的副屏验证闭环：

- `HelloMultiDisplay`
  副屏按钮可独立点击，录制脚本会推进副屏 activity
- `HelloMultiDisplayHetero`
  副屏状态面板可独立点击，录制脚本会验证 tick 重置和页签颜色切换

推荐验证命令：

```bash
python scripts/release_check.py --scope multi-display

python scripts/code_compile_check.py --scope multi-display --case-jobs 2
python scripts/code_runtime_check.py --scope multi-display --jobs 2 --timeout 10 --keep-screenshots

make all APP=HelloMultiDisplay PORT=pc
python scripts/code_runtime_check.py --app HelloMultiDisplay --timeout 10 --keep-screenshots

make all APP=HelloMultiDisplayHetero PORT=pc
python scripts/code_runtime_check.py --app HelloMultiDisplayHetero --timeout 10 --keep-screenshots
```

其中 `release_check.py --scope multi-display` 适合一键串起多屏 compile/runtime/doc 回归；命令启动后会先打印 scoped profile 摘要、`--only` 可用 step、当前激活的 `--only/--skip` 过滤结果、compile/runtime/doc 的 drill-down 命令和关键产物目录，方便失败后直接拆看，summary 尾部也会把失败步骤对应的 `python scripts/release_check.py --scope multi-display --only <step>`、底层命令与产物位置再列出来；如果这一轮通过，summary 里还会把已完成步骤的关键产物目录再汇总一遍。runtime scope 会按 `EGUI_CONFIG_MAX_DISPLAY_COUNT` 校验主屏 `frame_XXXX.png` 和各个额外屏幕 `frame_XXXX_dispN.png` 成套输出，并校验多屏录制阶段标签，确认主屏/副屏关键交互后的稳定快照都已经产出。`HelloMultiDisplay` 还会在录制过程中自检“点主屏只推进主屏 activity”以及“主副屏 activity 动画在重叠窗口内同时处于运行态”；`HelloMultiDisplayHetero` 会自检“主屏连续拖动时副屏 tick 仍持续递增”以及“副屏点击后 tick 归零”。如果要细查单个示例截图，再继续跑下面两条单例命令。

截图输出位于：

- `runtime_check_output/HelloMultiDisplay/default/`
- `runtime_check_output/HelloMultiDisplayHetero/default/`

runtime scope 的终端摘要里还会带上 `checks=...`、`stages=...` 和 `shutdown=begin->threads:N->cleanup:1+M->deinit`，分别用于确认示例内建自检项、录制阶段序列，以及多屏退出阶段的线程回收和 SDL 窗口销毁顺序。

多屏示例建议至少覆盖：

- 主屏交互
- 副屏交互
- 不同 display 的截图输出
- 退出路径和线程回收

## 示例

| 示例 | 说明 |
|------|------|
| `HelloMultiDisplay` | 主屏和副屏同为 240x320，演示多屏 activity 切换与副屏独立输入 |
| `HelloMultiDisplayHetero` | 主屏 240x320，副屏 128x64，演示异构副屏状态面板和跨屏状态联动 |

## 注意事项

1. `display_id` 必须正确设置，`egui_setup_display()` 会写入 `core->id`，PC 端依赖该值做窗口路由、输入分发和截图命名。
2. 多屏 `core` 应由 `porting` 层维护，不要在应用层重复保存另一套副本。
3. 每块屏幕都必须有自己的 PFB，副屏不能复用主屏 PFB。
4. 显式接收 `core` 的 API 必须传目标屏幕自己的 `core`，例如 `egui_timer_start_timer(core, ...)`。
5. 不显式接收 `core` 的对象式 API 依赖对象初始化时绑定的 `core`，对象必须从一开始就构造在正确屏幕上。
6. 对 foreign core 的跨线程 UI 操作不要直接调用对象 API，优先通过 `egui_port_post_core_task()` 投递。
7. PC 当前已支持 descriptor 化扩展，但新增更多屏幕时仍需同步检查线程退出、窗口销毁和录制输出是否完整。
