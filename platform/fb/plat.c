#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "plat.h"
#include "filemap.h"

#define FB_DEV "/dev/fb0"

static FbMap_Struct *fbmap;
static uint8_t plat_map[PLAT_X_SIZE * PLAT_Y_SIZE * PLAT_PB];

//返回 PLAT_X_SIZE * PLAT_Y_SIZE * PLAT_PB 的内存
void *plat_map_init(void)
{
    //初始化fb设备
    fbmap = fbMap_open(FB_DEV);
    if (!fbmap)
        fprintf(stderr, "plat_map_init: fbMap_open %s failed \r\n", FB_DEV);
    else
        printf("plat_map_init: %s %d x %d x %d \r\n",
            FB_DEV, fbmap->fbInfo.xres, fbmap->fbInfo.yres, fbmap->bpp);
    //不直接使用fb的内存
    return plat_map;
}

//使能输出(刷新屏幕)
void plat_map_en(void)
{
    int i;
    int xSize, ySize, pbSize;

    if (!fbmap)
        return;

    //分辨率一致时直接拷贝
    if (fbmap->fbInfo.xres == PLAT_X_SIZE &&
        fbmap->fbInfo.yres == PLAT_Y_SIZE &&
        fbmap->bpp == PLAT_PB)
    {
        memcpy(fbmap->fb, plat_map, fbmap->size);
    }
    //不一致时按行拷贝
    else
    {
        //谁边小以谁为准
        ySize = fbmap->fbInfo.yres > PLAT_Y_SIZE ? PLAT_Y_SIZE : fbmap->fbInfo.yres;
        xSize = fbmap->fbInfo.xres > PLAT_X_SIZE ? PLAT_X_SIZE : fbmap->fbInfo.xres;
        pbSize = fbmap->bpp > PLAT_PB ? PLAT_PB : fbmap->bpp;
        //一行字节数
        xSize *= pbSize;
        //按行拷贝
        for (i = 0; i < ySize; i++)
            memcpy(&fbmap->fb[i * fbmap->bw], &plat_map[i * PLAT_X_SIZE * PLAT_PB], xSize);
    }
}
