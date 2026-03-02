# 面向对象 C 语言模式

## 概述

为了代码结构清晰点，项目中有继承需要的代码采用面向对象的编程思维，主要用到类继承和虚函数的定义。本章介绍 EmbeddedGUI 中使用的 OOP 模式，以及配套的辅助宏。


## 基本模式

### 类

所有类用 struct 来实现，为方便使用，都用 typedef 声明下。

```c
// C++ class impl
class AAA
{
}

// C struct impl
typedef struct AAA AAA;
struct AAA
{

}
```



### 类-成员

成员用结构体成员来实现。

```c
// C++ class impl
class AAA
{
    int aaa;
}

// C struct impl
typedef struct AAA AAA;
struct AAA
{
    int aaa;
}
```



### 类-构造函数、方法

public、protect和private就不区分了，软件自己控制操作空间。直接在方法名前面加入`class_`来区分。第一个传参调整为类对象的指针，名称为self。

注意：为方便后续部分方法调整为虚函数，以及代码统一，所有类对象代码的第一个参数都为**基类**的class指针对象。

对于构造函数（析构也一样，不过本项目没有），定义函数`class_init`来实现，**因为编译器不会帮你调用，所以需要自己手动调用**。

为了避免后续维护麻烦，init 只做基本的操作，如默认值配置，主题加载等。参数的负责由外界其他操作方法来实现。

```c
// C++ class impl
class AAA
{
    AAA(aaa) {}
public:
    void func_1(void) {}
protect:
    void func_2(void) {}
private:
    void func_3(void) {}

    int aaa;
}

// C struct impl
typedef struct AAA AAA;
struct AAA
{
    int aaa;
}

void AAA_func_1(AAA *self) {}
void AAA_func_2(AAA *self) {}
void AAA_func_3(AAA *self) {}

void AAA_init(AAA *self)
{
    self->aaa = 0;
}
```



### 类-虚函数

这里最麻烦的就是**虚函数**了处理了，因为涉及到类继承，函数覆盖等处理。简单的处理就是一个虚函数一个函数指针，但是这样当类里面的虚函数比较多时，所以RAM就很多了。

所以这里用一个麻烦的处理，用函数列表来做，所有集成类的构造函数（也就是`class_init`）需要重新赋值虚函数表。

为区分，虚函数的函数命令需要在函数前面加入`class_`。还要声明一个结构体为`struct class_api`来定义虚函数表，同时类的成员加入`const class_api *api`来存储函数列表指针。

```c
// C++ class impl
class AAA
{
    AAA(aaa) {}

    virtual void func_virtual_1(void) {}

    int aaa;
}

// C virtual api impl
typedef struct AAA_api AAA_api;
struct AAA_api
{
    void (*func_virtual_1)(AAA *self);
}

// C struct impl
typedef struct AAA AAA;
struct AAA
{
    const AAA_api* api; // virtual api
    int aaa;
}

void AAA_func_virtual_1(AAA *self)
{
}

static const AAA_api AAA_api_table = {
    AAA_func_virtual_1,
};

void AAA_init(AAA *self)
{
    self->aaa = 0;
    self->api = &AAA_api_table; // set virtual api.
}
```

**虚函数表（api table）存储在 ROM 中**（声明为 `static const`），因此不会消耗额外的 RAM。每个类实例仅需要一个指针（`api`）指向共享的虚函数表。

### 虚函数表的实际使用

以 `egui_view_t` 为例，其虚函数表定义如下：

```c
typedef struct egui_view_api egui_view_api_t;
struct egui_view_api
{
    int (*dispatch_touch_event)(egui_view_t *self, egui_motion_event_t *event);
    int (*on_touch_event)(egui_view_t *self, egui_motion_event_t *event);
    int (*on_intercept_touch_event)(egui_view_t *self, egui_motion_event_t *event);
    void (*compute_scroll)(egui_view_t *self);
    void (*calculate_layout)(egui_view_t *self);
    void (*request_layout)(egui_view_t *self);
    void (*draw)(egui_view_t *self);
    void (*on_attach_to_window)(egui_view_t *self);
    void (*on_draw)(egui_view_t *self);
    void (*on_detach_from_window)(egui_view_t *self);
};
```

调用虚函数时，通过 `api` 指针间接调用：

```c
// 调用虚函数 draw
view->api->draw(view);

// 调用虚函数 on_touch_event
view->api->on_touch_event(view, event);
```



### 类-继承

暂时只考虑只继承一个父类，不考虑继承多个父类的处理。

子类需要定义第一个成员为父类`base`，构造函数需要先调用父类的构造函数，有虚函数重写的，需要重新定义虚函数表，并覆盖。

所有涉及虚函数，使用基类作为函数self传参。虚函数实现的api接口，传参为基类的api，需要转一下`BBB *b= (BBB*)self;`。

```c
// C++ class impl
class AAA
{
    virtual void func_virtual_1(void) {}
}

class BBB : public AAA
{
    virtual void func_virtual_1(void) {}
}

// C virtual api impl
typedef struct AAA_api AAA_api;
struct AAA_api
{
    void (*func_virtual_1)(AAA *self);
}

// C struct impl - base class
typedef struct AAA AAA;
struct AAA
{
    const AAA_api* api; // virtual api
}

void AAA_func_virtual_1(AAA *self) {}

static const AAA_api AAA_api_table = {
    AAA_func_virtual_1,
};

void AAA_init(AAA *self)
{
    self->api = &AAA_api_table;
}

// C struct impl - derived class
typedef struct BBB BBB;
struct BBB
{
    AAA base; // base class, MUST be the first member
}

void BBB_func_virtual_1(AAA *self)
{
    BBB *bbb = (BBB *)self;
    // access BBB specific members through bbb->...
}

static const AAA_api BBB_api_table = {
    BBB_func_virtual_1,
};

void BBB_init(BBB *self)
{
    AAA_init(&self->base);              // call base init func
    self->base.api = &BBB_api_table;    // override virtual api table
}
```

继承链的关键要点：

1. 子类的第一个成员必须是父类结构体 `base`，这样基类指针和子类指针的地址相同，可以安全地进行类型转换
2. 子类的 init 函数必须先调用父类的 init 函数，然后覆盖虚函数表
3. 虚函数的参数始终使用基类指针（`AAA *self`），在实现中需要手动转换为子类指针


## OOP 辅助宏（egui_oop.h）

项目提供了一套 OOP 辅助宏，定义在 `src/core/egui_oop.h` 中，用于简化上述类型转换操作。所有宏都在编译时展开，零运行时开销，兼容 C99 标准。

### 第一类：self 指针向下转换

在虚函数实现中，需要将基类指针转换为子类指针。使用 `EGUI_LOCAL_INIT` 宏可以简化这一操作：

```c
// 改造前
void egui_view_label_set_text(egui_view_t *self, const char *text)
{
    egui_view_label_t *local = (egui_view_label_t *)self;
    local->text = text;
}

// 改造后
void egui_view_label_set_text(egui_view_t *self, const char *text)
{
    EGUI_LOCAL_INIT(egui_view_label_t);
    local->text = text;
}
```

| 宏 | 说明 | 展开结果 |
|----|------|---------|
| `EGUI_LOCAL_INIT(_type)` | 声明 `local` 变量并从 `self` 向下转换 | `_type *local = (_type *)self;` |
| `EGUI_LOCAL_INIT_VAR(_type, _var)` | 自定义变量名的向下转换 | `_type *_var = (_type *)self;` |
| `EGUI_INIT_LOCAL(_type)` | 用于 init 函数的向下转换 | `_type *local = (_type *)self;` |

### 第二类：向上转换（derived -> base）

将子类指针转换为基类指针，用于需要基类指针的 API 调用：

```c
// 将 linearlayout 指针转换为 egui_view_t* 用于添加子视图
egui_view_group_add_child(EGUI_VIEW_OF(&local->container), EGUI_VIEW_OF(&local->label));

// 将 animation 指针转换为 egui_animation_t*
egui_core_animation_append(EGUI_ANIM_OF(&local->anim_alpha));
```

| 宏 | 说明 | 展开结果 |
|----|------|---------|
| `EGUI_VIEW_OF(_ptr)` | 转换为 `egui_view_t*` | `((egui_view_t *)(_ptr))` |
| `EGUI_ANIM_OF(_ptr)` | 转换为 `egui_animation_t*` | `((egui_animation_t *)(_ptr))` |
| `EGUI_BG_OF(_ptr)` | 转换为 `egui_background_t*` | `((egui_background_t *)(_ptr))` |
| `EGUI_INTERP_OF(_ptr)` | 转换为 `egui_interpolator_t*` | `((egui_interpolator_t *)(_ptr))` |
| `EGUI_MASK_OF(_ptr)` | 转换为 `egui_mask_t*` | `((egui_mask_t *)(_ptr))` |
| `EGUI_FONT_OF(_ptr)` | 转换为 `egui_font_t*` | `((egui_font_t *)(_ptr))` |
| `EGUI_BASE_OF(_ptr)` | 通过 `base` 成员获取基类指针 | `((void *)&((_ptr)->base))` |

### 第三类：显式向下转换

当已知实际类型时，将基类指针转换为子类指针：

```c
egui_view_label_t *label = EGUI_CAST_TO(egui_view_label_t, base_ptr);
```

| 宏 | 说明 |
|----|------|
| `EGUI_CAST_TO(_type, _ptr)` | `((_type *)(_ptr))` |

### 第四类：父视图访问

简化视图树中父子关系的访问：

```c
egui_view_t *parent = EGUI_VIEW_PARENT(self);
if (EGUI_VIEW_HAS_PARENT(self)) {
    // do something with parent
}
```

| 宏 | 说明 |
|----|------|
| `EGUI_VIEW_PARENT(_view)` | 获取父视图指针（`egui_view_t*`） |
| `EGUI_VIEW_HAS_PARENT(_view)` | 检查是否有父视图 |


## 如何自定义控件

继承 `egui_view_t` 创建自定义控件的完整步骤：

### 步骤 1：定义结构体

```c
// my_custom_view.h
typedef struct my_custom_view my_custom_view_t;
struct my_custom_view
{
    egui_view_t base;       // 基类，必须是第一个成员
    int my_value;           // 自定义成员
    egui_color_t my_color;
};
```

### 步骤 2：实现虚函数

```c
// my_custom_view.c
static void my_custom_view_on_draw(egui_view_t *self)
{
    EGUI_LOCAL_INIT(my_custom_view_t);

    // 先调用基类的 on_draw 绘制背景
    egui_view_on_draw(self);

    // 获取绘制区域
    egui_region_t region;
    egui_view_get_work_region(self, &region);

    // 自定义绘制逻辑
    egui_canvas_draw_rectangle_fill(
        region.location.x, region.location.y,
        region.size.width, region.size.height,
        local->my_color, EGUI_ALPHA_100
    );
}
```

### 步骤 3：定义虚函数表并初始化

```c
// 定义虚函数表，只覆盖需要自定义的虚函数
static const egui_view_api_t my_custom_view_api_table = {
    .dispatch_touch_event = egui_view_dispatch_touch_event,     // 复用基类
    .on_touch_event = egui_view_on_touch_event,                 // 复用基类
    .on_intercept_touch_event = egui_view_on_intercept_touch_event,
    .compute_scroll = egui_view_compute_scroll,
    .calculate_layout = egui_view_calculate_layout,
    .request_layout = egui_view_request_layout,
    .draw = egui_view_draw,                                     // 复用基类
    .on_attach_to_window = egui_view_on_attach_to_window,
    .on_draw = my_custom_view_on_draw,                          // 覆盖 on_draw
    .on_detach_from_window = egui_view_on_detach_from_window,
};

void my_custom_view_init(egui_view_t *self)
{
    EGUI_INIT_LOCAL(my_custom_view_t);

    // 调用基类 init
    egui_view_init(self);

    // 覆盖虚函数表
    self->api = &my_custom_view_api_table;

    // 初始化自定义成员
    local->my_value = 0;
    local->my_color = EGUI_COLOR_WHITE;
}
```

### 步骤 4：使用自定义控件

```c
static my_custom_view_t my_view;

void setup_ui(void)
{
    my_custom_view_init(EGUI_VIEW_OF(&my_view));
    egui_view_set_position(EGUI_VIEW_OF(&my_view), 10, 10);
    egui_view_set_size(EGUI_VIEW_OF(&my_view), 100, 50);
    egui_core_add_user_root_view(EGUI_VIEW_OF(&my_view));
}
```


## 继承层次示例

EmbeddedGUI 中典型的继承层次：

```
egui_view_t                          // 所有视图的基类
  +-- egui_view_group_t              // 容器视图
  |     +-- egui_view_linearlayout_t // 线性布局
  |     +-- egui_view_gridlayout_t   // 网格布局
  |     +-- egui_view_scroll_t       // 滚动容器
  |     +-- egui_view_viewpage_t     // 滑动页面
  +-- egui_view_label_t              // 文本标签
  +-- egui_view_button_t             // 按钮
  +-- egui_view_image_t              // 图片
  +-- egui_view_progress_bar_t       // 进度条
  +-- egui_view_switch_t             // 开关
  +-- ...                            // 更多控件
```

所有控件通过统一的 `egui_view_t *` 基类指针进行管理，虚函数表机制保证了正确的多态行为。
