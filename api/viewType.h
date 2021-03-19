
#ifndef __VIEWTYPE_H_
#define __VIEWTYPE_H_

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>  //calloc()
#include <string.h>  //strlen()
#include <pthread.h> //pthread_xxx
#include <stdarg.h>  //变长参数

//========== 数据类型代号 ==========
typedef enum
{
    VT_NULL = 0,
    VT_CHAR = 1,
    VT_STRING,
    VT_STRING_ARRAY,
    VT_INT,
    VT_INT_ARRAY,
    VT_DOUBLE,
    VT_DOUBLE_ARRAY,
    VT_BOOL,
    VT_BOOL_ARRAY,
    VT_POINT,       //不参与文件读/写
    VT_POINT_ARRAY, //不参与文件读/写
    VT_END
} ViewValue_Type;

//========== 类型+名称+数据存储 ==========
#define VIEW_VALUE_NAME_LEN 64 //变量名称限长32字节
typedef struct ViewValueFormat
{
    //[互斥锁]
    pthread_mutex_t lock;
    //[类型]
    ViewValue_Type type;
    //[名称]
    char name[VIEW_VALUE_NAME_LEN];
    //[这是一块固定大小的内存,用来存储任意类型数据]
    union {
        char Char;
        char *String;
        char **StringArray;
        int Int;
        int *IntArray;
        double Double;
        double *DoubleArray;
        bool Bool;
        bool *BoolArray;
        void *Point;
        void **PointArray;
    } value;
    //[描述value内存大小的变量]
    //value 为普通变量时:
    //      vSize 值为变量大小字节数
    //      例如: sizeof(char), sizeof(int), sizeof(double)
    //value 为指针时:
    //      vSize 反映分配内存字节数,可用于计算有效数据长度
    //      例如: IntArray, 数组长度为 vSize/sizeof(int)
    //value 为 指针数组 StringArray, PointArray 时:
    //      vSize = 数组长度 * sizeof(void *)
    //      StringArray 里的每个字符串长度是任意的
    int vSize;
    //单向链表
    struct ViewValueFormat *next;
    //[多用途参数]
    //param[0]: value 为数组时(**StringArray,*DoubleArray,*BoolArray), 指定打印的分隔符
    //      例如: param[0]='/', Api输出时: "Tom/25/Man"
    //      0/默认: 使用逗号','
    //param[1]: value 为浮点数时(Double,*DoubleArray), 指定保留小数位数
    //      例如: param[0]=' ',param[1]=2, Api输出时: "12.34 98.01 1.20"
    //      0/默认:  浮点数保留6位小数
    char param[2];
} ViewValue_Format;

//========== 形状封装 ==========

//形状列表
typedef enum ViewShapeType
{
    VST_NULL = -1,
    VST_RECT = 0,     //矩形
    VST_PARA,         //平行四边形
    VST_LINE,         //线条
    VST_CIRCLE,       //圆
    VST_SECTOR,       //扇形
    VST_PROGRESS_BAR, //进度条
    VST_SCROLL_BAR,   //滚动条
    VST_SWITCH,       //开关
} ViewShape_Type;

//矩形
typedef struct ViewShapeRectType
{
    int rad;      //圆角半径
    int lineSize; //线宽
    int angle;    //旋转角度(-360, +360)
} ViewShapeRect_Type;

//平行四边形
typedef struct ViewShapeParaType
{
    int err;      //矩形基础上,>0时左上和右下缩进量,<0时右上和左下缩进量
    int lineSize; //线宽
    int mode;     //缩进模式 0:向内缩进(控件宽减少) 1:向外扩充(控件宽不变) 2:向外扩充(控件宽变大) 注意1,2会超出控件范围需要设置overDraw
} ViewShapePara_Type;

//线条
typedef struct ViewShapeLineType
{
    int mode;     //线条模式 0/左上-右下 1/左下-右上 2/左上-右上 3/右上-右下 4/左下-右下 5/左上-左下
    int lineSize; //线宽
    int space;    //不为0时画 虚线
    // int sErr[2], eErr[2];//线段两端点在控件内的进一步偏移量 注意参照mode说明来定位起始和结束端点
} ViewShapeLine_Type;

//圆
typedef struct ViewShapeCircleType
{
    int rad;      //半径
    int rad2;     //内径
    int div;      //分块数
    int divStart; //分块起始
    int divEnd;   //分块结束
} ViewShapeCircle_Type;

//扇形
typedef struct ViewShapeSector
{
    int rad;    //外径
    int rad2;   //内径
    int rotary; //旋转角度0~359 //控件中心为圆心
    int angle;  //开角
} ViewShapeSector_Type;

//进度条
typedef struct ViewShapeProcessBarType
{
    int rad;      //圆角半径
    int lineSize; //线宽
    int percent;  //百分数值
    int mode;     //0:左到右 1:右到左 2:下到上 3:上到下
    int edge;     //精度条和边界间距
} ViewShapeProcessBar_Type;

//滚动条
typedef struct ViewShapeScrollBarType
{
    int rad;            //圆角半径
    int lineSize;       //线宽
    int percent;        //百分数值
    int percentOfTotal; //滚动部分占总长比例
    int mode;           //0:左-右滚动 1:上-下滚动
} ViewShapeScrollBar_Type;

//开关
typedef struct ViewShapeSwitchType
{
    int rad;       //圆角半径
    int lineSize;  //线宽
    int status;    //0/关 1/开
    int printType; //字体 不填/默认240
    int mode;      //0/[关/开] 1/圆心圆 2/保留
} ViewShapeSwitch_Type;

//形状汇总
typedef union ViewShapeUnion {
    ViewShapeRect_Type rect;
    ViewShapePara_Type para;
    ViewShapeLine_Type line;
    ViewShapeCircle_Type circle;
    ViewShapeSector_Type sector;
    ViewShapeProcessBar_Type processBar;
    ViewShapeScrollBar_Type scrollBar;
    ViewShapeSwitch_Type sw;
} ViewShape_Union;

//========== 回调函数 和 返回标准 ==========
typedef enum
{
    CALLBACK_IGNORE = -3, //直接结束该view的绘制(在"绘制前回调函数"返回该值将跳过该view的绘制)
    CALLBACK_BREAK = -2,  //结束整个界面的绘制(在"绘制前回调函数"操作后有重要的结构更改,继续往下绘制可能出错)
    CALLBACK_ERR = -1,    //指针检查为NULL, 操作失败等常规错误
    CALLBACK_OK = 0,
} ViewCallBackReturn_Type;

/*
 *  用于view的绘制前(viewStart),绘制后(viewEnd)回调函数
 *  参数:
 *      own： 为当前 *view
 *      object: 工程的总结构体指针
 *      focus: View_Focus *
 *      event: ViewButtonTouch_Event *
 *  example:
 *  int fun(View_Struct *view, void *object, View_Focus *focus, ViewButtonTouch_Event *event)
 *  {
 *      return CALLBACK_OK;
 *  }
 */
typedef int (*ViewCallBack)(void *own, void *object, void *focus, void *event);

/*
 *  用于view的绘制前(drawStart),绘制后(drawEnd)回调函数
 *  example:
 *  void fun(View_Struct *view, int xyLimit[2][2])
 *  {
 *  }
 */
typedef void (*DrawCallBack)(void *own, int xyLimit[2][2]);

/*
 *  用于自定义光标效果的回调函数(focusCallBack 或 focusCallBackFront 用到)
 *  example:
 *  void fun(View_Struct *view, View_Focus *focus, int xyLimit[2][2])
 *  {
 *  }
 */
typedef int (*FocusCallBack)(void *own, void *focus, int xyLimit[2][2]);

//========== 控件参数 ==========
typedef enum
{
    VWHT_MATCH = -10000,  //和父控件同长, VWHT_MATCH*n-m 表示父控件的 m/n 长 // 注意: n,m < 100
    VWHT_FULL = -1000000, //和屏幕同长, VWHT_FULL*n-m 表示屏幕的 m/n 长     // 注意: n,m < 100
} ViewWidthHeight_Type;

typedef enum
{
    VRNT_PARENT = 0,
    VRNT_LAST = -10000,   //相对对象为前一个, VRNT_LAST-n 表示前 n 个  // 注意: n < 100
    VRNT_NEXT = -1000000, //相对对象为后一个, VRNT_NEXT-n 表示后 n 个  // 注意: n < 100
} ViewWRelativeNumber_Type;

typedef enum
{
    VRT_TOP = 0x01,
    VRT_BOTTOM = 0x03, //和 VRT_TOP 互斥使用
    VRT_LEFT = 0x10,
    VRT_RIGHT = 0x30, //和 VRT_LEFT 互斥使用
} ViewRelative_Type;

typedef struct
{
    char *valueOutput;
    int type;
    int vSize;
    int valueOutputLen;
} ViewPrint_Struct;

//========== view 结构 ==========
typedef struct ViewStruct
{
    //==================== 链表参数 [系统赋值] ====================

    int number; //自己作为子级时, 在同级链表中的序号(从1数起)
    int total;  //自己作为父级时, 子view链表中view的总数
    struct ViewStruct *next;
    struct ViewStruct *last;
    struct ViewStruct *parent;

    //==================== 基本参数 ====================

    //途私有数据(系统回收内存时会调free(privateData),仅仅释放了该指针的内存,所以注意不要二级分配内存)
    void *privateData;

    //指针参数,回收时不会释放
    void *privatePoint;

    //名称
    char name[64];

    //失能该view,后面的回调函数和绘图全部不进行
    bool disable;

    //绘制前 和 绘制后 回调,由用户自定义,常用于数据更新,动画
    ViewCallBack viewStart;
    ViewCallBack viewEnd;

    //不绘制当前 view 的绘制内容及子 view 都不作绘制 (位置参数还是会计算的)
    bool invisible;

    //==================== 跳转替换 ====================

    //跳转 view, 启用时后面的 绘图 和 子 view 不绘制
    struct ViewStruct *jumpView;
    //在当前 view 按下确认后,有 jumpView 则跳转,并自动置为 true; 返回时自动置 false [半系统赋值]
    bool jumpViewOn;
    //默认在绘制 jumpView 时屏蔽当前 unit,要同时可见置为 true [主要用于 Home 界面]
    bool jumpViewKeepOn;
    //在使用 jumpView 之后, jumpView 通过 backView 来返回上一级
    struct ViewStruct *backView;

    //==================== 位置参数 ====================

    //绘制同步标志,用于标记当前 view 是否已经计算过位置信息 [系统赋值]
    //通过缓存该值,比较是否每次+1来判断是否第一次进入自己的界面
    uint8_t drawSync;
    uint8_t drawSyncOld;

    //宽高 直接指定数值或使用如下宏
    //宏定义: VWHT_MATCH, VWHT_MATCH*n : 父级控件长,父级控件长的1/n
    //宏定义: VWHT_FULL, VWHT_FULL*n : 屏幕长,屏幕长的1/n
    int width, height;

    //指定相对 view 的序号(从1数起),为0时相对父控件
    //宏定义: VRNT_LAST, VRNT_LAST*n : 前一个,前 n 个
    //宏定义: VRNT_NEXT, VRNT_NEXT*n : 后一个,后 n 个
    int rNumber;

    //相对位置关系,用 VRT_TOP/VRT_BOTTOM 和 VRT_LEFT/VRT_RIGHT 组合
    //      例如左上: rType = VRT_LEFT | VRT_TOP;
    //      例如右下: rType = VRT_RIGHT | VRT_BOTTOM;
    //当 rType = 0 (也就是不填), 判定当前 view 的坐标和 rNumber 指定的 view 的左上角坐标一致
    int rType;

    //相对偏移量
    //相对对象为其它 view 时(rNumber != 0): 下面数值为外偏移量,也就是靠在一起时的间距
    //相对对象为父控件时(rNumber = 0): 下面数值为内偏移量,也就是在肚子里的相对偏移量
    //      例如: rNumber=0; rType=VRT_BOTTOM;
    //           当前 view 在父控件内部的左下角, rTopBottomErr 将指定与父控件底线的距离 rLeftRightErr 与左边的距离
    //      例如: rNumber=VRNT_LAST; rType=VRT_RIGHT;
    //           当前 view 在上一个 view 的右边,且上边平齐, rTopBottomErr 将指定上边往下平移量 rLeftRightErr 往右平移量
    //      例如: rNumber=VRNT_LAST; rType=0;
    //           当前 view 与上一个 view 的左上角坐标对齐, rTopBottomErr 将指定往下平移量 rLeftRightErr 往右平移量
    int rTopBottomErr;
    int rLeftRightErr;

    //横/纵向 相对父控件居中(优先于上面 rTopBottomErr rLeftRightErr 判定)
    bool centerHor, centerVer;

    //真实宽高 [系统赋值]
    int absWidth, absHeight;

    //控件在屏幕中的绝对位置范围 [系统赋值]
    int absXY[2][2];

    //允许超出父控件范围绘制
    bool overDraw;

    //==================== 绘图部分 ====================

    //不绘制当前 view 的绘制内容
    bool hide;

    //---------- 自定义绘图 ----------

    DrawCallBack drawStart;

    //---------- 背景色 ----------

    //背景颜色的颜色,绘制范围为 absXY[2][2]
    ViewValue_Format *backGroundColor;
    int backGroundRad;     //圆角
    float backGroundAlpha; //透明度 0~1 1表示完全透明

    //---------- 形状 ----------

    //形状 0 默认为矩形
    ViewShape_Type shapeType;

    //形状具体数据
    ViewShape_Union shape;

    //相对于控件在4个方向上的缩进
    int shapeTopEdge, shapeBottomEdge;
    int shapeLeftEdge, shapeRightEdge;

    //形状的真正绘制范围 [系统赋值]
    int shapeAbsXY[2][2];

    //不同形状 shapeColorPrint 参数意义不同(看上面"104行"的说明)
    ViewValue_Format *shapeColorPrint;      //默认为打印色
    ViewValue_Format *shapeColorBackground; //默认为打底色

    //透明度 0~1, 1表示完全透明
    float shapeAlpha;

    //---------- 图片 ----------

    //图片路径,不为NULL时启用
    char *picPath;
    //备用指针, 用来检查图片是否已更改 [系统赋值]
    char *picPath_bak;

    //文字输出框相对于控件在4个方向上的缩进,用以确定输出范围
    int picTopEdge, picBottomEdge;
    int picLeftEdge, picRightEdge;

    //图片实际输出范围 [系统赋值]
    int picAbsXY[2][2];

    //图片原始数据 [系统赋值]
    uint8_t *pic;
    //指向图片原始数据的指针矩阵 [系统赋值]
    uint8_t ***picMap;

    //图片的实际 宽/高/像素字节 [系统赋值]
    int picWidth, picHeight, picPB;

    //使用替换颜色
    bool picUseReplaceColor;             //启用替换
    int picReplaceColor;                 //目标颜色值
    ViewValue_Format *picReplaceColorBy; //替换成

    //使用透过颜色
    bool picUseInvisibleColor; //启用透明色
    int picInvisibleColor;     //指定透明色

    //透明度 0~1, 1表示完全透明
    float picAlpha;

    //---------- 内容 ----------

    //数据
    ViewValue_Format *value;

    //备用数据(比如 value 要输出字符串内容, valueBackup 来备份其整形值)
    ViewValue_Format *valueBackup;

    //数据输出缓冲区 [系统赋值]
    ViewPrint_Struct valuePrint;

    //最终输出的字符串指针 [系统赋值]
    char *valueOutput;

    //文字输出框相对于控件在4个方向上的缩进,用以确定输出范围
    int valueTopEdge, valueBottomEdge;
    int valueLeftEdge, valueRightEdge;

    //文字输出范围 [系统赋值]
    int valueAbsXY[2][2];

    //文字在输出范围内的 横/纵向 相对位置 0/居中(默认) 1/向上对齐 2/向下对齐
    int valueHorType, valueVerType;

    //字体代号,例如:160,240,320,400,480,560,640 前两位表示字号,最后位表示线宽(0表示最细)
    int valueType;

    //字颜色
    ViewValue_Format *valueColor;

    //横向字间距
    int valueXEdge;

    //纵向字间距,>0时启用自动换行,并作为行间隔
    int valueYEdge;

    //滚动(和 valueEdge 互斥使用) >0时启用并作为平移像素量
    int scroll;
    //计数,当前平移量 = scrollCount * scroll [系统赋值]
    int scrollCount;
    //多少个绘制周期算一次计数
    int scrollPeriod;
    //计数,满 scrollPeriod 时 scrollCount+1 [系统赋值]
    int scrollCount2;

    //透明度 0~1, 1表示完全透明
    float valueAlpha;

    //---------- 下划线 ----------

    //下划线 >0启用并表示线宽,与控件的 width 同长度
    int bottomLine;
    ViewValue_Format *bottomLineColor;
    float bottomLineAlpha;

    //---------- 描边 ---------- [主要用于 focus]

    //描边,>0时启用,并作为描边的宽度
    int side;

    //描边的颜色
    ViewValue_Format *sideColor;

    //---------- 其它标注 ----------

    //右上角特殊标记,例如未读的邮件
    bool mark;

    //系统滴答时钟,省去回调函数内周期任务的计时 [系统赋值]
    int tickMs;

    //---------- 自定义绘图 ----------

    DrawCallBack drawEnd;

    //==================== 操作 ====================

    //焦点遍历时可在此停留
    bool focusStop;

    //锁定状态,不能操作,变成灰色
    bool lock;

    //在 "Focus下点击确认" 或 "触摸点击" 判定为该控件时, 响应的回调函数
    ViewCallBack callBack;

    //接收拖动触屏事件
    bool enMoveEvent;

    //接收任何输入(不包括上面 MoveEvent)
    bool enAnyInput;

    //该控件触屏时没有震动
    bool silence;

    //第一次绘制该界面
    bool firstIn;

    //==================== 子 view ====================

    //不绘制子 view
    bool hideView;

    //子view 链表的头/尾
    struct ViewStruct *view;
    struct ViewStruct *lastView;

    //==================== focus 指针 ====================

    //当前控件被光标标记时,该指针不为空,api会使用focus内的属性对当前控件进行附加的绘制 [系统赋值]
    void *focus;

    //自定义当前控件的光标效果
    FocusCallBack focusCallBackFront; //在控件之前绘制
    FocusCallBack focusCallBack;

    //==================== 互斥标志 ====================

    //当前 view 正在绘制,禁止在按键或触屏的回调函数里操作该 view [系统赋值]
    bool callBackForbid;

} View_Struct;

//========== focus 结构 ==========
typedef enum
{
    VFE_NULL = 0,
    VFE_RETURN,
    VFE_ENTER,
    VFE_NEXT,
    VFE_LAST,
    VFE_HOME,
} ViewFocus_Event;

typedef struct ViewFocus
{
    View_Struct *topView;
    View_Struct *view;       //当前view
    ViewValue_Format *color; //聚焦时绘制颜色
    int lineSize;
    float alpha;
} View_Focus;

//========== 触屏事件 结构 ==========

typedef enum
{
    // _M_ 连续拖动,手指不离开屏幕的连续拖动的方向
    // unit->enMoveEvent = true 的控件可以收到该类型事件
    VBTT_M_LEFT = -8,
    VBTT_M_DOWN = -7,
    VBTT_M_RIGHT = -6,
    VBTT_M_UP = -5,
    // _S_ 划屏,手指的一次触屏到离开,从其轨迹得到的方向
    VBTT_S_LEFT = -4,
    VBTT_S_DOWN = -3,
    VBTT_S_RIGHT = -2,
    VBTT_S_UP = -1,
    //
    VBTT_NULL = 0,
    //
    VBTT_CLICK_DOWN = 1,
    VBTT_CLICK_UP = 2,
} ViewButtonTouch_Type;

typedef enum
{
    VBTS_NULL = 0,
    VBTS_DOWN = 1,
    VBTS_MOV = 2,
    VBTS_UP = 3,
} ViewButtonTouch_Status;

typedef struct ViewButtonTouchEvent
{
    ViewButtonTouch_Type type; //触屏事件
    int time;                  //触屏维持时间(ms)
    int move;                  //触屏移动量(像素) type = VBTT_M_XXX 或 VBTT_S_XXX 时用到
    ViewButtonTouch_Status status;
} ViewButtonTouch_Event;

//========== 基本数据的 生成/重设/释放 方法 ==========

ViewValue_Format *viewValue_init(
    char *name,
    ViewValue_Type type,
    int valueNum, ...);
ViewValue_Format *viewValue_reset(
    ViewValue_Format *vvf,
    char *name,
    ViewValue_Type type,
    int valueNum, ...);
void viewValue_release(ViewValue_Format *vvf);
bool viewValue_compare(ViewValue_Format *vvf, ViewValue_Format *vvfArray, int *retNum);
int viewValue_find(ViewValue_Format *vvfArray, ...);
ViewValue_Format *viewValue_copy(ViewValue_Format *vvf1, ViewValue_Format *vvf2);
ViewValue_Format *viewValue_arrayAdd(ViewValue_Format *vvf, ...);
ViewValue_Format *viewValue_arrayRemoveByNum(ViewValue_Format *vvf, int num);

//ViewValue_Format 的文件读/写
int viewValue_save(char *filePath, ViewValue_Format *array, int arrayLen);
int viewValue_load(char *filePath, ViewValue_Format *array, int arrayLen);

#endif
