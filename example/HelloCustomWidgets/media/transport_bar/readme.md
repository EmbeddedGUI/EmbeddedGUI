# transport_bar

## 1. 为什么需要这个控件？
`transport_bar` 用来表达“标准媒体播放控制栏”的语义：中心播放/暂停、前后跳转、进度 seek rail 与当前时长反馈聚合在同一块低噪音面板中，适合播客、音乐、剪辑审阅和节目播控等需要快速播放控制的场景。

## 2. 为什么现有控件不够用？
- `jog_shuttle_wheel` 强调环形穿梭和速度感，不是标准线性 transport bar。
- `frame_scrubber` 强调逐帧时间轴编辑，不是播放控制栏。
- `waveform_strip` 强调波形展示和播放头，不承担 previous / play / next 组合语义。
- `command_bar` 是通用命令工具栏，不包含标准媒体 seek 反馈。

## 3. 目标场景与示例概览
- 标准态：展示 badge、标题、时长胶囊、seek rail 与 previous / play/pause / next。
- 状态栏：同步回显当前轨道、当前聚焦部件和时间位置。
- compact 预览：保留核心 transport 语义，但压缩为更短标题和短时间胶囊。
- read-only 预览：保留视觉结构，但冻结交互输入，验证禁用边界。

## 4. 视觉与布局规格
- 根容器尺寸：`224 x 312`
- 主控件尺寸：`196 x 132`
- 底部预览尺寸：`104 x 72`
- 顶部结构：标题、guide、section label
- 主卡要求：badge、time pill、title、subtitle、seek rail、center hub 都需要完整可见
- 紧凑态要求：短标题、短时间胶囊、缩小按钮仍保持居中与左右留白平衡

## 5. 控件清单
| 变量名 | 类型 | 尺寸 | 初始状态 | 用途 |
| --- | --- | --- | --- | --- |
| `transport_bar_primary` | `egui_view_transport_bar_t` | `196 x 132` | `Studio mix / playing` | 标准主态 |
| `transport_bar_compact` | `egui_view_transport_bar_t` | `104 x 72` | `Pocket / playing` | compact 对照 |
| `transport_bar_locked` | `egui_view_transport_bar_t` | `104 x 72` | `Audit / paused` | read-only 对照 |
| `primary_tracks` | `transport_bar_track_t[3]` | - | `Studio mix` | 主态轨道切换 |
| `compact_tracks` | `transport_bar_track_t[2]` | - | `Pocket` | 紧凑态切换 |

## 6. 状态覆盖矩阵
| Track | Snapshot | 关键状态 | 语义 |
| --- | --- | --- | --- |
| `Studio mix` | `Live` | 默认播放态 | 标准 transport 卡 |
| `Studio mix` | `Paused` | 播放/暂停切换 | center hub 反馈 |
| `Studio mix` | `Seek` | 位置推进 | seek rail 焦点与时间更新 |
| `Podcast edit` | `Voice` | guide 切换 | 不同节目数据轨复用 |
| `Compact` | `Clip` | 小尺寸对照 | 保留 transport 语义 |
| `Read only` | `Audit` | 禁用边界 | 视觉保留、交互冻结 |

## 7. `egui_port_get_recording_action()` 录制动作设计
- 首帧等待并截图，确认默认 `Studio mix` 播放态。
- 发送 `Space`，验证中心 hub 从播放切到暂停。
- 发送 `Plus`，验证 seek rail 位置推进。
- 点击 guide，切换到 `Podcast edit` 轨道。
- 点击 compact 标签，切换底部 compact 预览。

## 8. 编译、runtime、截图验收标准
```bash
make all APP=HelloCustomWidgets APP_SUB=media/transport_bar PORT=pc
python scripts/code_runtime_check.py --app HelloCustomWidgets --app-sub media/transport_bar --timeout 10 --keep-screenshots
make all APP=HelloUnitTest PORT=pc_test
output/main.exe
```

验收重点：
- 主卡、seek rail、按钮和底部双预览全部可见，无黑屏或裁切
- badge、time pill、短标题与圆形 hub 视觉居中，左右留白平衡
- 文本与圆角边框之间有合理空隙，不出现贴边
- 录制帧能直接看出播放/暂停、seek、换轨和 compact 切换

## 9. 已知限制与下一轮迭代计划
- 当前版本使用纯绘制卡片模拟 transport，不接入真实音频资源。
- previous / next 用固定秒数跳转，不扩展到真实播放列表切歌。
- 后续如需更接近真实播放器，可继续补 chapter marker、buffer rail 和动画过渡。

## 10. 与现有控件的重叠分析与差异化边界
- 区别于 `jog_shuttle_wheel`：这里保留标准直线 seek rail，不做环形穿梭。
- 区别于 `frame_scrubber`：这里不做逐帧编辑与帧精确定位。
- 区别于 `waveform_strip`：这里不强调波形振幅，只表达 transport controls。
- 区别于 `command_bar`：这里的核心是播放控制与时间反馈，不是通用工具命令栏。

## 11. 参考设计系统与开源母本
- Fluent 2 / Windows media transport 语义
- WinUI / Windows media playback transport control 布局惯例

## 12. 对应组件命名，以及本次保留的核心状态
- 组件名：`transport_bar`
- 保留状态：`previous`、`play_pause`、`next`、`seek`、`compact`、`read_only`
- 保留交互：`Left/Right/Home/End/Tab/Enter/Space/Plus/Minus/Escape`，以及按钮点击与 seek 点击/拖动

## 13. 相比参考原型删减了哪些效果或装饰
- 删除真实封面图、缓冲进度和复杂波形背景。
- 删除 hover 动画、阴影过渡和系统媒体动画。
- 删除播放列表、音量、字幕等附属部件，只保留 transport 核心。

## 14. EGUI 适配时的简化点与约束
- 不引入外部图片资源，全部用纯色卡面和文字层级表达。
- 通过 `current_part + position_seconds + playing` 统一键盘与触摸状态机。
- compact / read-only 直接在同一控件内切换，避免拆出多套绘制逻辑。
