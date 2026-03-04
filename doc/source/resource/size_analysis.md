# 二进制大小分析

EmbeddedGUI 提供了 `scripts/utils_analysis_elf_size.py` 脚本，用于分析各示例应用的二进制大小，帮助开发者了解代码、资源和 RAM 的占用情况。

## 基本用法

```bash
# 在项目根目录执行
python scripts/utils_analysis_elf_size.py
```

脚本会自动遍历 `example/` 目录下的所有示例应用（包括 HelloBasic 的所有子应用），逐个编译并分析 ELF 文件，最终生成 `output/README.md` 报告。

## 工作原理

### 编译流程

脚本对每个示例执行以下步骤：

1. `make clean` 清理上次构建产物
2. `make all -j PORT=stm32g0_empty APP=xxx` 使用 stm32g0_empty 平台编译
3. 解析 `output/main.elf` 中的符号表，提取各段大小

### ELF 符号提取

脚本通过 `pyelftools` 库解析 ELF 文件的 `.symtab` 段，提取以下链接器定义的符号：

| 符号名 | 含义 |
|--------|------|
| `__code_size` | 代码段（.text）大小 |
| `__rodata_size` | 只读数据段（.rodata）大小，主要是资源数据 |
| `__data_size` | 已初始化数据段（.data）大小 |
| `__bss_size` | 未初始化数据段（.bss）大小 |
| `__bss_pfb_size` | PFB 缓冲区占用的 BSS 大小 |

### 输出指标

| 列名 | 计算方式 | 说明 |
|------|---------|------|
| Code | `__code_size` | 纯代码占用的 Flash 空间 |
| Resource | `__rodata_size` | 资源数据（图片、字体）占用的 Flash 空间 |
| RAM | `__data_size + __bss_size - __bss_pfb_size` | 运行时 RAM 占用（不含 PFB） |
| PFB | `__bss_pfb_size` | PFB 帧缓冲区占用的 RAM |

## 输出报告格式

生成的 `output/README.md` 包含一个 Markdown 表格：

```markdown
| app                    | Code(Bytes)         | Resource(Bytes)     | RAM(Bytes)          | PFB(Bytes)          |
| ---------------------- | ------------------- | ------------------- | ------------------- | ------------------- |
|             HelloSimple|                 5432|                 1024|                  896|                 2400|
|          HelloActivity|                 8192|                 4096|                 1280|                 2400|
|    HelloBasic(button)  |                 6144|                 2048|                 1024|                 2400|
```

## 典型示例分析

以下是各类示例的典型资源占用范围（基于 stm32g0_empty 平台，RGB565，PFB 30x40）：

### 基础控件类

| 示例 | Code | Resource | RAM | PFB |
|------|------|----------|-----|-----|
| HelloSimple | ~5KB | ~1KB | ~1KB | 2.4KB |
| HelloBasic(button) | ~6KB | ~2KB | ~1KB | 2.4KB |
| HelloBasic(label) | ~6KB | ~2KB | ~1KB | 2.4KB |
| HelloBasic(image) | ~6KB | ~10KB+ | ~1KB | 2.4KB |

### 复杂应用类

| 示例 | Code | Resource | RAM | PFB |
|------|------|----------|-----|-----|
| HelloActivity | ~8KB | ~4KB | ~1.5KB | 2.4KB |
| HelloAPP | ~10KB | ~8KB | ~2KB | 2.4KB |

### 资源占用规律

- Code 大小主要取决于使用的控件种类和数量
- Resource 大小主要取决于图片和字体资源
- RAM 大小相对稳定，主要是控件实例和状态数据
- PFB 大小由 `EGUI_CONFIG_PFB_WIDTH * EGUI_CONFIG_PFB_HEIGHT * sizeof(egui_color_int_t)` 决定

## 分析自己项目的资源占用

### 前提条件

1. 安装 Python 依赖：`pip install pyelftools`
2. 确保 ARM GCC 工具链已安装（用于 stm32g0_empty 平台编译）
3. 链接脚本中需要定义 `__code_size`、`__rodata_size` 等符号

### 链接脚本符号定义

在你的链接脚本（`.ld` 文件）中添加以下符号定义：

```text
/* 在 .text 段末尾 */
__code_size = SIZEOF(.text);

/* 在 .rodata 段末尾 */
__rodata_size = SIZEOF(.rodata);

/* 在 .data 段末尾 */
__data_size = SIZEOF(.data);

/* 在 .bss 段末尾 */
__bss_size = SIZEOF(.bss);
```

### 单独分析某个 ELF 文件

如果只想分析单个 ELF 文件，可以直接调用脚本中的函数：

```python
from scripts.utils_analysis_elf_size import utils_process_elf_file

info = utils_process_elf_file('output/main.elf')
print(f"Code:     {info.code_size} bytes")
print(f"Resource: {info.rodata_size} bytes")
print(f"RAM:      {info.data_size + info.bss_size - info.bss_pfb_size} bytes")
print(f"PFB:      {info.bss_pfb_size} bytes")
```

### 对比分析

开发过程中，建议在关键节点运行分析脚本，对比资源占用变化：

```bash
# 修改前
python scripts/utils_analysis_elf_size.py
cp output/README.md output/README_before.md

# 修改后
python scripts/utils_analysis_elf_size.py

# 对比
diff output/README_before.md output/README.md
```

## 优化方向

根据分析结果，可以针对性地优化：

- Code 过大：检查是否引入了不必要的控件，使用条件编译裁剪
- Resource 过大：优化图片格式和尺寸，裁剪字体中不需要的字符
- RAM 过大：减少控件实例数量，使用 Page union 模式复用内存
- PFB 过大：减小 PFB 尺寸（需权衡渲染效率）

详细的优化策略请参考 [优化技巧](optimization_tips.md)。
