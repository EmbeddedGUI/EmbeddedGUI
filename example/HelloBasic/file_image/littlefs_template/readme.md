# littlefs_template

这个目录提供一个 `LittleFS` 文件 IO 接入模板，适合图片资源放在 `SPI NOR / QSPI NOR` 上，并通过 `LittleFS` 暴露成普通文件路径的场景。

设计意图：
- 模板仍然放在 `example/`，不进入 `src/`，保持核心层只负责统一接口和调度。
- 默认不参与 `HelloBasic/file_image` 的 PC 编译；当前 `Makefile` 只会编译 `file_image/` 顶层 `.c`，不会递归编译这个子目录。
- 图片 path 继续使用逻辑文件名，例如 `sample_overlay.png`；真正的挂载点前缀由 `root_prefix` 提供，例如 `/ui/`。

模板文件：
- `file_io_littlefs_template.c`
- `file_io_littlefs_template.h`

模板能力：
- `open` 对接 `lfs_file_open`
- `read` 对接 `lfs_file_read`
- `seek` 对接 `lfs_file_seek`
- `tell` 对接 `lfs_file_tell`
- `close` 对接 `lfs_file_close`

接入方式：
1. 把模板复制到你的目标 app 目录，或在自己的 `build.mk` 里显式把它加入编译。
2. 确保工程已经集成 `lfs.h`、块设备驱动和 `lfs_mount()` 初始化流程。
3. 在 app 里初始化一个 `file_image_littlefs_context_t`，按需设置 `lfs` 和 `root_prefix`。
4. 初始化 `egui_image_file_io_t`，再注册为默认 IO。

示例：
```c
static lfs_t g_lfs;
static file_image_littlefs_context_t g_file_ctx = {
        .lfs = &g_lfs,
        .root_prefix = "/ui/",
};
static egui_image_file_io_t g_file_io;

file_image_littlefs_io_init(&g_file_io, &g_file_ctx);
egui_image_file_set_default_io(&g_file_io);
```

路径约定：
- 如果 `root_prefix` 是 `/ui/`，而图片路径是 `sample.jpg`，最终会访问 `/ui/sample.jpg`。
- 如果你的 app 直接传完整路径，也可以把 `root_prefix` 设为 `NULL` 或空字符串。

落地建议：
- 如果你是 `SD/FatFs`，优先使用 `fatfs_template/`。
- 如果你是 `SPI NOR + LittleFS`，优先使用这个 `littlefs_template/`。
- 如果你没有文件系统，而是直接从资源包偏移读取，优先使用 `flash_map_template/`。
- decoder 仍然按芯片能力自行组合，例如 `BMP stream -> vendor JPEG -> TJpgDec -> stb_image`。
