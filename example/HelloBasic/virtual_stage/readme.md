# Virtual Stage 示例

## 这个示例演示什么

`HelloBasic/virtual_stage` 演示的是一套“页面节点很多，但控件实例很少”的页面组织方式。

当前示例是一个 `800x800` 的业务驾驶舱：

- 页面同时呈现 `100` 个节点
- 默认节点以 render-only 方式绘制
- 只有交互节点才按需创建真实控件
- 输入态、选中态、展开态、业务态由外部数据保存

它适合验证：

- 大画布页面是否可以不为每个节点都常驻一个 view
- 复杂 render-only 节点是否能和少量 live widget 混用
- textinput / combobox / picker / roller / segmented 等状态是否可以离场后恢复

## 页面组成

当前页面包含四个业务面板：

- `Overview`
- `Floor Grid`
- `Control Rack`
- `Alert Queue`

示例里同时覆盖了大量控件形态，包括：

- render-only：KPI、line、scale、chart、clock、gauge、calendar、message、textblock、list、window、tileview 等
- live widget：button、textinput、switch、checkbox、radio、slider、toggle、number picker、combobox、roller、segmented、button matrix

## 核心演示点

### 1. render-only 优先

大多数节点都不分配真实控件实例，而是由 `draw_node()` 直接绘制。

### 2. 交互节点按需 materialize

当用户点击某个可交互节点时：

- `virtual_stage` 才会从 slot 池里拿一个位置
- 创建/复用真实 view
- 绑定当前业务数据

### 3. 状态外置

示例把以下状态都放在业务层：

- search / note 文本
- switch / checkbox / radio 状态
- slider / picker 数值
- combobox / roller / segmented 当前索引
- quick matrix、pin、shift、zone、machine active 等业务状态

### 4. 少量节点保活

示例通过三种方式控制节点暂时不要释放：

- focus：输入框聚焦时保活
- pin：Mixer / AGV 可以手动 pin
- keepalive：combobox 展开时保活

## 关键文件

- `src/widget/egui_view_virtual_stage.h`
- `src/widget/egui_view_virtual_stage.c`
- `example/HelloBasic/virtual_stage/app_egui_config.h`
- `example/HelloBasic/virtual_stage/demo_virtual_stage_internal.h`
- `example/HelloBasic/virtual_stage/demo_virtual_stage_adapter.c`
- `example/HelloBasic/virtual_stage/test.c`
- `example/HelloUnitTest/test/test_virtual_stage.c`

## 当前示例结构

示例现在分成三层：

- `test.c`：页面布局、业务状态、render-only 绘制、录制动作
- `demo_virtual_stage_adapter.c`：`virtual_stage` adapter 实现、view 创建/绑定/保存/恢复
- `demo_virtual_stage_internal.h`：共享 enum / struct / helper 声明

这样后续要做新的业务页面时，可以更方便地：

- 保留一套通用 adapter/helper 组织方式
- 只替换节点数据和业务状态
- 避免把 adapter 逻辑和大段 render-only 绘制全部揉在一个文件里

## 最小接入思路

### 1. 先规划节点和业务状态

把页面拆成：

- 节点描述：位置、大小、类型、stable_id、z_order
- 业务状态：文本、数值、选中项、展开态、动画态

### 2. 只给交互节点分配 `view_type`

如果节点只是显示：

- `view_type = EGUI_VIEW_VIRTUAL_STAGE_VIEW_TYPE_NONE`
- 走 render-only 绘制

如果节点需要真实交互：

- 设置自己的 `view_type`
- 在 adapter 里实现 `create_view / bind_view / save_state / restore_state`

### 3. 初始化 virtual_stage

```c
static egui_view_virtual_stage_t page;
EGUI_VIEW_VIRTUAL_STAGE_PARAMS_INIT(page_params, 16, 124, 768, 660);

egui_view_virtual_stage_init_with_params(EGUI_VIEW_OF(&page), &page_params);
egui_view_virtual_stage_set_live_slot_limit(EGUI_VIEW_OF(&page), 4);
egui_view_virtual_stage_set_adapter(EGUI_VIEW_OF(&page), &my_adapter, &my_context);
```

### 4. 业务变化时定向刷新

```c
egui_view_virtual_stage_notify_node_changed(EGUI_VIEW_OF(&page), stable_id);
egui_view_virtual_stage_notify_node_bounds_changed(EGUI_VIEW_OF(&page), stable_id);
```

不要所有变化都退化成整页刷新。

### 5. 输入和展开态显式处理

典型经验：

- textinput：失焦前保存文本
- combobox：展开态 keepalive，收起后释放
- 动画：外置进度，或者动画期间 keepalive

## 当前录制链路

示例已经带了一条较完整的业务录制链路，覆盖：

- switch / checkbox / radio / toggle / slider
- picker / combobox / roller / segmented
- pin Mixer / pin AGV
- search / note 输入与恢复
- quick matrix、zone、export
- machine、shift、purge、lock、crew
- 最终 reset

所以它不只是静态摆控件，而是在验证：

- slot 复用
- 焦点清理
- 状态回写
- keepalive / pin
- reset 后资源释放

## 运行命令

构建示例：

```bash
make all APP=HelloBasic APP_SUB=virtual_stage PORT=pc
```

运行时检查：

```bash
python scripts/code_runtime_check.py --app HelloBasic --app-sub virtual_stage --keep-screenshots
```

渲染工作流：

```bash
python scripts/hello_basic_render_workflow.py --widgets virtual_stage --skip-unit-tests
```

框架回归：

```bash
make all APP=HelloUnitTest PORT=pc_test
output\main.exe
```

## 更适合什么项目

如果你的页面满足下面任意几条，就可以优先考虑 `virtual_stage`：

- 页面节点数很多
- 大部分节点只是展示
- 只有少量节点真的要交互
- 设备 SRAM 很紧
- 你愿意把状态放在业务层

如果你的主要问题是“长列表滚动”，请优先看 `HelloBasic/virtual_viewport`。
