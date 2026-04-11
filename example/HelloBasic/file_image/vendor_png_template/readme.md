# vendor_png_template

这个目录提供一个“芯片厂商 PNG 库 / 第三方 PNG 库”的 decoder 接入模板，用来说明怎么把各家 MCU/SoC 侧的 PNG 解码能力挂到 `egui_image_file`。

设计意图：
- 模板仍然放在 `example/`，不进入 `src/`，保持核心层只负责统一接口和调度。
- 默认不参与 `HelloBasic/file_image` 的 PC 编译；当前 `Makefile` 只会编译 `file_image/` 顶层 `.c`，不会递归编译这个子目录。
- 模板假设 PNG 侧更常见的是“整图解码到 RAM / PSRAM / 外部 SRAM”，然后按行回传给 `egui_image_file`。

模板文件：
- `decoder_png_vendor_template.c`
- `decoder_png_vendor_template.h`

建议接入步骤：
1. 把模板复制到你的目标例程或产品 app 目录。
2. 用芯片 SDK 或第三方库替换 `file_image_vendor_png_prepare()`，完成路径/文件绑定、PNG 头解析和 session 创建。
3. 用真实库替换 `file_image_vendor_png_decode()`，把整张图转换成 `RGB565`，如果有透明度再同时输出 `alpha8`。
4. 在 `file_image_vendor_png_release()` 里释放 session 和中间 buffer。
5. 注册顺序放在 `stb_image` 前面，例如：

```c
egui_image_file_register_decoder(&g_file_image_bmp_stream_decoder);
egui_image_file_register_decoder(&g_file_image_jpeg_vendor_template_decoder);
egui_image_file_register_decoder(&g_file_image_tjpgd_stream_decoder);
egui_image_file_register_decoder(&g_file_image_png_vendor_template_decoder);
egui_image_file_register_decoder(&g_file_image_stb_decoder);
```

接入建议：
- 如果你的 PNG 库直接输出 `RGBA8888`，就在 `file_image_vendor_png_decode()` 里转成 `RGB565 + alpha8`。
- 如果图片资源都无透明度，也可以把 `has_alpha` 固定成 `0`，省掉 `alpha` buffer。
- 如果整图解码 RAM 压力太大，可以把这份模板改造成“逐行输出”或“逐块输出”，接口层不需要改变。

这个模板的重点不是提供一套统一 PNG 驱动，而是给出“文件 IO 抽象 + decoder 注册 + 整图缓冲”的最小接线骨架，方便按芯片能力落地。
