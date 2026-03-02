# HelloEasyPage

## 应用说明

HelloEasyPage 是 EmbeddedGUI 框架的简易页面管理示例，展示了 3 个页面的切换管理。与 HelloAPP 类似，但包含更多页面。每个页面有独立的定时器和不同的更新频率，页面间通过 "Next"/"Finish" 按钮导航。三个页面分别使用绿色、蓝色和红色背景以示区分。

本示例演示了以下核心功能：
- 多页面管理（3个页面的前进/后退导航）
- 不同页面使用不同定时器频率（1s/2s/3s）
- 页面生命周期管理（open/close）
- 按键事件驱动的页面切换
- Toast 提示信息

## 控件列表

| 控件类型 | 变量名 | 说明 |
|---------|--------|------|
| `egui_page_0_t` | `g_page_array.page_0` | 页面0（绿色背景，1秒定时器） |
| `egui_page_1_t` | `g_page_array.page_1` | 页面1（蓝色背景，2秒定时器） |
| `egui_page_2_t` | `g_page_array.page_2` | 页面2（红色背景，3秒定时器） |
| `egui_view_linearlayout_t` | `layout_1`（每个页面内） | 居中垂直布局 |
| `egui_view_label_t` | `label_1`（每个页面内） | 页面标题/定时计数标签 |
| `egui_view_button_t` | `button_1`（每个页面内） | "Next" 按钮 |
| `egui_view_button_t` | `button_2`（每个页面内） | "Finish" 按钮（返回上一页） |
| `egui_toast_std_t` | `toast` | Toast 提示 |

## 录制动作

| 序号 | 动作类型 | 说明 |
|------|---------|------|
| 0 | WAIT | 等待初始页面加载完成 |
| 1 | CLICK | 点击 "Next"，切换到页面 1 |
| 2 | CLICK | 点击 "Next"，切换到页面 2 |
| 3 | CLICK | 点击 "Finish"，返回页面 1 |
| 4 | CLICK | 点击 "Finish"，返回页面 0 |
