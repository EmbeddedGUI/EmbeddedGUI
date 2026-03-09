# layer_stack

## 1. 为什么需要这个控件

`layer_stack` 用于表达图层面板、遮罩栈、合成栈、导出栈这类“前后叠层”的界面语义。它不是简单列表，而是把前后深度、当前层焦点和结果状态一起可视化。

## 2. 为什么现有控件不够用

- `tab_expose` 是并列页面总览，不表达前后深度。
- `kanban_board` 是并列泳道，不适合叠放层关系。
- `node_topology` 强调连线依赖，不是叠层堆栈。
- `server_rack` 是竖向槽位，不是前后偏移堆叠。

## 3. 目标场景与示例概览

- 主卡：四层叠栈、当前层高亮、右侧深度刻度、当前摘要和底部状态。
- 左 compact：mask 层组。
- 右 compact：render/output 层组，带更克制的 locked 语义。
- 点击三张卡可分别轮询 3 个 snapshot。

## 4. 视觉与布局规格

- 根布局：`220 x 306`
- 标题：`220 x 18`
- guide：`220 x 11`
- 主卡：`194 x 126`
- 状态说明：`220 x 12`
- 分隔线：`154 x 2`
- 底部容器：`216 x 90`
- mask compact：`105 x 84`
- render compact：`105 x 84`

主卡以冷蓝为主，mask 偏清绿，render 偏暖琥珀。顶部短词胶囊、compact 顶部短词、底部短文案和右侧刻度都必须检查真实居中和边距。

## 5. 控件清单

| 变量名 | 类型 | 尺寸 | 初始状态 | 用途 |
| --- | --- | --- | --- | --- |
| `root_layout` | `egui_view_linearlayout_t` | `220 x 306` | 垂直布局 | 页面骨架 |
| `title_label` | `egui_view_label_t` | `220 x 18` | `Layer Stack` | 页面标题 |
| `guide_label` | `egui_view_label_t` | `220 x 11` | `Tap stacks to cycle layers` | 操作提示 |
| `stack_primary` | `egui_view_layer_stack_t` | `194 x 126` | snapshot 0 | 主图层总览 |
| `status_label` | `egui_view_label_t` | `220 x 12` | `Primary layer ready` | 外部状态反馈 |
| `section_divider` | `egui_view_line_t` | `154 x 2` | 静态 | 分隔上下区域 |
| `bottom_row` | `egui_view_linearlayout_t` | `216 x 90` | 横向布局 | 承载两个 compact |
| `stack_masked` | `egui_view_layer_stack_t` | `105 x 84` | snapshot 0 | mask 层组 |
| `stack_output` | `egui_view_layer_stack_t` | `105 x 84` | locked snapshot 0 | render 层组 |

## 6. 状态覆盖矩阵

| 区域 | Snapshot | 状态 | 摘要 | 底部文案 |
| --- | --- | --- | --- | --- |
| 主卡 | 0 | `EDIT` | `Four layers ready` | `active layer live` |
| 主卡 | 1 | `FOCUS` | `Mask layer solo` | `focus layer lock` |
| 主卡 | 2 | `SAFE` | `Merged stack calm` | `export layer sealed` |
| mask | 0 | `HOLD` | `Mask deck` | `mask group` |
| mask | 1 | `SCAN` | `Feather pass` | `edge scan` |
| mask | 2 | `SAFE` | `Mask calm` | `seal group` |
| render | 0 | `LIVE` | `Render deck` | `render group` |
| render | 1 | `WARN` | `Blend queue` | `guard queue` |
| render | 2 | `LOCK` | `Publish calm` | `archive stack` |

## 7. `egui_port_get_recording_action()` 录制动作设计

录制动作共 13 步：

1. 等待 `400ms`
2. 点击主卡一次
3. 等待 `300ms`
4. 点击 mask 卡一次
5. 等待 `300ms`
6. 点击 render 卡一次
7. 等待 `300ms`
8. 再点击主卡一次
9. 等待 `300ms`
10. 再点击 mask 卡一次
11. 等待 `300ms`
12. 再点击 render 卡一次
13. 等待 `800ms`

这样 runtime 输出 15 帧，覆盖三张卡的三种状态。

## 8. 编译、runtime、截图验收标准

### 编译

```bash
make all APP=HelloCustomWidgets APP_SUB=display/layer_stack PORT=pc
```

### Runtime

```bash
python scripts/code_runtime_check.py --app HelloCustomWidgets --app-sub display/layer_stack --timeout 10 --keep-screenshots
```

### 验收标准

- 不能黑屏、卡死、崩溃。
- 主卡和两个 compact 卡都必须完整可见，不能裁切。
- 顶部短词和状态胶囊必须检查真实居中。
- 右侧深度刻度与外框必须保留安全距离，不能贴边。
- compact 底部短文案必须完整可读，不能贴底或内边距失衡。
- `iteration_log/iteration_log.md` 必须记录 30 轮改动、截图和结论。

## 9. 已知限制与下一轮迭代计划

- 当前版本聚焦点击轮询和静态层组表达，未实现真实图层拖拽重排。
- 当前 preview 是信息化图层缩略，不是真实内容截图。
- 如果后续要升级为框架级控件，可继续补：
  - 层可见/锁定切换图标
  - 拖拽调整层顺序
  - 与真实画布或图层管理器联动

## 10. 与现有控件的重叠分析与差异化边界

- 相对 `tab_expose`：这里是前后叠层，不是并列页面工作区。
- 相对 `node_topology`：这里没有连线关系，核心是深度堆叠。
- 相对 `kanban_board`：这里没有列结构，核心是重叠层偏移。
- 相对 `server_rack`：这里不是槽位单元，而是前后图层和当前层焦点。
