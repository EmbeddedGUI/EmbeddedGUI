---
name: runtime-verification
description: Use when verifying code changes compile and render correctly, or when diagnosing rendering issues like black screens, missing widgets, or layout problems
---

# Runtime Verification Skill

After writing or modifying code, AI MUST complete the verification workflow below before considering the task done. Compilation alone is NOT sufficient.

## Verification Levels

### Level 1: Basic Verification (ALL code changes)

Every code change MUST pass these steps:

1. **Compile check** (PC port):
   ```bash
   make clean APP={APP} && make all APP={APP} PORT=pc BITS=64
   ```

2. **Unit tests** (headless):
   ```bash
   make clean APP=HelloUnitTest && make all APP=HelloUnitTest PORT=pc_test BITS=64 && output\main.exe
   ```
   Exit code must be 0.

3. **Runtime check** (default resolution):
   ```bash
   python scripts/code_runtime_check.py --app {APP} --bits64 --keep-screenshots
   ```
   Must print `ALL PASSED`.

4. **Visual verification**: Use the Read tool to read the screenshot BMP files printed by the script. Confirm:
   - UI elements render correctly (not black/corrupted screen)
   - Layout looks reasonable
   - Text/icons are readable

### Level 2: Multi-Resolution Verification (NEW widgets/controls)

When creating a new widget or a new example that demonstrates a widget, ALL of Level 1 plus:

5. **Multi-resolution runtime check**:
   ```bash
   python scripts/code_runtime_check.py --app {APP} --multi-resolution --bits64 --keep-screenshots
   ```
   Must print `ALL PASSED` for all 4 resolutions.

6. **Visual verification of all resolutions**: Read each screenshot BMP and verify:
   - 240x320 (3:4 Portrait): Layout fits vertically
   - 320x240 (4:3 Landscape): Layout adapts to horizontal space
   - 320x320 (1:1 Square): No overflow or severe distortion
   - 480x320 (3:2 Wide): No excessive stretching

   Screenshots are saved as PNG (auto-converted from BMP for AI readability):
   ```
   runtime_check_output/{AppName}/portrait_3_4/frame_0000.png
   runtime_check_output/{AppName}/landscape_4_3/frame_0000.png
   runtime_check_output/{AppName}/square_1_1/frame_0000.png
   runtime_check_output/{AppName}/wide_3_2/frame_0000.png
   ```

7. If any resolution shows display issues, fix the code and re-run verification.

## When to Use Each Level

| Scenario | Level |
|----------|-------|
| Bug fix in existing widget | Level 1 |
| Modify existing widget behavior | Level 1 |
| Add new feature to existing widget | Level 1 |
| Create new widget/control | **Level 2** |
| Create new example app | **Level 2** |
| Modify layout logic | **Level 2** |
| Change core rendering code | **Level 2** |

## Pass Criteria

### Automatic checks (script):
- Process exits with code 0 (no crash)
- BMP frame files are generated
- Frame file size is reasonable (not empty/corrupted)

### Visual checks (AI reads screenshots):
- UI is not a blank/black screen
- Widgets are visible and positioned correctly
- No obvious rendering artifacts (garbage pixels, misaligned elements)
- Text is legible (if applicable)
- Layout adapts reasonably to different aspect ratios (Level 2)

## Failure Handling

If verification fails:
1. Read the error message from the script output
2. If compile failure: fix code, re-compile
3. If runtime crash (non-zero exit): check for null pointer, array bounds, stack overflow
4. If timeout: check for infinite loops or deadlocks
5. If visual issues: fix layout/rendering code
6. Re-run the full verification after fixes

## HelloBasic Sub-App Testing

For HelloBasic sub-applications:
```bash
python scripts/code_runtime_check.py --app HelloBasic --app-sub {SUB} --bits64 --keep-screenshots
python scripts/code_runtime_check.py --app HelloBasic --app-sub {SUB} --multi-resolution --bits64 --keep-screenshots
```

## Full Regression Check

To verify all examples pass runtime check:
```bash
python scripts/code_runtime_check.py --full-check --bits64
```

## Multi-Resolution Mechanism

Screen dimensions are overridden via `USER_CFLAGS` in the Makefile:
```bash
make all APP=xxx PORT=pc USER_CFLAGS="-DEGUI_CONFIG_SCEEN_WIDTH=320 -DEGUI_CONFIG_SCEEN_HEIGHT=240"
```

PFB sizes auto-adjust via `SCREEN/8` formula. All test resolutions (240x320, 320x240, 320x320, 480x320) are divisible by 8.

## Files Reference

| File | Description |
|------|-------------|
| `scripts/code_runtime_check.py` | Runtime verification automation script |
| `porting/pc/Makefile.base` | `USER_CFLAGS` support for resolution overrides |
| `porting/pc/sdl_port.c` | Recording mechanism (--record flag) |
| `runtime_check_output/` | Screenshot output directory |

## 渲染问题排查决策树

截图验证发现问题时，按以下顺序排查：

```
黑屏/无输出？
├── 是 → 检查 PFB 配置（宽高必须是屏幕尺寸的整数约数）
│        检查 draw 函数是否注册
│        检查 egui_init() 是否调用
└── 否 → 元素缺失？
         ├── 是 → 控件未添加到父容器？
         │        控件尺寸为0？
         │        控件被父容器裁剪？（子控件超出父边界）
         │        资源未生成？（undefined reference）
         └── 否 → 布局错乱？
                  ├── 是 → x/y/width/height 计算错误？
                  │        LinearLayout/GridLayout 子控件 margin 设置？
                  │        父容器尺寸不够容纳所有子控件？
                  └── 否 → 颜色/透明度异常？
                           ├── alpha 格式图片缺少 image_color 设置
                           ├── 颜色值 RGB 顺序错误（EGUI 用 0xRRGGBB）
                           └── alpha 值设为 EGUI_ALPHA_0（完全透明）
```

## 常见渲染异常对照表

| 症状 | 原因 | 修复 |
|------|------|------|
| 完全黑屏 | PFB 尺寸配置错误或 init 未调用 | 检查 `app_egui_config.h` 中 PFB_WIDTH/HEIGHT |
| 图标空白 | alpha 格式图片未设置渲染颜色 | 调用 `egui_view_image_set_image_color()` |
| 图标被裁剪 | 图片尺寸≠控件尺寸且未启用缩放 | XML 中设置 `image_resize="true"` |
| 文字显示方框 | 字符不在字体字符集中 | 将字符添加到 `supported_text.txt` 并重新生成资源 |
| 文字底部截断 | Label 高度 < 字体行高 | 参考 html-to-egui skill 中的字体行高表 |
| 控件重叠 | 绝对定位坐标冲突 | 检查 x/y 坐标，或改用 LinearLayout 自动排列 |
| 子控件不可见 | 超出父容器边界被裁剪 | 增大父容器尺寸或调整子控件位置 |
| 背景色不显示 | Card/Group 未添加 Background 子元素 | 添加 `<Background>` 子元素 |
| 触控区域偏移 | 控件实际位置与视觉位置不一致 | 检查父容器的偏移是否正确传递 |
| 动画不播放 | 动画未启动或插值器配置错误 | 确认 `egui_animation_start()` 已调用 |

## 截图对比分析方法

1. 运行时检查生成截图后，用 Read 工具读取 PNG 文件直接查看
2. 多页面应用需检查每一帧截图（`frame_0000.png`, `frame_0001.png`, ...）
3. 对比设计稿时，可用 `scripts/figmamake/figma_visual_compare.py` 生成差异图：
   ```bash
   python scripts/figmamake/figma_visual_compare.py \
       --design mockup.png \
       --rendered runtime_check_output/{APP}/default/frame_0000.png \
       --output comparison.png
   ```
4. 差异图中红色/亮色区域表示不匹配，重点排查这些区域对应的控件
