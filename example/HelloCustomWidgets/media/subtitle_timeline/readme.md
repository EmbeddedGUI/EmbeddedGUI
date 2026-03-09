# subtitle_timeline

## 1. 为什么需要这个控件

`subtitle_timeline` 用来表达字幕 cue 在时间轴上的分布、当前 cue 焦点、前后 cue 预读和时长差异，适合字幕校对、台词跟播、转写审阅这类场景。

## 2. 为什么现有控件不够用

- `frame_scrubber` 强调缩略帧、播放头和 marker，不表达带文本的 cue 块。
- `waveform_strip` 强调波形振幅，不表达文本片段、起止时长和字幕切换。
- `range_band_editor` 强调范围选择，不表达连续字幕 cue 的阅读顺序。

## 3. 目标场景与示例概览

- 中央主卡：展示当前字幕 cue、speaker、正文、状态胶囊和时间轴主带。
- 左右预读窗：分别展示上一条和下一条 cue 的短预读卡。
- 时间轴主带：多段字幕块按时长比例排布，当前 cue 高亮。
- 底部反馈带：提示“上一条 / 当前推进 / 下一条”切换来源。

## 4. 视觉与布局规格

- 根控件尺寸：`240 x 280`
- 主卡区域：`184 x 154`
- 左右预读窗：`48 x 112`
- 主卡中段使用独立底板承托字幕时间轴
- 当前 cue 段必须明显强于普通 cue 段，但不能亮到抢过主卡正文
- 状态胶囊、侧窗顶部短条、底部反馈带都必须单独检查短词居中和安全边距

## 5. 控件清单

| 变量名 | 类型 | 尺寸 | 初始状态 | 用途 |
| --- | --- | --- | --- | --- |
| `subtitle_timeline` | `egui_view_subtitle_timeline_t` | `240 x 280` | `current=0` | 整个字幕时间轴控件 |
| `cues` | `egui_view_subtitle_timeline_cue_t[4]` | - | `Host / LIVE` | 四组字幕 cue 场景 |

## 6. 状态覆盖矩阵

| Cue | 状态词 | speaker | line | footer | emphasis | active_index |
| --- | --- | --- | --- | --- | --- | --- |
| 0 | `LIVE` | `Host` | `Camera pans across intro` | `00:01.2 - intro` | 0 | 1 |
| 1 | `SCAN` | `Guest` | `Reply subtitle stretches wide` | `00:02.6 - reply` | 1 | 2 |
| 2 | `SAFE` | `Editor` | `Locked cue keeps pause calm` | `00:01.6 - hold` | 2 | 0 |
| 3 | `SYNC` | `Narrator` | `Next cue lands on cut` | `00:02.1 - sync` | 1 | 3 |

## 7. `egui_port_get_recording_action()` 录制动作设计

- 首帧等待，确认默认态稳定
- 点击右预读窗一次，切到下一条 cue
- 点击左预读窗一次，回到上一条 cue
- 点击主卡一次，推进到下一条 cue
- 再点击右预读窗一次，覆盖另一组状态和文案长度

## 8. 编译、runtime、截图验收标准

```bash
make all APP=HelloCustomWidgets APP_SUB=media/subtitle_timeline PORT=pc
python scripts/code_runtime_check.py --app HelloCustomWidgets --app-sub media/subtitle_timeline --timeout 10 --keep-screenshots
```

验收重点：

- 不能黑屏、白屏、全空白
- 文本不能截断到不可读
- 当前 cue、前后 cue 和时间轴块必须能从截图里清楚区分
- 状态胶囊、短句预读窗和底部反馈带必须检查真实视觉居中
- 文字与胶囊或边框之间必须保留安全距离
- 每轮截图都归档进 `iteration_log/images/iter_xx/`

## 9. 已知限制与下一轮迭代计划

- 当前是离散 cue 切换，不是真实拖拽时间轴
- 侧窗和时间轴仍是抽象 cue 结构，不是真实字幕编辑器
- 后续若升级到框架层，可继续增加真实起止刻度、拖拽和多行字幕布局

## 10. 与现有控件的重叠分析与差异化边界

- 区别于 `frame_scrubber`：这里是字幕 cue 块和文本阅读顺序，不是缩略帧时间轴。
- 区别于 `waveform_strip`：这里强调文本片段和时长比例，不是音频能量。
- 区别于 `range_band_editor`：这里是离散 cue 切换和阅读流，不是双端范围操作。

## 11. 当前收口结论

- 已完成 30 次迭代闭环，最新一轮为 `PASS`
- `iteration_log/iteration_log.md` 已记录 30 轮，并按相对路径归档关键截图
- 当前版本重点收口了 cue 时长比例、当前 cue 锚点、预读窗层次、短词安全距离和底部反馈带留白
