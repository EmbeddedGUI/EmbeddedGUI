# 介绍

EmbeddedGUI是专为嵌入式打造的GUI框架，项目支持PFB、脏矩阵、动画和抗锯齿支持。

本项目仓库路径：[EmbeddedGUI(gitee.com)](https://gitee.com/embeddedgui/EmbeddedGUI)，[EmbeddedGUI(github.com)](https://github.com/EmbeddedGUI/EmbeddedGUI)。



# 功能特点


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

# License

遵循`MIT`协议。





