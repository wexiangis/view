
#include "viewSrc.h"

ViewConfig_Type ViewConfig;
ViewSrc_Type ViewSrc;
ViewColor_Type ViewColor;
void *ViewTTF;

void viewConfig_init(void)
{
    viewValue_reset(&ViewConfig.device, "device", VT_INT, 1, 0);
}

void viewSrc_init(void)
{
#if (MAKE_FREETYPE)
    //ttf字体解析句柄初始化
    ViewTTF = ttf_init(TTF_FILE);
#endif

    //控件参数
    viewValue_reset(&ViewSrc.Shape_Rad, "Shape_Rad", VT_INT, 1, VIEW_RESOLUTION / 24);
    viewValue_reset(&ViewSrc.Content_Type, "Content_Type", VT_INT, 1, VIEW_RESOLUTION_PRINT);
    viewValue_reset(&ViewSrc.Label_Size, "Label_Size", VT_INT, 1, VIEW_RESOLUTION_PRINT * 3 / 4);
    printf("ViewSrc print type : label/%d content/%d rad/%d\n",
           ViewSrc.Label_Size.value.Int,
           ViewSrc.Content_Type.value.Int,
           ViewSrc.Shape_Rad.value.Int);

    //API
    viewValue_reset(&ViewSrc.Api_Button_Enter, "Api_Button_Enter", VT_STRING, 1, "确认");
    viewValue_reset(&ViewSrc.Api_Button_Return, "Api_Button_Return", VT_STRING, 1, "返回");
    viewValue_reset(&ViewSrc.Api_Button_Cancel, "Api_Button_Cancel", VT_STRING, 1, "取消");
    viewValue_reset(&ViewSrc.Api_Del_Char, "Api_Del_Char", VT_STRING, 1, "空");

    //API 候选项
    viewValue_reset(&ViewSrc.Api_Input_Ascii, "Api_Input_Ascii", VT_STRING, 1,
                    "  !\"#$%&\'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~");
    viewValue_reset(&ViewSrc.Api_Input_Number, "Api_Input_Number", VT_STRING, 1, " 0123456789");
    viewValue_reset(&ViewSrc.Api_Input_Double, "Api_Input_Double", VT_STRING, 1, " .0123456789");
    viewValue_reset(&ViewSrc.Api_Input_S_Number, "Api_Input_S_Number", VT_STRING, 1, " -0123456789");
    viewValue_reset(&ViewSrc.Api_Input_S_Double, "Api_Input_S_Double", VT_STRING, 1, " -.0123456789");
    viewValue_reset(&ViewSrc.Api_Input_Ip, "Api_Input_Ip", VT_STRING, 1, " .0123456789");
    viewValue_reset(&ViewSrc.Api_Input_Url, "Api_Input_Url", VT_STRING, 1, " .-0123456789abcdefghijklmnopqrstuvwxyz");
    viewValue_reset(&ViewSrc.Api_Input_Normal_String, "Api_Input_Normal_String", VT_STRING, 1,
                    " _-0123456789aAbBcCdDeEfFgGhHiIjJkKlLmMnNoOpPqQrRsStTuUvVwWxXyYzZ");
}

void viewColor_init(void)
{
    ViewColor.BackGround = 0x000000;
    ViewColor.Content = 0xFFFFFF;
    ViewColor.Label = 0xFFFFFF;
    ViewColor.Tips = 0xFF8000;
    ViewColor.Button = 0x4060FF;
    ViewColor.ButtonValue = 0xFFFFFF;
    ViewColor.Focus = 0x00FF00;

    ViewColor.White = 0xFFFFFF;
    ViewColor.Black = 0x202020;
    ViewColor.Gray = 0x808080;
    ViewColor.Red = 0xFF0000;
    ViewColor.Green = 0x00FF00;
    ViewColor.Blue = 0x0000FF;
}

bool viewSrc_compare(ViewValue_Format *value)
{
    int i, len;
    ViewValue_Format *vvf;

    if (value == NULL)
        return false;

    vvf = (ViewValue_Format *)&ViewSrc;
    len = sizeof(ViewSrc_Type) / sizeof(ViewValue_Format *);
    for (i = 0; i < len; i++)
    {
        if (vvf == value)
            return true;
        vvf += 1;
    }

    return false;
}

bool viewColor_compare(ViewValue_Format *value)
{
    int i, len;
    ViewValue_Format *vvf;

    if (value == NULL)
        return false;

    vvf = (ViewValue_Format *)&ViewColor;
    len = sizeof(ViewColor_Type) / sizeof(ViewValue_Format *);
    for (i = 0; i < len; i++)
    {
        if (vvf == value)
            return true;
        vvf += 1;
    }

    return false;
}
