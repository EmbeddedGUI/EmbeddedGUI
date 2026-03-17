# flip_view

## 1. 为什么需要这个控件

`flip_view` 用来表达“当前只展示一张主卡，但允许连续翻页浏览”的标准轮播视图语义，适合做图片故事、步骤卡片、精选内容或焦点摘要这类一次只强调一个 hero surface 的场景。

## 2. 为什么现有控件不够用

- `coverflow_strip` 强调中心主卡 + 左右透视侧卡的空间层级，不是标准单页轮播视图。
- `pips_pager` 强调离散页码 pips 和分页位置反馈，不承担 hero content 的主展示责任。
- `tab_expose` 强调多页总览与缩略内容，不强调一次只保留一张活动卡的低噪音浏览。

## 3. 目标场景与示例概览

- 标准态：一张主卡承载当前内容，左右 overlay 按钮负责翻页。
- 状态栏：反馈当前 track、当前页序号和当前卡片标题。
- compact 预览：保留同一语义，但压缩标题/辅助文案，验证小尺寸收口。
- read-only 预览：冻结前后翻页操作，验证禁用边界与视觉弱化。

## 4. 视觉与布局规格

- 根容器尺寸：`224 x 300`
- 主控件尺寸：`196 x 118`
- 紧凑预览尺寸：`104 x 64`
- 顶部结构：标题、guide、section label
- 主卡要求：边框、阴影、eyebrow pill、counter pill、title、footer 均需完整可见
- 按钮要求：previous / next 的 chevron 需要真实居中，左右内边距平衡

## 5. 控件清单

| 变量名 | 类型 | 尺寸 | 初始状态 | 用途 |
| --- | --- | --- | --- | --- |
| `flip_view_primary` | `egui_view_flip_view_t` | `196 x 118` | `Stories track / index=1` | 标准主视图 |
| `flip_view_compact` | `egui_view_flip_view_t` | `104 x 64` | `Compact track / index=0` | 紧凑对照 |
| `flip_view_locked` | `egui_view_flip_view_t` | `104 x 64` | `Read only / index=1` | 只读对照 |
| `primary_tracks` | `flip_view_track_t[3]` | - | `Stories` | 主视图切换的数据轨 |
| `compact_tracks` | `flip_view_track_t[2]` | - | `Compact` | 紧凑态切换的数据轨 |

## 6. 状态覆盖矩阵

| Track | Snapshot | 关键状态 | 语义 |
| --- | --- | --- | --- |
| `Stories` | `Aurora deck` | 默认焦点页 | 标准 hero card 浏览 |
| `Stories` | `Quiet route` | 尾页边界 | next 按钮禁用 |
| `Planner` | `Review board` | 中间翻页态 | 单卡轮播，不显示透视侧卡 |
| `Archive` | `Paper trail` | 只读对照参考 | muted shell + disabled arrows |
| `Compact` | `Pocket deck` | 小尺寸收口 | helper 隐藏、布局压缩 |

## 7. `egui_port_get_recording_action()` 录制动作设计

- 首帧等待并截图，确认默认 `Stories` track 稳定。
- 发送 `Right` 键，验证主卡翻到下一页。
- 发送 `End` 键，验证尾页边界和 next 禁用态。
- 点击 guide 标签，切换主数据轨，验证标题/helper/状态栏同步变化。
- 点击 compact 标签，切换 compact 数据轨，验证小尺寸对照态同步更新。

## 8. 编译、runtime、截图验收标准

```bash
make all APP=HelloCustomWidgets APP_SUB=navigation/flip_view PORT=pc
python scripts/code_runtime_check.py --app HelloCustomWidgets --app-sub navigation/flip_view --timeout 10 --keep-screenshots
```

验收重点：

- 不能黑屏、白屏、全空白
- 主卡、overlay 按钮、状态栏、compact / read-only 对照都必须完整可见
- eyebrow、counter、按钮 chevron、短标题都要检查真实居中和左右留白
- 翻页后必须从截图中明确看出主卡标题、底色和状态栏发生变化

## 9. 已知限制与下一轮迭代计划

- 当前版本用纯绘制卡片模拟 Fluent `FlipView` 的 hero surface，不加载真实图片资源。
- 当前没有 hover fade in/out 动画，只保留静态 overlay 按钮语义。
- 后续如果需要更接近图片浏览器，可继续增加 image 资源与自动轮播逻辑。

## 10. 与现有控件的重叠分析与差异化边界

- 区别于 `coverflow_strip`：这里不展示左右透视侧卡，只保留单 hero surface。
- 区别于 `pips_pager`：这里主目标是展示当前卡片内容，而不是页码指示器本身。
- 区别于 `tab_expose`：这里不是多页平铺总览，而是一次只看一页的连续翻页浏览。

## 11. 参考设计系统与开源母本

- Fluent 2 / qfluentwidgets `FlipView`
- 参考源：`.venv/Lib/site-packages/qfluentwidgets/components/widgets/flip_view.py`

## 12. 对应组件名，以及本次保留的核心状态

- 组件名：`flip_view`
- 保留状态：`previous`、`surface`、`next`、`compact`、`read_only`
- 保留交互：`Left/Right/Home/End/Tab/Enter/Space/Plus/Minus/Escape`、触摸 previous/next/surface

## 13. 相比参考原型删减了哪些效果或装饰

- 删除了真实图片加载、滚轮动画和平滑滚动条。
- 删除了 hover 淡入淡出的浮层按钮动画。
- 删除了 KeepAspectRatio 等图像缩放语义，只保留 hero card 结构表达。

## 14. EGUI 适配时的简化点与约束

- 不引入图片资源，使用纯色卡面 + 文本层次模拟内容卡。
- 通过 `current_part` + `current_index` 统一键盘与触摸状态机。
- compact / read-only 直接在同一控件内切换，避免拆分多套绘制逻辑。