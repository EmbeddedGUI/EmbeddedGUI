# radial_menu 迭代记录

## 记录规则

- 总迭代目标：30 次
- 每次迭代都要填写目标、改动、视觉验证、交互验证、结果
- 如有 runtime 截图，优先记录关键帧路径

| 迭代 | 目标 | 改动摘要 | 视觉验证 | 交互验证 | 关键产物 | 结果 |
| --- | --- | --- | --- | --- | --- | --- |
| 1 | 初始化设计文档与目录 | 建立 readme.md 与 iteration_log.md | 目录结构已创建，待首轮 runtime | 待实现交互 | example/HelloCustomWidgets/navigation/radial_menu/readme.md | PASS |
| 2 | 完成首版控件与示例 | 新增 egui_view_radial_menu.h/.c 与 test.c，支持展开、拖拽高亮、释放选择 | 首版布局可正常绘制 | 主菜单、紧凑菜单、禁用态录制动作已接通 | example/HelloCustomWidgets/navigation/radial_menu/egui_view_radial_menu.c | PASS |
| 3 | 完成首轮 runtime 验证 | 使用 HelloCustomWidgets navigation/radial_menu 完成编译与运行检查 | 成功输出 11 帧截图，未见黑屏 | 录制动作执行完成，交互帧已生成 | runtime_check_output/HelloCustomWidgets_navigation/radial_menu/default/frame_0000.png | PASS |
| 4 | 增强视觉层次 | 为外圈增加底板、chip 标签背景、中心光晕 | 标签可读性与层次感提升 | 原有拖拽高亮逻辑保持正常 | example/HelloCustomWidgets/navigation/radial_menu/egui_view_radial_menu.c | PASS |
| 5 | 调整示例观感并复测 runtime | 为标题/提示文本补颜色，并重新编译运行 | 11 帧截图再次通过，未见黑屏与中断 | 录制动作仍完整执行 | runtime_check_output/HelloCustomWidgets_navigation/radial_menu/default/frame_0003.png | PASS |
| 6 | 压缩紧凑态展开文本 | 新增字体 setter，紧凑态切换到更小字号，并放大主菜单尺寸 | 小尺寸控件静态观感更清晰 | 原录制动作仍有效 | example/HelloCustomWidgets/navigation/radial_menu/test.c | PASS |
| 7 | 收敛小尺寸标签重叠 | 小尺寸展开时仅显示热点标签，减少 chip 重叠 | 紧凑态展开截图明显更干净 | compact 拖拽帧仍能看出目标扇区 | runtime_check_output/HelloCustomWidgets_navigation/radial_menu/default/frame_0007.png | PASS |
| 8 | 增加展开态中间帧 | 在录制动作里为主菜单和紧凑菜单补 WAIT，确保 runtime 能捕捉展开状态 | 主菜单与紧凑菜单展开帧已可见 | 录制帧从 11 张提升到 14 张 | example/HelloCustomWidgets/navigation/radial_menu/test.c | PASS |
| 9 | 修正紧凑态热点标签裁切 | 将小尺寸热点标签改到控件内部上缘显示 | compact 展开帧里 Zoom 标签已完整可见 | 小尺寸拖拽目标更容易确认 | runtime_check_output/HelloCustomWidgets_navigation/radial_menu/default/frame_0009.png | PASS |
| 10 | 稳定 compact 热点标签展示 | 小尺寸热点标签固定到控件内部顶部，避免被裁切 | compact 展开时 Zoom 标签完整可见 | 小尺寸拖拽目标更容易识别 | runtime_check_output/HelloCustomWidgets_navigation/radial_menu/default/frame_0009.png | PASS |
| 11 | 补齐页面信息层 | 让标题与状态提示稳定显示在截图中，并缩短提示文案 | 初始态与展开态截图已能看到标题和当前状态 | 交互不受影响，runtime 仍通过 | runtime_check_output/HelloCustomWidgets_navigation/radial_menu/default/frame_0000.png | PASS |
| 12 | 调整中心文案策略 | 展开态无热点时显示当前选项，有热点时显示热点项，重复点击时显示 TAP | 主菜单展开态语义更一致 | 录制帧能直观看到当前/热点状态切换 | runtime_check_output/HelloCustomWidgets_navigation/radial_menu/default/frame_0004.png | PASS |
| 13 | 压低非热点项存在感 | 为当前/热点项增加外圈标记点，并降低普通标签 chip 存在感 | 展开态主次更明显，当前/热点更易读 | 主菜单悬停帧能更直观看到焦点 | runtime_check_output/HelloCustomWidgets_navigation/radial_menu/default/frame_0004.png | PASS |
| 14 | 继续压主菜单噪音 | 当前项在主菜单展开态不再重复显示 chip，普通标签更轻，仅保留当前/热点标记点 | 主菜单展开态更干净，中心信息更突出 | 录制帧里当前项与热点项更容易区分 | runtime_check_output/HelloCustomWidgets_navigation/radial_menu/default/frame_0006.png | PASS |
| 15 | 强化热点外圈反馈 | 为当前/热点段增加外圈弧形高亮，并降低分割线强度 | 展开态焦点更集中，分割线不再过硬 | 主菜单 hover 帧仍稳定可读 | runtime_check_output/HelloCustomWidgets_navigation/radial_menu/default/frame_0006.png | PASS |
| 16 | 继续减弱非焦点文本 | 普通标签整体外移并降低透明度，分割线进一步减弱 | 主菜单 hover 态更干净，普通项退到辅助层 | 焦点项仍清晰可辨，runtime 稳定通过 | runtime_check_output/HelloCustomWidgets_navigation/radial_menu/default/frame_0006.png | PASS |
| 17 | 优化折叠态语义 | 小尺寸菜单折叠时显示当前项，禁用菜单折叠时显示 LOCK | compact 与 disabled 的静态语义更清晰 | runtime 截图中可直接区分可交互与禁用状态 | runtime_check_output/HelloCustomWidgets_navigation/radial_menu/default/frame_0012.png | PASS |
| 18 | 统一页面级状态色 | 状态提示颜色跟随交互来源切换：主菜单蓝色、紧凑菜单橙色 | 页面状态反馈与控件配色更一致 | compact 选择完成后提示语更易关联来源 | runtime_check_output/HelloCustomWidgets_navigation/radial_menu/default/frame_0012.png | PASS |
| 19 | 提升禁用态可读性 | disabled 菜单中心文案改为更亮的 LOCK，并提升字号 | 禁用态静态截图更容易识别 | 不影响其他交互录制，runtime 继续通过 | runtime_check_output/HelloCustomWidgets_navigation/radial_menu/default/frame_0000.png | PASS |
| 20 | 增加装饰色配置能力 | 新增 decoration color API，让 compact 与 disabled 可单独设置边框/文字层次 | 底部两个菜单的风格差异更明确 | 不影响主菜单交互与 runtime 稳定性 | example/HelloCustomWidgets/navigation/radial_menu/egui_view_radial_menu.h | PASS |
| 21 | 统一 compact / disabled 页面语义 | compact 用金色装饰、disabled 用冷灰装饰，页面状态提示与卡片视觉更统一 | 页面整体观感更像成品示例 | runtime 帧里三个控件的角色区分更直观 | runtime_check_output/HelloCustomWidgets_navigation/radial_menu/default/frame_0012.png | PASS |
| 22 | 平衡底部两张小卡片 | 为 compact / disabled 增加独立装饰色配置，进一步拉开角色语义 | 底部两张卡片的身份区分更明显 | runtime 中主菜单不受影响，整体仍稳定 | runtime_check_output/HelloCustomWidgets_navigation/radial_menu/default/frame_0000.png | PASS |
| 23 | 增强示例讲解性 | 给底部两张卡片补 Compact / Disabled 标识，并重排底部布局 | 页面更像完整示例页，阅读顺序更清晰 | runtime 截图中两个辅控件角色一眼可辨 | runtime_check_output/HelloCustomWidgets_navigation/radial_menu/default/frame_0012.png | PASS |
| 24 | 修正底部示例标签裁切 | 重新分配 root / bottom_row / column 高度与间距，确保 Compact / Disabled 标签进入可视区域 | 初始帧已能完整看到两个底部说明标签 | 录制流程不受影响，runtime 继续稳定通过 | runtime_check_output/HelloCustomWidgets_navigation/radial_menu/default/frame_0000.png | PASS |
| 25 | 平衡页面纵向留白 | 压缩标题、状态提示与主控件之间的垂直间距，让页面信息层更紧凑 | 页面布局更完整，底部辅助区域不再像被截掉 | compact 完成态截图阅读顺序更清晰 | runtime_check_output/HelloCustomWidgets_navigation/radial_menu/default/frame_0012.png | PASS |
| 26 | 增强页面分组感 | 增加 `Compact and Disabled States` 分组说明标签，强化底部区域的示例语义 | 初始帧中主控件区与底部变体区分层更清晰 | 不影响交互录制与状态切换 | runtime_check_output/HelloCustomWidgets_navigation/radial_menu/default/frame_0000.png | PASS |
| 27 | 再平衡纵向留白 | 压缩标题、主控件、状态提示之间间距，为分组说明腾出空间 | 页面信息层更完整，底部说明不再显得突兀 | compact 完成态截图阅读顺序更顺 | runtime_check_output/HelloCustomWidgets_navigation/radial_menu/default/frame_0012.png | PASS |
| 28 | 补充交互引导说明 | 在页面顶部增加 `Tap center and drag to choose` 引导文案 | 首帧即能看懂主控件用法 | 不影响录制动作与状态切换 | runtime_check_output/HelloCustomWidgets_navigation/radial_menu/default/frame_0000.png | PASS |
| 29 | 增强区块分隔 | 在状态提示与底部变体区之间增加细分隔线，提升版面层级 | 页面分区更明确，底部区域不再像平铺堆叠 | runtime 截图阅读路径更顺 | runtime_check_output/HelloCustomWidgets_navigation/radial_menu/default/frame_0000.png | PASS |
| 30 | 最终收口验收 | 完成 30 次迭代后再次构建并运行验证，确认页面、交互、截图展示与追踪表同步完成 | 最新 14 帧 runtime 截图通过，页面结构与控件语义稳定 | 准备进入 completed 列表并执行单控件提交 | runtime_check_output/HelloCustomWidgets_navigation/radial_menu/default/frame_0012.png | PASS |
