# xy_pad

## 1. 为什么需要这个控件

`xy_pad` 用于表达二维连续输入场景，例如滤波器坐标、运动向量、灯光定位、机械臂平面校准等。这类场景不仅要看到当前位置，还要同时感知横纵两个维度的变化趋势、预设点以及当前状态。

## 2. 为什么现有控件不够用

- `slider` 和 `fader_bank` 只能表达单轴连续值，无法直接体现二维坐标关系。
- `button_matrix` 更适合离散命令，不适合连续拖拽定位。
- `swatch_picker` 关注离散选项选择，不包含空间坐标反馈。
- `step_sequencer` 表达的是时间步进结构，不是平面定位控制。

## 3. 目标场景与示例概览

- 主卡展示一个二维 XY 控制平面，包含准线、触点、halo ring 和 preset 文案。
- 左下 compact 卡展示 macro 预备坐标，并用 arm / scan 状态区分模式。
- 右下 compact 卡展示 latch 锁定坐标，并用 safe / warn 状态区分风险等级。
- 通过 3 组 snapshot 覆盖主卡、左卡、右卡在 runtime 下的切换效果。

## 4. 视觉与布局规格

- 根布局：`220 x 306`
- 顶部标题：`220 x 18`
- 顶部说明：`220 x 11`
- 主卡：`192 x 118`
- 主卡状态说明：`220 x 12`
- 中部分隔线：`142 x 2`
- 底部容器：`212 x 86`
- 左侧 compact：`108 x 80`
- 右侧 compact：`100 x 80`
- 验收重点：
  - 顶部标题、状态胶囊和边框之间必须保持稳定留白。
  - 主卡底部 `Filter/tilt` 双行文案要保持视觉居中和上下平衡。
  - compact 卡的 `Macro/Latch` 文字不能贴边，左右留白要均衡。

## 5. 控件清单

| 变量名 | 类型 | 尺寸 | 初始状态 | 用途 |
| --- | --- | --- | --- | --- |
| `root_layout` | `egui_view_linearlayout_t` | `220 x 306` | 垂直布局 | 页面骨架 |
| `title_label` | `egui_view_label_t` | `220 x 18` | `XY Pad` | 页面标题 |
| `guide_label` | `egui_view_label_t` | `220 x 11` | `Tap panels to rotate motion scenes` | 页面说明 |
| `xy_primary` | `egui_view_xy_pad_t` | `192 x 118` | snapshot 0 | 主二维控制卡 |
| `status_label` | `egui_view_label_t` | `220 x 12` | `Primary field live` | 主卡状态说明 |
| `section_divider` | `egui_view_line_t` | `142 x 2` | 静态 | 分隔上下区域 |
| `bottom_row` | `egui_view_linearlayout_t` | `212 x 86` | 横向布局 | 承载两个 compact 卡 |
| `xy_macro` | `egui_view_xy_pad_t` | `108 x 80` | compact snapshot 0 | macro 预备卡 |
| `xy_latch` | `egui_view_xy_pad_t` | `100 x 80` | locked snapshot 0 | latch 锁定卡 |

## 6. 状态覆盖矩阵

| 区域 | Snapshot | 状态文案 | Preset | 坐标 | 说明 |
| --- | --- | --- | --- | --- | --- |
| 主卡 | 0 | `LIVE` | `Filter A` | `(64,71)` | 正常实时控制 |
| 主卡 | 1 | `DRAG` | `Filter B` | `(82,43)` | 拖拽中 |
| 主卡 | 2 | `LIVE` | `Filter C` | `(34,28)` | 低位滑行 |
| 左卡 | 0 | `ARM` | `Macro 1` | `(26,74)` | 待触发 |
| 左卡 | 1 | `SCAN` | `Macro 2` | `(53,51)` | 扫描中 |
| 左卡 | 2 | `ARM` | `Macro 3` | `(73,29)` | 备用预设 |
| 右卡 | 0 | `SAFE` | `Latch A` | `(69,66)` | 安全锁定 |
| 右卡 | 1 | `WARN` | `Latch B` | `(41,35)` | 告警锁定 |
| 右卡 | 2 | `SAFE` | `Latch C` | `(21,57)` | 低位安全 |

## 7. `egui_port_get_recording_action()` 录制动作设计

1. 首帧等待 `400ms`，确保初始界面稳定。
2. 点击主卡，切换到主卡 snapshot 1。
3. 等待 `300ms`。
4. 点击左侧 compact 卡，切换到左卡 snapshot 1。
5. 等待 `300ms`。
6. 点击右侧 compact 卡，切换到右卡 snapshot 1。
7. 等待 `800ms`，输出关键截图。

## 8. 编译、runtime、截图验收标准

### 编译

```bash
make all APP=HelloCustomWidgets APP_SUB=input/xy_pad PORT=pc
```

### Runtime

```bash
python scripts/code_runtime_check.py --app HelloCustomWidgets --app-sub input/xy_pad --timeout 10 --keep-screenshots
```

### 验收标准

- 运行不能黑屏、白屏、卡死或崩溃。
- 主卡和两个 compact 卡都必须完整可见，不能被裁切。
- 顶部标题、状态胶囊、底部文案都要检查视觉居中，不允许只以“不截断”为通过标准。
- 文字与圆角边框、状态胶囊之间必须保留合理空隙，不能贴边。
- 每轮截图必须复制到 `iteration_log/images/iter_xx/`，并在 `iteration_log/iteration_log.md` 中记录结论。

## 9. 已知限制与下一轮迭代计划

- 当前 halo ring 仍是静态视觉反馈，后续可继续考虑更强的动态呼吸效果。
- compact 卡信息密度较高，后续若扩展状态种类，需要重新评估字体与留白。
- 如后续沉淀到框架层，可再规划真实拖拽事件和回调接口。

## 10. 与现有控件的重叠分析与差异化边界

- 区别于 `slider` / `fader_bank`：核心差异在二维连续控制，而不是单轴数值调节。
- 区别于 `swatch_picker`：这里强调平面坐标与空间反馈，不是离散样本选择。
- 区别于 `step_sequencer`：这里关注平面定位和状态反馈，不包含时间步进语义。
- 区别于 `button_matrix`：这里强调连续平面操作，而不是离散命令矩阵。
