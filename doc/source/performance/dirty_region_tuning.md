# 脏矩形细粒度优化指南

## 概述

在 EmbeddedGUI 中，默认的 `egui_view_invalidate()` 会把整个控件的 `region_screen` 标记为脏区域。这对布局变化最安全，但在“控件内部只有一小块内容发生变化”的场景里，会带来额外的重绘面积、PFB 分块遍历和 `on_draw()` 计算开销。

这类场景后续可以继续做细粒度优化：由控件自己定义一组稳定的子区域，把它们当成控件内部的“自定义脏矩形矩阵”来使用。这里的“矩阵”本质上仍然是一组矩形，只是这组矩形由控件作者按业务语义手工拆分，而不是让框架自动推导任意形状。

当前框架已经具备以下能力：

- `egui_view_invalidate_region()`：只让控件内部的一个局部区域失效
- `egui_view_invalidate_sub_region()`：通过预定义的子区域表失效固定分区
- `egui_canvas_is_region_active()`：在 `on_draw()` 内判断当前子元素是否落在当前 PFB tile 中，避免无关绘制计算
- `EGUI_CONFIG_DEBUG_DIRTY_REGION_STATS`：输出每帧脏区面积和 PFB tile 统计
- `scripts/dirty_region_stats_report.py`：把日志自动汇总成 Markdown / CSV 报告

注意：这套方法优化的是**脏区面积**和**绘制计算量**。最终帧时间仍应通过 QEMU 性能测试确认，PC 录制结果更适合用来观察脏区覆盖是否缩小。

## 什么时候值得做

- 高频、小面积变化：例如光标闪烁、按钮按压态、单个网格项选中态
- 控件内部结构稳定：例如上下按钮区、中间数值区、日期格子区
- 子元素边界容易计算：例如固定 cell、固定 icon、固定 badge、固定 progress 段
- `on_draw()` 内部本身存在明显分区，可以按区域跳过计算

## 什么时候不建议做

- 布局重新计算：尺寸、位置、padding、换行范围发生变化时
- 文本重排：内容长度变化导致整体 text layout 变化时
- 大范围翻页/切月/切主题：旧区域和新区域整体差异较大时
- 控件内部几何关系难以稳定描述，维护成本高于收益时

这类场景优先继续使用 `egui_view_invalidate()` 做整控件失效。

## 可用工具

| 工具 | 作用 | 适用场景 |
|------|------|----------|
| `egui_view_invalidate()` | 让整个控件失效 | 布局变化、文本重排、全量刷新 |
| `egui_view_invalidate_region()` | 让控件局部区域失效 | 光标、局部高亮、单个子元素变化 |
| `egui_view_invalidate_sub_region()` | 通过固定索引失效子区域 | 网格、分段按钮、仪表盘固定分区 |
| `egui_canvas_is_region_active()` | 判断子区域是否与当前 PFB tile 相交 | `on_draw()` 内跳过不相关绘制 |
| `EGUI_CONFIG_DEBUG_DIRTY_REGION_STATS` | 输出每帧脏区统计 | 观察优化是否真正缩小了脏区 |
| `scripts/dirty_region_stats_report.py` | 汇总日志为 Markdown / CSV | 多个场景横向对比、留档 |

## 推荐实现模式

### 模式 1：直接计算局部脏区

适合少量、动态但仍可计算的位置。

```c
static void my_widget_invalidate_icon(egui_view_t *self, egui_dim_t x, egui_dim_t y)
{
    EGUI_REGION_DEFINE(dirty_region, x, y, 16, 16);
    egui_view_invalidate_region(self, &dirty_region);
}
```

要点：

- `dirty_region` 使用**控件本地坐标**
- 框架会自动转换到屏幕坐标并裁剪到 `region_screen`
- 如果旧状态和新状态位置不同，需要同时失效旧区域和新区域

### 模式 2：预定义子区域表

适合固定分区明显的控件，例如九宫格、分栏按钮、步骤条、日历格子。

```c
enum
{
    MY_WIDGET_REGION_LEFT,
    MY_WIDGET_REGION_CENTER,
    MY_WIDGET_REGION_RIGHT,
};

static egui_sub_region_t s_regions[] = {
    { .region = {{0, 0}, {40, 32}} },
    { .region = {{40, 0}, {80, 32}} },
    { .region = {{120, 0}, {40, 32}} },
};

static const egui_sub_region_table_t s_region_table = {
    .regions = s_regions,
    .count = sizeof(s_regions) / sizeof(s_regions[0]),
};
```

状态变化时：

```c
egui_view_invalidate_sub_region(self, &s_region_table, MY_WIDGET_REGION_CENTER);
```

推荐把子区域表当成控件内部的“脏矩形矩阵”：每个分区都保持语义稳定、边界固定，后续维护成本最低。

### 模式 3：绘制阶段配合 PFB 早裁剪

仅缩小 dirty area 还不够，如果 `on_draw()` 仍然无条件遍历所有子元素，CPU 计算开销还是会留下来。

```c
EGUI_REGION_DEFINE(icon_screen_region,
                   region.location.x + icon_x,
                   region.location.y + icon_y,
                   16,
                   16);

if (egui_canvas_is_region_active(&icon_screen_region))
{
    draw_icon(...);
}
```

推荐只对以下“计算不便宜”的子元素做检查：

- 文本测量、字符串格式化
- 渐变、阴影、圆弧等较重的绘制逻辑
- 大量循环中的单元格/节点/子项

对于非常便宜的直线、分隔线，通常可以不必过度拆分。

## 设计原则

### 1. 按业务语义拆分，不按像素极限拆分

优先拆成“光标区 / 文本区 / 日期格子区 / 上下按钮区”这种稳定的语义块，而不是为了追求最小面积把控件拆成大量零碎矩形。矩形越多，维护成本越高，合并和遍历成本也越高。

### 2. 同时考虑“失效”和“绘制”

只做 `invalidate_region()` 而不做 `egui_canvas_is_region_active()`，通常只能减少 dirty area，不能完全减少 `on_draw()` 内部的无效计算。最佳实践是两边一起做。

### 3. 旧状态和新状态都要覆盖

选择态、焦点态、拖动位置、光标位置这类状态切换，经常不是只刷“新位置”就够了，旧位置留下的视觉残影也必须失效。

### 4. 布局变化时及时回退到全量 invalidate

以下情况应优先回退为 `egui_view_invalidate()`：

- 文本长度变化导致换行或滚动范围变化
- 月份/年份切换导致整个网格内容变化
- 控件尺寸变化、padding 变化、字体变化
- 多个子区域会一起失效，局部优化已经不划算

### 5. 统计值只作为“覆盖率证据”

`dirty_area`、`dirty_ratio_percent`、`pfb_tiles` 证明的是“刷新的面积是不是变小了”，不是最终 FPS。真正的时间收益仍然要看 QEMU 上的性能基准。

## 当前优化案例

下面的数据来自 `perf_output/dirty_region_stats/hello_basic_targets.md`，反映的是脏区覆盖率，不是 QEMU 帧时间：

| 控件 | 触发场景 | 细粒度策略 | 观测结果 |
|------|----------|------------|----------|
| `textinput` | 光标闪烁 | 只失效光标区域，并在 `on_draw()` 中区分文本区/光标区 | 最优局部帧 `dirty_area=18`，仅占全屏 `0.02%` |
| `number_picker` | 按压态 / 数值更新 | 拆成上、中、下三区域 | 局部刷新平均占全屏 `4.79%` |
| `mini_calendar` | 日期切换 | 只失效旧日期格和新日期格 | 局部刷新约占全屏 `2.18%` |

这些案例说明：一旦控件内部存在稳定的局部分区，手工定义脏矩形矩阵通常比整控件失效更有效。

## 观测与验证流程

### 1. 开启脏区统计

构建时打开：

```bash
make all APP=HelloBasic APP_SUB=textinput PORT=pc \
  USER_CFLAGS="-DEGUI_CONFIG_RECORDING_TEST=1 -DEGUI_CONFIG_DEBUG_DIRTY_REGION_STATS=1"
```

### 2. 录制运行日志

运行 PC 录制，让日志里输出 `DIRTY_REGION_STATS:`。

### 3. 生成汇总报告

```bash
python scripts/dirty_region_stats_report.py \
  --input textinput=perf_output/dirty_region_logs/textinput.log \
  --input number_picker=perf_output/dirty_region_logs/number_picker.log \
  --input mini_calendar=perf_output/dirty_region_logs/mini_calendar.log \
  --output-prefix perf_output/dirty_region_stats/hello_basic_targets
```

脚本会生成：

- `<prefix>.md`
- `<prefix>.csv`

脚本支持 PC 录制产生的 UTF-8 / UTF-16 日志，适合直接留档和横向比较。

### 4. 检查三类指标

- `dirty_area / screen_area`：局部刷新面积是否真的缩小
- `pfb_tiles`：同一帧涉及的 PFB 分块数量是否下降
- `best partial frame / worst partial frame`：最好和最差局部帧是否符合预期

### 5. 最终仍用 QEMU 验证性能

如果要确认“时间上到底快了多少”，仍然要回到 QEMU 基准测试。PC 录制适合看脏区覆盖，QEMU 适合看真实耗时。

## 开发检查清单

- 是否真的只有控件内部局部区域在变化
- 是否优先选择了稳定、可维护的子区域划分
- 是否同时处理了旧状态和新状态的失效
- `on_draw()` 内是否为高成本子元素增加了 `egui_canvas_is_region_active()` 判断
- 是否保留了回退到 `egui_view_invalidate()` 的路径
- 是否检查了运行截图，确认没有残影、缺刷和布局错乱
- 是否输出了 `DIRTY_REGION_STATS` 并生成对比报告
- 是否在 QEMU 上做了最终性能确认
