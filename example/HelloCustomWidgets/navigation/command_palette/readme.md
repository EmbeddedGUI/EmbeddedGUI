# command_palette

## 1. 为什么需要这个控件

`command_palette` 用于表达桌面工具、创作工具或控制台中“搜索即执行”的命令入口。它需要同时承载查询框、结果高亮、快捷键提示和轻量状态反馈。

## 2. 为什么现有控件不够用

- `menu` 和 `list` 更偏静态菜单，不强调搜索驱动。
- `breadcrumb_trail` 表达的是路径层级，不是命令检索。
- `dock_launcher` 表达的是高频入口停靠，不是结果列表和键位提示。
- `textinput` 只负责输入，不覆盖命令项高亮和快捷键预览。

## 3. 目标场景与示例概览

- 主卡展示完整 command palette，包含查询框和两条结果。
- 左侧 compact 卡展示 pinned commands 预览。
- 右侧 compact 卡展示 recent commands 预览。
- runtime 通过多组 snapshot 检查查询内容、结果高亮和状态胶囊切换。

## 4. 视觉与布局规格

- 根布局：`220 x 306`
- 顶部标题：`220 x 18`
- 顶部说明：`220 x 11`
- 主卡：`192 x 118`
- 状态说明：`220 x 12`
- 分隔线：`142 x 2`
- 底部容器：`215 x 86`
- 左侧 compact：`108 x 80`
- 右侧 compact：`102 x 80`

## 5. 控件清单

| 变量名 | 类型 | 尺寸 | 初始状态 | 用途 |
| --- | --- | --- | --- | --- |
| `root_layout` | `egui_view_linearlayout_t` | `220 x 306` | 垂直布局 | 页面骨架 |
| `title_label` | `egui_view_label_t` | `220 x 18` | `Command Palette` | 页面标题 |
| `guide_label` | `egui_view_label_t` | `220 x 11` | `Tap cards to rotate command states` | 页面说明 |
| `palette_primary` | `egui_view_command_palette_t` | `192 x 118` | snapshot 0 | 主命令面板 |
| `status_label` | `egui_view_label_t` | `220 x 12` | `Primary command find` | 状态说明 |
| `section_divider` | `egui_view_line_t` | `142 x 2` | 静态 | 分隔上下区域 |
| `bottom_row` | `egui_view_linearlayout_t` | `215 x 86` | 横向布局 | 承载两个 compact 卡 |
| `palette_pinned` | `egui_view_command_palette_t` | `108 x 80` | compact snapshot 0 | 收藏命令预览 |
| `palette_recent` | `egui_view_command_palette_t` | `102 x 80` | locked snapshot 0 | 最近命令预览 |

## 6. 状态覆盖矩阵

| 区域 | Snapshot | 状态文案 | 查询 | 主结果 | 说明 |
| --- | --- | --- | --- | --- | --- |
| 主卡 | 0 | `FIND` | `jump to audio route` | `Open mixer scene` | 搜索态 |
| 主卡 | 1 | `LIVE` | `resume capture lane` | `Resume live stack` | 热路径态 |
| 主卡 | 2 | `SAFE` | `return calm layout` | `Seal transport grid` | 安全收口态 |
| 左卡 | 0 | `SAVE` | `open pin` | `Pinned cues` | 已收藏 |
| 左卡 | 1 | `SCAN` | `scan pins` | `Scan recall` | 复查态 |
| 左卡 | 2 | `SAVE` | `seal pin` | `Saved cues` | 已保存 |
| 右卡 | 0 | `LAST` | `open last` | `Return mix` | 最近命令 |
| 右卡 | 1 | `WARN` | `hot lane` | `Guard route` | 风险提醒 |
| 右卡 | 2 | `LAST` | `calm lane` | `Recall cue` | 最近恢复 |

## 7. `egui_port_get_recording_action()` 录制动作设计

1. 首帧等待 `400ms`。
2. 点击主卡，切换主卡 snapshot。
3. 等待 `300ms`。
4. 点击左侧 compact 卡，切换收藏预览 snapshot。
5. 等待 `300ms`。
6. 点击右侧 compact 卡，切换最近预览 snapshot。
7. 等待 `800ms` 输出关键截图。

## 8. 编译、runtime、截图验收标准

### 编译

```bash
make all APP=HelloCustomWidgets APP_SUB=navigation/command_palette PORT=pc
```

### Runtime

```bash
python scripts/code_runtime_check.py --app HelloCustomWidgets --app-sub navigation/command_palette --timeout 10 --keep-screenshots
```

### 验收标准

- 运行不能黑屏、卡死或崩溃。
- 查询框、结果行、状态胶囊和快捷键提示必须完整可见。
- 查询框文本、短词胶囊、短词快捷键必须检查视觉居中和左右留白。
- 每轮截图都要复制到 `iteration_log/images/iter_xx/` 并写入结论。

## 9. 已知限制与下一轮迭代计划

- 首版先跑通静态渲染和 snapshot 切换。
- 后续优先打磨查询框与结果行的边距平衡。
- 若后续需要更复杂交互，再考虑接入真实键盘事件。

## 10. 与现有控件的重叠分析与差异化边界

- 区别于 `menu/list`：这里强调搜索即执行与命令结果预览。
- 区别于 `breadcrumb_trail`：这里不是路径层级导航。
- 区别于 `dock_launcher`：这里核心是查询框和结果列表，不是停靠入口。
- 区别于 `textinput`：这里额外承载结果高亮、快捷键胶囊和命令状态反馈。
