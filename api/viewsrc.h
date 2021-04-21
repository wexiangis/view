
#ifndef _VIEWSRC_H_
#define _VIEWSRC_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include "viewtype.h"
#include "ttftype.h"

//全局开放的词条
typedef struct ViewSrcType
{
    //控件参数
    ViewValue_Format Shape_Rad;         //圆角
    ViewValue_Format Label_Size;   //标签字体大小
    ViewValue_Format Content_Type; //正文字体大小
    //API
    ViewValue_Format Api_Button_Enter;
    ViewValue_Format Api_Button_Return;
    ViewValue_Format Api_Button_Cancel;
    ViewValue_Format Api_Del_Char;
    //API 候选项
    ViewValue_Format Api_Input_Ascii;
    ViewValue_Format Api_Input_Number;
    ViewValue_Format Api_Input_Double;
    ViewValue_Format Api_Input_S_Number; //有符整数
    ViewValue_Format Api_Input_S_Double; //有符浮点数
    ViewValue_Format Api_Input_Ip;
    ViewValue_Format Api_Input_Url;
    ViewValue_Format Api_Input_Normal_String;
} ViewSrc_Type;

//全局开放的颜色参数
typedef struct ViewColorType
{
    uint32_t BackGround;  //背景
    uint32_t Content;     //内容
    uint32_t Label;       //注释
    uint32_t Tips;        //右上角标
    uint32_t Button;      //按键
    uint32_t ButtonValue; //按键
    uint32_t Focus;       //按键
    uint32_t White;
    uint32_t Black;
    uint32_t Gray;
    uint32_t Red;
    uint32_t Green;
    uint32_t Blue;
} ViewColor_Type;

//全局开放的ui配置,将保存到文件
typedef struct ViewConfigType
{
    ViewValue_Format device;
} ViewConfig_Type;

extern ViewConfig_Type ViewConfig; //公共的配置文件
extern ViewSrc_Type ViewSrc;       //公共内容资源
extern ViewColor_Type ViewColor;   //公共颜色资源
extern void *ViewTTF;              //公共ttf解析控制符(用来调用ttfType.h中的接口用)

//配置文件初始化
void viewConfig_init(int width, int height);
//viewSrc初始化
void viewSrc_init(int width, int height, char *ttfFile);
//viewColor初始化
void viewColor_init(void);

//检查 value 指针是否存在于 viewSrc, 用于回收站决定是否释放内存
bool viewSrc_compare(ViewValue_Format *value);
//检查 value 指针是否存在于 viewColor, 用于回收站决定是否释放内存
bool viewColor_compare(ViewValue_Format *value);

#ifdef __cplusplus
}
#endif

#endif
