# 说明

MP4这个控件其实就是对一堆图片按照特定帧率进行图片顺序播放的控件。

那主要问题就是如何从mp4中提取出特定帧率的图片集合来，在`scripts/tools/app_mp4_image_generate.py`提供了脚本来实现mp4转图片集的工作。

同时这个脚本参数比较多，当有多个mp4需要处理是，还需要用户自己写一个脚本管理如何调用`scripts/tools/app_mp4_image_generate.py`脚本。

本例程提供的`resource\src\app_resource_mp4_work.py`脚本其实就是去配置了如何调用脚本的一个脚本。

`target_fps`：设置要提取图片的帧率

`input_video`：输入视频

`target_width`和`target_height`：如果需要改变尺寸，可以设置这个，不改变不用设置

`target_format`、`target_alpha`和`target_ext`：图像的基本配置，用于后续资源生成时使用

```python
import os

def generate_mp4_image(script_path, input_video, target_fps, width_px, height_px, target_format, target_alpha, target_ext):
    output_path = input_video.split('.')[0]
    os.system("python " + script_path 
              + " -i " + input_video 
              + " -o " + output_path 
              + " -fps " + str(target_fps) 
              + " -width " + str(width_px) 
              + " -height " + str(height_px) 
              + " -f " + target_format 
              + " -a " + str(target_alpha) 
              + " -ext " + str(target_ext))

# basic param
script_path = "../../../"
script_path = os.path.join(script_path, "scripts/tools/app_mp4_image_generate.py")
# print(script_path)
target_fps = 10
target_width = 240 # change to size
target_height = 240 # change to size
target_format = "rgb565"
target_alpha = 0
target_ext = 0

input_video = "test.mp4"
generate_mp4_image(script_path, input_video, target_fps, target_width, target_height, target_format, target_alpha, target_ext)

```



运行完脚本后，同级目录会生成和输入视频`input_video`同名的一个文件夹，这里是`test`。里面包含3种东西。

一堆图片：从MP4中提取出来的图片。

`test.h`：后续用于写代码的图片资源列表，手动复制到需要的地方就行。

`test.json`：提供给`app_resource_config.json`使用的json配置文件，把里面的内容复制到`app_resource_config.json`的`img`参数里，后续用于生成资源配置文件。

也就是说，本脚本运行完，代码并不能使用，只是帮忙把一些信息给你提取出来了。还需要自己去到各个地方配置使用。

![image-20250524164128174](https://markdown-1306347444.cos.ap-shanghai.myqcloud.com/img/image-20250524164128174.png)

将生成的json内容复制到`app_resource_config.json`的`img`参数里，然后运行`make resource_refresh`，生成可以被代码使用的资源文件。

之后到需要用的代码里面。将生成的.h文件内容复制进去，而后配置到控件中。最终调用`egui_view_mp4_start_work`接口，按照特定帧率播放即可。

```c

#define MP4_IMAGE_COUNT_TEST 50
extern const egui_image_t *mp4_arr_test[MP4_IMAGE_COUNT_TEST] = 
{
    (const egui_image_t *)&egui_res_image_test_frame_test_0001_rgb565_0,
    (const egui_image_t *)&egui_res_image_test_frame_test_0002_rgb565_0,
    (const egui_image_t *)&egui_res_image_test_frame_test_0003_rgb565_0,
    (const egui_image_t *)&egui_res_image_test_frame_test_0004_rgb565_0,
    (const egui_image_t *)&egui_res_image_test_frame_test_0005_rgb565_0,
    (const egui_image_t *)&egui_res_image_test_frame_test_0006_rgb565_0,
    (const egui_image_t *)&egui_res_image_test_frame_test_0007_rgb565_0,
    (const egui_image_t *)&egui_res_image_test_frame_test_0008_rgb565_0,
    (const egui_image_t *)&egui_res_image_test_frame_test_0009_rgb565_0,
    (const egui_image_t *)&egui_res_image_test_frame_test_0010_rgb565_0,
    (const egui_image_t *)&egui_res_image_test_frame_test_0011_rgb565_0,
    (const egui_image_t *)&egui_res_image_test_frame_test_0012_rgb565_0,
    (const egui_image_t *)&egui_res_image_test_frame_test_0013_rgb565_0,
    (const egui_image_t *)&egui_res_image_test_frame_test_0014_rgb565_0,
    (const egui_image_t *)&egui_res_image_test_frame_test_0015_rgb565_0,
    (const egui_image_t *)&egui_res_image_test_frame_test_0016_rgb565_0,
    (const egui_image_t *)&egui_res_image_test_frame_test_0017_rgb565_0,
    (const egui_image_t *)&egui_res_image_test_frame_test_0018_rgb565_0,
    (const egui_image_t *)&egui_res_image_test_frame_test_0019_rgb565_0,
    (const egui_image_t *)&egui_res_image_test_frame_test_0020_rgb565_0,
    (const egui_image_t *)&egui_res_image_test_frame_test_0021_rgb565_0,
    (const egui_image_t *)&egui_res_image_test_frame_test_0022_rgb565_0,
    (const egui_image_t *)&egui_res_image_test_frame_test_0023_rgb565_0,
    (const egui_image_t *)&egui_res_image_test_frame_test_0024_rgb565_0,
    (const egui_image_t *)&egui_res_image_test_frame_test_0025_rgb565_0,
    (const egui_image_t *)&egui_res_image_test_frame_test_0026_rgb565_0,
    (const egui_image_t *)&egui_res_image_test_frame_test_0027_rgb565_0,
    (const egui_image_t *)&egui_res_image_test_frame_test_0028_rgb565_0,
    (const egui_image_t *)&egui_res_image_test_frame_test_0029_rgb565_0,
    (const egui_image_t *)&egui_res_image_test_frame_test_0030_rgb565_0,
    (const egui_image_t *)&egui_res_image_test_frame_test_0031_rgb565_0,
    (const egui_image_t *)&egui_res_image_test_frame_test_0032_rgb565_0,
    (const egui_image_t *)&egui_res_image_test_frame_test_0033_rgb565_0,
    (const egui_image_t *)&egui_res_image_test_frame_test_0034_rgb565_0,
    (const egui_image_t *)&egui_res_image_test_frame_test_0035_rgb565_0,
    (const egui_image_t *)&egui_res_image_test_frame_test_0036_rgb565_0,
    (const egui_image_t *)&egui_res_image_test_frame_test_0037_rgb565_0,
    (const egui_image_t *)&egui_res_image_test_frame_test_0038_rgb565_0,
    (const egui_image_t *)&egui_res_image_test_frame_test_0039_rgb565_0,
    (const egui_image_t *)&egui_res_image_test_frame_test_0040_rgb565_0,
    (const egui_image_t *)&egui_res_image_test_frame_test_0041_rgb565_0,
    (const egui_image_t *)&egui_res_image_test_frame_test_0042_rgb565_0,
    (const egui_image_t *)&egui_res_image_test_frame_test_0043_rgb565_0,
    (const egui_image_t *)&egui_res_image_test_frame_test_0044_rgb565_0,
    (const egui_image_t *)&egui_res_image_test_frame_test_0045_rgb565_0,
    (const egui_image_t *)&egui_res_image_test_frame_test_0046_rgb565_0,
    (const egui_image_t *)&egui_res_image_test_frame_test_0047_rgb565_0,
    (const egui_image_t *)&egui_res_image_test_frame_test_0048_rgb565_0,
    (const egui_image_t *)&egui_res_image_test_frame_test_0049_rgb565_0,
    (const egui_image_t *)&egui_res_image_test_frame_test_0050_rgb565_0,
};

static void mp4_callback(egui_view_mp4_t *view, int is_end)
{
    EGUI_LOG_INF("MP4 Callback, is_end: %d\n", is_end);
    if(is_end)
    {
        // egui_view_set_visible((egui_view_t *)view, 0);
        // replay
        egui_view_mp4_start_work(view, (1000 / 10));
    }
}


static egui_view_mp4_t view_mp4;
void main(void)
{
    // view_mp4
    egui_view_mp4_init((egui_view_t *)&view_mp4);
    egui_view_set_position((egui_view_t *)&view_mp4, 0, 0);
    egui_view_set_size((egui_view_t *)&view_mp4, 320, 240);
    view_mp4.mp4_image_list = mp4_arr_test;
    view_mp4.mp4_image_count = MP4_IMAGE_COUNT_TEST;
    view_mp4.callback = mp4_callback;

    // Work in 10 fps
    egui_view_mp4_start_work(&view_mp4, (1000 / 10));
}
```





# 使用步骤

通过上述说明可知，最终要使用需要分多个步骤：

Step1：运行`app_resource_mp4_work.py`脚本，生成图片、json和c文件。

Step2：将生成的json复制到`app_resource_config.json`的`img`参数里，然后运行`make resource_refresh`。

Step3：将生成的c复制到所需的代码中，用`egui_view_mp4_t`控件播放就行





