#include <stdint.h>
#include <string.h>

#include "plat.h"
#include "pngType.h" //在 api 文件夹内有该文件

#define PLAT_OUTPUT_PNG "./screen.png"

static uint8_t plat_map[PLAT_X_SIZE * PLAT_Y_SIZE * PLAT_PB];

/*
 *  平台初始化
 *  返回: FB_X_SIZE * FB_Y_SIZE * FB_PB 的内存
 */
void *plat_map_init(void)
{
    return plat_map;
}

/*
 *  使能输出(刷新屏幕)
 */
void plat_map_en(void)
{
    png_create(PLAT_OUTPUT_PNG, plat_map, PLAT_X_SIZE, PLAT_Y_SIZE, PT_BGRA);
}
