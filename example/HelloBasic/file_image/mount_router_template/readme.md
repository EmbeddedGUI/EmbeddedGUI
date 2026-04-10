# mount_router_template

这个目录提供一个“按路径前缀路由到不同存储后端”的 IO 模板，适合同一个 app 同时挂多种资源来源，例如：
- `sd:photo.jpg` 走 `FatFs/SD`
- `lfs:icon.png` 走 `LittleFS`
- `flash:logo.bmp` 走 `FlashMap/XIP`

设计意图：
- 模板仍然放在 `example/`，不进入 `src/`，核心层仍只看到一个普通的 `egui_image_file_io_t`。
- 通过前缀分流，应用层可以把多个 IO 适配器组合成一个“总入口”，不需要改 `egui_image_file` 本身。
- 当路径没有匹配任何前缀时，还可以回退到一个 `fallback_io`。

模板文件：
- `file_io_mount_router_template.c`
- `file_io_mount_router_template.h`

模板能力：
- 按最长前缀匹配路由，例如 `flash_cfg:` 会优先于 `flash:`
- 可选 `strip_prefix`，把 `sd:foo.jpg` 转成 `foo.jpg` 再交给下层 IO
- 统一包装 `open / read / seek / tell / close`

接入方式：
1. 先分别初始化底层 IO，例如 `FatFs`、`LittleFS`、`FlashMap`。
2. 准备一个前缀表，把每个 prefix 绑定到对应 `egui_image_file_io_t`。
3. 初始化路由 IO，再把它注册成 `egui_image_file` 的默认 IO。

示例：
```c
static const file_image_mount_router_entry_t g_mounts[] = {
        { "sd:", &g_sd_io, 1 },
        { "lfs:", &g_lfs_io, 1 },
        { "flash:", &g_flash_io, 1 },
};

static file_image_mount_router_context_t g_router_ctx = {
        .entries = g_mounts,
        .entry_count = sizeof(g_mounts) / sizeof(g_mounts[0]),
        .fallback_io = &g_lfs_io,
};
static egui_image_file_io_t g_router_io;

file_image_mount_router_io_init(&g_router_io, &g_router_ctx);
egui_image_file_set_default_io(&g_router_io);
```

路径示例：
- `sd:album/cat.jpg` -> 路由到 `g_sd_io`，若 `strip_prefix=1`，下层收到 `album/cat.jpg`
- `flash:boot/logo.bmp` -> 路由到 `g_flash_io`
- `splash.png` -> 如果没有前缀命中，则回退到 `fallback_io`

落地建议：
- 如果你的产品同时支持 `SD` 卡和片上/板载 Flash，这个模板可以直接做统一入口。
- 下层 IO 仍然可以继续使用各自的 `root_prefix`、地址表、文件系统挂载点等机制。
- decoder 注册顺序不受影响，仍然按 `BMP stream -> vendor JPEG -> TJpgDec -> stb_image` 之类的策略组合。
