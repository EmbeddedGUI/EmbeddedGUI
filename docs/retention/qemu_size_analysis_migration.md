# QEMU Size 分析迁移保留报告

## 背景

`scripts/utils_analysis_elf_size.py` 之前使用 `stm32g0_empty` 口径编译所有 case，只能拿到 ELF 静态 size，无法得到运行期 `heap` 和 `stack` 数据。  
现在 qemu 侧已经具备运行期统计能力，因此可以把 size 文档的默认统计口径切到 qemu，从而减少一条专门为 size 报告维护的 porting 链路。

本次迁移的目标有两个：

1. 用 qemu 统一输出静态 `code/resource/ram/pfb` 与运行期 `heap/stack`。
2. 评估 qemu 本身是否会引入明显的“无关 size 膨胀”。

本次采用的阈值：

- code 增量不应明显大于 `10KB`
- static RAM 增量不应明显大于 `1KB`

## 实现内容

本次迁移包含以下关键改动：

- `scripts/utils_analysis_elf_size.py`
  - 统计范围改为固定集合：
    - `HelloBasic` 全部子 case
    - `HelloSimple`
    - `HelloPerformance`
    - `HelloShowcase`
    - `HelloStyleDemo`
    - `HelloVirtual(APP_SUB=virtual_stage_showcase)`
  - 静态 size 统一使用 `PORT=qemu CPU_ARCH=cortex-m0plus`
  - 运行期统计统一使用 `qemu-system-arm -machine mps2-an385 -cpu cortex-m3`
  - 输出 `heap_idle / heap_peak / stack_peak`
  - 每次构建前清理 `output/main.elf` / `output/main.map`，修复 Windows 下连续 size/measure 构建的“未重新链接”问题

- `porting/qemu/port_main.c`
  - `egui_pfb` 放入 `.bss.pfb_area`
  - 新增 stack watermark 统计，输出 `HEAP_RESULT:stack_peak_bytes=...`
  - 补 `recording_request_snapshot()` 的 weak no-op，实现录制回放 case 的 qemu measure 构建

- `porting/qemu/build.mk`
  - 默认使用 `nano.specs + rdimon.specs`

- `porting/qemu/GCC/Makefile.base`
  - qemu 构建继承根 Makefile 的 `COMPILE_OPT_LEVEL` / `COMPILE_DEBUG`

- 构建阻塞修复
  - `src/image/egui_image_qoi.c`
  - `src/image/egui_image_rle.c`
  - `src/mask/egui_mask_circle.c`
  - `example/HelloBasic/image/app_egui_resource_generate.h`

## 本次结果

### 1. QEMU 正式口径

执行时间：`2026-03-31`  
提交：`e60544f`

执行命令：

```bash
python scripts/utils_analysis_elf_size.py
python scripts/size_to_doc.py
```

结果：

- qemu 目标集合 `64/64` 成功
- `failures = 0`
- 已生成：
  - `output/README.md`
  - `output/size_results.json`
  - `doc/source/size/size_overview.md`
  - `doc/source/size/size_report.md`
  - `doc/source/size/images/size_report.png`

### 2. qemu vs stm32g0_empty 静态 size 对比

为了评估 qemu 是否引入额外膨胀，本次额外对同一批 case 跑了 `stm32g0_empty` 静态 size 对比。  
对比结果文件：

- `output/stm32g0_empty_size_results.json`

共同成功 case 数：`61`

静态 delta 定义：

- `delta = qemu - stm32g0_empty`

对比结论：

| 指标 | min | median | max |
|------|-----|--------|-----|
| Code delta | `-728 B` | `-728 B` | `-292 B` |
| ROM delta(code + resource) | `-520 B` | `-520 B` | `-80 B` |
| RAM delta(不含 PFB) | `-120 B` | `-112 B` | `48 B` |
| PFB delta | `0 B` | `0 B` | `0 B` |

阈值检查：

- `code > +10KB`：`0` 个
- `static RAM > +1KB`：`0` 个

结论：

- 当前 qemu 口径没有引入用户担心的 `>10KB` 代码膨胀
- 当前 qemu 口径没有引入用户担心的 `>1KB` 静态 RAM 膨胀
- 在这批样本里，qemu 的静态 size 反而通常略小于 `stm32g0_empty`

### 3. 旧链路维护成本

`stm32g0_empty` 对比过程中，仍有 `3` 个大 case 无法在 Windows 下稳定完成链接：

- `HelloBasic(mp4)`：链接命令过长，`make Error 87`
- `HelloPerformance`：链接命令过长，`make Error 87`
- `HelloVirtual(virtual_stage_showcase)`：`collect2.exe CreateProcess` 失败

这说明如果继续保留“文档 size 统计必须依赖 `stm32g0_empty`”这条链路，除了拿不到运行期 `heap/stack` 之外，还会持续承担额外的构建兼容性维护成本。

## 决策

本次结论如下：

1. `scripts/utils_analysis_elf_size.py` 默认统计口径切换到 qemu 是可行的。
2. qemu 口径可以同时提供静态 size 与运行期 `heap/stack`，信息完整性优于 `stm32g0_empty`。
3. qemu 没有带来超过阈值的额外 code/RAM 膨胀。
4. `stm32g0_empty` 保留为移植模板与必要时的定向交叉验证口径，不再作为 size 文档的默认自动统计依赖。

## 后续建议

- 如果后续需要评估“真实 MCU HAL / linker script / startup”带来的额外差异，建议单独做专项对比，不要重新把默认 size 文档绑定回 `stm32g0_empty`。
- 若后续新增 qemu runtime 指标，例如 transient heap、idle stack、interaction stack，可继续沿用同一条 qemu 统计链路扩展，不需要再引入新的 porting 分支。
