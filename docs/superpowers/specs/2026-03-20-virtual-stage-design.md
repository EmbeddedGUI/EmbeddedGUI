# virtual_stage 设计说明

## 目标

`virtual_stage` 的目标是让“节点很多、交互很少”的页面也能在小 SRAM 设备上成立。

核心诉求：

- 页面可以同时摆下 `N` 个节点
- 非交互节点不常驻控件实例
- 交互节点只在需要时创建
- 关键节点支持保活
- 控件离场后，状态能保存并恢复
- 单节点变化尽量做定向刷新，而不是整页重建

## 非目标

`virtual_stage` 不试图替代以下能力：

- 长列表滚动复用：这属于 `virtual_viewport`
- 自动布局系统：`virtual_stage` 更适合业务层明确给出绝对区域
- 所有动画都零成本保留：复杂动画仍要靠外部状态或 keepalive

## 问题背景

传统页面如果有 100 个控件，并且每个控件实例平均要 100B 以上 SRAM，那么仅控件实例就可能吃掉 10KB 量级内存。

对很多 MCU 项目来说，这个成本过高，尤其是：

- 页面节点多，但大部分只负责显示
- 页面交互是局部发生的
- 同一时刻真正活跃的控件只有 2~4 个

因此更合理的做法是：

- 页面维护节点描述和业务状态
- 需要交互时再把节点 materialize 成真实 view
- 交互结束后保存状态并释放 view

## 核心抽象

### 1. 节点描述

每个节点由 `egui_virtual_stage_node_desc_t` 描述：

- `region`：节点在 page 内的位置
- `stable_id`：节点稳定身份
- `view_type`：需要时创建哪种 view
- `z_order`：重叠节点的顺序
- `flags`：交互、隐藏、默认保活等

其中：

- `view_type = NONE` 表示纯 render-only
- `stable_id` 是整个方案的锚点，所有 pin、恢复、定向刷新都依赖它

### 2. Adapter

Adapter 是 `virtual_stage` 和业务数据之间的桥梁。

框架只负责：

- slot 管理
- 事件命中
- 生命周期切换
- 状态回调时机

业务层负责：

- 节点数据
- 真实控件创建与绑定
- render-only 绘制
- 状态保存与恢复

### 3. Slot 池

框架内部使用固定大小 slot 池，而不是按节点数分配 view。

slot 的价值在于：

- 将“页面节点总数”与“控件实例总数”解耦
- 把 SRAM 上限收敛到 `live_slot_limit`
- 允许控件在不同节点之间复用

## 生命周期设计

### render-only 节点

流程：

1. `get_desc()` 返回节点
2. `view_type = NONE`
3. `draw_node()` 直接绘制
4. 不进入 slot 池

收益：

- 不占 view 实例
- 不需要 bind/unbind
- 最适合轻量摘要、图标、状态卡、缩略图表

### interactive 节点

流程：

1. 命中节点
2. 分配或复用 slot
3. `create_view()` / `bind_view()`
4. `restore_state()`
5. 派发事件
6. 不需要时 `save_state()` 并释放

这样就把控件实例的生存期缩短到“真正活跃的时间窗口”。

## 状态保留设计

### 为什么必须外置状态

因为 view 可能被释放，所以不能把关键状态只放在 view 内部。

建议外置的状态包括：

- 文本框文本
- 光标相关的可恢复业务值
- switch / checkbox / radio 的选中态
- slider / picker 数值
- combobox / roller / segmented 的当前索引
- 业务动画的进度或开始时间

### save / restore 的角色分工

- `save_state()`：view 即将离场时，把内部状态写回业务层
- `restore_state()`：view 重新 materialize 时，从业务层还原状态

如果业务层已经是单一真源，`bind_view()` 也可以承担一部分恢复工作，但对于 textinput 这类控件，显式 `save/restore` 更稳妥。

## keepalive 设计

`virtual_stage` 并不是“所有控件都立即释放”，而是允许少量节点暂时保活。

三类常见来源：

### 1. Focus

聚焦中的输入控件不能随意回收，否则输入体验会断裂。

### 2. Pin

通过 `pin_node()` 把关键节点提升为常驻 slot。

适合：

- 关键设备卡
- 正在监控的局部窗口
- 必须持续保持实例身份的节点

### 3. Adapter keepalive

通过 `should_keep_alive()` 控制临时保活。

典型例子：

- combobox 展开态
- 正在运行中的动画态
- 正在处理手势或内部菜单态

## 定向刷新设计

这是 `virtual_stage` 能真正落地的关键点之一。

### `notify_data_changed`

用于整体失效。

特点：

- 简单
- 代价最大
- 容易退化成整页重绑

### `notify_node_changed`

用于“内容变了，但 bounds 没变”。

适合：

- 文案变化
- 颜色/数值变化
- render-only 节点重绘
- live slot 重新 bind

### `notify_node_bounds_changed`

用于“节点矩形或可见性变了”。

适合：

- 节点尺寸变化
- 节点位置变化
- hidden 切换
- view_type 切换

设计原则：

- 联动关系由业务层掌握
- 框架只负责把稳定 ID 映射到目标 slot / 目标 cache
- 已存在 live slot 时优先局部重绑，而不是重绑全部 slot

## 命中测试设计

默认情况下，节点命中使用矩形区域。

但大画布页面经常出现：

- 标题栏不可点、内容区可点
- 镂空卡片
- 小热点按钮

因此提供 `hit_test()` 给业务层精确控制。

这让 render-only 节点也能拥有“业务级交互区域”，而不必一定创建真实控件。

## 内存收益模型

粗略估算：

- 传统方案：`节点数 * 平均控件实例大小`
- `virtual_stage`：`live_slot_limit * 平均控件实例大小 + 业务状态 + 节点描述`

当页面节点数远大于活跃交互节点数时，收益最明显。

例如：

- 页面 100 节点
- 实时活跃 slot 仅 3~4 个
- 大部分节点 render-only

那么实例 SRAM 从“与 100 成正比”变成“与 4 成正比”。

## 约束与边界

- 当前最大 slot 数由 `EGUI_VIEW_VIRTUAL_STAGE_MAX_SLOTS` 限制
- 页面更适合绝对定位，而不是复杂自动布局
- `stable_id` 必须稳定
- 复杂动画若不外置状态，就必须保活
- 交互路径要显式处理失焦和释放，否则会出现 slot 滞留

## 当前示例落地方式

`example/HelloBasic/virtual_stage/test.c` 当前示例采用了下面这套策略：

- `800x800` 大画布
- `100` 节点同时可见
- 默认 render-only
- 交互节点按需 materialize
- 输入值、选项值、业务态全部外置
- `pin + focus + should_keep_alive` 共同控制保活
- 业务联动优先走 `notify_node_changed`

示例里已经覆盖：

- textinput 的保存/恢复
- combobox 展开态保活
- pinned 设备卡常驻
- render-only 复杂控件绘制
- 单节点定向刷新

## 验证策略

### 单测

`example/HelloUnitTest/test/test_virtual_stage.c` 当前重点覆盖：

- render-only 节点不占 slot
- 点击后 materialize / 释放
- pin / unpin
- 双 pin 引用计数
- textinput save / restore
- keepalive 生命周期
- `notify_node_changed` 仅刷新目标 slot
- `notify_node_bounds_changed` 仅更新目标 region
- hidden 节点释放
- focus 抢占与恢复
- 同类型 slot 驱逐策略

### 运行时验证

`HelloBasic/virtual_stage` 通过录制动作验证：

- 交互链路是否完整
- slot 是否被错误滞留
- 文本状态是否恢复
- pin 节点是否常驻
- reset 后状态是否全部归零

## 后续演进方向

- 增加更细粒度的局部绘制优化
- 沉淀一套更通用的 adapter 模板
- 给 `virtual_stage` 提供更标准的接入文档和示例拆分
- 补更多“render-only + 少量 live widget”业务页面模板
