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
CFLAG += -lui -lfreetype -ljpeg -lpng -lz -lpthread -lm -Wall
# 完全使用静态库时启用
CFLAG += -static

# 本地源文件集合
SRC += ${wildcard $(ROOT)/*.c}

# 生成目标文件到上级目录
target:
	@$(GCC) -o $(ROOT)/../../demo-app $(SRC) $(INC) $(LIB) $(CFLAG)
