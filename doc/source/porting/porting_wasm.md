# WebAssembly (Emscripten) 移植

EmbeddedGUI 支持通过 Emscripten 编译为 WebAssembly，在浏览器中运行 GUI 演示。这对于在线展示、文档演示和跨平台测试非常有用。

## 构建环境

### 前提条件

- Emscripten SDK (emsdk) 已安装并激活
- Python 3.x
- Make 工具

### 环境变量

```bash
# 激活 emsdk
source /path/to/emsdk/emsdk_env.sh

# 或设置 EMSDK_PATH
export EMSDK_PATH=/path/to/emsdk
```

## Emscripten 编译配置

### 构建命令

```bash
# 构建单个示例
make all APP=HelloSimple PORT=emscripten

# 构建后在 output/ 目录生成：
# - HelloSimple.html  (入口页面)
# - HelloSimple.js    (JS 胶水代码)
# - HelloSimple.wasm  (WebAssembly 二进制)
# - HelloSimple.data  (预加载资源，如有)
```

### build.mk 配置

`porting/emscripten/build.mk` 定义了 Emscripten 特有的编译选项：

```makefile
# 使用 Emscripten 的 SDL2 端口
COMMON_FLAGS += -s USE_SDL=2
LFLAGS += -s USE_SDL=2

# 内存配置
LFLAGS += -s ALLOW_MEMORY_GROWTH=1
LFLAGS += -s INITIAL_MEMORY=33554432    # 32MB 初始内存
LFLAGS += -s STACK_SIZE=5242880         # 5MB 栈

# 导出函数
LFLAGS += -s EXPORTED_FUNCTIONS='["_main"]'
LFLAGS += -s EXPORTED_RUNTIME_METHODS='["ccall","cwrap"]'

# 自定义 HTML 模板
LFLAGS += --shell-file $(EGUI_PORT_PATH)/shell.html

# 预加载资源文件到虚拟文件系统
LFLAGS += --preload-file $(OUTPUT_PATH)/app_egui_resource_merge.bin@app_egui_resource_merge.bin
```

### Makefile.emscripten

`porting/emscripten/Makefile.emscripten` 是 Emscripten 专用的构建规则文件，替代 PC 平台的 `Makefile.base`。主要区别：

- 编译器使用 `emcc` 而非 `gcc`
- 输出目标为 `.html` 而非可执行文件
- 排除 PC 平台的 `main.c` 和 `egui_port_pc.c`，使用 Emscripten 专用版本
- 使用 per-app OBJDIR 避免重复编译共享核心库

## 浏览器事件映射

### 主循环适配

浏览器环境不允许阻塞式主循环。Emscripten 使用 `emscripten_set_main_loop` 替代 `while(1)`：

```c
static void main_loop_iteration(void)
{
    egui_polling_work();
    VT_sdl_refresh_task();

    if (VT_is_request_quit())
    {
        emscripten_cancel_main_loop();
    }
}

int main(int argc, const char *argv[])
{
    VT_init();
    egui_port_init();
    egui_init(&init_config);
    uicode_create_ui();
    egui_screen_on();

    // 0 = requestAnimationFrame (~60fps), 1 = simulate infinite loop
    emscripten_set_main_loop(main_loop_iteration, 0, 1);

    VT_deinit();
    return 0;
}
```

与 PC 版本的关键区别：
- 单线程运行（浏览器主线程）
- 使用 `requestAnimationFrame` 驱动帧循环
- `delay` 函数为空操作（不能阻塞浏览器）

### 时间戳

使用 `emscripten_get_now()` 获取高精度时间戳：

```c
static uint32_t em_get_tick_ms(void)
{
    return (uint32_t)emscripten_get_now();
}
```

### 断言处理

浏览器环境下不能使用 `while(1)` 死循环，改为取消主循环并退出：

```c
static void em_assert_handler(const char *file, int line)
{
    printf("Assert@ file = %s, line = %d\n", file, line);
    emscripten_cancel_main_loop();
    emscripten_force_exit(1);
}
```

### 触摸/鼠标事件

Emscripten 的 SDL2 端口自动将浏览器鼠标和触摸事件映射为 SDL 事件，与 PC 模拟器共享同一套 SDL 事件处理代码。

### 资源加载

外部资源通过 Emscripten 的 `--preload-file` 预加载到虚拟文件系统，代码中使用标准 `fopen`/`fread` 访问：

```c
strcpy(input_file_path, "app_egui_resource_merge.bin");
// 后续通过 fopen(input_file_path, "rb") 访问
```

## HTML 模板

`porting/emscripten/shell.html` 提供了简洁的 HTML 模板：

- 全屏黑色背景，居中显示 Canvas
- 加载状态指示器（spinner + 进度）
- WebGL 上下文丢失自动恢复
- 响应式布局，适配不同屏幕

模板中的 `{{{ SCRIPT }}}` 占位符会被 Emscripten 替换为生成的 JS 代码。

## wasm_build_demos.py 批量构建

`scripts/wasm_build_demos.py` 用于批量构建所有示例的 WASM 版本。

### 用法

```bash
# 构建所有示例
python scripts/wasm_build_demos.py

# 指定 emsdk 路径
python scripts/wasm_build_demos.py --emsdk-path /path/to/emsdk

# 指定输出目录
python scripts/wasm_build_demos.py --output-dir web/demos

# 只构建指定示例
python scripts/wasm_build_demos.py --app HelloSimple
```

### 工作流程

1. 扫描 `example/` 目录获取所有示例列表
2. 对每个示例：
   - 生成资源文件（`make resource`）
   - 使用 Emscripten 编译（`make all PORT=emscripten`）
   - 复制输出文件到部署目录
3. 使用 per-app OBJDIR 优化，核心库只编译一次
4. HelloBasic 的子应用共享 OBJDIR，进一步减少编译时间

### 并行构建

脚本使用 `ProcessPoolExecutor` 支持并行构建多个示例，大幅缩短总构建时间。

## GitHub Pages 部署

### 目录结构

批量构建后的部署目录结构：

```
web/
├── index.html          # 示例列表页面
└── demos/
    ├── HelloSimple/
    │   ├── HelloSimple.html
    │   ├── HelloSimple.js
    │   ├── HelloSimple.wasm
    │   └── HelloSimple.data
    ├── HelloActivity/
    │   └── ...
    └── HelloBasic_button/
        └── ...
```

### CI 配置

在 GitHub Actions 中自动构建和部署：

```yaml
- name: Setup Emscripten
  uses: mymindstorm/setup-emsdk@v11

- name: Build WASM demos
  run: python scripts/wasm_build_demos.py --output-dir web/demos

- name: Deploy to GitHub Pages
  uses: peaceiris/actions-gh-pages@v3
  with:
    publish_dir: ./web
```

### 本地预览

```bash
# 构建完成后，在 output/ 目录启动 HTTP 服务器
cd output
python3 -m http.server 8000

# 浏览器打开
# http://localhost:8000/HelloSimple.html
```

## Windows 本地 emsdk 工作流

推荐先在仓库根目录准备本地 emsdk：

```bat
python scripts\setup_env.py --python-mode none --install-emsdk
```

Windows 下 `porting/emscripten/build.mk` 会通过 `python scripts/emcc_wrapper.py` 优先使用仓库内的 `tools\emsdk`。因此直接执行以下命令即可，不要求先手动激活当前 shell：

```bat
make all APP=HelloSimple PORT=emscripten
```

`scripts/wasm_build_demos.py` 也会优先复用本地 emsdk。只有在当前终端手动执行 `emcc -v`、`em++ -v` 等命令时，才需要额外运行：

```bat
call tools\emsdk\emsdk_env.bat
```

如需跳过 Emscripten 检查，可在环境脚本中使用：

```bat
python scripts\setup_env.py --python-mode none --skip-emsdk
```

## SDL2 端口缓存排障

当前 WASM 端口使用 `-s USE_SDL=2`。首次构建时，Emscripten 会在 `tools/emsdk/upstream/emscripten/cache/ports/` 下载 SDL2 端口，因此第一次构建可能明显更慢。

如果下载过程中网络中断，或者历史代理残留导致 SDL2 端口缓存变成半成品，常见现象是构建长时间停在 SDL2 port 阶段。可删除以下缓存后重试：

```text
tools/emsdk/upstream/emscripten/cache/ports/sdl2/
tools/emsdk/upstream/emscripten/cache/ports/sdl2.zip
```

清理后重新执行：

```bat
make all APP=HelloSimple PORT=emscripten
```

## 注意事项

- Emscripten 构建禁用了录制测试（`EGUI_CONFIG_RECORDING_TEST=0`）
- 浏览器中 `delay` 为空操作，不会阻塞
- 初始内存设为 32MB，启用了 `ALLOW_MEMORY_GROWTH` 允许动态增长
- 栈大小设为 5MB，足够大多数 GUI 应用使用
- WASM 文件通常比原生二进制大，但经过 gzip 压缩后传输量可接受
