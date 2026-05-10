---
name: edge-tts
description: Use when generating Chinese or multilingual narration audio without an OpenAI API key, using the Python edge-tts package and Microsoft Edge online neural voices.
---

# Edge TTS

用于生成教学视频旁白。它不需要 OpenAI API key，但依赖 Microsoft 在线 TTS 服务和网络连接。

## 安装

```powershell
python -m pip install edge-tts
```

查看中文声音：

```powershell
python -m edge_tts --list-voices | Select-String -Pattern "zh-CN"
```

常用声音：

| voice | 风格 |
|------|------|
| `zh-CN-XiaoxiaoNeural` | 女声，温和，适合默认中文教程 |
| `zh-CN-YunyangNeural` | 男声，正式、新闻感 |
| `zh-CN-YunxiNeural` | 男声，轻快 |
| `en-US-JennyNeural` | 英文女声 |
| `en-US-GuyNeural` | 英文男声 |

## 生成旁白

后续 EmbeddedGUI 视频默认只生成音频，不生成字幕：

```powershell
python -m edge_tts `
  --voice zh-CN-XiaoxiaoNeural `
  --rate +12% `
  --text "欢迎来到 EmbeddedGUI 教学视频。" `
  --write-media .\audio\intro.mp3
```

检查时长：

```powershell
ffprobe -v error -show_entries format=duration -of default=nw=1:nk=1 .\audio\intro.mp3
```

只有用户明确要求字幕时，才额外增加：

```powershell
--write-subtitles .\subtitles\intro.vtt
```

## 使用约束

- 音频时长超过场景时，优先缩短文案，不靠长停顿拖场景。
- 中文教程常用 `--rate +8%` 到 `+18%`。
- 不把字幕作为默认输出；视频画面默认无字幕。
- 生成后更新 `index.html` 中对应 `<audio data-duration="...">`。
