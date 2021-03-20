
#ifndef _VIEWSRC_H_
#define _VIEWSRC_H_

#include "viewPlat.h"
#include "viewType.h"
#include "ttfType.h"

//全局开放的词条
typedef struct ViewSrcType
{
    //控件参数
    ViewValue_Format Common_Rad;         //圆角
    ViewValue_Format Common_LabelType;   //标签字体大小
    ViewValue_Format Common_ContentType; //正文字体大小
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
    //选型部分
    ViewValue_Format BackGround;  //背景
    ViewValue_Format Content;     //内容
    ViewValue_Format Label;       //注释
    ViewValue_Format Tips;        //右上角标
    ViewValue_Format Button;      //按键
    ViewValue_Format ButtonValue; //按键
    ViewValue_Format Focus;       //按键
    ViewValue_Format White;
    ViewValue_Format Black;
    ViewValue_Format Gray;
    ViewValue_Format Gray2;
    ViewValue_Format Gray3;
    //公共部分
    ViewValue_Format Red;
    ViewValue_Format Red2;
    ViewValue_Format Red3;
    ViewValue_Format Green;
    ViewValue_Format Green2;
    ViewValue_Format Green3;
    ViewValue_Format Blue;
    ViewValue_Format Blue2;
    ViewValue_Format Blue3;
    ViewValue_Format Yellow;
    ViewValue_Format Yellow2;
    ViewValue_Format Yellow3;
    ViewValue_Format Cyan; //天蓝色(绿+蓝)
    ViewValue_Format Cyan2;
    ViewValue_Format Cyan3;
    ViewValue_Format Magenta; //紫色(红+蓝)
    ViewValue_Format Magenta2;
    ViewValue_Format Magenta3;
    ViewValue_Format Orange;
    ViewValue_Format Orange2;
    ViewValue_Format Orange3;
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
void viewConfig_init(void);
//viewSrc初始化
void viewSrc_init(void);
//viewColor初始化
void viewColor_init(void);

//检查 value 指针是否存在于 viewSrc, 用于回收站决定是否释放内存
bool viewSrc_compare(ViewValue_Format *value);
//检查 value 指针是否存在于 viewColor, 用于回收站决定是否释放内存
bool viewColor_compare(ViewValue_Format *value);

#endif
