#include <stdint.h>
#include <string.h>

#include "fb_plat.h"

#define FB_OUTPUT_BMP "./test.bmp"
#ifdef FB_OUTPUT_BMP
#include "bmp.h"
#endif

static uint8_t fb_map[FB_X_SIZE * FB_Y_SIZE * FB_PB];

//返回 FB_X_SIZE * FB_Y_SIZE * FB_PB 的内存
void *fb_map_init(void)
{
    return fb_map;
}

//使能输出(刷新屏幕)
void fb_map_en(void)
{
#ifdef FB_OUTPUT_BMP
    bmp_create(FB_OUTPUT_BMP, fb_map, FB_X_SIZE, FB_Y_SIZE, FB_PB);
#else
    ;
#endif
}
