---
name: hyperframes-cli
description: Use when validating, inspecting, previewing, rendering, and troubleshooting HyperFrames video projects for EmbeddedGUI tutorial videos.
---

# HyperFrames CLI

EmbeddedGUI 教学视频默认使用 HyperFrames CLI 与本地 ffmpeg。为保证复现性，优先固定版本：

```powershell
npx --yes hyperframes@0.4.43 <command>
```

## 常规流程

在 `output/video/<video_name>/` 下执行：

```powershell
npx --yes hyperframes@0.4.43 lint
npx --yes hyperframes@0.4.43 inspect --at 8,52,88,116,150,168 --json
npx --yes hyperframes@0.4.43 render --quality draft --workers 1 --output <video_name>_layout_check.mp4
npx --yes hyperframes@0.4.43 render --quality high --workers 1 --output <video_name>_final.mp4
```

质量约定：

| 阶段 | 命令 |
|------|------|
| 布局检查 | `lint` + `inspect` |
| 快速验收 | `render --quality draft --workers 1` |
| 最终交付 | `render --quality high --workers 1` |

`composition_file_too_large` 是非阻塞 warning；只要 `lint/inspect` 没有实质布局 error，可以继续 draft/final。

## Inspect

`inspect` 用于检查文字越界、容器溢出、画布外内容等问题：

```powershell
npx --yes hyperframes@0.4.43 inspect --at 6,28,52,74,98,121,145,170 --json
```

取点原则：

- 封面 1 张。
- 每个关键图解 1 张。
- 终端/模拟器页面 1 到 2 张。
- 最近修改的场景必须覆盖。

## Proof Frames

draft 和 final 都要抽关键帧。示例：

```powershell
ffmpeg -y -ss 00:00:06 -i .\<video>.mp4 -frames:v 1 -update 1 .\proof_frames_revised\check_006_cover.png
ffmpeg -y -ss 00:00:52 -i .\<video>.mp4 -frames:v 1 -update 1 .\proof_frames_revised\check_052_diagram.png
```

人工检查：

- 文本清晰，没有越界、截断、互相覆盖。
- 默认没有字幕；如果用户明确要求字幕，字幕必须底部小字且不挡主体。
- 转场没有长停顿、空白帧、冻结帧。
- 命令、路径、公式与仓库实际一致。
- 右侧截图/图解没有被裁掉或放到全屏遮住其他内容。

## Final Metadata

最终 MP4 用 `ffprobe` 验证：

```powershell
ffprobe -v error -show_entries format=duration,size `
  -show_entries stream=index,codec_type,codec_name,width,height,avg_frame_rate,duration,channels,sample_rate,nb_frames `
  -of json .\<video_name>_final.mp4
```

期望：

- duration：按脚本目标时长。
- video：H.264，1920x1080，30fps。
- audio：AAC，48000Hz，stereo。

## 常见处理

- CDN 资源超时：把依赖复制到 `assets/`，例如 `assets/gsap.min.js`。
- `inspect` 超时但 `lint` 通过：先渲染 draft，再抽 proof frames 人工检查。
- 字体或文字问题：先改 HTML/CSS 静态布局，再重新 inspect/render。
- 渲染机器压力大：保持 `--workers 1`。
