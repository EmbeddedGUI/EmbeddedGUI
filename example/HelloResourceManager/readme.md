# HelloResourceManager

## 应用说明

HelloResourceManager 是 EmbeddedGUI 框架的资源管理器示例，展示了外部资源（External Resource）的加载和使用。应用包含一个图片控件和两个文本标签，其中一个标签使用内部字体资源，另一个使用外部二进制字体资源。图片同样使用外部二进制资源加载。

本示例演示了以下核心功能：
- 资源管理器功能（EGUI_CONFIG_FUNCTION_RESOURCE_MANAGER）
- 外部资源加载（EGUI_CONFIG_FUNCTION_EXTERNAL_RESOURCE）
- 图片资源显示（内部/外部 RGB565 格式）
- 字体资源使用（内部字体 vs 外部二进制字体）
- 调试信息显示

## 控件列表

| 控件类型 | 变量名 | 说明 |
|---------|--------|------|
| `egui_view_image_t` | `image_1` | 图片控件，显示外部二进制图片资源 |
| `egui_view_label_t` | `label_1` | 文本标签 "Hello World!"，使用内部字体 |
| `egui_view_label_t` | `label_2` | 文本标签 "External Resource!"，使用外部二进制字体 |

## 录制动作

| 序号 | 动作类型 | 说明 |
|------|---------|------|
| 0 | WAIT | 等待初始显示（2秒） |
| 1 | WAIT | 等待观察资源加载效果（2秒） |
