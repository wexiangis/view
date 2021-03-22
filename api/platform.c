#include <stdint.h>
#include <string.h>

#include "platform.h"

#define PLAT_OUTPUT_BMP "./screen.bmp"
#ifdef PLAT_OUTPUT_BMP
#include "bmp.h"
#endif

static uint8_t plat_map[PLAT_X_SIZE * PLAT_Y_SIZE * PLAT_PB];

//返回 PLAT_X_SIZE * PLAT_Y_SIZE * PLAT_PB 的内存
void *plat_map_init(void)
{
    return plat_map;
}

//使能输出(刷新屏幕)
void plat_map_en(void)
{
#ifdef PLAT_OUTPUT_BMP
    bmp_create(PLAT_OUTPUT_BMP, plat_map, PLAT_X_SIZE, PLAT_Y_SIZE, PLAT_PB);
#else
    ;
#endif
}
