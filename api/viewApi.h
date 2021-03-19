
#ifndef __VIEWAPI_H_
#define __VIEWAPI_H_

#include <time.h>

#include "viewPlat.h"
#include "viewType.h"
#include "viewSrc.h"

#include "bmp.h"      //图片解析
#include "jpegType.h" //图片解析
#include "pngType.h"  //图片解析
#include "ttfType.h"  //矢量字体解析

//========== UI系统初始化 ==========

void view_init(void);

//========== 控件在链表中的 添加/插入/移除 方法 ==========

//往 parent 的子链表添加 view,front=false 添加到尾,front=true 添加到头
void view_add(View_Struct *parentView, View_Struct *view, bool front);
//往 nodeView 所在链表插入 view,front=false 插入到 nodeView 后面,front=true 插入到 nodeView 前面
void view_insert(View_Struct *nodeView, View_Struct *view, bool front);
//在 view 所在链表中安全的移除 view
void view_remove(View_Struct *view);
//在 view 所在链表中安全的移除 view,并添加到垃圾桶中(内存将被释放掉,不允许继续访问这个 *view 指针)
void view_delete(View_Struct *view);
//返回与 view 同链表中序号为 n 的那个 view (n从1数起)
View_Struct *view_num(View_Struct *view, int n);

//========== 垃圾桶里的 unit/view 的释放 ==========

void viewTrash_clean(void);

//=============== viewTool is ? ===============

bool view_isChild(View_Struct *parent, View_Struct *child);

//==================== 控件的绘制 ====================

int view_draw(
    void *object,
    View_Focus *focus,
    ViewButtonTouch_Event *event,
    View_Struct *viewParent,
    View_Struct *view,
    int xyLimit[2][2]);

//==================== 触屏的遍历 ====================

ViewCallBack view_touchLocal(
    int xy[2],
    View_Struct *view,
    View_Struct **retView);

//==================== Focus 操作 ====================

View_Focus *view_focusInit(View_Struct *topView, View_Struct *cView, ViewValue_Format *color);
void view_focusRecover(View_Focus *focus);
void view_focusNote(View_Focus *focus, View_Struct *view);
void view_focusJump(View_Focus *focus, View_Struct *jumpView);
ViewCallBack view_focusEvent(View_Focus *focus, ViewFocus_Event event);

//==================== 输入方法封装 ====================

#define VIEW_DEL_CHAR ' ' //默认删除字符, 在候选数组*candidate中排在第一个的字符为' '时作删除符使用

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

//==================== 根据内容和限制宽度,获取可能的最大字体 ====================

//根据txt长度自动推荐字体,最小返回280
int view_getType(char *text, int width, int xEdge);

//==================== 公共绘图方法 ====================

//全世界都能调用的绘图方法,调用即输出到屏幕缓冲,最后记得用 PRINT_EN() 使能输出
void view_dot(
    int color,
    int xStart, int yStart,
    int size, float alpha);
int view_getDot(
    int xStart, int yStart,
    int xEnd, int yEnd,
    int *dotX, int *dotY);
void view_line(
    int color,
    int xStart, int yStart,
    int xEnd, int yEnd,
    int size, int space, float alpha);
void view_circle(
    int color,
    int xStart, int yStart,
    int rad, int size,
    float alpha,
    int minX, int minY,
    int maxX, int maxY);
void view_circleLoop(
    int color,
    int xStart, int yStart,
    int rad,
    int size,
    int div,
    int divStart, int divEnd);
void view_rectangle(
    int color,
    int xStart, int yStart,
    int xEnd, int yEnd,
    int size, int rad, float alpha,
    int minX, int minY,
    int maxX, int maxY);
void view_rectangle_padding(
    uint8_t *pic,
    int xStart, int yStart,
    int xEnd, int yEnd,
    int picWidth, int picHeight, int picPB,
    bool picUseReplaceColor,
    int picReplaceColor,
    int picReplaceColorBy,
    bool picUseInvisibleColor,
    int picInvisibleColor, float alpha,
    int xMin, int yMin, int xMax, int yMax);
void view_rectangle_padding2(
    uint8_t ***picMap,
    int xStart, int yStart,
    int xEnd, int yEnd,
    int picWidth, int picHeight, int picPB,
    bool picUseReplaceColor,
    int picReplaceColor,
    int picReplaceColorBy,
    bool picUseInvisibleColor,
    int picInvisibleColor, float alpha,
    int xMin, int yMin, int xMax, int yMax);
void view_parallelogram(
    int color,
    int xStart, int yStart,
    int xEnd, int yEnd,
    int size, int width, float alpha,
    int minX, int minY,
    int maxX, int maxY);
void view_printBitMap(
    int fColor, int bColor,
    int xStart, int yStart,
    Ttf_Map map);
void view_printBitMap2(
    int fColor, int bColor,
    int xStart, int yStart,
    int xScreenStart, int yScreenStart,
    int xScreenEnd, int yScreenEnd,
    float alpha,
    Ttf_Map map);
void view_string(
    int fColor,
    int bColor, char *str,
    int xStart, int yStart,
    int type, int space);
void view_string_rectangle(
    int fColor, int bColor,
    char *str,
    int xStart, int yStart,
    int strWidth, int strHight,
    int xScreenStart, int yScreenStart,
    int xScreenEnd, int yScreenEnd,
    int type, int space,
    float alpha);
int view_string_rectangleLineWrap(
    int fColor, int bColor,
    char *str,
    int xStart, int yStart,
    int strWidth, int strHight,
    int xScreenStart, int yScreenStart,
    int xScreenEnd, int yScreenEnd,
    int type,
    int xSpace, int ySpace,
    float alpha,
    int *retWordPerLine,
    int *retLine);
int view_string_rectangleCR(
    int fColor, int bColor,
    char *str,
    int xStart, int yStart,
    int strWidth, int strHight,
    int xScreenStart, int yScreenStart,
    int xScreenEnd, int yScreenEnd,
    int type, int space, int xErr,
    float alpha);

void view_delayms(uint32_t ms);

int view_tickMs(void);
struct tm *view_time(void);

#endif
