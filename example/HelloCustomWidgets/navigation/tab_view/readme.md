# tab_view 自定义控件设计说明

## 参考来源
- 参考设计系统：`Fluent 2`
- 参考开源库：`WPF UI`
- 次级补充参考：`ModernWpf`
- 对应组件名：`TabView`
- 本次保留状态：`standard`、`compact`、`read only`
- 删除效果：拖拽重排、窗口级系统菜单、Acrylic、复杂 hover 动画、多行 header 命令区
- EGUI 适配说明：保留标准页签头、内容面板、关闭入口和恢复入口，用轻量边框与 pill 取代桌面级大面积视觉层

## 1. 为什么需要这个控件
`tab_view` 用来表达页签头和内容面板是一体的浏览容器，适合文档页、后台工作区、设置页中的多面板切换场景。

## 2. 为什么现有控件不够用
- `tab_strip` 只解决页签头，不承载内容面板
- `tab_expose` 偏多页总览，不是页签内联阅读容器
- `flip_view` 是单页轮播，不是多页签工作区
- `viewpage` / `tab_bar` 更偏基础切页，不具备标准 `TabView` 的 header + content shell 语义

## 3. 目标场景与示例概览
- 主区域展示标准 `tab_view`，覆盖 `Docs workspace` 与 `Ops workspace` 两套 snapshot
- 主区域展示标准 `tab_view`，header 区保留 title/meta，body 区保留 badge/eyebrow/title/footer 的轻量信息层
- 左下 `Compact` 预览展示缩窄尺寸下的低噪音版本
- 右下 `Read only` 预览展示只读冻结态
- 主区域需要能看出切页、关闭标签和恢复标签后的内容联动

目录：
- `example/HelloCustomWidgets/navigation/tab_view/`

## 4. 视觉与布局规格
- 画布：`240 x 320`
- 根布局：`224 x 300`
- 主 `tab_view`：`198 x 128`
- 底部双预览：`216 x 90`
- `Compact` / `Read only` 预览：`104 x 72`
- 视觉规则：
  - 使用浅灰 page panel + 白底轻边框卡片
  - active tab 只保留轻量 fill、underline 和可见的关闭入口
  - `Tab` 切换时 `close / add` 要能从部件级描边看出 focus 位置
  - body panel 用低对比内容卡，不回退到 showcase 风格的大色块
  - `Compact` 保留语义，不保留冗长 body 文本

## 5. 控件清单
| 变量名 | 类型 | 尺寸 (W x H) | 初始状态 | 用途 |
| --- | --- | ---: | --- | --- |
| `root_layout` | `egui_view_linearlayout_t` | `224 x 300` | enabled | 页面根布局 |
| `title_label` | `egui_view_label_t` | `224 x 18` | `Tab View` | 页面标题 |
| `guide_label` | `egui_view_label_t` | `224 x 12` | clickable | 切换主 snapshot |
| `tab_view_primary` | `egui_view_tab_view_t` | `196 x 126` | `Docs workspace` | 主控件 |
| `status_label` | `egui_view_label_t` | `224 x 12` | 当前页签状态 | 底部状态文案 |
| `tab_view_compact` | `egui_view_tab_view_t` | `104 x 72` | compact | 紧凑预览 |
| `tab_view_locked` | `egui_view_tab_view_t` | `104 x 72` | read only | 只读预览 |

## 6. 状态覆盖矩阵
| 状态 / 区域 | 主 `tab_view` | Compact | Read only |
| --- | --- | --- | --- |
| 默认态 | `Docs / Home` | `Compact docs` | `Read only / Read` |
| 切换标签 | body 面板和 footer 同步变化 | 保持当前 compact snapshot | 不响应 |
| 关闭标签 | 可关闭标签从 header 消失，visible count 变化 | 不演示 | 不响应 |
| 恢复标签 | `+` 恢复所有已关闭标签 | 不演示 | 不响应 |
| 切换 snapshot | 主工作区整体切换 | 通过 label 单独切换 | 固定冻结 |

## 7. `egui_port_get_recording_action()` 录制动作设计
1. 首帧固定 `Docs / Home`
2. 切到 `Publish`
3. 关闭当前标签，展示 visible count 收缩
4. 用 `+` 恢复标签
5. 切换到 `Ops workspace`
6. 切换 `Compact` 预览 snapshot

## 8. 编译、runtime、截图验收标准
```bash
make all APP=HelloCustomWidgets APP_SUB=navigation/tab_view PORT=pc
python scripts/code_runtime_check.py --app HelloCustomWidgets --app-sub navigation/tab_view --timeout 10 --keep-screenshots
```

验收重点：
- 主 header、body、footer 三层必须完整可见
- active tab 与 close / add 入口的留白不能挤压正文
- 关闭标签前后 body 文案不能错位
- `eyebrow`、status text 与底部双预览要保持轻量，不得喧宾夺主
- `Compact` 和 `Read only` 要能明显区别于主态

## 9. 已知限制与下一轮计划
- 当前版本不做拖拽重排与多行 header command 区
- 关闭标签采用本地 `closed_mask`，只在当前 snapshot 内生效
- 文本宽度仍是轻量估算，不做真实字体测量
- 已补齐 `eyebrow`、footer 文案退化、部件级 focus 可视化与录制态 status 同步
- 后续若升级到框架公共控件，再单独规划拖拽重排和更复杂的 header command 区

## 10. 与现有控件的重叠分析与差异化边界
- 相比 `tab_strip`：这里把 content body 作为控件内部语义的一部分
- 相比 `tab_expose`：这里不是多页总览，而是单工作区内联切换
- 相比 `flip_view`：这里是多标签并列语义，不是前后翻页浏览
- 相比 `viewpage`：这里强调标准桌面式 `TabView` 外观与关闭/恢复入口

## 11. 参考设计系统与开源母本
- 设计系统：`Fluent 2`
- 开源母本：`WPF UI`
- 次级补充：`ModernWpf`

## 12. 对应组件名，以及本次保留的核心状态
- 对应组件名：`TabView`
- 本次保留：
  - `standard`
  - `compact`
  - `read only`
  - `close current tab`
  - `restore closed tabs`

## 13. 相比参考原型删掉了哪些效果或装饰
- 不做拖拽重排、tear-out、窗口级集成
- 不做复杂 hover、焦点环和多级阴影
- 不做系统菜单、图标页签、Acrylic 背景
- 不做多段 header 命令工具条

## 14. EGUI 适配时的简化点与约束
- 以轻量 `closed_mask` 完成关闭语义，避免复杂容器重排
- 所有状态在 `240 x 320` 中优先保证可审阅性
- 维持浅色低噪音方向，避免退回 showcase 风格
- 当前先作为 `HelloCustomWidgets` 示例控件推进，不下沉到 `src/widget/`
