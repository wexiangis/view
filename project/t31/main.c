

#include <stdio.h>

#include "ui/viewApi.h"
#include "viewInc.h"

int main(void)
{
    View_Struct *vsMain;
    View_Struct *vsBar;
    View_Struct *vsWin1;
    View_Struct *vsWin2;

    VIEW_DELAY_INIT;

    //UI系统初始化(底层平台初始化)
    viewApi_init();

    //view初始化
    vsMain = view_main_init();
    vsBar = view_bar_init();
    vsWin1 = view_win1_init();
    vsWin2 = view_win2_init();

    //状态栏作为常驻控件依附在主窗体上
    view_add(vsMain, vsBar, false);

    //vsWin1 和 vsWin2 通过跳转指针来引入主窗体
    vsMain->view->jumpView = vsWin1;

    //循环画view
    while (1)
    {
        VIEW_DELAY_US(REFRESH_MS * 1000);
        //清屏
        print_clean(0x2020FF);
        //绘制view链表
        view_draw(NULL, vsMain);
        //刷新屏幕
        print_en();
    }

    return 0;
}
