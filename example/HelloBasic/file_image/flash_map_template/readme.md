# flash_map_template

这个目录提供一个“逻辑文件名 -> 外部 Flash 地址表”的文件 IO 接入模板，适合没有 `FatFs`、但图片资源已经预先打包到 `SPI NOR / QSPI / OSPI / XIP Flash` 的场景。

设计意图：
- 模板仍然放在 `example/`，不进入 `src/`，保持核心层只负责统一接口和调度。
- 默认不参与 `HelloBasic/file_image` 的 PC 编译；当前 `Makefile` 只会编译 `file_image/` 顶层 `.c`，不会递归编译这个子目录。
- 图片 path 继续使用逻辑文件名，例如 `sample_landscape.jpg`；真正的“文件定位”由地址表和底层 flash read 回调完成。

模板文件：
- `file_io_flash_map_template.c`
- `file_io_flash_map_template.h`

模板能力：
- `open` 通过静态地址表把逻辑路径映射到 `offset + size`
- `read` 通过用户提供的 `storage_read()` 回调从外部 Flash 读取
- `seek / tell / close` 在内存里维护一个轻量句柄，不依赖文件系统

接入方式：
1. 把模板复制到你的目标 app 目录，或在自己的 `build.mk` 里显式把它加入编译。
2. 准备一个资源清单表，把每个逻辑文件名映射到 Flash 中的偏移和长度。
3. 用板级驱动替换 `storage_read()`，可以是 `qspi_read()`、`ospi_read()`，也可以是 XIP 场景下的 `memcpy()`。
4. 初始化 `file_image_flash_map_context_t` 和 `egui_image_file_io_t`，再注册为默认 IO。

示例：
```c
static const file_image_flash_map_entry_t g_file_entries[] = {
        { "sample_landscape.jpg", 0x00000000u, 18342u },
        { "sample_overlay.png",   0x00004800u,  6120u },
        { "sample_badge.bmp",     0x00006000u,  5174u },
};

static int32_t board_flash_read(void *user_data, uint32_t address, void *buf, uint32_t size)
{
    board_qspi_t *qspi = (board_qspi_t *)user_data;

    return board_qspi_read(qspi, address, buf, size) == 0 ? (int32_t)size : -1;
}

static file_image_flash_map_context_t g_file_ctx = {
        .base_address = 0x90000000u,
        .entries = g_file_entries,
        .entry_count = sizeof(g_file_entries) / sizeof(g_file_entries[0]),
        .storage_user_data = &g_board_qspi,
        .storage_read = board_flash_read,
};
static egui_image_file_io_t g_file_io;

file_image_flash_map_io_init(&g_file_io, &g_file_ctx);
egui_image_file_set_default_io(&g_file_io);
```

XIP 场景建议：
- 如果外部 Flash 已经映射到内存地址空间，可以让 `storage_read()` 直接 `memcpy()`，这样 `read()` 不需要再走文件系统。
- `base_address` 可以直接写成 XIP 起始地址，`offset` 填资源包内偏移。

落地建议：
- 如果你是 `SD/FatFs`，优先使用 `fatfs_template/`。
- 如果你是“资源打包进外部 Flash、运行时按偏移读”，优先使用这个 `flash_map_template/`。
- decoder 仍然按芯片能力自行组合，例如 `BMP stream -> vendor JPEG -> TJpgDec -> stb_image`。
