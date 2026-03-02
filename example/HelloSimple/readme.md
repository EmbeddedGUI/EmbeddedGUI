# HelloSimple

## 应用说明

HelloSimple 是 EmbeddedGUI 框架的入门示例，展示了最基本的 UI 构建方式。应用包含一个居中的垂直布局，内含一个文本标签和一个可点击的按钮。点击按钮后，按钮文字会更新为点击次数计数。

本示例演示了以下核心功能：
- LinearLayout 垂直布局与居中对齐
- Label 文本标签显示
- Button 按钮与点击事件回调
- 圆角矩形描边背景（normal/pressed/disabled 三态）

## 控件列表

| 控件类型 | 变量名 | 说明 |
|---------|--------|------|
| `egui_view_linearlayout_t` | `layout_1` | 根布局，全屏居中对齐 |
| `egui_view_label_t` | `label_1` | 显示 "Hello World!" 的文本标签 |
| `egui_view_button_t` | `button_1` | 可点击按钮，显示点击计数 |
| `egui_background_color_t` | `bg_button` | 按钮背景（圆角矩形描边样式） |

## 录制动作

| 序号 | 动作类型 | 说明 |
|------|---------|------|
| 0 | CLICK | 点击按钮，显示 "Clicked 1s" |
| 1 | CLICK | 点击按钮，显示 "Clicked 2s" |
| 2 | CLICK | 点击按钮，显示 "Clicked 3s" |
