# file_image

这个例程演示运行时从外部文件路径加载图片并显示，覆盖 `jpg / png / bmp` 三种常见格式。

设计边界：

- 文件图片能力在 `src/` 只提供统一接口、文件 IO 协议、解码器注册和绘制逻辑。
- 具体解码器放在例程中，这里使用 `stb_image` 做 PC 示例。
- BMP 额外提供了一个流式 decoder 示例，演示 MCU 场景下如何只保留文件句柄并按行读取。
- 例程里通过 `stdio` 模拟 SD 卡/文件系统访问，后续 MCU 可以替换成 FATFS、SPI Flash 或芯片厂商解码模块。

当前界面展示：

- JPG 正常显示
- PNG 透明图显示
- BMP 通过流式 decoder 正常显示
- 同一张 JPG 的缩放显示
- 缺失路径时显示占位图，并通过状态标签暴露失败原因

与 LVGL 的做法对比：

- LVGL 也是把“文件系统接入”和“图片解码”拆开，文件路径先经过 `lv_fs_drv` 抽象层，再交给 image decoder。
- LVGL 的自定义 decoder 接口是 `info / open / get_area / close` 四段式，支持一次性整图解码，也支持按区域或按行取像素。
- LVGL 现有的 BMP / TJPGD / LodePNG 等格式支持，通常作为可选库集成在 LVGL 主仓库中，打开配置宏后自动注册 decoder。
- 其中 BMP 和 TJPGD 更偏低 RAM 路线，只按需读取或按块解码，但通常不支持缩放/旋转；LodePNG 则偏简单方案，会把整张 PNG 解到内存。

本仓库当前选择：

- 参考 LVGL 的分层思路，但不把具体格式 decoder 固化到 `src/`。
- `src/` 只保留文件图片对象、IO 协议、decoder 注册口和统一绘制分发。
- `example/` 或应用侧按芯片能力接入 `stb_image`、流式 BMP、硬件 JPEG、厂商 PNG、FATFS/SD 卡等实现。
- 后续如果某个平台需要更低 RAM 的 JPG/PNG 方案，可以继续按 LVGL 的 `按块/按行解码` 思路实现 decoder，而不用改核心接口。
