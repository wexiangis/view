#include <stdint.h>
#include <string.h>

#include "plat.h"
#include "bmp.h" //在 api 文件夹内有该文件

#define BMP_OUTPUT "./screen.bmp"
#define BMP_WIDTH 240
#define BMP_HEIGHT 240
#define BMP_PB 3
#define BMP_COLOR_FORMAT PLAT_COLOR_FORMAT_RGB

static uint8_t plat_map[BMP_WIDTH * BMP_HEIGHT * BMP_PB];

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
    *width = BMP_WIDTH;
    *height = BMP_HEIGHT;
    *pb = BMP_PB;
    *format = BMP_COLOR_FORMAT;
    return plat_map;
}

/*
 *  使能输出(刷新屏幕)
 */
void plat_enable(void)
{
    bmp_create(BMP_OUTPUT, plat_map, BMP_WIDTH, BMP_HEIGHT, BMP_PB);
}
