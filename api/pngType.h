#ifndef _PNGTYPE_H_
#define _PNGTYPE_H_

#include <stdint.h>

typedef enum {
    PT_RGB = 0,
    PT_BGR,
    PT_RGBA,
    PT_BGRA,
    PT_ARGB,
    PT_ABGR,
} Png_Type;

//如果Makefile没有定义则自行定义
#include "viewDef.h"
#ifndef MAKE_PNG
#define MAKE_PNG 1
#endif

#if (MAKE_PNG)

/*
 *  png 图片数据获取
 *  参数:
 *      file: 路径
 *      width: 返回图片宽(像素), 不接收置NULL
 *      height: 返回图片高(像素), 不接收置NULL
 *      pixelBytes: 返回图片每像素的字节数, 不接收置NULL
 *  返回: argb图片数据指针,不透明~完全透明 !! 用完记得free()释放 !!
 */
uint8_t *png_get(char *file, int *width, int *height, int *pixelBytes, Png_Type pt);

/*
 *  生成 png 图片
 *  参数:
 *      file: 路径
 *      argb: 原始数据,根据pixelBytes值决定是否有a值
 *      width: 宽(像素)
 *      height: 高(像素)
 *  返回: 0成功 -1失败
 */
int png_create(char *file, uint8_t *argb, int width, int height, Png_Type pt);

#endif // #if(MAKE_PNG)

#endif // _PNGTYPE_H_
