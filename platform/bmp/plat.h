#ifndef _PLAT_H_
#define _PLAT_H_

#ifdef __cplusplus
extern "C"
{
#endif

//颜色排列方式(不要改动)
typedef enum
{
    PLAT_COLOR_FORMAT_RGB = 0,
    PLAT_COLOR_FORMAT_BGR,
    PLAT_COLOR_FORMAT_RGBA,
    PLAT_COLOR_FORMAT_BGRA,
    PLAT_COLOR_FORMAT_ARGB,
    PLAT_COLOR_FORMAT_ABGR,
} PLAT_COLOR_FORMAT;

/*
 *  平台初始化
 *  参数: 获得屏幕宽、高、每像素字节数、颜色排列模式(如上定义)
 *  返回: width * height * pb 的内存
 */
void *plat_init(
    int *width,
    int *height,
    int *pb,
    PLAT_COLOR_FORMAT *format);

/*
 *  使能输出(刷新屏幕)
 */
void plat_enable(void);

#ifdef __cplusplus
}
#endif

#endif
