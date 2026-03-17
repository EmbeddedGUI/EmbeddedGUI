# clip_launcher_grid 控件设计说明

## 1. 为什么需要这个控件

`clip_launcher_grid` 用来表达“轨道 x 场景”的实时触发矩阵，适合做音乐工作站、演出控制、实时 cue 触发或多素材编排场景。它的重点不是时间轴编辑，而是让用户快速看清当前播放列、下一待触发列、armed 轨道和焦点轨道。

## 2. 为什么现有控件不够用

- `step_sequencer` 强调按时间步推进的节拍编辑，不适合表达 scene 切换
- `button_matrix` 只有离散按钮语义，没有 queued / playing / armed 的多层状态
- `scene_crossfader` 强调双源混合，不是多轨多场景的并行矩阵
- `patch_bay` 关注路由关系，不是触发网格

所以需要一个新的复合控件，把 scene 头、track 头、playing 列、queued 列和 clip 状态放到同一个面板里。

## 3. 目标场景与示例概览

- 主卡：完整 4 x 4 clip 触发矩阵
- 紧凑卡：保留矩阵语义，用单字母轨道标记和紧凑行提示压缩信息密度
- 锁定卡：展示 recall / hold 这类只读状态

目录：`example/HelloCustomWidgets/input/clip_launcher_grid/`

## 4. 视觉与布局规格

- 设计分辨率：240 x 320
- 页面结构：标题 -> 引导文案 -> 主矩阵 -> 状态文本 -> 分隔线 -> compact / locked 双卡
- 主卡尺寸：194 x 126
- 紧凑卡尺寸：108 x 84
- 锁定卡尺寸：100 x 84
- scene 头固定 4 列，track 头固定 4 行
- scene 头用状态底板承接 4 列，playing 头部带底部高亮条，queued 头部带顶部排队条
- playing 列用纵向高亮区和实体 clip 块强调，queued 列用外框和顶部 cue 条强调
- armed / focus 轨道同时使用左侧 track pill、行级底板和细轨提示强化
- compact 模式保留左侧上下文列与分隔线，locked 模式增加内层 seal 边框
- footer pill 用顶部状态细条补充 track / scene 语义
- header 底部增加状态分隔线，footer pill 用对称状态点强化可扫读性
- 主卡 playing 列补充纵向 live spine，queued 列补充上下 cap 与侧向 cue rails
- 主卡 scene 头与矩阵本体之间增加状态桥接，locked 模式叠加轻 veil 与 hold rail
- 矩阵内部增加行分隔节奏线与行锚点，locked 模式进一步加深 veil 和 hold 节点
- 主卡左侧补充 label spine / row lead-in，右侧补充 active row echo tab，空槽保留 idle dot 节奏
- compact 模式补充 label lane ruler、active label underline、row pre-cap dot 与 footer center node
- locked 模式补充 inner corner bracket、center lock badge、row mask bar 与 footer guard node
- header status pill 补充 top shine / tail underline，主卡 footer 补充 center divider 与 track/scene notch cue
- 面板外壳与矩阵壳体补充细角标、top haze、bottom seat 等收尾细节

## 5. 控件清单

| 变量名 | 类型 | 尺寸 (W x H) | 初始状态 | 用途 |
| --- | --- | ---: | --- | --- |
| `root_layout` | `egui_view_linearlayout_t` | 224 x 306 | enabled | 整体纵向布局 |
| `title_label` | `egui_view_label_t` | 224 x 18 | `Clip Launcher` | 页面标题 |
| `guide_label` | `egui_view_label_t` | 224 x 11 | `Tap panels to rotate queued scenes` | 引导文案 |
| `grid_primary` | `egui_view_clip_launcher_grid_t` | 194 x 126 | `LIVE` | 主触发矩阵 |
| `status_label` | `egui_view_label_t` | 224 x 12 | `Primary scene live` | 当前动作反馈 |
| `section_divider` | `egui_view_line_t` | 146 x 2 | visible | 分隔上下区域 |
| `grid_compact` | `egui_view_clip_launcher_grid_t` | 108 x 84 | `ARM` | 紧凑预览 |
| `grid_locked` | `egui_view_clip_launcher_grid_t` | 100 x 84 | `HOLD` | 锁定预览 |

## 6. 状态覆盖矩阵

| 区域 / 状态 | 状态 A | 状态 B | 状态 C |
| --- | --- | --- | --- |
| 主卡 | `LIVE` | `QUEUE` | `LIVE` |
| 主卡 scene 列 | `B` 播放 / `C` 排队 | `C` 播放 / `D` 排队 | `A` 重触发 |
| 紧凑卡 | `ARM` | `SCAN` | `ARM` |
| 锁定卡 | `HOLD` | `WARN` | `HOLD` |

## 7. `egui_port_get_recording_action()` 录制动作设计

1. 等待 400ms，记录初始态
2. 点击主卡，切到 queued 场景
3. 等待 300ms
4. 点击 compact 卡，切到 scan 态
5. 等待 300ms
6. 点击 locked 卡，切到 warn 态
7. 等待 800ms，记录收尾态

## 8. 编译、runtime、截图验收标准

```bash
make all APP=HelloCustomWidgets APP_SUB=input/clip_launcher_grid PORT=pc
python scripts/code_runtime_check.py --app HelloCustomWidgets --app-sub input/clip_launcher_grid --timeout 10 --keep-screenshots
```

验收重点：

- 主卡 matrix 不能裁切，scene 头和 track 头要保持对齐
- `LIVE / QUEUE / HOLD` 等短词需要视觉居中
- playing 列与 queued 列必须一眼可分辨
- compact / locked 卡在小尺寸下仍然保留层次
- 必须完成 30 轮视觉迭代，并为每轮保留 `iteration_log/` 截图与日志
- runtime 输出必须为 `ALL PASSED`

## 9. 已知限制与下一轮迭代计划

- 本轮已完成 30 次迭代收敛，主卡 / compact / locked 三套视觉语义均已补齐验收
- 当前版本只覆盖 4 x 4 固定矩阵
- scene 标签仍是固定 `A / B / C / D`
- 后续可继续探索更多 clip 类型标识、动态 scene 标签或可变轨道数量

## 10. 与现有控件的重叠分析与差异化边界

- 相比 `step_sequencer`：这里不是时间步进，而是 scene x track 的实时触发矩阵
- 相比 `button_matrix`：这里不是纯按钮网格，而是带 queued / playing / armed 的状态网格
- 相比 `scene_crossfader`：这里是多轨多场景并行，不是 A/B 双源混合
- 相比 `patch_bay`：这里表达的是 clip 触发，不是路由连接
