# HelloSimple

## 应用说明

HelloSimple 保持原始的 `label + button` 两控件界面：
- 一个居中的文本标签，显示 `Hello World!`
- 一个居中的按钮，显示 `Click me!`
- 点击按钮后，按钮文案依次切换为 `Done 1`、`Done 2`、`Done 3`

这版不再使用 compact 文本路径，也不再退化成自绘文本。为了继续压缩 code size，只保留不影响视觉效果的裁剪：
- 去掉 `LinearLayout`，改为手工计算两个控件的位置
- 去掉 `egui_api_sprintf`，改为固定文案表
- 关闭 `button` 的 icon / icon+text 混排路径，仅保留纯文本按钮绘制
- 保留正常 `label` / `button` 控件和默认字体绘制路径
- 默认字体切换为 HelloSimple 自己的 Montserrat 14 子集资源，只保留本例实际显示字符，保持视觉一致同时减少字体资源体积
- 关闭 `label` 的 compact fallback，避免在本例中为无 font 标签保留额外代码
- `qemu` 端关闭 custom malloc 的 libc fallback，直接使用 port 自带小堆分配器，避免为本例额外链接 newlib `malloc/free`
- `qemu` 端关闭 platform `printf/vlog` 格式化链，避免把调试格式化代码带进最终 demo 二进制
- 保留 touch 点击链路
- 继续关闭本例未使用的 activity / dialog / toast
- 继续关闭本例未使用的 canvas extra clip / spec circle info / RGB565_4
- 继续关闭本例未使用的 image / text-transform 相关 frame-cache release 与辅助缓存

## 控件列表

| 控件类型 | 变量名 | 说明 |
|---------|--------|------|
| `egui_view_label_t` | `label_1` | 显示 `Hello World!` 的文本标签 |
| `egui_view_button_t` | `button_1` | 可点击按钮，点击后更新按钮文案 |

## 录制动作

| 序号 | 动作类型 | 说明 |
|------|---------|------|
| 0 | CLICK | 点击按钮，显示 `Done 1` |
| 1 | CLICK | 点击按钮，显示 `Done 2` |
| 2 | CLICK | 点击按钮，显示 `Done 3` |
