
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
#if(MAKE_FREETYPE)
    //ttf字体解析句柄初始化
    ViewTTF = ttf_init(TTF_FILE);
#endif

    //控件参数
    viewValue_reset(&ViewSrc.Common_Rad, "Common_Rad", VT_INT, 1, VIEW_RESOLUTION / 24);
    viewValue_reset(&ViewSrc.Common_ContentType, "Common_ContentType", VT_INT, 1, VIEW_RESOLUTION_PRINT);
    viewValue_reset(&ViewSrc.Common_LabelType, "Common_LabelType", VT_INT, 1, VIEW_RESOLUTION_PRINT * 3 / 4);
    printf("ViewSrc print type : label/%d content/%d rad/%d\n",
           ViewSrc.Common_LabelType.value.Int,
           ViewSrc.Common_ContentType.value.Int,
           ViewSrc.Common_Rad.value.Int);

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
        viewValue_reset(&ViewColor.BackGround, "BackGround", VT_INT, 1, 0x000000);
        viewValue_reset(&ViewColor.Content, "Content", VT_INT, 1, 0xFFFFFF);
        viewValue_reset(&ViewColor.Label, "Label", VT_INT, 1, 0xFFFFFF);
        viewValue_reset(&ViewColor.Tips, "Tips", VT_INT, 1, 0xFF8000);
        viewValue_reset(&ViewColor.Button, "Button", VT_INT, 1, 0x4060FF);
        viewValue_reset(&ViewColor.ButtonValue, "ButtonValue", VT_INT, 1, 0xFFFFFF);
        viewValue_reset(&ViewColor.Focus, "Focus", VT_INT, 1, 0x00FF00);

        viewValue_reset(&ViewColor.White, "White", VT_INT, 1, 0xFFFFFF);
        viewValue_reset(&ViewColor.Black, "Black", VT_INT, 1, 0x202020);
        viewValue_reset(&ViewColor.Gray, "Gray", VT_INT, 1, 0xD0D0D0);
        viewValue_reset(&ViewColor.Gray2, "Gray2", VT_INT, 1, 0x808080);
        viewValue_reset(&ViewColor.Gray3, "Gray3", VT_INT, 1, 0x404040);

    //公共部分
    if (ViewColor.Red.value.Int == 0) //只初始化一次
    {
        viewValue_reset(&ViewColor.Red, "Red", VT_INT, 1, 0xFF0000);
        viewValue_reset(&ViewColor.Red2, "Red2", VT_INT, 1, 0xC00000);
        viewValue_reset(&ViewColor.Red3, "Red3", VT_INT, 1, 0x800000);
        viewValue_reset(&ViewColor.Green, "Green", VT_INT, 1, 0x00FF00);
        viewValue_reset(&ViewColor.Green2, "Green2", VT_INT, 1, 0x00C000);
        viewValue_reset(&ViewColor.Green3, "Green3", VT_INT, 1, 0x008000);
        viewValue_reset(&ViewColor.Blue, "Blue", VT_INT, 1, 0x0000FF);
        viewValue_reset(&ViewColor.Blue2, "Blue2", VT_INT, 1, 0x0000C0);
        viewValue_reset(&ViewColor.Blue3, "Blue3", VT_INT, 1, 0x000080);
        viewValue_reset(&ViewColor.Yellow, "Yellow", VT_INT, 1, 0xF0F000);
        viewValue_reset(&ViewColor.Yellow2, "Yellow2", VT_INT, 1, 0xE0E000);
        viewValue_reset(&ViewColor.Yellow3, "Yellow3", VT_INT, 1, 0xA0A000);
        viewValue_reset(&ViewColor.Cyan, "Cyan", VT_INT, 1, 0x00FFFF);
        viewValue_reset(&ViewColor.Cyan2, "Cyan2", VT_INT, 1, 0x00C0C0);
        viewValue_reset(&ViewColor.Cyan3, "Cyan3", VT_INT, 1, 0x008080);
        viewValue_reset(&ViewColor.Magenta, "Magenta", VT_INT, 1, 0xFF00FF);
        viewValue_reset(&ViewColor.Magenta2, "Magenta2", VT_INT, 1, 0xC000C0);
        viewValue_reset(&ViewColor.Magenta3, "Magenta3", VT_INT, 1, 0x800080);
        viewValue_reset(&ViewColor.Orange, "Orange", VT_INT, 1, 0xFF8000);
        viewValue_reset(&ViewColor.Orange2, "Orange2", VT_INT, 1, 0xC06000);
        viewValue_reset(&ViewColor.Orange3, "Orange3", VT_INT, 1, 0x804000);
    }
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
