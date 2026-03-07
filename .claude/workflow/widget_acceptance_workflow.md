# Widget Acceptance Workflow

本工作流适用于后续所有新控件，默认目标目录为：

- `example/HelloCustomWidgets/<category>/<widget>/`

## Phase-1 固定规则

- 设计新颖优先：优先做交互模式、视觉语言、布局结构明显不同的控件。
- 单控件串行：同一时刻只允许 1 个控件处于进行中状态。
- 30 次迭代门槛：每个控件至少完成 30 次递归质量迭代。
- 一控件一提交：每个控件验收收口后，先更新追踪表，再单独提交一次 commit。
- 运行截图进对话框：每次完成 runtime 检查后，要把关键截图直接贴到对话框里，避免来回切目录查看。

## Step -1：同步进度追踪表（必做）

开始新控件前，先读取并更新：

- `.claude/workflow/widget_progress_tracker.md`

规则：

- 如果 `当前进行中` 不为空，优先继续该控件，不允许切换到新控件。
- 只有当前控件完成验收，或明确转入 `已搁置 / 待恢复`，才能开始下一个控件。
- 一旦选定新控件，立即把控件名、分类、日期、目标创新点写入 `当前进行中`。

## Step 0：选择下一个控件

选择原则：

- 先看 `已完成控件`，避免能力边界重复。
- 优先选择具有以下特征的候选项：
  - 新交互模型
  - 新视觉结构
  - 新布局组织方式
  - 新的复合控件模式
- 仅做小变体、换皮、轻包装的控件默认后移。
- 控件命名统一使用小写下划线风格，例如：`radial_menu`。
- 分类目录统一使用语义化英文，例如：`navigation`、`input`、`display`、`chart`。

## Step 1：创建控件目录与设计文档（必做）

目标目录：

- `example/HelloCustomWidgets/<category>/<widget>/`

本步骤至少创建：

- `example/HelloCustomWidgets/<category>/<widget>/readme.md`
- `example/HelloCustomWidgets/<category>/<widget>/iteration_log.md`

要求：

- `readme.md` 使用中文、UTF-8。
- `readme.md` 与后续 `test.c` 必须同步维护。
- `iteration_log.md` 用于记录 30 次迭代，不允许只口头说明。

`readme.md` 必须包含以下内容：

1. 为什么需要这个控件。
2. 为什么现有控件不够用。
3. 目标场景与示例概览。
4. 视觉与布局规格。
5. 控件清单（变量名、类型、尺寸、初始状态、用途）。
6. 状态覆盖矩阵。
7. `egui_port_get_recording_action()` 录制动作设计。
8. 编译、runtime、截图验收标准。
9. 已知限制与下一轮迭代计划。
10. 与现有控件的重叠分析与差异化边界。

## Step 2：实现 HelloCustomWidgets 控件

在 `example/HelloCustomWidgets/<category>/<widget>/` 中实现：

- `egui_view_<widget>.h`
- `egui_view_<widget>.c`
- `test.c`

按需要增加：

- `resource/`
- `resource/img/`
- `resource/font/`

说明：

- 当前阶段默认先做 `HelloCustomWidgets` 版本，不要求一开始就放进 `src/widget/`。
- 只有当该控件已经稳定、确定需要复用到框架层时，再单独规划升级为核心控件，并补 `scripts/ui_designer/custom_widgets/<widget>.py`。

## Step 3：编译验证（必做）

先编译：

```bash
make all APP=HelloCustomWidgets APP_SUB=<category>/<widget> PORT=pc
```

要求：

- 编译必须成功。
- 若控件依赖资源，资源路径必须与该子目录保持一致。
- 若编译失败，先修复，再进入 runtime 验证。

## Step 4：Runtime 验证（必做）

运行：

```bash
python scripts/code_runtime_check.py --app HelloCustomWidgets --app-sub <category>/<widget> --timeout 10 --keep-screenshots
```

截图输出目录约定：

- `runtime_check_output/HelloCustomWidgets_<category>/<widget>/default/frame_*.png`

检查要求：

- 不能黑屏、白屏、全空白。
- 控件主体必须完整可见，不能被裁切。
- 文本、边界、关键反馈状态必须可辨认。
- 交互型控件必须能从截图中看出状态变化。
- 若日志出现 `[RUNTIME_CHECK_FAIL]`，必须先修复，再重跑。

## Step 4.5：截图直接贴到对话框（必做）

每次 runtime 通过后，在对话回复里直接附上关键截图，使用绝对路径图片引用。

建议最少附 1 张，交互型控件建议附 3 张：

1. 初始状态帧
2. 交互中间帧
3. 最终结果帧

回复时要同时写清楚：

- 截图对应的状态
- 是否看到了预期交互反馈
- 是否还存在视觉问题

图片引用示例：

```md
![radial menu runtime](D:/workspace/gitee/EmbeddedGUI/runtime_check_output/HelloCustomWidgets_navigation/radial_menu/default/frame_0000.png)
```

## Step 5：记录 30 次递归迭代（必做）

在 `example/HelloCustomWidgets/<category>/<widget>/iteration_log.md` 中持续记录。

每次迭代至少要写：

- 迭代序号
- 本轮目标
- 代码改动摘要
- 视觉验证结论
- 交互验证结论
- 对应截图或产物路径
- 本轮结果（PASS / FAIL / HOLD）

通过门槛：

- 累计不少于 30 次迭代
- 最新一轮结果必须是 `PASS`
- 不能只有代码改动，没有截图或交互观察结论

## Step 6：验收收口

控件可收口前，必须同时满足：

- `readme.md` 完整且与当前实现一致
- `iteration_log.md` 已记录至少 30 次迭代
- `make all APP=HelloCustomWidgets APP_SUB=<category>/<widget> PORT=pc` 通过
- `code_runtime_check.py` 运行通过
- 关键截图已在对话框中展示并人工确认
- 当前控件与既有控件的差异化边界仍然成立

## Step 7：更新追踪表并提交（必做）

完成后立即更新：

- `.claude/workflow/widget_progress_tracker.md`

更新规则：

- 从 `当前进行中` 移除该控件
- 在 `已完成控件` 中追加该控件
- 写明分类、完成日期、实际迭代次数、创新关键词、差异边界、关键路径、验收结果
- 如果只是暂停，不要伪装成完成，必须移到 `已搁置 / 待恢复`

随后执行：

- 为该控件单独创建一次 commit
- 提交内容只围绕该控件，不混入下一个控件的改动

提交信息示例：

```bash
git commit -m "feat: add radial_menu custom widget"
```

## 当前阶段推荐最小交付物

每个新控件至少应包含：

- `example/HelloCustomWidgets/<category>/<widget>/readme.md`
- `example/HelloCustomWidgets/<category>/<widget>/iteration_log.md`
- `example/HelloCustomWidgets/<category>/<widget>/egui_view_<widget>.h`
- `example/HelloCustomWidgets/<category>/<widget>/egui_view_<widget>.c`
- `example/HelloCustomWidgets/<category>/<widget>/test.c`

## 备注

- 当前工作流的主目标是批量推进 `HelloCustomWidgets` 下的 1000 个控件，而不是优先把控件沉入框架核心层。
- 如果后续某个控件确定要升级为框架公共控件，再单独补一轮 `src/widget/`、`src/egui.h`、`scripts/ui_designer/custom_widgets/` 的升级计划。