#ifndef _FB_PLAT_H_
#define _FB_PLAT_H_

#define FB_X_SIZE 240
#define FB_Y_SIZE 240
#define FB_X_END (FB_X_SIZE - 1)
#define FB_Y_END (FB_Y_SIZE - 1)
#define FB_PB 3

void fb_print_dot(int x, int y, int rgb);
void fb_print_dot2(int x, int y, int rgb, float alpha);
void fb_print_en(void);
void fb_print_clean(int rgb);

void fb_init(void);

#endif
