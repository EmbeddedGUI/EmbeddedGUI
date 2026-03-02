# TextInput 控件演示

## 应用说明

演示 TextInput 控件，展示文本输入框和虚拟键盘。布局采用垂直居中排列，虚拟键盘在底部弹出。

## 控件列表

| 变量名 | 控件类型 | 尺寸 (宽x高) | 说明 |
|--------|---------|-------------|------|
| label_title | Label | - | 标题文本 |
| textinput_1 | TextInput | 180x28 | 文本输入框 |
| button_submit | Button | 100x30 | 提交按钮 |
| label_result | Label | - | 结果显示文本 |
| keyboard | Keyboard | - | 虚拟键盘（底部弹出） |

## 录制动作

| 序号 | 动作类型 | 目标 | 间隔 | 预期效果 |
|------|---------|------|------|---------|
| 1 | CLICK | textinput_1 | 1500ms | 输入框获得焦点，虚拟键盘弹出显示 |
| 2 | WAIT | - | 1500ms | 等待键盘完全显示 |
| 3 | CLICK | button_submit | 1000ms | 键盘收起，焦点状态变化 |
