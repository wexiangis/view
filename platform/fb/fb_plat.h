#ifndef _FB_PLAT_H_
#define _FB_PLAT_H_

#define FB_X_SIZE 240
#define FB_Y_SIZE 240
#define FB_PB 3

//返回 FB_X_SIZE * FB_Y_SIZE * FB_PB 的内存
void *fb_map_init(void);
//使能输出(刷新屏幕)
void fb_map_en(void);

#endif
