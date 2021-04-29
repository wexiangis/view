## 编译和运行

* make libs # 编译依赖库，并且编译api文件夹内文件为libui.a

* make api # api文件或者platform更改之后编译一次

* make demo # 编译演示程序

* ./demo-app # 运行演示程序,同时产生 screen.bmp 屏幕文件

## 运行环境部署

* make libs 之后,拷贝 ./libs/lib/\*.so\* 到 /usr/lib/

## 切换 framebuffer 输出

* 修改 Makefile 中 PLATFORM = fb
* 如果在 ubuntu 中运行记得 sudo

## 新的UI应用工程构建

* 参考 ./project/demo 文件夹,UI相关接口头文件在 ./libs/include/ui 或直接到 api 文件夹查看

## 新的平台底层接入

* 参考 platform 内 bmp 文件夹添加自己的文件夹,后修改 Makefile 中 PLATFORM 的值为新增文件夹名称

## api或platform文件更新之后

* make libui 或者 make api 可以更新 libui.a 文件,而后再编译自己的工程
