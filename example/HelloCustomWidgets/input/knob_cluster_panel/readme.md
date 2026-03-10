# knob_cluster_panel

## 1. 为什么需要这个控件

`knob_cluster_panel` 用来表达三旋钮分组、当前旋钮焦点和小型参数面板，适合效果器、合成器、通道条和小型硬件面板。

## 2. 为什么现有控件不够用

- `fader_bank` 强调垂直推子，不适合三旋钮聚类。
- `xy_pad` 强调二维连续控制，不表达离散旋钮组。
- `jog_shuttle_wheel` 是单拨轮 transport，不表达多个参数旋钮。

## 3. 目标场景与示例概览

- 主卡：三旋钮聚类、当前焦点旋钮、状态胶囊和参数 footer
- 左右预读：前一档和后一档旋钮 preset 预读
- 底部状态带：反馈左切、右切和中心推进

## 4. 视觉与布局规格

- 根控件尺寸目标：`240 x 280`
- 三旋钮必须形成明确主次，不可均匀平铺成普通按钮阵列
- 顶部短词胶囊、footer 参数胶囊和底部状态带都要检查真实视觉居中
- 旋钮和边框之间必须保留安全距离

## 5. 控件清单

| 变量名 | 类型 | 尺寸 | 初始状态 | 用途 |
| --- | --- | --- | --- | --- |
| `knob_cluster_panel` | `egui_view_knob_cluster_panel_t` | `240 x 280` | `current=0` | 整个旋钮群控件 |
| `states` | `egui_view_knob_cluster_panel_state_t[]` | - | 第一组 preset | 旋钮焦点和参数数据 |

## 6. 状态覆盖矩阵

| 状态 | 当前 preset | 左预读 | 右预读 | 焦点反馈 |
| --- | --- | --- | --- | --- |
| 默认态 | 当前 preset 高亮 | 前一档 | 后一档 | 当前旋钮高亮 |
| 左切换 | 前一档高亮 | 更前一档 | 原当前档 | 左侧反馈 |
| 右切换 | 后一档高亮 | 原当前档 | 更后一档 | 右侧反馈 |
| 中心推进 | 下一档高亮 | 原当前档 | 更后一档 | 中央推进反馈 |

## 7. `egui_port_get_recording_action()` 录制动作设计

- 首帧等待
- 点击右预读
- 点击左预读
- 点击主卡
- 再连续点击右预读，覆盖 4 个 preset

## 8. 编译、runtime、截图验收标准

```bash
make all APP=HelloCustomWidgets APP_SUB=input/knob_cluster_panel PORT=pc
python scripts/code_runtime_check.py --app HelloCustomWidgets --app-sub input/knob_cluster_panel --timeout 10 --keep-screenshots
```

验收重点：

- 不能黑屏、白屏、全空白
- 三旋钮、左右预读、短词胶囊、footer 参数和底部状态带都必须可见
- 4 个 preset 都必须被录制动作覆盖
- 关键短词与文案必须检查真实视觉居中和边距

## 9. 已知限制与下一轮迭代计划

- 首版先做离散焦点切换，不做真实旋钮旋转拖拽
- 后续可继续增加刻度、数值窗和双层旋钮语义

## 10. 与现有控件的重叠分析与差异化边界

- 区别于 `fader_bank`：这里是三旋钮群，不是通道推子列。
- 区别于 `xy_pad`：这里是离散旋钮焦点，不是二维连续定位。
- 区别于 `jog_shuttle_wheel`：这里强调多旋钮参数组，而不是 transport 单拨轮。
