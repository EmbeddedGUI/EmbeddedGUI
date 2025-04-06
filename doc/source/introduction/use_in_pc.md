# PC调试

## 概述

EmbeddedGUI支持在PC编译调试。PC上的资源多，这样平时可以在PC上先进行调试，等UI交互行为测试成功后，再将代码移植到嵌入式芯片中。

如果熟悉代码结构，可以做到PC编译调试和嵌入式开发同一份代码，只是Porting代码不同。

本项目仓库路径：[EmbeddedGUI(gitee.com)](https://gitee.com/embeddedgui/EmbeddedGUI)，[EmbeddedGUI(github.com)](https://github.com/EmbeddedGUI/EmbeddedGUI)。



## 特点

- **支持Windows、Linux和MacOS**
- **支持在VSCode中实时GUI应用程序**
- **在PC平台上本地编译和运行** 
- **在PC上使用与嵌入式相同的代码来开发GUI应用程序**
- **在PC上开发并在MCU上运行**，应用程序代码和资源文件可以直接在MCU上使用，因为它们与硬件无关。





## 环境搭建

运行代码根据自身的运行平台，搭建运行环境。

主要来讲就是需要配置好Python环境、SDL2库支持（这个不同平台有不同安装方式）以及GCC编译环境。

其中Python环境可以用Python2也可以用Python3按需安装环境即可。SDL2和GCC按照平台要求安装。

![image-20250119164136477](https://markdown-1306347444.cos.ap-shanghai.myqcloud.com/img/image-20250119164136477.png)



### Python环境搭建

需要安装Python环境，Python2或者Python3都可以，把相关依赖安装好就行。Python主要用于生成资源文件。

安装根目录的`requirements.txt`，就会安装所有所需的python环境。

```shell
python -m pip install -r requirements.txt
```

**注意**，经常有网友报告`freetype_py`安装后，依然无法运行，提示`Freetype library not found`，可以自行搜索解决方案。如：https://blog.csdn.net/wyx100/article/details/73527117





### Windows环境搭建

目前需要安装如下环境：

- [Python3](http://www.python.org/getit/)，用于资源管理。
- SDL2库，使用工程自带的SDL2静态lib即可，需要考虑编译环境是64bit还是32bit。
- GCC环境，笔者用的msys64+mingw，用于编译生成exe，参考这个文章安装即可。[Win7下msys64安装mingw工具链 - Milton - 博客园 (cnblogs.com)](https://www.cnblogs.com/milton/p/11808091.html)。



### MacOS环境搭建

SDL2的安装麻烦点，可以参考[SDL2](https://www.bilibili.com/opus/940636995053420548)。其他比较简单，就不展开了。

打开`Terminal`，并[安装homebrew](https://brew.sh/)

Second, please open the terminal in MacOS and [install the homebrew](https://brew.sh/) with the following command:

```sh
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
```





![image-20250119165100719](https://markdown-1306347444.cos.ap-shanghai.myqcloud.com/img/image-20250119165100719.png)









### Linux环境搭建

还是SDL2安装麻烦一些。

```sh
sudo apt-get update && sudo apt-get install -y build-essential libsdl2-dev
```

![image-20250119165500449](https://markdown-1306347444.cos.ap-shanghai.myqcloud.com/img/image-20250119165500449.png)















## Makefile编译说明

本项目都是由makefile组织编译的，编译整个项目只需要执行`make all`即可。

根据具体需要可以调整一些参数，目前Makefile支持如下参数配置。

- **APP**：选择example中的例程，默认选择为`HelloSimple`。
- **PORT**：选择porting中的环境，也就是当前平台，默认选择为`pc`。
- **BITS**：选择平台编译环境，默认选择为`32`，如果是64bit的cpu，需要选择`64`。

### 编译

也就是可以通过如下指令来编译工程：

```shell
make all APP=HelloSimple PORT=pc BITS=64
```

![image-20250104152959083](https://markdown-1306347444.cos.ap-shanghai.myqcloud.com/img/image-20250104152959083.png)

![image-20250104153021161](https://markdown-1306347444.cos.ap-shanghai.myqcloud.com/img/image-20250104153021161.png)

### 运行

运行整个工程，只需要运行`make run`即可。

注意，如果修改了配置，需要带上修改的参数。

```shell
make run APP=HelloSimple PORT=pc BITS=64
```

![image-20250104153055742](https://markdown-1306347444.cos.ap-shanghai.myqcloud.com/img/image-20250104153055742.png)

### 配置

上述的配置方法太麻烦了，老是忘记参数。

在项目根目录的`Makefile`中配置好了以后，之后只要运行`make all`和`make run`就行。

![image-20250104153734842](https://markdown-1306347444.cos.ap-shanghai.myqcloud.com/img/image-20250104153734842.png)





## Cmake编译说明

**注意**，在大家的支持下，目前也支持cmake编译，但是由于作者能力有限，并没深入完善，后续有空再完善。但是基本架子有了，可以看看代码结构，或者根据需要在本地环境调整。可以先创建`build`目录，然后到`build`目录下，执行`cmake ..`，最后执行make all就可以运行了。

![image-20250104152601840](https://markdown-1306347444.cos.ap-shanghai.myqcloud.com/img/image-20250104152601840.png)

![image-20250104152621970](https://markdown-1306347444.cos.ap-shanghai.myqcloud.com/img/image-20250104152621970.png)





## VSCode编译调试

平时不可避免要进行调试工作，直接用vscode的Debug即可。

![image-20250406173257928](https://markdown-1306347444.cos.ap-shanghai.myqcloud.com/img/image-20250406173257928.png)

设置好断点就可以调试了。

![image-20250406173522522](https://markdown-1306347444.cos.ap-shanghai.myqcloud.com/img/image-20250406173522522.png)



























