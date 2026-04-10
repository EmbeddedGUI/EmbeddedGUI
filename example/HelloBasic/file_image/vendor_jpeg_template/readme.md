# vendor_jpeg_template

这个目录提供一个“芯片厂商 JPEG / 硬件 JPEG 外设”的 decoder 接入模板，用来说明怎么把各家 MCU/SoC 自带的 JPEG block 挂到 `egui_image_file`。

设计意图：
- 模板仍然放在 `example/`，不进入 `src/`，保持核心层只负责统一接口和调度。
- 默认不参与 `HelloBasic/file_image` 的 PC 编译；当前 `Makefile` 只会编译 `file_image/` 顶层 `.c`，不会递归编译这个子目录。
- 你可以直接把这个模板复制到自己的 app 目录顶层，或者在自己的 `build.mk` 里显式把它加入编译。

模板文件：
- `decoder_jpeg_vendor_template.c`
- `decoder_jpeg_vendor_template.h`

建议接入步骤：
1. 把模板复制到你的目标例程或产品 app 目录。
2. 用芯片 SDK 替换 `file_image_vendor_jpeg_prepare()`，完成文件绑定、JPEG 头解析、session 创建。
3. 用芯片 SDK 替换 `file_image_vendor_jpeg_decode_band()`，把硬件每次输出的一段像素写成连续的 `RGB565` 行缓存。
4. 在 `file_image_vendor_jpeg_release()` 中释放 session / DMA / 中间 buffer。
5. 注册顺序放在 `TJpgDec` 前面，例如：

```c
egui_image_file_register_decoder(&g_file_image_bmp_stream_decoder);
egui_image_file_register_decoder(&g_file_image_jpeg_vendor_template_decoder);
egui_image_file_register_decoder(&g_file_image_tjpgd_stream_decoder);
egui_image_file_register_decoder(&g_file_image_stb_decoder);
```

接入建议：
- 如果厂商 JPEG block 需要文件句柄持续有效，保持 `out_info->keep_file_open = 1`。
- 如果厂商 SDK 会自己重新打开路径，或者在 `open` 阶段已经把输入复制完，也可以改成 `0`，让 `egui_image_file` 提前关闭文件。
- 如果硬件输出的是 `RGB888 / YUV`，就在 `file_image_vendor_jpeg_decode_band()` 里转换成 `RGB565` 再写入 `dst_rgb565`。
- 如果厂商硬件只能整图解码，也可以把整图输出暂存到外部 RAM，再在 `read_row()` 里按行拷贝；接口不需要变。

这个模板的重点不是提供一套可直接运行的统一 JPEG 驱动，而是给出“文件 IO 抽象 + decoder 注册 + 按带缓存”的最小接线骨架，方便按芯片特性改写。
