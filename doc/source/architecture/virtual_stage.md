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

推荐阅读顺序：

1. 先看 `example/HelloVirtual/virtual_stage_basic/`，这是推荐入门 case，聚焦最小接法和 helper API。
2. 再看 `example/HelloVirtual/virtual_stage_showcase/`，这是 showcase 风格的对比 case，适合和 `HelloShowcase` 并排理解“render-only panel + 真实 widget”怎么混合组织。
3. 最后看 `example/HelloVirtual/virtual_stage/`，这是复杂 cockpit 场景，覆盖更多节点类型、keepalive 和状态恢复。
4. 回到本文，对照 `adapter`、slot、pin、save/restore 的设计细节。

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

如果节点集合本身就是固定数组，推荐直接用 array bridge：

- `EGUI_VIEW_VIRTUAL_STAGE_DESC_ARRAY_SOURCE_INIT(...)`
- `EGUI_VIEW_VIRTUAL_STAGE_NODE_ARRAY_SOURCE_INIT(...)`
- `EGUI_VIEW_VIRTUAL_STAGE_ARRAY_OPS_SIMPLE_INIT(...)`
- `EGUI_VIEW_VIRTUAL_STAGE_ARRAY_OPS_SIMPLE_CONST_INIT(...)`
- `EGUI_VIEW_VIRTUAL_STAGE_ARRAY_OPS_INTERACTIVE_INIT(...)`
- `EGUI_VIEW_VIRTUAL_STAGE_ARRAY_OPS_INTERACTIVE_CONST_INIT(...)`
- `EGUI_VIEW_VIRTUAL_STAGE_ARRAY_OPS_STATEFUL_INIT(...)`
- `EGUI_VIEW_VIRTUAL_STAGE_ARRAY_OPS_STATEFUL_CONST_INIT(...)`
- `EGUI_VIEW_VIRTUAL_STAGE_NODE_ARRAY_SIMPLE_BRIDGE_INIT_WITH_LIMIT(...)`
- `EGUI_VIEW_VIRTUAL_STAGE_NODE_ARRAY_INTERACTIVE_BRIDGE_INIT_WITH_LIMIT(...)`
- `EGUI_VIEW_VIRTUAL_STAGE_NODE_ARRAY_STATEFUL_BRIDGE_INIT_WITH_LIMIT(...)`
- `EGUI_VIEW_VIRTUAL_STAGE_DESC_ARRAY_BRIDGE_INIT(...)`
- `EGUI_VIEW_VIRTUAL_STAGE_NODE_ARRAY_BRIDGE_INIT(...)`
- `EGUI_VIEW_VIRTUAL_STAGE_ARRAY_BRIDGE_INIT(...)`
- `EGUI_VIEW_VIRTUAL_STAGE_NODE_ARRAY_BRIDGE_INIT_WITH_LIMIT(...)`
- `EGUI_VIEW_VIRTUAL_STAGE_NODE_ARRAY_SCREEN_BRIDGE_INIT_WITH_LIMIT(...)`
- `EGUI_VIEW_VIRTUAL_STAGE_AS_VIEW(...)`
- `EGUI_VIEW_VIRTUAL_STAGE_INIT_ARRAY_BRIDGE(...)`
- `EGUI_VIEW_VIRTUAL_STAGE_ADD_ROOT(...)`
- `EGUI_VIEW_VIRTUAL_STAGE_SET_BACKGROUND(...)`
- `EGUI_VIEW_VIRTUAL_STAGE_REQUEST_LAYOUT(...)`
- `EGUI_VIEW_VIRTUAL_STAGE_NOTIFY_NODE(...)`
- `EGUI_VIEW_VIRTUAL_STAGE_NOTIFY_IDS(...)`
- `EGUI_VIEW_VIRTUAL_STAGE_NOTIFY_NODES(...)`
- `EGUI_VIEW_VIRTUAL_STAGE_NOTIFY_BOUNDS_IDS(...)`
- `EGUI_VIEW_VIRTUAL_STAGE_TOGGLE_PIN(...)`
- `EGUI_VIEW_VIRTUAL_STAGE_SLOT_COUNT(...)`
- `egui_view_virtual_stage_init_with_array_bridge(...)`

这样可以省掉重复的 `get_count / get_desc` 样板，也不用再手工拼 `params + node_source + adapter + setup`，只保留 `create / bind / draw / hit_test / should_keep_alive` 这些真正和业务有关的部分。

第一次接入时可以先按这条最小判断来写：

- 必需：`create_view`、`bind_view`
- render-only 节点推荐实现：`draw_node`
- 需要跨回收恢复状态时再补：`save_state`、`restore_state`
- 需要展开态/焦点态短时保活时再补：`should_keep_alive`
- 命中区域不是普通矩形时再补：`hit_test`

缺省行为也可以先记住：

- 不写 `hit_test` 时，默认按节点矩形区域命中
- 不写 `should_keep_alive` 时，默认不额外保活
- 不写 `destroy_view` / `unbind_view` / `save_state` / `restore_state` 时，框架不会替你补业务状态逻辑
- 如果 stage 就是整屏页面，优先用 `NODE_ARRAY_SCREEN_*_BRIDGE_INIT_WITH_LIMIT(...)`，少写一组屏幕尺寸参数

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
- 如果在一次交互尚未结束时降低上限，capture 结束后也会立即按新上限重新裁剪 slot

### `egui_view_virtual_stage_notify_data_changed`

作用：通知整个数据源发生了全量变化。

适合：

- 整体换页
- 批量重排
- 大量节点联动刷新

代价：

- 这是最重的一种刷新通知
- 没必要时不要把所有更新都退化成它
- 如果一次 capture 尚未结束就通过它把当前节点移除或替换掉，`virtual_stage` 会先向旧 view 发送 `cancel`，再回收或复用 slot，避免 pooled view 残留 pressed 等临时交互状态

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

### 新增便捷 helper

为了让 `virtual_stage` 更适合真实业务接入，当前还补了几组高频 helper：

- `EGUI_VIEW_VIRTUAL_STAGE_NOTIFY_NODES(...)`
- `EGUI_VIEW_VIRTUAL_STAGE_NOTIFY_IDS(...)`
- `EGUI_VIEW_VIRTUAL_STAGE_NOTIFY_NODE_BOUNDS(...)`
- `EGUI_VIEW_VIRTUAL_STAGE_NOTIFY_BOUNDS_IDS(...)`
- `EGUI_VIEW_VIRTUAL_STAGE_NOTIFY_NODES_BOUNDS(...)`
- `EGUI_VIEW_VIRTUAL_STAGE_ARRAY_SETUP_INIT(...)`
- `egui_view_virtual_stage_array_adapter_init()`
- `egui_view_virtual_stage_apply_setup()` / `egui_view_virtual_stage_init_with_setup()`
- `egui_view_virtual_stage_apply_array_setup()` / `egui_view_virtual_stage_init_with_array_setup()`
- `EGUI_VIEW_VIRTUAL_STAGE_APPLY_ARRAY_BRIDGE(...)` / `EGUI_VIEW_VIRTUAL_STAGE_INIT_ARRAY_BRIDGE(...)`
- `egui_view_virtual_stage_is_node_pinned()`
- `EGUI_VIEW_VIRTUAL_STAGE_TOGGLE_PIN(...)`
- `egui_view_virtual_stage_find_slot_by_stable_id()`
- `egui_view_virtual_stage_find_view_by_stable_id()`
- `egui_view_virtual_stage_resolve_node_by_view()`
- `EGUI_VIEW_VIRTUAL_STAGE_RESOLVE_ID_BY_VIEW(...)`
- `egui_view_virtual_stage_notify_nodes_changed()` / `egui_view_virtual_stage_notify_nodes_bounds_changed()`

推荐用法：

- 初始化阶段优先使用 `init_with_setup()`，一次性收口 `params + adapter + context`
- 固定节点数组且回调就在当前文件时，优先直接用一步式 `NODE_ARRAY_*_BRIDGE_INIT_WITH_LIMIT(...) + INIT_ARRAY_BRIDGE(...)`；如果 stage 是整屏页，进一步用 `NODE_ARRAY_SCREEN_*_BRIDGE_INIT_WITH_LIMIT(...)`；如果 `ops` 需要跨文件导出、复用，或者你已经把 `params` / `node_source` 单独拆出来了，再回退到 `ARRAY_OPS_* + NODE_ARRAY_BRIDGE_INIT* + INIT_ARRAY_BRIDGE(...)`
- `array ops` 分层 helper 继续保留给复用场景：只需要 `create/bind/draw` 用 `ARRAY_OPS_SIMPLE_INIT(...)`；还要 `hit_test + should_keep_alive` 用 `ARRAY_OPS_INTERACTIVE_INIT(...)`；还要 `save_state + restore_state` 时再用 `ARRAY_OPS_STATEFUL_INIT(...)`；如果 `ops` 需要跨文件导出，改用对应的 `*_CONST_INIT(...)`
- 如果你手上拿的是 `egui_view_virtual_stage_t *`，优先用 bridge 头里的 typed convenience 宏，例如 `INIT_ARRAY_BRIDGE(...)`、`ADD_ROOT(...)`、`SET_BACKGROUND(...)`、`REQUEST_LAYOUT(...)`、`NOTIFY_NODE(...)`、`NOTIFY_IDS(...)`、`NOTIFY_BOUNDS_IDS(...)`、`PIN_IDS(...)`、`UNPIN_IDS(...)`、`TOGGLE_PIN(...)`、`SLOT_COUNT(...)`，继续少写一层 `EGUI_VIEW_OF(...)`
- 事件回调里如果只需要业务身份，优先使用 `RESOLVE_ID_BY_VIEW(...)`；需要 index 和 desc 时再用 `RESOLVE_NODE_BY_VIEW(...)`
- 调试和摘要面板里优先使用 `find_slot_by_stable_id()` / `find_view_by_stable_id()` 观察 live slot
- 一次联动少量 `stable_id` 时优先使用 `NOTIFY_IDS(...)` / `NOTIFY_BOUNDS_IDS(...)` 这类 inline helper；如果 `stable_id` 数组需要复用，再继续使用 `NOTIFY_NODES(...)` / `NOTIFY_NODES_BOUNDS(...)`
- 业务 pin 状态判断或开关按钮直接用 `IS_PINNED(...)` / `TOGGLE_PIN(...)`，不要重复维护一份“框架侧是否已 pin”的影子状态

### 固定节点数组场景的简化接法

如果 stage 里的节点本身就是一组固定数组，推荐直接用下面这条接法：

```c
typedef struct app_stage_node
{
    egui_virtual_stage_node_desc_t desc;
    uint8_t business_state;
} app_stage_node_t;

static egui_view_virtual_stage_t stage_view;
static app_stage_context_t stage_ctx;
static app_stage_node_t stage_nodes[12];

EGUI_VIEW_VIRTUAL_STAGE_NODE_ARRAY_INTERACTIVE_BRIDGE_INIT_WITH_LIMIT(stage_bridge, 8, 80, 304, 200, 3, stage_nodes, app_stage_node_t, desc,
                                                                      app_stage_create_view, app_stage_destroy_view, app_stage_bind_view,
                                                                      app_stage_draw_node, app_stage_hit_test, app_stage_should_keep_alive, &stage_ctx);

EGUI_VIEW_VIRTUAL_STAGE_INIT_ARRAY_BRIDGE(&stage_view, &stage_bridge);
EGUI_VIEW_VIRTUAL_STAGE_ADD_ROOT(&stage_view);
```

如果你只是在固定节点数组上接一层真实控件，这里通常直接记住三档 `ops` helper 就够了：

- `EGUI_VIEW_VIRTUAL_STAGE_ARRAY_OPS_SIMPLE_INIT(...)`：只有 `create_view / destroy_view / bind_view / draw_node`
- `EGUI_VIEW_VIRTUAL_STAGE_ARRAY_OPS_INTERACTIVE_INIT(...)`：再加 `hit_test / should_keep_alive`
- `EGUI_VIEW_VIRTUAL_STAGE_ARRAY_OPS_STATEFUL_INIT(...)`：再加 `save_state / restore_state`
- 上面三档如果要跨文件导出 `const ops`，分别改用 `..._SIMPLE_CONST_INIT(...)`、`..._INTERACTIVE_CONST_INIT(...)`、`..._STATEFUL_CONST_INIT(...)`

只有当你还要自定义 `unbind_view` 或其他更少见的组合时，再回退到手写 `egui_view_virtual_stage_array_ops_t`。

这条路适合：

- 节点总数固定，或者按场景初始化后基本不变
- 你已经有一组本地节点数组
- 你只想关心节点描述、真实控件创建和业务绑定

### 三个可直接照抄的模板

固定矩形 stage：

```c
EGUI_VIEW_VIRTUAL_STAGE_NODE_ARRAY_INTERACTIVE_BRIDGE_INIT_WITH_LIMIT(stage_bridge, 8, 80, 304, 200, 3, stage_nodes, app_stage_node_t, desc,
                                                                      app_stage_create_view, app_stage_destroy_view, app_stage_bind_view,
                                                                      app_stage_draw_node, app_stage_hit_test, app_stage_should_keep_alive, &stage_ctx);

EGUI_VIEW_VIRTUAL_STAGE_INIT_ARRAY_BRIDGE(&stage_view, &stage_bridge);
EGUI_VIEW_VIRTUAL_STAGE_ADD_ROOT(&stage_view);
```

整屏 stage：

```c
EGUI_VIEW_VIRTUAL_STAGE_NODE_ARRAY_SCREEN_STATEFUL_BRIDGE_INIT_WITH_LIMIT(stage_bridge, 2, stage_nodes, app_stage_node_t, desc, app_stage_create_view,
                                                                          app_stage_destroy_view, app_stage_bind_view, app_stage_save_state,
                                                                          app_stage_restore_state, app_stage_draw_node, app_stage_hit_test,
                                                                          app_stage_should_keep_alive, &stage_ctx);

EGUI_VIEW_VIRTUAL_STAGE_INIT_ARRAY_BRIDGE(&stage_view, &stage_bridge);
EGUI_VIEW_VIRTUAL_STAGE_ADD_ROOT(&stage_view);
```

已有 `params`：

```c
EGUI_VIEW_VIRTUAL_STAGE_PARAMS_INIT_WITH_LIMIT(stage_params, 8, 80, 304, 200, 3);
EGUI_VIEW_VIRTUAL_STAGE_ARRAY_OPS_SIMPLE_INIT(stage_ops, app_stage_create_view, app_stage_destroy_view, app_stage_bind_view, app_stage_draw_node);

EGUI_VIEW_VIRTUAL_STAGE_NODE_ARRAY_BRIDGE_INIT(stage_bridge, &stage_params, stage_nodes, app_stage_node_t, desc, &stage_ops, &stage_ctx);

EGUI_VIEW_VIRTUAL_STAGE_INIT_ARRAY_BRIDGE(&stage_view, &stage_bridge);
EGUI_VIEW_VIRTUAL_STAGE_ADD_ROOT(&stage_view);
```

如果节点数量会频繁变化，或者节点描述来自外部动态模型，再继续使用 raw adapter 即可。

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

通过 `PIN(...)` / `UNPIN(...)` 由业务显式控制常驻。

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

### 3. 动态数据源时实现 raw adapter

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
EGUI_VIEW_VIRTUAL_STAGE_NODE_ARRAY_BRIDGE_INIT_WITH_LIMIT(page_bridge, 0, 0, 320, 240, 3, stage_nodes, app_stage_node_t, desc, &stage_ops, &my_context);

EGUI_VIEW_VIRTUAL_STAGE_INIT_ARRAY_BRIDGE(&page, &page_bridge);
EGUI_VIEW_VIRTUAL_STAGE_ADD_ROOT(&page);
```

### 5. 业务变化时显式通知

```c
const uint32_t changed_ids[] = {
        MY_NODE_SPEED_BAR_ID,
        MY_NODE_SUMMARY_ID,
};

my_ctx.slider_value = 68;
EGUI_VIEW_VIRTUAL_STAGE_NOTIFY_NODES(&page, changed_ids);
```

如果只是一次性联动少量 id，也可以直接写成：

```c
EGUI_VIEW_VIRTUAL_STAGE_NOTIFY_IDS(&page, MY_NODE_SPEED_BAR_ID, MY_NODE_SUMMARY_ID);
```

### 6. 需要长期活跃的节点做保活

```c
EGUI_VIEW_VIRTUAL_STAGE_TOGGLE_PIN(&page, MY_NODE_CORE_CARD_ID);
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

`virtual_stage` 在 `DOWN` 命中节点后，后续 `MOVE/UP` 也会继续按 `hit_test()` 判断“当前是否仍在热点内”。
这意味着拖出热点会像普通控件那样取消按压，拖回热点后又能恢复。

## HelloVirtual 示例

参考：

- `src/widget/egui_view_virtual_stage.h`
- `src/widget/egui_view_virtual_stage.c`
- `example/HelloVirtual/virtual_stage_basic/test.c`
- `example/HelloVirtual/virtual_stage_showcase/test.c`
- `example/HelloVirtual/virtual_stage/test.c`
- `example/HelloVirtual/virtual_stage/demo_virtual_stage_internal.h`
- `example/HelloVirtual/virtual_stage/demo_virtual_stage_adapter.c`
- `example/HelloUnitTest/test/test_virtual_stage.c`

推荐先看 `HelloVirtual/virtual_stage_basic`，它演示的是：

- 固定页面上的最小节点集
- `Image + ProgressBar + Button + Combobox` 四类基础 live widget
- 固定节点数组 + `NODE_ARRAY_BRIDGE_INIT_WITH_LIMIT(...)`
- 外置业务状态
- `INIT_ARRAY_BRIDGE + RESOLVE_ID_BY_VIEW + TOGGLE_PIN + batch notify`
- 用 `find_slot_by_stable_id()` / `find_view_by_stable_id()` 做运行态观测

然后看 `HelloVirtual/virtual_stage_showcase`，它演示的是：

- 更接近 `HelloShowcase` 的深色分区布局
- panel / summary / badge 继续走 render-only
- 8 个代表性控件直接使用真实 widget，而不是画布重画
- 当前 slot ceiling 为 `8` 时，如何做一个便于对比的 stage 页面
- render-only 区域的动画由外部 tick 重绘驱动，所以观感可能比 `HelloShowcase` 更“静”；如果要保留原生控件自驱动画，就要把对应控件也提升为 live widget

### `HelloShowcase` vs `HelloVirtual/virtual_stage_showcase`

如果你关心的是“显示效果不变，但 SRAM 能省多少”，这两个例程就是最直接的对照组。

这里不建议直接用整个 ELF 的 `data + bss` 做对比，因为那会混入：

- `PFB`
- 平台移植层自身 SRAM
- 框架公共静态 SRAM

更适合衡量 `virtual_stage` 收益的口径，是“示例自身对象文件的 `.data* + .bss*`”。

对应目录分别是：

- `output/obj/HelloShowcase_stm32g0/example/HelloShowcase/`
- `output/obj/HelloVirtual_virtual_stage_showcase_stm32g0/example/HelloVirtual/virtual_stage_showcase/`

以 `PORT=stm32g0`、`COMPILE_OPT_LEVEL=-Os` 为例：

| 示例 | 示例自身静态 SRAM |
| --- | ---: |
| `HelloShowcase` | 13248 B |
| `HelloVirtual/virtual_stage_showcase` | 7128 B |

可以直接得到这组 tradeoff：

- `HelloVirtual/virtual_stage_showcase` 比 `HelloShowcase` 少占 `6120` 字节示例自身静态 SRAM，约 `46.2%`
- 收益来自 `virtual_stage` 不再常驻整页控件树，而是保留 `stage + scratch + 外置状态 + 少量 live slot`
- `HelloShowcase` 的主要静态 SRAM 消耗集中在整页真实控件，例如 `wg_keyboard`、`wg_list`、`wg_table`
- `virtual_stage_showcase` 的主要静态 SRAM 消耗集中在 `showcase_keyboard_view`、`showcase_scratch`、`showcase_stage_view`、`showcase_ctx`
- 这个口径不包含运行时栈/堆，所以它衡量的是“示例自身静态占用”，不是整机运行时总 SRAM 峰值

### QEMU 实测 heap

如果你还关心“运行时 heap 会不会涨”，要单独看 QEMU 实测。

推荐命令：

```bash
python scripts/perf_analysis/compare_virtual_showcase_heap_qemu.py --mode app-recording
```

`app-recording` 模式会直接复用各自示例的 `egui_port_get_recording_action()`，比统一假动作更接近真实页面交互。

以 `2026-03-22` 的 QEMU 实测结果为例：

| 示例 | idle current heap | interaction total peak heap |
| --- | ---: | ---: |
| `HelloShowcase` | 0 B | 0 B |
| `HelloVirtual/virtual_stage_showcase` | 2488 B | 3328 B |

这组数据和前面的静态 SRAM 结论并不矛盾：

- `HelloShowcase` 主要是整页静态全局控件，页面自身几乎不走运行时 `malloc`
- `virtual_stage_showcase` 省下的是“整页真实控件树的静态 SRAM”
- 同时它会引入一部分“按需 materialize”的运行时 heap

为什么 `virtual_stage_showcase` 会有这部分 heap：

- stage 节点缓存本身会分配 heap：`node_cache` 和 `draw_order` 在 `egui_view_virtual_stage_reload_cache()` 中通过 `egui_malloc()` 创建
- 正常模式下页面默认会保留少量 live widget，尤其 `List` 节点带 `KEEPALIVE`，所以 idle 阶段就会出现少量常驻 heap
- 交互过程中，`Button`、`TextInput`、`Slider`、`Combobox`、`List` 等真实控件会在 `showcase_adapter_create_view()` 里按 `view_type` 动态 `egui_malloc()`，释放后再复用或销毁
- 当前实测里 `virtual_stage_showcase` 的 idle 阶段有 `3` 次分配，交互阶段额外出现 `24` 次分配和 `24` 次释放，说明 heap 大头来自“少量常驻 slot + 交互时反复 materialize live widget”

怎么判断这 `3.25 KB` heap 值不值：

- 如果目标板最紧的是静态 SRAM，这个 tradeoff 是成立的，因为它换回了约 `46.2%` 的示例自身静态 SRAM
- 如果目标板对 heap 更敏感，那就要继续压 live widget 数量和 stage 运行时元数据

后续优化方向：

- 继续减少默认 keepalive 节点，优先检查 `List`、展开态 `Combobox` 这类会长期占 slot 的控件
- 严格控制 `live_slot_limit`，只给真正需要并存的交互控件留预算
- 能做成 render-only 的节点不要升级成 live widget，尤其是纯展示卡片、说明文字、装饰层
- 把常用 view_type 改成更轻的专用 live view，而不是直接保留较重控件的完整实例
- 如果目标是“heap 可控”而不是“heap 绝对最小”，可以把 stage cache / pin 表 / live view 改成固定池或专用 arena，避免碎片并锁定峰值
- 对于重量级控件，优先评估“展开时临时创建，收起后立即释放”，不要长期 keepalive

因此可以这样选：

- 想要最低接入成本、最直接复用整页控件树：优先 `HelloShowcase`
- 想保住 `HelloShowcase` 的视觉和交互，但目标板 SRAM 更紧：优先 `HelloVirtual/virtual_stage_showcase`

最后再看 `HelloVirtual/virtual_stage`，它演示了：

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
make all APP=HelloVirtual APP_SUB=virtual_stage_basic PORT=pc
python scripts/code_runtime_check.py --app HelloVirtual --keep-screenshots
python scripts/code_runtime_check.py --app HelloVirtual --app-sub virtual_stage_basic --keep-screenshots
python scripts/hello_basic_render_workflow.py --app HelloVirtual --widgets virtual_stage_basic --skip-unit-tests --bits64

make all APP=HelloVirtual APP_SUB=virtual_stage_showcase PORT=pc
python scripts/code_runtime_check.py --app HelloVirtual --app-sub virtual_stage_showcase --keep-screenshots
python scripts/showcase_stage_parity_check.py --timeout 35 --bits64
python scripts/hello_basic_render_workflow.py --app HelloVirtual --widgets virtual_stage_showcase --skip-unit-tests --bits64

make -j1 all APP=HelloShowcase PORT=stm32g0
make -j1 all APP=HelloVirtual APP_SUB=virtual_stage_showcase PORT=stm32g0
python scripts/perf_analysis/compare_virtual_showcase_ram.py --skip-build
python scripts/perf_analysis/compare_virtual_showcase_heap_qemu.py --mode app-recording

make all APP=HelloVirtual APP_SUB=virtual_stage PORT=pc
python scripts/code_runtime_check.py --app HelloVirtual --app-sub virtual_stage --keep-screenshots
python scripts/hello_basic_render_workflow.py --app HelloVirtual --widgets virtual_stage,virtual_stage_showcase,virtual_stage_basic --skip-unit-tests --bits64
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
