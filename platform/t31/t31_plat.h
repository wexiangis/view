#ifndef _T31_PLAT_H_
#define _T31_PLAT_H_

#define T31_X_SIZE 240
#define T31_Y_SIZE 240
#define T31_PB 3

//返回 T31_X_SIZE * T31_Y_SIZE * T31_PB 的内存
void *t31_map_init(void);
//使能输出(刷新屏幕)
void t31_map_en(void);

#endif
