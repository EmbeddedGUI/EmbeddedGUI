# HelloLayer

## 应用说明

HelloLayer 是 EmbeddedGUI 框架的图层（Layer）功能演示示例。应用展示了三个重叠的彩色矩形标签（红、绿、蓝），通过底部的三个按钮可以控制哪个标签显示在最前面。应用使用 `egui_view_set_layer()` API 来动态调整视图的层级顺序。

本示例演示了以下核心功能：
- 视图层级控制（Layer Top / Default）
- 多个视图重叠显示
- ViewGroup 容器嵌套
- 水平 LinearLayout 按钮排列
- 状态文本实时更新

## 控件列表

| 控件类型 | 变量名 | 说明 |
|---------|--------|------|
| `egui_view_group_t` | `root_group` | 根容器，全屏深灰背景 |
| `egui_view_label_t` | `label_title` | 标题标签 "Layer Demo" |
| `egui_view_group_t` | `overlap_group` | 重叠区域容器 |
| `egui_view_label_t` | `label_red` | 红色矩形标签 |
| `egui_view_label_t` | `label_green` | 绿色矩形标签 |
| `egui_view_label_t` | `label_blue` | 蓝色矩形标签 |
| `egui_view_label_t` | `label_status` | 状态标签，显示当前最前面的颜色 |
| `egui_view_linearlayout_t` | `btn_layout` | 底部按钮水平布局 |
| `egui_view_button_t` | `btn_red_top` | "Red Top" 按钮 |
| `egui_view_button_t` | `btn_green_top` | "Grn Top" 按钮 |
| `egui_view_button_t` | `btn_blue_top` | "Blu Top" 按钮 |

## 录制动作

| 序号 | 动作类型 | 说明 |
|------|---------|------|
| 0 | CLICK | 点击 "Red Top" 按钮，红色标签移到最前 |
| 1 | CLICK | 点击 "Grn Top" 按钮，绿色标签移到最前 |
| 2 | CLICK | 点击 "Blu Top" 按钮，蓝色标签移到最前 |
