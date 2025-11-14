# 简易Page开发模式

在日常开发中，通常需要开发多个页面，每个页面呈现不一样的内容，而不同页面启动后，另外一个页面是隐藏的。虽然比较建议大家实用Activity来管理多个页面，其不但支持多页面共存，还支持页面切换动画。但是实际来看，很多人对一整套的Activity管理用的并不习惯。
大家更喜欢使用更简单直接的管理方式来管理多个页面，从而导致各种奇奇怪怪的问题。
所以为了更进一步规范大家实现多页面的管理，加入`HelloEasyPage`例程，在本例程中实现了简易Page的开发模式。其主要解决：

1. Page生命周期管理
2. Page页面和资源独立管理
3. 多Page RAM资源优化

## Page生命周期管理

简单应用场景下，不考虑Page的层级关系，永远只打开一个当前Page的场景，那Page的生命周期可以简化为Open和Close两个状态。
Open状态：页面打开，界面开始显示该页面。
Close状态：页面关闭，界面隐藏该页面，相关资源销毁。

以egui_page_base_t为例，其包含Page的`Open`和`Close`调用接口，以及`on_open`和`on_close`的回调。

```c

typedef struct egui_page_base_api egui_page_base_api_t;
struct egui_page_base_api
{
    void (*on_open)(egui_page_base_t *self);
    void (*on_close)(egui_page_base_t *self);
    
    ...
};


void egui_page_base_open(egui_page_base_t *self);
void egui_page_base_close(egui_page_base_t *self);


void egui_page_base_on_open(egui_page_base_t *self);
void egui_page_base_on_close(egui_page_base_t *self);

```





## Page页面和资源独立管理

一般在特定页面下，有特定页面的逻辑需要处理，和其他页面关系不大，所以每个页面一个独立的结构体，由uicode调度Page的启动关闭，公用资源可以放在uicode管理。

```c
struct egui_page_0
{
    egui_page_base_t base;

    int index;
    char label_str[20];

    egui_timer_t timer;

    egui_view_linearlayout_t layout_1;
    egui_view_label_t label_1;
    egui_view_button_t button_1;
    egui_view_button_t button_2;

    egui_view_image_t image_1;
};
```



在`on_open`接口中，实现对所有控件和资源的初始化，最后调用`egui_page_base_add_view`，将控件加入到page中。

在`on_close`接口中，需要实现所有控件和资源的销毁动作。如`on_open`启动了一个定时器，需要考虑在`on_close`关闭这个定时器。







## 多Page RAM资源优化

在资源比较紧张场景下，每个页面下有很多个控件，而且Page数目还很多，这个情况下，如果所有Page都静态声明，RAM资源浪费比较大。

考虑这个场景，结合Page的生命周期，可以将所有的Page用union管理，这样多个Page只占用其中最大哪个Page的RAM资源。

```c

union page_array{
    egui_page_0_t page_0;
    egui_page_1_t page_1;
};

static union page_array g_page_array;



```



配合`egui_page_base_open`和`egui_page_base_close`接口，就可以确保每次只有一个页面在工作，自然只要一个Page的RAM资源即可。

```c

void uicode_switch_page(int page_index)
{
    index = page_index;

    egui_api_sprintf(toast_str, "Start page %d", page_index);
    egui_core_toast_show_info(toast_str);

    if(current_page)
    {
        egui_page_base_close(current_page);
    }

    switch(page_index)
    {
        case 0:
            egui_page_0_init((egui_page_base_t *)&g_page_array.page_0);
            current_page = (egui_page_base_t *)&g_page_array.page_0;
            break;
        case 1:
            egui_page_1_init((egui_page_base_t *)&g_page_array.page_1);
            current_page = (egui_page_base_t *)&g_page_array.page_1;
            break;
        default:
            break;
    }
    
    egui_page_base_open(current_page);
}
```









