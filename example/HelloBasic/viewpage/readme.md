# ViewPage 控件演示

## 应用说明

演示 ViewPage 控件，展示水平翻页容器。布局采用全屏显示。

## 控件列表

| 变量名 | 控件类型 | 尺寸 (宽x高) | 说明 |
|--------|---------|-------------|------|
| viewpage_1 | ViewPage | SCREEN_W x SCREEN_H | 全屏水平翻页容器 |
| page_1 (label) | Label | SCREEN_W x SCREEN_H | 第1页，绿色背景 |
| page_2 (label) | Label | SCREEN_W x SCREEN_H | 第2页，橙色背景 |
| page_3 (label) | Label | SCREEN_W x SCREEN_H | 第3页，蓝色背景 |

## 录制动作

| 序号 | 动作类型 | 目标 | 间隔 | 预期效果 |
|------|---------|------|------|---------|
| 1 | SWIPE LEFT | viewpage_1 | 1500ms | 从第1页翻到第2页，带翻页动画 |
| 2 | SWIPE LEFT | viewpage_1 | 1500ms | 从第2页翻到第3页，带翻页动画 |
| 3 | SWIPE RIGHT | viewpage_1 | 1500ms | 从第3页翻回第2页，带翻页动画 |
| 4 | SWIPE RIGHT | viewpage_1 | 1500ms | 从第2页翻回第1页，带翻页动画 |
