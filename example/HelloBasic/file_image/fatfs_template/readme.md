# fatfs_template

这个目录提供一个 `FATFS/SD` 文件 IO 接入模板，用来说明怎么把 `egui_image_file_io_t` 挂到 `FatFs`。

设计意图：
- 模板仍然放在 `example/`，不进入 `src/`。
- 默认不参与 `HelloBasic/file_image` 的 PC 编译；当前 `Makefile` 只会编译 `file_image/` 顶层 `.c`，不会递归编译这个子目录。
- 你可以直接把模板复制到自己的 app 目录顶层，或者在自己的 `build.mk` 里显式把它加入编译。

模板文件：
- `file_io_fatfs_template.c`
- `file_io_fatfs_template.h`

模板能力：
- `open` 对接 `f_open`
- `read` 对接 `f_read`
- `seek` 对接 `f_lseek`
- `tell` 对接 `f_tell`
- `close` 对接 `f_close`

接入方式：
1. 把模板复制到你的目标 app 目录。
2. 确保工程已经集成 `ff.h` 和底层 SD/FATFS 驱动。
3. 在 app 里初始化一个 `file_image_fatfs_context_t`，按需设置 `root_prefix`，例如 `0:/ui/`。
4. 初始化 `egui_image_file_io_t`，再注册为默认 IO。

示例：

```c
static file_image_fatfs_context_t g_file_ctx = {
        .root_prefix = "0:/ui/",
};
static egui_image_file_io_t g_file_io;

file_image_fatfs_io_init(&g_file_io, &g_file_ctx);
egui_image_file_set_default_io(&g_file_io);
```

路径约定：
- 如果 `root_prefix` 是 `0:/ui/`，而图片路径是 `sample.jpg`，最终会访问 `0:/ui/sample.jpg`。
- 如果你的 app 直接传完整路径，也可以把 `root_prefix` 设为 `NULL` 或空字符串。

落地建议：
- 在 MCU 上把当前示例里的 `stdio` root-prefix IO 初始化替换成这个 `FatFs` 模板。
- decoder 仍然按芯片能力自行组合，例如 `BMP stream -> vendor JPEG -> TJpgDec -> stb_image`。
- 如果需要缓存目录、做路径映射或多分区切换，可以在 `file_image_fatfs_context_t` 上继续扩展。
