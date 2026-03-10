# piano_roll_editor

## 1. 为什么需要这个控件
`piano_roll_editor` 用于表达音高-时间二维排布、循环窗、播放头和音符长度，适合音序器、嵌入式 groovebox、采样器和演出控制器。

## 2. 为什么现有控件不够用

- `step_sequencer` 强调离散步进触发，不表达可变长度 note block。
- `frame_scrubber` 强调线性时间轴，不表达 pitch lane 和左侧琴键列。
- `subtitle_timeline` 强调文本段阅读，不表达音符编辑语义。

## 3. 目标场景与示例概览

- 中央主卡：左侧琴键列 + 右侧 piano roll 网格 + note block + playhead + loop window。
- 左右预读：前后 clip 的缩略卷帘。
- 顶部模式胶囊：显示 `LATCH`、`DUB`、`SCAN`、`TRIM`。
- 底部状态带：反馈前后 clip 切换与中心推进。

## 4. 视觉与布局规格

- 根控件尺寸：`240 x 280`
- 中央主卡必须保持唯一视觉焦点，左右预读只能作为辅助阅读。
- 顶部短词胶囊、footer 胶囊、底部状态带都要检查真实视觉居中。
- 短词与胶囊边框之间必须保留安全距离，避免贴边。

## 5. 控件清单

| 变量名 | 类型 | 尺寸 | 初始状态 | 用途 |
| --- | --- | --- | --- | --- |
| `piano_roll_editor` | `egui_view_piano_roll_editor_t` | `240 x 280` | `current=0` | 整个钢琴卷帘控件 |
| `states` | `egui_view_piano_roll_editor_state_t[]` | - | 第一组 clip | 控制模式、循环窗、播放头和焦点行 |

## 6. 状态覆盖矩阵

| 状态 | 当前 clip | 左预读 | 右预读 | 焦点反馈 |
| --- | --- | --- | --- | --- |
| 默认态 | 当前 clip 高亮 | 前一档 | 后一档 | 当前行与播放头高亮 |
| 左切换 | 前一档进入主卡 | 更前一档 | 原当前档 | 左侧反馈 |
| 右切换 | 后一档进入主卡 | 原当前档 | 更后一档 | 右侧反馈 |
| 中心推进 | 下一档进入主卡 | 原当前档 | 更后一档 | playhead 推进反馈 |

## 7. `egui_port_get_recording_action()` 录制动作设计

- 首帧等待。
- 点击右预读。
- 点击左预读。
- 点击主卡中心区域。
- 再连续点击右预读，覆盖 4 个状态。

## 8. 编译、runtime、截图验收标准

```bash
make all APP=HelloCustomWidgets APP_SUB=input/piano_roll_editor PORT=pc
python scripts/code_runtime_check.py --app HelloCustomWidgets --app-sub input/piano_roll_editor --timeout 10 --keep-screenshots
```

验收重点：

- 不能黑屏、白屏、全空白。
- 左侧琴键列、右侧音符网格、顶部模式胶囊和底部状态带必须完整可见。
- 顶部短词胶囊、footer 胶囊、底部状态带都必须检查真实视觉居中。
- 文字与边框之间必须留有安全距离，不能贴边。

## 9. 已知限制与下一轮迭代计划

- 首版先做离散 clip 切换，不做真实拖拽编辑。
- 后续可以继续增加 velocity lane、量化刻度和更多音符行。

## 10. 与现有控件的重叠分析与差异化边界

- 区别于 `step_sequencer`：这里强调长度型音符块，不是固定步进开关。
- 区别于 `frame_scrubber`：这里强调 pitch-time 二维网格，不是单轴时间刷。
- 区别于 `subtitle_timeline`：这里是音符编辑视图，不是文本 cue 时间轴。
