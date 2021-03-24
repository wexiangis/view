
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

//传入字体文件ttf的路径
void viewApi_init(char *ttfFile);

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

//颜色透明度快速配置(alpha,0~255,越大越透明)
#define view_set_alpha(color, alpha) (((View_Point *)&color)->a = alpha & 0xFF)
#define view_get_alpha(color) ((View_Point *)&color)->a

/*
 *  图片透明图设置
 *  参数:
 *      pic: BGRA图片数据
 *      alpha: 透明度,0~255,越大越透明
 *      pb: 每像素字节数,必须为4(即BGRA),否则图片不支持透明度
 */
void view_set_picAlpha(uint32_t *pic, uint8_t alpha, int width, int height, int pb);

/*
 *  把RGB数据转换为BGRA(重新分配内存)
 *  参数:
 *      pic: RGB数据起始地址的指针
 *
 *  注意: 处理后原地址会被free(),要求原地址内存必须是malloc或calloc所得
 */
void view_RGB_to_BGRA(uint8_t **pic, int width, int height);

/*
 *  读取png、jpg、bmp数据,并转换为BGRA格式
 *  参数:
 *      picPath: 图片路径,支持bmp、jpg、png格式
 *      pb: 返回每像素字节数,必定为4
 *
 *  返回: BGRA格式的图片数据,NULL失败
 */
uint32_t *view_getPic(char *picPath, int *width, int *height, int *pb);

/*
 *  为什么要用 BGRA ?
 * 
 *  因为在用4字节整形表示颜色时,可以把不透明红色写作0xFF0000,
 *  把半透明红色写作0x80FF0000,而这个整形的4字节内存顺序即为BGRA,
 * 
 */

//-------------------- 快速节点配置 --------------------

/*
 *  链表节点(视图、控件、单元?随便你叫)的初始化
 *  参数:
 *      name: 不超过31字节长度名称指针,可以用 name = “xxx” 方式赋值
 *      width,height: 节点最终占用像素宽高,拓展赋值参考 viewType.h 中 ViewWH_Type
 *      rType: 相对布局方式,参考 viewType.h 中 ViewRelative_Type
 *      rNumber: 相对对象在链表中的序号,参考 viewType.h 中 ViewWRelativeNumber_Type
 *  返回: 生成的节点
 *  注意: 节点虽然由calloc生成,但不要自行free,有专门的回收函数 view_delete
 */
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

/*
 *  下拉列表注册
 *  参数:
 *      *label: 说明
 *      *value: 被编辑对象
 *      *candidate: 候选数组
 *      *backView: 输入结束后光标返回的控件
 *      *vsParent: 输入界面布局参考对象
 *      *focus:
 *      callBack: 输入(按确认键)结束后回调
 *      callBackNext: 输入(按确认键)结束后返回到 *callBackNextView 或 *backView->next
 *      *callBackNextView: callBackNext=true时, 输入(按确认键)结束后返回到 *callBackNextView 优先于 *backView->next
 *      astrict: 限制输入长度(针对横向列表会无限增长问题), 0/不限制
 */
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

/*
 *  功能: 画点函数
 *  参数:
 *      color: 颜色ARGB格式,示例: 纯红0xFF0000,半透明纯红0x80FF0000
 *      xStart,yStart :
 *      size: 最小为1
 */
void view_dot(
    uint32_t color,
    int xStart, int yStart,
    int size);

/*
 *  功能: 划线函数
 *  参数:
 *      color: 颜色ARGB格式,示例: 纯红0xFF0000,半透明纯红0x80FF0000
 *      xStart, yStart, xEnd, yEnd: 起止坐标
 *      size: 线宽
 *      space: 不为0时画的是虚线, 其值代表虚线的点密度
 */
void view_line(
    uint32_t color,
    int xStart, int yStart,
    int xEnd, int yEnd,
    int size, int space);

/*
 *  功能: 画圆或圆环
 *  参数:
 *      color: 颜色ARGB格式,示例: 纯红0xFF0000,半透明纯红0x80FF0000
 *      xStart,yStart: 圆心
 *      rad: 半径(外经)
 *      size: 半径向里画环的圈数  0:完全填充, >0: 画环
 */
void view_circle(
    uint32_t color,
    int xStart, int yStart,
    int rad, int size,
    int minX, int minY,
    int maxX, int maxY);

/*
 *  功能: 画圆环,扇形,扇形圆环
 *  参数:
 *      color: 颜色ARGB格式,示例: 纯红0xFF0000,半透明纯红0x80FF0000
 *      xStart,yStart: 圆心
 *      rad: 半径(外经)
 *      size: 半径向里画环的圈数  0:完全填充, >0: 画环
 *      div: 把圆拆分多少分  0或1: 画整圆, >1: 拆分多份(此时 divStart, divEnd 参数有效)
 *      divStart, divEnd: 只画 divStart ~ divEnd 的圆环
 */
void view_circleLoop(
    uint32_t color,
    int xStart, int yStart,
    int rad, int size, int div,
    int divStart, int divEnd);

/*
 *  功能: 画矩形
 *  参数:
 *      color: 颜色ARGB格式,示例: 纯红0xFF0000,半透明纯红0x80FF0000
 *      xStart, yStart, xEnd, yEnd: 起止坐标
 *      size: 线宽
 *      rad: 圆角半径
 *      minY, maxY: 超出上下 Y 坐标部分不绘制
 */
void view_rectangle(
    uint32_t color,
    int xStart, int yStart,
    int xEnd, int yEnd,
    int size, int rad,
    int minX, int minY,
    int maxX, int maxY);

/*
 *  用pic(BGRA格式)数据填充矩形
 *  参数:
 *      pic: BGRA排列的图片数据,字节长度要求 picWidth*picHeight*4
 *      xStart,yStart,xEnd,yEnd: 原图输出范围
 *      picWidth,picHeight: 图片宽高
 *      useReplaceColor: 是否启用颜色替换
 *      replaceColor,replaceColorBy: 寻找颜色和替换成的颜色,颜色ARGB格式,示例: 纯红0xFF0000,半透明纯红0x80FF0000
 *      xMin,yMin,xMax,yMax: 输出屏幕限制范围,包含2个End所在点
 */
void view_rectangle_padding(
    uint32_t *pic,
    int xStart, int yStart,
    int xEnd, int yEnd,
    int picWidth, int picHeight,
    bool useReplaceColor,
    uint32_t replaceColor,
    uint32_t replaceColorBy,
    int xMin, int yMin, int xMax, int yMax);

/*
 *  功能: 画平行四边形
 *  参数:
 *      color: 颜色ARGB格式,示例: 纯红0xFF0000,半透明纯红0x80FF0000
 *      xStart, yStart, xEnd, yEnd: 起止坐标, 平行四边形 左上 和 右下 的坐标
 *      size: 线宽
 *      width: 平行四边形上边长度
 *      xMin,yMin,xMax,yMax: 输出屏幕限制范围,包含2个End所在点
 *  返回: 无
 */
void view_parallelogram(
    uint32_t color,
    int xStart, int yStart,
    int xEnd, int yEnd,
    int size, int width,
    int minX, int minY,
    int maxX, int maxY);

/*
 *  根据 ttf_map 画点阵,增加范围限制和透明度参数
 *  参数:
 *      fColor,bColor: 写颜色和背景颜色ARGB,示例: 纯红0xFF0000,半透明纯红0x80FF0000
 *      xStart,yStart: 开始写入的左上角坐标在屏幕的位置
 *      xScreenXXX: 限制在屏幕上的输出范围,2个End的所在点也包含在内
 *      map: 使用 ttf_getMapByUtf8 解析得到的文字矩阵信息
 */
void view_printBitMap(
    uint32_t fColor, uint32_t bColor,
    int xStart, int yStart,
    int xScreenStart, int yScreenStart,
    int xScreenEnd, int yScreenEnd,
    Ttf_Map map);

/*
 *  功能: 字符串输出
 *  参数:
 *      fColor,bColor: 写颜色和背景颜色ARGB格式,示例: 纯红0xFF0000,半透明纯红0x80FF0000
 *      str: 字符串
 *      xStart, yStart: 矩阵的左上角定位坐标
 *      type: 字体, 例如 160, 240, 320, 400, 480, 560, 640, 前两位标识像素尺寸, 后1位表示字体
 *      space: 字符间隔, 正常输出为0
 *  返回: 无
 */
void view_string(
    uint32_t fColor, uint32_t bColor,
    char *str,
    int xStart, int yStart,
    int type, int space);

/*
 *  功能: 字符串输出, 带范围限制
 *  参数:
 *      fColor,bColor: 写颜色和背景颜色ARGB格式,示例: 纯红0xFF0000,半透明纯红0x80FF0000
 *      str: 字符串
 *      xStart, yStart: 矩阵的左上角定位坐标
 *      strWidth, strHight: 相对左上角定位坐标, 限制宽, 高的矩阵内输出字符串
 *      type: 字体, 例如 160, 240, 320, 400, 480, 560, 640, 前两位标识像素尺寸, 后1位表示字体
 *      space: 字符间隔, 正常输出为0
 *  返回: 无
 */
void view_string_rectangle(
    uint32_t fColor, uint32_t bColor,
    char *str,
    int xStart, int yStart,
    int strWidth, int strHight,
    int xScreenStart, int yScreenStart,
    int xScreenEnd, int yScreenEnd,
    int type, int space);

/*
 *  功能: 字符串输出, 带范围限制, 自动换行
 *  参数:
 *      fColor,bColor: 写颜色和背景颜色ARGB格式,示例: 纯红0xFF0000,半透明纯红0x80FF0000
 *      str: 字符串
 *      xStart, yStart: 矩阵的左上角定位坐标
 *      strWidth, strHight: 相对左上角定位坐标, 限制宽, 高的矩阵内输出字符串
 *      type: 字体, 例如 160, 240, 320, 400, 480, 560, 640, 前两位标识像素尺寸, 后1位表示字体
 *      xSpace, ySpace: 字符间隔, 正常输出为0
 *      lineSpace: 上下行间隔
 *      retWordPerLine: 传入记录每行占用字节数的数组指针, 不用可置NULL
 *      retLine: 传入记录占用行数的指针, 不用可置NULL
 * 返回: 成功输出的字符数
 */
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

/*
 *  功能: 字符串输出, 带范围限制, 加滚动
 *  参数:
 *      fColor,bColor: 写颜色和背景颜色ARGB格式,示例: 纯红0xFF0000,半透明纯红0x80FF0000
 *      str: 字符串
 *      xStart, yStart: 矩阵的左上角定位坐标
 *      strWidth, strHight: 相对左上角定位坐标, 限制宽, 高的矩阵内输出字符串
 *      type: 字体, 例如 160, 240, 320, 400, 480, 560, 640, 前两位标识像素尺寸, 后1位表示字体
 *      space: 字符间隔, 正常输出为0
 *      xErr: 相对 xStart 坐标, 字符串输出前先按xErr的 负/正的量 进行 左/右偏移一定像素
 * 返回: 返回此次绘制的偏差值, 以便后续无缝衔接
 */
int view_string_rectangleCR(
    uint32_t fColor, uint32_t bColor,
    char *str,
    int xStart, int yStart,
    int strWidth, int strHight,
    int xScreenStart, int yScreenStart,
    int xScreenEnd, int yScreenEnd,
    int type, int space, int xErr);

#endif
