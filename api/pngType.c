#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "pngType.h"

#if (MAKE_PNG)
#include "libpng16/png.h"

//读取图片时以255作为最大透明度
#define PNG_ALPHA_MAX_BY_255_ON_READ
//写图片时以255作为最大透明度
#define PNG_ALPHA_MAX_BY_255_ON_WRITE

static uint32_t png_types[] = {
    PNG_FORMAT_RGB,
    PNG_FORMAT_BGR,
    PNG_FORMAT_RGBA,
    PNG_FORMAT_BGRA,
    PNG_FORMAT_ARGB,
    PNG_FORMAT_ABGR};

/*
 *  png 图片数据获取
 *  参数:
 *      file: 路径
 *      width: 返回图片宽(像素), 不接收置NULL
 *      height: 返回图片高(像素), 不接收置NULL
 *      pixelBytes: 返回图片每像素的字节数, 不接收置NULL
 *  返回: argb图片数据指针,不透明~完全透明 !! 用完记得free()释放 !!
 */
uint8_t *png_get(char *file, int *width, int *height, int *pixelBytes, Png_Type pt)
{
    uint8_t *argb = NULL;
    png_image image;
#ifdef PNG_ALPHA_MAX_BY_255_ON_READ
    int i;
#endif

    memset(&image, 0, (sizeof image));
    image.version = PNG_IMAGE_VERSION;

    //解析文件
    if (!png_image_begin_read_from_file(&image, file))
    {
        fprintf(stderr, "png_image_begin_read_from_file %s failed \r\n", file);
        goto exit;
    }

    //返回参数
    if (width)
        *width = image.width;
    if (height)
        *height = image.height;
    if (pixelBytes)
        *pixelBytes = pt > PT_BGR ? 4 : 3;

    argb = (uint8_t *)calloc(image.width * image.height * 4, 1);

    //设置目标格式
    image.format = png_types[pt];

    //开始读取和转换数据
    if (!png_image_finish_read(
            &image,
            NULL, // background
            argb,
            0,     //row_stride
            NULL)) //colormap
    {
        fprintf(stderr, "png_image_finish_read failed \r\n");
    }
#ifdef PNG_ALPHA_MAX_BY_255_ON_READ
    else if (pt > PT_BGR)
    {
        //对透明度进行取反,即255时为完全透明
        for (i = pt > PT_BGRA ? 0 : 3; i < image.width * image.height * 4; i += 4)
            argb[i] = ~argb[i];
    }
#endif

exit:
    return argb;
}

/*
 *  生成 png 图片
 *  参数:
 *      file: 路径
 *      argb: 原始数据,根据pixelBytes值决定是否有a值
 *      width: 宽(像素)
 *      height: 高(像素)
 *  返回: 0成功 -1失败
 */
int png_create(char *file, uint8_t *argb, int width, int height, Png_Type pt)
{
    int i;
    png_image image;

    memset(&image, 0, (sizeof image));
    image.version = PNG_IMAGE_VERSION;

    //参数准备
    image.width = width;
    image.height = height;
    image.format = png_types[pt];

#ifdef PNG_ALPHA_MAX_BY_255_ON_WRITE
    //翻转透明度数值
    if (pt == PT_RGBA || pt == PT_BGRA)
    {
        for (i = 0; i < width * height * 4;)
        {
            i += 3;
            argb[i] = ~argb[i];
            i += 1;
        }
    }
    else if (pt == PT_ARGB || pt == PT_ABGR)
    {
        for (i = 0; i < width * height * 4;)
        {
            argb[i] = ~argb[i];
            i += 4;
        }
    }
#endif

    if (png_image_write_to_file(
            &image,
            file,
            0, //convert_to_8bit
            argb,
            0,     //row_stride
            NULL)) //colormap
        return 0;

    return -1;
}

#endif // #if(MAKE_PNG)
