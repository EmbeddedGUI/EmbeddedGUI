# jog_shuttle_wheel

## 1. 为什么需要这个控件

`jog_shuttle_wheel` 用来表达媒体 transport 场景里的环形穿梭拨轮、当前模式、前后模式预读和中心 hub，适合剪辑台、录音机、采样器和播控面板。

## 2. 为什么现有控件不够用

- `radial_menu` 强调扇区选择，不表达连续 shuttle wheel 语义。
- `frame_scrubber` 强调线性时间轴，不表达环形拨轮和中心 hub。
- `xy_pad` 强调二维平面定位，不表达 transport 模式切换。

## 3. 目标场景与示例概览

- 中央主拨轮：展示环形 shuttle ring、焦点 marker、中心 hub 和当前模式摘要。
- 左右预读卡：分别展示前一档和后一档 transport mode。
- 顶部状态胶囊：展示当前短词状态，如 `JOG`、`TRIM`、`LOOP`、`CUE`。
- 底部反馈带：反馈左切、右切和中心推进。

## 4. 视觉与布局规格

- 根控件尺寸目标：`240 x 280`
- 中央主拨轮必须是唯一主焦点，左右预读只做从属信息
- 环形 marker 和中心 hub 必须能一眼读出当前模式焦点
- 顶部短词胶囊、底部反馈带和 footer 参数胶囊都要检查真实视觉居中
- 文案与圆角边框之间必须保留安全距离，不能只满足“不截断”

## 5. 控件清单

| 变量名 | 类型 | 尺寸 | 初始状态 | 用途 |
| --- | --- | --- | --- | --- |
| `jog_shuttle_wheel` | `egui_view_jog_shuttle_wheel_t` | `240 x 280` | `current=0` | 整个 transport 拨轮控件 |
| `modes` | `egui_view_jog_shuttle_wheel_mode_t[]` | - | 第一组模式 | transport mode 和 ring 焦点数据 |

## 6. 状态覆盖矩阵

| 状态 | 当前 mode | 左预读 | 右预读 | 环形反馈 |
| --- | --- | --- | --- | --- |
| 默认态 | 当前 mode 高亮 | 前一档 mode | 后一档 mode | 当前 marker 聚焦 |
| 左切换 | 前一档高亮 | 更前一档 | 原当前档 | 左侧切换反馈 |
| 右切换 | 后一档高亮 | 原当前档 | 更后一档 | 右侧切换反馈 |
| 中心推进 | 下一档高亮 | 原当前档 | 更后一档 | 中央推进反馈 |

## 7. `egui_port_get_recording_action()` 录制动作设计

- 首帧等待，确认默认态稳定
- 点击右预读卡，切到下一档 mode
- 点击左预读卡，回到上一档 mode
- 点击中心拨轮，推进到下一档 mode
- 再连续点击右预读卡，覆盖 `LOOP` 和 `CUE`，确保 4 个 mode 都进入截图

## 8. 编译、runtime、截图验收标准

```bash
make all APP=HelloCustomWidgets APP_SUB=media/jog_shuttle_wheel PORT=pc
python scripts/code_runtime_check.py --app HelloCustomWidgets --app-sub media/jog_shuttle_wheel --timeout 10 --keep-screenshots
```

验收重点：

- 不能黑屏、白屏、全空白
- 主拨轮、中心 hub、左右预读卡和底部反馈带都必须完整可见
- 4 个 mode 都必须被录制动作覆盖
- 顶部短词胶囊、footer 参数胶囊和底部反馈带都必须检查真实视觉居中
- 文字与圆形或胶囊边框之间必须保留安全距离
- 每轮截图都归档进 `iteration_log/images/iter_xx/`

## 9. 已知限制与下一轮迭代计划

- 首版先做离散 mode 切换，不做真实旋转拖拽
- 首版先验证拨轮层次、marker 焦点和中心 hub
- 后续若升级到框架层，可继续增加真实旋转惯性、刻度文本和速度档位

## 10. 与现有控件的重叠分析与差异化边界

- 区别于 `radial_menu`：这里是 transport shuttle wheel，不是菜单扇区选择。
- 区别于 `frame_scrubber`：这里是环形拨轮，不是线性帧时间轴。
- 区别于 `xy_pad`：这里强调 mode 切换和 transport hub，不是二维连续控制。
