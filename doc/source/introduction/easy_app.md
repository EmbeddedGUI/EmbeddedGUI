# 简易应用开发模式

在日常开发过程中，除了要管理多个页面外，还需要对各个页面的资源进行管理，不同页面需要用到不同的字体，不同的字体的大小，显示的文本也各不相同。也会用到不同的图片，以及不同的图片尺寸。

资源管理的原理请参考[资源生成概述 — EmbeddedGUI 1.0.0 documentation](https://embeddedgui.readthedocs.io/en/latest/resource/resource_generate_introduction.html)，本文重点讲述在应用开发中如何高效和精简的完成资源管理。

总的来说就是完成字体和图片的资源管理、页面管理和按键管理。本文所讨论的程序可以在`HelloAPP`看具体操作流程。

## 资源管理

### 字体管理

字体的管理有其特殊性，因为每个页面下，所需的字体、字体的大小已经所需显示的文本各不相同。

像英文总共就是26个字符，算上大小写也就是26个字母。但是如果要显示中文，不可能把所有字符都转成资源，那会浪费很多资源。

比较简易的管理模式是，需要显示什么文本，就定义一个txt文件，每个页面/控件所需显示的文本，都列在txt文件中，而后在`app_resource_config.json`声明其所需的字体和大小。

如page0字体为`test.ttf`，大小为`25`的文本为`supported_text_page0.txt`。

如page1字体为`DejaVuSans.ttf`，大小为`30`的文本为`supported_text_page1.txt`。

**注意**，不要嫌麻烦，重复的文本会自动融合，不用担心重复，分得越细，后续管理越方便，资源占用最小。

```json
"font": [
        {
            "file": "test.ttf",
            "text": "supported_text_page0.txt",
            "external": "0",
            "pixelsize": "25",
            "fontbitsize": "4",
        },
        {
            "file": "DejaVuSans.ttf",
            "text": "supported_text_page1.txt",
            "external": "0",
            "pixelsize": "30",
            "fontbitsize": "4",
        },
    ],
}
```

一般来讲，在json中不会配置name属性（如果要声明，自己要确保名称不同，不然可能会自动覆盖），脚本会自动生成名称，并且可以确保名称之间不会异常覆盖。命名规则为`egui_res_font_XXX_YYY_ZZZ`，`XXX`为字体文件名称`file`字段，`YYY`为`pixelsize`字段，`ZZZ`为`fontbitsize`字段。

在txt文件中，如果考虑资源使用，不要声明多余的东西，只放所需显示的文本。

```
egui_page_0
0123456789
Tick: 
```



### 图片管理

图片管理相对简单，只要声明不同的名称就可以了。一般情况下，不要配置为`all`，需要什么尺寸什么分辨率的图片，就只声明那个配置即可。

```json
"img": [
        {
            "file": "star.png",
            "external": "0",
            "format": "rgb565",
            "alpha": "4",
        },
        {
            "file": "test.png",
            "external": "0",
            "format": "rgb565",
            "alpha": "4",
        },
    ],
```

一般来讲，在json中不会配置name属性（如果要声明，自己要确保名称不同，不然可能会自动覆盖），脚本会自动生成名称，并且可以确保名称之间不会异常覆盖。命名规则为`egui_res_image_XXX_YYY_ZZZ`，`XXX`为字体文件名称`file`字段，`YYY`为`format`字段，`ZZZ`为`alpha`字段。



### 使用说明

当有调整`app_resource_config.json`时，必须要调用`make resource_refresh`。

之后再调用`make run`运行即可。

```PowerShell
PS D:\workspace\gitee\EmbeddedGUI> make resource_refresh
python ./scripts/tools/app_resource_generate.py -r example/HelloAPP/resource -o output -f ture
Generating egui_res_image_star_rgb565_4
Generating egui_res_image_test_rgb565_4
Generating egui_res_font_test_25_4 with ['supported_text_page0.txt']
Generating egui_res_font_dejavusans_30_4 with ['supported_text_page1.txt']
Generating example/HelloAPP/resource\app_egui_resource_generate.h
Generating example/HelloAPP/resource\app_egui_resource_generate.c
Generating example/HelloAPP/resource\app_egui_resource_generate_report.md
===============内部===============
  Image       Font      Total   
  61000       6240      67240   
===============外部===============
  Image       Font      Total   
    0          0          0     
===============总计===============
  Image       Font      Total   
  61000       6240      67240   
```





## 页面管理

多页面管理可以考虑用Activity管理，也可以考虑用Page管理。

这里使用简单的Page管理模式，详细可以看[简易Page开发模式](https://embeddedgui.readthedocs.io/en/latest/introduction/easy_page.html)。



## 按键管理

应用开发过程中，除了触摸屏操作外，很多嵌入式设备需要支持按键控制等功能。

在`uicode.c`中需要实现`egui_port_hanlde_key_event`接口，来处理当前处理的按键事件。至于这个按键怎么来的，具体平台具体处理，自行定义keycode即可。

应用自己根据需要，响应按键处理。默认`egui_page_base_t`支持key_press相关处理。



























