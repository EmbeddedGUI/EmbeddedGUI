---
name: imagegen
description: Use when an EmbeddedGUI tutorial video needs generated visual assets such as covers, diagrams, illustrations, or edited screenshots. Prefer deterministic SVG/HTML/Canvas for exact technical diagrams; use AI bitmap generation only when explicitly useful and credentials are available.
---

# Image Generation

用于教学视频中的封面、原理图、流程图、场景图和位图素材。

## 默认策略

EmbeddedGUI 技术视频优先使用可复现的确定性素材：

- 精确公式、函数名、流程图：优先生成 SVG、HTML/CSS 或 Canvas，再导出 PNG。
- 网站、终端、模拟器流程：优先使用真实截图或录屏帧。
- 封面背景、抽象技术氛围、插画：可以使用 AI 位图生成，但必须检查文字和 logo。

涉及命令、代码、公式、函数名时，不依赖 AI 图片里的文字；文字应由 SVG/HTML/CSS 直接排版。

## 输出位置

```text
output/video/<video_name>/assets/
output/video/<video_name>/proof_frames_revised/
```

建议素材命名稳定，例如：

```text
assets/cover_<topic>.svg
assets/diagram_dirty_layers.svg
assets/gitee_download_screenshot.png
proof_frames_revised/draft_052_diagram.png
```

## 确定性 SVG/HTML 图解

适合：

- PFB/dirty/view/canvas/virtual/focus 原理图。
- 内存计算、bpp、矩阵、pipeline。
- 函数分层、调用链、构建运行流程。

检查项：

- 中文不越界。
- 函数名和说明不互相覆盖。
- 公式逐项正确。
- 颜色在深色背景上清晰。
- 右侧图解不铺满屏幕。

## AI 位图生成

只有在用户需要 AI 生成封面、背景、插画、概念图时使用。若使用 OpenAI Image API：

- 本地必须设置 `OPENAI_API_KEY`，不要把 key 写进仓库或聊天。
- 输出放到 `output/video/<video_name>/assets/`。
- 生成后必须人工打开检查，不合格就小步迭代。
- 不让 AI 图片承担精确文字、公式或代码展示。

如果没有可用 key 或用户不希望使用 OpenAI，改用确定性 SVG/HTML/Canvas、本地截图、已有素材。

## Prompt 要点

生成图时 prompt 保持短而明确：

```text
Use case: infographic-diagram
Asset type: EmbeddedGUI tutorial diagram
Primary request: explain dirty region passthrough pipeline
Style: dark engineering UI, cyan lime yellow accents, clean vector-like layout
Constraints: no extra text beyond provided labels, no watermark, no random logos
```

封面风格必须跟视频正文一致：官方 logo、深色工程背景、青绿/黄强调色、标题层级一致。
