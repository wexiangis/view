#include <stdio.h>
#include <sys/time.h>

#include "viewApi.h"
#include "viewInc.h"

// 获取系统tick可以作为us时长参考
long int getTickUs(void)
{
    struct timeval tv = {0};
    gettimeofday(&tv, NULL);
    return tv.tv_sec * 1000000u + tv.tv_usec;
}

void delayUs(long int us)
{
    struct timeval tv;
    tv.tv_sec = us / 1000000;
    tv.tv_usec = us % 1000000;
    select(0, NULL, NULL, NULL, &tv);
}

// 延时初始化
#define DELAY_INIT \
long int _tick1 = 0, _tick2;

// 检查时差,差多少延时多少
#define DELAY_US(us)                        \
_tick2 = getTickUs();                       \
if (_tick2 > _tick1 && _tick2 - _tick1 < us)\
    delayUs(us - (_tick2 - _tick1));        \
_tick1 = getTickUs();

int main(void)
{
    View_Struct *vsMain;
    View_Struct *vsBar;
    View_Struct *vsWin1;
    View_Struct *vsWin2;

    DELAY_INIT;

    //UI系统初始化(底层平台初始化)
    view_init();

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
    while(1)
    {
        DELAY_US(REFRESH_MS * 1000);

        //清屏
        PRINT_CLEAR(ViewColor.Blue.value.Int);

        //绘制view链表
        view_draw(NULL, NULL, NULL, NULL, vsMain, NULL);

        //刷新屏幕
        PRINT_EN();
    }

    return 0;
}
