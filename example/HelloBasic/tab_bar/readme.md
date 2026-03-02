# TabBar + ViewPage 控件演示

## 应用说明

演示 TabBar + ViewPage 双向同步，点击标签切换页面或滑动页面同步标签选中状态。布局为 TabBar 在上，ViewPage 在下。

## 控件列表

| 变量名 | 控件类型 | 尺寸 (宽x高) | 说明 |
|--------|---------|-------------|------|
| tab_bar | TabBar | SCREEN_W x 30 | 顶部标签栏，包含3个标签 |
| tab_home | Tab | - | 标签: Home |
| tab_list | Tab | - | 标签: List |
| tab_settings | Tab | - | 标签: Settings |
| viewpage | ViewPage | SCREEN_W x (SCREEN_H-30) | 下方翻页容器，包含3个彩色页面 |

## 录制动作

| 序号 | 动作类型 | 目标 | 间隔 | 预期效果 |
|------|---------|------|------|---------|
| 1 | CLICK | tab_list | 1000ms | 标签切换到 List，ViewPage 同步翻到第2页 |
| 2 | CLICK | tab_settings | 1000ms | 标签切换到 Settings，ViewPage 同步翻到第3页 |
| 3 | SWIPE RIGHT | viewpage | 1000ms | ViewPage 翻回上一页，TabBar 同步切换标签 |
| 4 | CLICK | tab_home | 1000ms | 标签切换到 Home，ViewPage 同步翻到第1页 |
