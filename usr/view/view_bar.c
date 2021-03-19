
#include "viewInc.h"

int view_bar_vStart(View_Struct *view, void *object, View_Focus *focus, ViewButtonTouch_Event *event)
{
    static int tickSec = 0;

    //1秒周期
    if (view->tickMs / 1000 != tickSec)
    {
        tickSec = view->tickMs / 1000;
        //更新时间
        view->view->value->value.IntArray[2] = tickSec % 60;
    }

    return CALLBACK_OK;
}

View_Struct *view_bar_init(void)
{
    View_Struct *vs = (View_Struct *)calloc(1, sizeof(View_Struct));
    View_Struct *vsTemp;

    vs->width = BAR_WIDTH;
    vs->height = BAR_HEIGHT;
    vs->viewStart = (ViewCallBack)&view_bar_vStart;

    //日期
    vsTemp = (View_Struct *)calloc(1, sizeof(View_Struct));
    vsTemp->width = BAR_DATE_WIDTH;
    vsTemp->height = BAR_HEIGHT;
    vsTemp->rType = VRT_LEFT; //靠在父控件(内部)左边
    vsTemp->value = viewValue_init("date", VT_INT_ARRAY, 3, 2021, 3, 19);
    vsTemp->value->param[0] = '/'; //分隔符
    vsTemp->value->param[1] = 2;   //保留2个0
    vsTemp->valueType = 240;
    vsTemp->valueColor = &ViewColor.White;
    view_add(vs, vsTemp, false);

    //电量图标
    vsTemp = (View_Struct *)calloc(1, sizeof(View_Struct));
    vsTemp->width = BAR_ICON_WIDTH;
    vsTemp->height = BAR_HEIGHT;
    vsTemp->rType = VRT_RIGHT; //靠在父控件(内部)右边
    vsTemp->picPath = "./usr/src/edit.bmp";
    vsTemp->picUseInvisibleColor = true;
    view_add(vs, vsTemp, false);

    //电量百分数
    vsTemp = (View_Struct *)calloc(1, sizeof(View_Struct));
    vsTemp->width = BAR_BATT_WIDTH;
    vsTemp->height = BAR_HEIGHT;
    vsTemp->rNumber = VRNT_LAST; //相对于上一个控件的位置
    vsTemp->rType = VRT_LEFT;    //靠在上一个控件(外部)左边
    vsTemp->value = viewValue_init("percent", VT_INT_ARRAY, 1, 95);
    vsTemp->value->param[0] = '%'; //分隔符
    vsTemp->valueType = 240;
    vsTemp->valueColor = &ViewColor.White;
    view_add(vs, vsTemp, false);

    return vs;
}
