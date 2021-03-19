#include <stdint.h>
#include <string.h>

#include "fb_plat.h"

#define FB_OUTPUT_BMP "./test.bmp"
#ifdef FB_OUTPUT_BMP
#include "bmp.h"
#endif

typedef union {
    uint8_t map[FB_X_SIZE * FB_Y_SIZE * FB_PB];
    uint8_t rgb[FB_Y_SIZE][FB_X_SIZE][FB_PB];
} FB_Map;

static FB_Map fbmap;

void fb_init(void)
{
    ;
}

void fb_print_dot(int x, int y, int rgb)
{
    if (rgb < 0 ||
        x < 0 || x > FB_X_END ||
        y < 0 || y > FB_Y_END)
        return;

    fbmap.rgb[y][x][2] = (uint8_t)(rgb & 0xFF);
    rgb >>= 8;
    fbmap.rgb[y][x][1] = (uint8_t)(rgb & 0xFF);
    rgb >>= 8;
    fbmap.rgb[y][x][0] = (uint8_t)(rgb & 0xFF);
}

void fb_print_dot2(int x, int y, int rgb, float alpha)
{
    if (rgb < 0 ||
        x < 0 || x > FB_X_END ||
        y < 0 || y > FB_Y_END)
        return;

    fbmap.rgb[y][x][2] = (uint8_t)(fbmap.rgb[y][x][2] * alpha + (uint8_t)(rgb & 0xFF) * (1 - alpha));
    rgb >>= 8;
    fbmap.rgb[y][x][1] = (uint8_t)(fbmap.rgb[y][x][1] * alpha + (uint8_t)(rgb & 0xFF) * (1 - alpha));
    rgb >>= 8;
    fbmap.rgb[y][x][0] = (uint8_t)(fbmap.rgb[y][x][0] * alpha + (uint8_t)(rgb & 0xFF) * (1 - alpha));
}

void fb_print_en(void)
{
#ifdef FB_OUTPUT_BMP
    bmp_create(FB_OUTPUT_BMP, fbmap.map, FB_X_SIZE, FB_Y_SIZE, FB_PB);
#else
#endif
}

void fb_print_clean(int rgb)
{
    int x, y, c;

    uint8_t R = (uint8_t)((rgb & 0xFF0000) >> 16);
    uint8_t G = (uint8_t)((rgb & 0x00FF00) >> 8);
    uint8_t B = (uint8_t)(rgb & 0x0000FF);

    if (R == G && G == B)
        memset(fbmap.map, R, sizeof(fbmap));
    else
    {
        for (y = 0, c = 0; y < FB_Y_SIZE; y++)
        {
            for (x = 0; x < FB_X_SIZE; x++)
            {
                fbmap.map[c++] = R;
                fbmap.map[c++] = G;
                fbmap.map[c++] = B;
            }
        }
    }
}
