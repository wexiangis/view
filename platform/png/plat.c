#include <stdint.h>
#include <string.h>

#include "plat.h"
#include "pngtype.h" //在 api 文件夹内有该文件

#define PNG_OUTPUT "./screen.png"
#define PNG_WIDTH 240
#define PNG_HEIGHT 240
#define PNG_PB 4
#define PNG_COLOR_FORMAT PLAT_COLOR_FORMAT_ARGB
#define PNG_TYPE PT_ARGB

static uint8_t plat_map[PNG_WIDTH * PNG_HEIGHT * PNG_PB];

/*
 *  平台初始化
 *  参数: 获得屏幕宽、高、每像素字节数、颜色排列模式(如上定义)
 *  返回: width * height * pb 的内存
 */
void *plat_init(
    int *width,
    int *height,
    int *pb,
    PLAT_COLOR_FORMAT *format)
{
    *width = PNG_WIDTH;
    *height = PNG_HEIGHT;
    *pb = PNG_PB;
    *format = PNG_COLOR_FORMAT;
    return plat_map;
}

/*
 *  使能输出(刷新屏幕)
 */
void plat_enable(void)
{
    png_create(PNG_OUTPUT, plat_map, PNG_WIDTH, PNG_HEIGHT, PNG_TYPE);
}
