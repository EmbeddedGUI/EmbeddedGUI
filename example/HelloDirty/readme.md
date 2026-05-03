# HelloDirty

`HelloDirty` 用于演示脏区刷新：画面看起来像是整屏在动，但每一帧只把真正变化的局部区域标记为 dirty。

本示例使用 480x480 的 `viewpage`，包含 13 个页面：

- Line dirty：线段角度变化时，只刷新旧线段和新线段的包围区域。
- Bouncing balls：多个小球随机运动并互相碰撞，只刷新每个小球的旧圆和新圆。
- Streaming wave：波形持续更新，只刷新受影响的采样列和游标条；相距较远的列分开上报。
- Live chart：实时曲线图保持固定横向点数，曲线区域每帧向左搬移一个采样宽度，右侧持续写入新点；支持屏幕内 copy 的显示端只重绘左侧轴线和右侧新样本窄条，缺失该能力时退化为图表区重绘。
- Gauge delta：仪表变化时，只刷新旧/新指针、变化的弧段和数值区域。
- Cell changes：单元格高亮移动时，只刷新旧单元格、新单元格和闪烁光标。
- Radar sweep：雷达扫描线绕圈运动，只刷新旧扫描线和新扫描线。
- Spectrum bars：频谱柱持续跳动，只刷新本帧变化的少数柱子。
- Digital clock：看起来整块时间在跳，只刷新秒数字区域。
- Progress edge：进度条持续变化，只刷新移动边界和数值。
- List select：列表高亮逐行移动，只刷新旧行和新行。
- Map marker：地图路线上的定位点移动，只刷新旧点和新点。
- Chart cursor：图表十字游标移动，只刷新旧游标列和新游标列。

普通运行：

```bash
make all APP=HelloDirty PORT=pc
make run APP=HelloDirty PORT=pc
```

运行时检查：

```bash
python scripts/code_runtime_check.py --app HelloDirty --keep-screenshots
```

显示脏区边框：

```bash
make all APP=HelloDirty PORT=pc USER_CFLAGS="-DEGUI_CONFIG_DEBUG_DIRTY_REGION_REFRESH=1 -DEGUI_CONFIG_DEBUG_REFRESH_DELAY=40"
make run APP=HelloDirty PORT=pc
```

收集脏区统计和 trace：

```bash
make all APP=HelloDirty PORT=pc USER_CFLAGS="-DEGUI_CONFIG_DEBUG_DIRTY_REGION_STATS=1 -DEGUI_CONFIG_DEBUG_DIRTY_REGION_TRACE=1"
make run APP=HelloDirty PORT=pc
```

首帧和切页时可能触发整页刷新；稳定动画帧应只集中在运动元素附近的小区域。
