﻿# QEMU Size 分析迁移保留报告

## 背景

过去 `scripts/size_analysis/utils_analysis_elf_size.py` 依赖 `stm32g0_empty` 口径批量编译所有 case。
这种方式只能拿到 ELF 静态体积，无法得到运行期的：

- `heap`
- `stack`

而当前 qemu 侧已经具备运行期统计能力，因此可以把 size 文档的默认统计口径迁移到 qemu，用同一条链路同时产出静态和运行期数据。

本次迁移的目标有两个：

1. 统一输出 `code / resource / ram / pfb / heap / stack`
2. 评估 qemu 口径是否会引入明显的额外体积膨胀

本次评估采用的判断阈值：

- `code` 增量不应明显大于 `10KB`
- `static RAM` 增量不应明显大于 `1KB`

## 本次改动

涉及的核心内容如下：

- `scripts/size_analysis/utils_analysis_elf_size.py`
  - 默认统计口径切到 qemu
  - 同时采集静态 size 与运行期 `heap/stack`
  - 统一输出 `size_results.json`

- `porting/qemu/port_main.c`
  - 增加运行期 watermark / heap 结果输出

- `scripts/size_analysis/size_to_doc.py`
  - 根据新的 qemu 结果生成 `doc/source/size/` 下的文档

## 结果

执行时间：`2026-03-31`
对应提交：`e60544f`

执行命令：

```bash
python scripts/size_analysis/utils_analysis_elf_size.py
python scripts/size_analysis/size_to_doc.py
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

## qemu vs 旧 stm32g0_empty 对比结论

为了评估 qemu 是否带来无关膨胀，本次额外做了同一批 case 的 `stm32g0_empty` 静态体积对比。

结论摘要：

| 指标 | min | median | max |
|------|-----:|-------:|----:|
| Code delta | `-728B` | `-728B` | `-292B` |
| ROM delta(code + resource) | `-520B` | `-520B` | `-80B` |
| RAM delta(不含 PFB) | `-120B` | `-112B` | `48B` |
| PFB delta | `0B` | `0B` | `0B` |

阈值检查结果：

- `code > +10KB`: `0` 个
- `static RAM > +1KB`: `0` 个

因此可以得出：

- qemu 口径没有引入超过阈值的额外 `code` 膨胀
- qemu 口径没有引入超过阈值的额外 `static RAM` 膨胀
- 在这批样本里，qemu 的静态体积通常还略小于旧 `stm32g0_empty`

## 迁移决策

本次迁移后的结论是：

1. `scripts/size_analysis/utils_analysis_elf_size.py` 默认统计口径切到 qemu 是可行的
2. qemu 可以同时提供静态体积与运行期 `heap/stack`，信息完整性优于旧链路
3. qemu 没有带来不可接受的额外体积膨胀
4. 这份对比报告只保留为历史记录；当前仓库已删除 `stm32g0_empty`，相关 MCU 入口统一收敛到 `stm32g0`

## 后续建议

- 如果后续需要评估“真实 MCU linker/startup/HAL”带来的差异，建议基于 `stm32g0` 单独做定向对比，不要再把默认 size 文档绑回旧模板口径
- 如果后续需要增加新的运行期指标，例如更细的 `heap` 或 `stack` 分层统计，继续沿用 qemu 这条链路扩展即可
