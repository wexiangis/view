/*
 *  jpeg文件解析,依赖 libjpeg
 */
#ifndef _JPEGTYPE_H_
#define _JPEGTYPE_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

// -------------------------- 文件数据整读整写模式 --------------------------

/*
 *  jpeg 图片数据获取
 *  参数:
 *      inFile: 路径
 *      width: 返回图片宽(像素), 不接收置NULL
 *      height: 返回图片高(像素), 不接收置NULL
 *      pixelBytes: 返回图片每像素的字节数, 不接收置NULL
 *  返回: rgb图片数据指针, 已分配内存 !! 用完记得free()释放 !!
 */
uint8_t *jpeg_get(char *inFile, int *width, int *height, int *pixelBytes);

/*
 *  生成 jpeg 图片
 *  参数:
 *      outFile: 路径
 *      rgb: 原始数据
 *      width: 宽(像素)
 *      height: 高(像素)
 *      pixelBytes: 每像素字节数
 *      quality: 压缩质量,1~100,越大越好,文件越大
 *  返回: 0成功 -1失败
 */
int jpeg_create(char *outFile, uint8_t *rgb, int width, int height, int pixelBytes, int quality);

// -------------------------- 行数据流处理模式 --------------------------

/*
 *  行处理模式
 *  参数: 同上
 *  返回: 行处理指针,NULL失败
 */
void *jpeg_getLine(char *inFile, int *width, int *height, int *pixelBytes);

/*
 *  行处理模式
 *  参数: 同上
 *  返回: 行处理指针,NULL失败
 */
void *jpeg_createLine(char *outFile, int width, int height, int pixelBytes, int quality);

/*
 *  按行rgb数据读、写
 *  参数:
 *      obj: 行处理指针
 *      rgbLine: 行rgb数据,一行长度为width*height*pixelBytes,多行时继续加
 *      line: 要处理的行数
 *  返回:
 *      写图片时返回成功写入行,
 *      读图片时返回实际读取行数,
 */
int jpeg_line(void *obj, uint8_t *rgbLine, int line);

/*
 *  完毕释放指针
 */
void jpeg_closeLine(void *obj);

// -------------------------- 直接文件缩放 --------------------------

/*
 *  文件缩放
 *  参数:
 *      inFile, outFile: 输入输出文件,类型.jpg.jpeg.JPG.JPEG
 *      zoom: 缩放倍数,0.1到1为缩放,1.0以上放大
 *      quality: 输出图片质量,1~100,越大越好,文件越大
 */
void jpeg_zoom(char *inFile, char *outFile, float zoom, int quality);

#ifdef __cplusplus
}
#endif

#endif // _JPEGTYPE_H_
