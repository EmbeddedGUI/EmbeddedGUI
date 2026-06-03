# egui stm32f407vgt6 porting
stm32cubemx生成的cmake项目

1. 复制EmbeddedGUI的`src`、`driver`、`thirdparty`、`example`目录到app/egui_port目录下
2. 根据自己的配置，配置`stm32f407vgt6.ioc`，然后重新生成项目代码（为了精简体积，本项目只包含必要的`main.c`文件，必须重新生成代码）
3. 修改`egui_port`目录下的显示、触控接口
4. 修改`CMakeLists.txt`文件，修改自己的源文件以及example对象
5. 编译
6. 烧录：脚本在`.vscode\tasks.json`中配置，修改`STM32_Programmer_CLI.exe`位置，然后在vscode顶部`终端`-`运行任务`，选择`CubeProg: Flash project (SWD)`烧录程序


> 使用arm-none-eabi-gcc，Release，编译`HelloSimple`例子资源占用：

| [build] Memory region     | Used Size | Region Size | %age Used |
| ------------------------- | --------- | ----------- | --------- |
| [build]              RAM: | 5600 B    | 128 KB      | 4.27%     |
| [build]           CCMRAM: | 0 B       | 64 KB       | 0.00%     |
| [build]            FLASH: | 43040 B   | 1 MB        | 4.10%     |