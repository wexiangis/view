
# 平台选择
#   0: 通用平台输出图像到framebuffer
#   1: T31平台
MAKE_PLATFORM = 0

##### 通用fb平台对接 #####
ifeq ($(MAKE_PLATFORM),0)
# cross = arm-linux-gnueabihf
endif

##### T31平台配置 #####
ifeq ($(MAKE_PLATFORM),1)
cross = mips-linux-gnu
DIR += ./platform/t31
CFLAG += -Wl,-gc-sections -lrt -ldl
CFLAG += -muclibc # 使用 uclibc 时添加该项
endif

# 启用ttf字体支持 (0/不启用 1/启用)
MAKE_FREETYPE ?= 1
# 启用jpeg文件支持 (0/不启用 1/启用)
MAKE_JPEG ?= 1
# 启用iconv用于UTF8文字检索支持 (0/不启用 1/启用)
MAKE_ICONV ?= 0

# 根据 MAKE_XXX 统计要编译的库列表
ifeq ($(MAKE_FREETYPE),1)
BUILD += libfreetype
CFLAG += -lfreetype
endif
ifeq ($(MAKE_JPEG),1)
BUILD += libjpeg
INC += -I./libs/include/freetype2
CFLAG += -ljpeg
endif
ifeq ($(MAKE_ICONV),1)
BUILD += libiconv
CFLAG += -liconv
endif

# 传递宏定义给代码
DEF += -DMAKE_PLATFORM=$(MAKE_PLATFORM)
DEF += -DMAKE_JPEG=$(MAKE_JPEG)
DEF += -DMAKE_ICONV=$(MAKE_ICONV)

# 用于依赖库编译
GCC = gcc
GXX = g++
ifdef cross
	HOST = $(cross)
	GCC = $(cross)-gcc
	GXX = $(cross)-g++
endif

# 根目录获取
ROOT = $(shell pwd)

# 源文件包含
DIR += ./src
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

%.o:%.c
	@$(GCC) -c $< $(INC) $(LIB) $(CFLAG) $(DEF) -o $@

app: $(OBJS)
	@$(GCC) -o app $(OBJS) $(INC) $(LIB) $(CFLAG) $(DEF)

clean:
	@rm app $(OBJS) -rf

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
	rm $(ROOT)/libs/lib/libjpeg.so* && \
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

# UTF8文字检索支持
libiconv:
	@tar -xzf $(ROOT)/pkg/libiconv-1.15.tar.gz -C $(ROOT)/libs && \
	cd $(ROOT)/libs/libiconv-1.15 && \
	./configure --prefix=$(ROOT)/libs --host=$(HOST) --enable-static --enable-shared && \
	make -j4 && make install && \
	cd - && \
	rm $(ROOT)/libs/libiconv-1.15 -rf
