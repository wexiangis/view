
#ifndef _TTFTYPE_H_
#define _TTFTYPE_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

//如果Makefile没有定义则自行定义
#include "viewDef.h"
#ifndef MAKE_FREETYPE
#define MAKE_FREETYPE 1
#endif

//字体文件路径
#define TTF_FILE "./ttf/Droid_Sans_Fallback.ttf"

typedef struct
{
    int width, height;       //目标输出点阵宽高(bitMap点阵就包含在内)
    int bitWidth, bitHeight; //bitMap点阵宽高
    int bitLeft, bitTop;     //bitMap点阵相对 width*height 点阵左上角偏移量
    int lineByte;            //bitMap每行可用字节数(会有盈余,比如 bitWidth=6,而 lineByte 会给2字节)
    uint8_t *bitMap;   //实际有效点阵数组,字节数为 bitHeight * lineByte
} Ttf_Map;

#if(MAKE_FREETYPE)

//主结构体初始化
void *ttf_init(char *ttfFile);
//清缓存
void ttf_clean(void *obj);
//释放主结构体
void ttf_release(void *obj);

/*
 *  功能: 根据文字的UTF-8编码, 返回该字的矩阵数据
 *  参数:
 *      obj: 初始化ttf获得的指针
 *      utf8: utf-8编码
 *      type: 字体代号,例如160表示16*16大小,0号线宽
 *      map: 返回内存块信息体指针
 *  返回: 识别的utf-8编码长度(ascii为1字符,汉字为3字符,用户根据该返回值来确定下次的偏移量),-1表示失败可以跳过该字符
 */
int ttf_getMapByUtf8(void *obj, char *utf8, int type, Ttf_Map *map);

/*
 *  根据utf8编码和字体代号,返回当前字符串最终输出时矩阵横向像素个数,用于确认当前输出的矩阵会不会超出屏幕或别的限制范围
 *  参数:
 *      utf8: 目标字符串
 *      type: 字体代号,例如160表示16*16大小,0号线宽
 *      xEdge, yEdge: 字与字之间横向、纵向间隔(像素)
 *      widthOfLine: 横向(一行)限制像素数量,超出时换行
 *      retW, retH: 返回最终占用点阵的宽和高
 *  返回: 横向字节数, -1表示失败
 */
void ttf_getSizeByUtf8_multiLine(void *obj, char *utf8, int type, int xEdge, int yEdge, int widthOfLine, int *retW, int *retH);

//单行模式
int ttf_getSizeByUtf8(void *obj, char *utf8, int type, int xEdge, int *retH);

#endif // #if(MAKE_FREETYPE)

#ifdef __cplusplus
}
#endif

#endif // _TTFTYPE_H_

/************************* demo *************************

#include <stdio.h>
#include <string.h>

#include "ttfType.h"

void draw(Ttf_Map map)
{
    //总字节
    int byteTotal = map.height * map.lineByte;
    //总字节计数
    int byteTotalCount;
    //一行字节计数
    int lineByteCount;
    //画点用
    uint8_t bit;
    int i;
    //横纵向点计数
    int x = 0, y = 0;

    //纵向起始跳过行数
    for (y = 0; y < map.bitTop; y++)
    {
        for (x = 0; x < map.width; x++)
            printf("_");
        //换行
        printf("\r\n");
    }
    //正文
    for (byteTotalCount = 0; byteTotalCount < byteTotal && y < map.height; y++)
    {
        //横向起始跳过点数
        for (x = 0; x < map.bitLeft; x++)
            printf("_");
        //正文
        for (lineByteCount = 0; lineByteCount < map.lineByte; lineByteCount++, byteTotalCount++)
        {
            //画一行的点(注意 map.lineByte 有盈余,需判断 x < map.width 保证不多画)
            for (i = 0, bit = map.bitMap[byteTotalCount]; i < 8 && x < map.width; i++, x++)
            {
                if (bit & 0x80)
                    printf("#");
                else
                    printf("_");
                bit <<= 1;
            }
        }
        //横向补足点数
        for (; x < map.width; x++)
            printf("_");
        //换行
        printf("\r\n");
    }
    //纵向补足行数
    for (; y < map.height; y++)
    {
        for (x = 0; x < map.width; x++)
            printf("_");
        //换行
        printf("\r\n");
    }
}

int main(void)
{
    int i, ret;
    char str[] = "123_abc-ABC 上下!";
    Ttf_Map map;
    void *obj = ttf_init(TTF_FILE);

    for(i = 0; i < strlen(str);)
    {
        //逐个字符解析,返回偏移量
        ret = ttf_getMapByUtf8(obj, &str[i], 240, &map);
        //字符信息
        printf("ret %d, addr %d, window %d x %d, bitMap %d x %d, err %d %d, lineByte %d\r\n",
            ret,
            i,
            map.width,
            map.height,
            map.bitWidth,
            map.bitHeight,
            map.bitLeft,
            map.bitTop,
            map.lineByte);
        //偏移地址(一般ascii偏移为1,中文为3)
        i += ret < 0 ? (-ret) : ret;
        //简易画图
        draw(map);
    }

    ttf_release(obj);
    return 0;
}

************************* demo *************************/
