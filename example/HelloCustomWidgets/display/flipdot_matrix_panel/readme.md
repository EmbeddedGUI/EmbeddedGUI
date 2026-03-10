# flipdot_matrix_panel

## 1. 为什么需要这个控件
`flipdot_matrix_panel` 用来表达点阵翻点信息牌、聚焦行和左右预读，适合交通牌、工业状态板、演播室 tally 面板和复古信息墙。

## 2. 为什么现有控件不够用

- `split_flap_board` 强调翻牌窗格，不表达点阵式单点翻转。
- `signal_matrix` 强调规则信号格阵，不表达信息牌式的聚焦行。
- `tag_cloud` 强调离散标签，不表达板式状态阅读。

## 3. 目标场景与示例概览

- 中央主卡：3 行 flip-dot 点阵、当前聚焦行、状态胶囊和 footer 条。
- 左右预读：前后 board 的简化点阵卡片。
- 顶部状态胶囊：显示 `LIVE`、`HOLD`、`SCAN`、`LINK` 这类短词。
- 底部状态带：反馈左切、右切和中心推进。

## 4. 视觉与布局规格

- 根控件尺寸：`240 x 280`
- 中央主卡必须是唯一视觉焦点。
- 顶部短词胶囊、footer 胶囊、底部状态带都要检查真实视觉居中。
- 文字、圆点、短线与边框之间必须保留安全距离。

## 5. 控件清单

| 变量名 | 类型 | 尺寸 | 初始状态 | 用途 |
| --- | --- | --- | --- | --- |
| `flipdot_matrix_panel` | `egui_view_flipdot_matrix_panel_t` | `240 x 280` | `current=0` | 整个 flip-dot 控件 |
| `states` | `egui_view_flipdot_matrix_panel_state_t[]` | - | 第一组 board | 聚焦行和状态数据 |

## 6. 状态覆盖矩阵

| 状态 | 当前 board | 左预读 | 右预读 | 焦点反馈 |
| --- | --- | --- | --- | --- |
| 默认态 | 当前 board 高亮 | 前一档 | 后一档 | 当前行高亮 |
| 左切换 | 前一档进入主卡 | 更前一档 | 原当前档 | 左侧反馈 |
| 右切换 | 后一档进入主卡 | 原当前档 | 更后一档 | 右侧反馈 |
| 中心推进 | 下一档进入主卡 | 原当前档 | 更后一档 | 中间推进反馈 |

## 7. `egui_port_get_recording_action()` 录制动作设计

- 首帧等待。
- 点击右预读。
- 点击左预读。
- 点击主卡。
- 再连续点击右预读，覆盖 4 个状态。

## 8. 编译、runtime、截图验收标准

```bash
make all APP=HelloCustomWidgets APP_SUB=display/flipdot_matrix_panel PORT=pc
python scripts/code_runtime_check.py --app HelloCustomWidgets --app-sub display/flipdot_matrix_panel --timeout 10 --keep-screenshots
```

验收重点：

- 不能黑屏、白屏、全空白。
- 点阵主板、左右预读、顶部短词胶囊和底部状态带必须完整可见。
- 顶部短词胶囊、footer 胶囊、底部状态带都必须检查真实视觉居中。
- 文字与边框之间必须留有安全距离，不能贴边。

## 9. 已知限制与下一轮迭代计划

- 首版先做离散 board 切换，不做真实点阵动画翻转。
- 后续可继续增加更多行数、滚动提示和数字计数。

## 10. 与现有控件的重叠分析与差异化边界

- 区别于 `split_flap_board`：这里是点阵翻点，不是分割翻牌。
- 区别于 `signal_matrix`：这里强调信息牌式聚焦行，不是纯矩阵监控格。
- 区别于 `tag_cloud`：这里是结构化信息牌，不是自由分布标签。
