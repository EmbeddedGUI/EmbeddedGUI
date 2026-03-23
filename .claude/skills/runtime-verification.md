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
   要求退出码为 0。

3. **运行时检查**
   ```bash
   python scripts/code_runtime_check.py --app {APP} --keep-screenshots
   ```
   要求最终输出 `ALL PASSED`。

4. **截图确认**
   检查 `runtime_check_output/{APP}/default/frame_*.png`：
   - 不能黑屏或空白
   - 控件可见，布局合理
   - 文字和图标可读

## HelloBasic 子应用检查

```bash
python scripts/code_runtime_check.py --app HelloBasic --app-sub {SUB} --keep-screenshots
```

截图路径：
`runtime_check_output/HelloBasic_{SUB}/default/frame_*.png`

## HelloCustomWidgets 检查

单个控件：

```bash
python scripts/code_runtime_check.py --app HelloCustomWidgets --app-sub {CATEGORY}/{WIDGET} --keep-screenshots
```

分类批量回归：

```bash
python scripts/code_runtime_check.py --app HelloCustomWidgets --category {CATEGORY} --bits64
```

截图路径：
`runtime_check_output/HelloCustomWidgets_{CATEGORY}_{WIDGET}/default/frame_*.png`

说明：
- `code_runtime_check.py` 会把 `APP_SUB` 里的 `/` 和 `\` 统一替换为 `_`
- 分类批量回归适合和 `custom-widgets-check.yml`、本地收口回归保持一致

## 多页面应用检查要求

- 必须检查所有页面对应截图，不允许只看首页。
- 如果录制动作里通过 `uicode_switch_page()` 切页，需要逐帧确认每页都有正确渲染。

## 可选：手动多分辨率验证（布局改动/新控件建议执行）

`code_runtime_check.py` 当前没有 `--multi-resolution` 参数。需要手动改分辨率编译并录制：

```bash
make clean
make all APP={APP} PORT=pc USER_CFLAGS="-DEGUI_CONFIG_SCEEN_WIDTH=320 -DEGUI_CONFIG_SCEEN_HEIGHT=240"
output\main.exe output\app_egui_resource_merge.bin --record runtime_check_output/{APP}/manual_320x240 2 30 --speed 1
```

可按同样方式测试 `240x320`、`320x320`、`480x320`。

## 失败处理

1. 编译失败：先修复编译错误再重跑
2. 运行崩溃/超时：排查空指针、越界、死循环、线程同步
3. 渲染异常：排查布局参数、父子裁剪、资源生成和颜色/透明度设置
4. 修复后，重复完整流程直到通过

## 常见渲染异常对照

| 症状 | 常见原因 | 修复方向 |
|------|----------|----------|
| 黑屏 | init/渲染流程未触发，或 PFB 配置不合理 | 检查初始化和 `app_egui_config.h` |
| 图标空白 | alpha 图未设置渲染色 | 设置 `egui_view_image_set_image_color()` |
| 文本乱码/方框 | 字符未进字体子集 | 更新文本提取并重新生成资源 |
| 子控件不可见 | 父容器裁剪或尺寸不足 | 调整父容器尺寸/子控件位置 |
| 动画不动 | 动画未启动或回调未执行 | 检查 `egui_animation_start()`/定时器逻辑 |

## 文件参考

| 文件 | 说明 |
|------|------|
| `scripts/code_runtime_check.py` | 运行时验证主脚本 |
| `porting/pc/sdl_port.c` | 录制与截图输出机制 |
| `runtime_check_output/` | 运行截图输出目录 |
