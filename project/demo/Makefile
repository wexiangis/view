#如果没有传入该参数,则使用该定义
GCC ?= gcc

# 根目录获取
ROOT = $(shell pwd)

# 依赖库位置
LIBS = $(ROOT)/../../libs

# -I
INC += -I$(LIBS)/include
# -L
LIB += -L$(LIBS)/lib
# -l 注意依赖顺序 -lui 要放到其它依赖库前面
CFLAG += -Wall -lui -lfreetype -ljpeg -lz -lpng -lhiredis -lpthread

# 本地源文件集合
SRC += ${wildcard $(ROOT)/*.c}

# 生成目标文件到上级目录
all:
	@$(GCC) -o $(ROOT)/../../demo $(SRC) $(INC) $(LIB) $(CFLAG)
