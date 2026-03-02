# Scroll 控件演示

## 应用说明

演示 Scroll 控件，展示垂直滚动容器。布局采用全屏显示。

## 控件列表

| 变量名 | 控件类型 | 尺寸 (宽x高) | 说明 |
|--------|---------|-------------|------|
| scroll_1 | Scroll | SCREEN_W x SCREEN_H | 全屏垂直滚动容器 |
| label_1 | Label | SCREEN_W x (SCREEN_H/2) | 第1个半屏内容，绿色背景 |
| label_2 | Label | SCREEN_W x (SCREEN_H/2) | 第2个半屏内容，橙色背景 |
| label_3 | Label | SCREEN_W x (SCREEN_H/2) | 第3个半屏内容，蓝色背景 |

总内容高度为 1.5 倍屏幕高度。

## 录制动作

| 序号 | 动作类型 | 目标 | 间隔 | 预期效果 |
|------|---------|------|------|---------|
| 1 | SWIPE UP | scroll_1 | 1500ms | 内容向上滚动，显示下方内容 |
| 2 | SWIPE DOWN | scroll_1 | 1500ms | 内容向下滚动，返回上方内容 |
