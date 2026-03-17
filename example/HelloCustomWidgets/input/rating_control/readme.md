# rating_control 自定义控件设计说明

## 参考来源

- 参考设计系统：`Fluent 2`
- 参考开源库：`WPF UI`
- 次级补充参考：`ModernWpf`
- 对应组件名：`RatingControl`
- 本次保留状态：`standard`、`compact`、`read only`、`clear action`、`keyboard rating`
- 删减效果：Acrylic、hover 动画、半星评分、真实 glyph 资源、桌面级 tooltip 与复杂 pointer over 状态
- EGUI 适配说明：保留标准星级评分、页内 `Clear` 入口和键盘步进语义；在 `240 x 320` 下优先保证星形排布、标题留白和 compact / read-only 对照清晰可审阅

## 1. 为什么需要这个控件

`rating_control` 用来表达标准页内评分语义，比如服务评价、交付速度、安装体验、反馈表单里的满意度选择。它比 `radio_button` 更接近用户熟悉的星级评分模型，也比 `slider` 更适合离散的 ordinal rating。

## 2. 为什么现有控件不够用

- `radio_button` 只能表达互斥选项，不具备星级评分的视觉语义
- `slider` 偏连续拖动，不适合 `1..5` 档位的离散评分
- `segmented_control` 更适合页内切换，不适合表达满意度或质量等级
- 当前主线里缺少一版接近 `Fluent 2 / WPF UI RatingControl` 的标准评分控件

## 3. 目标场景与示例概览

- 主区域展示标准 `rating_control`，包含标题、低高标签、caption 和 `Clear`
- 左下 `Compact` 预览展示紧凑页内评分条
- 右下 `Read only` 预览展示只读弱化评分条
- 主卡支持触摸点击星级、点击 `Clear` 清空
- 主卡支持 `Left / Right / Up / Down / Home / End / Tab / Enter / Space / Esc`
- `Clear` 聚焦时会预览空评分；按 `Left` 可回到已提交星位焦点；按 `Enter` / `Space` / `Esc` 可执行清空
- 点击 guide 文案切换标准场景；点击 `Compact` 标题切换紧凑场景

目录：

- `example/HelloCustomWidgets/input/rating_control/`

## 4. 视觉与布局规格

- 画布：`240 x 320`
- 根布局：`224 x 288`
- 页面结构：标题 -> guide -> `Standard` -> 主评分卡 -> 状态文案 -> 分隔线 -> `Compact / Read only`
- 主评分卡：`196 x 92`
- 底部双预览容器：`216 x 68`
- `Compact` 预览：`106 x 42`
- `Read only` 预览：`106 x 42`
- 视觉规则：
  - 采用浅灰 page panel + 白色评分卡，避免回到 showcase / HMI 风格
  - 星形使用暖金色 accent，符合标准 rating control 语义
  - `Clear` 保留轻量 chip 入口，聚焦时只做弱 accent 强化，不做重 hover 装饰
  - read-only 通过统一降噪 palette 弱化，而不是增加额外装饰层

## 5. 控件清单

| 变量名 | 类型 | 尺寸 (W x H) | 初始状态 | 用途 |
| --- | --- | ---: | --- | --- |
| `root_layout` | `egui_view_linearlayout_t` | 224 x 288 | enabled | 页面根布局 |
| `title_label` | `egui_view_label_t` | 224 x 18 | `Rating Control` | 页面标题 |
| `guide_label` | `egui_view_label_t` | 224 x 12 | 可点击 | 切换标准 snapshot |
| `control_primary` | `egui_view_rating_control_t` | 196 x 92 | `Service quality` | 标准评分卡 |
| `status_label` | `egui_view_label_t` | 224 x 12 | `Service 4/5 Great` | 当前已提交评分状态 |
| `control_compact` | `egui_view_rating_control_t` | 106 x 42 | `3 / 5` | 紧凑评分预览 |
| `control_locked` | `egui_view_rating_control_t` | 106 x 42 | `4 / 5` | 只读评分预览 |

## 6. 状态覆盖矩阵

| 状态 / 区域 | 主评分卡 | Compact | Read only |
| --- | --- | --- | --- |
| 默认态 | `Service quality = 4/5` | `3/5` | `4/5` |
| 点击星级 | 更新评分与 caption | 更新 compact 评分 | 不响应 |
| 点击 `Clear` | 清空主评分 | 不显示 | 不显示 |
| 键盘 `Home` | 跳到最低评分 | 不录制 | 不适用 |
| 键盘 `End` | 跳到最高评分 | 不录制 | 不适用 |
| 键盘 `Left` | 从已评分退一档；若当前聚焦 `Clear` 则回到已提交评分焦点 | 不录制 | 不适用 |
| 键盘 `Esc` | 执行清空或回到默认焦点 | 不录制 | 不适用 |
| 场景切换 | `Service / Delivery / Setup` | `Compact A / B` | 固定只读 |

## 7. `egui_port_get_recording_action()` 录制动作设计

1. 等待首帧并抓取默认场景
2. 点击主卡第 5 颗星，验证触摸评分提交
3. 点击 `Clear`，验证真实 `part hit-testing`
4. 切换到第二组标准 snapshot
5. 调用 `egui_view_rating_control_handle_navigation_key()` 执行 `End`
6. 再执行 `Left`，验证键盘回退
7. 通过 `Tab -> Tab` 聚焦 `Clear`
8. 抓取 `Clear` 聚焦预览截图
9. 执行 `Left`，验证从 `Clear` 回到已提交评分焦点
10. 再次 `Tab -> Tab` 回到 `Clear`
11. 执行 `Esc`，验证 `Clear` 聚焦时的键盘清空
12. 执行 `Home`，验证清空后能回到最低评分
13. `Tab` 到第 2 颗星并执行 `Space`，验证键盘激活评分提交
14. 再 `Tab` 到第 3 颗星并执行 `Enter`，验证另一条键盘激活路径
15. 再 `Tab -> Tab -> Tab` 回到 `Clear`，执行 `Space`，验证 `Clear` 聚焦下的另一条键盘清空路径
16. 恢复主卡到有值状态，再 `Tab -> Tab -> Tab` 回到 `Clear`，执行 `Enter`，验证第三条键盘清空路径
17. 切换 compact snapshot
18. 点击 compact 第 3、4 颗星之间的空隙，验证 row-level hit-testing 与紧凑态交互
19. 重置主卡到 `Delivery speed 2/5`，从第 2 颗星拖拽到第 5 颗星，验证触摸拖拽会以最新命中星位提交

录制同时依赖：

- `egui_view_rating_control_get_part_region()`：提供真实点击和拖拽起止区域
- `egui_view_rating_control_handle_navigation_key()`：复用控件内部键盘路径，而不是单独伪造状态
- `EGUI_SIM_ACTION_DRAG`：补齐触摸拖拽链路的 runtime 证据，不只停留在单测层

## 8. 编译、runtime、截图验收标准

```bash
make all APP=HelloCustomWidgets APP_SUB=input/rating_control PORT=pc
python scripts/code_runtime_check.py --app HelloCustomWidgets --app-sub input/rating_control --timeout 10 --keep-screenshots
make all APP=HelloUnitTest PORT=pc_test
output\main.exe
```

验收重点：

- 主卡、compact、read-only 三个评分控件都必须完整可见
- 主卡标题、星形、caption、低高标签与 `Clear` 之间留白平衡
- 星形焦点 ring 需要可辨识，但不能变成高噪音装饰
- compact 必须像标准控件压缩版，而不是另一种 showcase 卡片
- read-only 必须明显弱化，同时保留评分结果
- `Clear` 聚焦预览、从 `Clear` 回退到星位、`Enter / Space / Esc` 键盘清空、`Home` 恢复最低评分、`Space / Enter` 键盘提交这几段链路都要能从截图读出来
- runtime 录制还要能看出至少一条触摸拖拽链路，确认拖拽换星后会以最新命中星位提交
- `HelloUnitTest` 需要同时通过 `rating_control` 相关断言，至少覆盖：
  - `Enter / Space / Esc` 三条 `Clear` 键盘清空路径
  - `Left / Right / Up / Down / Home / End / Tab` 的导航与边界语义
  - `compact / read only / disabled / clear disabled` 下的状态保护
  - `get_part_region()` 的 `Clear` 显隐规则与 `item_count / value / label_count` 归一化行为

## 9. 已知限制与下一轮迭代计划

- 当前只支持整星评分，不做半星或浮点评分
- 当前 `Clear` 是轻量页内入口，不弹出二次确认
- 当前键盘路径聚焦在标准表单步进，不覆盖复杂 accessibility narration
- 当前已具备 runtime 录制证据链与 `HelloUnitTest` 导航/归一化断言，但如果后续沉入框架层，仍需要补与通用 touch/focus API 的公共测试接线
- 如果后续需要沉入框架层，再评估与通用 focus / form 体系的对接

## 10. 与现有控件的重叠分析与差异化边界

- 相比 `radio_button`：这里是标准星级评分，不是单纯表单互斥项
- 相比 `slider`：这里表达离散等级，不是连续区间拖动
- 相比 `segmented_control`：这里是评价语义，不承担页内切换
- 相比 `number_box`：这里表达质量 / 满意度档位，而不是数值输入

## 11. 参考设计系统与开源母本

- 参考设计系统：`Fluent 2`
- 开源母本：`WPF UI`
- 次级补充：`ModernWpf`

## 12. 对应组件名，以及本次保留的核心状态

- 对应组件名：`RatingControl`
- 本次保留状态：
  - `standard`
  - `compact`
  - `read only`
  - `clear action`
  - `keyboard rating`

## 13. 相比参考原型删掉了哪些效果或装饰

- 不做 hover glow、桌面级 pointer over 动画
- 不做半星、caption 图标、系统级 tooltip
- 不做复杂主题层、Acrylic 和阴影扩散
- 不接入真实 emoji / glyph 资源，星形直接用轻量绘制

## 14. EGUI 适配时的简化点与约束

- 用固定 `1..5` 星级 reference 作为主演示，优先保证 `240 x 320` 下排布稳定
- 标准态保留 `title + stars + caption + low/high + clear` 五段结构
- caption 跟随预览值绘制，确保 `Clear` 聚焦和星级按压时语义一致
- compact 只保留核心评分排布，压缩非必要文案
- read-only 通过 palette 弱化与交互关闭收口
