# step_sequencer

## 1. 为什么需要这个控件

`step_sequencer` 用来表达节拍、灯光、触发器、自动化步骤等按时间推进的离散序列。普通表格或按钮矩阵只能表达开关状态，无法同时说明“时间步进、播放头位置、当前轨道重点和 pattern 摘要”。

## 2. 为什么现有控件不够用

- `button_matrix` 只有按键网格，没有时间轴和播放头语义。
- `fader_bank` 关注连续控制值，不适合表达离散 step pattern。
- `table/list` 只能列步骤文本，缺少网格节奏感和播放头反馈。
- `patch_bay` 是路由插接语义，不具备时间推进和轨道 pattern 概念。

## 3. 目标场景与示例概览

- 主卡：展示 3 条轨道、8 个时间步、播放头高亮和当前 groove 摘要。
- 左下 compact 卡：展示预备 pattern，强调 arm / scan 等排队态。
- 右下 compact 卡：展示锁定 pattern，强调 hold / warn 等受控态。
- 点击三张卡分别轮播 3 组 snapshot，用于 runtime 截图验证状态变化。

## 4. 视觉与布局规格

- 整体 root：`220 x 306`
- 标题区：标题 `220 x 18`，说明文 `220 x 11`
- 主卡：`192 x 118`
- 状态提示文：`220 x 12`
- 分割线：`142 x 2`
- 底部容器：`212 x 86`
- 左下 compact：`108 x 80`
- 右下 compact：`100 x 80`
- 关键检查项：
  - 标题、状态胶囊、pattern 文案必须视觉居中或左右留白平衡。
  - 播放头高亮不能压到网格边缘，轨道步骤分布要均匀。
  - 小卡顶部标题和胶囊不能贴边，底部 `Scene/Bank` 文案要保留呼吸空间。

## 5. 控件清单

| 变量名 | 类型 | 尺寸 | 初始状态 | 用途 |
| --- | --- | --- | --- | --- |
| `root_layout` | `egui_view_linearlayout_t` | `220 x 306` | 垂直布局 | 根容器 |
| `title_label` | `egui_view_label_t` | `220 x 18` | `Step Sequencer` | 标题 |
| `guide_label` | `egui_view_label_t` | `220 x 11` | `Tap panels to rotate groove scenes` | 说明文 |
| `sequencer_primary` | `egui_view_step_sequencer_t` | `192 x 118` | snapshot 0 | 主节拍卡 |
| `status_label` | `egui_view_label_t` | `220 x 12` | `Primary groove live` | 当前状态反馈 |
| `section_divider` | `egui_view_line_t` | `142 x 2` | 常显 | 分割层级 |
| `bottom_row` | `egui_view_linearlayout_t` | `212 x 86` | 水平布局 | 承载下方两张 compact 卡 |
| `sequencer_preview` | `egui_view_step_sequencer_t` | `108 x 80` | compact snapshot 0 | 预备 pattern |
| `sequencer_locked` | `egui_view_step_sequencer_t` | `100 x 80` | locked snapshot 0 | 锁定 pattern |

## 6. 状态覆盖矩阵

| 区域 | Snapshot | 状态胶囊 | Pattern | 播放头 | 说明 |
| --- | --- | --- | --- | --- | --- |
| 主卡 | 0 | `LIVE` | `Groove A` | Step 3 | 稳定播放 |
| 主卡 | 1 | `EDIT` | `Groove B` | Step 6 | 编辑填充 |
| 主卡 | 2 | `LIVE` | `Groove C` | Step 7 | 帽镲变化 |
| 左卡 | 0 | `ARM` | `Scene 1` | Step 2 | 预备排队 |
| 左卡 | 1 | `SCAN` | `Scene 2` | Step 4 | 扫描检查 |
| 左卡 | 2 | `ARM` | `Scene 3` | Step 6 | 预备切换 |
| 右卡 | 0 | `HOLD` | `Bank A` | Step 3 | 锁定保持 |
| 右卡 | 1 | `WARN` | `Bank B` | Step 6 | 守护预警 |
| 右卡 | 2 | `HOLD` | `Bank C` | Step 7 | 再次锁定 |

## 7. `egui_port_get_recording_action()` 录制动作设计

1. 首帧等待 `400ms`，确保默认布局稳定。
2. 点击主卡，切到主卡 snapshot 1。
3. 等待 `300ms`。
4. 点击左下 compact 卡，切到左卡 snapshot 1。
5. 等待 `300ms`。
6. 点击右下 compact 卡，切到右卡 snapshot 1。
7. 等待 `800ms`，输出最终截图。

## 8. 编译、runtime、截图验收标准

### 编译

```bash
make all APP=HelloCustomWidgets APP_SUB=input/step_sequencer PORT=pc
```

### Runtime

```bash
python scripts/code_runtime_check.py --app HelloCustomWidgets --app-sub input/step_sequencer --timeout 10 --keep-screenshots
```

### 截图验收

- 不能黑屏、白屏、空白。
- 主卡与底部两张 compact 卡都必须完整可见，不能裁切。
- 播放头高亮、活动 step、状态胶囊和底部 pattern 文案必须清晰可辨。
- 标题、胶囊、底部文案与边框之间必须有合理空隙，不能贴边。
- 每次迭代都要把关键截图复制到 `iteration_log/images/iter_xx/`，并写入 `iteration_log/iteration_log.md`。

## 9. 已知限制与下一轮迭代计划

- 当前版本已完成 30 轮基于截图的递归迭代，主卡与 compact 卡的标题、胶囊、网格、底部文案关系已稳定。
- 后续如果要继续升级，可考虑补充更多轨道数、可变 step 数或真实编辑交互，而不是继续做同层级的边距微调。
- 若未来沉淀到框架公共控件，可再评估是否抽象 snapshot 数据接口、轨道标签配置和主题色参数。

## 10. 与现有控件的重叠分析与差异化边界

- 不同于 `button_matrix`：这里有时间步进和播放头，不只是离散按钮。
- 不同于 `fader_bank`：这里是离散节拍 pattern，而不是连续电平控制。
- 不同于 `table/list`：这里用节奏网格表达时间结构，不是纯文本列表。
- 不同于 `patch_bay`：这里的核心是时间推进和轨道 pattern，不是左右端口路由。
