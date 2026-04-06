# Fast Path Config

本页是当前 fast-path 相关配置的唯一用户入口。

它只回答三件事：

- 现在还有哪些 fast-path 相关配置值得用户关注
- 每个配置大致在换什么 `ROM / SRAM / perf`
- 哪些历史宏已经不再是推荐用户配置

如果某个 fast-path 名字不在本页里，默认就不要把它当成当前支持的用户配置项。

## 1. 先看结论

当前真正建议用户关注的，主要只有 6 个共享配置宏：

- `EGUI_CONFIG_IMAGE_DECODE_MAX_PIXEL_SIZE`
- `EGUI_CONFIG_IMAGE_CODEC_ROW_CACHE_ENABLE`
- `EGUI_CONFIG_IMAGE_CODEC_TAIL_ROW_CACHE_ENABLE`
- `EGUI_CONFIG_IMAGE_CODEC_PERSISTENT_CACHE_MAX_BYTES`
- `EGUI_CONFIG_IMAGE_EXTERNAL_PERSISTENT_CACHE_MAX_BYTES`
- `EGUI_CONFIG_IMAGE_RLE_EXTERNAL_CACHE_WINDOW_SIZE`

这 6 个配置还保留着，是因为它们现在仍然在表达明确的产品取舍：

- decode heap 上界
- 压缩图片主路径策略
- cache / SRAM 预算

而一批只省几十到几百字节的微型 fast-path 宏，已经不再作为用户配置项提供。

## 2. 当前支持的共享配置

下表可以直接当成用户选型入口看。

| 宏 | 当前默认 | 主要作用 | 典型代价 / 收益 | 什么时候考虑调整 |
| --- | --- | --- | --- | --- |
| `EGUI_CONFIG_IMAGE_DECODE_MAX_PIXEL_SIZE` | `2` | 限制 decode 阶段的单像素 scratch 上界 | `2 -> 4` 约 `text +108B`，完整 `239` 场景无明显性能波动 | 只有当资源格式或解码路径确实需要更高像素 scratch 时再调大；一般保持默认 |
| `EGUI_CONFIG_IMAGE_CODEC_ROW_CACHE_ENABLE` | 框架默认 `0` | 控制压缩图片是否启用 row cache | 关闭可少约 `text -5108B`，但 internal tiled `QOI/RLE` 热点会回退 `+111% ~ +634%` | 项目大量使用 internal tiled `QOI/RLE` 时，优先做 A/B；否则保持默认即可 |
| `EGUI_CONFIG_IMAGE_CODEC_TAIL_ROW_CACHE_ENABLE` | `0` | 决定是否为压缩图片的 tail-row 保留更完整的 cache | 打开约 `text +9976B`，但可明显改善 `40` 个 `QOI/RLE` 热点，改善范围对应历史回退约 `+16.2% ~ +189.9%` | 只有当压缩图片热点很重要，且能接受接近 `10KB` ROM 成本时再考虑打开 |
| `EGUI_CONFIG_IMAGE_CODEC_PERSISTENT_CACHE_MAX_BYTES` | `0` | 给 codec 整图 persistent cache 预留预算 | `0 -> 5000` 约 `text +2948B, bss +24B`；历史上只有 `IMAGE_TILED_QOI_565_0` 有明显改善 `-13.1%` | 只在你已经确认 codec 热点集中在少量重复资源，且愿意为此付出额外 ROM/BSS 时再调大 |
| `EGUI_CONFIG_IMAGE_EXTERNAL_PERSISTENT_CACHE_MAX_BYTES` | `0` | 给 external image persistent cache 预留预算 | `0 -> 5000` 约 `text +948B, bss +40B`；部分 external tiled/resize 场景改善 `-32.6% ~ -61.6%` | 项目大量使用 external image，且更关心这些场景的吞吐时可考虑调大 |
| `EGUI_CONFIG_IMAGE_RLE_EXTERNAL_CACHE_WINDOW_SIZE` | `1024` | 控制 external RLE 的窗口大小，也就是一块直接的 SRAM 预算 | `1024 -> 64` 约 `text +96B, bss -1024B`，但 external RLE 主路径回退 `+17.5% ~ +21.8%` | 只在 SRAM 非常紧张，且可以接受 external RLE 明显变慢时再缩小 |

### 这 6 个配置怎么理解

- `MAX_PIXEL_SIZE` 主要是 heap 上界问题，不是典型 fast-path 开关。
- `ROW_CACHE_ENABLE` 和 `TAIL_ROW_CACHE_ENABLE` 影响的是压缩图片主路径，应当用真实场景来做 A/B。
- 两个 `*_PERSISTENT_CACHE_MAX_BYTES` 更像“预算项”，不是“应该默认打开的优化项”。
- `RLE_EXTERNAL_CACHE_WINDOW_SIZE` 是最直接的 SRAM 换性能入口。

## 3. 示例里还能看到的局部开关

当前仓内仍保留两个示例局部开关，但它们不是框架统一配置面：

| 名字 | 用途 | 对用户的建议 |
| --- | --- | --- |
| `EGUI_CONFIG_FONT_STD_FAST_DRAW_ENABLE` | 示例或模板里用来做文字路径 A/B | 普通项目不要额外发散使用；历史上关闭虽可省约 `8328B` 代码，但文本热点会回退 `+27% ~ +465%` |
| `EGUI_CONFIG_CIRCLE_FILL_BASIC` | `HelloPerformance` 的 benchmark 局部入口 | 只在 benchmark 或定向实验里使用；普通产品配置不用关注 |

如果你是在复制 `tiny_rom` / `basic_ui` 这类模板配置，看到 `APP_EGUI_*` 名字是正常的；但它们只代表模板或示例的局部选择，不代表框架统一推荐接口。

## 4. 哪些历史宏已经不建议用户再调

下面这些类型的名字，当前都不建议再作为用户配置入口依赖：

- QOI / RLE 解码内部的小 checkpoint、小索引、小 cache 宏
- 字体内部的小型 lookup / line cache 宏
- 只换回几十字节到一两百字节 `SRAM` 的 external row persistent 宏
- 只换回几十字节 `ROM / BSS` 的 source-opaque / row-cache 细节宏

原因也很直接：

- 它们大多只能换回几十到几百字节
- 继续公开出来会让配置面越来越碎
- 其中一些甚至会用很小的 `size` 收益换来非常大的性能退化

几个典型例子：

- `QOI checkpoint count`
  - 只省约 `text -276B, bss -8B`
  - 但历史 `20` 个 QOI 场景会回退 `+110% ~ +911%`
- `RLE checkpoint`
  - 只省约 `text -92B, bss -16B`
  - 但历史 `20` 个 RLE 场景会回退，峰值约 `+452%`
- `font` 的小 lookup / line cache 宏
  - 典型收益只有 `text +200B / +488B` 量级
  - 不值得继续暴露给用户做长期调参
- `std / transform external row persistent cache`
  - 典型也只有 `bss -40B / -56B`
  - 这类收益太小，不值得保留成用户可配项

判断规则可以简单记成一句话：

- 不能明显改变产品预算的微型宏，不值得继续开放给用户。

## 5. 按目标选择

### 5.1 如果你优先压 ROM

优先做这几件事：

- 保持 `EGUI_CONFIG_IMAGE_CODEC_TAIL_ROW_CACHE_ENABLE=0`
- 保持两个 `*_PERSISTENT_CACHE_MAX_BYTES=0`
- 只有在压缩图片热点确实存在时，才评估 `EGUI_CONFIG_IMAGE_CODEC_ROW_CACHE_ENABLE`

不要做的事：

- 不要再去追那些已经移除的历史微宏
- 不要为了省几百字节，把 QOI/RLE 主路径打穿

### 5.2 如果你优先压 SRAM

优先看：

- `EGUI_CONFIG_IMAGE_RLE_EXTERNAL_CACHE_WINDOW_SIZE`
- `EGUI_CONFIG_IMAGE_DECODE_MAX_PIXEL_SIZE`

推荐顺序：

1. 先确认 `MAX_PIXEL_SIZE=2` 是否已经满足需求
2. 再评估 `RLE_EXTERNAL_CACHE_WINDOW_SIZE` 是否能从 `1024` 缩小
3. 如果缩小，就必须做 external RLE 场景回归

### 5.3 如果你优先要压缩图片性能

优先看：

- `EGUI_CONFIG_IMAGE_CODEC_ROW_CACHE_ENABLE`
- `EGUI_CONFIG_IMAGE_CODEC_TAIL_ROW_CACHE_ENABLE`
- `EGUI_CONFIG_IMAGE_CODEC_PERSISTENT_CACHE_MAX_BYTES`

适用场景：

- 项目里有明显的 `QOI/RLE` 压缩图片热点
- 尤其是 internal tiled scene

### 5.4 如果你优先要 external image 性能

优先看：

- `EGUI_CONFIG_IMAGE_EXTERNAL_PERSISTENT_CACHE_MAX_BYTES`

适用场景：

- external image 读取频繁
- 同一批资源会被重复访问
- 你可以接受额外的 `ROM / BSS`

## 6. 用户文档的一条边界

本页只覆盖“当前仍值得用户调”的入口。

如果你在其它 retention 文档里看到更多 fast-path 名字，需要这样理解：

- 这些名字大多只是历史 A/B 记录
- 它们不是当前推荐用户配置面
- 对普通项目来说，不要因为历史文档里出现过，就把它们重新加回产品配置

只有在你明确要做框架维护或重新做一轮完整 A/B 时，才需要继续阅读：

- `example/HelloPerformance/fast_path_retention.md`
- `example/HelloPerformance/macro_decision_matrix.md`
- `example/HelloPerformance/macro_cleanup_summary.md`

## 7. 一句话总结

当前 fast-path 配置的用户口径很简单：

- 真正值得用户调的，只剩少数几个能明显改变 `ROM / SRAM / perf` 预算的配置项。
- 其余微型 fast-path 宏，默认都不要再碰。
