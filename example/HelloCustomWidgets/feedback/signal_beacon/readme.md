# SignalBeacon 控件设计说明

## 1. 为什么需要这个控件

`signal_beacon` 用于表达链路健康、告警波段、继电器锁定这类“以脉冲和状态级别为中心”的反馈场景。

适合场景：

- 工业控制台里的链路健康监控。
- 机房、安防、边缘网关的信号中继状态。
- 需要在 240x320 小屏里快速看懂“正常 / 观察 / 告警”层级的反馈面板。

## 2. 为什么现有控件不够用

- `alert_banner` 更像横向消息条，强在线性阅读，不适合表达中心信标和节点联动。
- `notification_stack` 强调卡片堆叠，不适合表达脉冲级别和环形核心状态。
- `progress_bar` 只能表达单轴进度，无法同时承载核心节点、侧节点和告警胶囊。

`signal_beacon` 的差异化边界：

1. 中心信标以环形脉冲表达强弱级别。
2. 左右节点和核心节点共同构成链路语义。
3. 同时带有标题、状态胶囊、底部摘要，适合反馈类状态总览。

## 3. 目标场景与示例概览

目录：

- `example/HelloCustomWidgets/feedback/signal_beacon/`

本次示例包含：

- `beacon_primary`：主链路信标面板。
- `beacon_field`：现场中继 compact 预览。
- `beacon_lock`：锁定中继 compact 预览。

## 4. 视觉与布局规格

- 屏幕基准：240 x 320。
- 顶部标题区：220 x 18。
- 主信标面板：192 x 116。
- 底部双卡区：210 x 86。
- 主面板使用冷蓝色链路反馈，底部两卡分别使用绿色和琥珀色分组。
- 验收时重点检查：环形信标是否光学居中，左右节点是否平衡，标题 / 胶囊 / 底部文字是否贴边。

## 5. 控件清单

| 变量名 | 类型 | 尺寸 | 初始状态 | 用途 |
| --- | --- | --- | --- | --- |
| root_layout | egui_view_linearlayout_t | 220 x 306 | enabled | 页面总容器 |
| title_label | egui_view_label_t | 220 x 18 | 可见 | 页面标题 |
| guide_label | egui_view_label_t | 220 x 11 | 可见 | 交互说明 |
| beacon_primary | egui_view_signal_beacon_t | 192 x 116 | snapshot 0 | 主信标面板 |
| status_label | egui_view_label_t | 220 x 12 | 可见 | 当前交互反馈 |
| section_divider | egui_view_line_t | 148 x 2 | 可见 | 分隔线 |
| bottom_row | egui_view_linearlayout_t | 210 x 86 | enabled | 底部容器 |
| beacon_field | egui_view_signal_beacon_t | 108 x 80 | snapshot 0 | 现场中继卡 |
| beacon_lock | egui_view_signal_beacon_t | 100 x 80 | snapshot 0 | 锁定中继卡 |

## 6. 状态覆盖矩阵

| 状态 | 覆盖方式 | 预期结果 |
| --- | --- | --- |
| 默认态 | 初始渲染 | 主信标和底部两卡完整可见 |
| 主信标切换 | 点击 `beacon_primary` | 核心脉冲级别、状态胶囊、底部摘要变化清楚 |
| 现场卡切换 | 点击 `beacon_field` | 绿色 compact 卡状态变化可见 |
| 锁定卡切换 | 点击 `beacon_lock` | 琥珀色 compact 卡状态变化可见 |
| 锁定态 | `locked_mode` 开启 | 信标核心亮度收敛但仍可辨认 |

## 7. egui_port_get_recording_action() 录制动作设计

| 序号 | 动作 | 目标 | 预期 |
| ---: | --- | --- | --- |
| 0 | WAIT | 首帧稳定 | 记录默认态 |
| 1 | CLICK | `beacon_primary` | 切主信标 |
| 2 | WAIT | 稳定 | 记录主信标变化 |
| 3 | CLICK | `beacon_field` | 切现场卡 |
| 4 | WAIT | 稳定 | 记录现场卡变化 |
| 5 | CLICK | `beacon_lock` | 切锁定卡 |
| 6 | WAIT | 稳定 | 记录尾帧 |

## 8. 编译、runtime、截图验收标准

命令：

- `make all APP=HelloCustomWidgets APP_SUB=feedback/signal_beacon PORT=pc`
- `python scripts/code_runtime_check.py --app HelloCustomWidgets --app-sub feedback/signal_beacon --timeout 10 --keep-screenshots`

通过条件：

1. 编译通过。
2. runtime 通过。
3. 核心信标、左右节点、底部脉冲条必须完整可见并保持视觉居中。
4. 标题、胶囊、底部摘要文字与边框之间必须保留合理空隙。
5. 截图同步整理到 `iteration_log/`。

## 9. 已知限制与收口结果

已知限制：

- 当前版本先做静态脉冲环，不做连续动画。
- 当前版本状态胶囊仍使用固定宽度，不做文本自适应。
- 当前版本先覆盖 3 组 snapshot，不接动态数据源。

本轮收口结果：

1. 已完成 30 轮基于 runtime 截图的递归迭代。
2. 主卡、状态文案、divider、底部双卡的中心轴与留白关系已收稳。
3. 后续如升级到框架级公共控件，再单独评估脉冲动画和动态文本宽度适配。

## 10. 与现有控件的重叠分析与差异化边界

- 不等同于 `alert_banner`：`signal_beacon` 不是横向文本告警条，而是中心信标结构。
- 不等同于 `notification_stack`：它不依赖卡片堆叠，而是核心节点 + 环形脉冲反馈。
- 不等同于 `progress_bar`：它不是单轴进度，而是多节点信号层级。
- 不等同于 `node_topology`：它不表达网状拓扑，而是聚焦单链路信标状态。
