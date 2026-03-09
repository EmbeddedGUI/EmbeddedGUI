# coverflow_strip

## 1. 为什么需要这个控件

`coverflow_strip` 用来表达“当前主卡 + 左右待切换卡”的导航选择关系，适合做媒体场景、工作区切换、页组浏览这类以当前焦点为核心的连续入口。

## 2. 为什么现有控件不够用

- `dock_launcher` 强调底部停靠栏和图标放大，不强调中心主卡预览。
- `tab_expose` 强调多页总览，不强调左右透视侧卡和连续轮转。
- `command_palette` 强调搜索和结果列表，不表达触达前后的空间层级。

## 3. 目标场景与示例概览

- 中央主卡：展示当前焦点场景和主信息。
- 左右侧卡：展示前后相邻场景，用更小尺寸和更弱层级承托。
- 底部索引点：表达当前页位于序列中的位置。
- 底部状态带：反馈最近一次由左卡、主卡或右卡触发的切换行为。

## 4. 视觉与布局规格

- 根控件尺寸：`240 x 280`
- 顶部：居中标题 + guide
- 中央主卡：约 `136 x 154`
- 左右侧卡：约 `58 x 120`
- 状态胶囊短词必须检查真实居中和左右内边距
- 底部状态带需要保留上下安全距离，不能贴近边框

## 5. 控件清单

| 变量名 | 类型 | 尺寸 | 初始状态 | 用途 |
| --- | --- | --- | --- | --- |
| `coverflow_strip` | `egui_view_coverflow_strip_t` | `240 x 280` | `current=0` | 整个 coverflow 导航控件 |
| `snapshots` | `egui_view_coverflow_strip_snapshot_t[4]` | - | `Focus Stack` | 四个可轮转场景 |

## 6. 状态覆盖矩阵

| Snapshot | 状态词 | Summary | Footer | 语义 |
| --- | --- | --- | --- | --- |
| 0 | `LIVE` | `Center card tracks the active route` | `live queue` | 默认焦点态 |
| 1 | `SCAN` | `Side cards stay visible for fast recall` | `scan lane` | 搜索轮转态 |
| 2 | `SAFE` | `Locked mode keeps the strip calm` | `safe shelf` | 保守审阅态 |
| 3 | `SYNC` | `Primary card jumps forward on tap` | `sync deck` | 同步推进态 |

## 7. `egui_port_get_recording_action()` 录制动作设计

- 首帧等待，确认默认态稳定。
- 点击右侧卡一次，验证右侧提升为主卡。
- 点击左侧卡一次，验证回退逻辑。
- 点击主卡一次，验证中心点击推进。
- 再点击右侧卡一次，覆盖另一组状态变化。

## 8. 编译、runtime、截图验收标准

```bash
make all APP=HelloCustomWidgets APP_SUB=navigation/coverflow_strip PORT=pc
python scripts/code_runtime_check.py --app HelloCustomWidgets --app-sub navigation/coverflow_strip --timeout 10 --keep-screenshots
```

验收重点：

- 不能黑屏、白屏、全空白
- 中央主卡、左右透视侧卡和底部状态带都必须完整可见
- 状态胶囊和底部短文案要检查居中与内边距
- 交互后必须能从截图中看出主卡轮转变化

## 9. 已知限制与下一轮迭代计划

- 当前是静态 snapshot 轮转，没有真实透视动画。
- 当前侧卡仍是平面矩形层级，不是斜切几何。
- 后续迭代将重点打磨主卡/侧卡比例、短词胶囊居中和底部状态带边距。

## 10. 与现有控件的重叠分析与差异化边界

- 区别于 `dock_launcher`：这里是中心主卡驱动，不是图标停靠栏。
- 区别于 `tab_expose`：这里是前后相邻卡轮转，不是多页平铺总览。
- 区别于 `command_palette`：这里是空间化浏览导航，不是搜索驱动命令列表。
