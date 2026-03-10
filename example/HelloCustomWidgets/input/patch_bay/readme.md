# patch_bay

## 1. 为什么需要这个控件

`patch_bay` 用来表达音频、广播、舞台控制等场景中的离散路由关系。常规按钮、列表或推子只能表现单点状态，无法把“输入端口 -> 输出端口 -> 当前激活路由”这种结构一眼说明白。

## 2. 为什么现有控件不够用

- `fader_bank` 关注连续通道电平，不适合表达离散端口之间的映射。
- `node_topology` 更偏网络拓扑和多节点依赖，信息密度与 patch bay 的“成对插接”语义不一致。
- `table` / `list` 可以列出路由文本，但不能直观看到端口位置、激活孔位与连线走向。
- `signal_beacon` 强调告警脉冲和节点反馈，不适合做路由插接预览。

## 3. 目标场景与示例概览

- 主卡：展示当前核心 patch 路由、状态胶囊、左右端口列与中枢连线路径。
- 左下 compact 卡：展示预备路由预览，强调 scan / arm 等临时状态。
- 右下 compact 卡：展示锁定路由预览，强调 hold / warn 等受控状态。
- 点击三个卡片分别轮播 3 组 snapshot，用于 runtime 截图验证状态覆盖。

## 4. 视觉与布局规格

- 整体 root：`220 x 306`
- 标题区：标题 `220 x 18`，说明文 `220 x 11`
- 主卡：`192 x 118`
- 状态提示文：`220 x 12`
- 分割线：`142 x 2`
- 底部容器：`212 x 86`
- 左下 compact：`108 x 80`
- 右下 compact：`100 x 80`
- 关键检查项：
  - 标题、胶囊、路由文本必须视觉居中。
  - 左右端口列到边框的留白要平衡。
  - 胶囊文字、route 文本与边框之间必须留有呼吸空间，不能贴边。

## 5. 控件清单

| 变量名 | 类型 | 尺寸 | 初始状态 | 用途 |
| --- | --- | --- | --- | --- |
| `root_layout` | `egui_view_linearlayout_t` | `220 x 306` | 垂直布局 | 根容器 |
| `title_label` | `egui_view_label_t` | `220 x 18` | `Patch Bay` | 标题 |
| `guide_label` | `egui_view_label_t` | `220 x 11` | `Tap panels to rotate routes` | 说明文 |
| `bay_primary` | `egui_view_patch_bay_t` | `192 x 118` | snapshot 0 | 主 patch 卡 |
| `status_label` | `egui_view_label_t` | `220 x 12` | `Core route live` | 当前状态反馈 |
| `section_divider` | `egui_view_line_t` | `142 x 2` | 常显 | 分割层级 |
| `bottom_row` | `egui_view_linearlayout_t` | `212 x 86` | 水平布局 | 承载下方两张 compact 卡 |
| `bay_preview` | `egui_view_patch_bay_t` | `108 x 80` | compact snapshot 0 | 预备路由预览 |
| `bay_locked` | `egui_view_patch_bay_t` | `100 x 80` | locked snapshot 0 | 锁定路由预览 |

## 6. 状态覆盖矩阵

| 区域 | Snapshot | 状态胶囊 | 路由文本 | 端口变化 | 说明 |
| --- | --- | --- | --- | --- | --- |
| 主卡 | 0 | `LIVE` | `Mic A -> Bus 2` | `A -> 2` | 正常直播路由 |
| 主卡 | 1 | `WATCH` | `Synth -> Bus 3` | `B -> 3` | 延迟预警 |
| 主卡 | 2 | `LIVE` | `FX -> Bus 1` | `C -> 1` | 切回稳定路由 |
| 左卡 | 0 | `ARM` | `Cue 1` | `A -> 1` | 预备 arm |
| 左卡 | 1 | `SCAN` | `Cue 2` | `B -> 3` | 扫描中 |
| 左卡 | 2 | `ARM` | `Cue 3` | `C -> 2` | 切换预案 |
| 右卡 | 0 | `HOLD` | `Aux 4` | `B -> 2` | 锁定保持 |
| 右卡 | 1 | `WARN` | `Aux 5` | `A -> 3` | 守护预警 |
| 右卡 | 2 | `HOLD` | `Aux 6` | `C -> 1` | 再次锁定 |

## 7. `egui_port_get_recording_action()` 录制动作设计

1. 首帧等待 `400ms`，确保默认布局稳定。
2. 点击主卡，切到主卡 snapshot 1。
3. 等待 `300ms`。
4. 点击左下 compact 卡，切到左卡 snapshot 1。
5. 等待 `300ms`。
6. 点击右下 compact 卡，切到右卡 snapshot 1。
7. 等待 `800ms`，输出最终截图。

## 8. 编译、runtime、截图验收标准

### 编译

```bash
make all APP=HelloCustomWidgets APP_SUB=input/patch_bay PORT=pc
```

### Runtime

```bash
python scripts/code_runtime_check.py --app HelloCustomWidgets --app-sub input/patch_bay --timeout 10 --keep-screenshots
```

### 截图验收

- 不能黑屏、白屏、空白。
- 主卡与底部两张 compact 卡都必须完整可见，不能裁切。
- 左右端口列与边框留白必须平衡，不能出现一侧明显更挤。
- 状态胶囊、route 文本、footer 文本要检查是否居中或左右呼吸空间合理。
- 右下 locked 卡即使样式较紧，也要确保文字不贴边。
- 每次迭代都要把关键截图复制到 `iteration_log/images/iter_xx/`，并写入 `iteration_log/iteration_log.md`。

## 9. 已知限制与下一轮迭代计划

- 当前 30 轮迭代后，主卡与 compact 卡的边距、胶囊留白、文字贴边风险已收敛到可验收状态。
- 后续若继续演进，可考虑加入更多 snapshot 或动画态，而不是继续压缩当前静态布局。
- 若后续迁移到框架公共控件层，再补充可配置端口数量、颜色主题和路由动画参数。

## 10. 与现有控件的重叠分析与差异化边界

- 不同于 `fader_bank`：这里是离散端口插接，不是连续电平控制。
- 不同于 `node_topology`：这里强调左列输入、右列输出和单条当前激活路由，而不是网状关系。
- 不同于 `table/list`：这里用几何位置表达路由，不只是文本枚举。
- 不同于 `signal_beacon`：这里的主视觉中心是端口列与 patch 连线，而不是环形脉冲信标。
