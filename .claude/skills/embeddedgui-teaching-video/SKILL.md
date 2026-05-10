---
name: embeddedgui-teaching-video
description: Use when creating or revising EmbeddedGUI tutorial videos with HyperFrames, local ffmpeg, Edge TTS narration, screenshots, generated diagrams, proof frames, and final MP4 export. Subtitles are off by default unless explicitly requested.
---

# EmbeddedGUI Teaching Video

用于制作 EmbeddedGUI 系列教学视频。视频工程默认放在 `output/video/<video_name>/`，使用 HyperFrames + 本地 ffmpeg + Edge TTS，不依赖 Sora，TTS 不需要 OpenAI API key。

## 关联参考

本 skill 包会按需读取以下同目录参考文件：

| 文件 | 用途 |
|------|------|
| `references/hyperframes.md` | 编写 `index.html`、场景结构、动画、转场和媒体轨道 |
| `references/hyperframes-cli.md` | `lint`、`inspect`、`render`、`ffprobe`、proof frames |
| `references/edge-tts.md` | 中文旁白 MP3 生成，默认不生成字幕 |
| `references/gsap.md` | HyperFrames 中的 GSAP timeline、entrance、短转场和动画细节 |
| `references/imagegen.md` | 需要生成封面、图解、位图素材时使用；优先确定性 SVG/HTML/Canvas |
| `build-and-debug.md`、`runtime-verification.md`、`resource-generation.md` | 需要真实命令、运行截图、资源链路证据时读取 |

如果本地用户目录也存在同名 Codex skills，优先使用本仓库 `.claude/skills/embeddedgui-teaching-video/` 下的版本，保证流程跟随仓库。

## 默认约束

- 视频工程放在 `output/video/<video_name>/`。
- 官方 logo 使用 `doc/source/images/embeddedgui-ai-pfb-dirty-flat-logo.svg`。
- 视觉保持系列统一：深色工程背景、官方 logo、青绿/黄强调色、右侧截图或图解不铺满屏幕。
- 后续视频默认不加字幕，不生成 `subtitles/`，也不在画面里渲染字幕。只有用户明确要求字幕时才加，并且必须小字放底部。
- 转场保持紧凑，不用长时间静止、空白停顿或过慢过渡。默认快速切换或 0.3 到 0.8 秒短交叠。
- 封面与正文场景保持同一风格，不做脱离系列的单独海报风。
- 命令、公式、文件名必须从仓库实际内容确认；用户给出的技术修正直接体现在最终内容里，不显示“修正说明”。
- 右侧图解、截图、终端、模拟器画面不能被左侧文本或字幕挡住。
- 优先使用本地 `assets/gsap.min.js`，避免 CDN 超时导致渲染不稳定。
- `composition_file_too_large` 是非阻塞 warning；只有影响维护时再拆分子 composition。

## 推荐工程结构

```text
output/video/<video_name>/
  index.html
  design.md
  hyperframes.json
  assets/
  audio/
  scripts/
    segments.json
    narration.txt
  proof_frames_revised/
  subtitles/          # 可选，仅用户明确要求字幕时创建
```

## 制作流程

1. **收集输入**
   - 主题、受众、视频时长、必须出现的命令或例程。
   - 真实截图、终端录制、模拟器录制、源码片段、logo。
   - 需要讲解的核心机制，例如 PFB、dirty、view、canvas、virtual、focus。

2. **脚本与分镜**
   - 创建 `scripts/segments.json`，每个场景包含 `id`、`start`、`duration`、`voice`、`text`。
   - 旁白要短，中文教程一般用 `zh-CN-XiaoxiaoNeural`，语速可用 `+8%` 到 `+18%`。
   - 音频时长超过场景时，优先删减旁白，不靠拉长停顿解决。

3. **生成旁白**
   - 使用 `references/edge-tts.md`。
   - 默认只生成 MP3，不生成 VTT，不在画面中添加字幕。
   - 更新 `index.html` 中对应 `<audio data-duration="...">`。

4. **准备视觉素材**
   - 封面、流程图、原理图优先用确定性 SVG/HTML/Canvas 生成，保证文字和公式准确。
   - 网站下载流程、命令行流程、运行效果尽量使用真实截图或真实录制。
   - 需要 AI 位图素材时再读 `references/imagegen.md`。

5. **合成 HyperFrames**
   - 先做静态 hero frame，确认布局正确，再加动画。
   - 左侧讲解文本、右侧图解/截图/模拟器是默认结构。
   - 每页文本和方块标记要垂直居中；文字不能越界或互相覆盖。
   - 转场短促，避免旁白结束后长时间停住。

6. **验证**
   - 使用 `references/hyperframes-cli.md` 执行 `lint` 和 `inspect`。
   - 渲染 draft，抽 proof frames，人工检查遮挡、越界、文字、公式、节奏。
   - 最终高质量渲染后用 `ffprobe` 验证音视频流。

## 常用命令

```powershell
npx --yes hyperframes@0.4.43 lint
npx --yes hyperframes@0.4.43 inspect --at 8,52,88,116,150,168 --json
npx --yes hyperframes@0.4.43 render --quality draft --workers 1 --output <video_name>_layout_check.mp4
npx --yes hyperframes@0.4.43 render --quality high --workers 1 --output <video_name>_final.mp4

ffprobe -v error -show_entries format=duration,size `
  -show_entries stream=index,codec_type,codec_name,width,height,avg_frame_rate,duration,channels,sample_rate,nb_frames `
  -of json .\<video_name>_final.mp4
```

抽 proof frame：

```powershell
ffmpeg -y -ss 00:00:52 -i .\<draft_or_final>.mp4 -frames:v 1 -update 1 .\proof_frames_revised\check_052.png
```

## 交付前检查

- `lint` 无 error。
- `inspect` 无布局 error；如只有 `composition_file_too_large`，记录为非阻塞。
- proof frames 覆盖封面、关键图解、终端/模拟器、最近修改场景。
- 无默认字幕；如用户明确要求字幕，字幕必须底部小字且不挡主体。
- 转场没有长停顿、空白帧、无意义冻结。
- `ffprobe` 确认最终 MP4 为 1920x1080、30fps、H.264 + AAC。
