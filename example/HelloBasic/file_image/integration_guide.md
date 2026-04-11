# integration_guide

这份文档把 `file_image/` 目录里的零散模板串成一条可落地的 MCU 接线方案，重点回答三个问题：

- 外部图片路径怎么进 `egui_image_file`
- 多种存储后端怎么合并成一个默认 IO
- `BMP / JPEG / PNG` decoder 应该按什么顺序注册

## 1. 推荐分层

推荐保持下面这条边界，不要把芯片相关实现塞进 `src/`：

- `src/image/egui_image_file.*`
  只保留统一图片对象、文件 IO 抽象、decoder 注册与逐行绘制调度
- `example/` 或产品 app
  放文件系统适配、外部 Flash 映射、硬件 JPEG/PNG decoder、挂载路由

推荐的 app 初始化顺序：

1. 先初始化底层存储或文件系统
2. 把存储后端包装成 `egui_image_file_io_t`
3. 如果有多个后端，再用 `mount_router_template/` 合成一个总入口
4. 调用 `egui_image_file_set_default_io()`
5. 用 `decoder_registry_apply()` 注册 decoder 顺序
6. 业务侧只保留逻辑路径，例如 `photo/cat.jpg` 或 `sd:photo/cat.jpg`

## 2. 单一存储后端：FatFs / SD

如果产品只有一个 `FatFs/SD` 挂载点，最简单的接法就是：

```c
#include "decoder_bmp_stream.h"
#include "decoder_registry.h"
#include "decoder_tjpgd_stream.h"
#include "fatfs_template/file_io_fatfs_template.h"
#include "vendor_jpeg_template/decoder_jpeg_vendor_template.h"
#include "vendor_png_template/decoder_png_vendor_template.h"

static file_image_fatfs_context_t g_sd_ctx = {
        .root_prefix = "0:/ui/",
};
static egui_image_file_io_t g_sd_io;

static const file_image_decoder_registry_config_t g_decoder_cfg = {
        .bmp_stream = &g_file_image_bmp_stream_decoder,
        .jpeg_vendor = &g_file_image_jpeg_vendor_template_decoder,
        .jpeg_stream = &g_file_image_tjpgd_stream_decoder,
        .png_vendor = &g_file_image_png_vendor_template_decoder,
        .generic_fallback = NULL,
        .clear_first = 1,
};

void app_file_image_stack_init(void)
{
    file_image_fatfs_io_init(&g_sd_io, &g_sd_ctx);
    egui_image_file_set_default_io(&g_sd_io);
    file_image_decoder_registry_apply(&g_decoder_cfg);
}
```

说明：

- `root_prefix = "0:/ui/"` 时，业务路径 `photo/cat.jpg` 最终会访问 `0:/ui/photo/cat.jpg`
- `generic_fallback` 在 MCU 上通常可以先留空，避免引入一个高 RAM 的整图 decoder
- 如果芯片没有硬件 JPEG / PNG，只保留 `bmp_stream + jpeg_stream` 也可以

`LittleFS` 的接法完全类似，只是把 `file_image_fatfs_context_t` 与 `file_image_fatfs_io_init()` 换成 `littlefs_template/` 里的对应接口。

## 3. 多存储后端：SD + LittleFS + Flash

如果同一个 app 里同时要读：

- `SD/FatFs` 上的大图
- `LittleFS` 上的小图标
- `QSPI/OSPI Flash` 上的开机图

推荐把它们先各自包装成 `egui_image_file_io_t`，再交给 `mount_router_template/` 做统一路由：

```c
#include "decoder_bmp_stream.h"
#include "decoder_registry.h"
#include "decoder_tjpgd_stream.h"
#include "fatfs_template/file_io_fatfs_template.h"
#include "flash_map_template/file_io_flash_map_template.h"
#include "littlefs_template/file_io_littlefs_template.h"
#include "mount_router_template/file_io_mount_router_template.h"
#include "vendor_jpeg_template/decoder_jpeg_vendor_template.h"
#include "vendor_png_template/decoder_png_vendor_template.h"

static file_image_fatfs_context_t g_sd_ctx = {
        .root_prefix = "0:/ui/",
};
static file_image_littlefs_context_t g_lfs_ctx = {
        .lfs = &g_app_lfs,
        .root_prefix = "/ui/",
};
static const file_image_flash_map_entry_t g_flash_entries[] = {
        { "boot/logo.bmp", 0x00000000u, 4096u },
        { "boot/bg.jpg",   0x00002000u, 32768u },
};
static file_image_flash_map_context_t g_flash_ctx = {
        .base_address = 0x90000000u,
        .entries = g_flash_entries,
        .entry_count = sizeof(g_flash_entries) / sizeof(g_flash_entries[0]),
        .storage_user_data = &g_board_qspi,
        .storage_read = board_flash_read,
};

static egui_image_file_io_t g_sd_io;
static egui_image_file_io_t g_lfs_io;
static egui_image_file_io_t g_flash_io;
static egui_image_file_io_t g_router_io;

static const file_image_mount_router_entry_t g_mounts[] = {
        { "sd:",    &g_sd_io,    1 },
        { "lfs:",   &g_lfs_io,   1 },
        { "flash:", &g_flash_io, 1 },
};
static file_image_mount_router_context_t g_router_ctx = {
        .entries = g_mounts,
        .entry_count = sizeof(g_mounts) / sizeof(g_mounts[0]),
        .fallback_io = &g_lfs_io,
};

static const file_image_decoder_registry_config_t g_decoder_cfg = {
        .bmp_stream = &g_file_image_bmp_stream_decoder,
        .jpeg_vendor = &g_file_image_jpeg_vendor_template_decoder,
        .jpeg_stream = &g_file_image_tjpgd_stream_decoder,
        .png_vendor = &g_file_image_png_vendor_template_decoder,
        .generic_fallback = NULL,
        .clear_first = 1,
};

void app_file_image_stack_init(void)
{
    file_image_fatfs_io_init(&g_sd_io, &g_sd_ctx);
    file_image_littlefs_io_init(&g_lfs_io, &g_lfs_ctx);
    file_image_flash_map_io_init(&g_flash_io, &g_flash_ctx);
    file_image_mount_router_io_init(&g_router_io, &g_router_ctx);

    egui_image_file_set_default_io(&g_router_io);
    file_image_decoder_registry_apply(&g_decoder_cfg);
}
```

这样业务层就可以直接写：

- `sd:album/cat.jpg`
- `lfs:icon/battery.png`
- `flash:boot/logo.bmp`

如果某张图没有前缀，例子里会回退到 `fallback_io = &g_lfs_io`。

当前 `HelloBasic/file_image` 的 PC 例程已经按这个思路接线，只是为了避免额外维护三套 PC 文件目录，示例里把 `sd:` / `lfs:` / `flash:` 都路由到了不同的 `stdio` IO，再共同指向同一个 `files/` 目录。迁到 MCU 时，把这三个 IO 分别替换成真后端即可。

## 4. Decoder 顺序建议

当前推荐顺序已经固定在 `decoder_registry_apply()` 里：

`BMP stream -> vendor JPEG -> TJpgDec -> vendor PNG -> generic fallback`

原因如下：

- `BMP stream`
  BMP 最适合按行读，优先级最高，避免被整图 decoder 截走
- `vendor JPEG`
  芯片硬件 JPEG 或厂商库通常最贴近量产形态，应该先尝试
- `TJpgDec`
  作为通用低 RAM JPEG 兜底，适合没有 vendor JPEG 的 MCU
- `vendor PNG`
  让 MCU 侧定制 PNG 先命中，再决定是否回退到通用实现
- `generic fallback`
  只在 PC 或高 RAM 平台上保留，例如 `stb_image`

如果你的产品只需要 `BMP + JPEG`，可以把 `png_vendor` 和 `generic_fallback` 留空。

## 5. 路径约定建议

建议优先使用逻辑路径，而不是把板级路径细节散到每个 view 里。

推荐：

- 单后端产品：`photo/cat.jpg`
- 多后端产品：`sd:photo/cat.jpg`、`flash:boot/logo.bmp`

不推荐：

- 在 UI 代码里直接写 `0:/ui/photo/cat.jpg`
- 在 UI 代码里直接写物理 flash 地址或分区偏移

这样后续从 `FatFs` 切到 `LittleFS`、或者从 `SD` 切到 `FlashMap` 时，只需要换 app 初始化，不需要改界面层。

## 6. 常见坑

- 重复初始化但没清 decoder 列表
  如果 app 会多次进入初始化流程，把 `clear_first` 设成 `1`
- 把 vendor PNG 注册在 `stb_image` 后面
  会导致 MCU 自己的 PNG decoder 永远命不中
- 把 `generic_fallback` 带到低 RAM 量产版本
  容易在大 JPG/PNG 上引入不可控的整图 RAM 占用
- 多后端产品却没有前缀路由
  最终所有路径都落到同一个默认 IO，调试时很难看出问题
- 把文件系统根路径写死在业务 view
  后面一换存储拓扑就要全局改路径

## 7. 推荐抄写顺序

如果你要把这套能力搬到产品 app，建议按这个顺序拷贝：

1. 先拷 `decoder_registry.c/.h`
2. 按存储类型拷一个或多个 `*_template/file_io_*.c/.h`
3. 如果有多挂载点，再拷 `mount_router_template/`
4. 按芯片能力拷 `vendor_jpeg_template/` 或 `vendor_png_template/`
5. 最后在 app 初始化里把 `default_io + decoder_registry_apply()` 接起来

这样改动点都留在 app 侧，核心库接口可以保持不变。
