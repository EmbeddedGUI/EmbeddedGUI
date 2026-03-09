# tab_expose

## 1. 为什么需要这个控件

`tab_expose` 用于表达多标签页工作区的总览切换场景。它不是单纯的 tab 切页条，而是同时提供：

- 当前 tab 的活动态提示
- 多个 tab 内容的缩略预览
- 保存页组与最近页组的紧凑预览

适合文件编辑器、浏览器、设计工具、工作台类应用中的“标签页总览”入口。

## 2. 为什么现有控件不够用

- `tab_bar` 只负责切页，不负责展示多个页面缩略图。
- `breadcrumb_trail` 表达的是路径层级，不是同层标签工作区。
- `dock_launcher` 侧重入口停靠和应用切换，不是页面簇预览。
- `command_palette` 是搜索执行入口，不负责页面总览。

## 3. 目标场景与示例概览

- 主卡：展示三段 tab strip、三张 preview、当前状态胶囊与底部说明。
- 左侧 compact：展示保存页组的预览与切换。
- 右侧 compact：展示最近页组的预览与切换，并带只读感更强的 locked 语义。
- 点击主卡、saved 卡、recent 卡，分别轮询各自的 3 个 snapshot。

## 4. 视觉与布局规格

- 根布局：`220 x 306`
- 标题：`220 x 18`
- guide：`220 x 11`
- 主卡：`194 x 120`
- 状态说明：`220 x 12`
- 分隔线：`154 x 2`
- 底部容器：`216 x 87`
- saved compact：`105 x 81`
- recent compact：`105 x 81`

主卡配色以冷蓝为主，saved 偏清绿，recent 偏暖琥珀。顶部短词胶囊与底部短文案都要求保留明确左右留白，不能出现贴边或视觉偏心。

## 5. 控件清单

| 变量名 | 类型 | 尺寸 | 初始状态 | 用途 |
| --- | --- | --- | --- | --- |
| `root_layout` | `egui_view_linearlayout_t` | `220 x 306` | 垂直布局 | 页面骨架 |
| `title_label` | `egui_view_label_t` | `220 x 18` | `Tab Expose` | 页面标题 |
| `guide_label` | `egui_view_label_t` | `220 x 11` | `Tap cards to cycle tab sets` | 操作提示 |
| `expose_primary` | `egui_view_tab_expose_t` | `194 x 120` | snapshot 0 | 主标签总览 |
| `status_label` | `egui_view_label_t` | `220 x 12` | `Primary grid ready` | 外部状态反馈 |
| `section_divider` | `egui_view_line_t` | `154 x 2` | 静态 | 分隔上下区域 |
| `bottom_row` | `egui_view_linearlayout_t` | `216 x 87` | 横向布局 | 承载两个 compact 卡 |
| `expose_saved` | `egui_view_tab_expose_t` | `105 x 81` | snapshot 0 | 保存页组预览 |
| `expose_recent` | `egui_view_tab_expose_t` | `105 x 81` | locked snapshot 0 | 最近页组预览 |

## 6. 状态覆盖矩阵

| 区域 | Snapshot | 状态胶囊 | 摘要 | 底部短文案 | 说明 |
| --- | --- | --- | --- | --- | --- |
| 主卡 | 0 | `GRID` | `Three tabs ready` | `focus tab open` | 初始总览态 |
| 主卡 | 1 | `LIVE` | `Review tabs spread` | `center tab live` | 中间页活跃态 |
| 主卡 | 2 | `SAFE` | `Recall stack calm` | `last tab sealed` | 收口只读态 |
| saved | 0 | `PIN` | `Pinned tabs` | `saved group` | 已保存页组 |
| saved | 1 | `SCAN` | `Review pack` | `sort tabs` | 扫描整理态 |
| saved | 2 | `PIN` | `Pinned calm` | `safe group` | 固定收口态 |
| recent | 0 | `LAST` | `Last tabs` | `recent group` | 最近页组 |
| recent | 1 | `WARN` | `Hot queue` | `guard set` | 风险提醒态 |
| recent | 2 | `LAST` | `Recall calm` | `recent seal` | 最近封存态 |

## 7. `egui_port_get_recording_action()` 录制动作设计

运行时录制动作共 13 步：

1. 等待 `400ms`
2. 点击主卡一次
3. 等待 `300ms`
4. 点击 saved 卡一次
5. 等待 `300ms`
6. 点击 recent 卡一次
7. 等待 `300ms`
8. 再点击主卡一次
9. 等待 `300ms`
10. 再点击 saved 卡一次
11. 等待 `300ms`
12. 再点击 recent 卡一次
13. 等待 `800ms`

这样 runtime 可以输出 15 帧截图，覆盖三张卡的初始态、中间态和终态。

## 8. 编译、runtime、截图验收标准

### 编译

```bash
make all APP=HelloCustomWidgets APP_SUB=navigation/tab_expose PORT=pc
```

### Runtime

```bash
python scripts/code_runtime_check.py --app HelloCustomWidgets --app-sub navigation/tab_expose --timeout 10 --keep-screenshots
```

### 验收标准

- 不能黑屏、卡死、崩溃。
- 主卡、saved、recent 三张卡都必须完整可见，不能裁切。
- 主卡顶部 `TABS` 与 `GRID / LIVE / SAFE`，compact 顶部 `PIN / SCAN / WARN / LAST` 必须逐项检查视觉居中。
- 顶部短词胶囊与右边框必须有真实空隙，不能只以“没截断”作为通过标准。
- 底部短文案必须可读，不能贴底、贴边或左右留白失衡。
- `iteration_log/iteration_log.md` 必须同步记录每一轮目标、改动、截图与结论。

## 9. 已知限制与下一轮迭代计划

- 当前版本聚焦静态总览与点击轮询，未实现拖拽重排。
- preview 仍是信息化缩略图，不是完整内容截图。
- 如果后续要升级为框架级控件，可继续补充：
  - 键盘切换/焦点导航
  - 更多 tab 数量的分页方案
  - 与真实 viewpage/view stack 的联动

## 10. 与现有控件的重叠分析与差异化边界

- 相对 `tab_bar`：这里的重点是标签页总览和多 preview，不是简单切页。
- 相对 `breadcrumb_trail`：这里没有层级路径语义，只有同层工作页簇。
- 相对 `dock_launcher`：这里不是应用入口停靠，而是页面工作区暴露。
- 相对 `command_palette`：这里不是搜索命令，而是可视化工作页切换。
