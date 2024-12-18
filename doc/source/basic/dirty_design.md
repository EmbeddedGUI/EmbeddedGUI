# 脏矩阵设计说明

## 背景介绍

一般情况下，当屏幕内容绘制完毕后，实际应用通常需要更新屏幕中的一部分内容，而不是单纯显示一个静态图片在那。

如下图所示，屏幕中有一个图片控件（Img2）和一个文本控件（Change Text），图片控件是静态不变的，但是文本控件内容是需要变化的。从左边变到右边，文本从《Change Text》变成《New Text》，但是背景并没有变化，如果屏幕有内容变化就去更新整个屏幕，这个不仅会影响CPU性能，还会影响功耗。

对于下面的场景理论上只需要更新红色区域内容即可。**只在需要的时候重绘画面中变化的部分**。由于基于光栅的绘图技术在数据结构上总表现为一个矩形区域，且画面中变化的内容往往又被称为“弄脏了”的部分——“脏矩阵”故此得名。

脏矩阵技术的应用非常广泛，例如我们所熟悉的Windows系统，大部分时候就只会更新鼠标指针滑动所经过的那一小片矩形区域而已。

因为**脏矩阵在降低传输带宽和CPU占用方面有着不可替代的优势**，几乎所有的知名**GUI**协议栈都在默认情况下悄悄地使用各种各样的脏矩阵算法对系统帧率进行优化。

![image-20241114205533910](https://markdown-1306347444.cos.ap-shanghai.myqcloud.com/img/image-20241114205533910.png)

了解脏矩阵基本背景后，脏矩阵主要要解决的问题是**屏幕上有哪些内容需要重绘**。引用[【玩转Arm-2D】如何使用脏矩阵优化帧率（基础篇）-腾讯云开发者社区-腾讯云](https://cloud.tencent.com/developer/article/2413643)的一个极端场景。

如下图显示屏从空白到显示一条直线，大多数GUI框架都会重绘整个屏幕内容。

![image-20241114211724243](https://markdown-1306347444.cos.ap-shanghai.myqcloud.com/img/image-20241114211724243.png)



理想的脏矩阵希望是只绘制线覆盖的这部分区域，但是因为脏矩阵都是矩形的，所以这个奇怪的多边形是无法构建的。

![image-20241114211951830](https://markdown-1306347444.cos.ap-shanghai.myqcloud.com/img/image-20241114211951830.png)

那用一连串宽度（**Width**）和高度（**Height**）都为1个像素的脏矩阵来精确逼近整个斜线，也就是下图这种形式，理论上是可行，但如何精准的找到这个这些脏矩阵呢？这个需要一个特别聪明的算法，本项目是做不到的。

![image-20241114212557053](https://markdown-1306347444.cos.ap-shanghai.myqcloud.com/img/image-20241114212557053.png)

所以，通过上面分析可以发现。脏矩阵本身虽然可以有效降低刷新区域面积——提高帧率，但生成脏矩阵的算法本身却**可能**消耗巨大。这里的矛盾在于：太简陋的算法会因为一条对角线就更新整个屏幕；而“想的太多”的算法也可能会因为时间成本的积累而把脏矩阵的带来的收益抵消了不少——难就难在一个度的把握上。

## 脏矩阵策略

从上面分析来看，脏矩阵不能太智能（嵌入式小设备带不动），也不能说不要，如何设计一个好的策略至关重要。和ARM-2D的设计理念一致，与其让GUI去智能分析哪些区域需要更新，不如让用户自己去告知GUI那些内容需要更新。

对于GUI而言，只需要将用户告知的脏矩阵合并即可。避免作为用户是知道哪些内容是需要更新的，更新的时机也完全由用户来决定，这样对嵌入式设备来说问题就简化了。

在本项目中，所有的控件都是用户构建的，用户需要更新哪个控件的内容，只需要调用`egui_view_invalidate`接口，就会通知GUI，当前控件需要更新，之后GUI绘制之前，会去计算所有控件的脏矩阵区域，并将重叠的脏矩阵区域合并在一起，最终构建一个脏矩阵列表，在进行屏幕绘制时，会选择PFB和脏矩阵重叠部分，进行绘制。

下面针对各个场景的脏矩阵合并策略进行说明。

### 场景1

如下图所示，这些控件之间没有重合，`egui_core_update_region_dirty`这里判断控件和其他脏矩阵有没有重合，没有重合就从`egui_core.region_dirty_arr`找一个空的位置记录脏矩阵的坐标信息。

最终`egui_core.region_dirty_arr`会有4个脏矩阵信息。

![image-20241114215148268](https://markdown-1306347444.cos.ap-shanghai.myqcloud.com/img/image-20241114215148268.png)





### 场景2

如下图所示，Img0和Img1有重叠区域，并且Img0和Img1都需要更新时，Img0对应的脏矩阵是`Dirty0.0`，Img1对应的脏矩阵是`Dirty0.1`，`egui_core_update_region_dirty`这里会判断两个脏矩阵有重绘，会通过`egui_region_union`将2个矩阵合并为`Dirty0`，存入`egui_core.region_dirty_arr`中。

`Dirty1`和`Dirty0`无重合，所以最终`egui_core.region_dirty_arr`中会有2个脏矩阵信息。

![image-20241114215702160](https://markdown-1306347444.cos.ap-shanghai.myqcloud.com/img/image-20241114215702160.png)





### 场景3

如下图所示，Img0和Img1有重叠区域，但是只有Img1需要更新时，虽然2个控件重叠了，但是`egui_core_update_region_dirty`只会保存`Dirty0`信息，存入`egui_core.region_dirty_arr`中。

`Dirty1`和`Dirty0`无重合，所以最终`egui_core.region_dirty_arr`中会有2个脏矩阵信息。

![image-20241114220221459](https://markdown-1306347444.cos.ap-shanghai.myqcloud.com/img/image-20241114220221459.png)





### 场景4

如下图所示，考虑移动的场景，用户调用`egui_view_scroll_by`或者`egui_view_scroll_to`接口来移动控件，当新移动的位置和之前的位置没有重叠，在`egui_view_layout`中，会先将当前控件的脏矩阵信息调用`egui_core_update_region_dirty`更新到脏矩阵列表中，新的位置通过`egui_view_invalidate`更新，在下次layout时更新脏矩阵中。之后`egui_core_update_region_dirty`发现新的位置`Dirty1`和`Dirty0`没重叠，两个信息都存入`egui_core.region_dirty_arr`中。

`Dirty1`和`Dirty0`无重合，所以最终`egui_core.region_dirty_arr`中会有2个脏矩阵信息。

![image-20241114221017435](https://markdown-1306347444.cos.ap-shanghai.myqcloud.com/img/image-20241114221017435.png)





### 场景5

如下图所示，考虑移动的场景，用户调用`egui_view_scroll_by`或者`egui_view_scroll_to`接口来移动控件，当新移动的位置和之前的位置有重叠时，在`egui_view_layout`中，会先将当前控件的脏矩阵信息调用`egui_core_update_region_dirty`更新到脏矩阵列表中，新的位置通过`egui_view_invalidate`更新，在下次layout时更新脏矩阵中。之后`egui_core_update_region_dirty`发现新的位置`Dirty1`和`Dirty0`有重叠，两个矩阵合并存入`egui_core.region_dirty_arr`中。

所以最终`egui_core.region_dirty_arr`中只有1个脏矩阵`Dirty0`信息。

![image-20241114221420787](https://markdown-1306347444.cos.ap-shanghai.myqcloud.com/img/image-20241114221420787.png)



通过简单的处理，由用户来通知脏矩阵信息，相比于ARM-2D必须用户给出具体坐标信息，本项目用户只需要告知GUI哪些控件的内容有变化即可，GUI通过控件去构建脏矩阵列表。



## 脏矩阵屏幕绘制

在构建好脏矩阵信息后，就需要完成屏幕绘制。由于项目绘制是按照PFB来进行的，下面给出几个场景来说明脏矩阵如何实现提升刷新率的目的。

假设按行扫描，下面是可能遇到的各个场景。

### 场景1

当PFB和脏矩阵不重叠时，这个情况下，这个区域无需绘制。这样就少了很多绘制动作。

![image-20241114222128351](https://markdown-1306347444.cos.ap-shanghai.myqcloud.com/img/image-20241114222128351.png)



### 场景2

当PFB包含脏矩阵时，这个情况下，虚线是原本需要绘制的PFB区域，但是因为脏矩阵不大，只需要绘制实现部分的PFB就行。这样就少了很多绘制动作。

![image-20241114222331905](https://markdown-1306347444.cos.ap-shanghai.myqcloud.com/img/image-20241114222331905.png)





### 场景3

当PFB部分包含脏矩阵时，这个情况下，虚线是原本需要绘制的PFB区域，但是因为脏矩阵不大，只需要绘制实现部分的PFB就行。这样就少了很多绘制动作。

![image-20241114222434353](https://markdown-1306347444.cos.ap-shanghai.myqcloud.com/img/image-20241114222434353.png)







### 场景4

当PFB中包含多个脏矩阵时，这个情况下，虚线是原本需要绘制的PFB区域，但是因为两个脏矩阵不重叠，原本一次绘制，需要变成2次绘制，PFB0绘制1次，FPB1绘制1次。这样就少了很多绘制动作。

![image-20241114222617197](https://markdown-1306347444.cos.ap-shanghai.myqcloud.com/img/image-20241114222617197.png)










## 脏矩阵无法解决的场景

开篇说的画线的极端场景是无法优化的。

此外在支持触控的场景中，显示帧率性能并不能通过脏矩阵方案提升，如下所示场景，有2个图片Img1和Img2，从右滑动到左边时，整个屏幕的内容还是需要重绘。

所以说脏矩阵并不是万能的，部分场景还是得一个个去挖掘CPU性能。

![image-20241114211124771](https://markdown-1306347444.cos.ap-shanghai.myqcloud.com/img/image-20241114211124771.png)













