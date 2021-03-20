#include <stdio.h>
#include <sys/time.h>

#if 0

int main(void)
{
    printf("hello !!\r\n");
    return 0;
}

#elif 0

#include <stdlib.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "pngType.h"
#include "bmp.h"
#include "png.h"

int main(void)
{
#if 0
    int i, j;
    uint8_t *argb;
    png_image image;

    memset(&image, 0, (sizeof image));
    image.version = PNG_IMAGE_VERSION;

    if (png_image_begin_read_from_file(&image, "./usr/src/signal.png"))
    {
        image.format = PNG_FORMAT_ARGB;

        argb = malloc(PNG_IMAGE_SIZE(image));

        if (png_image_finish_read(
            &image,
            NULL/*background*/,
            argb,
            0/*row_stride*/,
            NULL/*colormap*/))
        {
            // png_create("./test.png", argb, image.width, image.height, 4);
            png_image_write_to_file(
                &image,
                "./test.png",
                0/*convert_to_8bit*/,
               argb,
               0/*row_stride*/,
               NULL/*colormap*/);

            for (i = j = 0; j < image.width * image.height * 4;)
            {
                j++;
                argb[i++] = argb[j++];
                argb[i++] = argb[j++];
                argb[i++] = argb[j++];
            }

            bmp_create("./test.bmp", argb, image.width, image.height, 3);
        }

        free(argb);
    }

#elif 1
    uint8_t *argb;
    int width = 0, height = 0, pixelBytes = 0;
    int i, j;

    argb = png_get("./usr/src/signal.png", &width, &height, &pixelBytes);

    if (argb)
    {
        png_create("./test.png", argb, width, height, pixelBytes);

        for (i = j = 0; j < width * height * pixelBytes;)
        {
            if (j % (24 * 4) == 0)
                printf("\r\n");
            printf("%03d ", argb[j]);
            j++;
            argb[i++] = argb[j++];
            argb[i++] = argb[j++];
            argb[i++] = argb[j++];
        }

        bmp_create("./test.bmp", argb, width, height, 3);
        png_create("./test2.png", argb, width, height, 3);

        free(argb);
    }
#else
    uint8_t *rgb;
    uint8_t argb[24 * 24 * 4] = {0};
    int width = 0, height = 0, pixelBytes = 0;
    int i, j;

    rgb = bmp_get("./usr/src/edit.bmp", &width, &height, &pixelBytes);
    printf("%d x %d x %d \r\n", width, height, pixelBytes);

    if (rgb)
    {
        for (i = j = 0; j < width * height * pixelBytes;)
        {
            argb[i++] = rgb[j++];
            argb[i++] = rgb[j++];
            argb[i++] = rgb[j++];
            argb[i++] = 50;
        }

        png_create("./test.png", argb, width, height, 4);
        // png_create("./test.png", rgb, width, height, 3);

        free(rgb);
    }

#endif
    return 0;
}

#else

#include "viewApi.h"
#include "viewInc.h"

// 获取系统tick可以作为us时长参考
long int getTickUs(void)
{
    struct timeval tv = {0};
    gettimeofday(&tv, NULL);
    return tv.tv_sec * 1000000u + tv.tv_usec;
}

void delayUs(long int us)
{
    struct timeval tv;
    tv.tv_sec = us / 1000000;
    tv.tv_usec = us % 1000000;
    select(0, NULL, NULL, NULL, &tv);
}

// 延时初始化
#define DELAY_INIT \
    long int _tick1 = 0, _tick2;

// 检查时差,差多少延时多少
#define DELAY_US(us)                             \
    _tick2 = getTickUs();                        \
    if (_tick2 > _tick1 && _tick2 - _tick1 < us) \
        delayUs(us - (_tick2 - _tick1));         \
    _tick1 = getTickUs();

int main(void)
{
    View_Struct *vsMain;
    View_Struct *vsBar;
    View_Struct *vsWin1;
    View_Struct *vsWin2;

    DELAY_INIT;

    //UI系统初始化(底层平台初始化)
    view_init();

    //view初始化
    vsMain = view_main_init();
    vsBar = view_bar_init();
    vsWin1 = view_win1_init();
    vsWin2 = view_win2_init();

    //状态栏作为常驻控件依附在主窗体上
    view_add(vsMain, vsBar, false);

    //vsWin1 和 vsWin2 通过跳转指针来引入主窗体
    vsMain->view->jumpView = vsWin1;

    //循环画view
    while (1)
    {
        DELAY_US(REFRESH_MS * 1000);
        //清屏
        print_clean(ViewColor.Blue.value.Int);
        //绘制view链表
        view_draw(NULL, NULL, NULL, NULL, vsMain, NULL);
        //刷新屏幕
        print_en();
    }

    return 0;
}
#endif