
#include "viewInc.h"

int view_bar_vStart(View_Struct *view, void *object, View_Focus *focus, ViewButtonTouch_Event *event)
{
    static int tickSec = 0;

    //1秒周期
    if (view->tickMs / 1000 != tickSec)
    {
        tickSec = view->tickMs / 1000 % 60;
        //更新时间
        view->view->text->value.IntArray[2] = tickSec;
        view_set_alpha(view->view->textColor, tickSec % 25 * 10);
    }

    return CALLBACK_OK;
}

View_Struct *view_bar_init(void)
{
    View_Struct *vs;
    View_Struct *vsTemp;

    vs = view_init("bar", BAR_WIDTH, BAR_HEIGHT, 0, 0);
    vs->viewStart = (ViewCallBack)&view_bar_vStart;

    //日期 : 在父控件左边(内部)
    vsTemp = view_init("bar-date", BAR_DATE_WIDTH, BAR_HEIGHT, VRT_LEFT, 0);
    vsTemp->text = viewValue_init("date", VT_INT_ARRAY, 3, 2021, 3, 19);
    vsTemp->text->param[0] = '/'; //分隔符
    vsTemp->text->param[1] = 2;   //保留2个0
    vsTemp->textSize = 240;
    vsTemp->textColor = 0x80FFFFFF;
    view_add(vs, vsTemp, false);

    //电量图标 : 在父控件右边边(内部)
    vsTemp = view_init("bar-batt-icon", BAR_ICON_WIDTH, BAR_HEIGHT, VRT_RIGHT, 0);
    // vsTemp->picPath = "./usr/src/edit.bmp";
    vsTemp->picPath = "./usr/src/signal.png";
    view_add(vs, vsTemp, false);

    //电量百分数 : 在上一控件(电量图标)的左边
    vsTemp = view_init(
        "bar-batt-percent", BAR_BATT_WIDTH, BAR_HEIGHT, VRT_LEFT, VRNT_LAST);
    vsTemp->text = viewValue_init("percent", VT_INT_ARRAY, 1, 95);
    vsTemp->text->param[0] = '%'; //分隔符
    vsTemp->textSize = 240;
    vsTemp->textColor = 0xFF0000;
    view_add(vs, vsTemp, false);

    return vs;
}
