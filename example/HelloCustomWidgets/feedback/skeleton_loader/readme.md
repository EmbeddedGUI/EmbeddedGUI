# SkeletonLoader 控件设计说明

## 1. 为什么需要这个控件？

`skeleton_loader` 用于在真实内容尚未到达时，先用占位骨架表达页面结构、信息层级和重点区域，适合列表卡片、详情摘要、媒体封面等加载前预览场景。

## 2. 为什么现有控件不够用？

- `spinner` 只能表达“正在加载”，不能表达内容将以什么布局出现。
- `progress_bar` 更适合线性进度反馈，不适合页面结构占位。
- `card`、`list`、`label` 需要真实内容数据，不能在空数据阶段提供一致的加载预览。
- `notification_stack`、`status_timeline` 等已完成控件强调业务语义，不是通用的内容占位骨架。

差异化边界：`skeleton_loader` 的核心是“骨架块布局 + 当前高亮块 + 紧凑预览 / 只读预览对比”，不是单一 loading 动画或进度条换皮。

## 3. 目标场景与示例概览

- primary：主骨架卡，展示两种典型内容装配顺序 `Layout A / Layout B`。
- compact：紧凑预览，验证小尺寸下仍可辨认头像位、标题位和卡片位。
- locked：只读预览，验证禁用态下仍能看出结构，但不会给交互反馈。

目录：
- `example/HelloCustomWidgets/feedback/skeleton_loader/`

## 4. 视觉与布局规格

- 屏幕基准：240 x 320。
- 页面结构：标题 -> 引导文案 -> 主骨架卡 -> 状态文案 -> 分割线 -> 底部双卡。
- 主骨架卡尺寸：176 x 132，顶部显示布局名称，底部显示 `Loading layout`。
- 底部双卡尺寸：106 x 92，分别用于紧凑预览和锁定预览。
- 视觉语言：深色容器、低对比度占位块、焦点块高亮描边、禁用态交叉线遮罩。

## 5. 控件清单

| 变量名 | 类型 | 尺寸 (W x H) | 初始状态 | 用途 |
| --- | --- | ---: | --- | --- |
| root_layout | egui_view_linearlayout_t | 220 x 304 | enabled | 页面根容器 |
| loader_primary | egui_view_skeleton_loader_t | 176 x 132 | Layout A | 主骨架卡 |
| loader_compact | egui_view_skeleton_loader_t | 106 x 92 | Compact A | 紧凑预览 |
| loader_locked | egui_view_skeleton_loader_t | 106 x 92 | disabled | 只读预览 |
| status_label | egui_view_label_t | 220 x 14 | Layout A phase | 中部状态反馈 |

## 6. 状态覆盖矩阵

| 状态 / 能力 | 覆盖方式 | 预期结果 |
| --- | --- | --- |
| 默认态 | 初始渲染 | 主骨架卡显示 Layout A，内容区块完整可见 |
| 主骨架切换态 | 点击 primary | 切到 Layout B，焦点块和状态文案同步变化 |
| 紧凑预览切换态 | 点击 compact | 切到 Compact B，暖色描边高亮生效 |
| 只读态 | 点击 locked | 无状态变化，保持灰化和交叉线遮罩 |

## 7. egui_port_get_recording_action() 录制动作设计

1. 初始等待 400ms
2. 点击主骨架卡，切到 Layout B
3. 等待 300ms
4. 点击紧凑预览，切到 Compact B
5. 等待 300ms
6. 点击 locked 预览，验证禁用态无响应
7. 最后等待 800ms 输出稳定截图

## 8. 编译、runtime、截图验收标准

```bash
make all APP=HelloCustomWidgets APP_SUB=feedback/skeleton_loader PORT=pc
python scripts/code_runtime_check.py --app HelloCustomWidgets --app-sub feedback/skeleton_loader --timeout 10 --keep-screenshots
```

验收要求：
- 编译通过，无未定义符号或资源路径错误。
- runtime 输出 `ALL PASSED`。
- 主骨架、紧凑预览、只读预览完整可见。
- 高亮块、底部状态文案、禁用态交叉线都能从截图中明确辨认。

## 9. 已知限制与下一轮计划

- 当前使用静态骨架块，不包含波浪扫光动画。
- 骨架布局由固定快照驱动，还未支持运行时动态拼装。
- 文案区仅展示布局名与加载提示，未加入百分比或阶段说明。
- 后续可扩展脉冲高亮、自动轮播布局和更多模板类型。

## 10. 与现有控件的重叠分析与差异化边界

- 与 `spinner`：这里强调内容结构占位，不只是等待状态。
- 与 `progress_bar`：这里强调布局预览，不是线性进度。
- 与 `card`：这里展示的是未加载内容的骨架结构，不是真实卡片信息。
- 与 `waveform_strip`、`tag_cloud` 等展示型控件：这些控件表达真实数据，`skeleton_loader` 表达数据到达前的占位形态。
