# 图像生成说明

嵌入式项目中，要显示一个图片，最简单的做法就是将一个图片的RGB信息保存在数组中， 然后再将数组传到显存中显示。但是因为图像所占用的资源最大，如果这样直接弄会涉及几个问题。

1. 显示和图片资源不匹配，显示是RGB565，图片是RGB888，如果直接将图片放进来，存储资源大了1/2，此外还需要将像素点做格式变化，占用计算资源。
2. Alpha通道，JPG图片是没有Alpha通道的，但是PNG图片有，需要对2者进行区分。此外直接用8bit的Alpha通道也很浪费资源，实际项目一般考虑用4bit就可以。这样存储资源可以减少1/2。
3. 格式转换，嵌入式项目只支持RGB像素点，如果直接存PNG或者JPG，其不像bmp直接是RGB像素点，所以需要通过脚本将文件翻译为像素矩阵。

为了解决上述问题，项目提供了`img2c.py`脚本来完成图像资源转换。



## img2c.py

为了解决上述问题，支持如下参数配置，通过调整参数可以满足所有项目需要。

![image-20241227222311100](https://markdown-1306347444.cos.ap-shanghai.myqcloud.com/img/image-20241227222311100.png)



## 内部资源

只会生成一个`egui_res_image_xxxxx.c`文件，里面保持了所有图片信息，生成的内部资源包括，

`egui_res_image_xxxxx_alpha_buf`，透明通道数组。

`egui_res_image_xxxxx_data_buf`，RGB像素点数组。

`egui_res_image_xxxxx_info`，图片配置信息。

`egui_image_std_t egui_res_image_xxxxx`，结构体。

![image-20241227222514952](https://markdown-1306347444.cos.ap-shanghai.myqcloud.com/img/image-20241227222514952.png)

![image-20241227222727409](https://markdown-1306347444.cos.ap-shanghai.myqcloud.com/img/image-20241227222727409.png)

![image-20241227222749250](https://markdown-1306347444.cos.ap-shanghai.myqcloud.com/img/image-20241227222749250.png)





## 外部资源

会生成3个文件，`egui_res_image_xxxxx_bin.c`文件，里面保持了图片配置信息；`egui_res_image_xxxxx_alpha.bin`透明通道二进制文件，也就是`egui_res_image_xxxxx_alpha_buf`；`egui_res_image_xxxxx_data.bin`RGB像素点二进制文件，也就是`egui_res_image_xxxxx_data_buf`。生成的外部资源包括，

`egui_res_image_xxxxx_bin_info`，图片配置信息。

`egui_image_std_t egui_res_image_xxxxx_bin`，结构体。

**注意**，为了方便管理，有外部资源需求，最好用`app_resource_config.json`统一管理。

![image-20241227223017199](https://markdown-1306347444.cos.ap-shanghai.myqcloud.com/img/image-20241227223017199.png)



![image-20241227223143798](https://markdown-1306347444.cos.ap-shanghai.myqcloud.com/img/image-20241227223143798.png)



## 图片格式选择指南

项目支持三种像素格式，选择合适的格式对存储和显示效果至关重要。

### RGB565（16-bit）

- 每像素 2 字节，无透明通道时最省空间
- 适合大多数嵌入式 LCD（SPI 接口通常使用 RGB565）
- 色彩精度：R5G6B5，共 65536 色
- 推荐场景：背景图、照片、不需要透明度的图标

### RGB32（32-bit ARGB8888）

- 每像素 4 字节，自带 8-bit Alpha 通道
- 色彩精度最高，适合需要高质量透明效果的场景
- 存储占用是 RGB565 的 2 倍
- 推荐场景：需要精细透明度的 UI 元素、32-bit 色深的显示屏

### Gray8（8-bit 灰度）

- 每像素 1 字节，仅灰度信息
- 存储最省，适合单色或灰度图标
- 推荐场景：状态图标、简单的 UI 装饰元素

### Alpha 通道位深选择

| Alpha 位深 | 透明度级别 | 存储开销 | 适用场景 |
|------------|-----------|---------|---------|
| 0 | 无透明 | 0 | 矩形背景图、照片 |
| 1 | 2 级（透明/不透明） | 1/8 像素 | 简单形状的图标 |
| 2 | 4 级 | 1/4 像素 | 低精度抗锯齿图标 |
| 4 | 16 级 | 1/2 像素 | 大多数 UI 图标（推荐） |
| 8 | 256 级 | 1 像素 | 高质量渐变透明效果 |

实际项目中，Alpha 4-bit 是最佳平衡点：透明度效果足够好，存储开销仅为 8-bit 的一半。

## 图片压缩策略

### 尺寸优化

在 `app_resource_config.json` 中使用 `dim` 参数缩放图片：

```json
{
    "file": "background.png",
    "format": "rgb565",
    "alpha": "0",
    "dim": "120,160"
}
```

缩放到实际显示尺寸可以大幅减少存储占用。一张 240x320 的 RGB565 图片占 153,600 字节，缩放到 120x160 后仅需 38,400 字节。

### 格式降级

- 不需要透明度的图片，将 `alpha` 设为 `"0"`
- 对色彩要求不高的图标，考虑使用 `gray8` 格式
- Alpha 通道优先使用 4-bit 而非 8-bit

### 内外部资源分配策略

| 资源类型 | 建议存储位置 | 原因 |
|---------|------------|------|
| 主界面小图标 | 内部 | 高频访问，需要快速响应 |
| 大尺寸背景图 | 外部 | 占用空间大，加载频率低 |
| 常用字体 | 内部 | 每帧都需要访问 |
| 多语言字体 | 外部 | 按需加载，节省内部空间 |

