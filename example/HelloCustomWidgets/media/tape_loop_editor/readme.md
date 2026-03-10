# tape_loop_editor

## 1. 为什么需要这个控件
`tape_loop_editor` 用来表达双盘磁带、中心循环窗口、剪切点和 cue 状态，适合采样器、现场循环器、磁带延迟和复古编辑台场景。

## 2. 为什么现有控件不够用

- `frame_scrubber` 强调线性时间轴，不表达双盘磁带结构。
- `waveform_strip` 强调波形振幅，不表达 loop window 和剪切点。
- `jog_shuttle_wheel` 强调单中心拨轮，不表达双盘 tape deck。

## 3. 目标场景与示例概览

- 中央主卡：双盘磁带、中心 loop window、剪切点和底部 cue 条。
- 左右预读：上一档和下一档 tape preset 的简化预览。
- 顶部状态胶囊：显示 `LOOP`、`SLIP`、`CUT`、`CUE` 这类短词状态。
- 底部状态带：反馈左切、右切和中心推进的当前动作。

## 4. 视觉与布局规格

- 根控件目标尺寸：`240 x 280`
- 中央主卡必须是唯一视觉焦点，左右预读只做辅助信息。
- 顶部短词胶囊、footer 胶囊和底部状态带都要检查真实视觉居中。
- 短词、圆点、短线与边框之间必须保留安全距离，不能贴边。

## 5. 控件清单

| 变量名 | 类型 | 尺寸 | 初始状态 | 用途 |
| --- | --- | --- | --- | --- |
| `tape_loop_editor` | `egui_view_tape_loop_editor_t` | `240 x 280` | `current=0` | 整个 tape loop 控件 |
| `states` | `egui_view_tape_loop_editor_state_t[]` | - | 第一组 preset | 双盘磁带与循环窗口状态数据 |

## 6. 状态覆盖矩阵

| 状态 | 当前 preset | 左预读 | 右预读 | 焦点反馈 |
| --- | --- | --- | --- | --- |
| 默认态 | 当前 preset 高亮 | 前一档 | 后一档 | 当前 focus zone 高亮 |
| 左切换 | 前一档进入主卡 | 更前一档 | 原当前档 | 左侧预读反馈 |
| 右切换 | 后一档进入主卡 | 原当前档 | 更后一档 | 右侧预读反馈 |
| 中心推进 | 下一档进入主卡 | 原当前档 | 更后一档 | 中心 deck 反馈 |

## 7. `egui_port_get_recording_action()` 录制动作设计

- 首帧等待，确认默认态稳定。
- 点击右预读卡，切到下一档 preset。
- 点击左预读卡，回到上一档 preset。
- 点击主卡，推进到下一档 preset。
- 再连续点击右预读卡，覆盖 4 个 preset。

## 8. 编译、runtime、截图验收标准

```bash
make all APP=HelloCustomWidgets APP_SUB=media/tape_loop_editor PORT=pc
python scripts/code_runtime_check.py --app HelloCustomWidgets --app-sub media/tape_loop_editor --timeout 10 --keep-screenshots
```

验收重点：

- 不能黑屏、白屏、全空白。
- 双盘磁带、中心 loop window、左右预读和底部状态带必须完整可见。
- 顶部短词胶囊、footer 胶囊、底部状态带都必须检查真实视觉居中。
- 短词与边框之间必须有安全距离，不能出现贴边或某一侧留白明显失衡。
- 每轮截图都要归档进 `iteration_log/images/iter_xx/`。

## 9. 已知限制与下一轮迭代计划

- 首版先做离散 preset 切换，不做真实拖拽卷盘。
- 后续可以继续增加磁带刻度、时间码和更复杂的 splice 标记。

## 10. 与现有控件的重叠分析与差异化边界

- 区别于 `frame_scrubber`：这里是双盘 tape deck，不是线性帧带。
- 区别于 `waveform_strip`：这里强调循环窗口和磁带盘，不是振幅条。
- 区别于 `jog_shuttle_wheel`：这里是双盘磁带和 loop deck，不是单中心 shuttle wheel。
