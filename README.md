# EmbeddedGUI

[![Compile Check](https://github.com/EmbeddedGUI/EmbeddedGUI/actions/workflows/github-actions-demo.yml/badge.svg)](https://github.com/EmbeddedGUI/EmbeddedGUI/actions/workflows/github-actions-demo.yml) [![Documentation Status](https://readthedocs.org/projects/embeddedgui/badge/?version=latest)](https://embeddedgui.readthedocs.io/en/latest/?badge=latest)

面向资源受限嵌入式系统的轻量级 C 语言 GUI 框架（RAM <8KB，ROM <64KB，CPU ~100MHz，无需浮点运算单元）。



## 核心特点

| 特点 | 说明 |
|------|------|
| 超轻量 | <4KB RAM（含 PFB）+ <64KB ROM，适合资源受限 MCU |
| PFB 设计 | 局部帧缓冲，小缓冲区即可实现高帧率 |
| 纯 C 实现 | 无第三方依赖，支持 C++ 调用，MIT 许可证 |
| 抗锯齿 | 圆、圆角矩形、弧形、线条等基本图形均支持抗锯齿 |
| 动画系统 | 9 种插值器，类 Android 动画 API |
| 60+ 控件 | 从基础按钮到图表、时钟、仪表盘等专业控件 |
| 脏矩形 | 只重绘变化区域，降低功耗，提高特定页面帧率 |
| 定点运算 | 全部使用定点数，无需浮点运算单元 |



## 效果展示

<!-- TODO: 添加 HelloStyleDemo 截图 -->

- [在线体验](https://embeddedgui.github.io/EmbeddedGUI/)
- [完整文档](https://embeddedgui.readthedocs.io/en/latest/)



## 快速开始

```bash
git clone https://gitee.com/embeddedgui/EmbeddedGUI.git
cd EmbeddedGUI && setup.bat
make all APP=HelloStyleDemo && make run
```

> Linux/Mac 用户请参考文档的[环境搭建](https://embeddedgui.readthedocs.io/en/latest/)章节。



## 资源占用

| 示例 | Code(Bytes) | Resource(Bytes) | RAM(Bytes) | PFB(Bytes) |
|------|-------------|-----------------|------------|------------|
| HelloSimple | 15360 | 6620 | 800 | 1536 |
| HelloStyleDemo | -- | -- | -- | -- |

> 完整资源分析请参考[文档](https://embeddedgui.readthedocs.io/en/latest/)。运行 `python scripts/utils_analysis_elf_size.py` 可在本地生成详细报告。



## 文档导航

[快速入门](https://embeddedgui.readthedocs.io/en/latest/) | [架构原理](https://embeddedgui.readthedocs.io/en/latest/) | [控件参考](https://embeddedgui.readthedocs.io/en/latest/) | [动画系统](https://embeddedgui.readthedocs.io/en/latest/) | [资源管理](https://embeddedgui.readthedocs.io/en/latest/) | [性能测试](https://embeddedgui.readthedocs.io/en/latest/) | [移植指南](https://embeddedgui.readthedocs.io/en/latest/) | [应用开发](https://embeddedgui.readthedocs.io/en/latest/) | [UI Designer](https://embeddedgui.readthedocs.io/en/latest/)



## 仓库地址

- Gitee: https://gitee.com/embeddedgui/EmbeddedGUI
- GitHub: https://github.com/EmbeddedGUI/EmbeddedGUI



## 社区交流

欢迎大家入群交流讨论。

<table>
  <tr>
    <td align="center"><img src="https://markdown-1306347444.cos.ap-shanghai.myqcloud.com/img/23901725079354_.pic.jpg" width="300px;" height="400px"/><br /><sub><b>QQ</b></sub></a>
  </tr>
</table>
