# HelloBasic 渲染工作流

## 目标

对 `example/HelloBasic/` 下的基础控件做一轮可重复执行的本地回归，覆盖三件事：

1. 构建和运行是否正常。
2. 录制动作是否覆盖了关键操作行为。
3. 渲染帧和交互帧是否符合预期，没有退化成空白或“动作太少”。

## 入口

完整基础控件集：

```bash
python scripts/checks/hello_basic_render_workflow.py --suite basic
```

只跑交互控件：

```bash
python scripts/checks/hello_basic_render_workflow.py --suite interactive
```

快速冒烟：

```bash
python scripts/checks/hello_basic_render_workflow.py --suite smoke
```

自定义控件列表：

```bash
python scripts/checks/hello_basic_render_workflow.py --widgets button,checkbox,slider,combobox,textinput
```

## 流程

1. 先执行 `HelloUnitTest`，确认底层没有明显回归。
2. 逐个运行 `HelloBasic/<widget>` 的运行时录制检查。
3. 对每个控件做静态审计：
   - 必须存在 `egui_port_get_recording_action()`
   - 必须包含配置要求的动作类型
   - 必须达到最少录制步骤数
   - 若配置要求运行时自检，则 `test.c` 必须包含 `[RUNTIME_CHECK_FAIL]`
4. 对录制输出做结果审计：
   - 帧不能是空白或近似空白
   - 需要交互变化的控件，必须达到最少变化帧次数

## 配置

工作流配置文件：

- `scripts/checks/hello_basic_render_workflow.json`

配置里定义：

- suite 成员
- 每个控件期望的动作类型
- 最少录制步骤数
- 是否要求 `[RUNTIME_CHECK_FAIL]`
- 最少交互变化帧次数

## 输出

- 汇总报告：`runtime_check_output/hello_basic_render_workflow_<suite>.json`
- 每个控件的录制帧：`runtime_check_output/HelloBasic_<widget>/default/`

## 维护规则

当新增或修改 `HelloBasic` 控件时，按下面顺序维护：

1. 先补齐 `test.c` 的录制动作；离散点击控件不要用跨目标 drag 代替真实点击。
2. 对关键交互补 `[RUNTIME_CHECK_FAIL]` 自检。
3. 如果控件位于 `src/widget/` 且属于非拖拽/非连续交互模型，补 `HelloUnitTest` 回归，至少覆盖 `DOWN(A) -> MOVE(B) -> UP(B)` 不提交，以及 `DOWN(A) -> MOVE(B) -> MOVE(A) -> UP(A)` 才提交。
4. 执行 `python scripts/checks/check_touch_release_semantics.py --scope core`；若是连续交互控件，在脚本 allowlist 中登记例外理由。
5. 如果控件行为是多步状态切换，录制动作不能只保留一次点击。
6. 同步更新 `scripts/checks/hello_basic_render_workflow.json`。
7. 重新执行 `python scripts/checks/hello_basic_render_workflow.py --suite basic`。
