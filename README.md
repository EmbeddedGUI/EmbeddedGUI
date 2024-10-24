# 简介

[![Compile Check](https://github.com/EmbeddedGUI/EmbeddedGUI/actions/workflows/github-actions-demo.yml/badge.svg)](https://github.com/bobwenstudy/zephyr_polling/actions/workflows/github-actions-demo.yml) [![Documentation Status](https://readthedocs.org/projects/embeddedgui/badge/?version=latest)](https://embeddedgui.readthedocs.io/en/latest/?badge=latest)

文档地址：[欢迎来到EmbeddedGUI的文档](https://embeddedgui.readthedocs.io/en/latest/)

本项目主要面对RAM资源有限（<8KB），ROM资源有限（<64KB，主要看所需字体和贴图资源），CPU资源还充裕（<100MHz，不支持浮点，FPS在30左右）。需要支持触控、ViewPage等主流的UI控制行为。Framebuffer采用PFB设计，用户可以根据需要选择不同尺寸的PFB大小来平衡屏幕刷新率。

本项目提供一套基于PFB设计的GUI架构。基于面向对象的编码方式，UI参考Android UI架构，用户可以轻松定义项目所需的控件。

本项目主要参考：[GuiLite](https://gitee.com/idea4good/GuiLite)、[Arm-2D](https://github.com/ARM-software/Arm-2D)、[EasyGUI](https://github.com/MaJerle/EasyGUI)、[lvgl](https://github.com/lvgl/lvgl)和Android GUI框架。

本项目仓库路径：[EmbeddedGUI(gitee.com)](https://gitee.com/embeddedgui/EmbeddedGUI)，[EmbeddedGUI(github.com)](https://github.com/EmbeddedGUI/EmbeddedGUI)。



# 产品特点

- 易于移植，全部由C代码编写，支持C++调用，无第三方依赖库。
- 基于轮询结构，可以在任何MCU环境下运行，无需OS支持。
- 支持多种显示支持，RGB8、RGB565、RGB32。
- 基于MIT协议，随便使用。
- 只需要不到4KB RAM（包含Framebuffer）和64KB CODE，针对PFB有特别优化，小PFB和大PFB性能差异不大。
- UTF-8字体支持。
- 图片透明通道支持。
- Mask支持，可以绘制圆角图片等功能。
- 动画支持，支持Android的全部动画效果。
- 抗锯齿支持，基本图形，线，圆，圆环，圆角矩阵，扇形等都支持抗锯齿。
- 脏矩阵支持，平时只绘制需要绘制的区域，不仅省功耗，同时对于特定页面，可以实现高帧率。
- 定点支持，所有代码全部用定点运算实现，避免在没有浮点运算单元的芯片上，运行太卡。
- PFB支持，只需要简单一点点RAM。
- 双缓存支持，可以充分利用SPI写入屏幕时间间隙。
- PC调试，C部署，可以在PC上运行调试，而后在嵌入式项目上运行。
- Makefile组织编译，没有乱七八糟的配置。



# 例程演示

有点丑，但是核心机制已经演示出来了，剩下基于这个框架加自己东西就行。

<table>
  <tr>
    <td align="center"><img src="https://markdown-1306347444.cos.ap-shanghai.myqcloud.com/img/bbgq1-zg4av.gif" width="200px;"/><br /><sub><b>HelloSimple</b></sub></a>
    <td align="center"><img src="https://markdown-1306347444.cos.ap-shanghai.myqcloud.com/img/7cd1j-vnj2z.gif" width="200px;"/><br /><sub><b>HelloViewPageAndScroll</b></sub></a>
    <td align="center"><img src="https://markdown-1306347444.cos.ap-shanghai.myqcloud.com/img/ty6sb-byumu.gif" width="200px;"/><br /><sub><b>HelloActivity</b></sub></a>
    <td align="center"><img src="https://markdown-1306347444.cos.ap-shanghai.myqcloud.com/img/4riwf-3j8ki.gif" width="200px;"/><br /><sub><b>HelloTest</b></sub></a>
  </tr>
  <tr>
    <td align="center"><img src="https://markdown-1306347444.cos.ap-shanghai.myqcloud.com/img/20240831_140522_anim.gif" width="200px;"/><br /><sub><b>HelloBasic(anim)</b></sub></a>
    <td align="center"><img src="https://markdown-1306347444.cos.ap-shanghai.myqcloud.com/img/20240831_163738_button.gif" width="200px;"/><br /><sub><b>HelloBasic(button)</b></sub></a>
    <td align="center"><img src="https://markdown-1306347444.cos.ap-shanghai.myqcloud.com/img/20240831_163834_button_img.gif" width="200px;"/><br /><sub><b>HelloBasic(button_img)</b></sub></a>
    <td align="center"><img src="https://markdown-1306347444.cos.ap-shanghai.myqcloud.com/img/20240831_163916_image.gif" width="200px;"/><br /><sub><b>HelloBasic(image)</b></sub></a>
  </tr>
  <tr>
    <td align="center"><img src="https://markdown-1306347444.cos.ap-shanghai.myqcloud.com/img/20240831_163934_label.gif" width="200px;"/><br /><sub><b>HelloBasic(label)</b></sub></a>
    <td align="center"><img src="https://markdown-1306347444.cos.ap-shanghai.myqcloud.com/img/20240831_163953_linearlayout.gif" width="200px;"/><br /><sub><b>HelloBasic(linearlayout)</b></sub></a>
    <td align="center"><img src="https://markdown-1306347444.cos.ap-shanghai.myqcloud.com/img/20240831_164042_mask.gif" width="200px;"/><br /><sub><b>HelloBasic(mask)</b></sub></a>
    <td align="center"><img src="https://markdown-1306347444.cos.ap-shanghai.myqcloud.com/img/20240831_164122_scroll.gif" width="200px;"/><br /><sub><b>HelloBasic(scroll)</b></sub></a>
  </tr>
  <tr>
    <td align="center"><img src="https://markdown-1306347444.cos.ap-shanghai.myqcloud.com/img/20240831_164159_switch.gif" width="200px;"/><br /><sub><b>HelloBasic(switch)</b></sub></a>
    <td align="center"><img src="https://markdown-1306347444.cos.ap-shanghai.myqcloud.com/img/20240831_164234_viewpage.gif" width="200px;"/><br /><sub><b>HelloBasic(viewpage)</b></sub></a>
    <td align="center"><img src="https://markdown-1306347444.cos.ap-shanghai.myqcloud.com/img/20240831_164411_progress_bar.gif" width="200px;"/><br /><sub><b>HelloBasic(progress_bar)</b></sub></a>
    <td align="center"><img src="" width="200px;"/><br /><sub><b></b></sub></a>
  </tr>
</table>








# 需求说明

作为芯片从业人员，国产芯片普遍资源有限（ROM和RAM比较少-都是成本，CPU速度比较高-100MHz），需要在512KB ROM，20KB左右RAM资源上实现手环之类的GUI操作（要有触摸），CPU可以跑96MHz。

第一次搞嵌入式GUI，问了一圈朋友，LVGL直接放弃（太绚丽了，个人觉得也不可能跑得动，而且代码应该也比较复杂，魔改会比较困难），有人建议`手撸`，那要死人了。

有朋友推荐了[GuiLite](https://github.com/idea4good/GuiLite)，看了下介绍，GUI简单直接，所需的ROM和RAM也比较少，效果图里面也有很多所需的场景，持续有更新，[ Apache-2.0 license](https://github.com/idea4good/egui#Apache-2.0-1-ov-file)，比较符合我的需求。但是实际看了下，Framebuffer设计所需资源太多了，并没有PFB设计，多个Surface需要多个Framebuffer（跟着屏幕大小来的）；此外自定义控件需要考虑自己清除像素，涉及到透明度和滑动等业务场景时，就非常痛苦了。当然项目也有优势，用户完全掌握UI操作代码（4000行代码，可以轻松看懂）后，基本就不会有多余的MIPS浪费了。

又有朋友推荐了[Arm-2D](https://github.com/ARM-software/Arm-2D)，看了下介绍，很有吸引力，支持PFB，小资源芯片就可以跑高帧率，大公司靠谱。实际一看，各种宏定义，由于设计考虑的是卖芯片，所以基本不会去实现GUI的控件管理和触摸管理，脏矩阵还得自己去定义，不要太麻烦。里面实现了很多酷炫的效果，有很好的借鉴意义。

[lvgl](https://github.com/lvgl/lvgl)，很成熟的一个架构了，玩家也很多，看了下最新的代码v9.1.0，找了一会没找到底层canvas实现，看来是我能力太弱了，效果很酷炫，公司芯片基本没有能跑动的可能，想了下，还是放弃。

综上所述，还是自己写一套吧，什么都可控，酷炫的效果无法实现，那就贴图好了。



# 所需资源分析

资源分GUI代码和控件所需的资源以及Framebuffer。PFB用户根据需要自己定义。

## GUI代码和控件所需资源

对于嵌入式环境而言，code size和ram size至关重要。所以以典型的cm0嵌入式开发环境为例，对code size和ram size进行分析。编译出来的大小见下表。

可以看到不同的例程所需资源差异巨大，这个涉及到GUI用到了哪些控件，字库，图片等。

注意：由于不同lib库对于printf、malloc等接口影响较大，库这些接口都不实现。资源紧张的场景可以按需简易实现。

注意：直接在根目录运行`python .\scripts\utils_analysis_elf_size.py`脚本，可以打印下面的表格保存在`output\README.md`。

| app                      | Code(Bytes) | Resource(Bytes) | RAM(Bytes) | PFB(Bytes) |
| ------------------------ | ----------- | --------------- | ---------- | ---------- |
| HelloActivity            | 20464       | 9968            | 1384       | 1536       |
| HelloBasic(anim)         | 13500       | 3500            | 656        | 2400       |
| HelloBasic(button)       | 15124       | 9132            | 648        | 2400       |
| HelloBasic(button_img)   | 17852       | 27840           | 624        | 2400       |
| HelloBasic(image)        | 17748       | 7756            | 624        | 2400       |
| HelloBasic(label)        | 7584        | 5728            | 624        | 2400       |
| HelloBasic(linearlayout) | 15188       | 9140            | 768        | 2400       |
| HelloBasic(mask)         | 19460       | 31136           | 720        | 2400       |
| HelloBasic(progress_bar) | 8512        | 3472            | 624        | 2400       |
| HelloBasic(scroll)       | 17572       | 9248            | 936        | 2400       |
| HelloBasic(switch)       | 8116        | 3468            | 632        | 2400       |
| HelloBasic(viewpage)     | 17708       | 9248            | 936        | 2400       |
| HelloPerformace          | 35172       | 443544          | 640        | 1536       |
| HelloSimple              | 15360       | 6620            | 800        | 1536       |
| HelloTest                | 32616       | 56224           | 1536       | 1536       |
| HelloViewPageAndScroll   | 20024       | 15552           | 1760       | 1536       |

可以看到，项目的Code基本上远小于Resource，变量RAM也远小于PFB所需的RAM空间。

以`HelloSimple`为例，实现1个button+1个label只需要`15360`的code size和`800`字节的ram size，资源占用`6620`Bytes，PFB占用`1536`Bytes。

![bbgq1-zg4av](https://markdown-1306347444.cos.ap-shanghai.myqcloud.com/img/bbgq1-zg4av.gif)





## Framebuffer

GUI设计之初就支持PFB（Partial Frame-buffer），唯一的要求是PFB的Width和Height是屏幕尺寸的整数倍。如屏幕尺寸是`240*320`，那PFB尺寸为`24*32`，RGB565的屏幕，所需的RAM为：1536Bytes，可以说很小了。








# 代码架构

没什么东西，也就是源代码，例程和编译配置。

- **example**：各种GUI例程。
- **porting**：程序的主入口，根据平台不同，有一些不同实现。pc支持windows、linux和macos。嵌入式支持stm32g0。
- **src**：EmbeddedGUI代码实现部分。
- **build.mk和Makefile**：Makefile文件。

```
EmbeddedGUI
 ├── build.mk
 ├── Makefile
 ├── example
 │   ...
 ├── porting
 │   ...
 └── src
     ...
```





# 使用说明

## 环境搭建-Windows

Windows编译，最终生成exe，可以直接在PC上跑。

目前需要安装如下环境：

- GCC环境，笔者用的msys64+mingw，用于编译生成exe，参考这个文章安装即可。[Win7下msys64安装mingw工具链 - Milton - 博客园 (cnblogs.com)](https://www.cnblogs.com/milton/p/11808091.html)。

## 环境搭建-Linux/Mac

参考[ARM-software/Arm-2D: 2D Graphic Library optimized for Cortex-M processors (github.com)](https://github.com/ARM-software/Arm-2D)搭建MAC环境。



## 编译说明

本项目都是由makefile组织编译的，编译整个项目只需要执行`make all`即可，调用`make run`可以运行。

根据具体需要可以调整一些参数，目前Makefile支持如下参数配置。

- **APP**：选择example中的例程，默认选择为`HelloSimple`。
- **PORT**：选择porting中的环境，也就是当前平台，默认选择为`pc`，stm32g0需要专门的开发板环境，平时可以用于评估code size和ram size。

也就是可以通过如下指令来编译工程：

```shell
make all APP=HelloSimple
```

执行`make run`后，在PC环境就会弹出一个窗口，演示GUI效果了。





# 社区交流

欢迎大家入群交流讨论。

<table>
  <tr>
    <td align="center"><img src="https://markdown-1306347444.cos.ap-shanghai.myqcloud.com/img/23901725079354_.pic.jpg" width="300px;" height="400px"/><br /><sub><b>QQ</b></sub></a>
  </tr>
</table>































