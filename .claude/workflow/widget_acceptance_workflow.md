# Widget Acceptance Workflow

本工作流适用于后续所有新控件，默认目标目录为：

- `example/HelloCustomWidgets/<category>/<widget>/`

## Phase-1 固定规则

- 设计新颖优先：优先做交互模式、视觉语言、布局结构明显不同的控件。
- 单控件串行：同一时刻只允许 1 个控件处于进行中状态。
- 30 次迭代门槛：每个控件至少完成 30 次递归质量迭代；默认做满 30 轮再向用户统一同步，中途不逐轮汇报，除非用户明确要求。
- 单控件自主收口：一旦开始某个控件，默认连续执行 Step 1 到 Step 7，直到该控件完成 30 次迭代、验收收口、更新追踪表并完成单独 commit 后，才允许停下来向用户同步；中途除非出现明确阻塞、需要用户决策，或用户主动打断，否则不要要求用户重复输入“继续”。
- 一控件一提交：每个控件验收收口后，先更新追踪表，再单独提交一次 commit。
- 运行截图归档：每次完成 runtime 检查后，要把关键截图复制到 `iteration_log/images/`，并在 `iteration_log/iteration_log.md` 中用相对路径记录，作为后续 review 依据。

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
- 优先选择能在选定开源参考体系（`Fluent 2 / WPF UI`）中找到稳定原型的候选项。
- 优先选择具有以下特征的候选项：
  - 新交互模型
  - 新视觉结构
  - 新布局组织方式
  - 新的复合控件模式
- 仅做小变体、换皮、轻包装的控件默认后移。
- 强行业场景、强叙事和强装饰语法控件默认后移，除非用户明确要求。
- 控件命名统一使用小写下划线风格，例如：`radial_menu`。
- 分类目录统一使用语义化英文，例如：`navigation`、`input`、`display`、`chart`。

## Step 1：创建控件目录与设计文档（必做）

目标目录：

- `example/HelloCustomWidgets/<category>/<widget>/`

本步骤至少创建：

- `example/HelloCustomWidgets/<category>/<widget>/readme.md`
- `example/HelloCustomWidgets/<category>/<widget>/iteration_log/`
- `example/HelloCustomWidgets/<category>/<widget>/iteration_log/iteration_log.md`
- `example/HelloCustomWidgets/<category>/<widget>/iteration_log/images/`

要求：

- `readme.md` 使用中文、UTF-8。
- `readme.md` 与后续 `test.c` 必须同步维护。
- `iteration_log/iteration_log.md` 用于记录 30 次迭代，不允许只口头说明。
- `iteration_log/images/` 用于保存从 `runtime_check_output/` 复制出的本轮关键截图，供阶段性 review。

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
11. 参考设计系统与开源母本（例如 `Fluent 2`、`WPF UI`）。
12. 对应组件名，以及本次保留的核心状态。
13. 相比参考原型删掉了哪些效果或装饰。
14. EGUI 适配时的简化点与约束。

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

## Step 3.5：Touch Release 审计（必做）

先执行：

```bash
python scripts/check_touch_release_semantics.py --scope custom --category <category>
```

要求：

- 非拖拽/非连续交互控件，`ACTION_MOVE` 不得改写 `pressed_*` 之类的“按下目标”状态。
- 非拖拽/非连续交互控件，`ACTION_UP` 只能在 `ACTION_DOWN` 命中的同一目标上提交，不能在移动到其他目标后于新目标触发点击或选择。
- 必须补充模拟交互测试，至少覆盖 `DOWN(A) -> MOVE(B) -> UP(B)` 不提交，以及 `DOWN(A) -> MOVE(B) -> MOVE(A) -> UP(A)` 才提交。
- 若控件本身就是拖拽/连续交互模型，必须在 `scripts/check_touch_release_semantics.py` 的 allowlist 中登记，并在提交说明中写清楚为何属于例外。
- 若本轮同时修改了 `src/widget/`，或该控件准备下沉到核心层，必须额外执行 `python scripts/check_touch_release_semantics.py --scope core`，并同步补 `HelloUnitTest` 的 same-target release 回归测试。
- 审计失败时，必须先修复事件分发语义，再继续 runtime 验证。

## Step 4：Runtime 验证（必做）

运行：

```bash
python scripts/code_runtime_check.py --app HelloCustomWidgets --app-sub <category>/<widget> --timeout 10 --keep-screenshots
```

截图输出目录约定：

- `runtime_check_output/HelloCustomWidgets_<category>_<widget>/default/frame_*.png`

分类回归命令（用于批量复查同类控件）：

```bash
python scripts/code_runtime_check.py --app HelloCustomWidgets --category <category> --bits64
```

检查要求：

- 不能黑屏、白屏、全空白。
- 控件主体必须完整可见，不能被裁切。
- 文本、边界、关键反馈状态必须可辨认。
- 文本、图标、中心按钮等关键元素必须检查视觉居中是否准确，左右/上下留白是否平衡；不能只因为“没有裁切”就判定通过。
- 文本与圆形、按钮、胶囊等边框之间必须保留合理空隙，不能出现文字贴边或某一侧内边距明显小于另一侧。
- 顶部圆圈内文案、状态胶囊短词、按钮短词都要单独检查是否真实居中，像 `Open`、`Pause` 这类文本要避免左右内边距失衡或视觉偏心。
- 交互型控件必须能从截图中看出状态变化。
- 若日志出现 `[RUNTIME_CHECK_FAIL]`，必须先修复，再重跑。

## Step 4.5：归档 iteration_log（必做）

每次 runtime 通过后，必须立刻把关键截图整理进当前控件目录下的 `iteration_log/`，并同步更新 `iteration_log/iteration_log.md`，不能只在对话里口头说明，也不能只保留 `runtime_check_output/` 临时结果。

归档路径：

- `example/HelloCustomWidgets/<category>/<widget>/iteration_log/iteration_log.md`
- `example/HelloCustomWidgets/<category>/<widget>/iteration_log/images/iter_01/`
- `example/HelloCustomWidgets/<category>/<widget>/iteration_log/images/iter_02/`

要求：

- 每轮至少归档 1 张关键截图；交互型控件建议归档 2 到 3 张，覆盖初始态、交互态、结果态。
- 必须先从 `runtime_check_output/...` 复制截图到 `iteration_log/images/iter_xx/`，再在日志中使用相对路径引用。
- `iteration_log/iteration_log.md` 必须记录本轮目标、代码改动摘要、编译结果、runtime 结果、视觉结论、交互结论和最终判定。
- `iteration_log/iteration_log.md` 中必须额外记录：关键文字、图标、中心按钮是否视觉居中，左右/上下边距是否平衡。
- `iteration_log/iteration_log.md` 中必须额外记录：文字与按钮/圆形/胶囊边框之间是否留有合理空隙，是否存在贴边或内边距失衡。
- Windows 上如通过 PowerShell 管道把脚本内容喂给 `python -` 来写入中文日志，必须先设置 `$OutputEncoding = [Console]::OutputEncoding = [Text.Encoding]::UTF8`，否则中文可能被写成问号占位符。
- 顶部圆形按钮、短文本胶囊、短词按钮必须单独检查视觉居中与左右留白，例如 `Open`、`Pause` 这类短文本不能出现视觉偏心或贴边。

```md
![iter 01 frame 0000](images/iter_01/frame_0000.png)
```

- `iteration_log/` 只作为本地审阅产物，不纳入 git commit。

## Step 5：记录 30 次递归迭代（必做）

在 `example/HelloCustomWidgets/<category>/<widget>/iteration_log/iteration_log.md` 中持续记录。

每次迭代都必须完整执行以下闭环，不允许只编译通过就进入下一轮：

1. 改代码
2. 执行 `make all APP=HelloCustomWidgets APP_SUB=<category>/<widget> PORT=pc`
3. 执行 `python scripts/check_touch_release_semantics.py --scope custom --category <category>`
4. 执行 `python scripts/code_runtime_check.py --app HelloCustomWidgets --app-sub <category>/<widget> --timeout 10 --keep-screenshots`
5. 检查截图中的视觉问题
6. 复制关键截图到 `iteration_log/images/iter_xx/`
7. 更新 `iteration_log/iteration_log.md`

如果同一轮修改影响了同分类多个控件，单控件验证通过后，再追加执行一次：

8. 执行 `python scripts/code_runtime_check.py --app HelloCustomWidgets --category <category> --bits64`

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
- `iteration_log/iteration_log.md` 已记录至少 30 次迭代
- `make all APP=HelloCustomWidgets APP_SUB=<category>/<widget> PORT=pc` 通过
- `python scripts/check_touch_release_semantics.py --scope custom --category <category>` 通过；若触及核心控件，还需额外通过 `python scripts/check_touch_release_semantics.py --scope core`；若属于拖拽例外，已同步更新 allowlist 并写明理由
- `code_runtime_check.py` 运行通过
- 已对当前控件相关代码执行格式化，并确认格式化后仍可通过构建与 runtime 验证
- 关键截图已整理到 `iteration_log/` 且能通过 `iteration_log/iteration_log.md` 直接 review
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

- 先执行 `python scripts/code_format.py`，并确保格式化后没有引入新的构建或 runtime 问题
- 为该控件单独创建一次 commit
- 提交内容只围绕该控件，不混入下一个控件的改动
- `iteration_log/` 目录保持本地，不纳入本次 commit
- 当前控件只有在 commit 完成后才能暂停并向用户汇报；如果还未到 commit 阶段，不要因为阶段性通过而中途停下等待“继续”

提交信息示例：

```bash
git commit -m "feat: add radial_menu custom widget"
```

## 当前阶段推荐最小交付物

每个新控件至少应包含（提交到 git 的交付物）：

- `example/HelloCustomWidgets/<category>/<widget>/readme.md`
- `example/HelloCustomWidgets/<category>/<widget>/egui_view_<widget>.h`
- `example/HelloCustomWidgets/<category>/<widget>/egui_view_<widget>.c`
- `example/HelloCustomWidgets/<category>/<widget>/test.c`

每个新控件还应维护（本地审阅产物，不提交 git）：

- `example/HelloCustomWidgets/<category>/<widget>/iteration_log/iteration_log.md`
- `example/HelloCustomWidgets/<category>/<widget>/iteration_log/images/`

## 备注

- 当前工作流的主目标是批量推进 `HelloCustomWidgets` 下的 1000 个控件，而不是优先把控件沉入框架核心层。
- 如果后续某个控件确定要升级为框架公共控件，再单独补一轮 `src/widget/`、`src/egui.h`、`scripts/ui_designer/custom_widgets/` 的升级计划。

