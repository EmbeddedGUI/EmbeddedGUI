# HelloPFB

## 应用说明

HelloPFB 是 EmbeddedGUI 框架的局部帧缓冲（Partial Frame Buffer）测试示例。应用展示了 PFB 刷新机制的工作原理，通过定时器驱动自动切换多个标签控件的背景颜色和位置，验证脏矩形（Dirty Region）的正确性。本示例使用自定义 PFB 配置（8x320 垂直刷新模式）和调试选项来可视化 PFB 刷新区域。

本示例演示了以下核心功能：
- PFB 垂直/水平刷新模式配置
- 脏矩形（Dirty Region）刷新可视化
- 定时器驱动的多种背景切换测试
- 视图移动（scroll_by）测试
- 单区域与多区域同时刷新测试

## 控件列表

| 控件类型 | 变量名 | 说明 |
|---------|--------|------|
| `egui_view_label_t` | `label_vertical` | 垂直标签（黄色，左侧全高） |
| `egui_view_label_t` | `label_horizontal` | 水平标签（橙色，顶部全宽） |
| `egui_view_label_t` | `label_1` | 小标签1（绿色） |
| `egui_view_label_t` | `label_2` | 小标签2（紫色） |

## 录制动作

| 序号 | 动作类型 | 说明 |
|------|---------|------|
| 0 | WAIT | 等待初始显示（2秒） |
| 1 | WAIT | 等待定时器驱动的背景切换（3秒） |
| 2 | WAIT | 等待更多定时器周期（3秒） |
| 3 | WAIT | 等待更多定时器周期（3秒） |
