# Widget Progress Tracker

## 用途说明

- 本文件是 `HelloCustomWidgets` 1000 控件计划的长期进度索引。
- 每次重新开始任务前，先读取 `.claude/workflow/widget_acceptance_workflow.md` 与本文件。
- 同一时刻只允许 1 个 `当前进行中` 控件，避免并行导致计划错位。
- 每个已完成控件都要补齐创新点、差异边界、关键文件路径、验收结果，后续选题必须先避开这些已完成能力。

## 当前阶段固定规则

- 第一优先级：设计新颖，优先做交互模式、视觉语言、组合方式明显不同的控件。
- 执行节奏：一次只做 1 个控件，当前控件未验收完成前不启动下一个。
- 质量门槛：每个控件至少完成 30 次递归迭代，并通过工作流中的全部 gate。
- 截图要求：runtime 检查后的关键截图要直接放进对话框，不再只让人去目录里查看。
- 收尾动作：控件完成后，必须先更新本文件，再提交一次该控件专属 commit，随后才能开始下一轮选题。

## 当前进行中

| 状态 | 控件名 | 分类 | 开始日期 | 当前阶段 | 目标创新点 | 备注 |
| --- | --- | --- | --- | --- | --- | --- |
| 空 | - | - | - | - | - | 当前没有进行中的控件 |

## 已完成控件

| 序号 | 控件名 | 分类 | 完成日期 | 迭代次数 | 创新关键词 | 与现有控件差异边界 | 关键路径 | 验收结果 | 备注 |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| 1 | radial_menu | navigation | 2026-03-08 | 30 | 极坐标菜单、中心触发、拖拽选择、状态分层 | 区别于 menu/button_matrix/tab_bar/compass，核心在中心触发 + 扇区命中 + 拖拽释放确认 | `example/HelloCustomWidgets/navigation/radial_menu/`; `runtime_check_output/HelloCustomWidgets_navigation/radial_menu/default/` | runtime PASS；截图已展示；30 次迭代完成 | 已提交：`aaaa7bb` |`r`n| 2 | radar_chart | chart | 2026-03-08 | 30 | 多维极坐标、闭合轮廓、双组对比、面板分层 | 区别于 chart_line/chart_bar/chart_pie/chart_scatter/compass，核心在多维极坐标 + 闭合轮廓 + 多组对比 | `example/HelloCustomWidgets/chart/radar_chart/`; `runtime_check_output/HelloCustomWidgets_chart/radar_chart/default/` | runtime PASS；截图已展示；30 次迭代完成 | 准备提交单控件 commit |

## 已搁置 / 待恢复

| 控件名 | 分类 | 日期 | 原因 | 恢复前提 | 备注 |
| --- | --- | --- | --- | --- | --- |
| 暂无 | - | - | - | - | - |

## 下一控件选择前检查清单

1. `当前进行中` 是否为空；不为空就继续当前控件，不换题。
2. 拟选控件是否与 `已完成控件` 中的能力边界重叠；如重叠，需要在 readme 中明确新的差异化目标。
3. 拟选控件是否满足“设计新颖优先”；如果只是小变体，默认后移。
4. 是否已经为新控件写明分类、创新目标、预期状态覆盖、录制动作范围。
5. 是否已经确认目录放在 `example/HelloCustomWidgets/<category>/<widget>/`。
6. 是否确认本次仍按 30 次迭代门槛执行。
7. 是否记得 runtime 后把关键截图直接贴到对话框。

## 录入模板

### 新控件开始时

把 `当前进行中` 改成一条真实记录，例如：

| 状态 | 控件名 | 分类 | 开始日期 | 当前阶段 | 目标创新点 | 备注 |
| --- | --- | --- | --- | --- | --- | --- |
| 进行中 | radial_menu | navigation | 2026-03-07 | Step 5 首轮可视化打磨 | 极坐标扇区布局、中心触发区、拖拽释放选择 | 目录：`example/HelloCustomWidgets/navigation/radial_menu/` |

### 控件完成时

1. 将 `当前进行中` 清空。
2. 在 `已完成控件` 追加一行真实记录。
3. 如中途暂停，则不要删除记录，移到 `已搁置 / 待恢复`。

建议完成记录字段：

- 控件名 / 分类
- 完成日期
- 实际通过验收的迭代次数
- 2~4 个创新关键词
- 与现有控件的差异边界说明
- `example/HelloCustomWidgets/` 与 `runtime_check_output/` 里的关键路径
- runtime 截图是否已在对话框展示
- baseline / runtime / 交互 / readme 等最终结果
