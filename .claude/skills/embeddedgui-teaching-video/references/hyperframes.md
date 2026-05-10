---
name: hyperframes
description: Use when authoring HyperFrames HTML video compositions, scenes, media tracks, GSAP animations, overlays, diagrams, and transitions for EmbeddedGUI tutorial videos.
---

# HyperFrames

HyperFrames 视频以 HTML 为源文件，通常是 `output/video/<video_name>/index.html`。一个 composition 由 `data-*` 时间属性、CSS 布局、媒体轨道和 GSAP timeline 组成。

## 基本结构

顶层 `index.html` 不能使用 `<template>` 包裹 composition：

```html
<body>
  <div data-composition-id="main" data-width="1920" data-height="1080">
    <div id="scene-intro" data-start="0" data-duration="12" data-track-index="0">
      ...
    </div>
    <audio id="audio-intro" data-start="0" data-duration="10.2" data-track-index="10" src="audio/intro.mp3"></audio>
  </div>
  <script src="assets/gsap.min.js"></script>
  <script>
    window.__timelines = window.__timelines || {};
    const tl = gsap.timeline({ paused: true });
    window.__timelines["main"] = tl;
  </script>
</body>
```

关键属性：

| 属性 | 说明 |
|------|------|
| `data-composition-id` | composition id，必须和 `window.__timelines` key 一致 |
| `data-width` / `data-height` | 推荐 1920x1080 |
| `data-start` | clip 开始时间，秒 |
| `data-duration` | clip 持续时间，秒 |
| `data-track-index` | 时间轨道，同轨道 clip 不要重叠 |

音频永远使用单独 `<audio>`。如果有视频素材，视频元素要 `muted playsinline`，声音另走 `<audio>`。

## 布局优先

先写每个场景最清楚的静态画面，再添加动画：

- `.scene-content` 用 `width: 100%; height: 100%; box-sizing: border-box;`。
- 内容容器优先用 grid/flex 和 padding，不用硬编码绝对定位堆叠正文。
- 右侧截图/图解不铺满屏幕，留出标题、边框和呼吸空间。
- 文本、按钮、标记块不能越界、贴边或互相覆盖。
- 不使用 `<br>` 强制换行，除非是短标题的刻意排版；正文用 `max-width` 自然换行。

EmbeddedGUI 教学视频默认结构：

```css
.scene-body {
  display: grid;
  grid-template-columns: 0.9fr 1.1fr;
  gap: 48px;
  align-items: center;
}

.bullet-list > div {
  display: grid;
  grid-template-columns: 34px 1fr;
  gap: 12px;
  align-items: center;
}
```

## 转场节奏

- 多场景视频要有转场或明确的短切换，但不能靠长时间停顿过渡。
- 默认 0.3 到 0.8 秒短交叠、wipe、fade-through 或快速 cut。
- 旁白结束后不要冻结好几秒等待下一页。
- 不出现空白帧、黑屏过渡或无意义慢转场，除非用户明确要求。

## 字幕默认关闭

后续 EmbeddedGUI 视频默认不渲染字幕，不生成字幕轨道。只有用户明确要求字幕时，才按底部小字处理，不能遮挡主体内容。

## HyperFrames 硬规则

- 不使用 `Math.random()`、`Date.now()` 或运行时非确定逻辑。
- timeline 必须同步创建，不能放在 `async`、`setTimeout`、Promise 回调里。
- timeline 使用 `{ paused: true }`，由 HyperFrames 控制播放。
- 注册 `window.__timelines["<composition-id>"] = tl`。
- 不用 `repeat: -1`，循环次数必须按视频时长计算为有限值。
- 不直接调用 `video.play()`、`audio.play()`、`pause()`、`seek()`。
- 避免同时在多个 timeline 动画同一元素的同一属性。
- 优先动画 `opacity`、`x`、`y`、`scale`、`rotation`、`backgroundColor` 等视觉属性。

## 资源路径

- 旁白：`audio/*.mp3`
- 图解、截图、logo、录制：`assets/*`
- 证明帧：`proof_frames_revised/*.png`
- GSAP：优先 `assets/gsap.min.js`，避免 CDN 渲染不稳定。
