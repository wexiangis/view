
#include <stdio.h>
#include "ui/viewapi.h"

//资源文件夹
#define RES_PATH "./project/demo/res"

//ttf文件在资源文件夹内
#define TTF_FILE RES_PATH"/Droid_Sans_Fallback.ttf"
// #define TTF_FILE RES_PATH"/ZhaiZaiJiaMaiKeBi.ttf"

int myView_vStart(View_Struct *view, void *object, View_Focus *focus, ViewButtonTouch_Event *event)
{
    View_Struct *vsTemp;
    struct tm *time = view_time(); //年月不需要再加1900和1

    if (view->isFirstIn)
        printf("myView_vStart: first in \r\n");

    //取得链表第一个节点,更新日期
    vsTemp = view->view;
    vsTemp->text->value.IntArray[0] = time->tm_year;
    vsTemp->text->value.IntArray[1] = time->tm_mon;
    vsTemp->text->value.IntArray[2] = time->tm_mday;

    //取得链表第2个节点,更新时间
    vsTemp = vsTemp->next;
    vsTemp->text->value.IntArray[0] = time->tm_hour;
    vsTemp->text->value.IntArray[1] = time->tm_min;
    vsTemp->text->value.IntArray[2] = time->tm_sec;

    //取得链表第4个节点,更新进度条
    vsTemp = view_num(view->view, 4);
    //显示百分数更性
    vsTemp->text->value.IntArray[0] += 5;
    if (vsTemp->text->value.IntArray[0] > 100)
        vsTemp->text->value.IntArray[0] -= 100;
    //进度条百分比更新
    vsTemp->shape.processBar.percent =
        vsTemp->text->value.IntArray[0];

    //取得链表第5个节点,圆环转动
    vsTemp = view_num(view->view, 5);
    //圆环起始位置跑动
    vsTemp->shape.circle.divStart += 1;
    if (vsTemp->shape.circle.divStart > vsTemp->shape.circle.div)
        vsTemp->shape.circle.divStart -= vsTemp->shape.circle.div;
    //圆环结束位置跑动
    vsTemp->shape.circle.divEnd += 3;
    if (vsTemp->shape.circle.divEnd > vsTemp->shape.circle.div)
        vsTemp->shape.circle.divEnd -= vsTemp->shape.circle.div;

    return CALLBACK_OK;
}

View_Struct *myView_init(void)
{
    View_Struct *vs;
    View_Struct *vsTemp;

    //本控件初始化,占用全屏
    vs = view_init("main", VWHT_FULL, VWHT_FULL, 0, 0);
    //窗口背景填充图片
    vs->picPath = RES_PATH "/fruits.jpg";
    //注册绘制前回调函数
    vs->viewStart = (ViewCallBack)&myView_vStart;

    //子控件: 年月日, 占用1/2窗口宽, 1/5窗口高, 靠在父控件左上角(默认)
    vsTemp = view_init("date", VWHT_MATCH * 2, VWHT_MATCH * 5, 0, 0);
    //添加背景色,以方便观察布局情况
    vsTemp->backGroundColor = 0x80C0C0C0; //透明度0x80,0~0xFF,越大越透明
    //添加文字
    vsTemp->text = viewValue_init(NULL, VT_INT_ARRAY, 3, 2021, 3, 21);
    vsTemp->text->sep = '-'; //分隔符
    vsTemp->text->zero = 2;  //2位补0
    vsTemp->textSize = 240;  //24号字体,0加粗
    vsTemp->textColor = 0xFFFFFF;
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
    vsTemp->textColor = 0x40FFFF00;
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
    vsTemp->shape.rect.rad = 20; //圆角半径20像素
    vsTemp->shapeColorPrint = 0xB000FFFF;
    //指定图片,会自动缩放为当前控件大小
    vsTemp->picPath = RES_PATH "/signal.png";
    //文字
    vsTemp->text = viewValue_init(NULL, VT_STRING, 1, "你好,世界!");
    vsTemp->textSize = 320;
    vsTemp->textColor = 0xFFFFFF;
    //添加到父控件链表
    view_add(vs, vsTemp, false);

    //控件4: 进度条, 在父控件的底部,横向占满,高1/10屏幕高
    vsTemp = view_init(NULL, VWHT_MATCH, VWHT_MATCH * 10, VRT_BOTTOM, 0);
    //进度条参数
    vsTemp->shapeType = VST_PROGRESS_BAR;
    vsTemp->shape.processBar.rad = 6;        //圆角半径6
    vsTemp->shape.processBar.percent = 0;    //进度条百分数设置
    vsTemp->shape.processBar.lineSize = 1;   //画边框
    vsTemp->shape.processBar.edge = 1;       //边框和进度条间隔
    vsTemp->shapeColorPrint = 0x8000FF00;    //进度条颜色
    vsTemp->shapeColorBackground = 0xFFFFFF; //背景色(lineSize>0时为边框颜色)
    //进度条百分数显示
    vsTemp->text = viewValue_init(NULL, VT_INT_ARRAY, 1, 0);
    vsTemp->text->sep = '%'; //分隔符,这里作为单位使用
    vsTemp->textSize = 240;
    vsTemp->textColor = 0xFFFFFF;
    view_add(vs, vsTemp, false);

    //控件5: 圆环, 在上一个控件的正上方
    vsTemp = view_init(NULL, 32, 32, VRT_TOP, VRNT_LAST);
    //圆(rad2>0时为圆环)
    vsTemp->shapeType = VST_CIRCLE;
    vsTemp->shape.circle.div = 20;     //拆分成20块
    vsTemp->shape.circle.divStart = 1; //只画1~2块的范围
    vsTemp->shape.circle.divEnd = 2;
    vsTemp->shape.circle.rad = 16;  //圆外半径
    vsTemp->shape.circle.rad2 = 12; //圆内半径
    vsTemp->shapeColorPrint = 0x0080FF;
    vsTemp->shapeColorBackground = 0xB0FFFFFF;
    view_add(vs, vsTemp, false);

    return vs;
}

int main(void)
{
    View_Struct *myView;

    //UI系统初始化
    viewApi_init(TTF_FILE);
    //初始化自己的视图文件
    myView = myView_init();

    while (1)
    {
        //清屏(白底色)
        view_clear(0xFFFFFF);
        //画界面
        view_draw(NULL, myView);
        //使能输出
        view_enable();
        //一帧延时
        view_delayms(200);
    }

    //内存回收示例
    //删除函数(从原链表移除并添加到垃圾桶链表)
    //删除后用户不能再访问该指针
    view_delete(myView);
    //再清空垃圾桶(从链表内彻底释放内存)
    viewTrash_clean();

    return 0;
}
