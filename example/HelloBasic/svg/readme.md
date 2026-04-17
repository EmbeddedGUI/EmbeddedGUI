# HelloBasic/svg

`svg` 是 `HelloBasic` 下的 runtime SVG 动画子例程，用来演示在运行时拼装 SVG 字符串并直接交给 `egui_image_svg_load_memory()` 渲染。

当前包含 3 个可左右滑动的 page：

- `Pulse`：HUD 扫描、旋转框体、动态 telemetry bars
- `Tunnel`：透视隧道、推进感、旋转闸门
- `Orbit`：椭圆轨道、卫星绕行、底部波形

构建与运行：

```bash
make all APP=HelloBasic APP_SUB=svg
make run
```

运行时校验：

```bash
python scripts/code_runtime_check.py --app HelloBasic --app-sub svg --keep-screenshots
```

SVG 规范对比校验仍然由 `HelloSVGSpec` 负责：

```bash
python scripts/checks/svg_validation_check.py
python scripts/checks/svg_validation_check.py --with-unit
```

这个子例程的重点不是预生成位图，而是展示 SVG 在运行时也能通过少量模板和参数变化做出连续动画，并组合成多页可滑动的交互展示。

## SVG Samples

`svg_frames/` 下保留了由帧生成逻辑导出的静态 SVG 样本，方便人工查看和对照。这些文件不参与构建。

当前每个 page 提供 3 帧固定采样：

- `page 0 / Pulse`：`pulse_t000.svg`、`pulse_t012.svg`、`pulse_t024.svg`
- `page 1 / Tunnel`：`tunnel_t000.svg`、`tunnel_t012.svg`、`tunnel_t024.svg`
- `page 2 / Orbit`：`orbit_t000.svg`、`orbit_t012.svg`、`orbit_t024.svg`

这些样本由 `test.c` 中的实际帧构建逻辑导出，便于直接审阅原始 SVG 内容，而不只看截图。
