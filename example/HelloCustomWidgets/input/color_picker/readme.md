# color_picker 自定义控件设计说明

## 参考来源

- 参考设计系统：`Fluent 2`
- 参考开源库：`WPF UI`
- 次级补充参考：`ModernWpf`
- 对应组件名：`ColorPicker`
- 本次保留状态：`standard`、`compact`、`read only`、`tone palette`、`hue rail`、`keyboard / touch`
- 删减效果：弹出式高级编辑器、透明度通道、文本框数值录入、桌面 hover 动画、主题同步特效
- EGUI 适配说明：保留标准颜色选择器的色板 + hue rail + 当前色预览语义，在 `240 x 320` 下优先保证色块可辨识、hex 文本可读与 compact / read-only 对照成立

## 1. 为什么需要这个控件

`color_picker` 用于表达标准颜色选择语义，比如主题色、状态色、卡片强调色、图标前景色等。它补足了当前 `HelloCustomWidgets` 里“可派生色调 + 连续色相切换”的标准颜色选择器能力。

## 2. 为什么现有控件不够用

- `swatch_picker` 只覆盖离散色样切换，不支持从同一 hue 派生明暗 / 饱和度变化
- `slider` / `xy_pad` 有连续输入能力，但不具备颜色语义和即时色彩预览
- `number_box` / `textinput` 可以录数值或文本，但不适合直接做颜色选择体验
- 当前主线里缺少一版更接近 `Fluent 2 / WPF UI ColorPicker` 的标准轻量色彩选择器

## 3. 目标场景与示例概览

- 主区域展示标准 `color_picker`，包含 label、当前色预览、hex 文本、tone palette 与 hue rail
- 左下 `Compact` 预览展示紧凑色彩选择卡
- 右下 `Read only` 预览展示只读配色卡
- 主卡支持触摸点击 tone grid / hue rail，也支持键盘 `Tab`、方向键、`Home / End`
- guide 标签切换主 preset；compact 标签切换紧凑 preset

目录：

- `example/HelloCustomWidgets/input/color_picker/`

## 4. 视觉与布局规格

- 画布：`240 x 320`
- 根布局：`224 x 286`
- 页面结构：标题 -> guide -> `Standard` -> 主 `color_picker` -> 状态文案 -> 分隔线 -> `Compact / Read only`
- 主色彩选择器：`196 x 112`
- 底部双预览：`216 x 68`
- `Compact`：`104 x 52`
- `Read only`：`104 x 52`
- 视觉规则：
  - 采用浅灰 page panel + 白色表单卡，不回到 show-case / HMI 风格
  - 顶部 preview 行显示 swatch + hex + mode pill，优先表达当前颜色状态
  - 主体 tone palette 与 hue rail 保持清晰边界，避免与 `swatch_picker` 的离散标签色样混淆

## 5. 控件清单

| 变量名 | 类型 | 尺寸 (W x H) | 初始状态 | 用途 |
| --- | --- | ---: | --- | --- |
| `root_layout` | `egui_view_linearlayout_t` | 224 x 286 | enabled | 页面根布局 |
| `title_label` | `egui_view_label_t` | 224 x 18 | `Color Picker` | 页面标题 |
| `guide_label` | `egui_view_label_t` | 224 x 12 | 可点击 | 切换主 preset |
| `picker_primary` | `egui_view_color_picker_t` | 196 x 112 | `Ocean` preset | 标准颜色选择器 |
| `status_label` | `egui_view_label_t` | 224 x 12 | `Ocean #xxxxxx` | 当前颜色摘要 |
| `picker_compact` | `egui_view_color_picker_t` | 104 x 52 | `Mint` preset | 紧凑预览 |
| `picker_locked` | `egui_view_color_picker_t` | 104 x 52 | `Locked` preset | 只读预览 |

## 6. 状态覆盖矩阵

| 状态 / 区域 | 主色彩选择器 | Compact | Read only |
| --- | --- | --- | --- |
| 默认态 | preview + tone grid + hue rail | 紧凑显示 | 只读显示 |
| 触摸 tone grid | 切换明暗 / 饱和度 | 预览页内不响应 | 不响应 |
| 触摸 hue rail | 切换色相家族 | 预览页内不响应 | 不响应 |
| `Tab` | `Tone` / `Hue` 切换 | 可用但示例页禁用 | 不适用 |
| `Left / Right / Up / Down` | 调整 tone 或 hue | 可用但示例页禁用 | 不适用 |
| `Home / End` | 跳转边界颜色 | 可用但示例页禁用 | 不适用 |
| guide / compact 标签 | 切换 preset | 切换 preset | 固定 |

## 7. `egui_port_get_recording_action()` 录制动作设计

1. 抓取默认 `Ocean` 配色
2. 发送 `Right`，提升 tone 饱和度
3. 抓取 tone 更新后的颜色
4. 发送 `Tab` + `Down`，切到 hue rail 并切换色相
5. 抓取 hue 更新后的颜色
6. 点击 guide，切换到下一个主 preset
7. 抓取新的主 preset
8. 点击 compact 标签，切换紧凑 preset
9. 抓取最终对照态

## 8. 编译、runtime、截图验收标准

```bash
make all APP=HelloCustomWidgets APP_SUB=input/color_picker PORT=pc
python scripts/code_runtime_check.py --app HelloCustomWidgets --app-sub input/color_picker --timeout 10 --keep-screenshots
make all APP=HelloUnitTest PORT=pc_test
output\main.exe
```

验收重点：

- tone grid、hue rail、preview swatch 与 hex 文本都必须完整可见
- 色块之间要能清楚区分，不允许因边框或留白不足导致混在一起
- `Tone` / `Hue` mode pill 要保持视觉居中，不能出现左右内边距失衡
- compact / read-only 仍要像标准颜色选择器预览，而不是另造装饰卡片

## 9. 已知限制与下一轮迭代计划

- 当前只覆盖 hue + tone，不含 alpha channel
- 没有加入文本输入式 `#RRGGBB` 编辑
- 后续如果沉入框架层，可补数值输入、透明度、最近使用色与 eyedropper 语义

## 10. 与现有控件的重叠分析与差异化边界

- 与 `swatch_picker` 的差异：核心在派生 tone grid + hue rail，不是离散命名色样列表
- 与 `xy_pad` 的差异：核心在颜色语义、即时色预览和 hex 摘要，不是二维参数控制
- 与 `slider` / `range_band_editor` 的差异：核心在综合色彩选择，而不是单轴数值变化

## 11. 参考设计系统与开源母本

- 设计系统：`Fluent 2`
- 开源母本：`WPF UI`
- 次级参考：`ModernWpf`

## 12. 对应组件名，以及本次保留的核心状态

- 对应组件名：`ColorPicker`
- 保留状态：`tone palette`、`hue rail`、`compact`、`read only`、`keyboard focus`

## 13. 相比参考原型删掉了哪些效果或装饰

- 去掉弹出式高级编辑器与透明度轨道
- 去掉 eyedropper、最近使用色与系统主题联动
- 去掉 hover 动画、阴影层级和桌面端复杂细节

## 14. EGUI 适配时的简化点与约束

- 优先保证 `240 x 320` 下的色块可辨识和 hex 文本可读
- 只保留单层 tone palette + hue rail，不引入浮层面板
- 用离散步进键盘闭环代替桌面端连续拖拽与文本框混合输入
