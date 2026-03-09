# frame_scrubber

## 1. 为什么需要这个控件

`frame_scrubber` 用来表达视频或动画时间轴上的缩略帧浏览、播放头定位和标记点跳转，适合剪辑、回放、审校这类需要按帧定位的场景。

## 2. 为什么现有控件不够用

- `waveform_strip` 强调音频振幅和播放头，不表达缩略帧序列。
- `range_band_editor` 强调范围选区，不表达逐帧内容预览。
- `coverflow_strip` 强调中心主卡轮转，不是连续时间轴语义。

## 3. 目标场景与示例概览

- 中央主卡：展示当前片段标题、状态胶囊、摘要、缩略帧带和底部 cue。
- 左右预读窗：表达前后片段预览，并承担快速切换入口。
- 缩略帧带：6 格缩略帧 + 播放头 + marker，用于说明当前时间位置和标记点。
- 底部状态带：反馈本次点击来自左预读、右预读还是主卡推进。

## 4. 视觉与布局规格

- 根控件尺寸：`240 x 280`
- 主卡区域：`184 x 154`
- 左右预读窗：`48 x 112`
- 主卡中段保留独立工作区，用于承托缩略帧带
- 右上状态胶囊、侧窗顶部短条、底部状态带必须检查视觉居中和边框安全距离
- 播放头、marker、当前帧和普通帧必须形成明确层次，不允许混成一片

## 5. 控件清单

| 变量名 | 类型 | 尺寸 | 初始状态 | 用途 |
| --- | --- | --- | --- | --- |
| `frame_scrubber` | `egui_view_frame_scrubber_t` | `240 x 280` | `current=0` | 整个 frame scrubber |
| `snapshots` | `egui_view_frame_scrubber_snapshot_t[4]` | - | `Edit Reel` | 四组回放场景 |

## 6. 状态覆盖矩阵

| Snapshot | 状态词 | Summary | Footer | accent_mode |
| --- | --- | --- | --- | --- |
| 0 | `LIVE` | `Playhead centered on shot` | `live reel` | 0 |
| 1 | `SCAN` | `Marker jumps to bright scene` | `scan cue` | 1 |
| 2 | `SAFE` | `Read-only hold keeps clip calm` | `safe trim` | 2 |
| 3 | `SYNC` | `Preview windows queue next frame` | `sync roll` | 1 |

## 7. `egui_port_get_recording_action()` 录制动作设计

- 首帧等待，确认默认态稳定
- 点击右预读窗一次，切到下一个 snapshot
- 点击左预读窗一次，回退到上一个 snapshot
- 点击主卡一次，推进播放头和当前快照
- 再点击右预读窗一次，覆盖另一组状态

## 8. 编译、runtime、截图验收标准

```bash
make all APP=HelloCustomWidgets APP_SUB=media/frame_scrubber PORT=pc
python scripts/code_runtime_check.py --app HelloCustomWidgets --app-sub media/frame_scrubber --timeout 10 --keep-screenshots
```

验收重点：

- 不能黑屏、白屏、全空白
- 主卡、左右预读窗、缩略帧带和底部状态带都必须完整可见
- 右上状态胶囊、侧窗顶部短条、底部状态带文字区必须检查真实视觉居中
- 短词和边框之间必须保留安全距离，不能出现贴边或一侧明显更窄
- 播放头、marker、当前帧切换必须能从截图中连续看出变化
- 每轮截图都要归档进 `iteration_log/images/iter_xx/`

## 9. 已知限制与下一轮迭代计划

- 当前是离散 snapshot 切换，不是真实拖拽 scrub
- 缩略帧仍是抽象结构，不是真实图像帧
- 后续若升级到框架层，可继续增加真实时间刻度、拖拽和长按预览

## 10. 与现有控件的重叠分析与差异化边界

- 区别于 `waveform_strip`：这里是缩略帧时间轴，不是音频波形。
- 区别于 `range_band_editor`：这里强调播放头、marker 和逐帧预览，不是区间选择。
- 区别于 `coverflow_strip`：这里是线性时间轴和预读窗组合，不是中心卡片轮转。

## 11. 当前收口结论

- 已完成 30 次迭代闭环，最新一轮为 `PASS`
- `iteration_log/iteration_log.md` 已记录 30 轮，并按相对路径归档关键截图
- 当前版本重点收口了缩略带层次、短词胶囊安全距离、底部状态带留白和侧窗层次
