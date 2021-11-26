
# 平台选择(即 platform 内的子文件夹)
#   bmp: 屏幕数据输出到 screen.bmp 文件,便于界面布局调试
#   png: 同上,只不过这是BGRA输出到 screen.png 文件
#   fb: 输出到标准 framebuffer 设备,注意 ubuntu 要 sudo 运行
#   其它: 自定义平台,自行往 platform 添加自己的文件夹
PLATFORM ?= bmp

# 交叉编译器选择
# cross = arm-linux-gnueabihf
# cross = arm-himix200-linux
# cross = arm-himix100-linux
# cross = mips-linux-gnu

# T31平台编译器额外配置
# CFLAG += -Wl,-gc-sections -lrt -ldl

# 用于依赖库编译
GCC = gcc
GPP = g++
AR = ar
ifdef cross
	HOST = $(cross)
	GCC = $(cross)-gcc
	GPP = $(cross)-g++
	AR = $(cross)-ar
endif

# 根目录获取
ROOT = $(shell pwd)

# 源文件包含
DIR += $(ROOT)/platform/$(PLATFORM)
DIR += $(ROOT)/api
# 头文件路径 -I
INC += -I$(ROOT)/libs/include
INC += -I$(ROOT)/libs/include/freetype2
INC += $(foreach n,$(DIR),-I$(n))
# 其它编译参数
CFLAG += -lfreetype -ljpeg -lpng -lhiredis -lz -lpthread -lm
CFLAG += -Wall

# 遍历DIR统计UI所有.o文件
OBJ += $(foreach n,$(DIR),${patsubst %.c,$(n)/%.o,${notdir ${wildcard $(n)/*.c}}})

%.o: %.c
	@$(GCC) -c $< $(INC) $(CFLAG) $(DEF) -o $@

# 在这里添加自己的工程编译跳转
demo:
	@cd $(ROOT)/project/$@ && \
	make GCC=$(GCC) && \
	cd -

# api文件或者platform更改之后编译一次
api: libui

clean:
	@rm -rf demo-app

cleanall: clean
	@rm -rf $(ROOT)/libs/*

# 所有依赖库
libs: libfreetype libpng libjpeg libhiredis libui
	@rm $(ROOT)/libs/lib/*.la && \
	echo "---------- make libs complete !! ----------"

libui: $(OBJ)
	@rm -rf $(ROOT)/libs/lib/libui.a && \
	rm -rf $(ROOT)/libs/include/ui && \
	ar r $(ROOT)/libs/lib/libui.a $(OBJ) && \
	rm -rf $(ROOT)/api/*.o && \
	rm -rf $(ROOT)/platform/$(PLATFORM)/*.o && \
	mkdir $(ROOT)/libs/include/ui -p && \
	cp -rf $(ROOT)/api/*.h $(ROOT)/libs/include/ui && \
	cp -rf $(ROOT)/platform/$(PLATFORM)/*.h $(ROOT)/libs/include/ui && \
	echo "output: $(ROOT)/libs/lib/libui.a"

# 用于辅助生成动态库的工具
libtool:
	@tar -xzf $(ROOT)/pkg/libtool-2.4.tar.gz -C $(ROOT)/libs && \
	cd $(ROOT)/libs/libtool-2.4 && \
	./configure --prefix=$(ROOT)/libs --host=$(HOST) --disable-ltdl-install && \
	make -j4 && make install && \
	cd - && \
	rm $(ROOT)/libs/libtool-2.4 -rf

# 编译libjpeg依赖libtool工具
libjpeg: libtool
	@rm $(ROOT)/libs/jpeg-6b -rf && \
	unzip $(ROOT)/pkg/jpegsr6.zip -d $(ROOT)/libs && \
	cd $(ROOT)/libs/jpeg-6b && \
	sed -i 's/\x0D//' ./configure && \
	sed -i 's/\x0D//' ./ltconfig && \
	./configure --prefix=$(ROOT)/libs --host=$(HOST) --enable-shared && \
	sed -i 's/CC= gcc/CC= $(GCC)/' ./Makefile && \
	cp $(ROOT)/libs/bin/libtool ./ && \
	mkdir $(ROOT)/libs/man/man1 -p && \
	make -j4 && make install && \
	sed -i '/^#define JPEGLIB_H/a\#include <stddef.h>' ../include/jpeglib.h && \
	cd - && \
	rm $(ROOT)/libs/jpeg-6b -rf

# 矢量字体(ttf)解析支持
libfreetype:
	@tar -xzf $(ROOT)/pkg/freetype-2.10.4.tar.gz -C $(ROOT)/libs && \
	cd $(ROOT)/libs/freetype-2.10.4 && \
	./configure --prefix=$(ROOT)/libs --host=$(HOST) --enable-static --with-bzip2=no --with-zlib=no --with-harfbuzz=no --with-png=no && \
	make -j4 && make install && \
	cd - && \
	rm $(ROOT)/libs/freetype-2.10.4 -rf

# libpng依赖库
zlib:
	@tar -xzf $(ROOT)/pkg/zlib-1.2.11.tar.gz -C $(ROOT)/libs && \
	cd $(ROOT)/libs/zlib-1.2.11 && \
	sed -i '1i\CC=$(GCC)' ./configure && \
	./configure --prefix=$(ROOT)/libs && \
	make -j4 && make install && \
	cd - && \
	rm $(ROOT)/libs/zlib-1.2.11 -rf

# UI怎么可以没有透明度文件
libpng: zlib
	@tar -xzf $(ROOT)/pkg/libpng-1.6.37.tar.gz -C $(ROOT)/libs && \
	cd $(ROOT)/libs/libpng-1.6.37 && \
	./configure --prefix=$(ROOT)/libs --host=$(HOST) LDFLAGS="-L$(ROOT)/libs/lib" CFLAGS="-I$(ROOT)/libs/include" CPPFLAGS="-I$(ROOT)/libs/include" && \
	make -j4 && make install && \
	cd - && \
	rm $(ROOT)/libs/libpng-1.6.37 -rf

# redis的c语言轻量版本
libhiredis:
	@tar -xzf $(ROOT)/pkg/hiredis-1.0.0.tar.gz -C $(ROOT)/libs && \
	cd $(ROOT)/libs/hiredis-1.0.0 && \
	sed -i '/^PREFIX?=/a\PREFIX=$(ROOT)/libs' ./Makefile && \
	sed -i '/^CC:=/a\CC=$(GCC)' ./Makefile && \
	sed -i '/^CXX:=/a\CXX=$(GPP)' ./Makefile && \
	make -j4 CFLAGS="-lm" && make install && \
	cd - && \
	rm $(ROOT)/libs/hiredis-1.0.0 -rf
