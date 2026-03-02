# RadioButton 示例

## 应用说明

演示 RadioButton 控件（互斥单选按钮），4个radio按钮在2x2网格中，点击切换选中项，初始选中radio_1。

## 控件列表

| 控件 | 类型 | 尺寸 (宽x高) | 说明 |
|------|------|--------------|------|
| radio_1 | egui_view_radio_button_t | 60x30 | 宽+小，初始选中 |
| radio_2 | egui_view_radio_button_t | 80x40 | 宽+大 |
| radio_3 | egui_view_radio_button_t | 30x30 | 方+小 |
| radio_4 | egui_view_radio_button_t | 55x55 | 方+大 |
| radio_group | egui_view_radio_group_t | - | 互斥分组 |
| grid | egui_view_gridlayout_t | 220x200 | 2列网格布局 |

## 录制动作

| 序号 | 动作类型 | 目标控件 | 说明 |
|------|----------|----------|------|
| 0 | 点击 | radio_2 | 选中radio_2 |
| 1 | 点击 | radio_3 | 选中radio_3 |
| 2 | 点击 | radio_4 | 选中radio_4 |
| 3 | 点击 | radio_1 | 选中radio_1（回到初始） |
