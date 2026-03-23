# Virtual Stage Basic

## 这个示例是做什么的

`HelloVirtual/virtual_stage_basic` 是给 `virtual_stage` 准备的最小入门 case。

它不再展示复杂 cockpit，也不再混入 `header`、说明卡片、搜索框、slider、日志区这些额外结构，只保留一组最容易理解的基础控件组合：

- 3 组真实控件：`Image + Progress + Button`
- 1 个真实 `Combobox`
- 1 个 `Pin Pump` 按钮，用来演示 pin 节点保活
- 1 个 `Reset` 按钮，用来恢复默认状态

如果你第一次接触 `virtual_stage`，先看这个 basic case 会更直接。

## 这个 case 覆盖了什么

页面里一共 `12` 个节点：

- `Pump / Valve / Fan` 各有一组 `Image + Progress + Button`
- 底部有 `Mode Combobox`
- 右侧有 `Pin Pump` 和 `Reset`

这个 basic case 的重点是：

1. 默认状态下先保留一组最基础的真实控件，用户一上来就能看到 `Image`、`ProgressBar`、`Button`、`Combobox` 这几类真实控件。
2. 其余节点仍然按需 materialize，保留 `stage` 的节点化接法和 slot 复用思路。
3. 固定节点数组直接通过 `EGUI_VIEW_VIRTUAL_STAGE_NODE_ARRAY_INTERACTIVE_BRIDGE_INIT_WITH_LIMIT(...) + EGUI_VIEW_VIRTUAL_STAGE_INIT_ARRAY_BRIDGE() + EGUI_VIEW_VIRTUAL_STAGE_ADD_ROOT()` 接到 stage，不再额外拆 `params / node_source / array_adapter / ops / setup` 这几层。
4. 业务状态只维护设备开关、进度值、当前模式和 pin 状态。
5. `Combobox` 展开时通过 `should_keep_alive + hit_test` 保持交互，收起后释放 slot。
6. `Pin Pump` 直接用 `EGUI_VIEW_VIRTUAL_STAGE_TOGGLE_PIN()` 切换 pin，方便观察 live slot 保活。
7. `Reset` 会恢复默认数据，并释放 pin 与 live slot。

## 推荐阅读顺序

1. 先看 `test.c` 里的 `basic_init_nodes()`，理解节点布局和 stable id。
2. 再看 `basic_stage_bridge`、`EGUI_VIEW_VIRTUAL_STAGE_INIT_ARRAY_BRIDGE()` 和 `EGUI_VIEW_VIRTUAL_STAGE_ADD_ROOT()`，重点看 `create / bind / draw / hit_test / should_keep_alive`。
3. 最后看点击回调和录制动作，理解 `EGUI_VIEW_VIRTUAL_STAGE_RESOLVE_ID_BY_VIEW(...)`、批量 `notify`、pin 切换和运行时验证是怎么串起来的。

## 为什么它适合作为入门

和完整的 `HelloVirtual/virtual_stage` 相比，这个 case 刻意做了减法：

- 去掉顶部 `Header`
- 只保留最基础、最常见的真实控件
- 所有节点都放在同一个固定 stage 里
- 固定节点数组直接走 array adapter bridge
- 业务逻辑只围绕开关、进度、模式、pin 四类状态

这样用户只需要先理解两件事：

1. `virtual_stage` 如何描述节点
2. 真实控件如何按需创建、绑定和释放

## 更短的初始化写法

现在这个 basic case 用的是更短的一步式数组桥接写法：

```c
EGUI_VIEW_VIRTUAL_STAGE_NODE_ARRAY_INTERACTIVE_BRIDGE_INIT_WITH_LIMIT(stage_bridge, 8, 8, 304, 224, 8, ctx.nodes, stage_node_t, desc, stage_create_view,
                                                                      stage_destroy_view, stage_bind_view, stage_draw_node, stage_hit_test,
                                                                      stage_should_keep_alive, &ctx);

EGUI_VIEW_VIRTUAL_STAGE_INIT_ARRAY_BRIDGE(&stage_view, &stage_bridge);
EGUI_VIEW_VIRTUAL_STAGE_ADD_ROOT(&stage_view);
```

如果只是 basic 这类“真实控件 + hit_test + keepalive”的常见场景，优先用 `EGUI_VIEW_VIRTUAL_STAGE_NODE_ARRAY_INTERACTIVE_BRIDGE_INIT_WITH_LIMIT(...)`，把 `ops + bridge` 一起收掉；初始化后再直接用 `EGUI_VIEW_VIRTUAL_STAGE_INIT_ARRAY_BRIDGE(...) + EGUI_VIEW_VIRTUAL_STAGE_ADD_ROOT(...)` 挂到页面上，连 stage 到 `egui_view_t` 的那层转换也不用自己写。

相比旧写法，这里把 `params + node_source + array_adapter + ops + setup` 继续收成了一条声明，用户第一次抄例程时更直接。

## 更短的批量通知写法

这个 basic case 里，很多交互一次只会联动 2 到 3 个 `stable_id`，例如：

```c
EGUI_VIEW_VIRTUAL_STAGE_NOTIFY_IDS(&basic_stage_view, BASIC_NODE_PIN_PUMP_BUTTON, BASIC_NODE_PUMP_IMAGE);
```

这样就不用为了少量 id 先额外声明一个 `const uint32_t changed_ids[]`。

如果同一组 `stable_id` 需要复用，或者它本来就是数组来源，继续用 `EGUI_VIEW_VIRTUAL_STAGE_NOTIFY_NODES(...)`。

如果更新后还需要重算节点 bounds，对应使用 `EGUI_VIEW_VIRTUAL_STAGE_NOTIFY_BOUNDS_IDS(...)` 或 `EGUI_VIEW_VIRTUAL_STAGE_NOTIFY_NODES_BOUNDS(...)`。

## 关键文件

- `src/widget/egui_view_virtual_stage.h`
- `src/widget/egui_view_virtual_stage.c`
- `example/HelloVirtual/virtual_stage_basic/app_egui_config.h`
- `example/HelloVirtual/virtual_stage_basic/test.c`

## 运行方式

构建：

```bash
make all APP=HelloVirtual APP_SUB=virtual_stage_basic PORT=pc
```

运行时检查：

```bash
python scripts/code_runtime_check.py --app HelloVirtual --app-sub virtual_stage_basic --keep-screenshots
```

录制工作流检查：

```bash
python scripts/hello_basic_render_workflow.py --app HelloVirtual --widgets virtual_stage_basic --skip-unit-tests --bits64
```

## 看图时建议关注什么

- 初始界面没有 `Header`
- 默认先看到一组基础真实控件，其余节点仍是按需 materialize
- 点击 `Pump Image` 后，对应 image 会 materialize，进度会变化
- 点击 `Mode` 后，会展开真实 `Combobox`，并能选择 `Boost`
- 点击 `Valve Progress` 后，阀门进度会继续推进
- 点击 `Fan Button` 后，风扇开关状态会切换
- 点击 `Pin Pump` 后，`Pump Image` 会保持 live
- 点击 `Reset` 后，所有状态恢复默认，live slot 释放
