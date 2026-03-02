# HelloAPP

## 应用说明

HelloAPP 是 EmbeddedGUI 框架的页面管理示例，展示了基于 `egui_page_base_t` 的轻量级页面切换机制。应用包含 2 个页面（Page 0 和 Page 1），每个页面有独立的定时器更新标签文字、"Next"/"Finish" 导航按钮以及图片显示。支持按键事件驱动的页面切换。

本示例演示了以下核心功能：
- Page 页面管理（open/close 生命周期）
- 页面间导航（前进/后退）
- 页面内定时器驱动的文本更新
- 按键事件处理
- 图片控件显示
- Toast 提示信息

## 控件列表

| 控件类型 | 变量名 | 说明 |
|---------|--------|------|
| `egui_page_0_t` | `g_page_array.page_0` | 页面0（绿色背景） |
| `egui_page_1_t` | `g_page_array.page_1` | 页面1（蓝色背景） |
| `egui_view_linearlayout_t` | `layout_1`（每个页面内） | 居中垂直布局 |
| `egui_view_label_t` | `label_1`（每个页面内） | 页面标题/定时计数标签 |
| `egui_view_button_t` | `button_1`（每个页面内） | "Next" 按钮 |
| `egui_view_button_t` | `button_2`（每个页面内） | "Finish" 按钮（返回上一页） |
| `egui_view_image_t` | `image_1`（每个页面内） | 星形图片 |
| `egui_toast_std_t` | `toast` | Toast 提示 |

## 录制动作

| 序号 | 动作类型 | 说明 |
|------|---------|------|
| 0 | WAIT | 等待初始页面加载完成 |
| 1 | CLICK | 点击 "Next" 按钮，切换到页面 1 |
| 2 | WAIT | 等待观察页面 1 |
| 3 | CLICK | 点击 "Finish" 按钮，返回页面 0 |
