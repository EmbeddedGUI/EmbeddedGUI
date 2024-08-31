# 代码结构说明

## 面向对象编程代码结构说明

为了代码结构清晰点，项目中有继承需要的代码采用面向对象的编程思维，主要用到类继承和虚函数的定义。

### 类

所有类用struct来实现，为方便使用，都用typedef声明下。

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

为了避免后续维护麻烦，init只做基本的操作，如默认值配置，主题加载等。参数的负责由外界其他操作方法来实现。

```c
// C++ class impl
class AAA
{
    AAA(aaa) {}
public:
    void func_1(void)
    {
        
    }
    
protect:
    void func_2(void)
    {
        
    }
    
private:
    void func_3(void)
    {
        
    }
    
    int aaa;
}



// C struct impl
typedef struct AAA AAA;
struct AAA
{
}
void AAA_func_1(AAA *self)
{

}

void AAA_func_2(AAA *self)
{

}

void AAA_func_3(AAA *self)
{
    
}
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

    virtual void func_virtual_1(void)
    {
        
    }
    
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





### 类-继承

暂时只考虑只继承一个父类，不考虑继承多个父类的处理。

子类需要定义第一个成员为父类`base`，构造函数需要先调用父类的构造函数，有虚函数重写的，需要重新定义虚函数表，并覆盖。

所有涉及虚函数，使用基类作为函数self传参。虚函数实现的api接口，传参为基类的api，需要转一下`BBB *b= (BBB*)self;`。

```c
// C++ class impl
class AAA
{
    virtual void func_virtual_1(void)
    {
        
    }
}

class BBB : public AAA
{
    virtual void func_virtual_1(void)
    {
        
    }
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
}
void AAA_func_virtual_1(AAA *self)
{

}

static const AAA_api AAA_api_table = {
        AAA_func_virtual_1,
};

void AAA_init(AAA *self)
{
    self->api = &AAA_api_table; // set virtual api.
}




// C struct impl
typedef struct BBB BBB_t;
struct BBB
{
    AAA base; // base class
}
void BBB_func_virtual_1(AAA *self)
{
	BBB *bbb = (BBB *)self;
}

static const AAA_api BBB_api_table = {
        BBB_func_virtual_1,
};

void BBB_init(BBB *self)
{
	AAA_init(&self->base); // call base init func
    
    self->base.api = &BBB_api_table; // set virtual api.
}
```







