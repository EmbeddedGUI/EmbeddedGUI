# ResourceManager API

ResourceManager 是 EmbeddedGUI 的运行时资源管理模块，负责外部资源的加载和管理。通过 ResourceManager，应用可以在运行时从外部存储（SPI Flash、SD 卡、文件系统等）按需加载图片和字体资源。

## 启用 ResourceManager

在 `app_egui_config.h` 中启用相关配置：

```c
#define EGUI_CONFIG_FUNCTION_RESOURCE_MANAGER  1
#define EGUI_CONFIG_FUNCTION_EXTERNAL_RESOURCE 1
```

## 工作原理

### 资源索引机制

资源生成脚本会为每个外部资源分配一个唯一的 ID（`EGUI_EXT_RES_ID_xxx`），并生成偏移地址映射表 `egui_ext_res_id_map[]`。运行时，框架通过 ID 查找资源在 `app_egui_resource_merge.bin` 中的偏移地址，再调用平台层的 `load_external_resource` 接口加载数据。

```
应用代码 → 引用外部资源结构体（_bin 后缀）
    → 框架检测到外部资源标记
    → 通过 res_id 查 egui_ext_res_id_map 获取偏移
    → 调用 platform.load_external_resource(dest, res_id, offset, size)
    → 平台层从外部存储读取数据到 dest 缓冲区
```

### 资源 ID 枚举

生成的 `app_egui_resource_generate.h` 中包含资源 ID 枚举：

```c
enum {
    EGUI_EXT_RES_ID_BASE = 0x00,
    EGUI_EXT_RES_ID_EGUI_RES_IMAGE_STAR_RGB565_4_DATA,
    EGUI_EXT_RES_ID_EGUI_RES_IMAGE_STAR_RGB565_4_ALPHA,
    EGUI_EXT_RES_ID_EGUI_RES_FONT_TEST_16_4_PIXEL_BUFFER,
    EGUI_EXT_RES_ID_EGUI_RES_FONT_TEST_16_4_CHAR_DESC,
    EGUI_EXT_RES_ID_MAX,
};
```

### 偏移地址表

`app_egui_resource_generate.c` 中的偏移表记录每个资源在 bin 文件中的起始位置：

```c
const uint32_t egui_ext_res_id_map[EGUI_EXT_RES_ID_MAX] = {
    0x00000000,  // BASE
    0x00000000,  // IMAGE_STAR_RGB565_4_DATA
    0x00004B00,  // IMAGE_STAR_RGB565_4_ALPHA
    // ...
};
```

## 平台层实现

### PC 平台（文件系统）

PC 模拟器通过标准文件 I/O 加载外部资源：

```c
static void pc_load_external_resource(void *dest, uint32_t res_id,
                                       uint32_t start_offset, uint32_t size)
{
    FILE *file;
    extern const uint32_t egui_ext_res_id_map[];
    uint32_t res_offset = egui_ext_res_id_map[res_id];
    uint32_t res_real_offset = res_offset + start_offset;

    file = fopen(pc_get_input_file_path(), "rb");
    if (file == NULL) return;

    fseek(file, res_real_offset, SEEK_SET);
    fread(dest, 1, size, file);
    fclose(file);
}
```

### MCU 平台（SPI Flash）

嵌入式平台通过 SPI Flash 驱动加载：

```c
static void mcu_load_external_resource(void *dest, uint32_t res_id,
                                        uint32_t start_offset, uint32_t size)
{
    extern const uint32_t egui_ext_res_id_map[];
    uint32_t res_offset = egui_ext_res_id_map[res_id];
    uint32_t res_real_offset = res_offset + start_offset;

    spi_flash_read(RESOURCE_BASE_ADDR + res_real_offset, dest, size);
}
```

注册到平台驱动：

```c
static const egui_platform_ops_t mcu_platform_ops = {
    // ...
    .load_external_resource = mcu_load_external_resource,
    // ...
};
```

## 使用示例

参考 `example/HelloResourceManager/` 示例工程。

### 资源配置

`resource/src/app_resource_config.json` 中同时配置内部和外部资源：

```json
{
    "img": [
        {
            "file": "star.png",
            "external": "all",
            "format": "all",
            "alpha": "all"
        },
        {
            "file": "test.png",
            "external": "1",
            "format": "rgb565",
            "alpha": "4"
        }
    ],
    "font": [
        {
            "file": "build_in/Montserrat-Medium.ttf",
            "name": "test",
            "text": "supported_text_test.txt",
            "external": "all",
            "pixelsize": "16",
            "fontbitsize": "all"
        }
    ]
}
```

### 应用代码

```c
#include "app_egui_resource_generate.h"

static egui_view_image_t image_1;
static egui_view_label_t label_1;
static egui_view_label_t label_2;

// 使用外部图片资源（_bin 后缀）
EGUI_VIEW_IMAGE_PARAMS_INIT(image_1_params, 10, 10, 100, 100,
    (egui_image_t *)&egui_res_image_test_rgb565_4_bin);

// 使用内部字体资源
EGUI_VIEW_LABEL_PARAMS_INIT(label_1_params, 10, 10, 160, 160,
    "Hello World!", (egui_font_t *)&egui_res_font_test_16_4,
    EGUI_COLOR_WHITE, EGUI_ALPHA_100);

// 使用外部字体资源（_bin 后缀）
EGUI_VIEW_LABEL_PARAMS_INIT(label_2_params, 10, 100, 160, 160,
    "External Resource!", (egui_font_t *)&egui_res_font_test_16_4_bin,
    EGUI_COLOR_WHITE, EGUI_ALPHA_100);

void uicode_init_ui(void)
{
    egui_view_image_init_with_params(EGUI_VIEW_OF(&image_1), &image_1_params);
    egui_view_label_init_with_params(EGUI_VIEW_OF(&label_1), &label_1_params);
    egui_view_label_init_with_params(EGUI_VIEW_OF(&label_2), &label_2_params);

    egui_core_add_user_root_view(EGUI_VIEW_OF(&image_1));
    egui_core_add_user_root_view(EGUI_VIEW_OF(&label_1));
    egui_core_add_user_root_view(EGUI_VIEW_OF(&label_2));
}
```

### 运行时切换资源

可以在运行时动态切换图片资源：

```c
// 切换到不同的外部图片
egui_view_image_set_image(EGUI_VIEW_OF(&image_1),
    (egui_image_t *)&egui_res_image_star_rgb565_4_bin);
```

## 外部资源烧录

`app_egui_resource_merge.bin` 文件需要烧录到外部存储的指定地址。平台层的 `load_external_resource` 实现中需要加上基地址偏移：

```c
#define RESOURCE_BASE_ADDR  0x00000000  // 外部 Flash 中资源的起始地址

static void mcu_load_external_resource(void *dest, uint32_t res_id,
                                        uint32_t start_offset, uint32_t size)
{
    extern const uint32_t egui_ext_res_id_map[];
    uint32_t offset = egui_ext_res_id_map[res_id] + start_offset;
    spi_flash_read(RESOURCE_BASE_ADDR + offset, dest, size);
}
```

## 注意事项

- 外部资源的命名规则：内部资源为 `egui_res_xxx`，对应的外部资源为 `egui_res_xxx_bin`
- `external` 设为 `"all"` 会同时生成内部和外部两个版本，方便测试，但实际项目中建议明确指定
- 外部资源加载是同步的，大资源可能导致帧率下降，建议将高频访问的资源放在内部
- PC 模拟器运行时，需要将 `app_egui_resource_merge.bin` 放在可执行文件同目录或通过命令行参数指定路径
