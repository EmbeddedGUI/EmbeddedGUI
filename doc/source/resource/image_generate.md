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

只会生成一个`xxxxx.c`文件，里面保持了所有图片信息，生成的内部资源包括，

`xxxxx_alpha_buf`，透明通道数组。

`xxxxx_data_buf`，RGB像素点数组。

`xxxxx_info`，图片配置信息。

`egui_image_std_t xxxxx`，结构体。

![image-20241227222514952](https://markdown-1306347444.cos.ap-shanghai.myqcloud.com/img/image-20241227222514952.png)

![image-20241227222727409](https://markdown-1306347444.cos.ap-shanghai.myqcloud.com/img/image-20241227222727409.png)

![image-20241227222749250](https://markdown-1306347444.cos.ap-shanghai.myqcloud.com/img/image-20241227222749250.png)





## 外部资源

会生成3个文件，`xxxxx_bin.c`文件，里面保持了图片配置信息，`xxxxx_alpha.bin`透明通道二进制文件，`xxxxx_data.bin`RGB像素点二进制文件，生成的外部资源包括，

`xxxxx_bin_info`，图片配置信息。

`egui_image_std_t xxxxx_bin`，结构体。

**注意**，为了方便管理，有外部资源需求，最好用`app_resource_config.json`统一管理。

![image-20241227223017199](https://markdown-1306347444.cos.ap-shanghai.myqcloud.com/img/image-20241227223017199.png)



![image-20241227223143798](https://markdown-1306347444.cos.ap-shanghai.myqcloud.com/img/image-20241227223143798.png)



