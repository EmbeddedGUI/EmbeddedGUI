# PFB设计说明

## 背景介绍

一般来说，要实现屏幕显示，就是向特定像素点写入颜色值，最简单的办法就是直接通过SPI接口，向显示器芯片的特定缓存地址，写入像素点。一般来说，显示器芯片会提供2个基本操作API，`WritePoint`和`WriteRegion`。`WritePoint`实现对特定像素点写入一个色彩值，通常用于绘制文字这些。`WriteRegion`用于向特定像素点开始的区域Width/Height区间写入一块数据，通常用于绘制图像。

如下图所示，主芯片通过SPI接口，直接向显存中写入数据。调用`WriteRegion`接口，向(x0,y0)坐标点绘制图像；多次调用`WritePoint`，主芯片计算每个字体的像素点偏移，加上(x1,y1)基本坐标，来绘制字体。

这个基本上所有卖的显示器提供的测试Demo都是这样子操作方式，这个方式的好处是简单直接，对主芯片的资源暂用最少。RAM全部用显示器芯片的显存。大家学校期间搞的东西很多都是这个样子，基本满足很多场景的需要了。

![image-20241024095939664](https://markdown-1306347444.cos.ap-shanghai.myqcloud.com/img/image-20241024095939664.png)

这种方式这么好，资源用的也少，为什么不都这样干呢？那下面举1个例子，如果要更新文字或者更新图像，基本就是要清除原本的显示缓存，重新向新坐标点写入数据。

### 动态刷新需求

这里会涉及一个问题，显示屏有内容刷新需求了，主芯片就要有状态机管理什么时候显示那些内容，显示器内容不再是一成不变的。像是下面的需求看起来也还好，不是很难，继续用显示器芯片的缓存就行了。

![image-20241024101744908](https://markdown-1306347444.cos.ap-shanghai.myqcloud.com/img/image-20241024101744908.png)



### 带透明通道图片叠加

如下一个带背景，并且有透明通道的图片要显示呢？

如果没有背景，那直接显示，有Alpha=0xFF的用图片值，没Alpha=0x00就不绘制，Alpha其他值，就直接混合一个默认值就好了。

有背景，并且背景不是单色呢，那这时候就必须做图像混合了。一般情况下，就需要用RAM去记录背景色，取到新图片和就图片之间的重叠区域，当Alpha不为0时，就需要进行混合了。

![image-20241024103223469](https://markdown-1306347444.cos.ap-shanghai.myqcloud.com/img/image-20241024103223469.png)

其实好看点的字体也是带透明通道的，如下一个字体放大后是这样的，透明通道本身需要根据背景色的不同混合的呈现效果也不相同。

![image-20241024110328220](https://markdown-1306347444.cos.ap-shanghai.myqcloud.com/img/image-20241024110328220.png)

![image-20241024110448996](https://markdown-1306347444.cos.ap-shanghai.myqcloud.com/img/image-20241024110448996.png)

### 页面滑动需求

而要是有下面这种滑动页面需求，并且里面的图像还涉及混合透明图片怎么办呢？虽然依然可以主芯片自己算，但是为了维护需求，都是需要用缓存来实现的。

![20240831_164234_viewpage.gif](https://markdown-1306347444.cos.ap-shanghai.myqcloud.com/img/20240831_164234_viewpage.gif)



## 部分帧缓存PFB（Partial Frame Buffer）

上面说的，还有个一个办法，直接用显示器芯片的显存不就好了吗，但是因为要从显示器芯片获取像素点数据，这个又会影响性能了。

一般的做法就是在主芯片里面做显示缓存，最省事就是缓存整个屏幕，但是嵌入式开发中，RAM资源是非常宝贵的，一般小资源的芯片只有16KB~32KB，富裕点的也就100~200KB。但是一个320*240的RGB565屏幕，如果需要缓存整个屏幕的资源的话，需要`320*240*2=153,600B=150KB`，根本不现实。

部分帧缓存PFB技术就是为了解决这个问题。本项目的PFB技术是可以支持任意大小的，哪怕只有8*8大小的PFB也可以支持任意大小的屏幕，最关键的是用户界面API是不用关心PFB的大小的，本项目都弄好了，我们可以假装拥有完整的Frame Buffer。

通过配置`EGUI_CONFIG_PFB_WIDTH`和`EGUI_CONFIG_PFB_HEIGHT`就可以指定PFB大小，可以根据项目需求调整。



## PFB刷新机制

如下图的显示屏，从坐标(0,0)点，按照列到行的扫描顺序覆盖整个屏幕，每次都只计算PFB区域要显示的内容，然后通过`WriteRegion`接口，将这块区域的显示信息更新到显示器芯片中。

当然这里涉及到一个脏矩阵的概念，只有需要更新的区域才会进行PFB计算，并更新到显示屏中，这样大大加快的屏幕刷新速度。

![image-20241024112832708](https://markdown-1306347444.cos.ap-shanghai.myqcloud.com/img/image-20241024112832708.png)



## PFB实现机制

本项目底层的图像绘制都是通过`egui_canvas_t`实现的，`egui_canvas_t`中会保持当前的PFB缓存（`egui_color_int_t *pfb;`）和大小信息（`egui_region_t pfb_region;`）。到最后绘制接口最终会调用`egui_canvas_draw_point()`接口。

这个接口中会通过`egui_region_pt_in_rect`判断所需绘制的像素点是否在`base_view_work_region`（这个是PFB在view的坐标Region，本质是`pfb_region`）中，如果在则转换为PFB坐标，并更新到PFB缓存`pfb`中。

```
void egui_canvas_draw_point(egui_dim_t x, egui_dim_t y, egui_color_t color, egui_alpha_t alpha)
{
    egui_canvas_t *self = &canvas_data;

	...

    if (egui_region_pt_in_rect(&self->base_view_work_region, x, y))
    {
        egui_dim_t pos_x = x - self->pfb_location_in_base_view.x;
        egui_dim_t pos_y = y - self->pfb_location_in_base_view.y;

        egui_color_t *back_color = (egui_color_t *)&self->pfb[pos_y * self->pfb_region.size.width + pos_x];

        if (alpha == EGUI_ALPHA_100)
        {
            *back_color = color;
        }
        else
        {
            egui_rgb_mix_ptr(back_color, &color, back_color, alpha);
        }
    }
}
```

这样应用层并不需要管PFB是什么，只管在canvas进行绘制，最终canvas会判断像素点是否在PFB中，并进行像素点混合。

## PFB性能分析

既然PFB技术这么好，还省RAM，那有什么不足吗？显而易见的就是性能问题，用的PFB越小，屏幕刷新的速率就越慢，因为本身就是用时间来换空间的。

如之前所述，PFB机制对用户是无感的，是canvas在最后时刻决定的，当绘制各种基本图形/图片/文字时，其实CPU是有很多运算的，这些都会影响mips，从设计角度讲，最好不在PFB区域内的图形计算不要进行。

但是实际要实现这一逻辑会很复杂，大多数GUI只做了矩形的二次裁剪。本项目考虑到实际业务中会使用很小的PFB，所以对项目所有基本图形都进行了二次裁剪，进而尽量满足有限RAM依然能够有较好性能。

如下所示例子，最好是Img0全部绘制，Img2上半部分参与绘制，Img1左侧区域绘制。实际大多数项目能处理好Img0和Img2的CPU损耗，但是对于Img1的处理很多时候都不是很友好。

![image-20241024115741046](https://markdown-1306347444.cos.ap-shanghai.myqcloud.com/img/image-20241024115741046.png)



## PFB不同尺寸测试

项目设计之初就考虑到PFB很小的场景，专门设计各种case来验证CPU所需的性能（不考虑降数据写入屏幕显存时间）。进行性能测试时，不建议在PC上运行，最好是在单片机环境中进行，这样也可以知道自己芯片中那些绘图操作最耗mips，进而从UI层面优化，减少复杂动作。

运行`HelloPerformace`例程中的，平台选择`stm32g0`。命令如下，接好项目配套开发板后，就会下载程序到单片机中，串口会打印时间信息。

```
make run APP=HelloPerformace PORT=stm32g0
```

同时修改`HelloPerformace`例程中的一些配置，`EGUI_CONFIG_DEBUG_SKIP_DRAW_ALL`设置为1（去除绘制到屏幕的时间）。`EGUI_CONFIG_PFB_WIDTH`和`EGUI_CONFIG_PFB_HEIGHT`按需配置。

![image-20241024135911010](https://markdown-1306347444.cos.ap-shanghai.myqcloud.com/img/image-20241024135911010.png)

对串口打印数据进行统计如下。

选择行和高为屏幕尺寸10分之一的场景`24*32`，大多数基本图形绘制性能就与`240*32`还有`24*320`差不多了。

| 场景                                    | 12*8    | 24*32   | 240*32  | 24*320  |
| --------------------------------------- | ------- | ------- | ------- | ------- |
| LINE                                    | 104.8ms | 21.6ms  | 10.7ms  | 9.6ms   |
| IMAGE_565                               | 49.1ms  | 27.4ms  | 23.6ms  | 24.8ms  |
| IMAGE_565_1（Flash不够，未绘制）        | 23.8ms  | 7.4ms   | 5.4ms   | 5.4ms   |
| IMAGE_565_2（Flash不够，未绘制）        | 23.8ms  | 7.5ms   | 5.4ms   | 5.4ms   |
| IMAGE_565_4（Flash不够，未绘制）        | 23.8ms  | 7.4ms   | 5.3ms   | 5.4ms   |
| IMAGE_565_8（Flash不够，未绘制）        | 23.7ms  | 7.4ms   | 5.4ms   | 5.3ms   |
| IMAGE_RESIZE_565                        | 77.5ms  | 40.0ms  | 33.5ms  | 35.6ms  |
| IMAGE_RESIZE_565_1（Flash不够，未绘制） | 23.8ms  | 7.5ms   | 5.4ms   | 5.4ms   |
| IMAGE_RESIZE_565_2（Flash不够，未绘制） | 23.8ms  | 7.4ms   | 5.4ms   | 5.4ms   |
| IMAGE_RESIZE_565_4（Flash不够，未绘制） | 23.7ms  | 7.4ms   | 5.4ms   | 5.4ms   |
| IMAGE_RESIZE_565_8（Flash不够，未绘制） | 23.7ms  | 7.5ms   | 5.4ms   | 5.4ms   |
| TEXT                                    | 92.1ms  | 19.5ms  | 12.5ms  | 17.1ms  |
| TEXT_RECT                               | 135.0ms | 24.8ms  | 13.0ms  | 17.6ms  |
| RECTANGLE                               | 40.7ms  | 15.7ms  | 12.3ms  | 12.6ms  |
| CIRCLE                                  | 135.4ms | 93.8ms  | 84.8ms  | 89.1ms  |
| ARC                                     | 122.3ms | 51.0ms  | 36.3ms  | 42.4ms  |
| ROUND_RECTANGLE                         | 138.8ms | 96.0ms  | 86.7ms  | 91.2ms  |
| ROUND_RECTANGLE_CORNERS                 | 138.6ms | 93.5ms  | 84.0ms  | 88.4ms  |
| RECTANGLE_FILL                          | 37.7ms  | 18.0ms  | 15.1ms  | 15.6ms  |
| CIRCLE_FILL                             | 387.3ms | 63.5ms  | 23.5ms  | 25.6ms  |
| ARC_FILL                                | 212.5ms | 121.0ms | 108.4ms | 108.1ms |
| ROUND_RECTANGLE_FILL                    | 391.4ms | 66.5ms  | 26.0ms  | 28.4ms  |
| ROUND_RECTANGLE_CORNERS_FILL            | 393.8ms | 66.8ms  | 26.0ms  | 28.3ms  |



### 12*8性能测试数据

```
=========== Test Mode: LINE ===========
Refresh Screen Time: 104.8ms
=========== Test Mode: IMAGE_565 ===========
Refresh Screen Time: 49.1ms
=========== Test Mode: IMAGE_565_1 ===========
Refresh Screen Time: 23.8ms
=========== Test Mode: IMAGE_565_2 ===========
Refresh Screen Time: 23.8ms
=========== Test Mode: IMAGE_565_4 ===========
Refresh Screen Time: 23.8ms
=========== Test Mode: IMAGE_565_8 ===========
Refresh Screen Time: 23.7ms
=========== Test Mode: IMAGE_RESIZE_565 ===========
Refresh Screen Time: 77.5ms
=========== Test Mode: IMAGE_RESIZE_565_1 ===========
Refresh Screen Time: 23.8ms
=========== Test Mode: IMAGE_RESIZE_565_2 ===========
Refresh Screen Time: 23.8ms
=========== Test Mode: IMAGE_RESIZE_565_4 ===========
Refresh Screen Time: 23.7ms
=========== Test Mode: IMAGE_RESIZE_565_8 ===========
Refresh Screen Time: 23.7ms
=========== Test Mode: TEXT ===========
Refresh Screen Time: 92.1ms
=========== Test Mode: TEXT_RECT ===========
Refresh Screen Time: 135.0ms
=========== Test Mode: RECTANGLE ===========
Refresh Screen Time: 40.7ms
=========== Test Mode: CIRCLE ===========
Refresh Screen Time: 135.4ms
=========== Test Mode: ARC ===========
Refresh Screen Time: 122.3ms
=========== Test Mode: ROUND_RECTANGLE ===========
Refresh Screen Time: 138.8ms
=========== Test Mode: ROUND_RECTANGLE_CORNERS ===========
Refresh Screen Time: 138.6ms
=========== Test Mode: RECTANGLE_FILL ===========
Refresh Screen Time: 37.7ms
=========== Test Mode: CIRCLE_FILL ===========
Refresh Screen Time: 387.3ms
=========== Test Mode: ARC_FILL ===========
Refresh Screen Time: 212.5ms
=========== Test Mode: ROUND_RECTANGLE_FILL ===========
Refresh Screen Time: 391.4ms
=========== Test Mode: ROUND_RECTANGLE_CORNERS_FILL ===========
Refresh Screen Time: 393.8ms
```





### 24*32性能测试数据

```
=========== Test Mode: LINE ===========
Refresh Screen Time: 21.6ms
=========== Test Mode: IMAGE_565 ===========
Refresh Screen Time: 27.4ms
=========== Test Mode: IMAGE_565_1 ===========
Refresh Screen Time: 7.4ms
=========== Test Mode: IMAGE_565_2 ===========
Refresh Screen Time: 7.5ms
=========== Test Mode: IMAGE_565_4 ===========
Refresh Screen Time: 7.4ms
=========== Test Mode: IMAGE_565_8 ===========
Refresh Screen Time: 7.4ms
=========== Test Mode: IMAGE_RESIZE_565 ===========
Refresh Screen Time: 40.0ms
=========== Test Mode: IMAGE_RESIZE_565_1 ===========
Refresh Screen Time: 7.5ms
=========== Test Mode: IMAGE_RESIZE_565_2 ===========
Refresh Screen Time: 7.4ms
=========== Test Mode: IMAGE_RESIZE_565_4 ===========
Refresh Screen Time: 7.4ms
=========== Test Mode: IMAGE_RESIZE_565_8 ===========
Refresh Screen Time: 7.5ms
=========== Test Mode: TEXT ===========
Refresh Screen Time: 19.5ms
=========== Test Mode: TEXT_RECT ===========
Refresh Screen Time: 24.8ms
=========== Test Mode: RECTANGLE ===========
Refresh Screen Time: 15.7ms
=========== Test Mode: CIRCLE ===========
Refresh Screen Time: 93.8ms
=========== Test Mode: ARC ===========
Refresh Screen Time: 51.0ms
=========== Test Mode: ROUND_RECTANGLE ===========
Refresh Screen Time: 96.0ms
=========== Test Mode: ROUND_RECTANGLE_CORNERS ===========
Refresh Screen Time: 93.5ms
=========== Test Mode: RECTANGLE_FILL ===========
Refresh Screen Time: 18.0ms
=========== Test Mode: CIRCLE_FILL ===========
Refresh Screen Time: 63.5ms
=========== Test Mode: ARC_FILL ===========
Refresh Screen Time: 121.0ms
=========== Test Mode: ROUND_RECTANGLE_FILL ===========
Refresh Screen Time: 66.5ms
=========== Test Mode: ROUND_RECTANGLE_CORNERS_FILL ===========
Refresh Screen Time: 66.8ms
```





### 240*32性能测试数据

```
=========== Test Mode: LINE ===========
Refresh Screen Time: 10.7ms
=========== Test Mode: IMAGE_565 ===========
Refresh Screen Time: 23.6ms
=========== Test Mode: IMAGE_565_1 ===========
Refresh Screen Time: 5.4ms
=========== Test Mode: IMAGE_565_2 ===========
Refresh Screen Time: 5.4ms
=========== Test Mode: IMAGE_565_4 ===========
Refresh Screen Time: 5.3ms
=========== Test Mode: IMAGE_565_8 ===========
Refresh Screen Time: 5.4ms
=========== Test Mode: IMAGE_RESIZE_565 ===========
Refresh Screen Time: 33.5ms
=========== Test Mode: IMAGE_RESIZE_565_1 ===========
Refresh Screen Time: 5.4ms
=========== Test Mode: IMAGE_RESIZE_565_2 ===========
Refresh Screen Time: 5.4ms
=========== Test Mode: IMAGE_RESIZE_565_4 ===========
Refresh Screen Time: 5.4ms
=========== Test Mode: IMAGE_RESIZE_565_8 ===========
Refresh Screen Time: 5.4ms
=========== Test Mode: TEXT ===========
Refresh Screen Time: 12.5ms
=========== Test Mode: TEXT_RECT ===========
Refresh Screen Time: 13.0ms
=========== Test Mode: RECTANGLE ===========
Refresh Screen Time: 12.3ms
=========== Test Mode: CIRCLE ===========
Refresh Screen Time: 84.8ms
=========== Test Mode: ARC ===========
Refresh Screen Time: 36.3ms
=========== Test Mode: ROUND_RECTANGLE ===========
Refresh Screen Time: 86.7ms
=========== Test Mode: ROUND_RECTANGLE_CORNERS ===========
Refresh Screen Time: 84.0ms
=========== Test Mode: RECTANGLE_FILL ===========
Refresh Screen Time: 15.1ms
=========== Test Mode: CIRCLE_FILL ===========
Refresh Screen Time: 23.5ms
=========== Test Mode: ARC_FILL ===========
Refresh Screen Time: 108.4ms
=========== Test Mode: ROUND_RECTANGLE_FILL ===========
Refresh Screen Time: 26.0ms
=========== Test Mode: ROUND_RECTANGLE_CORNERS_FILL ===========
Refresh Screen Time: 26.0ms
```







### 24*320性能测试数据

```
=========== Test Mode: LINE ===========
Refresh Screen Time: 9.6ms
=========== Test Mode: IMAGE_565 ===========
Refresh Screen Time: 24.8ms
=========== Test Mode: IMAGE_565_1 ===========
Refresh Screen Time: 5.4ms
=========== Test Mode: IMAGE_565_2 ===========
Refresh Screen Time: 5.4ms
=========== Test Mode: IMAGE_565_4 ===========
Refresh Screen Time: 5.4ms
=========== Test Mode: IMAGE_565_8 ===========
Refresh Screen Time: 5.3ms
=========== Test Mode: IMAGE_RESIZE_565 ===========
Refresh Screen Time: 35.6ms
=========== Test Mode: IMAGE_RESIZE_565_1 ===========
Refresh Screen Time: 5.4ms
=========== Test Mode: IMAGE_RESIZE_565_2 ===========
Refresh Screen Time: 5.4ms
=========== Test Mode: IMAGE_RESIZE_565_4 ===========
Refresh Screen Time: 5.4ms
=========== Test Mode: IMAGE_RESIZE_565_8 ===========
Refresh Screen Time: 5.4ms
=========== Test Mode: TEXT ===========
Refresh Screen Time: 17.1ms
=========== Test Mode: TEXT_RECT ===========
Refresh Screen Time: 17.6ms
=========== Test Mode: RECTANGLE ===========
Refresh Screen Time: 12.6ms
=========== Test Mode: CIRCLE ===========
Refresh Screen Time: 89.1ms
=========== Test Mode: ARC ===========
Refresh Screen Time: 42.4ms
=========== Test Mode: ROUND_RECTANGLE ===========
Refresh Screen Time: 91.2ms
=========== Test Mode: ROUND_RECTANGLE_CORNERS ===========
Refresh Screen Time: 88.4ms
=========== Test Mode: RECTANGLE_FILL ===========
Refresh Screen Time: 15.6ms
=========== Test Mode: CIRCLE_FILL ===========
Refresh Screen Time: 25.6ms
=========== Test Mode: ARC_FILL ===========
Refresh Screen Time: 108.1ms
=========== Test Mode: ROUND_RECTANGLE_FILL ===========
Refresh Screen Time: 28.4ms
=========== Test Mode: ROUND_RECTANGLE_CORNERS_FILL ===========
Refresh Screen Time: 28.3ms
```





















