# chapter_strip

## 1. 为什么需要这个控件？
`chapter_strip` 用来表达“媒体章节导航条”的语义：用户可以在播客、课程回放、预告片或审阅片段中，以离散章节为单位快速切换，而不是进入连续时间轴编辑。它适合需要快速浏览结构、定位章节边界、保持低噪音界面的场景。

## 2. 为什么现有控件不够用？
- `transport_bar` 强调 previous / play / next 与 seek，不表达章节结构。
- `frame_scrubber` 强调连续时间轴和逐帧浏览，不适合低成本的章节跳转。
- `subtitle_timeline` 强调字幕 cue 文本与前后预读，不是标准章节导航条。
- `pips_pager` 是通用分页点阵，不具备媒体章节摘要与章节时间标记语义。

## 3. 目标场景与示例概览
- 标准态：展示当前章节摘要、章节 rail、previous / next 两侧按钮。
- 状态栏：同步回显当前章节编号、当前聚焦部件和章节标题。
- compact 预览：保留章节 rail 结构，但压缩为轻量预览卡。
- read-only 预览：保留视觉层级，但冻结输入，验证禁用边界。

## 4. 视觉与布局规格
- 根容器尺寸：`224 x 312`
- 主控件尺寸：`196 x 126`
- 底部预览卡尺寸：`104 x 96`
- 底部预览本体尺寸：`92 x 70`
- 顶部结构：标题、guide、section label
- 主卡要求：章节摘要区、active bridge、章节编号进度胶囊、时间标记、章节 rail 背板与前后切换按钮完整可见
- 紧凑态要求：缩小后的章节 rail 仍能看出当前章节与相邻章节边界，并通过淡色 rail 背板保持连接感
- 底部对照要求：`compact / read-only` 预览放进独立浅色 preview card，标签、控件本体与卡片边界要有清晰层次
- 卡内留白要求：header pill 与预览本体之间保留稳定竖向间距，卡片底部也要留下轻量缓冲，避免 preview body 贴近外层边界
- 节奏要求：状态回显使用居中的 status pill，divider 保持更短更轻，避免主卡与底部预览之间出现松散空带
- 顶部层级要求：guide 使用短句 chip，`Standard` 使用独立 section pill，标题、guide 与 section label 要形成清晰三层层级
- 主卡节奏要求：`Reference track` 标题与 summary 之间使用更紧的垂直间距，并通过短锚线把 title、summary、rail 组织成连续阅读路径
- 主卡横向节奏要求：summary 内部使用左侧章节信息列与右侧 meta panel 两列组织，中间保留细分隔节奏，eyebrow pill 宽度随文案收敛，避免标题区漂浮
- 主卡底部要求：helper 说明行使用低噪音 footer strip 承接 rail，下方 copy 保持短句，避免再次出现截断或悬空灰字
- Rail 层级要求：标准态 inactive chapter 需要比 active chapter 更短、更轻，编号与顶部亮条都要退到次级层，确保当前章节始终是 row 的第一视觉焦点
- 辅助按钮要求：标准态 previous / next 按钮要作为 rail 的辅助入口存在，idle 态不应比 active chapter 更醒目，但 chevron 仍需保持清晰可辨与焦点边界一致
- 标题行要求：`Reference track` 这类 section header 需要带一个低噪音 accent marker，且 marker、标题文本和短锚线要共享同一条起始节奏，避免标题再次漂浮
- 底部对照卡要求：`Compact / Read only` 头部使用独立 header pill，并和外层 preview card 保持稳定上下间距，让底部两张卡明确呈现为成组对照面板
- 状态回显要求：中部 `status pill` 需要跟随当前 session 色相做轻量变化，帮助用户感知轨道切换，但填充和描边都必须保持低噪音，不压过主卡和 rail
- 中段过渡要求：`status pill` 下方 divider 需要更短、更轻，并和底部 preview cards 保持稳定的中轴过渡，避免中段再次出现松散断层
- 中轴收口要求：主卡底边、`status pill` 和 divider 需要形成逐级收束的中心桥接，避免中段出现“上重下轻”的断层感
- 底部横向关系要求：两张 preview cards 需要更像一组并列对照面板，卡间距应略收，外侧留白保持稳定，避免左右卡被拉得太散
- 底部对齐要求：两张 preview cards 的 header pill 与内层 preview body 需要尽量共享一致的横向中心和左右呼吸感，避免左右卡内部节奏不一致
- 卡头留白要求：preview card 顶部描边到 header pill 之间要保留稳定的 cap space，避免标题胶囊贴近卡顶导致头部显得发闷
- 头身过渡要求：header pill 与 preview body 之间的竖向距离要像同一张卡内部的自然接力，既不能挤在一起，也不能断开成两层孤立模块
- 底边缓冲要求：preview body 与卡片底边之间仍需保留轻量缓冲，但不能因为底部空带过重而削弱卡片内部重心
- 色阶一致性要求：底部 preview cards 的外层 card、描边和内层 preview body 需要尽量保持同一浅色系，避免出现外层偏灰而内层突然跳白的断层
- 卡头灰阶要求：`Compact / Read only` 的 header pill 需要比下方 preview body 略深半级，尤其是 read-only 卡头要保留清楚但低噪音的中性分层
- 只读卡头识别要求：`Read only` header pill 的文字与描边需要在低对比灰阶里仍能读清，不可因为过度融入卡身而失去卡头边界
- 卡头内缩要求：`Read only` 这类更长标题的 header pill 可以为光学平衡略放宽 1-2px，但需要继续与 `Compact` 保持同组居中感，避免一宽一窄过于明显
- 右卡纵向重心要求：更宽的 `Read only` header pill 不能把右卡的视觉重心顶高，必要时应通过 header 上下 margin 微调，让卡头到 body 的纵向接力继续自然

## 5. 控件清单
| 变量名 | 类型 | 尺寸 | 初始状态 | 用途 |
| --- | --- | --- | --- | --- |
| `chapter_strip_primary` | `egui_view_chapter_strip_t` | `196 x 126` | `Podcast edit / chapter 02` | 标准主态 |
| `chapter_strip_compact` | `egui_view_chapter_strip_t` | `92 x 70` | `Pocket mix / compact` | compact 对照 |
| `chapter_strip_locked` | `egui_view_chapter_strip_t` | `92 x 70` | `Audit / read-only` | read-only 对照 |
| `primary_sessions` | `chapter_strip_session_t[3]` | - | `Podcast edit` | 主态章节映射切换 |
| `compact_sessions` | `chapter_strip_session_t[2]` | - | `Pocket mix` | 紧凑态切换 |

## 6. 状态覆盖矩阵
| Session | Snapshot | 关键状态 | 语义 |
| --- | --- | --- | --- |
| `Podcast edit` | `Host / Scene setup` | 默认章节态 | 标准章节导航 |
| `Podcast edit` | `Arrow Right` | 章节切换 | 章节 rail 焦点与摘要同步 |
| `Podcast edit` | `End` | 尾章节 | 尾部边界与 next 禁用 |
| `Trailer reel` | `Guide switch` | guide 切换 | 不同章节集合复用 |
| `Compact` | `Story` | 小尺寸对照 | 保留章节 rail 语义 |
| `Read only` | `Audit` | 禁用边界 | 视觉保留、输入冻结 |

## 7. `egui_port_get_recording_action()` 录制动作设计
- 首帧等待并截图，确认默认 `Podcast edit` 状态。
- 发送 `Right`，验证章节从 `02` 切到 `03`。
- 发送 `End`，验证尾章节与 next 边界。
- 点击 guide，切换到 `Trailer reel` 映射。
- 点击 compact 标签，切换底部 compact 预览集合。

## 8. 编译、runtime、截图验收标准
```bash
make all APP=HelloCustomWidgets APP_SUB=media/chapter_strip PORT=pc
python scripts/code_runtime_check.py --app HelloCustomWidgets --app-sub media/chapter_strip --timeout 10 --keep-screenshots
make all APP=HelloUnitTest PORT=pc_test
output/main.exe
```

验收重点：
- 主卡、章节摘要、章节 rail 和底部双预览全部可见，无黑屏和裁切
- 章节编号、时间短标记、previous / next 图标居中，左右留白平衡
- 文本与圆角边框之间留有合理空隙，不出现贴边
- 录制序列能直观看出章节切换、尾章节边界、guide 换组和 compact 切换

## 9. 已知限制与下一轮迭代计划
- 当前版本仅表达离散章节导航，不接入真实媒体解码或 seek 数据。
- 章节时间标记使用静态文本，不做动态进度条或缓冲段。
- 后续如需增强，可继续补章节缩略图、章节完成态或章节批注标记。
- 30 轮基础迭代已完成；如需继续做验收收口，可优先观察底部两张 preview cards 的整体对照关系是否已经足够稳定，再决定是否进入最终提交整理。

## 10. 与现有控件的重叠分析与差异化边界
- 区别于 `transport_bar`：这里只做章节导航，不承载播放暂停与连续 seek。
- 区别于 `frame_scrubber`：这里不做连续时间轴编辑和逐帧精确定位。
- 区别于 `subtitle_timeline`：这里不强调 cue 文本流，只保留章节块语义。
- 区别于 `pips_pager`：这里不是通用分页，而是带摘要和时间标记的媒体章节 rail。

## 11. 参考设计系统与开源母本
- Fluent 2 / Windows Media 章节导航语义
- WinUI / 媒体浏览卡片与分段 rail 的低噪音布局思路

## 12. 对应组件命名，以及本次保留的核心状态
- 组件名：`chapter_strip`
- 保留状态：`previous`、`chapter`、`next`、`compact`、`read_only`
- 保留交互：`Left/Right/Home/End/Tab/Enter/Space/Plus/Minus/Escape`，以及前后按钮和章节块点击

## 13. 相比参考原型删减了哪些效果或装饰？
- 删除真实封面缩略图、动态缓冲条和复杂渐变动效。
- 删除 hover 动画、系统级媒体转场和多层阴影。
- 删除章节搜索、批量编辑、右键菜单等附属功能，只保留章节导航核心。

## 14. EGUI 适配时的简化点与约束
- 不引入外部图片资源，全部使用纯色卡片与文字层级表达。
- 通过 `current_index + current_part` 统一键盘与触摸状态机。
- compact / read-only 在同一控件内切换，避免拆成多套绘制逻辑。
