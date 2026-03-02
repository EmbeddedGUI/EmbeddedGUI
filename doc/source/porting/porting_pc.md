# PC 模拟器移植

PC 模拟器是 EmbeddedGUI 的主要开发和调试平台，基于 SDL2 实现显示和输入。开发者可以在 PC 上快速迭代 UI 代码，无需烧录到硬件。

## 构建与运行

```bash
# 构建
make all APP=HelloSimple PORT=pc

# 运行
make run

# 或直接运行
./output/main.exe
```

## SDL2 显示后端

### 架构

PC 模拟器的显示通过 SDL2 实现，核心文件：

- `porting/pc/sdl_port.c` - SDL2 窗口管理、渲染、事件处理
- `porting/pc/sdl_port.h` - SDL 接口声明
- `porting/pc/egui_port_pc.c` - Display/Platform Driver 注册
- `porting/pc/main.c` - 主入口

### 显示驱动实现

`egui_port_pc.c` 中的 Display Driver 将 PFB 数据写入 SDL 纹理：

```c
static void pc_display_draw_area(int16_t x, int16_t y, int16_t w, int16_t h,
                                  const egui_color_int_t *data)
{
    VT_Fill_Multiple_Colors(x, y, x + w - 1, y + h - 1, (egui_color_int_t *)data);
}

static void pc_display_flush(void)
{
    VT_sdl_flush(1);
}
```

`VT_Fill_Multiple_Colors` 将像素数据写入内部帧缓冲，`VT_sdl_flush` 将帧缓冲更新到 SDL 纹理并呈现到窗口。

### 窗口配置

窗口大小由 `app_egui_config.h` 中的屏幕尺寸决定：

```c
#define EGUI_CONFIG_SCEEN_WIDTH  240
#define EGUI_CONFIG_SCEEN_HEIGHT 320
```

SDL 窗口会按此尺寸创建，可能会有缩放以适应桌面显示。

## 触摸/键盘事件映射

### 鼠标模拟触摸

SDL 鼠标事件被映射为触摸事件：

| SDL 事件 | EGUI 事件 |
|----------|-----------|
| `SDL_MOUSEBUTTONDOWN` | `EGUI_MOTION_EVENT_ACTION_DOWN` |
| `SDL_MOUSEMOTION`（按下时） | `EGUI_MOTION_EVENT_ACTION_MOVE` |
| `SDL_MOUSEBUTTONUP` | `EGUI_MOTION_EVENT_ACTION_UP` |

鼠标坐标会根据窗口缩放比例转换为屏幕坐标。

### 键盘事件

SDL 键盘事件被映射为 EGUI 按键事件：

| SDL 按键 | EGUI 按键 |
|----------|-----------|
| `SDLK_RETURN` | `EGUI_KEY_CODE_ENTER` |
| `SDLK_ESCAPE` | `EGUI_KEY_CODE_BACK` |
| `SDLK_UP/DOWN/LEFT/RIGHT` | 方向键 |

需要在 `app_egui_config.h` 中启用按键支持：

```c
#define EGUI_CONFIG_FUNCTION_SUPPORT_KEY 1
```

## 文件系统资源加载

PC 平台通过标准文件 I/O 加载外部资源：

```c
static void pc_load_external_resource(void *dest, uint32_t res_id,
                                       uint32_t start_offset, uint32_t size)
{
    extern const uint32_t egui_ext_res_id_map[];
    uint32_t res_offset = egui_ext_res_id_map[res_id];
    uint32_t res_real_offset = res_offset + start_offset;

    FILE *file = fopen(pc_get_input_file_path(), "rb");
    fseek(file, res_real_offset, SEEK_SET);
    fread(dest, 1, size, file);
    fclose(file);
}
```

资源文件路径可通过命令行参数指定：

```bash
# 默认加载当前目录的 app_egui_resource_merge.bin
./output/main.exe

# 指定资源文件路径
./output/main.exe /path/to/app_egui_resource_merge.bin
```

## Platform Driver 实现

PC 平台的 Platform Driver 使用标准库函数：

| 接口 | PC 实现 |
|------|---------|
| `malloc/free` | 标准 `malloc`/`free` |
| `vlog` | `vprintf`（输出到终端） |
| `get_tick_ms` | `sdl_get_system_timestamp_ms()`（SDL 时间戳） |
| `delay` | `sdl_port_sleep()` |
| `pfb_clear` | `memset` |
| `interrupt_disable/enable` | 空操作（PC 无中断概念） |

## 主循环结构

PC 模拟器使用双线程架构：

```c
int main(int argc, const char *argv[])
{
    VT_init();                    // 初始化 SDL 窗口
    egui_port_init();             // 注册驱动
    egui_init(&init_config);      // 初始化框架
    uicode_create_ui();           // 创建 UI
    egui_screen_on();             // 开屏

    // GUI 逻辑在独立线程
    SDL_CreateThread(egui_main_thread, "egui thread", NULL);

    // 主线程处理 SDL 事件
    while (1)
    {
        VT_sdl_refresh_task();    // 处理 SDL 事件和渲染
        if (VT_is_request_quit()) break;
    }

    VT_deinit();
    return 0;
}
```

- 主线程：处理 SDL 事件（鼠标、键盘、窗口关闭）和屏幕渲染
- GUI 线程：执行 `egui_polling_work()`，处理 UI 逻辑和 PFB 渲染

## 录制功能

PC 模拟器支持截图录制，用于自动化测试和 GIF 生成：

```bash
# 录制 10 秒，30fps
./output/main.exe --record output_dir 30 10

# 加速录制（2 倍速）
./output/main.exe --record output_dir 30 10 --speed 2
```

在 `app_egui_config.h` 中启用录制测试：

```c
#define EGUI_CONFIG_RECORDING_TEST 1
```

应用可以实现 `egui_port_get_recording_action()` 定义录制期间的模拟操作（点击、滑动、等待等）。

## 调试技巧

### 启用调试信息

```c
#define EGUI_CONFIG_DEBUG_LOG_LEVEL EGUI_LOG_IMPL_LEVEL_DBG
#define EGUI_CONFIG_DEBUG_PFB_REFRESH 1          // 显示 PFB 刷新区域
#define EGUI_CONFIG_DEBUG_DIRTY_REGION_REFRESH 1 // 显示脏区域
#define EGUI_CONFIG_DEBUG_INFO_SHOW 1            // 显示帧率等信息
```

### 运行时验证

使用 `code_runtime_check.py` 自动运行并截图验证：

```bash
python scripts/code_runtime_check.py --app HelloSimple --timeout 10
```

截图保存在 `runtime_check_output/` 目录下。

### 常见问题

- SDL2 未安装：确保系统安装了 SDL2 开发库
- 窗口不显示：检查 `EGUI_CONFIG_SCEEN_WIDTH/HEIGHT` 配置
- 触摸无响应：确认 `EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH` 为 1
- 资源加载失败：确认 `app_egui_resource_merge.bin` 在正确路径
