#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "pngType.h"

#if (MAKE_PNG)
#include "libpng16/png.h"

/*
 *  png 图片数据获取
 *  参数:
 *      file: 路径
 *      width: 返回图片宽(像素), 不接收置NULL
 *      height: 返回图片高(像素), 不接收置NULL
 *      pixelBytes: 返回图片每像素的字节数, 不接收置NULL
 *  返回: argb图片数据指针,不透明~完全透明 !! 用完记得free()释放 !!
 */
uint8_t *png_get(char *file, int *width, int *height, int *pixelBytes)
{
    uint8_t *argb = NULL;
    png_image image;

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
        *pixelBytes = 4;

    argb = (uint8_t *)calloc(image.width * image.height * 4, 1);

    //设置目标格式
    image.format = PNG_FORMAT_ARGB;

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
 *      pixelBytes: 每像素字节数,3(rgb)或者4(argb)
 *  返回: 0成功 -1失败
 */
int png_create(char *file, uint8_t *argb, int width, int height, int pixelBytes)
{
    png_image image;

    memset(&image, 0, (sizeof image));
    image.version = PNG_IMAGE_VERSION;

    //参数准备
    image.width = width;
    image.height = height;
    image.format = pixelBytes == 4 ? PNG_FORMAT_ARGB : PNG_FORMAT_RGB;

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
