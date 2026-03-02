# Mask 遮罩控件演示

## 应用说明

演示 Mask 遮罩/裁剪功能的基本用法。使用两个自定义遮罩视图（egui_view_test_mask_t），分别应用圆形遮罩和图片遮罩。两个视图均加载同一张测试图片（egui_res_image_test_rgb565_8），尺寸均为 50x50 像素，间距为 5 像素。布局采用垂直居中排列。

## 控件列表

| 变量名 | 控件类型 | 尺寸 (宽x高) | 遮罩类型 | 说明 |
|--------|---------|-------------|---------|------|
| test_mask_1 | TestMask | 50x50 | 圆形遮罩（mask_circle） | 使用圆形遮罩裁剪图片 |
| test_mask_2 | TestMask | 50x50 | 图片遮罩（mask_image） | 使用星形图片遮罩裁剪图片 |
| mask_circle | MaskCircle | 50x50 | - | 圆形遮罩 |
| mask_image | MaskImage | 50x50 | - | 星形图片遮罩（egui_res_image_star_rgb565_8） |

## 录制动作

| 序号 | 动作类型 | 间隔 | 说明 |
|------|---------|------|------|
| 1 | WAIT | 1500ms | 等待展示遮罩裁剪显示效果 |
