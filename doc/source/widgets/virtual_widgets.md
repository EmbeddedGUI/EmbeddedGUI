# 虚拟容器与大数据集控件

这组文档面向需要在 EmbeddedGUI 中承载大数据集、可变尺寸内容和复杂交互状态的场景。  
它讲的不是传统 `List`，而是建立在 `virtual_viewport` 之上的一整套虚拟容器家族：

- `virtual_viewport`
- `virtual_list`
- `virtual_page`
- `virtual_strip`
- `virtual_grid`
- `virtual_section_list`
- `virtual_tree`

如果你的界面只有十几个固定子控件，或者传统 `List` 已经足够，就不需要引入这套体系。  
如果你的场景满足下面任意一条，建议优先阅读本章：

- 数据量可能达到几百到上千条
- item 需要按可见区动态创建
- item 尺寸会变化
- 数据会频繁插入、删除、移动、patch
- 需要围绕 `stable_id` 做精确定位和局部刷新
- item 有动画态、编辑态、展开态、选中态，需要避免因回收而丢状态

## 阅读顺序

```{toctree}
:maxdepth: 1

virtual_design
virtual_api
```

## 你会在这组文档里看到什么

[设计理念与选型](virtual_design.md)

- 为什么这套控件不是单纯的 ListView
- `stable_id`、slot 复用、`keepalive`、`state cache` 的设计目的
- 各个 virtual 容器的适用边界
- 带截图的语义差异对比

[API 与接入模板](virtual_api.md)

- `params`、`data_source`、`setup` 应该怎么写
- 各容器最小接入骨架
- 常用 helper 和通知接口怎么选
- 动画、点击、滚动、定位和验证流程

## 快速选型表

| 容器 | 最小业务单元 | 典型场景 |
| --- | --- | --- |
| `virtual_viewport` | 完全自定义 | 画布块、聊天气泡、看板卡、自定义混排 |
| `virtual_list` | row | feed、日志、普通消息列表 |
| `virtual_page` | section / module | 仪表盘、设置页大区块、详情页模块 |
| `virtual_strip` | rail item | 海报带、时间轴条带、播放队列 |
| `virtual_grid` | tile / card | 商品宫格、缩略图墙、tile 面板 |
| `virtual_section_list` | section header + item | 设置分组、工单分组、消息分组 |
| `virtual_tree` | node | 文件树、拓扑树、组织树、任务树 |

## 与传统 `List` 的关系

传统 [列表控件](list_widgets.md) 解决的是“小规模、固定结构的滚动条目”。  
virtual 家族解决的是“大数据集、按需创建、状态恢复、精确通知”的问题。

因此在文档和工程实践里，可以这样区分：

- 条目数量很小，且没有复杂局部状态：优先传统 `List`
- 条目数量很大，或条目结构复杂，或需要围绕 `stable_id` 操作：优先 virtual 家族

## 相关示例

- `example/HelloBasic/virtual_viewport/`
- `example/HelloBasic/virtual_page/`
- `example/HelloBasic/virtual_strip/`
- `example/HelloBasic/virtual_grid/`
- `example/HelloBasic/virtual_section_list/`
- `example/HelloBasic/virtual_tree/`

## 相关头文件

- `src/widget/egui_view_virtual_viewport.h`
- `src/widget/egui_view_virtual_list.h`
- `src/widget/egui_view_virtual_page.h`
- `src/widget/egui_view_virtual_strip.h`
- `src/widget/egui_view_virtual_grid.h`
- `src/widget/egui_view_virtual_section_list.h`
- `src/widget/egui_view_virtual_tree.h`
