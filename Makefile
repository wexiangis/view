
# 平台选择
#   0: 通用平台输出图像到framebuffer
#   1: T31平台
MAKE_PLATFORM = 0

##### 通用fb平台对接 #####
ifeq ($(MAKE_PLATFORM),0)
# cross = arm-linux-gnueabihf
# cross = arm-himix200-linux
# cross = arm-himix100-linux
DIR += ./platform/fb
endif

##### T31平台配置 #####
ifeq ($(MAKE_PLATFORM),1)
cross = mips-linux-gnu
DIR += ./platform/t31
CFLAG += -Wl,-gc-sections -lrt -ldl
# 使用 uclibc 时添加该项,但很多三方库不支持uclibc
# CFLAG += -muclibc
endif

# 静态编译(优先使用.a库文件编译,程序直接能跑,但文件巨大,部分库还不支持)
# CFLAG += -static

# 启用ttf字体支持 (0/不启用 1/启用)
MAKE_FREETYPE ?= 1
# 启用jpeg文件支持 (0/不启用 1/启用)
MAKE_JPEG ?= 1
# 启用png文件支持 (0/不启用 1/启用)
MAKE_PNG ?= 1
# 启用hiredis支持 (0/不启用 1/启用)
MAKE_HIREDIS ?= 1

# 根据 MAKE_XXX 统计要编译的库列表
ifeq ($(MAKE_FREETYPE),1)
BUILD += libfreetype
CFLAG += -lfreetype
INC += -I./libs/include/freetype2
endif
ifeq ($(MAKE_JPEG),1)
BUILD += libjpeg
CFLAG += -ljpeg
endif
ifeq ($(MAKE_PNG),1)
BUILD += libpng
CFLAG += -lz -lpng
endif
ifeq ($(MAKE_HIREDIS),1)
BUILD += libhiredis
CFLAG += -lhiredis
INC += -I./libs/include/hiredis
endif

# 传递宏定义给代码
DEF += -DMAKE_PLATFORM=$(MAKE_PLATFORM)
DEF += -DMAKE_FREETYPE=$(MAKE_FREETYPE)
DEF += -DMAKE_JPEG=$(MAKE_JPEG)
DEF += -DMAKE_PNG=$(MAKE_PNG)
DEF += -DMAKE_HIREDIS=$(MAKE_HIREDIS)

# 用于依赖库编译
GCC = gcc
GPP = g++
ifdef cross
	HOST = $(cross)
	GCC = $(cross)-gcc
	GPP = $(cross)-g++
endif

# 根目录获取
ROOT = $(shell pwd)

# 源文件包含
DIR += ./api
DIR += ./usr
DIR += ./usr/view
# 库文件路径 -L
LIB += -L./libs/lib
# 头文件路径 -I
INC += -I./libs/include
# 包含所有源文件夹
INC += $(foreach n,$(DIR),-I$(n))
# 其它编译参数
CFLAG += -Wall -lm -lpthread
# 遍历DIR统计所有.o文件
OBJS = $(foreach n,$(DIR),${patsubst %.c,$(n)/%.o,${notdir ${wildcard $(n)/*.c}}})

%.o: %.c
	@$(GCC) -c $< $(INC) $(LIB) $(CFLAG) $(DEF) -o $@

out: $(OBJS)
	@$(GCC) -o out $(OBJS) $(INC) $(LIB) $(CFLAG) $(DEF)

clean:
	@rm out $(OBJS) -rf

cleanall: clean
	@rm ./libs/* -rf

# 所有依赖库
libs: $(BUILD)
	@echo "---------- make libs complete !! ----------"

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
	./configure --prefix=$(ROOT)/libs --host=$(HOST) --enable-shared --with-bzip2=no --with-zlib=no --with-harfbuzz=no --with-png=no && \
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
