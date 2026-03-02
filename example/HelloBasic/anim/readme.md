# Animation 动画演示

## 应用说明

演示动画系统的基本用法。创建一个半径为 10 的圆形视图（白色圆形背景），使用平移动画（Translate Animation）从屏幕顶部移动到底部。动画持续时间 1000ms，使用线性插值器（Linear Interpolator），重复 1 次并以反向模式回弹。动画自动播放，无需用户交互。

## 控件列表

| 变量名 | 控件类型 | 尺寸 (宽x高) | 说明 |
|--------|---------|-------------|------|
| view_1 | View | 21x21 | 圆形视图，半径 10，白色圆形背景 |
| anim_translate | TranslateAnimation | - | 平移动画，Y 方向从 0 到 SCEEN_HEIGHT-20，时长 1000ms |
| anim_linear_interpolator | LinearInterpolator | - | 线性插值器 |

## 录制动作

| 序号 | 动作类型 | 间隔 | 说明 |
|------|---------|------|------|
| 1 | WAIT | 1500ms | 等待动画自动播放，捕获动画运行效果 |
