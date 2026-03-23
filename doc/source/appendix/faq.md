# 常见问题 (FAQ)

本文档整理了 EmbeddedGUI 开发过程中的常见问题及解决方案。

---

## 环境搭建

### Q: 运行 setup.bat 失败，提示网络错误

**问题描述：** 执行 `setup.bat` 时下载 w64devkit 工具链失败，提示连接超时或 SSL 错误。

**原因分析：** `setup.bat` 需要从 GitHub 下载 w64devkit 工具链压缩包，国内网络环境可能无法直接访问。

**解决方案：**
1. 使用代理或镜像站手动下载 w64devkit，解压到项目根目录
2. 确保 `w64devkit/bin` 目录包含 `gcc.exe`、`make.exe` 等工具
3. 重新运行 `setup.bat`，脚本会检测已存在的工具链并跳过下载

### Q: 找不到 make 命令

**问题描述：** 执行 `make all` 时提示 `make: command not found`。

**原因分析：** 系统 PATH 中没有 make 工具。EmbeddedGUI 项目自带了 `make.exe`。

**解决方案：**
1. 运行 `setup.bat` 自动配置环境
2. 或手动将 `w64devkit/bin` 添加到系统 PATH
3. 确认执行：`make --version` 应输出版本信息

### Q: 找不到 gcc 编译器

**问题描述：** `make all` 报错 `gcc: No such file or directory`。

**原因分析：** GCC 工具链未安装或未加入 PATH。

**解决方案：**
1. 运行 `setup.bat` 会自动下载并配置 w64devkit（含 GCC）
2. 或安装 MinGW-w64 / MSYS2，确保 `gcc` 在 PATH 中
3. 验证：`gcc --version`

### Q: Python 版本不兼容

**问题描述：** 运行资源生成脚本或编译检查脚本时报语法错误。

**原因分析：** 项目脚本需要 Python 3.6+，系统默认可能是 Python 2 或过旧版本。

**解决方案：**
1. 确认 Python 版本：`python --version`，需要 3.6 以上
2. 安装 Python 依赖：`pip install -r requirements.txt`（如有）
3. Windows 上确保 `python` 命令指向 Python 3，而非 Python 2

### Q: setup.bat 提示 pip 安装失败

**问题描述：** `setup.bat` 执行到 Python 依赖安装步骤时报错。

**原因分析：** pip 版本过旧或网络问题导致包下载失败。

**解决方案：**
1. 先升级 pip：`python -m pip install --upgrade pip`
2. 使用国内镜像：`pip install -i https://pypi.tuna.tsinghua.edu.cn/simple <package>`
3. 手动安装缺失的包后重新运行 `setup.bat`

---

## 编译问题

### Q: undefined reference to `egui_xxx`

**问题描述：** 链接阶段报 `undefined reference` 错误。

**原因分析：** 对应的源文件未被编译或未链接。EmbeddedGUI 使用 `build.mk` 模块系统管理源文件。

**解决方案：**
1. 检查对应模块的 `build.mk` 是否被包含
2. 确认 `EGUI_CODE_SRC` 中包含了所需的 `.c` 文件
3. 如果是新增控件，确保在示例的 `build.mk` 中添加了依赖模块
4. 运行 `make clean && make all APP=<your_app>` 全量重编

### Q: 头文件找不到 (No such file or directory)

**问题描述：** 编译报错找不到 `egui_xxx.h` 头文件。

**原因分析：** 头文件搜索路径未正确配置。

**解决方案：**
1. 检查模块 `build.mk` 中的 `EGUI_CODE_INCLUDE` 是否包含头文件所在目录
2. 确认 `src/` 目录结构完整，没有缺失文件
3. 如果是自定义模块，在 `build.mk` 中添加：`EGUI_CODE_INCLUDE += path/to/your/headers`

### Q: 链接错误：multiple definition

**问题描述：** 链接时报 `multiple definition of 'xxx'`。

**原因分析：** 同一个符号在多个编译单元中被定义，通常是全局变量或函数在头文件中定义而非声明。

**解决方案：**
1. 确保头文件中只有声明（`extern`），定义放在 `.c` 文件中
2. 使用 `static` 修饰仅在单个文件中使用的函数和变量
3. 检查是否重复包含了同一个 `.c` 文件

### Q: 编译警告：implicit declaration of function

**问题描述：** 编译时出现隐式函数声明警告。

**原因分析：** 调用函数前未包含对应的头文件。

**解决方案：**
1. 在源文件顶部添加缺失的 `#include` 指令
2. 检查函数名拼写是否正确
3. 确认函数在对应头文件中有声明

---

## 运行问题

### Q: 窗口不显示 / 程序启动后无画面

**问题描述：** PC 模拟器编译成功但运行后看不到窗口或窗口全黑。

**原因分析：** 可能是 PFB 缓冲区未正确初始化，或屏幕尺寸配置为 0。

**解决方案：**
1. 检查 `app_egui_config.h` 中 `EGUI_CONFIG_SCEEN_WIDTH` 和 `EGUI_CONFIG_SCEEN_HEIGHT` 是否正确
2. 确认 `egui_init()` 被正确调用且传入了有效的 PFB 缓冲区
3. 确认主循环中调用了 `egui_polling_work()`
4. 使用运行时验证：`python scripts/code_runtime_check.py --app <APP> [--app-sub <SUB>] --timeout 10`

### Q: 触摸没有反应

**问题描述：** 点击控件无响应，按钮不触发回调。

**原因分析：** 触摸事件未正确传递，或控件未设置点击监听器。

**解决方案：**
1. 确认控件调用了 `egui_view_set_on_click_listener()` 设置回调
2. 检查控件的 `is_clickable` 是否为 true
3. 确认控件的 `is_visible` 为 true 且 `is_gone` 为 false
4. 检查父容器是否拦截了触摸事件（`is_disallow_intercept`）
5. PC 模拟器中确认鼠标事件正确映射到触摸事件

### Q: 程序卡死 / 无响应

**问题描述：** 程序运行一段时间后卡死，不再刷新画面。

**原因分析：** 可能是死循环、栈溢出或定时器回调中执行了耗时操作。

**解决方案：**
1. 检查定时器回调和动画回调中是否有阻塞操作
2. 确认递归调用有正确的终止条件
3. 嵌入式平台检查栈大小是否足够
4. 使用 `code_runtime_check.py --timeout 10` 检测是否超时
5. 在 PC 模拟器中使用调试器定位卡死位置

### Q: 白屏 / 显示全白

**问题描述：** 屏幕显示全白，没有任何控件内容。

**原因分析：** PFB 缓冲区内容未正确刷新到屏幕，或视图树为空。

**解决方案：**
1. 确认 Activity 的 `on_create` 中添加了视图
2. 检查 `egui_port_flush_screen()` 移植函数是否正确实现
3. 确认 PFB 尺寸是屏幕尺寸的整数约数
4. 检查颜色深度配置 `EGUI_CONFIG_COLOR_DEPTH` 是否与硬件匹配

---

## 性能问题

### Q: 帧率低 / 刷新慢

**问题描述：** UI 刷新明显卡顿，帧率不足。

**原因分析：** PFB 尺寸过小导致分块过多，或绘制内容过于复杂。

**解决方案：**
1. 适当增大 PFB 尺寸（在 RAM 允许范围内）
2. 减少同时显示的控件数量
3. 使用脏矩形机制，避免不必要的全屏刷新
4. 启用双缓冲（配置 `pfb_backup` 缓冲区）
5. 优化 SPI/LCD 传输速率
6. 性能测试使用 QEMU 验证，PC 模拟器计时精度仅 1ms

### Q: 画面闪烁

**问题描述：** 屏幕内容闪烁或出现撕裂。

**原因分析：** 单缓冲模式下 PFB 写入和屏幕刷新不同步。

**解决方案：**
1. 启用双缓冲：在 `egui_init_config_t` 中设置 `pfb_backup` 指针
2. 使用 DMA 传输并在完成后调用 `egui_pfb_notify_flush_complete()`
3. 确保 LCD 的 TE（Tearing Effect）信号正确处理

### Q: 动画卡顿

**问题描述：** 动画播放不流畅，有明显跳帧。

**原因分析：** 动画更新间隔不均匀，或单帧绘制时间过长。

**解决方案：**
1. 确保 `egui_polling_work()` 被均匀调用（建议 16-33ms 间隔）
2. 减少动画目标视图的绘制复杂度
3. 避免在动画回调中执行耗时操作
4. 使用合适的插值器，避免过于复杂的缓动计算

---

## 移植问题

### Q: SPI 屏幕不亮

**问题描述：** 移植到新硬件后 LCD 屏幕不显示。

**原因分析：** SPI 初始化、LCD 初始化序列或引脚配置不正确。

**解决方案：**
1. 先用简单的 SPI 测试确认通信正常（读取 LCD ID）
2. 检查 LCD 初始化命令序列是否与屏幕型号匹配
3. 确认 CS、DC、RST、BL 引脚配置正确
4. 检查 SPI 时钟频率是否在 LCD 支持范围内
5. 确认背光引脚已正确驱动

### Q: 颜色显示反了 / 颜色不对

**问题描述：** 屏幕显示的颜色与预期不符，红蓝互换或颜色偏差。

**原因分析：** 颜色格式（RGB/BGR）或字节序不匹配。

**解决方案：**
1. 检查 `EGUI_CONFIG_COLOR_DEPTH` 配置（8/16/32）
2. RGB565 格式下确认字节序（大端/小端）与 LCD 控制器一致
3. 部分 LCD 需要发送 MADCTL 命令设置 RGB/BGR 顺序
4. 检查 `egui_port_flush_screen()` 中是否需要字节交换

### Q: 触摸坐标偏移 / 触摸不准

**问题描述：** 触摸位置与实际控件位置不对应。

**原因分析：** 触摸屏校准参数不正确，或坐标映射有误。

**解决方案：**
1. 确认触摸坐标范围与屏幕像素范围一致
2. 检查是否需要坐标翻转（X/Y 互换、镜像）
3. 在 `egui_port_input_motion()` 中添加坐标映射逻辑
4. 使用触摸测试程序验证原始坐标值

---

## 资源问题

### Q: 字体不显示 / 显示方块

**问题描述：** 文本控件显示空白或乱码方块。

**原因分析：** 字体资源未正确生成或未包含所需字符。

**解决方案：**
1. 运行 `make resource` 重新生成资源文件
2. 检查字体配置中是否包含了所需的字符集（ASCII、中文等）
3. 确认字体指针正确传递给 `egui_view_label_set_font()`
4. 检查字体文件是否被正确链接到最终二进制

### Q: 图片花屏 / 显示异常

**问题描述：** 图片控件显示花屏或图案错乱。

**原因分析：** 图片资源格式与配置不匹配，或图片数据损坏。

**解决方案：**
1. 确认图片资源使用 `make resource` 正确生成
2. 检查图片颜色格式与 `EGUI_CONFIG_COLOR_DEPTH` 一致
3. 确认图片尺寸参数与实际数据匹配
4. 检查图片数组是否被正确声明为 `const`（存放在 ROM 中）

### Q: ROM 空间不够

**问题描述：** 编译后二进制文件超出 Flash 容量。

**原因分析：** 字体和图片资源占用大量 ROM 空间。

**解决方案：**
1. 使用 `python scripts/utils_analysis_elf_size.py` 分析各模块占用
2. 减少字体字符集范围，只包含实际使用的字符
3. 降低图片分辨率或使用压缩格式
4. 移除未使用的控件模块（通过 `build.mk` 控制）
5. 使用外部 Flash 存储资源，通过 `egui_api_load_external_resource()` 加载
6. 开启编译优化（`-Os`）减小代码体积

### Q: RAM 不够用

**问题描述：** 运行时内存不足，程序崩溃或行为异常。

**原因分析：** PFB 缓冲区、控件实例和栈空间占用过多 RAM。

**解决方案：**
1. 减小 PFB 尺寸（`EGUI_CONFIG_PFB_WIDTH` / `EGUI_CONFIG_PFB_HEIGHT`）
2. 减少同时存在的控件实例数量
3. 使用静态分配而非动态分配
4. 检查栈大小配置，避免深层递归
5. 使用 `EGUI_CONFIG_DIRTY_AREA_COUNT` 控制脏区域数组大小
