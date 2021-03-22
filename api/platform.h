#ifndef _PLATFORM_H_
#define _PLATFORM_H_

#define PLAT_X_SIZE 240
#define PLAT_Y_SIZE 240
#define PLAT_PB 3

//返回 FB_X_SIZE * FB_Y_SIZE * FB_PB 的内存
void *plat_map_init(void);
//使能输出(刷新屏幕)
void plat_map_en(void);

#endif

