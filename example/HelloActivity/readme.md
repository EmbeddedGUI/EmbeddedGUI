# HelloActivity

## 应用说明

HelloActivity 是 EmbeddedGUI 框架的 Activity 管理示例，展示了类 Android 的 Activity 生命周期管理机制。应用支持多个 Activity 的创建、启动和销毁，以及 Dialog 弹窗功能。每个 Activity 包含标签显示当前序号，以及"Next"、"Finish"、"Dialog"三个操作按钮。Activity 之间的切换带有平移动画效果。

本示例演示了以下核心功能：
- Activity 生命周期管理（create/start/resume/pause/stop/destroy）
- Activity 栈式导航（启动新 Activity、返回上一个）
- Activity 切换动画（水平平移过渡）
- Dialog 弹窗显示与关闭（带动画）
- Toast 提示信息

## 控件列表

| 控件类型 | 变量名 | 说明 |
|---------|--------|------|
| `egui_activity_test_t` | `activity_test_array[]` | Activity 实例数组（最多3个） |
| `egui_view_linearlayout_t` | `layout_1`（每个Activity内） | 居中垂直布局 |
| `egui_view_label_t` | `label_1`（每个Activity内） | 显示 Activity 序号 |
| `egui_view_button_t` | `button_1`（每个Activity内） | "Next" 按钮，启动下一个 Activity |
| `egui_view_button_t` | `button_2`（每个Activity内） | "Finish" 按钮，关闭当前 Activity |
| `egui_view_button_t` | `button_3`（每个Activity内） | "Dialog" 按钮，打开弹窗 |
| `egui_dialog_test_t` | `dialog` | 测试弹窗，含"Close"按钮 |
| `egui_toast_std_t` | `toast` | Toast 提示 |

## 录制动作

| 序号 | 动作类型 | 说明 |
|------|---------|------|
| 0 | WAIT | 等待初始 Activity 加载完成 |
| 1 | CLICK | 点击 "Next" 按钮，启动 Activity 1 |
| 2 | CLICK | 点击 "Next" 按钮，启动 Activity 2 |
| 3 | CLICK | 点击 "Dialog" 按钮，打开弹窗 |
| 4 | CLICK | 点击弹窗中的 "Close" 按钮，关闭弹窗 |
| 5 | CLICK | 点击 "Finish" 按钮，返回上一个 Activity |
