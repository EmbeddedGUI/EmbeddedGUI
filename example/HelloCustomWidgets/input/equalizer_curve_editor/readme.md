# equalizer_curve_editor

## 1. 为什么需要这个控件

`equalizer_curve_editor` 用来表达多频段均衡器的增益曲线、当前 band 焦点、前后 band 预读和参数摘要，适合音频调音、预设审校和设备面板这类场景。

## 2. 为什么现有控件不够用

- `xy_pad` 强调二维连续定位，不表达多段频率曲线。
- `range_band_editor` 强调区间范围，不表达离散 band 节点和曲线趋势。
- `waveform_strip` 和 `level_meter` 强调音频结果，不表达 EQ 参数编辑本身。

## 3. 目标场景与示例概览

- 中央主卡：展示当前 EQ preset、状态胶囊、曲线编辑区和参数摘要。
- 左右预读窗：分别展示前一档和后一档 band 的紧凑预读。
- 曲线编辑区：5 段 band 节点和连接曲线，当前 band 高亮。
- 底部反馈带：反馈左切换、右切换或主卡推进。

## 4. 视觉与布局规格

- 根控件尺寸目标：`240 x 280`
- 主卡优先承载曲线和 band 信息，不做列表式布局
- 当前 band 节点必须明确强于其它节点，但不能破坏整体曲线阅读
- 状态胶囊、侧窗短条、底部反馈带必须检查短词居中和边框安全距离
- 曲线和节点不能贴边，左右和上下都要保留安全留白

## 5. 控件清单

| 变量名 | 类型 | 尺寸 | 初始状态 | 用途 |
| --- | --- | --- | --- | --- |
| `equalizer_curve_editor` | `egui_view_equalizer_curve_editor_t` | `240 x 280` | `current=0` | 整个 EQ 曲线控件 |
| `bands` | `egui_view_equalizer_curve_editor_band_t[]` | - | 第一组 preset | 频段和增益数据 |

## 6. 状态覆盖矩阵

| 状态 | 当前 band | 左预读 | 右预读 | 曲线反馈 |
| --- | --- | --- | --- | --- |
| 默认态 | 当前 band 高亮 | 前一档 band | 后一档 band | 当前节点聚焦 |
| 左切换 | 前一档高亮 | 更前一档 | 原当前档 | 左侧切换反馈 |
| 右切换 | 后一档高亮 | 原当前档 | 更后一档 | 右侧切换反馈 |
| 主卡推进 | 下一档高亮 | 原当前档 | 更后一档 | 中央推进反馈 |

## 7. `egui_port_get_recording_action()` 录制动作设计

- 首帧等待，确认默认态稳定
- 点击右预读窗，切到下一档 band
- 点击左预读窗，回到上一档 band
- 点击主卡，推进到下一档 band
- 再连续点击右预读窗，覆盖 `SAFE` 和 `SYNC`，确保 4 个 preset 都进入 runtime 截图

## 8. 编译、runtime、截图验收标准

```bash
make all APP=HelloCustomWidgets APP_SUB=input/equalizer_curve_editor PORT=pc
python scripts/code_runtime_check.py --app HelloCustomWidgets --app-sub input/equalizer_curve_editor --timeout 10 --keep-screenshots
```

验收重点：

- 不能黑屏、白屏、全空白
- 曲线、节点、摘要和底部反馈带都必须完整可见
- 当前 band 节点、左右预读和状态切换必须能从截图中连续看出变化
- 4 个 preset 必须都被录制动作覆盖，不能只验证前 3 档
- 状态胶囊、短词按钮和底部反馈带必须检查真实视觉居中
- 文字与胶囊或边框之间必须保留安全距离
- 每轮截图都归档进 `iteration_log/images/iter_xx/`

## 9. 已知限制与下一轮迭代计划

- 首版先做离散 band 切换，不做真实拖拽节点
- 首版先验证曲线层次、节点高亮和摘要布局
- 侧窗下半区暂不增加新图元；新增 1px 装饰线会触发黑屏，后续统一保持 2px 及以上安全线条
- 后续若升级到框架层，可继续增加真实拖拽、Q 值和频率刻度

## 10. 与现有控件的重叠分析与差异化边界

- 区别于 `xy_pad`：这里是多频段曲线，不是二维自由控制面板。
- 区别于 `range_band_editor`：这里强调 band 节点和曲线趋势，不是双端区间。
- 区别于 `waveform_strip` / `level_meter`：这里表达参数编辑，不是音频结果显示。
