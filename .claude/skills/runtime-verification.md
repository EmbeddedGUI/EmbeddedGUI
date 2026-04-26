---
name: runtime-verification
description: Use when verifying code changes compile and render correctly, or when diagnosing rendering issues like black screens, missing widgets, or layout problems
---

# Runtime Verification Skill

代码修改后，必须执行运行验证。只通过编译不算完成。

## 必做流程（所有代码变更）

1. **编译检查（PC）**
   ```bash
   make clean
   make all APP={APP} PORT=pc
   ```

2. **单元测试**
   ```bash
   make clean
   make all APP=HelloUnitTest PORT=pc_test
   output\main.exe
   ```
   要求退出码为 `0`。

3. **运行时检查**
   ```bash
   python scripts/code_runtime_check.py --app {APP} --keep-screenshots
   ```
   要求最终输出 `ALL PASSED`。

4. **截图确认**
   检查 `runtime_check_output/{APP}/default/frame_*.png`
   - 不能黑屏或空白
   - 控件可见，布局合理
   - 文字和图标可读

## APP_SUB 子示例检查

```bash
python scripts/code_runtime_check.py --app HelloBasic --app-sub {SUB} --keep-screenshots
```

截图路径：
`runtime_check_output/HelloBasic_{SUB}/default/frame_*.png`

当前 `code_runtime_check.py` 支持 `HelloBasic`、`HelloVirtual`、`HelloSizeAnalysis` 这类 `APP_SUB` 家族应用；不传 `--app-sub` 时会批量跑该应用下所有子示例：

```bash
python scripts/code_runtime_check.py --app HelloVirtual --app-sub virtual_stage_basic --keep-screenshots
python scripts/code_runtime_check.py --app HelloSizeAnalysis --keep-screenshots
```

## 多屏专项检查

多屏相关改动后，除了单独跑示例，建议优先先跑一轮一键收口：

```bash
python scripts/release_check.py --scope multi-display
```

如果需要细查编译日志或副屏截图，再拆开补跑多屏 scope：

```bash
python scripts/code_compile_check.py --scope multi-display --case-jobs 2
python scripts/code_runtime_check.py --scope multi-display --jobs 2 --keep-screenshots
```

当前 `multi-display` scope 会覆盖：

- `HelloMultiDisplay`
- `HelloMultiDisplayHetero`

适合验证：

- descriptor 化副屏启动
- per-core GUI 线程
- 副屏输入路由
- 多屏录制动作和截图输出

## HelloCustomWidgets 已迁移

`HelloCustomWidgets` 不再属于当前主仓。当前 `scripts/code_runtime_check.py` 遇到 `--app HelloCustomWidgets` 或旧的 `--category` 参数会直接报错并提示切换仓库：

```text
https://github.com/EmbeddedGUI/EmbeddedGUI_Widgets
```

涉及 custom widgets、其截图归档、`iteration_log/` 或对应验收 workflow 时，不要在当前仓库继续执行旧命令，应切换到 `EmbeddedGUI_Widgets` 仓库后按那边的规范运行。

## 多页面应用检查要求

- 必须检查所有页面对应截图，不允许只看首页
- 如果录制动作里通过 `uicode_switch_page()` 切页，需要逐帧确认每页都有正确渲染

## 可选：手动多分辨率验证

`code_runtime_check.py` 当前没有 `--multi-resolution` 参数。需要手动改分辨率编译并录制：

```bash
make clean
make all APP={APP} PORT=pc USER_CFLAGS="-DEGUI_CONFIG_SCREEN_WIDTH=320 -DEGUI_CONFIG_SCREEN_HEIGHT=240"
output\main.exe output\app_egui_resource_merge.bin --record runtime_check_output/{APP}/manual_320x240 2 30 --speed 1
```

## 失败处理

1. 编译失败：先修复编译错误再重跑
2. 运行崩溃或超时：排查空指针、越界、死循环、线程同步
3. 渲染异常：排查布局参数、父子裁剪、资源生成和颜色/透明度设置
4. 修复后，重复完整流程直到通过

## 常见渲染异常对照

| 症状 | 常见原因 | 修复方向 |
|------|----------|----------|
| 黑屏 | init/渲染流程未触发，或 PFB 配置不合理 | 检查初始化和 `app_egui_config.h` |
| 图标空白 | alpha 图元未设置渲染色 | 设置 `egui_view_image_set_image_color()` |
| 文本乱码/方框 | 字符未进字体子集 | 更新文本提取并重新生成资源 |
| 子控件不可见 | 父容器裁剪或尺寸不足 | 调整父容器尺寸、子控件位置 |
| 动画不动 | 动画未启动或回调未执行 | 检查 `egui_animation_start()` / 定时器逻辑 |

## 文件参考

| 文件 | 说明 |
|------|------|
| `scripts/code_runtime_check.py` | 运行时验证主脚本 |
| `scripts/code_compile_check.py` | 编译检查主脚本 |
| `porting/pc/sdl_port.c` | 录制与截图输出机制 |
| `runtime_check_output/` | 运行截图输出目录 |
