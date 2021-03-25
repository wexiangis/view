
#include <stdio.h>
#include <unistd.h>

#include "ttfType.h"

#if(MAKE_FREETYPE)

#include "ft2build.h"
#include FT_FREETYPE_H
#include FT_MULTIPLE_MASTERS_H
#include FT_GLYPH_H
#include FT_OUTLINE_H

//基线高=height/8
#define BASELINE_OF_HEIGHT 8

//ascii输出时的宽高比例
#define ASCII_WIDTH_OF_HEIGHT(h) (h * 4 / 5)

//显示统计信息
// #define TTF_CACHE_STATICS

//不带参 和 带参 报错打印
#define TTF_ERR(fmt) fprintf(stderr, "%s(%d): " fmt, __func__, __LINE__)
#define TTF_ERR2(fmt, argv...) fprintf(stderr, "%s(%d): " fmt, __func__, __LINE__, ##argv)

//二级缓存链表,每个节点记录不同unicode的字
typedef struct TtfWordStruct
{
    FT_ULong unicode; //编码
    Ttf_Map map;      //内存块
    struct TtfWordStruct *next;
} TtfCache_Word;

//一级缓存链表,每个节点记录不同type的字型
typedef struct TtfTypeStruct
{
    int type;
    TtfCache_Word *word;  //文字链表(中文)
    TtfCache_Word *ascii; //ascii链表
#ifdef TTF_CACHE_STATICS
    int total;
#endif
    struct TtfTypeStruct *next;
} TtfCache_Type;

//主结构体
typedef struct
{
    FT_Face face;
    FT_Library lib;
    TtfCache_Type *cacheType; //缓存链表头
} Ttf_Struct;

//主结构体初始化
void *ttf_init(char *ttfFile)
{
    Ttf_Struct *ts;

    if (!ttfFile)
        return NULL;

    ts = (Ttf_Struct *)calloc(1, sizeof(Ttf_Struct));

    //载入ttf字体库
    if (FT_Init_FreeType(&ts->lib) != 0)
    {
        TTF_ERR("FT_Init_FreeType err \r\n");
        goto err1;
    }
    //创建一个face
    if (FT_New_Face(ts->lib, ttfFile, 0, &ts->face) != 0)
    {
        TTF_ERR("FT_New_Face err \r\n");
        goto err2;
    }
    return ts;
err2:
    FT_Done_Face(ts->face);
err1:
    free(ts);
    return NULL;
}

//清缓存
void ttf_clean(void *obj)
{
    Ttf_Struct *ts = (Ttf_Struct *)obj;
    TtfCache_Type *cType, *cTypeNext;
    TtfCache_Word *cWord, *cWordNext;

    // 禁止异步!!
    static uint8_t head = 0, tail = 0;
    uint8_t take = 0;
    take = tail++;
    while (take != head)
        usleep(1000);

    if (!ts)
        return;

    //一级链表
    cType = ts->cacheType;
    while (cType)
    {
        cTypeNext = cType->next;
        //释放文字链表(二级链表)
        cWord = cType->word;
        while (cWord)
        {
            cWordNext = cWord->next;
            //释放节点
            if (cWord->map.bitMap)
                free(cWord->map.bitMap);
            free(cWord);
            //下一个
            cWord = cWordNext;
        }
        //释放ascii链表(二级链表)
        cWord = cType->ascii;
        while (cWord)
        {
            cWordNext = cWord->next;
            //释放节点
            if (cWord->map.bitMap)
                free(cWord->map.bitMap);
            free(cWord);
            //下一个
            cWord = cWordNext;
        }
        //释放节点
        free(cType);
        //下一个
        cType = cTypeNext;
    }
    //清理完毕标志
    ts->cacheType = NULL;

    head += 1;
}

void ttf_release(void *obj)
{
    Ttf_Struct *ts = (Ttf_Struct *)obj;
    if (!ts)
        return;
    ttf_clean(ts);
    if (ts->face)
        FT_Done_Face(ts->face);
    if (ts->lib)
        FT_Done_FreeType(ts->lib);
    free(ts);
}

/*
 *  根据type、unicode码、utf8占用字节数,检索已有内存块
 *  返回: 无匹配返回NULL
 */
static TtfCache_Word *ttf_findWord(TtfCache_Type **cacheType, int type, FT_ULong unicode, int byteNum)
{
    static uint8_t head = 0, tail = 0;
    uint8_t take = 0;

    TtfCache_Word *cWord = NULL;
    TtfCache_Type *cType = NULL;

    // 禁止异步!!
    take = tail++;
    while (take != head)
        usleep(1000);

    //新建或取链表头
    if (*cacheType == NULL)
        cType = *cacheType = (TtfCache_Type *)calloc(1, sizeof(TtfCache_Type));
    else
        cType = *cacheType;

    //匹配 type
    while (cType->next)
    {
        if (cType->type == type)
            break;
        cType = cType->next;
    }

    //新 type
    if (cType->type != type)
    {
#ifdef TTF_CACHE_STATICS
        printf("ttf new type [%d]\n", type);
#endif
        //新建节点
        cType->next = (TtfCache_Type *)calloc(1, sizeof(TtfCache_Type));
        cType = cType->next;
        cType->type = type;
#ifdef TTF_CACHE_STATICS
        printf("ttf new word [%d %d] %ld\n", type, cType->total, unicode);
        cType->total += 1;
#endif
        //新建 word 节点
        if (byteNum == 1)
            cWord = cType->ascii = (TtfCache_Word *)calloc(1, sizeof(TtfCache_Word));
        else
            cWord = cType->word = (TtfCache_Word *)calloc(1, sizeof(TtfCache_Word));
        cWord->unicode = unicode;
    }
    //成功匹配 type
    else
    {
        //新建或取 type 链表头
        if (byteNum == 1)
        {
            if (cType->ascii)
                cWord = cType->ascii;
            else
                cWord = cType->ascii = (TtfCache_Word *)calloc(1, sizeof(TtfCache_Word));
        }
        else
        {
            if (cType->word)
                cWord = cType->word;
            else
                cWord = cType->word = (TtfCache_Word *)calloc(1, sizeof(TtfCache_Word));
        }
        //匹配 wrod
        while (cWord->next)
        {
            if (cWord->unicode == unicode)
                break;
            cWord = cWord->next;
        }
        //新 wrod
        if (cWord->unicode != unicode)
        {
#ifdef TTF_CACHE_STATICS
            printf("new word [%d %d] %ld\n", type, cType->total, unicode);
            cType->total += 1;
#endif
            //新建 word 节点
            cWord->next = (TtfCache_Word *)calloc(1, sizeof(TtfCache_Word));
            cWord = cWord->next;
            cWord->unicode = unicode;
        }
    }

    //异步下一个
    head += 1;

    //返回匹配项
    return cWord;
}

/*
 *  utf8转unicode
 *  返回: 使用掉(数组utf8)的字节数，失败返回-1
 */
static int ttf_utf8ToUnicode(uint8_t *utf8, FT_ULong *unicode)
{
    // ------unicode-------|-----utf8-----
    // 0000 0000-0000 007F | 0xxxxxxx
    // 0000 0080-0000 07FF | 110xxxxx 10xxxxxx
    // 0000 0800-0000 FFFF | 1110xxxx 10xxxxxx 10xxxxxx
    // 0001 0000-0010 FFFF | 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx

    if (utf8[0] < ' ')
    {
        *unicode = ' ';
        return -1;
    }
    else if (utf8[0] < 0x80)
    {
        *unicode = utf8[0];
        return 1;
    }
    else if (utf8[0] < 0xE0 && utf8[1] > 0x7F)
    {
        *unicode = ((utf8[0] & 0x1F) << 6) | (utf8[1] & 0x3F);
        return 2;
    }
    else if (utf8[0] < 0xF0 && utf8[1] > 0x7F && utf8[2] > 0x7F)
    {
        *unicode = ((utf8[0] & 0x0F) << 12) | ((utf8[1] & 0x3F) << 6) | (utf8[2] & 0x3F);
        return 3;
    }
    else if (utf8[0] < 0xF8 && utf8[1] > 0x7F && utf8[2] > 0x7F && utf8[3] > 0x7F)
    {
        *unicode = ((utf8[0] & 0x07) << 18) | ((utf8[1] & 0x3F) << 12) | ((utf8[2] & 0x3F) << 6) | (utf8[3] & 0x3F);
        return 4;
    }
    else
    {
        TTF_ERR2("ttf_utf8ToUnicode : utf8 code err %02X %02X %02X %02X\r\n",
                 utf8[0], utf8[1], utf8[2], utf8[3]);
        return -1;
    }
}

/*
 *  根据type、unicode计算当前字型的宽高(不是最终内存的宽高)
 *  返回: 线宽
 */
static int ttf_getBasicByType(int type, FT_ULong unicode, int *width, int *height, int *baseLineHeight)
{
    *height = type / 10;
    if (*height == 0)
        return -1;
    if (unicode < 0x80)
        *width = ASCII_WIDTH_OF_HEIGHT(*height);
    else
        *width = *height;
    *baseLineHeight = *height / BASELINE_OF_HEIGHT;
    return type % 10;
}

/*
 *  功能: 根据文字的UTF-8编码, 返回该字的矩阵数据
 *  参数:
 *      obj: 初始化ttf获得的指针
 *      utf8: utf8编码
 *      type: 字体代号,例如160表示16*16大小,0号线宽
 *      map: 返回内存块信息体指针
 *  返回: 识别的utf8编码长度(ascii为1字符,汉字为3字符,用户根据该返回值来确定下次的偏移量),-1表示失败可以跳过该字符
 */
int ttf_getMapByUtf8(void *obj, char *utf8, int type, Ttf_Map *map)
{
    Ttf_Struct *ts = (Ttf_Struct *)obj;
    //一次解析后的字型信息
    FT_GlyphSlot glyph;
    //解析的unicode码
    FT_ULong unicode = 0;
    //字体参数
    int width = 0, height = 0, size = 0;
    //基线高
    int baseLineHeight = 0;
    //最终返回
    int retByte = -1;
    //缓存中或将写入缓存的字型内存块
    TtfCache_Word *cWord = NULL;

    // 禁止异步!!
    static uint8_t head = 0, tail = 0;
    uint8_t take = 0;
    take = tail++;
    while (take != head)
        usleep(1000);

    if (!ts)
        return -1;

    //utf8 转 unicode
    if ((retByte = ttf_utf8ToUnicode((uint8_t *)utf8, &unicode)) < 0)
        goto exit;

    //从缓存检索字型内存块
    cWord = ttf_findWord(&ts->cacheType, type, unicode, retByte);
    if (cWord && cWord->map.bitMap)
    {
        //匹配后直接赋值(这里其实是结构体之间的直接赋值)
        *map = cWord->map;
        goto exit;
    }

    //宽/高/加粗
    size = ttf_getBasicByType(type, unicode, &width, &height, &baseLineHeight);

    //设置字体尺寸
    if (FT_Set_Char_Size(ts->face, width << 6, height << 6, 72, 72) != 0)
    {
        TTF_ERR("FT_Set_Char_Size failed \r\n");
        goto err;
    }
    //从字符码检索字形索引
    //装载字形图像到字形槽（将会抹掉先前的字形图像）
    if (FT_Load_Glyph(ts->face, FT_Get_Char_Index(ts->face, unicode), FT_LOAD_DEFAULT) != 0)
    {
        TTF_ERR("FT_Load_Glyph failed \r\n");
        goto err;
    }
    glyph = ts->face->glyph;

    //建议的横向移动量
    cWord->map.width = glyph->advance.x >> 6;
    cWord->map.height = height;

    //加粗
    if (FT_Outline_Embolden(&glyph->outline, size << 6) != 0) // *0 不加粗 *n加粗量
    {
        TTF_ERR("FT_Outline_Embolden failed \r\n");
        goto err;
    }

    //转换为一个抗锯齿位图
    //ft_render_mode_normal:灰度图, ft_render_mode_mono:黑白位图
    if (FT_Render_Glyph(glyph, ft_render_mode_mono) == 0)
    {
        //备份该字型数据
        cWord->map.bitMap = (uint8_t *)calloc((glyph->bitmap.rows + 1) * glyph->bitmap.pitch, 1);
        memcpy(cWord->map.bitMap, glyph->bitmap.buffer, glyph->bitmap.rows * glyph->bitmap.pitch);
        //信息拷贝
        cWord->map.bitWidth = glyph->bitmap.width;
        cWord->map.bitHeight = glyph->bitmap.rows;
        cWord->map.bitLeft = glyph->bitmap_left;
        cWord->map.bitTop = height - glyph->bitmap_top - baseLineHeight;
        cWord->map.lineByte = glyph->bitmap.pitch;
        //返回指针
        *map = cWord->map;
    }
    else
        map->bitMap = NULL;

exit:
    head += 1;
    return retByte;
err:
    head += 1;
    return -1;
}

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
void ttf_getSizeByUtf8_multiLine(void *obj, char *utf8, int type, int xEdge, int yEdge, int widthOfLine, int *retW, int *retH)
{
    Ttf_Struct *ts = (Ttf_Struct *)obj;
    //一次解析后的字型信息
    FT_GlyphSlot glyph;

    FT_ULong unicode = 0;
    int width = 0, height = 0, size = 0;
    int baseLineHeight = 0;
    int retByte = -1;

    int retWidth = 0, charWidth = 0, retWidthMax = 0;
    int retHeight = type / 10;
    int typeSize = type / 10;

    TtfCache_Word *cWord = NULL;

    // 禁止异步!!
    static uint8_t head = 0, tail = 0;
    uint8_t take = 0;
    take = tail++;
    while (take != head)
        usleep(1000);

    if (!ts)
        return;

    while (*utf8)
    {
        //utf8 转 unicode
        retByte = ttf_utf8ToUnicode((uint8_t *)utf8, &unicode);
        //得到字符宽度
        if (retByte < 0) //特殊字符处理
        {
            if (*utf8 == '\t')
                charWidth = typeSize * 2 + xEdge;
            else if (*utf8 == '\n')
                charWidth = -1;
            else
                charWidth = 0;
            retByte = -retByte;
        }
        else
        {
            if ((cWord = ttf_findWord(&ts->cacheType, type, unicode, retByte)) && cWord->map.bitMap)
                charWidth = cWord->map.width;
            else
            {
                //宽/高/加粗
                size = ttf_getBasicByType(type, unicode, &width, &height, &baseLineHeight);
                //设置字体尺寸
                if (FT_Set_Char_Size(ts->face, width << 6, height << 6, 72, 72) != 0)
                {
                    TTF_ERR("FT_Set_Char_Size failed \r\n");
                    goto exit;
                }
                //从字符码检索字形索引
                //装载字形图像到字形槽（将会抹掉先前的字形图像）
                if (FT_Load_Glyph(ts->face, FT_Get_Char_Index(ts->face, unicode), FT_LOAD_DEFAULT) != 0)
                {
                    TTF_ERR("FT_Load_Glyph failed \r\n");
                    goto exit;
                }

                glyph = ts->face->glyph;
                cWord->map.width = glyph->advance.x >> 6;
                cWord->map.height = height;
                charWidth = cWord->map.width;

                // 加粗
                if (FT_Outline_Embolden(&glyph->outline, size << 6) != 0) // *0 不加粗 *n加粗量
                {
                    TTF_ERR("FT_Outline_Embolden failed \r\n");
                    goto exit;
                }

                //转换为一个抗锯齿位图
                //ft_render_mode_normal:灰度图, ft_render_mode_mono:黑白位图
                if (FT_Render_Glyph(glyph, ft_render_mode_mono) == 0)
                {
                    //备份该字型数据
                    cWord->map.bitMap = (uint8_t *)calloc((glyph->bitmap.rows + 1) * glyph->bitmap.pitch, 1);
                    memcpy(cWord->map.bitMap, glyph->bitmap.buffer, glyph->bitmap.rows * glyph->bitmap.pitch);
                    //参数拷贝
                    cWord->map.bitWidth = glyph->bitmap.width;
                    cWord->map.bitHeight = glyph->bitmap.rows;
                    cWord->map.bitLeft = glyph->bitmap_left;
                    cWord->map.bitTop = height - glyph->bitmap_top - baseLineHeight;
                    cWord->map.lineByte = glyph->bitmap.pitch;
                }
            }
        }
        //得到最终字型宽
        if (charWidth > 0)
        {
            if (retWidth + charWidth > widthOfLine)
            {
                retWidthMax = widthOfLine;

                retWidth = charWidth;
                retHeight += (typeSize + yEdge);
            }
            else
                retWidth += (charWidth + xEdge);
        }
        else if (charWidth == -1)
        {
            if (retWidth > retWidthMax)
                retWidthMax = retWidth;

            retWidth = 0;
            retHeight += (typeSize + yEdge);
        }
        //解析下一个字型
        utf8 += retByte;
    }

    //返回
    if (retH)
        *retH = retHeight;
    if (retW)
    {
        if (retWidth > retWidthMax)
            *retW = retWidth;
        else
            *retW = retWidthMax;
        if (*retW > xEdge)
            *retW -= xEdge;
    }

exit:
    head += 1;
}

//单行模式
int ttf_getSizeByUtf8(void *obj, char *utf8, int type, int xEdge, int *retH)
{
    Ttf_Struct *ts = (Ttf_Struct *)obj;
    //一次解析后的字型信息
    FT_GlyphSlot glyph;

    FT_ULong unicode = 0;
    int width = 0, height = 0, size = 0;
    int baseLineHeight = 0;
    int retByte = -1;

    int retWidth = 0, charWidth = 0;
    int typeSize = type / 10;

    TtfCache_Word *cWord = NULL;

    // 禁止异步!!
    static uint8_t head = 0, tail = 0;
    uint8_t take = 0;
    take = tail++;
    while (take != head)
        usleep(1000);

    if (!ts)
        return 0;

    while (*utf8)
    {
        //utf8 转 unicode
        retByte = ttf_utf8ToUnicode((uint8_t *)utf8, &unicode);
        //得到字符宽度
        if (retByte < 0) //特殊字符处理
        {
            if (*utf8 == '\t')
                charWidth = typeSize * 2 + xEdge;
            else
                charWidth = 0;
            retByte = -retByte;
        }
        else
        {
            if ((cWord = ttf_findWord(&ts->cacheType, type, unicode, retByte)) && cWord->map.bitMap)
                charWidth = cWord->map.width;
            else
            {
                //宽/高/加粗
                size = ttf_getBasicByType(type, unicode, &width, &height, &baseLineHeight);
                //设置字体尺寸
                if (FT_Set_Char_Size(ts->face, width << 6, height << 6, 72, 72) != 0)
                {
                    TTF_ERR("FT_Set_Char_Size failed \r\n");
                    goto exit;
                }
                //从字符码检索字形索引
                //装载字形图像到字形槽（将会抹掉先前的字形图像）
                if (FT_Load_Glyph(ts->face, FT_Get_Char_Index(ts->face, unicode), FT_LOAD_DEFAULT) != 0)
                {
                    TTF_ERR("FT_Load_Glyph failed \r\n");
                    goto exit;
                }

                glyph = ts->face->glyph;
                cWord->map.width = glyph->advance.x >> 6;
                cWord->map.height = height;
                charWidth = cWord->map.width;
                // 加粗
                if (FT_Outline_Embolden(&glyph->outline, size << 6) != 0) // *0 不加粗 *n加粗量
                {
                    TTF_ERR("FT_Outline_Embolden failed \r\n");
                    goto exit;
                }

                //转换为一个抗锯齿位图
                //ft_render_mode_normal:灰度图, ft_render_mode_mono:黑白位图
                if (FT_Render_Glyph(glyph, ft_render_mode_mono) == 0)
                {
                    //备份该字型数据
                    cWord->map.bitMap = (uint8_t *)calloc((glyph->bitmap.rows + 1) * glyph->bitmap.pitch, 1);
                    memcpy(cWord->map.bitMap, glyph->bitmap.buffer, glyph->bitmap.rows * glyph->bitmap.pitch);

                    cWord->map.bitWidth = glyph->bitmap.width;
                    cWord->map.bitHeight = glyph->bitmap.rows;
                    cWord->map.bitLeft = glyph->bitmap_left;
                    cWord->map.bitTop = height - glyph->bitmap_top - baseLineHeight;
                    cWord->map.lineByte = glyph->bitmap.pitch;
                }
            }
        }
        //累加行宽
        if (charWidth > 0)
            retWidth += (charWidth + xEdge);
        //解析下一个字型
        utf8 += retByte;
    }

exit:
    if (retH)
        *retH = typeSize;
    if (retWidth > xEdge)
        retWidth -= xEdge;

    head += 1;
    return retWidth;
}

#endif // #if(MAKE_FREETYPE)
