# 字体生成说明

嵌入式项目中，字体显示也是至关重要的。在字体生成时需要解决以下几个问题。

1. 显示效果，文字渲染有其特殊性，因为字体本身特殊性，不同所需显示文字其宽度和高度各不相同，尤其是西文字体，`w`和`i`其宽度完全不同，如果按照固定宽度来，显示会很丑，目前大多数都是用freetype这种矢量字体库来渲染，这样呈现效果会好很多，并且支持抗锯齿。
2. 存储资源优化，ttf文件包含很多渲染的字体，动不动就几MB，对于嵌入式存储资源要求很高。嵌入式项目都是将所需的字体和文本渲染后的字体像素矩阵保存成图片，这样单片机只需要将所需的文本图像数据保存到芯片中就行，所需的资源很小。
3. 计算资源优化，ttf文件字体渲染就是一堆曲线的计算，还要计算抗锯齿，所需计算资源很多。所以在嵌入式项目中，显示字体其实一般都不是实时演算（也就是将ttf文件放到芯片里，需要显示什么字体直接换算），芯片的计算资源不够，而且ttf解析需要大量代码，此外ttf中还会包含很多多余的文字信息，通常是只保存渲染后文本保持。
4. 上下文字距，由于是存的渲染后的文本，这样就会涉及一个问题，其比较难利用freetype的很多上下文的特性，来动态调整文本间距和换行间距，但是这个并没有太好办法，通过一些取巧做法可以解决，显示效果也还不错。
5. 字体大小，由于文本是提前渲染的，所以字体大小其实也是提前指定了的，所以项目中需要什么字体大小，就必须把什么字体大小渲染好，由于嵌入式项目中一般文字所需显示的文本大小基本是预先确定的，所以只需支持个别文本大小即可。
6. 多国语言，现在嵌入式项目都需要支持多国语言，所以项目的字体文件编码都是用4字节的`utf-8`编码，来标记每个所需显示的字体。由于采用`utf-8`编码，程序编译时西文文本所占用的code size只有1字节，中文文本所占用的code size最小是3字节，极大节约嵌入式项目的code size资源。

为了解决上述问题，项目提供了`ttf2c.py`脚本来完成字体资源转换。





## 字体渲染说明

freetype一般来说，就是下面这个图比较典型了，当然还有纵向的一个图，这里只考虑横向渲染。

其他简单看到下面的图片一般认为就够了，但是在简易的渲染项目场景下，都希望是从左上角开始绘制，而freetype提供的却是下面的坐标系，为了简易实现渲染，本项目将字体转换为另外一个坐标系描述。

![image-20241228090147122](https://markdown-1306347444.cos.ap-shanghai.myqcloud.com/img/image-20241228090147122.png)



变化成如下格式，后续渲染会方便不少。下图的几个参数和上面的关系是：

`adv`，也就是`advance`。

`box_w`，也就是`xMax-xMin`。

`box_h`，也就是`yMax-yMin`。

`off_x`，也就是`beringX`。

`off_y`，也就是`ascender - bearing_y`。这里的就是本字体在特定字体大小下的最大高度。也就是`egui_res_font_xxxxx_info`中的`height`字段。

![image-20241228091651719](https://markdown-1306347444.cos.ap-shanghai.myqcloud.com/img/image-20241228091651719.png)





## ttf_easy_render.py

这是一个简易测试程序，用一个特定字体来渲染一串文本，并保持到图片中。里面直观的呈现了坐标系变化的计算过程。



![image-20241228092356263](https://markdown-1306347444.cos.ap-shanghai.myqcloud.com/img/image-20241228092356263.png)



![image-20241228092442529](https://markdown-1306347444.cos.ap-shanghai.myqcloud.com/img/image-20241228092442529.png)







## trf2c.py

为了解决上述问题，支持如下参数配置，通过调整参数可以满足所有项目需要。

![image-20241228084252074](https://markdown-1306347444.cos.ap-shanghai.myqcloud.com/img/image-20241228084252074.png)



## 内部资源

只会生成一个`egui_res_font_xxxxx.c`文件，里面保持了所有渲染文本信息，生成的内部资源包括，

`egui_res_font_xxxxx_pixel_buffer`，字体像素点矩阵数组。

`egui_res_font_xxxxx_char_array`，对每个所需渲染文本的描述信息，通过`egui_res_font_xxxxx_code_array`映射utf-8 code和pixel_buffer信息。

`egui_res_font_xxxxx_code_array`，字体的utf-8编码信息，和`egui_res_font_xxxxx_char_array`大小位置匹配，分开主要为了省code size，并加快外部资源情况下的效率。

`egui_res_font_xxxxx_info`，字体的全局配置信息，字体大小，字体最大高度，资源类型等信息。

`egui_font_std_t egui_res_font_xxxxx`，结构体。

![image-20241228085131488](https://markdown-1306347444.cos.ap-shanghai.myqcloud.com/img/image-20241228085131488.png)



![image-20241228085201141](https://markdown-1306347444.cos.ap-shanghai.myqcloud.com/img/image-20241228085201141.png)



![image-20241228085224714](https://markdown-1306347444.cos.ap-shanghai.myqcloud.com/img/image-20241228085224714.png)



![image-20241228085239365](https://markdown-1306347444.cos.ap-shanghai.myqcloud.com/img/image-20241228085239365.png)









## 外部资源

会生成3个文件，`egui_res_font_xxxxx_bin.c`文件，里面保持了字体配置信息，`egui_res_font_xxxxx_char_desc.bin`文件，里面保存了对每个所需渲染文本的描述信息也就是，`egui_res_font_xxxxx_char_array`，`egui_res_font_xxxxx_pixel_buffer.bin`文件，里面保存了字体像素点矩阵数组，也就是`egui_res_font_xxxxx_pixel_buffer`。生成的外部资源包括，

`egui_res_font_xxxxx_bin_info`，图片配置信息。

`egui_font_std_t egui_res_font_xxxxx_bin`，结构体。

**注意**，为了方便管理，有外部资源需求，最好用`app_resource_config.json`统一管理。

![image-20241228085324133](https://markdown-1306347444.cos.ap-shanghai.myqcloud.com/img/image-20241228085324133.png)



![image-20241228085554223](https://markdown-1306347444.cos.ap-shanghai.myqcloud.com/img/image-20241228085554223.png)



![image-20241228085605955](https://markdown-1306347444.cos.ap-shanghai.myqcloud.com/img/image-20241228085605955.png)



## Icon Font 支持

除了常规文本字体外，项目还支持 Icon Font（图标字体），可以将矢量图标作为字体字符使用。最常用的图标字体是 Google 的 MaterialSymbols。

### 使用 MaterialSymbols 图标字体

1. 下载 MaterialSymbols 的 TTF 文件（如 `MaterialSymbolsOutlined.ttf`）
2. 在 `supported_text.txt` 中写入所需图标的 Unicode 字符
3. 在 `app_resource_config.json` 中配置：

```json
{
    "font": [
        {
            "file": "MaterialSymbolsOutlined.ttf",
            "text": "supported_icons.txt",
            "external": "0",
            "pixelsize": "24",
            "fontbitsize": "4"
        }
    ]
}
```

### 在代码中使用图标字体

图标字体和普通字体的使用方式完全一致，通过 UTF-8 编码引用对应的图标字符：

```c
// 设置图标字体
egui_view_label_set_font(EGUI_VIEW_OF(&icon_label),
                         (egui_font_t *)&egui_res_font_material_symbols_24_4);
// 设置图标文本（UTF-8 编码的图标字符）
egui_view_label_set_text(EGUI_VIEW_OF(&icon_label), "\xEF\x80\x8C");
```

### 图标字体的优势

- 矢量渲染，不同大小都清晰
- 支持抗锯齿（通过 `fontbitsize` 控制）
- 占用空间远小于位图图标
- 可以和文本混排

## 多语言字体合并

嵌入式项目经常需要支持多国语言，不同语言可能使用不同的字体文件。项目支持将多个字体配置合并，减少资源冗余。

### 自动合并规则

当 `app_resource_config.json` 中多个字体配置项满足以下条件时，会自动合并：

- 相同的 `file`（字体文件）
- 相同的 `pixelsize`（字体大小）
- 相同的 `fontbitsize`（位深）
- 相同的 `external`（存储位置）

合并时，多个 `text` 文件中的文本会被合并，重复字符自动去重。

### 多语言配置示例

```json
{
    "font": [
        {
            "file": "NotoSansSC-Regular.ttf",
            "text": "chinese_text.txt",
            "external": "0",
            "pixelsize": "16",
            "fontbitsize": "4"
        },
        {
            "file": "NotoSansSC-Regular.ttf",
            "text": "english_text.txt",
            "external": "0",
            "pixelsize": "16",
            "fontbitsize": "4"
        }
    ]
}
```

上述配置中，`chinese_text.txt` 和 `english_text.txt` 的内容会被合并为一个字体资源，共享同一个 `egui_res_font_xxxxx` 结构体。

### 多字体文件的多语言方案

如果不同语言需要不同的字体文件（如中文用思源黑体，西文用 Roboto），则需要分别配置，在代码中根据语言切换字体：

```c
// 中文标签使用中文字体
egui_view_label_set_font(EGUI_VIEW_OF(&label_cn),
                         (egui_font_t *)&egui_res_font_noto_sans_sc_16_4);
// 英文标签使用英文字体
egui_view_label_set_font(EGUI_VIEW_OF(&label_en),
                         (egui_font_t *)&egui_res_font_roboto_16_4);
```
