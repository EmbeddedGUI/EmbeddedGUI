# HelloViewPageAndScroll

## 应用说明

HelloViewPageAndScroll 是 EmbeddedGUI 框架的 ViewPage 与 Scroll 组合示例。应用展示了一个全屏的 ViewPage 控件，包含 5 个可滑动切换的子页面：一个带滚动列表的页面、三个不同颜色的标签页面（绿、橙、蓝）、以及一个可点击的按钮页面。用户可以通过左右滑动手势在页面间切换。

本示例演示了以下核心功能：
- ViewPage 视图翻页（水平滑动切换）
- ScrollView 滚动列表嵌套在 ViewPage 中
- 多种子视图类型混合（Label、Button、ScrollView）
- 按钮点击交互
- 定时器回调

## 控件列表

| 控件类型 | 变量名 | 说明 |
|---------|--------|------|
| `egui_view_viewpage_t` | `test_viewpage_view` | 根 ViewPage 控件 |
| `egui_view_label_t` | `label_1` | 绿色标签页面 "Item 1" |
| `egui_view_label_t` | `label_2` | 橙色标签页面 "Item 2" |
| `egui_view_label_t` | `label_3` | 蓝色标签页面 "Item 3" |
| `egui_view_button_t` | `button_1` | 按钮页面，点击计数 |
| ScrollView | （在 uicode_scroll.c 中） | 滚动列表子页面 |

## 录制动作

| 序号 | 动作类型 | 说明 |
|------|---------|------|
| 0 | WAIT | 等待初始加载完成 |
| 1 | SWIPE | 向左滑动，切换到第 2 页 |
| 2 | SWIPE | 向左滑动，切换到第 3 页 |
| 3 | SWIPE | 向左滑动，切换到第 4 页 |
| 4 | SWIPE | 向左滑动，切换到第 5 页 |
| 5 | SWIPE | 向右滑动，返回第 4 页 |
| 6 | SWIPE | 向右滑动，返回第 3 页 |
