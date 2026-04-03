# Virtual Stage 示例

## 这个示例演示什么

`HelloVirtual/virtual_stage` 演示的是一类典型页面：

- 页面节点很多
- 大部分节点只负责显示
- 只有少量节点需要真实控件交互
- 输入态、选择态、展开态和业务状态全部外置维护

当前页面是一个 `800x800` 的业务驾驶舱，内部固定放了 `100` 个节点：

- 默认节点以 render-only 方式绘制
- 只有交互节点才按需 materialize 为真实控件
- textinput、combobox、picker、roller、segmented 等状态离场后仍可恢复

## 页面组成

页面包含四个业务面板：

- `Overview`
- `Floor Grid`
- `Control Rack`
- `Alert Queue`

示例同时覆盖了两类节点：

- render-only：KPI、line、scale、chart、clock、gauge、calendar、message、textblock、list、window、tileview 等
- live widget：button、textinput、switch、checkbox、radio、slider、toggle、number picker、combobox、roller、segmented、button matrix

## 核心演示点

### 1. render-only 优先

大多数节点不分配真实控件实例，而是直接走 `draw_node()` 绘制。

### 2. 交互节点按需 materialize

用户点击某个交互节点后，`virtual_stage` 才会：

- 从 slot 池中分配可用位置
- 创建或复用真实 view
- 绑定当前业务数据

### 3. 业务状态外置

示例把这些状态全部放在业务层：

- search / note 文本
- switch / checkbox / radio 状态
- slider / picker 数值
- combobox / roller / segmented 当前索引
- quick matrix、pin、shift、zone、machine active 等业务状态

### 4. 少量节点保活

示例同时覆盖三种保活方式：

- focus：输入框聚焦时保活
- pin：`Mixer` / `AGV` 可手动 pin
- keepalive：`Combobox` 展开时保活

## 关键文件

- `src/widget/egui_view_virtual_stage.h`
- `src/widget/egui_view_virtual_stage.c`
- `example/HelloVirtual/virtual_stage/app_egui_config.h`
- `example/HelloVirtual/virtual_stage/demo_virtual_stage_internal.h`
- `example/HelloVirtual/virtual_stage/demo_virtual_stage_adapter.c`
- `example/HelloVirtual/virtual_stage/test.c`
- `example/HelloUnitTest/test/test_virtual_stage.c`

## 当前示例结构

示例现在分成三层：

- `test.c`：页面布局、业务状态、render-only 绘制、录制动作
- `demo_virtual_stage_adapter.c`：真实控件创建、绑定、保存和恢复状态、命中测试、keepalive
- `demo_virtual_stage_internal.h`：共享 enum / struct / helper 声明

这样拆开后，用户可以直接复用一套“节点数组 + ops + bridge”的组织方式，而不用把所有逻辑都塞进一个文件里。

## 最小接入思路

### 1. 先规划节点和业务状态

把页面拆成两部分：

- 节点描述：位置、大小、类型、`stable_id`、`z_order`
- 业务状态：文本、数值、选中项、展开态、动画态

### 2. 只给交互节点分配 `view_type`

如果节点只负责显示：

- `view_type = EGUI_VIEW_VIRTUAL_STAGE_VIEW_TYPE_NONE`
- 直接走 render-only 绘制

如果节点需要真实交互：

- 设置自己的 `view_type`
- 在 adapter/ops 里实现 `create_view / bind_view / save_state / restore_state`

### 3. 初始化 virtual_stage

这个复杂示例现在也直接走数组 bridge：

```c
static egui_view_virtual_stage_t page;

EGUI_VIEW_VIRTUAL_STAGE_NODE_ARRAY_BRIDGE_INIT_WITH_LIMIT(stage_bridge, 16, 124, 768, 660, 4, nodes, my_node_t, desc, &stage_ops, &my_context);

EGUI_VIEW_VIRTUAL_STAGE_INIT_ARRAY_BRIDGE(&page, &stage_bridge);
EGUI_VIEW_VIRTUAL_STAGE_ADD_ROOT(&page);
```

也就是说，复杂例程不再先写 `params + setup + raw adapter`。如果你的节点本来就在本地数组里，优先直接抄这条；初始化后再直接用 `EGUI_VIEW_VIRTUAL_STAGE_ADD_ROOT(...)` 挂到页面即可。

如果你只是单文件接入，`stage_ops` 还可以直接用这几档 helper：

- `EGUI_VIEW_VIRTUAL_STAGE_ARRAY_OPS_SIMPLE_INIT(...)`
- `EGUI_VIEW_VIRTUAL_STAGE_ARRAY_OPS_INTERACTIVE_INIT(...)`
- `EGUI_VIEW_VIRTUAL_STAGE_ARRAY_OPS_STATEFUL_INIT(...)`
- `EGUI_VIEW_VIRTUAL_STAGE_INIT_ARRAY_BRIDGE(...)`

如果你像这个示例一样把 adapter/ops 单独拆到另一个 `.c` 文件里，直接用 `EGUI_VIEW_VIRTUAL_STAGE_ARRAY_OPS_*_CONST_INIT(...)` 导出一个独立的 `stage_ops` 常量即可。

### 4. 业务变化时定向刷新

```c
EGUI_VIEW_VIRTUAL_STAGE_NOTIFY_NODE(&page, stable_id);
EGUI_VIEW_VIRTUAL_STAGE_NOTIFY_NODE_BOUNDS(&page, stable_id);
```

不要把所有变化都退化成整页刷新。

如果一次只联动少量节点，例如一个按钮状态和它旁边的摘要卡片一起变化，也可以直接写成：

```c
EGUI_VIEW_VIRTUAL_STAGE_NOTIFY_IDS(&page, MY_NODE_BUTTON_ID, MY_NODE_SUMMARY_ID);
```

这样就不用为了 2 到 3 个 `stable_id` 先单独声明临时数组。

如果同一组 `stable_id` 需要反复复用，再继续使用 `EGUI_VIEW_VIRTUAL_STAGE_NOTIFY_NODES(...)` / `EGUI_VIEW_VIRTUAL_STAGE_NOTIFY_NODES_BOUNDS(...)`。

### 5. 输入和展开态显式处理

常见经验：

- textinput：失焦前保存文本
- combobox：展开时 keepalive，收起后释放
- 动画：外置进度，或在动画期间 keepalive

## 当前录制链路

示例已经带了一条比较完整的业务录制链路，覆盖：

- switch / checkbox / radio / toggle / slider
- picker / combobox / roller / segmented
- pin Mixer / pin AGV
- search / note 输入与恢复
- quick matrix、zone、export
- machine、shift、purge、lock、crew
- 最终 reset

它验证的不只是静态展示，还包括：

- slot 复用
- 焦点清理
- 状态回存
- keepalive / pin
- reset 后资源释放

## 运行命令

构建示例：

```bash
make all APP=HelloVirtual APP_SUB=virtual_stage PORT=pc
```

运行时检查：

```bash
python scripts/code_runtime_check.py --app HelloVirtual --app-sub virtual_stage --keep-screenshots
```

渲染工作流：

```bash
python scripts/checks/hello_basic_render_workflow.py --app HelloVirtual --widgets virtual_stage --skip-unit-tests
```

框架回归：

```bash
make all APP=HelloUnitTest PORT=pc_test
output\main.exe
```

## 更适合什么项目

如果你的页面满足下面任意几条，就可以优先考虑 `virtual_stage`：

- 页面节点数很多
- 大部分节点只是显示
- 只有少量节点真的要交互
- 设备 SRAM 很紧
- 你愿意把状态放在业务层

如果你的主要问题是“长列表滚动”，优先看 `HelloVirtual/virtual_viewport`。
