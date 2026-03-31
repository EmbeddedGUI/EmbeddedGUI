# `nano.specs` 导致 `memcpy`/`memset` 性能回退分析

## 问题概述

这次性能回退的根因，不在绘图算法本身，而在 QEMU Cortex-M 性能测试构建链路里引入了 `--specs=nano.specs`。

这个改动会把标准 C 库从常规 `newlib` 切换到 `newlib-nano` 变体。`newlib-nano` 的设计目标是减小代码体积和裁剪运行时开销，而不是追求 `memcpy`、`memset`、`printf` 这类基础库函数的极致吞吐。

对于普通应用，这个取舍可能可以接受；但对 EmbeddedGUI 这种软件渲染框架，它的影响非常大，因为大量热点路径都建立在高频、小批量到中批量的内存复制与清零之上。

## 这次回归是怎么引入的

QEMU 端历史配置里，`porting/qemu/build.mk` 曾经使用过下面的链接参数：

```makefile
STDCLIB_LDFLAGS = --specs=nano.specs --specs=rdimon.specs -lc -lrdimon -lgcc
```

其中：

- `rdimon.specs` 负责 semihosting 运行时支持，便于 QEMU 下输出日志与性能结果
- `nano.specs` 会把 C 库切换到 `newlib-nano`

现在已经改回：

```makefile
STDCLIB_LDFLAGS = --specs=rdimon.specs -lc -lrdimon -lgcc
```

也就是说，semihosting 仍然保留，但不再把整个性能测试构建默认切到 `newlib-nano`。

## 为什么 `nano.specs` 会显著拖慢 EmbeddedGUI

### 1. EmbeddedGUI 是典型的“内存操作密集型”软件渲染框架

虽然框架最终目标是画线、画圆、画文字、画图片，但底层真正消耗 CPU 的，不少时候不是算几何，而是反复做这些事情：

- 清空 PFB 局部帧缓冲
- 把解码后的图像行拷贝到目标缓冲
- 在 tile / row / cache 之间搬运像素数据
- 为文本和图片解码准备 scratch buffer

这些操作本质上都会落到 `memset` / `memcpy`，或者等价的内存搬运路径上。

### 2. PFB 机制会把小块内存操作放大成高频热点

EmbeddedGUI 使用局部帧缓冲（PFB）而不是整屏帧缓冲。优点是 RAM 占用低，但代价是：

- 一帧渲染会拆成很多 tile
- 每个 tile 都可能需要清零、覆盖、复制
- 同一类内存操作在一帧内会被调用很多次

因此，哪怕单次 `memcpy` / `memset` 只慢几十条指令，累计到一整帧后也会被放大成非常明显的总耗时差。

### 3. 图像直绘快路径直接依赖 `memcpy`

一个很典型的热点就在图像解码直绘路径里，例如 `src/image/egui_image_decode_utils.c` 中 RGB565 快路径会直接把源像素行拷贝到目标 PFB：

```c
egui_api_memcpy(dst, src_pixels, (size_t)count * sizeof(uint16_t));
```

这类路径的特点是：

- 调用频率高
- 数据长度通常不大，但非常密集
- 很容易受到 `memcpy` 实现质量影响

一旦底层 `memcpy` 退化成更保守、更偏体积优化的实现，图片、文字、填充图元都会一起变慢。

### 4. 在 QEMU `-icount shift=0` 下，这种差异会被稳定放大并精确观测到

性能框架采用 QEMU 指令计数模式，结果高度可重复。它不会像桌面系统那样被宿主机调度噪声掩盖。

这意味着：

- 如果 `memcpy` / `memset` 的实现多执行了很多指令
- 那么性能报告会非常稳定地显示出回退

这正是为什么这次问题能在多项基准里同时暴露出来，而且回退幅度非常一致。

## 为什么这不是“小影响”

从历史回退版本与当前修复版本对比，可以看到这不是局部波动，而是覆盖多个大类场景的系统性回退。

| 测试项 | 回退版本 | 修复后 | 提升倍数 |
|--------|----------|--------|----------|
| `RECTANGLE_FILL` | 0.564 ms | 0.155 ms | 3.64x |
| `TEXT` | 2.278 ms | 1.027 ms | 2.22x |
| `IMAGE_565` | 0.648 ms | 0.239 ms | 2.71x |
| `ROUND_RECTANGLE_FILL` | 1.190 ms | 0.781 ms | 1.52x |
| `LINE` | 0.944 ms | 0.535 ms | 1.76x |

这些数字说明了几个关键事实：

- 受影响的不是单一模块，而是基础图元、文字、图片一起回退
- 回退不是 5% 或 10% 级别，而是普遍达到 1.5x 到 3.6x
- `RECTANGLE_FILL`、`IMAGE_565` 这类高度依赖内存搬运的路径回退尤其明显

本次最新性能测试结果为：

- Commit: `f6cc71c`
- Profile: `cortex-m3`
- 解析测试项：222 项
- 失败项：0

这也说明根因修复后，性能基线已经恢复稳定。

## 从库实现角度看，为什么会这样

`newlib-nano` 的目标是缩小 ROM，而不是保证所有基础库函数都采用最激进的性能优化策略。

在这次问题里，表现出来的特征非常像下面这种情况：

- `memcpy` / `memset` 落到了更保守的通用实现
- 更偏向简单循环而不是更强的字宽批量拷贝
- 每次调用都多消耗了一批指令

如果应用只是偶尔拷贝一次大块数据，这种差异不一定显著；但对 GUI 软件渲染器而言，热点是“频繁、重复、小块到中块”的内存操作，这正好会把这种差异放大。

## 为什么会连带影响到文字、图片、填充图元

表面上看，`TEXT`、`IMAGE_565`、`RECTANGLE_FILL` 是不同模块，但它们底层会共享相同的内存行为：

### 填充图元

- PFB 清零
- 中间 tile 覆盖
- 行缓冲初始化

### 文本渲染

- glyph 像素准备
- 行缓冲与布局缓冲处理
- 旋转/裁剪时的 scratch buffer 搬运

### 图片渲染

- 解码行缓存
- RGB565 快路径直接拷贝
- 外部资源加载后的中间缓冲复制

因此，一旦 `memcpy` / `memset` 变慢，性能报告里通常会看到多个分类同时回退，而不是只有某一个 API 变慢。

## 当前修复方案为什么有效

这次修复不是只把 `nano.specs` 去掉就结束，而是顺手把“基础内存操作”做成了可控的平台抽象。

### 1. QEMU 性能测试默认回到常规 libc

QEMU 性能场景本质上是基准测试环境，目标是稳定测出框架真实渲染成本，而不是先为了省 ROM 把底层 libc 性能降下来。

因此，QEMU 端移除了 `--specs=nano.specs`，保留 `rdimon.specs`，这样可以：

- 继续使用 semihosting 输出
- 避免 `newlib-nano` 对性能测试结果造成系统性偏移

### 2. 增加 `egui_api_memset` / `egui_api_memcpy`

框架现在把内部高频内存操作统一收敛到 API 层：

- `egui_api_memset()`
- `egui_api_memcpy()`
- `egui_api_pfb_clear()` 内部也转到 `egui_api_memset()`

这样做的好处是：

- 默认情况下直接走标准库，路径最短
- 需要时可由平台层统一接管
- 不必在每个业务模块里分散写平台特判

### 3. 通过 `EGUI_CONFIG_PLATFORM_CUSTOM_MEMORY_OP` 开放平台优化入口

当平台确实有更快的实现时，例如：

- DMA 清零
- DMA 拷贝
- 特定 SRAM/TCM 优化例程

可以开启：

```c
EGUI_CONFIG_PLATFORM_CUSTOM_MEMORY_OP=1
```

然后在 `egui_platform_ops_t` 里注册：

- `memset_fast`
- `memcpy_fast`

如果没有开启这个宏，对应字段在编译期根本不会出现在 ops 里，也就没有额外的回调成本。

## 对移植和性能分析的结论

### 结论 1：不要把 `nano.specs` 当成“通用优化”盲目打开

对极端 ROM 紧张的产品，`nano.specs` 可能有价值；但它不是无副作用开关，尤其不适合作为性能基准环境的默认配置。

### 结论 2：当多个绘图类别一起回退时，先检查基础库与构建参数

如果你看到下面这种现象：

- 基础图元变慢
- 文本变慢
- 图片也变慢
- 算法代码最近又没发生同等规模改动

那么不要先怀疑每个绘图函数都退化了。更高概率的根因是：

- 编译选项变化
- `specs` 切换
- libc 实现变化
- LTO / 优化级别 / 内联行为变化

### 结论 3：性能敏感平台应把 `memcpy` / `memset` 视为一等热点

在软件渲染 GUI 里，`memcpy` / `memset` 不是“普通基础函数”，而是渲染主路径的一部分。对它们的实现选择，往往会直接决定填充、文本、图片这些上层指标。

## 建议的工程实践

1. 性能测试环境默认不要启用 `--specs=nano.specs`。
2. 如果量产版本必须使用 `newlib-nano`，必须重新跑完整性能回归，不要复用全量 libc 的基线。
3. 对 Cortex-M 真实硬件移植，优先评估是否需要接入 `EGUI_CONFIG_PLATFORM_CUSTOM_MEMORY_OP`。
4. 当 `RECTANGLE_FILL`、`TEXT`、`IMAGE_565` 同时明显回退时，先排查 `memcpy` / `memset` 实现，而不是先重写绘图算法。

## 相关文件

- `porting/qemu/build.mk`
- `src/core/egui_api.c`
- `src/core/egui_platform.h`
- `src/image/egui_image_decode_utils.c`
- `perf_output/perf_report.md`
