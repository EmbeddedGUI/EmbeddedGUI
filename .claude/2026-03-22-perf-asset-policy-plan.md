# 2026-03-22 性能测试资源策略收敛 Plan

## 目标

- 固定 `HelloPerformance` 的测试资源策略，避免后续再次为 `test_perf` 生成无意义的 alpha 版本
- 约束规则为：普通性能图永远使用 `RGB565`，凡是涉及透明通道的场景统一使用 `star`
- 在保证 QEMU 数据可对比的同时，继续检查 `HelloPerformance` 与 `HelloBasic/mask` 的渲染正确性

## 本轮方案

1. 审查 `example/HelloPerformance/egui_view_test_performance.c` 与 `example/HelloPerformance/resource/src/app_resource_config.json`，确认哪些场景仍依赖 `test_perf` alpha 资源
2. 调整测试例程：`IMAGE_*_1/2/4/8`、对应 resize / rotate / tiled / external 场景统一切换到 `star` 资源；`test_perf` 只保留 `alpha=0` 的 opaque 资源
3. 精简 legacy 重复场景，禁用不再需要的独立 `*_STAR_*` 场景和 `IMAGE_565_8_DOUBLE` 专用大图 alpha 资源
4. 重新生成 `HelloPerformance` 资源与性能文档，并刷新场景拼图，确保文档中的渲染对照与 QEMU 结果一致
5. 运行 QEMU、单测和 runtime check，并人工检查多行文本、透明图像和 mask 关键截图

## 结果

- `test_perf_40/120/240/480` 现在仅保留 `RGB565 alpha=0` 资源
- 透明测试统一切换为 `star_40/120/240` 及其 external 版本，删除了全部 `test_perf_*` alpha 资源和无用的默认 `test/star` 图片资源
- `HelloPerformance` 中 legacy `*_STAR_*` 场景保留为兼容别名，但已禁用，避免 perf 报告重复
- 资源总量从 `3126629` 降到 `2273129`，减少 `853500` bytes

## QEMU 结果

- `TEXT_RECT: 0.956 ms`
- `EXTERN_TEXT_RECT: 0.973 ms`
- `IMAGE_565_1: 1.517 ms`
- `IMAGE_RESIZE_565_1: 3.916 ms`
- `IMAGE_ROTATE_565_1: 5.078 ms`
- `MASK_RECT_FILL_CIRCLE: 1.333 ms`
- `MASK_RECT_FILL_IMAGE: 0.612 ms`
- `MASK_IMAGE_CIRCLE: 3.219 ms`

## 验证

- `python scripts/code_perf_check.py --profile cortex-m3 --with-scenes`
- `make clean; make all APP=HelloUnitTest PORT=pc_test; output\main.exe`
- `python scripts/code_runtime_check.py --app HelloPerformance --timeout 120 --keep-screenshots`
- `python scripts/code_runtime_check.py --app HelloBasic --app-sub mask --timeout 120 --keep-screenshots`
- `python scripts/perf_to_doc.py`

## 渲染检查

- `TEXT_RECT` 与 `EXTERN_TEXT_RECT` 仍保持多行文本
- `IMAGE_565_1` 场景已变为真实透明的 star 图，不再是 test_perf 伪 alpha 图
- `MASK_RECT_FILL_IMAGE`、`MASK_IMAGE_IMAGE`、`MASK_IMAGE_CIRCLE` 场景渲染正常
- `doc/source/performance/images/perf_scenes.png` 已更新，可直接对照性能数据与场景效果
