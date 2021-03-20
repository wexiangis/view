#include <stdint.h>
#include <string.h>

#include "t31_plat.h"

#define T31_OUTPUT_BMP "./test.bmp"
#ifdef T31_OUTPUT_BMP
#include "bmp.h"
#endif

static uint8_t t31_map[T31_X_SIZE * T31_Y_SIZE * T31_PB];

//返回 T31_X_SIZE * T31_Y_SIZE * T31_PB 的内存
void *t31_map_init(void)
{
    return t31_map;
}

//使能输出(刷新屏幕)
void t31_map_en(void)
{
#ifdef T31_OUTPUT_BMP
    bmp_create(T31_OUTPUT_BMP, t31_map, T31_X_SIZE, T31_Y_SIZE, T31_PB);
#else
    ;
#endif
}
