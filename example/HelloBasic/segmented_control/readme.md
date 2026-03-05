# SegmentedControl 控件示例

## 1. 应用说明

该示例用于验证 `segmented_control` 的完整能力：

- C 层独立控件实现（非 `tab_bar` 包装）
- UI Designer 注册与属性映射
- HelloBasic 录制与 baseline 回归

## 2. 场景与目标

`segmented_control` 适用于 2~5 个互斥选项的快速切换，例如：

- 时间维度切换（Day / Week / Month / Year）
- 状态过滤切换（All / Warn / Error）
- 等级切换（Low / Mid / High / Auto）

本示例目标：

1. 验证默认、选中、按压、禁用、边界输入等状态。
2. 验证快速点击和禁用态点击无效行为。
3. 验证 baseline 在多帧录制下稳定可回归。

## 2.1 与现有控件重复度评估

对比对象：

- `tab_bar`：偏页面导航语义。
- `toggle_button`：单开关，不适合多分段互斥。
- `button_matrix`：多按钮网格，不提供分段选择语义。

结论：

- 功能重叠度：中等（与 `tab_bar` 有部分重叠）。
- 保留价值：高，定位为“轻量互斥选项切换控件”，用于非页面导航场景。

## 3. 本轮关键改进

1. **独立实现**：从 `tab_bar` 包装改为独立结构、绘制与命中测试。
2. **样式能力**：新增背景色、选中色、文本色、边框色、圆角、间距、内边距 setter。
3. **交互细化**：补齐 down/move/up/cancel 行为，按压态与越界处理更稳定。
4. **录制增强**：示例扩展为 3 组控件，新增快速点击与禁用态点击动作。
5. **回归闭环**：基准帧刷新为 9 帧，并通过视觉回归。

## 4. 视觉与布局规格

- 根容器：`gridlayout`，区域 `232x300`，单列居中布局。
- `segmented_primary`：`208x34`，4 段（Day/Week/Month/Year）。
- `segmented_compact`：`208x30`，3 段（All/Warn/Error），紧凑样式。
- `segmented_disabled`：`220x34`，4 段（Low/Mid/High/Auto），禁用态。

## 5. 控件清单

| 变量名 | 类型 | 尺寸 (W x H) | 初始状态 | 说明 |
|---|---|---:|---|---|
| `grid` | `egui_view_gridlayout_t` | 232 x 300 | enabled | 示例容器 |
| `segmented_primary` | `egui_view_segmented_control_t` | 208 x 34 | index=1 | 主交互控件 |
| `segmented_compact` | `egui_view_segmented_control_t` | 208 x 30 | index=0 | 紧凑样式控件 |
| `segmented_disabled` | `egui_view_segmented_control_t` | 220 x 34 | index=3, disabled | 禁用态展示 |

## 6. 状态覆盖矩阵

| 状态/能力 | 覆盖方式 | 预期结果 |
|---|---|---|
| 默认态 | 初始化渲染 | 文本和分段布局正确 |
| 选中态 | `set_current_index()` | 选中段高亮，文本颜色切换 |
| 按压态 | ACTION_DOWN / MOVE / UP | 按压覆盖层跟随命中段 |
| 禁用态 | `egui_view_set_enable(..., false)` | 不响应点击，视觉降级 |
| 边界输入 | `set_current_index(9)` | 忽略非法索引，不越界 |
| 快速切换 | 300ms 连续点击 | 选中状态稳定，无错乱 |

## 7. 录制动作设计（`egui_port_get_recording_action`）

| 序号 | 动作类型 | 目标 | 间隔 | 预期效果 |
|---:|---|---|---:|---|
| 0 | CLICK | primary 第 3 段 | 800ms | 主控件切换到 Month |
| 1 | CLICK | primary 第 4 段 | 700ms | 主控件切换到 Year |
| 2 | CLICK | primary 第 1 段 | 700ms | 主控件切回 Day |
| 3 | CLICK | compact 第 3 段 | 300ms | 紧凑控件快速切换 |
| 4 | CLICK | compact 第 2 段 | 300ms | 紧凑控件快速切回 |
| 5 | CLICK | disabled 第 2 段 | 700ms | 禁用态保持不变 |
| 6 | WAIT | - | 800ms | 稳定帧收敛 |

## 8. 验收与 baseline 规则

推荐命令：

```bash
python scripts/widget/widget_acceptance_flow.py --widget segmented_control --clock-scale 6 --snapshot-settle-ms 0 --snapshot-stable-cycles 1 --snapshot-max-wait-ms 1500
```

通过标准：

1. 运行检查通过。
2. 非空白帧检查通过。
3. baseline 对比通过（当前为 9 帧）。

## 9. 后续建议

1. 增加选中滑块过渡动画（当前为静态切换）。
2. 支持分段文本/图标混排。
3. 增加运行时动态增删分段与重排能力。
