# Virtual Stage

## 概览

`virtual_stage` 面向“大画布 + 大量绝对定位节点 + 少量活跃交互控件”的页面场景。

它解决的不是长列表滚动复用，而是这类固定大页面问题：

- 一个页面上同时摆放几十到上百个节点
- 大多数节点只负责显示，不值得长期常驻完整控件实例
- 少量节点需要交互、焦点、输入态、展开态或短时动画
- 设备 SRAM 紧张，不能让每个节点都长期持有一份 `view`

和 `virtual_viewport` / `virtual_list` 的区别：

- `virtual_viewport` 解决“滚动窗口里的列表项复用”
- `virtual_stage` 解决“固定页面中的节点按需 materialize”

## 适用场景

- 工业驾驶舱、产线看板、设备总览页
- 大尺寸配置页、工位页、状态大盘
- 节点很多，但真正需要交互的节点很少
- 希望把状态尽量放到业务层，而不是全压在控件实例内部

不太适合的场景：

- 主要矛盾是长列表滚动，优先考虑 `virtual_viewport`
- 页面上绝大多数节点都要同时可编辑并长期活跃
- 控件动画强依赖内部瞬时状态，但业务层又不愿意外置保存

## 核心模型

### 节点描述 `egui_virtual_stage_node_desc_t`

每个节点由一个轻量描述结构体定义：

| 字段 | 说明 |
| --- | --- |
| `region` | 节点在 page 内的绝对区域 |
| `stable_id` | 稳定 ID，用于 pin、状态恢复、定向刷新 |
| `view_type` | 需要 materialize 时创建的控件类型；render-only 节点填 `EGUI_VIEW_VIRTUAL_STAGE_VIEW_TYPE_NONE` |
| `z_order` | 命中测试与绘制顺序 |
| `flags` | 交互、保活、隐藏等标记 |

建议：

- `stable_id` 一旦对外暴露，就保持稳定，不要跟着数组重排变化
- render-only 节点统一走 `draw_node`
- 只有真的要交互的节点才分配 `view_type`

### Adapter

`virtual_stage` 不理解业务语义，所有业务接入都通过 adapter 完成。

常用回调：

| 回调 | 是否建议实现 | 用途 |
| --- | --- | --- |
| `get_count` | 必需 | 返回节点数 |
| `get_desc` | 必需 | 返回指定节点描述 |
| `create_view` | 交互节点必需 | 按 `view_type` 创建真实控件 |
| `destroy_view` | 交互节点必需 | 销毁真实控件 |
| `bind_view` | 必需 | 把业务数据绑定到 live view |
| `draw_node` | 强烈建议 | render-only 节点绘制入口 |
| `hit_test` | 可选 | 自定义命中区域 |
| `save_state` | 推荐 | 保存 textinput / slider / combobox 等状态 |
| `restore_state` | 推荐 | 恢复交互状态 |
| `should_keep_alive` | 推荐 | 控制展开态、焦点态、动画态是否保活 |

### Slot 池模型

`virtual_stage` 内部维护固定数量的 live slot，真正占 SRAM 的不是“页面节点数”，而是“当前活跃 slot 数”。

典型 slot 状态：

- `ACTIVE`：当前命中或当前正在交互
- `KEEPALIVE`：因为 pin / focus / adapter keepalive 而继续保留
- `POOLED`：暂未绑定目标节点，但可以复用
- `UNUSED`：空闲 slot

内存思路：

- 传统方案：`100 节点 * 100 份 view 实例`
- `virtual_stage`：`live_slot_limit * 平均 view 大小 + 业务状态`

所以节点越多、交互节点占比越低，收益越明显。

## 生命周期

### render-only 节点

如果节点只是显示：

- `view_type = EGUI_VIEW_VIRTUAL_STAGE_VIEW_TYPE_NONE`
- 不创建真实控件
- 由 `adapter->draw_node()` 直接绘制

这种节点几乎不占控件实例 SRAM，适合：

- KPI 卡片
- badge / chip / status
- 复杂业务摘要卡
- 图表缩略态

### 交互节点按需 materialize

如果节点需要交互：

1. 用户点击命中节点
2. `virtual_stage` 依据 `view_type` 选择或创建 slot
3. 调用 `bind_view`
4. 如有历史状态，再调 `restore_state`
5. 将事件派发给真实控件

### 释放与状态保存

当节点不再需要常驻时：

1. `virtual_stage` 调用 `save_state`
2. 如实现了 `unbind_view`，先做解绑
3. slot 被回收或复用

建议把这些状态保存在业务层：

- 文本内容
- 当前选项
- 当前数值
- 当前展开项
- 动画进度或业务计时

## 公开接口逐项说明

以下接口定义见 `src/widget/egui_view_virtual_stage.h`。

### `egui_view_virtual_stage_apply_params`

作用：把 `egui_view_virtual_stage_params_t` 应用到一个已初始化的 `virtual_stage`。

典型用途：

- 更新 page 区域
- 更新初始 `live_slot_limit`
- 在二次初始化前复用同一个对象

注意：

- 它只应用参数，不负责完整初始化内部结构
- 正常首次接入更推荐直接使用 `init_with_params`

### `egui_view_virtual_stage_init_with_params`

作用：初始化 `virtual_stage`，并同时应用 `region`、`live_slot_limit`。

最常用接入方式：

```c
static egui_view_virtual_stage_t page;
EGUI_VIEW_VIRTUAL_STAGE_PARAMS_INIT(page_params, 16, 124, 768, 660);

egui_view_virtual_stage_init_with_params(EGUI_VIEW_OF(&page), &page_params);
```

建议：

- 首次创建 page 时优先用它
- 初始化后再设置 adapter，再挂到根视图

### `egui_view_virtual_stage_set_adapter`

作用：注册 adapter 和 adapter 上下文。

它决定了：

- 页面有多少节点
- 每个节点描述是什么
- 哪些节点需要真实控件
- render-only 节点如何绘制
- 状态如何保存与恢复

典型顺序：

```c
egui_view_virtual_stage_init_with_params(EGUI_VIEW_OF(&page), &page_params);
egui_view_virtual_stage_set_adapter(EGUI_VIEW_OF(&page), &my_adapter, &my_ctx);
```

注意：

- 业务层数据通常通过 `adapter_context` 传入
- 如果换了数据源，通常要配合 `notify_data_changed`

### `egui_view_virtual_stage_set_live_slot_limit`

作用：设置 page 允许同时保留的 live slot 数。

适合：

- 根据 SRAM 预算压缩 slot 数
- 根据页面交互复杂度提高或降低上限

经验值：

- 纯展示页面：`2~3`
- 有少量输入 / 展开交互：`3~4`
- 多个复杂交互需要并存：再按需增加

注意：

- 它影响的是“真实控件实例上限”，不是节点总数
- 上限过小会增加反复 materialize / release 的频率

### `egui_view_virtual_stage_notify_data_changed`

作用：通知整个数据源发生了全量变化。

适合：

- 整体换页
- 批量重排
- 大量节点联动刷新

代价：

- 这是最重的一种刷新通知
- 没必要时不要把所有更新都退化成它

### `egui_view_virtual_stage_notify_node_changed`

作用：通知某个 `stable_id` 对应节点内容变化，但区域不变。

适合：

- 文本变化
- 状态变化
- render-only 节点重绘
- 已 materialize 节点重新 bind

建议：

- 业务层最好维护依赖节点集合
- 一次交互只刷新真正受影响的 `stable_id`

### `egui_view_virtual_stage_notify_node_bounds_changed`

作用：通知某个节点的区域或描述边界相关信息发生变化。

适合：

- 节点位置变化
- 节点尺寸变化
- `hidden` / `view_type` / `region` 变化

和 `notify_node_changed` 的区别：

- `notify_node_changed`：内容变了，区域通常不变
- `notify_node_bounds_changed`：布局边界变了，需要更新 old/new region

### `egui_view_virtual_stage_pin_node`

作用：强制让指定节点保留 live slot，不要被普通回收策略释放。

适合：

- 关键设备卡
- 需要持续刷新诊断内容的节点
- 需要长时间保留运行态的局部控件

返回值：

- 成功 pin 返回真
- 失败通常意味着 slot 预算不足或节点不支持 materialize

### `egui_view_virtual_stage_unpin_node`

作用：撤销 `pin_node` 的强制常驻。

调用后：

- 节点不再享受 pin 保活
- 是否立即释放，取决于当前是否仍被 focus / keepalive 占用

建议：

- 对成对 pin / unpin 的业务动作做明确封装
- 不要把 pin 当作长期默认行为滥用

### `egui_view_virtual_stage_get_slot_count`

作用：返回当前 `virtual_stage` 实际持有的 slot 数。

常见用途：

- 调试 slot 预算
- 统计当前活跃实例数量
- 在示例里估算 live SRAM 占用

### `egui_view_virtual_stage_get_slot`

作用：读取指定 slot 的运行时信息。

可拿到：

- `state`
- `view_type`
- `stable_id`
- `view`
- `render_region`

适合：

- 调试和单测
- 业务示例做可视化统计

不建议：

- 业务逻辑强依赖 slot 下标
- 把它当作稳定业务接口长期使用

## keepalive / pin / focus

`virtual_stage` 常见有三种“暂时不要释放”的机制。

### focus 保活

输入类控件只要仍在焦点态，就应继续占用 slot。

典型控件：

- `textinput`
- 正在编辑的复合控件

### adapter keepalive

通过 `should_keep_alive()` 告诉框架：当前节点还不能释放。

典型场景：

- `combobox` 展开
- 正在播放的控件动画
- 控件内部手势尚未结束

### pin 保活

通过 `pin_node()` / `unpin_node()` 由业务显式控制常驻。

典型场景：

- 关键设备卡
- 持续诊断卡
- 局部长驻操作面板

## 时序图

### 点击交互节点 -> materialize -> bind -> restore -> dispatch

```text
User
  |
  | tap interactive node
  v
virtual_stage
  |
  | hit_test / locate stable_id
  | choose or allocate slot
  | create_view(view_type)        [if slot has no reusable view]
  | bind_view(index, stable_id)
  | restore_state(...)            [if adapter implements it]
  | dispatch input event
  v
live view
```

### 输入框失焦 -> save_state -> release

```text
Focused textinput
  |
  | blur / clear focus
  v
virtual_stage
  |
  | check keepalive conditions
  | save_state(textinput text, cursor related business state)
  | unbind_view(...)              [optional]
  | recycle slot
  v
adapter context / business state
```

### 业务层 notify_node_changed -> cache refresh -> rebind / redraw

```text
Business logic
  |
  | update external state
  | notify_node_changed(stable_id)
  v
virtual_stage
  |
  | refresh cached desc for target node
  | if target is live slot -> mark needs_bind and rebind target slot
  | if target is render-only -> invalidate target region and redraw
  v
screen
```

## 最小接入步骤

### 1. 先拆业务状态

优先分成两层：

- 节点描述层：位置、类型、`z_order`、`stable_id`
- 业务数据层：文本、数值、选中项、展开态、运行态

### 2. 约定 `view_type` 和 `stable_id`

```c
enum
{
    MY_VIEW_TYPE_BUTTON = 1,
    MY_VIEW_TYPE_TEXTINPUT = 2,
    MY_VIEW_TYPE_COMBOBOX = 3,
};
```

`stable_id` 最好按区域或业务模块分段，便于做定向刷新。

### 3. 实现 adapter

```c
static const egui_view_virtual_stage_adapter_t my_adapter = {
        .get_count = my_get_count,
        .get_desc = my_get_desc,
        .create_view = my_create_view,
        .destroy_view = my_destroy_view,
        .bind_view = my_bind_view,
        .save_state = my_save_state,
        .restore_state = my_restore_state,
        .draw_node = my_draw_node,
        .hit_test = my_hit_test,
        .should_keep_alive = my_should_keep_alive,
};
```

### 4. 初始化 page

```c
static egui_view_virtual_stage_t page;
EGUI_VIEW_VIRTUAL_STAGE_PARAMS_INIT(page_params, 0, 0, 320, 240);

egui_view_virtual_stage_init_with_params(EGUI_VIEW_OF(&page), &page_params);
egui_view_virtual_stage_set_live_slot_limit(EGUI_VIEW_OF(&page), 3);
egui_view_virtual_stage_set_adapter(EGUI_VIEW_OF(&page), &my_adapter, &my_context);
egui_core_add_user_root_view(EGUI_VIEW_OF(&page));
```

### 5. 业务变化时显式通知

```c
my_ctx.slider_value = 68;
egui_view_virtual_stage_notify_node_changed(EGUI_VIEW_OF(&page), MY_NODE_SPEED_BAR_ID);
egui_view_virtual_stage_notify_node_changed(EGUI_VIEW_OF(&page), MY_NODE_SUMMARY_ID);
```

### 6. 需要长期活跃的节点做保活

```c
egui_view_virtual_stage_pin_node(EGUI_VIEW_OF(&page), MY_NODE_CORE_CARD_ID);
```

## 接入建议

### render-only 优先

如果节点不需要真实复杂交互，优先做成 render-only。

判断标准：

- 只是显示文本、数字、图标、趋势
- 交互只需要页面级点击
- 视觉复杂但状态简单

### 输入态外置

不要把输入类控件的关键值只留在实例里。

推荐外置：

- `textinput` 文本
- `slider` 数值
- `picker` 当前值
- `combobox` 当前索引
- `roller` 当前索引
- `segmented` 当前索引

### 动画态两种做法

如果节点有动画：

- 简单方案：把动画进度也外置，render-only 按业务时钟直接绘制
- 保守方案：动画期间通过 `should_keep_alive()` 保活，结束后再释放

### 命中区域复杂时实现 `hit_test`

默认命中通常就是矩形区域。

如果节点：

- 有镂空区
- 只有局部可点
- 需要更严格的热点控制

就实现 `adapter->hit_test()`。

## HelloBasic 示例

参考：

- `src/widget/egui_view_virtual_stage.h`
- `src/widget/egui_view_virtual_stage.c`
- `example/HelloBasic/virtual_stage/test.c`
- `example/HelloBasic/virtual_stage/demo_virtual_stage_internal.h`
- `example/HelloBasic/virtual_stage/demo_virtual_stage_adapter.c`
- `example/HelloUnitTest/test/test_virtual_stage.c`

当前 `HelloBasic/virtual_stage` 示例演示了：

- `800x800` 大画布
- `100` 个节点同时呈现
- 默认 render-only
- 交互节点按需 materialize
- `stable_id + save_state/restore_state`
- `pin + focus + should_keep_alive`
- `notify_node_changed / notify_node_bounds_changed`
- adapter/helper 拆分后的更易复用组织方式

## 验证建议

代码改动后建议至少执行：

```bash
make all APP=HelloBasic APP_SUB=virtual_stage PORT=pc
python scripts/code_runtime_check.py --app HelloBasic --app-sub virtual_stage --keep-screenshots
python scripts/hello_basic_render_workflow.py --widgets virtual_stage --skip-unit-tests
```

如果改了框架层，再执行：

```bash
make all APP=HelloUnitTest PORT=pc_test
output\main.exe
```

## 常见陷阱

- `stable_id` 不稳定，导致 pin 和状态恢复错位
- 节点 bounds 变了，却只调用了 `notify_node_changed`
- 输入框失焦后没有把文本写回业务层
- `combobox` 展开态没保活，导致点击下拉项时控件已被释放
- 所有联动都退化成 `notify_data_changed`，导致整页重绑

如果页面本身是“很多节点同时可见，但只有少量节点真正活跃”，优先考虑 `virtual_stage`，不要为每个节点都常驻一份控件实例。
