#ifndef _VIEWPLAT_H_
#define _VIEWPLAT_H_

//编译时启用的宏
#include "viewDef.h"

// 平台对接
#include "platform.h"
#define VIEW_X_SIZE PLAT_X_SIZE
#define VIEW_Y_SIZE PLAT_Y_SIZE
#define VIEW_PB PLAT_PB
#define VIEW_MAP_INIT() plat_map_init() //获取屏幕缓存指针
#define VIEW_MAP_EN() plat_map_en()     //使能输出

//边界判定用
#define VIEW_X_END (VIEW_X_SIZE - 1)
#define VIEW_Y_END (VIEW_Y_SIZE - 1)

//参考尺寸
#define VIEW_RESOLUTION (VIEW_X_SIZE > VIEW_Y_SIZE ? VIEW_Y_SIZE : VIEW_X_SIZE)
//根据当前屏幕大小 得出推荐字号
#define VIEW_RESOLUTION_PRINT (VIEW_RESOLUTION / 10) //字体大小

#endif // end of file
