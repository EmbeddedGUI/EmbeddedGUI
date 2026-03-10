# SplitFlapBoard 控件设计说明

## 1. 为什么需要这个控件

split_flap_board 用于在小屏里表达“站牌 / 班次 / 柜台 / 队列”这类离散编号信息。它的重点不是连续数据，而是强编号感、强状态感和机械翻牌风格。

适合场景：

- 机场、车站、码头的小型出发牌。
- 工业产线工位号、队列号、班次号。
- 需要在 240x320 里用强视觉模块快速确认编号的状态看板。

## 2. 为什么现有控件还不够

现有 label、card、table、status_timeline 都能显示文本，但缺少 split_flap_board 这种“分格翻牌 + 焦点字符 + 状态牌”的专用视觉结构。

差异化边界：

1. 以多个独立字符窗格组成编号主体。
2. 每个字符窗格有上下翻片分割线和机械牌语义。
3. 同时承载状态胶囊、路线、时间等轻摘要信息。

## 3. 目标场景与示例概览

目录：

- example/HelloCustomWidgets/display/split_flap_board/

本次示例计划包含：

- board_primary：主出发牌。
- board_compact：紧凑队列牌。
- board_standby：右侧待机牌。

## 4. 视觉与布局规格

- 屏幕基准：240 x 320。
- 顶部标题区：220 x 18。
- 主牌区：192 x 116。
- 底部双牌区：211 x 86。
- 主牌强调蓝色信息层，底部两牌分别使用绿色和琥珀色分组。
- 字符牌必须检查上下分割线、字符居中、左右边界留白。

## 5. 控件清单

| 变量名 | 类型 | 尺寸 | 初始状态 | 用途 |
|---|---|---|---|---|
| root_layout | egui_view_linearlayout_t | 220 x 308 | enabled | 页面总容器 |
| title_label | egui_view_label_t | 220 x 18 | 可见 | 页面标题 |
| guide_label | egui_view_label_t | 220 x 11 | 可见 | 交互说明 |
| board_primary | egui_view_split_flap_board_t | 192 x 116 | snapshot 0 | 主出发牌 |
| status_label | egui_view_label_t | 220 x 12 | 可见 | 当前交互反馈 |
| section_divider | egui_view_line_t | 150 x 2 | 可见 | 分隔线 |
| bottom_row | egui_view_linearlayout_t | 211 x 86 | enabled | 底部容器 |
| board_compact | egui_view_split_flap_board_t | 122 x 80 | snapshot 0 | 紧凑队列牌 |
| board_standby | egui_view_split_flap_board_t | 86 x 80 | snapshot 0 | 待机牌 |

## 6. 状态覆盖矩阵

| 状态 | 覆盖方式 | 预期结果 |
|---|---|---|
| 默认态 | 初始渲染 | 主牌、队列牌、待机牌都完整可见 |
| 主牌切换 | 点击 board_primary | 主编号与状态胶囊变化清楚 |
| 队列切换 | 点击 board_compact | 绿色队列牌状态变化可见 |
| 待机切换 | 点击 board_standby | 琥珀色待机牌状态变化可见 |
| 紧凑态 | 底部两牌 compact_mode | 字符、标题、底部文案仍可读 |

## 7. egui_port_get_recording_action() 录制动作设计

| 序号 | 动作 | 目标 | 预期 |
|---:|---|---|---|
| 0 | WAIT | 首帧稳定 | 记录默认态 |
| 1 | CLICK | board_primary | 切主牌 |
| 2 | WAIT | 稳定 | 记录主牌变化 |
| 3 | CLICK | board_compact | 切队列牌 |
| 4 | WAIT | 稳定 | 记录队列变化 |
| 5 | CLICK | board_standby | 切待机牌 |
| 6 | WAIT | 稳定 | 记录尾帧 |

## 8. 编译、runtime、截图验收标准

命令：

- make all APP=HelloCustomWidgets APP_SUB=display/split_flap_board PORT=pc
- python scripts/code_runtime_check.py --app HelloCustomWidgets --app-sub display/split_flap_board --timeout 10 --keep-screenshots

通过条件：

1. 编译通过。
2. runtime 通过。
3. 字符牌必须完整显示，字符居中，左右留白平衡。
4. 标题、状态胶囊、路线/时间文字不能贴边。
5. 截图同步整理到 iteration_log/。

## 9. 已知限制与下一轮迭代计划

已知限制：

- 第一版未做字符翻转动画，只先做静态翻牌视觉。
- 第一版状态胶囊宽度先用固定值，不做文本自适应。
- 第一版先覆盖 3 组 snapshot，不做动态数据源。

本轮收口结果：

1. 已完成 30 轮基于 runtime 截图的递归迭代。
2. 主牌、状态文案、divider、底部双牌的中心轴与留白关系已收稳。
3. 后续如升级到框架级公共控件，再单独评估翻牌动画和动态文本宽度适配。

## 10. 与现有控件的重叠分析与差异化边界

- 不等同于 label/card：split_flap_board 有离散字符窗格和翻牌结构。
- 不等同于 table：它不承担表格浏览，核心是强编号确认。
- 不等同于 status_timeline：它不表达阶段链路，而是即时牌面状态。
- 不等同于 alert_banner：它不是横向警报条，而是字符牌展示模块。
