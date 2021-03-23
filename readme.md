## 编译和运行

* make libs # 编译依赖库、编译api文件夹为libui.a

* make demo # 编译演示程序

* ./demo # 运行演示程序,同时产生 screem.bmp 屏幕文件

## 运行环境部署

* make libs 之后,拷贝 ./libs/lib/lib*.so* 到 /usr/lib/

## 新的UI应用工程构建

* 参考 ./project/demo 文件夹,UI相关接口头文件在 ./libs/include/ui 或直接到 api 文件夹查看

## 替换新的平台底层

* 修改 ./api/platform.h 中的2个接口实现即可

## api文件更新之后

* make libui 编译更新 libui.a 文件,而后再编译自己的工程
