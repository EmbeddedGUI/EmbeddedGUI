# Image 控件演示

## 应用说明

该示例包含 3 个分页：
1. `test.png` 生成的 `RGB565 + alpha=0` 图片，展示 `STD / QOI / RLE`
2. `star.png` 生成的 `RGB565 + alpha=8` 图片，展示 `STD / QOI / RLE`
3. 运行时 SVG 示例，展示 `path / evenodd / group` 三类常见子集能力

`HelloBasic/image` 不再比较 SVG 的编译期位图化实现，也不再展示 `SVG STD / SVG QOI / SVG RLE` 这类链路。

## 资源来源

- 位图源文件位于 `example/HelloBasic/image/resource/src/`
- 生成命令：`make resource_refresh APP=HelloBasic APP_SUB=image PORT=pc`

## 录制动作

录制模式会自动覆盖 3 个页面：

1. 首屏等待
2. 左滑到 PNG alpha=8 页
3. 再左滑到运行时 SVG 页
