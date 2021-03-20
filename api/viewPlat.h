#ifndef _PLATFORM_H_
#define _PLATFORM_H_

#define PLATFORM_FB 0  // 通用平台
#define PLATFORM_T31 1 // T31平台

// 如果Makefile没有定义则自行定义
#ifndef MAKE_PLATFORM
#define MAKE_PLATFORM PLATFORM_FB
#endif

// ===== T31平台对接 =====
#if(MAKE_PLATFORM == PLATFORM_T31)

#include "t31_plat.h"

#define VIEW_X_SIZE T31_X_SIZE
#define VIEW_Y_SIZE T31_Y_SIZE
#define VIEW_PB T31_PB

#define VIEW_MAP_INIT() t31_map_init()
#define VIEW_MAP_EN() t31_map_en()

// ===== 通用fb平台对接 =====
#else

#include "fb_plat.h"

#define VIEW_X_SIZE FB_X_SIZE
#define VIEW_Y_SIZE FB_Y_SIZE
#define VIEW_PB FB_PB

#define VIEW_MAP_INIT() fb_map_init()
#define VIEW_MAP_EN() fb_map_en()

#endif

//边界判定用
#define VIEW_X_END (VIEW_X_SIZE - 1)
#define VIEW_Y_END (VIEW_Y_SIZE - 1)

//参考尺寸
#define VIEW_RESOLUTION (VIEW_X_SIZE > VIEW_Y_SIZE ? VIEW_Y_SIZE : VIEW_X_SIZE)
//根据当前屏幕大小 得出推荐字号
#define VIEW_RESOLUTION_PRINT (VIEW_RESOLUTION / 10) //字体大小

#endif // end of file
