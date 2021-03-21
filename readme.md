## demo.c

```c
#include <stdio.h>
#include "viewApi.h"

int myView_vStart(View_Struct *view, void *object, View_Focus *focus, ViewButtonTouch_Event *event)
{
    View_Struct *vsTemp;
    struct tm *time = view_time();

    if (view->isFirstIn)
        printf("myView_vStart: first in \r\n");

    //取得链表第一个节点,更新日期
    vsTemp = view->view;
    vsTemp->text->value.IntArray[0] = time->tm_year + 1900;
    vsTemp->text->value.IntArray[1] = time->tm_mon;
    vsTemp->text->value.IntArray[2] = time->tm_mday;

    //取得链表第2个节点,更新时间
    vsTemp = vsTemp->next;
    vsTemp->text->value.IntArray[0] = time->tm_hour;
    vsTemp->text->value.IntArray[1] = time->tm_min;
    vsTemp->text->value.IntArray[2] = time->tm_sec;

    return CALLBACK_OK;
}

View_Struct *myView_init(void)
{
    View_Struct *vs;
    View_Struct *vsTemp;

    //本控件初始化,占用全屏
    vs = view_init("main", VWHT_FULL, VWHT_FULL, 0, 0);
    //窗口背景填充图片
    vs->picPath = "./usr/src/fruits.jpg";
    //注册绘制前回调函数
    vs->viewStart = (ViewCallBack)&myView_vStart;

    //子控件: 年月日, 占用1/2窗口宽, 1/5窗口高, 靠在父控件左上角(默认)
    vsTemp = view_init("date", VWHT_MATCH * 2, VWHT_MATCH * 5, 0, 0);
    //添加背景色,以方便观察布局情况
    vsTemp->backGroundColor = 0x80C0C0C0;
    //添加文字
    vsTemp->text = viewValue_init(NULL, VT_INT_ARRAY, 3, 2021, 3, 21);
    vsTemp->text->sep = '-'; //分隔符
    vsTemp->text->zero = 2;  //2位补0
    vsTemp->textSize = 240;
    vsTemp->textColor = 0xFF0000;
    //添加到父控件链表
    view_add(vs, vsTemp, false); //false表示不要插入到头部

    //子控件2: 时分秒, 占用1/2窗口宽, 1/5窗口高, 靠在上一控件右边
    vsTemp = view_init("time", VWHT_MATCH * 2, VWHT_MATCH * 5, VRT_RIGHT, VRNT_LAST);
    //添加背景色,以方便观察布局情况
    vsTemp->backGroundColor = 0x80A0A0A0;
    //添加文字
    vsTemp->text = viewValue_init(NULL, VT_INT_ARRAY, 3, 12, 30, 25);
    vsTemp->text->sep = ':'; //分隔符
    vsTemp->text->zero = 2;  //2位补0
    vsTemp->textSize = 240;
    vsTemp->textColor = 0xFFFF00;
    vsTemp->textEdgeX = 2; //横向字间距2像素
    //添加到父控件链表
    view_add(vs, vsTemp, false);

    //子控件3: 在窗口正中间放置一张1/2窗口大小的带透明度图片
    vsTemp = view_init("pic", VWHT_MATCH * 2, VWHT_MATCH * 2, 0, 0);
    //相对父控件横纵向居中
    vsTemp->centerX = true;
    vsTemp->centerY = true;
    //画一个带圆角的矩形背景
    vsTemp->shapeType = VST_RECT;
    vsTemp->shape.rect.rad = 20;
    vsTemp->shapeColorPrint = 0xB000FFFF;
    //指定图片,会自动缩放为当前控件大小
    vsTemp->picPath = "./usr/src/signal.png";
    //添加到父控件链表
    view_add(vs, vsTemp, false);

    return vs;
}

int main(void)
{
    View_Struct *myView;

    //UI系统初始化
    viewApi_init();

    //初始化自己的视图文件
    myView = myView_init();

    while (1)
    {
        //清屏(白底色)
        print_clean(0xFFFFFF);
        //画界面
        view_draw(NULL, myView);
        //使能输出
        print_en();
        //一帧延时
        view_delayms(300);
    }

    //内存回收,先删除(从原链表移除并添加到垃圾桶链表)
    //用户不能再访问该指针
    view_delete(myView);
    //再清空垃圾桶
    viewTrash_clean();

    return 0;
}
```

---

![JPG](/example/demo.jpg)
