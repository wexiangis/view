
/*
 *  顶层view,用于构建链表
 */
#include "viewInc.h"

View_Struct *view_main_init(void)
{
    View_Struct *vs = (View_Struct *)calloc(1, sizeof(View_Struct));
    View_Struct *vsTemp;

    //占用范围
    vs->width = VWHT_MATCH;
    vs->height = VWHT_MATCH;

    /*
     *  空壳view,占用状态栏以下的空间,通过jumpView指针动态
     *  指向 vsWin1 或 vsWin2 来达到快速切换界面的效果
     */
    vsTemp = (View_Struct *)calloc(1, sizeof(View_Struct));
    vsTemp->width = WINDOW_WIDTH;
    vsTemp->height = WINDOW_HEIGHT;
    vsTemp->rType = VRT_BOTTOM;
    vsTemp->jumpViewKeepOn = true;
    view_add(vs, vsTemp, false);

    return vs;
}

