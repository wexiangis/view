#ifndef _PLAT_H_
#define _PLAT_H_

#ifdef __cplusplus
extern "C"
{
#endif

//颜色排列方式
#define COLOR_FORMAT_RGB 0
#define COLOR_FORMAT_BGR 1
#define COLOR_FORMAT_RGBA 2
#define COLOR_FORMAT_BGRA 3
#define COLOR_FORMAT_ARGB 4
#define COLOR_FORMAT_ABGR 5

//颜色排列方式
#define PLAT_FORMAT COLOR_FORMAT_RGB
//屏幕宽高
#define PLAT_X_SIZE 240
#define PLAT_Y_SIZE 240
//每像素字节数
#define PLAT_PB 3

/*
 *  平台初始化
 *  返回: FB_X_SIZE * FB_Y_SIZE * FB_PB 的内存
 */
void *plat_map_init(void);

/*
 *  使能输出(刷新屏幕)
 */
void plat_map_en(void);

#ifdef __cplusplus
}
#endif

#endif

