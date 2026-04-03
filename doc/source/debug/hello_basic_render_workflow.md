# HelloBasic 渲染与交互工作流

这个工作流用于批量验证 `example/HelloBasic/` 下基础控件的本地渲染与操作行为，目标是把下面几类问题尽量提前拦住：

- 控件能编译，但录制动作缺失或覆盖不够
- 控件能出图，但画面接近空白或状态没有变化
- 交互动作执行了，但回调/状态提交没有真正生效
- 某些控件只做了静态截图，没有覆盖关键操作路径

## 入口

推荐直接使用 `make`：

```bash
make hello_basic_render_workflow ARGS="--suite basic"
```

也可以直接调用脚本：

```bash
python scripts/checks/hello_basic_render_workflow.py --suite basic
```

## 常用套件

快速冒烟：

```bash
make hello_basic_render_workflow ARGS="--suite smoke --skip-unit-tests"
```

只跑交互控件：

```bash
python scripts/checks/hello_basic_render_workflow.py --suite interactive
```

只跑指定控件：

```bash
python scripts/checks/hello_basic_render_workflow.py --widgets button,checkbox,slider,combobox,textinput
```

## 工作流内容

1. 先跑 `HelloUnitTest`
2. 清理一次构建产物
3. 逐个构建 `HelloBasic` 子应用，并使用独立 `TARGET`
4. 录制运行时帧并检查：
   - `egui_port_get_recording_action()` 是否存在
   - 关键控件是否包含期望的 `click/drag/swipe/wait`
   - 需要自检的控件是否包含 `[RUNTIME_CHECK_FAIL]`
   - 渲染帧是否为空白/近空白
   - 交互过程是否产生了足够的状态变化

## 配置

工作流配置文件：

- `scripts/checks/hello_basic_render_workflow.json`

这里定义：

- 套件成员
- 哪些控件必须具备哪些动作
- 哪些控件必须做运行时自检
- 交互变化最少需要多少帧切换

## 输出

汇总报告：

- `runtime_check_output/hello_basic_render_workflow_<suite>.json`

每个控件的录制帧：

- `runtime_check_output/HelloBasic_<widget>/default/`

## 维护规则

新增或修改 `HelloBasic` 控件时，建议按下面顺序处理：

1. 在对应 `test.c` 里补齐 `egui_port_get_recording_action()`
2. 对关键状态提交补上 `[RUNTIME_CHECK_FAIL]` 自检
3. 如果控件有明显操作行为，不要只保留静态 `wait`
4. 把控件补进 `scripts/checks/hello_basic_render_workflow.json`
5. 跑一遍 `make hello_basic_render_workflow ARGS="--suite basic"`
