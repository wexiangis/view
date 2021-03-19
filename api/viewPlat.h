#ifndef _PLATFORM_H_
#define _PLATFORM_H_

#define PLATFORM_FB 0  // 通用平台
#define PLATFORM_T31 1 // T31平台

// 如果Makefile没有定义则自行定义
#ifndef MAKE_PLATFORM
#define MAKE_PLATFORM PLATFORM_FB
#endif

// ===== T31平台对接 =====
#if (MAKE_PLATFORM == PLATFORM_T31)

#define VIEW_X_SIZE 240
#define VIEW_Y_SIZE 240
#define VIEW_PB 3

// ===== 通用fb平台对接 =====
#else

#include "fb_plat.h"

#define VIEW_X_SIZE FB_X_SIZE
#define VIEW_Y_SIZE FB_Y_SIZE
#define VIEW_PB FB_PB

#define PLAT_INIT()                  fb_init()
#define PRINT_DOT(x, y, rgb)         fb_print_dot(x, y, rgb)
#define PRINT_DOT2(x, y, rgb, alpha) fb_print_dot2(x, y, rgb, alpha)
#define PRINT_EN()                   fb_print_en()
#define PRINT_CLEAR(rgb)             fb_print_clean(rgb)

#endif

#define VIEW_X_END (VIEW_X_SIZE - 1)
#define VIEW_Y_END (VIEW_Y_SIZE - 1)

//参考尺寸
#define VIEW_RESOLUTION (VIEW_X_SIZE > VIEW_Y_SIZE ? VIEW_Y_SIZE : VIEW_X_SIZE)
//根据当前屏幕大小 得出推荐字号
#define VIEW_RESOLUTION_PRINT (VIEW_RESOLUTION / 10) //字体大小

#endif // end of file
