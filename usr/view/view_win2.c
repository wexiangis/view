

#include "viewInc.h"

int view_win2_vStart(View_Struct *view, void *object, View_Focus *focus, ViewButtonTouch_Event *event)
{
    return CALLBACK_OK;
}

View_Struct *view_win2_init(void)
{
    View_Struct *vs = (View_Struct *)calloc(1, sizeof(View_Struct));
    View_Struct *vsTemp;

    vs->width = WINDOW_WIDTH;
    vs->height = WINDOW_HEIGHT;
    vs->rType = VRT_BOTTOM;
    
    vs->viewStart = (ViewCallBack)&view_win2_vStart;

    vsTemp = (View_Struct *)calloc(1, sizeof(View_Struct));
    ;
    view_add(vs, vsTemp, false);

    return vs;
}

