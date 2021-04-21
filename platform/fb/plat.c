#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>

#include "plat.h"
#include "filemap.h"

#define FB_DEV "/dev/fb0"
#define FB_X_SZIE 240
#define FB_Y_SZIE 240
#define FB_PB 4
#define FB_COLOR_FORMAT PLAT_COLOR_FORMAT_BGRA

static FbMap_Struct *fbmap;
static uint8_t *plat_map;

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
    //初始化fb设备
    fbmap = fbMap_open(FB_DEV);
    if (!fbmap)
    {
        fprintf(stderr, "plat_map_init: fbMap_open %s failed \r\n", FB_DEV);
        return NULL;
    }
    printf("plat_map_init: %s %d x %d x %d \r\n",
        FB_DEV, fbmap->fbInfo.xres, fbmap->fbInfo.yres, fbmap->bpp);

    *width = fbmap->fbInfo.xres;
    *height = fbmap->fbInfo.yres;
    *pb = fbmap->bpp;
    *format = FB_COLOR_FORMAT;

    //不直接使用fb的内存
    plat_map = calloc(fbmap->size, 1);
    return plat_map;
}

/*
 *  使能输出(刷新屏幕)
 */
void plat_enable(void)
{
    if (!fbmap)
        return;
    memcpy(fbmap->fb, plat_map, fbmap->size);
}
