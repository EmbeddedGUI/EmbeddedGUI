# tree_view 自定义控件设计说明

## 参考来源

- 参考设计系统：`Fluent 2`
- 参考开源库：`WPF UI`
- 次级补充参考：`ModernWpf`
- 对应组件名称：`TreeView`
- 本次保留状态：`standard`、`branch expanded`、`selection`、`compact`、`read only`
- 删除效果：系统级图标资源、完整键盘焦点环、拖拽排序、虚拟滚动、复杂展开动画
- EGUI 适配说明：保留层级缩进、展开箭头、轻量引导线与选中反馈；在 `240 x 320` 画布下优先保证层级可读性、留白稳定性与底部双预览的完整呈现

## 1. 为什么需要这个控件？

`tree_view` 用于表达标准层级导航和资源浏览语义，适合文件树、设置分组、目录结构、知识树和工程导航等页面。它强调父子层级、展开状态和当前选中项，而不是普通列表或标签切换。

## 2. 为什么现有控件不够用？

- `nav_panel` 更偏常驻侧边导航，不表达递进层级和父子关系
- `breadcrumb_bar` 只表达当前位置，不展示同层兄弟节点
- `menu_flyout` 是局部命令弹出，不适合持续浏览树结构
- 现有框架控件里缺少一个贴近 Fluent `TreeView` 语义的参考型 custom widget

## 3. 目标场景与示例概览

- 主卡片展示标准 `TreeView`，覆盖不同分支展开与焦点切换
- 底部左侧 `Compact` 预览展示紧凑树列表
- 底部右侧 `Read only` 预览展示只读弱化态
- 录制动作依次切换 `Controls / Docs / Resources / Settings` 主快照，并补充 `Compact` 第二组快照

目录：`example/HelloCustomWidgets/navigation/tree_view/`

## 4. 视觉与布局规格

- 画布：`240 x 320`
- 根布局：`224 x 300`
- 页面结构：标题 -> 引导文案 -> `Standard` 标签 -> 主树视图 -> 状态文案 -> 分隔线 -> `Compact / Read only` 双预览
- 主卡片：`196 x 122`
- 底部双预览容器：`218 x 102`
- 底部单列预览：`106 x 88`
- 视觉规则：浅灰页面底板 + 低噪音白色树卡片，保留顶部弱强调条、淡色列表容器、层级引导线与选中行，不引入 showcase 式装饰

## 5. 控件清单

| 变量名 | 类型 | 尺寸 (W x H) | 初始状态 | 用途 |
| --- | --- | ---: | --- | --- |
| `root_layout` | `egui_view_linearlayout_t` | 224 x 300 | enabled | 页面根布局 |
| `title_label` | `egui_view_label_t` | 224 x 18 | `Tree View` | 页面标题 |
| `guide_label` | `egui_view_label_t` | 224 x 12 | `Tap rows to change focus` | 引导文案 |
| `tree_primary` | `egui_view_tree_view_t` | 196 x 122 | `Controls open` | 标准树视图 |
| `status_label` | `egui_view_label_t` | 224 x 12 | `Focus Branches` | 焦点状态说明 |
| `tree_compact` | `egui_view_tree_view_t` | 106 x 88 | `Library branch` | 紧凑预览 |
| `tree_locked` | `egui_view_tree_view_t` | 106 x 88 | `Selection fixed` | 只读预览 |

## 6. 状态覆盖矩阵

| 状态 / 区域 | 主卡 | Compact | Read only |
| --- | --- | --- | --- |
| 默认 | `Controls open` | `Library branch` | `Selection fixed` |
| 切换 1 | `Docs open` | 保持 | 保持 |
| 切换 2 | `Resources open` | 保持 | 保持 |
| 切换 3 | `Settings open` | 保持 | 保持 |
| 紧凑切换 | 保持 | `Review branch` | 保持 |
| 只读弱化 | 不适用 | 不适用 | 整体降噪并锁定选择 |

## 7. `egui_port_get_recording_action()` 录制动作设计

1. 应用默认主快照与紧凑快照
2. 稳定后请求默认截图
3. 切到 `Docs`
4. 请求 `Docs` 截图
5. 切到 `Resources`
6. 请求 `Resources` 截图
7. 切到 `Compact` 第二组快照
8. 请求 `Compact` 截图
9. 切到 `Settings`
10. 请求最终截图并保留收尾等待

## 8. 编译、runtime、截图验收标准

```bash
make all APP=HelloCustomWidgets APP_SUB=navigation/tree_view PORT=pc
python scripts/code_runtime_check.py --app HelloCustomWidgets --app-sub navigation/tree_view --timeout 10 --keep-screenshots
```

验收重点：

- 主树与底部双预览必须完整可见，不能裁切
- 层级缩进、展开箭头、引导线与右侧 meta pill 需要稳定对齐
- 选中行需要清晰，但不能压过树层级本身
- `Compact` 和 `Read only` 需要一眼可区分，同时保持树结构可读
- 录制截图中必须看到主快照与紧凑快照的状态变化

## 9. 已知限制与下一轮迭代计划

- 当前仍以固定 snapshot 数据驱动，不做真实数据源绑定
- 当前不实现拖拽、虚拟滚动和完整桌面级焦点环
- 当前触摸只负责切换选中行；展开分支通过快照展示
- 当前增加了 `Up / Down / Home / End` 键盘焦点移动入口，但未演示完整桌面导航链路

## 10. 与现有控件的重叠分析与差异化边界

- 相比 `nav_panel`：这里强调树层级而不是平铺导航
- 相比 `breadcrumb_bar`：这里强调可见兄弟节点与分支展开
- 相比 `menu_flyout`：这里是持续浏览结构，不是临时命令列表
- 相比 showcase 导航控件：这里回到标准 Fluent reference 语义，不做叙事式装饰

## 11. 参考设计系统与开源母本

- 参考设计系统：`Fluent 2`
- 开源母本：`WPF UI`
- 次级补充参考：`ModernWpf`

## 12. 对应组件名称，以及本次保留的核心状态

- 对应组件名称：`TreeView`
- 本次保留：`branch expanded`、`selection`、`compact`、`read only`

## 13. 相比参考原型删掉了哪些效果或装饰

- 不做真实桌面图标资源和复选框体系
- 不做完整 hover / focus ring / drag target 和整套键盘树导航
- 不做复杂展开动画与长列表虚拟滚动
- 不做系统级上下文菜单或拖拽重排

## 14. EGUI 适配时的简化点与约束

- 使用固定 `snapshot + item` 数组保证示例稳定
- 用轻量 `folder / leaf` glyph 与引导线表达树结构，不引入额外图片资源
- 底部固定 `Compact / Read only` 双列，方便和主卡直接对照
- 当前优先完成示例级 `tree_view`；是否沉入框架层后续再评估
