# split_button 自定义控件设计说明

## 参考来源

- 参考设计系统：`Fluent 2`
- 参考开源库：`WPF UI`
- 次级补充参考：`ModernWpf`
- 对应组件名：`SplitButton`
- 本次保留状态：`save / share / export / archive`、`compact`、`disabled`
- 删除效果：Acrylic、真实图标资源、复杂下拉菜单弹层、桌面级 hover / pressed 动画、完整命令栏整合
- EGUI 适配说明：保留“主动作 + 菜单入口”的复合按钮语义、主段 / 菜单段双焦点、compact / disabled 对照；在 `240 x 320` 下优先保证段落边界清楚、短文案稳定和键盘切焦点可辨

## 1. 为什么需要这个控件？

`split_button` 用来表达“有一个默认主动作，同时还有一组次级动作藏在菜单里”的标准页内复合按钮。它适合保存、分享、导出、归档这类高频操作：默认点击直接执行主动作，而右侧菜单段保留更多选择。

## 2. 为什么现有控件不够用

- 普通 `button` 只有单一动作，不表达“默认动作 + 更多动作”的双入口
- `menu_flyout` 是单独的弹出命令面板，不是页内复合按钮
- `command_bar` 承担的是常驻工具栏语义，不是单个复合按钮语义
- `number_box`、`segmented_control` 等输入控件也不覆盖 SplitButton 的主段 / 菜单段交互

## 3. 目标场景与示例概览

- 主区域展示标准 `split_button`，覆盖 `save / share / export / archive` 四组 snapshot
- 左下 `Compact` 预览展示紧凑版 split button
- 右下 `Disabled` 预览展示禁用态 split button
- 主卡支持触摸切换 `primary / menu` 焦点
- 主卡支持 `Left / Right / Tab / Home / End` 键盘切换焦点
- 点击 guide 文案轮换主卡 snapshot；点击 `Compact` 标题轮换 compact snapshot

目录：

- `example/HelloCustomWidgets/input/split_button/`

## 4. 视觉与布局规格

- 画布：`240 x 320`
- 根布局：`224 x 292`
- 页面结构：标题 -> guide -> `Standard` -> 主卡 -> 状态文案 -> 分隔线 -> `Compact / Disabled` 双预览
- 主卡区域：`194 x 74`
- 底部双预览容器：`218 x 82`
- `Compact` 预览：`106 x 66`
- `Disabled` 预览：`106 x 66`
- 视觉规则：
  - 使用浅灰 page panel + 白色复合按钮卡，避免回到 HMI / 工业面板风格
  - 保留 SplitButton 的左右双段结构、垂直分隔和右侧下拉入口
  - 强调主动作段，但不做桌面级强 hover、阴影扩散和动画
  - compact 版本继续保留双段结构，只缩短文案并压缩辅助文案

## 5. 控件清单

| 变量名 | 类型 | 尺寸 (W x H) | 初始状态 | 用途 |
| --- | --- | ---: | --- | --- |
| `root_layout` | `egui_view_linearlayout_t` | 224 x 292 | enabled | 页面根布局 |
| `title_label` | `egui_view_label_t` | 224 x 18 | `Split Button` | 页面标题 |
| `guide_label` | `egui_view_label_t` | 224 x 12 | 可点击 | 轮换主卡 snapshot |
| `button_primary` | `egui_view_split_button_t` | 194 x 74 | `save` | 标准 split button |
| `status_label` | `egui_view_label_t` | 224 x 12 | `Save draft: Save` | 当前焦点状态 |
| `button_compact` | `egui_view_split_button_t` | 106 x 66 | `compact save` | 紧凑预览 |
| `button_disabled` | `egui_view_split_button_t` | 106 x 66 | `disabled` | 禁用预览 |

## 6. 状态覆盖矩阵

| 状态 / 区域 | 主卡 | Compact | Disabled |
| --- | --- | --- | --- |
| 默认态 | `save` | `compact save` | `disabled` |
| 切换 1 | `share` | 保持 | 保持 |
| 切换 2 | `export` | 保持 | 保持 |
| 切换 3 | `archive` | 保持 | 保持 |
| 紧凑切换 | 保持 | `compact review` | 保持 |
| 点击主段 | 更新 focus | 更新 focus | 不可交互 |
| 点击菜单段 | 更新 focus | 更新 focus | 不可交互 |
| 键盘焦点 | `Left / Right / Tab / Home / End` | 同步支持 | 禁用 |

## 7. `egui_port_get_recording_action()` 录制动作设计

1. 首帧等待并抓取 `save` snapshot
2. 切到 `share`
3. 切到 `export`
4. 切到底部 `Compact` 第二组 snapshot
5. 回到主卡 `archive`
6. 尾帧等待，供 runtime 抓最终截图

录制采用 setter + `recording_request_snapshot()`，不依赖真实触摸命中，保证回放稳定。

## 8. 编译、runtime、截图验收标准

```bash
make all APP=HelloCustomWidgets APP_SUB=input/split_button PORT=pc
python scripts/code_runtime_check.py --app HelloCustomWidgets --app-sub input/split_button --timeout 10 --keep-screenshots
```

验收重点：

- 主卡、compact、disabled 三个 split button 都必须完整可见
- 主段与菜单段边界清楚，不能糊成单按钮
- 短文案 `Save / Share / Export / Archive` 需要逐项检查居中与左右留白
- 右侧下拉入口必须看得出独立焦点，不可变成装饰性小图标
- disabled 预览必须一眼可辨，但仍保留 SplitButton 结构

## 9. 已知限制与下一轮迭代计划

- 当前只保留菜单入口语义，不弹出真实 flyout
- 当前 glyph 使用双字母占位，不接真实图标资源
- 当前是固定尺寸 reference，未覆盖真实响应式宽度策略
- 若后续需要沉入框架层，再评估与 `button`、`menu_flyout` 的复用边界

## 10. 与现有控件的重叠分析与差异化边界

- 相比 `button`：这里是复合双入口，不是单动作按钮
- 相比 `menu_flyout`：这里是页内复合按钮，不是独立弹出命令面板
- 相比 `command_bar`：这里是单个主动作控件，不承担整条工具栏语义
- 相比 `segmented_control`：这里的右段是更多动作入口，不是并列选项切换

## 11. 参考设计系统与开源母本

- 参考设计系统：`Fluent 2`
- 开源母本：`WPF UI`
- 次级补充参考：`ModernWpf`

## 12. 对应组件名，以及本次保留的核心状态

- 对应组件名：`SplitButton`
- 本次保留状态：
  - `save`
  - `share`
  - `export`
  - `archive`
  - `compact`
  - `disabled`

## 13. 相比参考原型删除了哪些效果或装饰

- 不做桌面级悬停、按下、阴影过渡
- 不做真实菜单弹层和二级命令列表
- 不做系统级图标资源与快捷键标签
- 不做 Acrylic、半透明模糊和复杂主题联动

## 14. EGUI 适配时的简化点与约束

- 用固定 snapshot 驱动，优先保证 `240 x 320` 下的稳定 reference
- 主卡保留 `title + split row + helper` 三段结构
- compact 版只保留最必要的 label 与双段结构
- disabled 版通过统一 palette 降噪，而不是新增额外装饰层
