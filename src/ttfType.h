
#ifndef _TTFTYPE_H
#define _TTFTYPE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "viewConfig.h"

#if(VIEW_CONFIG_PRINT_LIB == 1)

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_MULTIPLE_MASTERS_H
#include FT_GLYPH_H

#define TTF_FILE_ROOT  VIEW_CONFIG_TTF_ROOT

typedef struct{
    unsigned char *bitMap;
    int width, height;//bitMap点阵的实际宽/高
    int xErr, yErr;//绘制点阵时建议的横/纵向偏移量 绘制时xStart+xErr yStart+yErr即可
    int lineByte;//bitMap每行可用字节数
    int horMov;//绘制完该图后,建议的横向偏移量
}Ttf_Param;

int ttf_faceInit(FT_Face *face, FT_Library *library);
void ttf_faceRelease(FT_Face *face, FT_Library *library);

//返回: >0 已使用掉的字节数 <=0 当前字符解析失败 可跳过该字符
int ttf_getBitMapByUtf8(unsigned char *utf8_code, int type, Ttf_Param *param, FT_Face *face, FT_Library *library);

//接下来要输出的字或字符的宽度(像素)
void ttf_getRangeByUtf8_multiLine(unsigned char *utf8_code, int type, int xEdge, int yEdge, int widthLimit, int *retW, int *retH);
int ttf_getRangeByUtf8(unsigned char *utf8_code, int type, int xEdge, int *retH);

#endif //VIEW_CONFIG_PRINT_LIB

#ifdef __cplusplus
};
#endif

#endif
