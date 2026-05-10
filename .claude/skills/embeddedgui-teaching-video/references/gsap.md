---
name: gsap
description: Use when writing GSAP timelines for HyperFrames videos, including entrances, short transitions, easing, stagger, labels, finite loops, and performance-safe transforms.
---

# GSAP

HyperFrames 通过 GSAP timeline 控制动画。timeline 必须同步创建、暂停，并注册到 `window.__timelines`。

```html
<script src="assets/gsap.min.js"></script>
<script>
  window.__timelines = window.__timelines || {};
  const tl = gsap.timeline({ paused: true, defaults: { ease: "power2.out" } });

  tl.from(".title", { y: 48, opacity: 0, duration: 0.45 }, 0);
  tl.from(".diagram", { x: 56, opacity: 0, duration: 0.5 }, 0.15);

  window.__timelines["main"] = tl;
</script>
```

## 常用方法

| 方法 | 用途 |
|------|------|
| `gsap.to(target, vars)` | 从当前状态动画到目标状态 |
| `gsap.from(target, vars)` | 从指定状态进入当前 CSS 状态，适合入场 |
| `gsap.fromTo(target, fromVars, toVars)` | 显式指定起止状态 |
| `gsap.set(target, vars)` | 立即设置属性；在 HyperFrames 中优先用 `tl.set(..., time)` |

## 时间位置

第三个参数控制 tween 位置：

```javascript
tl.from(".a", { y: 40, opacity: 0, duration: 0.4 }, 0);
tl.from(".b", { y: 24, opacity: 0, duration: 0.35 }, "<0.1");
tl.to(".wipe", { scaleX: 1, duration: 0.45 }, "scene2-=0.4");
```

常用写法：

- `0`：绝对 0 秒。
- `"<"`：与上一个动画同时开始。
- `"<0.1"`：比上一个动画晚 0.1 秒开始。
- `"+=0.3"`：上一个动画结束后再等 0.3 秒。
- `"label"` / `"label+=0.2"`：按标签定位。

## EmbeddedGUI 视频动画约束

- 转场短促，通常 0.3 到 0.8 秒，不制造长停顿。
- 入场动画要帮助理解，不要让内容慢慢等出来。
- 旁白结束后不要让画面冻结很久。
- 默认无字幕，不需要给字幕写动画。
- 优先动画 `opacity`、`x`、`y`、`scale`、`rotation`、`backgroundColor`。
- 避免动画 `width`、`height`、`top`、`left`，能用 transform 就用 transform。

## HyperFrames 硬规则

- timeline 使用 `{ paused: true }`。
- 注册 key 必须等于 `data-composition-id`。
- 不调用 `tl.play()` 作为渲染入口。
- 不在 `async`、`setTimeout`、Promise、事件回调里创建 timeline。
- 不使用 `repeat: -1`；需要循环时按场景时长计算有限 repeat。
- 不动画 `display`、`visibility`，也不手动 `video.play()` 或 `audio.play()`。
- 不让多个 timeline 同时控制同一元素的同一属性。

## 有限循环示例

```javascript
const cycle = 2.4;
const visible = 18;
tl.to(".pulse", {
  scale: 1.04,
  opacity: 0.85,
  duration: cycle / 2,
  repeat: Math.ceil(visible / cycle) - 1,
  yoyo: true,
  ease: "sine.inOut",
}, 0);
```
