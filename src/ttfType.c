
#include "ttfType.h"

#include <stdio.h>
#include <unistd.h>

#if(VIEW_CONFIG_PRINT_LIB == 1)

// #define  TTF_FILE    TTF_FILE_ROOT"/yy.ttf"
#define TTF_FILE    TTF_FILE_ROOT"/qbox.ttf"

#define BASELINE_TOP_POW_OF_HEIGHT  8       //基线高=height/8
#define ASCII_WIDTH_POW_OF_WORD(h)  (h*4/5) //ascii和文字在输出时的宽度比

//----- 自动缓存已有字型 -----

#define TTF_BUFFER_ENABLE

#ifdef  TTF_BUFFER_ENABLE

// #define TTF_STATICS //显示统计信息

typedef struct TtfWordStruct{
    //匹配项
    FT_ULong unicode;
    //内容
    Ttf_Param param;
    //
    struct TtfWordStruct *next;
}TtfWord_Struct;

typedef struct TtfTypeStruct{
    //匹配项
    int type;
    //内容
    TtfWord_Struct *word;
    TtfWord_Struct *ascii;
#ifdef TTF_STATICS
    int total;
#endif
    //
    struct TtfTypeStruct *next;
}TtfType_Struct;

static TtfType_Struct *ttfType = NULL;

static TtfWord_Struct* _ttf_fitWord(int type, FT_ULong unicode, int byteNum)
{
    static unsigned char head = 0, tail = 0;
    unsigned char take = 0;
    //
    TtfWord_Struct *tWord = NULL;
    TtfType_Struct *tType = NULL;
    // 禁止异步!!
    take = tail++;
    while(take != head)
        usleep(1000);
    //
    //匹配 type
    if(!ttfType)
        tType = ttfType = (TtfType_Struct *)calloc(1, sizeof(TtfType_Struct));
    else
        tType = ttfType;
    while(tType->next)
    {
        if(tType->type == type)
            break;
        tType = tType->next;
    }
    //新 type
    if(tType->type != type)
    {
#ifdef TTF_STATICS
        printf("new type [%d]\n", type);
#endif
        tType->next = (TtfType_Struct *)calloc(1, sizeof(TtfType_Struct));
        tType = tType->next;
        tType->type = type;
        //新 word
#ifdef TTF_STATICS
        printf("new word [%d %d] %ld\n", type, tType->total, unicode);
        tType->total += 1;
#endif
        if(byteNum == 1){
            tWord = tType->ascii = (TtfWord_Struct *)calloc(1, sizeof(TtfWord_Struct));
            tWord->unicode = unicode;
        }else{
            tWord = tType->word = (TtfWord_Struct *)calloc(1, sizeof(TtfWord_Struct));
            tWord->unicode = unicode;
        }
    }
    //成功匹配 type
    else
    {
        // printf("fit type %d\n", type);
        //匹配 wrod
        if(byteNum == 1){
            if(tType->ascii)
                tWord = tType->ascii;
            else
                tWord = tType->ascii = (TtfWord_Struct *)calloc(1, sizeof(TtfWord_Struct));
        }else{
            if(tType->word)
                tWord = tType->word;
            else
                tWord = tType->word = (TtfWord_Struct *)calloc(1, sizeof(TtfWord_Struct));
        }
        while(tWord->next)
        {
            if(tWord->unicode == unicode)
                break;
            tWord = tWord->next;
        }
        //新 wrod
        if(tWord->unicode != unicode)
        {
#ifdef TTF_STATICS
            printf("new word [%d %d] %ld\n", type, tType->total, unicode);
            tType->total += 1;
#endif
            tWord->next = (TtfWord_Struct *)calloc(1, sizeof(TtfWord_Struct));
            tWord = tWord->next;
            tWord->unicode = unicode;
        }
    }
    //
    head += 1;
    return tWord;
}

#endif

//----- 自动缓存已有字型 -----

static int _ttf_utf8ToUnicode(unsigned char *utf8_code, FT_ULong *unicode)
{
    // 0000 0000-0000 007F | 0xxxxxxx
    // 0000 0080-0000 07FF | 110xxxxx 10xxxxxx
    // 0000 0800-0000 FFFF | 1110xxxx 10xxxxxx 10xxxxxx
    // 0001 0000-0010 FFFF | 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx
    //转换成unicode
    if(utf8_code[0] < ' ')
    {
        *unicode = ' ';
        return -1;
    }
    else if(utf8_code[0] < 0x80)
    {
        *unicode = utf8_code[0];
        return 1;
    }
    else if(utf8_code[0] < 0xE0 && utf8_code[1] > 0x7F)
    {
        *unicode = ((utf8_code[0] & 0x1F) << 6) | (utf8_code[1] & 0x3F);
        return 2;
    }
    else if(utf8_code[0] < 0xF0 && utf8_code[1] > 0x7F && utf8_code[2] > 0x7F)
    {
        *unicode = ((utf8_code[0] & 0x0F) << 12) | ((utf8_code[1] & 0x3F) << 6) | (utf8_code[2] & 0x3F);
        return 3;
    }
    else if(utf8_code[0] < 0xF8 && utf8_code[1] > 0x7F && utf8_code[2] > 0x7F && utf8_code[3] > 0x7F)
    {
        *unicode = ((utf8_code[0] & 0x07) << 18) | ((utf8_code[1] & 0x3F) << 12) | ((utf8_code[2] & 0x3F) << 6) | (utf8_code[3] & 0x3F);
        return 4;
    }
    else
    {
        //printf("_ttf_utf8ToUnicode : utf8 code err\r\n");
        return -1;
    }
}

static int _ttf_type(int type, FT_ULong unicode, int *width, int *height, int *baseLineHeight)
{
    *height = type/10;
    if(*height == 0)
        return -1;
    if(unicode < 0x80)
        *width = ASCII_WIDTH_POW_OF_WORD(*height);
    else
        *width = *height;
    *baseLineHeight = *height/BASELINE_TOP_POW_OF_HEIGHT;
    return type%10;
}

int ttf_faceInit(FT_Face *face, FT_Library *library)
{
    //载入ttf字体库
    if(*library == NULL)
    {
        if(FT_Init_FreeType(library) != 0)
        {
            *library = NULL;
            //printf("FT_Init_FreeType err\n");
            return -1;
        }
    }
    //创建一个face  
    if(*face == NULL)
    {
        if(FT_New_Face(*library, TTF_FILE, 0, face) != 0)
        {
            *face = NULL;
            //printf("FT_New_Face err\n");
            return -1;
        }
    }
    return 0;
}

void ttf_faceRelease(FT_Face *face, FT_Library *library)
{
    if(*face)
    {
        FT_Done_Face(*face);
        *face = NULL;
    }
    if(*library)
    {
        FT_Done_FreeType(*library);
        *library = NULL;
    }
}

//功能: 根据文字的UTF-8编码, 返回该字的矩阵数据
//参数: utf8_code: utf-8编码
//     type: 字体代号, 例如160表示16*16大小, 0号字体
//返回: 识别的utf-8编码长度(ascii为1字符, 汉字为3字符, 用户根据该返回值来确定下次的偏移量), -1 表示失败 可以跳过该字符
int ttf_getBitMapByUtf8(unsigned char *utf8_code, int type, Ttf_Param *param, FT_Face *face, FT_Library *library)
{
    FT_ULong unicode = 0;
    int width = 0, height = 0, size = 0;
    int baseLineHeight = 0;
    int retByte = -1;
    //
    // FT_Library library = NULL;
    // FT_Face face = NULL;
    FT_GlyphSlot glyph;
    //utf8 转 unicode
    if((retByte = _ttf_utf8ToUnicode(utf8_code, &unicode)) < 0)
        return retByte;
    //
#ifdef  TTF_BUFFER_ENABLE
    TtfWord_Struct *tWord = NULL;
    tWord = _ttf_fitWord(type, unicode, retByte);
    if(tWord && tWord->param.bitMap)
    {
        *param = tWord->param;
        return retByte;
    }
#endif
    //宽/高/加粗
    size = _ttf_type(type, unicode, &width, &height, &baseLineHeight);
    //
    if(ttf_faceInit(face, library) < 0)
    {
        ttf_faceRelease(face, library);
        return -1;
    }
    //设置字体尺寸
    if(FT_Set_Char_Size(*face, width<<6, height<<6, 72, 72) != 0)
    {
        //printf("FT_Set_Char_Size failure !!\r\n");
        ttf_faceRelease(face, library);
        return -1;
    }
    //从字符码检索字形索引
    //装载字形图像到字形槽（将会抹掉先前的字形图像）
    if(FT_Load_Glyph(*face, FT_Get_Char_Index(*face, unicode), FT_LOAD_DEFAULT) != 0)
    {
        //printf("FT_Load_Glyph failure !!\r\n");
        ttf_faceRelease(face, library);
        return -1;
    }
    glyph = (*face)->glyph;

#ifdef  TTF_BUFFER_ENABLE
    tWord->param.horMov = glyph->advance.x >> 6;
#endif

    //加粗
    if(FT_Outline_Embolden(&(glyph->outline), size<<6) != 0) // *0 不加粗 *n加粗量
    {
        //printf("FT_Outline_Embolden failure !!\r\n");
        ttf_faceRelease(face, library);
        return -1;
    }
    //转换为一个抗锯齿位图
    // ft_render_mode_normal : 灰度图
    // ft_render_mode_mono : 黑白位图
    if(FT_Render_Glyph(glyph, ft_render_mode_mono) == 0)
    {

#ifdef  TTF_BUFFER_ENABLE
        //备份该字型数据
        tWord->param.bitMap = (unsigned char *)calloc((glyph->bitmap.rows+1)*glyph->bitmap.pitch, 1);
        memcpy(tWord->param.bitMap, glyph->bitmap.buffer, glyph->bitmap.rows*glyph->bitmap.pitch);
        //
        tWord->param.width = glyph->bitmap.width;
        tWord->param.height = glyph->bitmap.rows;
        tWord->param.xErr = glyph->bitmap_left;
        tWord->param.yErr = height - glyph->bitmap_top - baseLineHeight;
        tWord->param.lineByte = glyph->bitmap.pitch;
        // tWord->param.horMov = glyph->advance.x >> 6;
        //
        *param = tWord->param;
#else
        param->bitMap = glyph->bitmap.buffer;
        param->width = glyph->bitmap.width;
        param->height = glyph->bitmap.rows;
        param->xErr = glyph->bitmap_left;
        param->yErr = height - glyph->bitmap_top - baseLineHeight;
        param->lineByte = glyph->bitmap.pitch;
        param->horMov = glyph->advance.x >> 6;
#endif
    }
    else
        param->bitMap = NULL;
    //
    // ttf_faceRelease(&face, &library);
    return retByte;
}

// static FT_Library library = NULL;
// static FT_Face face = NULL;

//功能: 根据utf-8编码和字体代号, 返回当前字符串最终输出时矩阵横向像素个数
//     用于确认当前输出的矩阵会不会超出屏幕或别的限制范围
//返回: 横向字节数, -1表示失败
void ttf_getRangeByUtf8_multiLine(unsigned char *utf8_code, int type, int xEdge, int yEdge, int widthLimit, int *retW, int *retH)
{
    FT_ULong unicode = 0;
    int width = 0, height = 0, size = 0;
    int baseLineHeight = 0;
    int retByte = -1;
    //
    int retWidth = 0, charWidth = 0, retWidthMax = 0;
    int retHeight = type/10;
    int typeSize = type/10;
    //
    static FT_Library library = NULL;
    static FT_Face face = NULL;
    //
#ifdef  TTF_BUFFER_ENABLE
    TtfWord_Struct *tWord = NULL;
#endif
    static unsigned char head = 0, tail = 0;
    unsigned char take = 0;
    // 禁止异步!!
    take = tail++;
    while(take != head)
        usleep(1000);
    //
    while(*utf8_code)
    {
        //utf8 转 unicode
        retByte = _ttf_utf8ToUnicode(utf8_code, &unicode);
        //得到字符宽度
        if(retByte < 0)//特殊字符处理
        {
            if(*utf8_code == '\t')
                charWidth = typeSize*2 + xEdge;
            else if(*utf8_code == '\n')
                charWidth = -1;
            else
                charWidth = 0;
            retByte = -retByte;
        }
        else
        {
            
#ifdef  TTF_BUFFER_ENABLE
            if((tWord = _ttf_fitWord(type, unicode, retByte)) && tWord->param.bitMap)
                charWidth = tWord->param.horMov;
            else
#endif
            {
                //
                if(ttf_faceInit(&face, &library) < 0)
                {
                    ttf_faceRelease(&face, &library);
                    head += 1;
                    return;
                }
                //宽/高/加粗
                size = _ttf_type(type, unicode, &width, &height, &baseLineHeight);
                //设置字体尺寸
                if(FT_Set_Char_Size(face, width<<6, height<<6, 72, 72) != 0)
                {
                    //printf("FT_Set_Char_Size failure !!\r\n");
                    ttf_faceRelease(&face, &library);
                    head += 1;
                    return ;
                }
                //从字符码检索字形索引
                //装载字形图像到字形槽（将会抹掉先前的字形图像）
                if(FT_Load_Glyph(face, FT_Get_Char_Index(face, unicode), FT_LOAD_DEFAULT) != 0)
                {
                    //printf("FT_Load_Glyph failure !!\r\n");
                    ttf_faceRelease(&face, &library);
                    head += 1;
                    return ;
                }
                
                charWidth = face->glyph->advance.x >> 6;

#ifdef  TTF_BUFFER_ENABLE

                tWord->param.horMov = charWidth;

                // 加粗
                if(FT_Outline_Embolden(&(face->glyph->outline), size<<6) != 0) // *0 不加粗 *n加粗量
                {
                    //printf("FT_Outline_Embolden failure !!\r\n");
                    ttf_faceRelease(&face, &library);
                    head += 1;
                    return ;
                }

                //转换为一个抗锯齿位图
                // ft_render_mode_normal : 灰度图
                // ft_render_mode_mono : 黑白位图
                if(FT_Render_Glyph(face->glyph, ft_render_mode_mono) == 0)
                {
                    //备份该字型数据
                    tWord->param.bitMap = (unsigned char *)calloc((face->glyph->bitmap.rows+1)*face->glyph->bitmap.pitch, 1);
                    memcpy(tWord->param.bitMap, face->glyph->bitmap.buffer, face->glyph->bitmap.rows*face->glyph->bitmap.pitch);
                    //
                    tWord->param.width = face->glyph->bitmap.width;
                    tWord->param.height = face->glyph->bitmap.rows;
                    tWord->param.xErr = face->glyph->bitmap_left;
                    tWord->param.yErr = height - face->glyph->bitmap_top - baseLineHeight;
                    tWord->param.lineByte = face->glyph->bitmap.pitch;
                    // tWord->param.horMov = face->glyph->advance.x >> 6;
                }
#endif
            }
        }
        //
        if(charWidth > 0)
        {
            if(retWidth + charWidth > widthLimit)
            {
                retWidthMax = widthLimit;
                //
                retWidth = charWidth;
                retHeight += (typeSize + yEdge);
            }
            else
                retWidth += (charWidth + xEdge);
        }
        else if(charWidth == -1)
        {
            if(retWidth > retWidthMax)
                retWidthMax = retWidth;
            //
            retWidth = 0;
            retHeight += (typeSize + yEdge);
        }
        //
        utf8_code += retByte;
    }
    //
    if(retH)
        *retH = retHeight;
    if(retW)
    {
        if(retWidth > retWidthMax)
            *retW = retWidth;
        else
            *retW = retWidthMax;
        if(*retW > xEdge)
            *retW -= xEdge;
    }
    //
    head += 1;
    // ttf_faceRelease(&face, &library);
}

//功能: 根据utf-8编码和字体代号, 返回当前字符串最终输出时矩阵横向像素个数
//     用于确认当前输出的矩阵会不会超出屏幕或别的限制范围
//返回: 横向宽度, -1表示失败
int ttf_getRangeByUtf8(unsigned char *utf8_code, int type, int xEdge, int *retH)
{
    FT_ULong unicode = 0;
    int width = 0, height = 0, size = 0;
    int baseLineHeight = 0;
    int retByte = -1;
    //
    int retWidth = 0, charWidth = 0;
    int typeSize = type/10;
    //
    static FT_Library library = NULL;
    static FT_Face face = NULL;
    //
#ifdef  TTF_BUFFER_ENABLE
    TtfWord_Struct *tWord = NULL;
#endif
    //
    while(*utf8_code)
    {
        //utf8 转 unicode
        retByte = _ttf_utf8ToUnicode(utf8_code, &unicode);
        //得到字符宽度
        if(retByte < 0)//特殊字符处理
        {
            if(*utf8_code == '\t')
                charWidth = typeSize*2 + xEdge;
            else
                charWidth = 0;
            retByte = -retByte;
        }
        else
        {
            
#ifdef  TTF_BUFFER_ENABLE
            if((tWord = _ttf_fitWord(type, unicode, retByte)) && tWord->param.bitMap)
                charWidth = tWord->param.horMov;
            else
#endif
            {
                //
                if(ttf_faceInit(&face, &library) < 0)
                {
                    ttf_faceRelease(&face, &library);
                    return -1;
                }
                //宽/高/加粗
                size = _ttf_type(type, unicode, &width, &height, &baseLineHeight);
                //设置字体尺寸
                if(FT_Set_Char_Size(face, width<<6, height<<6, 72, 72) != 0)
                {
                    //printf("FT_Set_Char_Size failure !!\r\n");
                    ttf_faceRelease(&face, &library);
                    return -1;
                }
                //从字符码检索字形索引
                //装载字形图像到字形槽（将会抹掉先前的字形图像）
                if(FT_Load_Glyph(face, FT_Get_Char_Index(face, unicode), FT_LOAD_DEFAULT) != 0)
                {
                    //printf("FT_Load_Glyph failure !!\r\n");
                    ttf_faceRelease(&face, &library);
                    return -1;
                }
                
                charWidth = face->glyph->advance.x >> 6;

#ifdef  TTF_BUFFER_ENABLE

                tWord->param.horMov = charWidth;

                // 加粗
                if(FT_Outline_Embolden(&(face->glyph->outline), size<<6) != 0) // *0 不加粗 *n加粗量
                {
                    //printf("FT_Outline_Embolden failure !!\r\n");
                    ttf_faceRelease(&face, &library);
                    return -1;
                }

                //转换为一个抗锯齿位图
                // ft_render_mode_normal : 灰度图
                // ft_render_mode_mono : 黑白位图
                if(FT_Render_Glyph(face->glyph, ft_render_mode_mono) == 0)
                {
                    //备份该字型数据
                    tWord->param.bitMap = (unsigned char *)calloc((face->glyph->bitmap.rows+1)*face->glyph->bitmap.pitch, 1);
                    memcpy(tWord->param.bitMap, face->glyph->bitmap.buffer, face->glyph->bitmap.rows*face->glyph->bitmap.pitch);
                    //
                    tWord->param.width = face->glyph->bitmap.width;
                    tWord->param.height = face->glyph->bitmap.rows;
                    tWord->param.xErr = face->glyph->bitmap_left;
                    tWord->param.yErr = height - face->glyph->bitmap_top - baseLineHeight;
                    tWord->param.lineByte = face->glyph->bitmap.pitch;
                    // tWord->param.horMov = face->glyph->advance.x >> 6;
                }
#endif
            }
        }
        //
        if(charWidth > 0)
            retWidth += (charWidth + xEdge);
        //
        utf8_code += retByte;
    }
    //
    if(retH)
        *retH = typeSize;
    //
    // ttf_faceRelease(&face, &library);
    //
    if(retWidth > xEdge)
        retWidth -= xEdge;
    return retWidth;
}

#endif //VIEW_CONFIG_PRINT_LIB
