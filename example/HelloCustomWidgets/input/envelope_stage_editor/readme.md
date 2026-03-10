# envelope_stage_editor

## 1. 为什么需要这个控件
`envelope_stage_editor` 用于表达 ADSR 包络阶段、阶段焦点、时值和电平关系，适合合成器、采样器、效果器和嵌入式音频工作站。

## 2. 为什么现有控件不够用

- `equalizer_curve_editor` 强调频段曲线，不表达 ADSR 阶段结构。
- `range_band_editor` 强调双端区间，不表达多阶段包络。
- `piano_roll_editor` 强调音高-时间编辑，不表达单声部包络塑形。

## 3. 目标场景与示例概览

- 中央主卡：四段 ADSR stage block、焦点阶段、包络轨迹和参数摘要。
- 左右预读：前后 envelope preset 的缩略包络卡。
- 顶部模式胶囊：显示 `SNAP`、`DUB`、`LOOP`、`TRIM`。
- 底部状态带：反馈前后 envelope 切换与中心推进。

## 4. 视觉与布局规格

- 根控件尺寸：`240 x 280`
- 中央主卡必须保持唯一视觉焦点。
- 顶部短词胶囊、footer 胶囊、底部状态带都要检查真实视觉居中。
- 文字与胶囊边框之间必须保留安全距离，避免贴边。

## 5. 控件清单

| 变量名 | 类型 | 尺寸 | 初始状态 | 用途 |
| --- | --- | --- | --- | --- |
| `envelope_stage_editor` | `egui_view_envelope_stage_editor_t` | `240 x 280` | `current=0` | 整个包络编辑控件 |
| `states` | `egui_view_envelope_stage_editor_state_t[]` | - | 第一组 env | 控制阶段电平、焦点和模式数据 |

## 6. 状态覆盖矩阵

| 状态 | 当前 env | 左预读 | 右预读 | 焦点反馈 |
| --- | --- | --- | --- | --- |
| 默认态 | 当前 env 高亮 | 前一档 | 后一档 | 当前 stage 高亮 |
| 左切换 | 前一档进入主卡 | 更前一档 | 原当前档 | 左侧反馈 |
| 右切换 | 后一档进入主卡 | 原当前档 | 更后一档 | 右侧反馈 |
| 中心推进 | 下一档进入主卡 | 原当前档 | 更后一档 | stage 焦点推进 |

## 7. `egui_port_get_recording_action()` 录制动作设计

- 首帧等待。
- 点击右预读。
- 点击左预读。
- 点击主卡中心区域。
- 再连续点击右预读，覆盖 4 个状态。

## 8. 编译、runtime、截图验收标准

```bash
make all APP=HelloCustomWidgets APP_SUB=input/envelope_stage_editor PORT=pc
python scripts/code_runtime_check.py --app HelloCustomWidgets --app-sub input/envelope_stage_editor --timeout 10 --keep-screenshots
```

验收重点：

- 不能黑屏、白屏、全空白。
- ADSR stage block、左右预读、顶部模式胶囊和底部状态带必须完整可见。
- 顶部短词胶囊、footer 胶囊、底部状态带都必须检查真实视觉居中。
- 文字与边框之间必须留有安全距离，不能贴边。

## 9. 已知限制与下一轮迭代计划

- 首版先做离散 preset 切换，不做真实拖拽控制点。
- 后续可以继续增加 velocity、curve shape 和 sync 标记。

## 10. 与现有控件的重叠分析与差异化边界

- 区别于 `equalizer_curve_editor`：这里强调 ADSR 阶段，不是频段曲线。
- 区别于 `range_band_editor`：这里是多段包络，不是单一范围带。
- 区别于 `piano_roll_editor`：这里是包络塑形，不是音符卷帘。
