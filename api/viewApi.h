
#ifndef _VIEWAPI_H_
#define _VIEWAPI_H_

#include <time.h>

#include "viewPlat.h"
#include "viewType.h"
#include "viewSrc.h"

#include "bmp.h"      //图片解析
#include "jpegType.h" //图片解析
#include "pngType.h"  //图片解析
#include "ttfType.h"  //矢量字体解析

//--------------------  UI系统初始化 --------------------

void viewApi_init(void);

//--------------------  基本画图接口 --------------------

//画点
void print_dot(int x, int y, uint32_t color);
//用rgb颜色清屏
void print_clean(uint32_t color);
//使能输出
void print_en(void);

//--------------------- 延时和时间 --------------------

void view_delayus(uint32_t us);
void view_delayms(uint32_t ms);

long view_tickUs(void);
int view_tickMs(void);

// 周期延时初始化
#define VIEW_DELAY_INIT \
    long _tick1 = 0, _tick2;

// 周期延时,按时差变化动态延时
#define VIEW_DELAY_US(us)                        \
    _tick2 = view_tickUs();                      \
    if (_tick2 > _tick1 && _tick2 - _tick1 < us) \
        view_delayus(us - (_tick2 - _tick1));    \
    _tick1 = view_tickUs();

// 获取系统时间
struct tm *view_time(void);

//--------------------- 颜色和图片 --------------------

//颜色透明度快速配置
#define view_set_alpha(color, alpha) (((View_Point *)&color)->a = alpha & 0xFF)
#define view_get_alpha(color) ((View_Point *)&color)->a

void view_set_picAlpha(uint32_t *pic, uint8_t alpha, int width, int height, int pb);

//图片数据格式整理
void view_RGB_to_BGRA(uint8_t **pic, int width, int height);
uint32_t *view_getPic(char *picPath, int *width, int *height, int *pb);

/*
 *  为什么要用 BGRA ?
 * 
 *  因为在用4字节整形表示颜色时,可以把不透明红色写作0xFF0000,
 *  把半透明红色写作0x80FF0000,而这个整形的4字节内存顺序即为BGRA,
 * 
 */

//-------------------- 快速节点配置 --------------------

View_Struct *view_init(
    char *name,
    int width,
    int height,
    int rType,
    int rNumber);

//---------------------- 链表操作 ---------------------

//往 parent 的子链表添加 view,front=false 添加到尾,front=true 添加到头
void view_add(View_Struct *parentView, View_Struct *view, bool front);
//往 nodeView 所在链表插入 view,front=false 插入到 nodeView 后面,front=true 插入到 nodeView 前面
void view_insert(View_Struct *nodeView, View_Struct *view, bool front);
//在 view 所在链表中安全的移除 view
void view_remove(View_Struct *view);
//在 view 所在链表中安全的移除 view,并添加到垃圾桶中(内存将被释放掉,不允许继续访问这个 *view 指针)
void view_delete(View_Struct *view);
//父子相认
bool view_isChild(View_Struct *parent, View_Struct *child);
//返回与 view 同链表中序号为 n 的那个 view (n从1数起)
View_Struct *view_num(View_Struct *view, int n);

//------------- 垃圾桶里的 unit/view 的释放 -------------

void viewTrash_clean(void);

//-------------------- 控件的绘制 ----------------------

int view_draw(void *object, View_Struct *view);
int view_draw2(
    void *object,
    View_Focus *focus,
    ViewButtonTouch_Event *event,
    View_Struct *viewParent,
    View_Struct *view,
    int xyLimit[2][2]);

//---------------------- 触屏的遍历 ---------------------

ViewCallBack view_touchLocal(
    int xy[2],
    View_Struct *view,
    View_Struct **retView);

//----------------------- Focus 操作 -------------------

View_Focus *view_focusInit(View_Struct *topView, View_Struct *cView, uint32_t color);
void view_focusRecover(View_Focus *focus);
void view_focusNote(View_Focus *focus, View_Struct *view);
void view_focusJump(View_Focus *focus, View_Struct *jumpView);
ViewCallBack view_focusEvent(View_Focus *focus, ViewFocus_Event event);

//----------------------- 输入方法封装 -------------------

//默认删除字符, 在候选数组*candidate中排在第一个的字符为' '时作删除符使用
#define VIEW_DEL_CHAR ' '

void view_input(
    char *label,
    ViewValue_Format *value,
    ViewValue_Format *candidate,
    View_Struct *backView,
    View_Struct *vsParent,
    View_Focus *focus,
    ViewCallBack callBack,
    bool callBackNext,
    View_Struct *callBackNextView,
    int astrict);

//-------------- 根据内容和限制宽度,获取可能的最大字体 ----------

//根据txt长度自动推荐字体,最小返回280
int view_getType(char *text, int width, int xEdge);

//---------------------- 公共绘图方法 ------------------------

void view_dot(
    uint32_t color,
    int xStart, int yStart,
    int size);
void view_line(
    uint32_t color,
    int xStart, int yStart,
    int xEnd, int yEnd,
    int size, int space);
void view_circle(
    uint32_t color,
    int xStart, int yStart,
    int rad, int size,
    int minX, int minY,
    int maxX, int maxY);
void view_circleLoop(
    uint32_t color,
    int xStart, int yStart,
    int rad,
    int size,
    int div,
    int divStart, int divEnd);
void view_rectangle(
    uint32_t color,
    int xStart, int yStart,
    int xEnd, int yEnd,
    int size, int rad,
    int minX, int minY,
    int maxX, int maxY);
void view_rectangle_padding(
    uint32_t *pic,
    int xStart, int yStart,
    int xEnd, int yEnd,
    int picWidth, int picHeight,
    bool useReplaceColor,
    uint32_t replaceColor,
    uint32_t replaceColorBy,
    int xMin, int yMin, int xMax, int yMax);
void view_parallelogram(
    uint32_t color,
    int xStart, int yStart,
    int xEnd, int yEnd,
    int size, int width,
    int minX, int minY,
    int maxX, int maxY);
void view_printBitMap(
    uint32_t fColor, uint32_t bColor,
    int xStart, int yStart,
    int xScreenStart, int yScreenStart,
    int xScreenEnd, int yScreenEnd,
    Ttf_Map map);
void view_string(
    uint32_t fColor, uint32_t bColor,
    char *str,
    int xStart, int yStart,
    int type, int space);
void view_string_rectangle(
    uint32_t fColor, uint32_t bColor,
    char *str,
    int xStart, int yStart,
    int strWidth, int strHight,
    int xScreenStart, int yScreenStart,
    int xScreenEnd, int yScreenEnd,
    int type, int space);
int view_string_rectangleLineWrap(
    uint32_t fColor, uint32_t bColor,
    char *str,
    int xStart, int yStart,
    int strWidth, int strHight,
    int xScreenStart, int yScreenStart,
    int xScreenEnd, int yScreenEnd,
    int type,
    int xSpace, int ySpace,
    int *retWordPerLine,
    int *retLine);
int view_string_rectangleCR(
    uint32_t fColor, uint32_t bColor,
    char *str,
    int xStart, int yStart,
    int strWidth, int strHight,
    int xScreenStart, int yScreenStart,
    int xScreenEnd, int yScreenEnd,
    int type, int space, int xErr);

#endif
