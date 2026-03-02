# Roller 示例

## 应用说明

演示 Roller 控件（滚轮选择器），4个不同尺寸在2x2网格中，显示星期选项(Mon-Fri)，上下拖动切换选中项。

## 控件列表

| 控件 | 类型 | 尺寸 (宽x高) | 初始选中 | 选项 |
|------|------|--------------|----------|------|
| roller_1 | egui_view_roller_t | 100x45 | Mon (0) | Mon, Tue, Wed, Thu, Fri |
| roller_2 | egui_view_roller_t | 130x60 | Tue (1) | Mon, Tue, Wed, Thu, Fri |
| roller_3 | egui_view_roller_t | 60x60 | Wed (2) | Mon, Tue, Wed, Thu, Fri |
| roller_4 | egui_view_roller_t | 80x90 | Thu (3) | Mon, Tue, Wed, Thu, Fri |
| grid | egui_view_gridlayout_t | 240x260 | - | - |

## 录制动作

| 序号 | 动作类型 | 目标控件 | 说明 |
|------|----------|----------|------|
| 0 | 拖动 | roller_1 | 从下(0.5,0.8)拖到上(0.5,0.2)，向后滚动，10步 |
| 1 | 拖动 | roller_2 | 从下(0.5,0.8)拖到上(0.5,0.2)，向后滚动，10步 |
| 2 | 拖动 | roller_3 | 从上(0.5,0.2)拖到下(0.5,0.8)，向前滚动，10步 |
| 3 | 拖动 | roller_4 | 从上(0.5,0.2)拖到下(0.5,0.8)，向前滚动，10步 |
