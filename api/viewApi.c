
#include <sys/time.h>
#include <unistd.h>

#include "viewApi.h"

typedef union {
    uint8_t map[VIEW_X_SIZE * VIEW_Y_SIZE * VIEW_PB];
    uint8_t grid[VIEW_Y_SIZE][VIEW_X_SIZE][VIEW_PB];
} View_Map;

//公共map指针
static View_Map *view_map = NULL;

//公共顶view
static View_Struct ViewCommonParent = {
    .name = "commonParent",
    .width = VIEW_X_SIZE,
    .height = VIEW_Y_SIZE,
    .absWidth = VIEW_X_SIZE,
    .absHeight = VIEW_Y_SIZE,
    .absXY = {{0, 0}, {VIEW_X_END, VIEW_Y_END}},
    .tickOfDraw = 1,
};

//纵向输入列表最后选中项的序号(从0数起)
static int viewInputSelectNumber = 0;

//垃圾桶
static View_Struct viewTrash = {
    .name = "viewTrash",
};

//--------------------  UI系统初始化 --------------------

/*
 *  UI系统初始化
 *  参数:
 *      传入字体文件ttf的路径
 *  返回: 0成功,其它失败   
 */
int viewApi_init(char *ttfFile)
{
    //平台初始化,获取屏幕缓存
    view_map = (View_Map *)VIEW_MAP_INIT();
    if (!view_map)
    {
        fprintf(stderr, "viewApi_init: VIEW_MAP_INIT failed !!\r\n");
        return -1;
    }
    //配置文件初始化
    viewConfig_init();
    //viewSrc初始化
    viewSrc_init(ttfFile);
    //viewColor初始化
    viewColor_init();
    return 0;
}

//--------------------  基本画图接口 --------------------

void print_dot(int x, int y, uint32_t color)
{
    //边界检查
    if (x < 0 || x > VIEW_X_END ||
        y < 0 || y > VIEW_Y_END)
        return;

    //序列化成结构体,直接忽略掉a值
    View_Point p = *((View_Point *)&color);

    //无透明度
    if (!p.a)
    {
#if(VIEW_FORMAT == COLOR_FORMAT_RGB || VIEW_FORMAT == COLOR_FORMAT_RGBA)
        view_map->grid[y][x][0] = p.r;
        view_map->grid[y][x][1] = p.g;
        view_map->grid[y][x][2] = p.b;
#elif(VIEW_FORMAT == COLOR_FORMAT_BGR || VIEW_FORMAT == COLOR_FORMAT_BGRA)
        view_map->grid[y][x][0] = p.b;
        view_map->grid[y][x][1] = p.g;
        view_map->grid[y][x][2] = p.r;
#elif(VIEW_FORMAT == COLOR_FORMAT_ARGB)
        view_map->grid[y][x][1] = p.r;
        view_map->grid[y][x][2] = p.g;
        view_map->grid[y][x][3] = p.b;
#elif(VIEW_FORMAT == COLOR_FORMAT_ABGR)
        view_map->grid[y][x][1] = p.b;
        view_map->grid[y][x][2] = p.g;
        view_map->grid[y][x][3] = p.r;
#endif
    }
    else if (p.a == 0xFF)
        ;
    //透明则按比例权重
    else
    {
#if(VIEW_FORMAT == COLOR_FORMAT_RGB || VIEW_FORMAT == COLOR_FORMAT_RGBA)
        view_map->grid[y][x][0] = (uint8_t)((view_map->grid[y][x][0] * p.a + p.r * (255 - p.a)) / 255);
        view_map->grid[y][x][1] = (uint8_t)((view_map->grid[y][x][1] * p.a + p.g * (255 - p.a)) / 255);
        view_map->grid[y][x][2] = (uint8_t)((view_map->grid[y][x][2] * p.a + p.b * (255 - p.a)) / 255);
#elif(VIEW_FORMAT == COLOR_FORMAT_BGR || VIEW_FORMAT == COLOR_FORMAT_BGRA)
        view_map->grid[y][x][0] = (uint8_t)((view_map->grid[y][x][0] * p.a + p.b * (255 - p.a)) / 255);
        view_map->grid[y][x][1] = (uint8_t)((view_map->grid[y][x][1] * p.a + p.g * (255 - p.a)) / 255);
        view_map->grid[y][x][2] = (uint8_t)((view_map->grid[y][x][2] * p.a + p.r * (255 - p.a)) / 255);
#elif(VIEW_FORMAT == COLOR_FORMAT_ARGB)
        view_map->grid[y][x][1] = (uint8_t)((view_map->grid[y][x][0] * p.a + p.r * (255 - p.a)) / 255);
        view_map->grid[y][x][2] = (uint8_t)((view_map->grid[y][x][1] * p.a + p.g * (255 - p.a)) / 255);
        view_map->grid[y][x][3] = (uint8_t)((view_map->grid[y][x][2] * p.a + p.b * (255 - p.a)) / 255);
#elif(VIEW_FORMAT == COLOR_FORMAT_ABGR)
        view_map->grid[y][x][1] = (uint8_t)((view_map->grid[y][x][0] * p.a + p.b * (255 - p.a)) / 255);
        view_map->grid[y][x][2] = (uint8_t)((view_map->grid[y][x][1] * p.a + p.g * (255 - p.a)) / 255);
        view_map->grid[y][x][3] = (uint8_t)((view_map->grid[y][x][2] * p.a + p.r * (255 - p.a)) / 255);
#endif
    }
}

void print_clean(uint32_t color)
{
    //序列化成结构体,直接忽略掉a值
    View_Point p = *((View_Point *)&color);
    int i;
    for (i = 0; i < VIEW_X_SIZE * VIEW_Y_SIZE * VIEW_PB;)
    {
#if(VIEW_FORMAT == COLOR_FORMAT_RGB)
        view_map->map[i++] = p.r;
        view_map->map[i++] = p.g;
        view_map->map[i++] = p.b;
#elif(VIEW_FORMAT == COLOR_FORMAT_BGR)
        view_map->map[i++] = p.b;
        view_map->map[i++] = p.g;
        view_map->map[i++] = p.r;
#elif(VIEW_FORMAT == COLOR_FORMAT_RGBA)
        view_map->map[i++] = p.r;
        view_map->map[i++] = p.g;
        view_map->map[i++] = p.b;
        view_map->map[i++] = p.a;
#elif(VIEW_FORMAT == COLOR_FORMAT_BGRA)
        view_map->map[i++] = p.b;
        view_map->map[i++] = p.g;
        view_map->map[i++] = p.r;
        view_map->map[i++] = p.a;
#elif(VIEW_FORMAT == COLOR_FORMAT_ARGB)
        view_map->map[i++] = p.a;
        view_map->map[i++] = p.r;
        view_map->map[i++] = p.g;
        view_map->map[i++] = p.b;
#elif(VIEW_FORMAT == COLOR_FORMAT_ABGR)
        view_map->map[i++] = p.a;
        view_map->map[i++] = p.b;
        view_map->map[i++] = p.g;
        view_map->map[i++] = p.r;
#endif
    }
}

void print_en(void)
{
    VIEW_MAP_EN();
}

//--------------------- 延时和时间 --------------------

void view_delayus(uint32_t us)
{
    struct timeval delay;
    delay.tv_sec = us / 1000000;
    delay.tv_usec = us % 1000000;
    select(0, NULL, NULL, NULL, &delay);
}

void view_delayms(uint32_t ms)
{
    view_delayus(ms * 1000);
}

long view_tickUs(void)
{
    struct timeval tv = {0};
    gettimeofday(&tv, NULL);
    return (long)(tv.tv_sec * 1000000 + tv.tv_usec);
}

int view_tickMs(void)
{
    return (int)(view_tickUs() / 1000);
}

struct tm *view_time(void)
{
    static time_t now;
    time(&now);
    return localtime(&now);
}

//--------------------- 图片内存管理 --------------------

void view_set_picAlpha(uint32_t *pic, uint8_t alpha, int width, int height, int pb)
{
    View_Point *p = (View_Point *)pic;
    int i;
    if (pb != 4)
        return;
    for (i = 0; i < width * height; i++)
        (p++)->a = alpha;
}

void view_RGB_to_BGRA(uint8_t **pic, int width, int height)
{
    uint8_t *picNew = *pic;
    uint8_t *picOld = *pic;
    int i, j;

    picNew = (uint8_t *)calloc(width * height * 4, 1);

    for (i = j = 0; j < width * height * 3; j += 3)
    {
        picNew[i++] = picOld[j + 2];
        picNew[i++] = picOld[j + 1];
        picNew[i++] = picOld[j];
        picNew[i++] = 0;
    }

    *pic = picNew;
    free(picOld);
}

uint32_t *view_getPic(char *picPath, int *width, int *height, int *pb)
{
    uint8_t *ret = NULL;

    //png 格式直接转换BGRA,不需后处理
    if (strstr(picPath, ".png") || strstr(picPath, ".PNG"))
    {
        ret = png_get(picPath, width, height, pb, PT_BGRA);
        if (ret)
            return (uint32_t *)ret;
    }
    //jpeg 和 bmp 读取数据默认为RGB格式,需转为BGRA格式
    else if (strstr(picPath, ".jpg") || strstr(picPath, ".JPG") ||
        strstr(picPath, ".jpeg") || strstr(picPath, ".JPEG"))
        ret = jpeg_get(picPath, width, height, pb);

    if (!ret)
        ret = bmp_get(picPath, width, height, pb);

    //转换格式
    if (ret)
    {
        *pb = 4;
        view_RGB_to_BGRA(&ret, *width, *height);
    }

    return (uint32_t *)ret;
}

//-------------------- 快速节点配置 --------------------

View_Struct *view_init(
    char *name,
    int width,
    int height,
    int rType,
    int rNumber)
{
    View_Struct *vs = (View_Struct *)calloc(1, sizeof(View_Struct));
    strcpy(vs->name, name ? name : "none");
    vs->width = width;
    vs->height = height;
    vs->rType = rType;
    vs->rNumber = rNumber;
    return vs;
}

//-------------------- 垃圾桶系统 --------------------

//垃圾释放
void viewTrash_clean(void)
{
    View_Struct *vsThis, *vsNext;

    //互斥保护
    while (viewTrash.callBackForbid)
        view_delayms(5);
    viewTrash.callBackForbid = true;

    //----- view 链表清空 -----

    vsThis = viewTrash.view;
    while (vsThis)
    {
        // printf("viewTrash_clean: free %s now ...\r\n", vsThis->name);

        vsNext = vsThis->next;

        //回收 view 的私有数据空间
        if (vsThis->privateData)
            free(vsThis->privateData);

        //数据资源释放
        if (vsThis->text && !viewSrc_compare(vsThis->text))
        {
            viewValue_release(vsThis->text);
            vsThis->text = NULL;
        }
        if (vsThis->textBakup && !viewSrc_compare(vsThis->textBakup))
        {
            viewValue_release(vsThis->textBakup);
            vsThis->textBakup = NULL;
        }
        //图片内存释放
        if (vsThis->pic)
        {
            free(vsThis->pic);
            vsThis->pic = NULL;
        }
        //文字输出缓冲区释放
        if (vsThis->textPrint.textOutput)
        {
            free(vsThis->textPrint.textOutput);
            vsThis->textPrint.textOutput = NULL;
        }
        //含有子 view
        if (vsThis->view)
        {
            if (!vsNext)
            {
                vsNext = vsThis->view;
                viewTrash.lastView = vsThis->lastView;
            }
            else
            {
                viewTrash.lastView->next = vsThis->view;
                viewTrash.lastView = vsThis->lastView;
            }
        }
        //完全释放
        free(vsThis);
        //下一个
        vsThis = vsNext;
    }
    //子 view 链表清空
    viewTrash.view = viewTrash.lastView = NULL;
    viewTrash.total = 0;
    viewTrash.callBackForbid = false;
}

//-------------------- 点, 圆, 环 --------------------

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
    int maxX, int maxY)
{
    int circle_a, circle_b;
    int circle_di;
    int circle_rad = rad;
    int circle_size = size;

    int xS = xStart - rad;
    int yS = yStart - rad;
    int xC, yC;
    int xC2, yC2;
    int xySize = rad * 2;
    uint8_t memBUff[rad * 2 + 1][rad * 2 + 1];

    memset(memBUff, 0, (rad * 2 + 1) * (rad * 2 + 1));

    if (circle_size <= 0)
        circle_size = rad;

    for (; circle_rad > rad - circle_size && circle_rad > 0; circle_rad--)
    {
        circle_a = 0;
        circle_b = circle_rad;
        circle_di = 3 - (circle_rad << 1);
        while (circle_a <= circle_b)
        {
            //1
            memBUff[rad - circle_b][rad + circle_a] = 1;
            memBUff[rad - circle_b + 1][rad + circle_a] = 1;
            //2
            memBUff[rad - circle_a][rad + circle_b] = 1;
            memBUff[rad - circle_a][rad + circle_b - 1] = 1;
            //3
            memBUff[rad + circle_a][rad + circle_b] = 1;
            memBUff[rad + circle_a][rad + circle_b - 1] = 1;
            //4
            memBUff[rad + circle_b][rad + circle_a] = 1;
            memBUff[rad + circle_b - 1][rad + circle_a] = 1;
            //5
            memBUff[rad + circle_b][rad - circle_a] = 1;
            memBUff[rad + circle_b - 1][rad - circle_a] = 1;
            //6
            memBUff[rad + circle_a][rad - circle_b] = 1;
            memBUff[rad + circle_a][rad - circle_b + 1] = 1;
            //7
            memBUff[rad - circle_a][rad - circle_b] = 1;
            memBUff[rad - circle_a][rad - circle_b + 1] = 1;
            //8
            memBUff[rad - circle_b][rad - circle_a] = 1;
            memBUff[rad - circle_b + 1][rad - circle_a] = 1;
            //
            circle_a++;
            //使用Bresenham算法画圆
            if (circle_di < 0)
                circle_di += 4 * circle_a + 6;
            else
            {
                circle_di += 10 + 4 * (circle_a - circle_b);
                circle_b--;
            }
        }
    }
    //输出
    for (yC = 0; yC < xySize; yC += 1)
    {
        yC2 = yS + yC;
        if (yC2 < minY || yC2 > maxY)
            continue;
        for (xC = 0; xC < xySize; xC += 1)
        {
            xC2 = xS + xC;
            if (xC2 < minX || xC2 > maxX)
                continue;
            else if (memBUff[yC][xC])
                print_dot(xC2, yC2, color);
        }
    }
}

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
    int divStart, int divEnd)
{
    int circle_a, circle_b;
    int circle_di;
    int circle_rad = rad;
    int circle_size = size;

    int angleCount, arrayCount;
    float rangeArray[8][2];
    float divStartTemp = 0, divEndTemp = 0;

    int intArray[rad + 1][2], sumCount, sumCount2;

    int i;

    if (div < 0 || (div > 1 && (divStart < 1 || divStart > div || divEnd < 1 || divEnd > div)))
        return;

    else if (divEnd < divStart)
    {
        view_circleLoop(color, xStart, yStart, rad, size, div, divStart, div);
        view_circleLoop(color, xStart, yStart, rad, size, div, 1, divEnd);
        return;
    }
    if (circle_rad <= 0)
        return;
    if (circle_size <= 0)
        circle_size = rad;

    if (div > 1)
    {
        divStartTemp = (divStart - 1) * 360.00 / div; //开始度数
        divEndTemp = divEnd * 360.00 / div;           //结束度数
        memset(intArray, 0, (rad + 1) * 2);
    }

    for (; circle_rad > rad - circle_size && circle_rad > 0; circle_rad--)
    {
        circle_a = 0;
        circle_b = circle_rad;
        circle_di = 3 - (circle_rad << 1);

        sumCount = 0;
        while (circle_a <= circle_b)
        {
            if (div < 2) //div < 2 的都是画完整一圈的
            {
                print_dot(xStart + circle_a, yStart - circle_b, color); //1
                print_dot(xStart + circle_b, yStart - circle_a, color); //2
                print_dot(xStart + circle_b, yStart + circle_a, color); //3
                print_dot(xStart + circle_a, yStart + circle_b, color); //4
                print_dot(xStart - circle_a, yStart + circle_b, color); //5
                print_dot(xStart - circle_b, yStart + circle_a, color); //6
                print_dot(xStart - circle_b, yStart - circle_a, color); //7
                print_dot(xStart - circle_a, yStart - circle_b, color); //8
                //if(circle_size > 1)
                {
                    print_dot(xStart + circle_a, yStart - circle_b + 1, color); //补点
                    print_dot(xStart + circle_b - 1, yStart - circle_a, color); //补点
                    print_dot(xStart + circle_b - 1, yStart + circle_a, color); //补点
                    print_dot(xStart + circle_a, yStart + circle_b - 1, color); //补点
                    print_dot(xStart - circle_a, yStart + circle_b - 1, color); //补点
                    print_dot(xStart - circle_b + 1, yStart + circle_a, color); //补点
                    print_dot(xStart - circle_b + 1, yStart - circle_a, color); //补点
                    print_dot(xStart - circle_a, yStart - circle_b + 1, color); //补点
                }
            }
            else //先记下一圈数据, 后期处理后再画
            {
                intArray[sumCount][0] = circle_a;
                intArray[sumCount][1] = circle_b;
                sumCount += 1;
            }

            circle_a++;
            //使用Bresenham算法画圆
            if (circle_di < 0)
                circle_di += 4 * circle_a + 6;
            else
            {
                circle_di += 10 + 4 * (circle_a - circle_b);
                circle_b--;
            }
        }
        //要分段画扇形的在此处理
        if (div > 1)
        {
            //计算范围数组 rangeArray[][]
            for (i = 0, angleCount = 45, arrayCount = 0; i < 8;)
            {
                if (divStartTemp >= angleCount || divEndTemp <= angleCount - 45)
                {
                    rangeArray[arrayCount][0] = 999;
                    rangeArray[arrayCount][1] = -1;
                }
                else
                {
                    if (divStartTemp <= angleCount - 45)
                        rangeArray[arrayCount][0] = -1;
                    else
                        rangeArray[arrayCount][0] = (divStartTemp - (angleCount - 45)) * sumCount / 45 - 1;

                    if (divEndTemp >= angleCount)
                        rangeArray[arrayCount][1] = 999;
                    else
                        rangeArray[arrayCount][1] = (divEndTemp - (angleCount - 45)) * sumCount / 45 + 1;
                }
                angleCount += 45;
                arrayCount += 1;
                i += 1;
            }
            //绘制一圈
            for (i = 0, sumCount2 = sumCount - 1; i < sumCount; i += 1)
            {
                circle_a = intArray[i][0];
                circle_b = intArray[i][1];

                if (circle_a > rangeArray[0][0] && circle_a < rangeArray[0][1])
                {
                    print_dot(xStart + circle_a, yStart - circle_b, color);     //1
                    print_dot(xStart + circle_a, yStart - circle_b + 1, color); //补点
                }
                if (sumCount2 - circle_a > rangeArray[1][0] && sumCount2 - circle_a < rangeArray[1][1])
                {
                    print_dot(xStart + circle_b, yStart - circle_a, color);     //2
                    print_dot(xStart + circle_b - 1, yStart - circle_a, color); //补点
                }
                if (circle_a > rangeArray[2][0] && circle_a < rangeArray[2][1])
                {
                    print_dot(xStart + circle_b, yStart + circle_a, color);     //3
                    print_dot(xStart + circle_b - 1, yStart + circle_a, color); //补点
                }
                if (sumCount2 - circle_a > rangeArray[3][0] && sumCount2 - circle_a < rangeArray[3][1])
                {
                    print_dot(xStart + circle_a, yStart + circle_b, color);     //4
                    print_dot(xStart + circle_a, yStart + circle_b - 1, color); //补点
                }
                if (circle_a > rangeArray[4][0] && circle_a < rangeArray[4][1])
                {
                    print_dot(xStart - circle_a, yStart + circle_b, color);     //5
                    print_dot(xStart - circle_a, yStart + circle_b - 1, color); //补点
                }
                if (sumCount2 - circle_a > rangeArray[5][0] && sumCount2 - circle_a < rangeArray[5][1])
                {
                    print_dot(xStart - circle_b, yStart + circle_a, color);     //6
                    print_dot(xStart - circle_b + 1, yStart + circle_a, color); //补点
                }
                if (circle_a > rangeArray[6][0] && circle_a < rangeArray[6][1])
                {
                    print_dot(xStart - circle_b, yStart - circle_a, color);     //7
                    print_dot(xStart - circle_b + 1, yStart - circle_a, color); //补点
                }
                if (sumCount2 - circle_a > rangeArray[7][0] && sumCount2 - circle_a < rangeArray[7][1])
                {
                    print_dot(xStart - circle_a, yStart - circle_b, color);     //8
                    print_dot(xStart - circle_a, yStart - circle_b + 1, color); //补点
                }
            }
        }
    }
    //输出
}

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
    int size)
{
    if (size == 1)
        print_dot(xStart, yStart, color);
    else if (size == 2)
    {
        //mid
        print_dot(xStart, yStart, color);
        //up
        print_dot(xStart, yStart + 1, color);
        //down
        print_dot(xStart, yStart - 1, color);
        //left
        print_dot(xStart + 1, yStart, color);
        //right
        print_dot(xStart - 1, yStart, color);
    }
    else if (size > 2)
        view_circle(color, xStart, yStart, size, 0, 0, 0, 9999, 9999);
}

//-------------------- 线 --------------------

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
    int size, int space)
{
    unsigned short t;
    int xerr = 0, yerr = 0;
    int delta_x, delta_y;
    int distance;
    int incx, incy, xCount, yCount;
    int spaceCount = 0, spaceVal = 0;

    if (size < 1)
        return;

    delta_x = xEnd - xStart; //计算坐标增量
    delta_y = yEnd - yStart;
    xCount = xStart;
    yCount = yStart;

    if (delta_x > 0)
        incx = 1; //设置单步方向
    else if (delta_x == 0)
        incx = 0; //垂直线
    else
    {
        incx = -1;
        delta_x = -delta_x;
    }

    if (delta_y > 0)
        incy = 1;
    else if (delta_y == 0)
        incy = 0; //水平线
    else
    {
        incy = -1;
        delta_y = -delta_y;
    }

    if (delta_x > delta_y)
        distance = delta_x; //选取基本增量坐标轴
    else
        distance = delta_y;

    if (distance > 1000)
    {
        fprintf(stderr, "%d - %d , %d - %d , %d over line\n",
                xStart, xEnd, yStart, yEnd, distance);
        return;
    }

    if (space == 0)
        spaceVal = 0;
    else if (space > 0)
    {
        spaceVal = space;
        spaceCount = -space;
    }
    else
    {
        spaceVal = -space;
        spaceCount = 0;
    }

    for (t = 0; t <= distance + 1; t += 1) //画线输出
    {
        if (spaceVal == 0 || spaceCount < 0)
        {
            spaceCount += 1;
            view_dot(color, xCount, yCount, size);
        }
        else
        {
            spaceCount += 1;
            if (spaceCount >= spaceVal)
                spaceCount = -spaceVal;
        }

        xerr += delta_x;
        yerr += delta_y;
        if (xerr > distance)
        {
            xerr -= distance;
            xCount += incx;
        }
        if (yerr > distance)
        {
            yerr -= distance;
            yCount += incy;
        }
    }
}

//-------------------- 矩形 --------------------

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
    int maxX, int maxY)
{
    int xS = xStart, yS = yStart, xE = xEnd, yE = yEnd;
    int xSize, ySize, sSize = size;
    int xC, yC, temp;
    int i, j, k;

    int circle_rad = rad;
    int circle_a, circle_b;
    int circle_di;

    int circle_localCentre[4][2] = {{0, 0}, {0, 0}, {0, 0}, {0, 0}};
    //4个角的圆心坐标
    int circle_point[8][2] = {{0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}};
    int circle_compare[4] = {0};

    char **outPutArray;

    if (circle_rad < 0)
        circle_rad = 0;
    if (sSize < 0)
        sSize = 0;

    // 矩阵端点整理
    if (xS > xE && yS <= yE) // 交换x坐标
    {
        temp = xS;
        xS = xE;
        xE = temp;
    }
    else if (yS > yE && xS <= xE) // 交换y坐标
    {
        temp = yS;
        yS = yE;
        yE = temp;
    }
    else if (yS > yE && xS > xE) // 交换x, y坐标
    {
        temp = xS;
        xS = xE;
        xE = temp;
        temp = yS;
        yS = yE;
        yE = temp;
    }
    xSize = xE - xS + 1;
    ySize = yE - yS + 1;

    //完全超出限制范围不绘制
    if (xS > maxX || xE < minX || yS > maxY || yE < minY || xSize < 1 || ySize < 1)
        return;

    //size过大当作完全填充处理
    if (sSize && (sSize * 2 > xSize || sSize * 2 > ySize))
        sSize = 0;

    //缓存数组初始化
    outPutArray = (char **)calloc(ySize, sizeof(char *));
    for (i = 0; i < ySize; i += 1)
        outPutArray[i] = (char *)calloc(xSize, sizeof(char));

    //腰两竖
    if (ySize > circle_rad)
    {
        if (sSize > 0)
        {
            for (i = circle_rad; i < ySize - circle_rad; i += 1)
            {
                for (j = 0; j < sSize; j += 1)
                    outPutArray[i][j] = 1;
                for (j = xSize - sSize; j < xSize; j += 1)
                    outPutArray[i][j] = 1;
            }
        }
        else
        {
            for (i = circle_rad; i < ySize - circle_rad; i += 1)
            {
                for (j = 0; j < xSize; j += 1)
                    outPutArray[i][j] = 1;
            }
        }
    }

    if (sSize > 0)
        temp = sSize;
    else
        temp = circle_rad;

    if (sSize > 0 && xSize > circle_rad)
    {
        //上横
        for (i = 0; i < temp; i += 1)
        {
            for (j = circle_rad; j < xSize - circle_rad; j += 1)
                outPutArray[i][j] = 1;
        }
        //下横
        for (i = ySize - temp; i < ySize; i += 1)
        {
            for (j = circle_rad; j < xSize - circle_rad; j += 1)
                outPutArray[i][j] = 1;
        }
    }

    // if(0)
    if (circle_rad > 0)
    {
        //4个圆心坐标(以[0,0]为原点)
        circle_localCentre[0][0] = circle_rad;
        circle_localCentre[0][1] = circle_rad; //左上角圆心
        circle_localCentre[1][0] = xSize - circle_rad - 1;
        circle_localCentre[1][1] = circle_rad; //右上角圆心
        circle_localCentre[2][0] = circle_rad;
        circle_localCentre[2][1] = ySize - circle_rad - 1; //左下角圆心
        circle_localCentre[3][0] = xSize - circle_rad - 1;
        circle_localCentre[3][1] = ySize - circle_rad - 1; //右下角圆心

        if (sSize == 0)
        {
            temp = 1;
            circle_compare[0] = -1;
            circle_compare[1] = circle_rad;
            circle_compare[2] = ySize - circle_rad - 1;
            circle_compare[3] = 9999;
        }

        for (i = 0; i < temp && circle_rad > 0; i++, circle_rad--)
        {
            circle_a = 0;
            circle_b = circle_rad;
            circle_di = 3 - (circle_rad << 1);
            while (circle_a <= circle_b)
            {
                //1
                circle_point[0][0] = circle_localCentre[1][0] + circle_a;
                circle_point[0][1] = circle_localCentre[1][1] - circle_b;
                //2
                circle_point[1][0] = circle_localCentre[1][0] + circle_b;
                circle_point[1][1] = circle_localCentre[1][1] - circle_a;
                //3
                circle_point[2][0] = circle_localCentre[3][0] + circle_b;
                circle_point[2][1] = circle_localCentre[3][1] + circle_a;
                //4
                circle_point[3][0] = circle_localCentre[3][0] + circle_a;
                circle_point[3][1] = circle_localCentre[3][1] + circle_b;
                //5
                circle_point[4][0] = circle_localCentre[2][0] - circle_a;
                circle_point[4][1] = circle_localCentre[2][1] + circle_b;
                //6
                circle_point[5][0] = circle_localCentre[2][0] - circle_b;
                circle_point[5][1] = circle_localCentre[2][1] + circle_a;
                //7
                circle_point[6][0] = circle_localCentre[0][0] - circle_b;
                circle_point[6][1] = circle_localCentre[0][1] - circle_a;
                //8
                circle_point[7][0] = circle_localCentre[0][0] - circle_a;
                circle_point[7][1] = circle_localCentre[0][1] - circle_b;

                //光头和光屁屁
                if (circle_point[0][1] > circle_point[3][1] || circle_point[0][0] < circle_point[7][0])
                {
                    circle_compare[0] = circle_point[0][1];
                    circle_compare[3] = circle_point[3][1];
                }
                else
                {
                    if (sSize)
                    {
                        //1
                        outPutArray[circle_point[0][1]][circle_point[0][0]] = 1;
                        //4
                        outPutArray[circle_point[3][1]][circle_point[3][0]] = 1;
                        //5
                        outPutArray[circle_point[4][1]][circle_point[4][0]] = 1;
                        //8
                        outPutArray[circle_point[7][1]][circle_point[7][0]] = 1;

                        if (sSize > 1) //补点
                        {
                            //1
                            outPutArray[circle_point[0][1] + 1][circle_point[0][0]] = 1;
                            //4
                            outPutArray[circle_point[3][1] - 1][circle_point[3][0]] = 1;
                            //5
                            outPutArray[circle_point[4][1] - 1][circle_point[4][0]] = 1;
                            //8
                            outPutArray[circle_point[7][1] + 1][circle_point[7][0]] = 1;
                        }
                    }
                    else
                    {
                        if (circle_localCentre[1][1] - circle_b > circle_compare[0]) //1, 8  y
                        {
                            circle_compare[0] = circle_localCentre[1][1] - circle_b;

                            xC = circle_localCentre[0][0] - circle_a; //8 x
                            k = circle_localCentre[1][0] + circle_a;  //1 x
                            yC = circle_compare[0];
                            for (; xC < k; xC += 1)
                                outPutArray[yC][xC] = 1;
                        }
                        if (circle_localCentre[3][1] + circle_b < circle_compare[3]) //4, 5  y
                        {
                            circle_compare[3] = circle_localCentre[3][1] + circle_b;

                            xC = circle_localCentre[2][0] - circle_a; //5 x
                            k = circle_localCentre[3][0] + circle_a;  //4 x
                            yC = circle_compare[3];
                            for (; xC < k; xC += 1)
                                outPutArray[yC][xC] = 1;
                        }
                    }
                }

                //大腹便便
                if (circle_point[1][1] > circle_point[2][1] || circle_point[1][0] < circle_point[6][0])
                {
                    circle_compare[1] = circle_point[1][1];
                    circle_compare[2] = circle_point[2][1];
                }
                else
                {
                    if (sSize)
                    {
                        //2
                        outPutArray[circle_point[1][1]][circle_point[1][0]] = 1;
                        //3
                        outPutArray[circle_point[2][1]][circle_point[2][0]] = 1;
                        //6
                        outPutArray[circle_point[5][1]][circle_point[5][0]] = 1;
                        //7
                        outPutArray[circle_point[6][1]][circle_point[6][0]] = 1;
                        //
                        if (sSize > 1) //补点
                        {
                            //2
                            outPutArray[circle_point[1][1]][circle_point[1][0] - 1] = 1;
                            //3
                            outPutArray[circle_point[2][1]][circle_point[2][0] - 1] = 1;
                            //6
                            outPutArray[circle_point[5][1] + 1][circle_point[5][0]] = 1;
                            //7
                            outPutArray[circle_point[6][1] + 1][circle_point[6][0]] = 1;
                        }
                    }
                    else
                    {
                        if (circle_localCentre[1][1] - circle_a < circle_compare[1]) //2, 7  y
                        {
                            circle_compare[1] = circle_localCentre[1][1] - circle_a;

                            xC = circle_localCentre[0][0] - circle_b; //7 x
                            k = circle_localCentre[1][0] + circle_b;  //2 x
                            yC = circle_compare[1];
                            for (; xC < k; xC += 1)
                                outPutArray[yC][xC] = 1;
                        }
                        if (circle_localCentre[3][1] + circle_a > circle_compare[2]) //3, 6  y
                        {
                            circle_compare[2] = circle_localCentre[3][1] + circle_a;

                            xC = circle_localCentre[2][0] - circle_b; //6 x
                            k = circle_localCentre[3][0] + circle_b;  //3 x
                            yC = circle_compare[2];
                            for (; xC <= k; xC += 1)
                                outPutArray[yC][xC] = 1;
                        }
                    }
                }

                circle_a++;
                //使用Bresenham算法画圆
                if (circle_di < 0)
                    circle_di += 4 * circle_a + 6;
                else
                {
                    circle_di += 10 + 4 * (circle_a - circle_b);
                    circle_b--;
                }
            }
        }
    }
    //输出
    for (i = 0, yC = yS; i < ySize; i++, yC += 1)
    {
        if (yC < minY || yC > maxY)
            continue;
        for (j = 0, xC = xS; j < xSize; j++, xC += 1)
        {
            if (xC < minX || xC > maxX)
                continue;
            else if (outPutArray[i][j])
                print_dot(xC, yC, color);
        }
    }

    for (i = 0; i < ySize; i += 1)
        free(outPutArray[i]);
    free(outPutArray);
}

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
    int xMin, int yMin,
    int xMax, int yMax)
{
    //目标输出矩阵范围
    int xS = xStart, yS = yStart, xE = xEnd, yE = yEnd;
    //输出矩阵横纵计数
    int xC, yC, temp;
    //图片像素纵计数
    int yPic;
    //图片像素横纵浮点计数以及分度值
    float xStep, yStep, xDiv, yDiv;

    uint32_t color;

    if (!pic)
        return;

    // 矩阵端点整理
    if (xS > xE && yS <= yE) // 交换x坐标
    {
        temp = xS;
        xS = xE;
        xE = temp;
    }
    else if (yS > yE && xS <= xE) // 交换y坐标
    {
        temp = yS;
        yS = yE;
        yE = temp;
    }
    else if (yS > yE && xS > xE) // 交换x, y坐标
    {
        temp = xS;
        xS = xE;
        xE = temp;
        temp = yS;
        yS = yE;
        yE = temp;
    }
    //完全超出绘制范围不绘制
    if (xS > xMax || yS > yMax || xE < xMin || yE < yMin)
        return;

    yE += 1;
    xE += 1;

    xDiv = (float)picWidth / (xE - xS);
    yDiv = (float)picHeight / (yE - yS);

    for (yC = yS, yPic = 0, yStep = 0; yC < yE; yC++, yStep += yDiv)
    {
        //范围检查
        if (yC < yMin)
            continue;
        else if (yC > yMax)
            break;

        for (xC = xS, yPic = yStep, xStep = 0; xC < xE; xC++, xStep += xDiv)
        {
            //范围检查
            if (xC < xMin)
                continue;
            else if (xC > xMax)
                break;

            color = pic[yPic * picWidth + (int)xStep];

            if (useReplaceColor && color == replaceColor)
                print_dot(xC, yC, replaceColorBy);
            else
                print_dot(xC, yC, color);
        }
    }
}

//-------------------- 平行四边形 --------------------

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
    int maxX, int maxY)
{
    int xS = xStart, yS = yStart, xE = xEnd, yE = yEnd;
    int xC, yC, temp, sSize = size;

    int xEnd2, yEnd2;
    unsigned short t;
    int xerr = 0, yerr = 0;
    int delta_x, delta_y;
    int distance;
    int incx, incy, xCount, yCount;

    // 矩阵端点整理
    if (xS > xE && yS <= yE) // 交换x坐标
    {
        temp = xS;
        xS = xE;
        xE = temp;
    }
    else if (yS > yE && xS <= xE) // 交换y坐标
    {
        temp = yS;
        yS = yE;
        yE = temp;
    }
    else if (yS > yE && xS > xE) // 交换x, y坐标
    {
        temp = xS;
        xS = xE;
        xE = temp;
        temp = yS;
        yS = yE;
        yE = temp;
    }

    if (sSize < 0)
        sSize = 0;

    xEnd2 = xEnd - width;
    yEnd2 = yEnd;

    delta_x = xEnd2 - xStart; //计算坐标增量
    delta_y = yEnd2 - yStart;
    xCount = xStart;
    yCount = yStart;

    if (delta_x > 0)
        incx = 1; //设置单步方向
    else if (delta_x == 0)
        incx = 0; //垂直线
    else
    {
        incx = -1;
        delta_x = -delta_x;
    }

    if (delta_y > 0)
        incy = 1;
    else if (delta_y == 0)
        incy = 0; //水平线
    else
    {
        incy = -1;
        delta_y = -delta_y;
    }

    if (delta_x > delta_y)
        distance = delta_x; //选取基本增量坐标轴
    else
        distance = delta_y;

    for (t = 0; t <= distance + 1; t += 1) //画线输出
    {
        // xCount, yCount
        if (yCount < minY || yCount > maxY)
            ;
        else
        {
            for (xC = xCount, yC = yCount; xC <= xCount + width; xC += 1)
            {
                if (xC < minX || xC > maxX)
                    continue;
                if (sSize == 0)
                    print_dot(xC, yC, color);
                else if (yC < yStart + sSize || xC < xCount + sSize || yC > yEnd2 - sSize || xC > xCount + width - sSize)
                    print_dot(xC, yC, color);
            }
        }

        xerr += delta_x;
        yerr += delta_y;
        if (xerr > distance)
        {
            xerr -= distance;
            xCount += incx;
        }
        if (yerr > distance)
        {
            yerr -= distance;
            yCount += incy;
        }
    }
    // xCount, yCount
    if (yCount < minY || yCount > maxY)
        ;
    else
    {
        for (xC = xEnd2, yC = yEnd2; xC <= xCount + width; xC += 1)
        {
            if (xC < minX || xC > maxX)
                continue;
            if (sSize == 0)
                print_dot(xC, yC, color);
            else if (yC < yStart + sSize ||
                     xC < xCount + sSize ||
                     yC > yEnd2 - sSize ||
                     xC > xCount + width - sSize)
                print_dot(xC, yC, color);
        }
    }
}

//-------------------- 写字符串 --------------------

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
    Ttf_Map map)
{
    //总字节
    int byteTotal = map.height * map.lineByte;
    //总字节计数
    int byteTotalCount;
    //一行字节计数
    int lineByteCount;
    //画点用
    uint8_t bit;
    int i;
    //横纵向点计数
    int x = 0, y = 0;
    int xp, yp;

    //纵向起始跳过行数
    for (y = 0, yp = yStart; y < map.bitTop; y += 1, yp += 1)
    {
        for (x = 0, xp = xStart; x < map.width; x++, xp += 1)
            print_dot(xp, yp, bColor);
    }
    //正文
    for (byteTotalCount = 0, yp = y + yStart; byteTotalCount < byteTotal && y < map.height; y += 1, yp += 1)
    {
        //横向起始跳过点数
        for (x = 0, xp = xStart; x < map.bitLeft; x += 1, xp += 1)
            print_dot(xp, yp, bColor);
        //正文
        for (lineByteCount = 0; lineByteCount < map.lineByte; lineByteCount++, byteTotalCount += 1)
        {
            //画一行的点(注意 map.lineByte 有盈余,需判断 x < map.width 保证不多画)
            for (i = 0, bit = map.bitMap[byteTotalCount]; i < 8 && x < map.width; i++, x += 1)
            {
                if (bit & 0x80)
                    print_dot(x + xStart, y + yStart, fColor);
                else
                    print_dot(x + xStart, y + yStart, bColor);
                bit <<= 1;
            }
        }
        //横向补足点数
        for (x = 0, xp = xStart; x < map.bitLeft; x += 1, xp += 1)
            print_dot(xp, yp, bColor);
    }
    //纵向补足行数
    for (yp = y + yStart; y < map.height; y += 1, yp += 1)
    {
        for (x = 0, xp = xStart; x < map.width; x++, xp += 1)
            print_dot(xp, yp, bColor);
    }
}

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
    int type, int space)
{
    int ret;
    Ttf_Map map;

    while (*str)
    {
        ret = ttf_getMapByUtf8(ViewTTF, str, type, &map);
        if (ret < 0)
            str -= ret;
        else
        {
            view_printBitMap(fColor, bColor, xStart, yStart, 0, 0, VIEW_X_END, VIEW_Y_END, map);
            xStart += map.width + space;
            str += ret;
        }
    }
}

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
    int type, int space)
{
    int ret;
    Ttf_Map map;

    if (xStart > xScreenEnd ||
        yStart > yScreenEnd ||
        xStart + strWidth < xScreenStart ||
        yStart + strHight < yScreenStart)
        return;

    while (*str)
    {
        ret = ttf_getMapByUtf8(ViewTTF, str, type, &map);
        if (ret < 0)
            str -= ret;
        else
        {
            view_printBitMap(
                fColor, bColor,
                xStart, yStart,
                xScreenStart, yScreenStart,
                xScreenEnd, yScreenEnd,
                map);
            xStart += map.width + space;
            str += ret;
        }
    }
}

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
    int *retLine)
{
    int ret;
    int xC = xStart, yC = yStart;
    int xE = xStart + strWidth, yE = yStart + strHight;
    Ttf_Map map;

    int typeHeight = type / 10, typeWidth = type / 10;
    char *strOld = str, *strStart = str;
    int *rwpl = retWordPerLine, *rl = retLine;

    if (xStart > xScreenEnd || yStart > yScreenEnd ||
        xE < xScreenStart || yE < yScreenStart)
        return 0;

    if (rl)
        *rl = 1;

    while (*str)
    {
        //tab键, 回车键处理
        while (*str == '\t' || *str == '\n' || (*str == '\r' && *(str + 1) == '\n'))
        {
            if (*str == '\t')
            {
                xC += (typeWidth + xSpace) * 2;
                str += 1;
                //返回前记得先保存行占用字节数
                if (*str == 0 && rwpl)
                    *rwpl = str - strOld;
            }
            else if (*str == '\n' || *str == '\r')
            {
                //换行
                yC += (typeHeight + ySpace);
                xC = xStart;
                if (*str == '\r')
                    str += 2;
                else
                    str += 1;

                if (rl && yC < yE)
                    *rl += 1;
                //行占用字节数记录
                if (rwpl)
                {
                    //计算偏移量得到一行字节数
                    *rwpl = str - strOld;
                    rwpl += 1;
                    //更新偏移指针
                    strOld = str;
                }
            }

            if (*str == 0 || yC > yE)
                return (str - strStart);
        }
        ret = ttf_getMapByUtf8(ViewTTF, str, type, &map);
        if (ret < 0)
            str -= ret;
        else
        {
            if (xC + map.width > xE)
            {
                // yC += (param.height + ySpace);  //换行
                yC += (typeHeight + ySpace); //换行
                xC = xStart;
                if (rl && yC < yE)
                    *rl += 1;
                if (rwpl) //行占用字节数记录
                {
                    *rwpl = str - strOld; //计算偏移量得到一行字节数
                    rwpl += 1;
                    strOld = str; //更新偏移指针
                }
                if (yC > yE)
                    break;
            }
            view_printBitMap(
                fColor, bColor,
                xC, yC,
                xScreenStart, yScreenStart,
                xScreenEnd, yScreenEnd,
                map);
            xC += map.width + xSpace;
            str += ret;
        }
    }

    //行占用字节数记录
    if (rwpl)
        *rwpl = str - strOld;

    return (str - strStart);
}

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
    int type, int space,
    int xErr)
{
    int xMov = xErr, typeHeight = type / 10;
    int ret;
    int strCount = 0, retVal = xErr;
    Ttf_Map map;

    if (xStart > xScreenEnd ||
        yStart > yScreenEnd ||
        xStart + strWidth < xScreenStart ||
        yStart + strHight < yScreenStart)
        return 0;

    while (xMov < strWidth)
    {
        ret = ttf_getMapByUtf8(ViewTTF, &str[strCount], type, &map);
        if (ret < 0)
            str -= ret;
        else
        {
            if (xMov + map.width > 0)
                view_printBitMap(
                    fColor, bColor,
                    xStart + xMov, yStart,
                    xScreenStart, yScreenStart,
                    xScreenEnd, yScreenEnd,
                    map);
            xMov += map.width + space;
            strCount += ret;
        }
        //字符串内容循环输出
        if (str[strCount] == '\0')
        {
            //自动填充空格分开字符串
            xMov += (typeHeight + space);
            //字符串遍历完还未抵达开始绘制的位置
            if (xMov <= 0)
                //返回此次绘制的偏差值, 以便后续无缝衔接
                retVal = xMov;
            strCount = 0;
        }
    }
    return retVal;
}

//--------------------  链表操作 --------------------

void view_add(View_Struct *parentView, View_Struct *view, bool front)
{
    View_Struct *vsTemp;

    if (!parentView || !view)
        return;

    //互斥保护
    // while(parentView->callBackForbid)
    //     view_delayms(5);

    if (parentView->view && parentView->lastView)
    {
        if (front)
        {
            parentView->view->last = view;
            view->next = parentView->view;
            parentView->view = view;
            view->last = NULL;
        }
        else
        {
            parentView->lastView->next = view;
            view->last = parentView->lastView;
            parentView->lastView = view;
            view->next = NULL;
        }
    }
    else
    {
        parentView->view = view;
        parentView->lastView = view;
    }
    //整理下序号
    if (parentView->view)
    {
        vsTemp = parentView->view;
        vsTemp->number = 1;
        while (vsTemp->next)
        {
            vsTemp->next->number = vsTemp->number + 1;
            vsTemp = vsTemp->next;
        }
        parentView->total = parentView->lastView->number;
    }
    else
    {
        parentView->lastView = NULL;
        parentView->total = 0;
    }
    //参数继承
    view->parent = parentView;
}

/*
 *  包 view 安插在节点 nodeView 的前/后
 *  参数:
 *      front: 是否添加到前面
 */
void view_insert(View_Struct *nodeView, View_Struct *view, bool front)
{
    View_Struct *vsTemp, *vsParent;

    if (!nodeView || !view || !nodeView->parent)
        return;

    vsParent = nodeView->parent;

    //互斥保护
    // while(vsParent->callBackForbid)
    //     view_delayms(5);

    if (front)
    {
        if (nodeView->last)
        {
            nodeView->last->next = view;
            view->last = nodeView->last;
        }
        else
        {
            vsParent->view = view;
            view->last = NULL;
        }
        nodeView->last = view;
        view->next = nodeView;
    }
    else
    {
        if (nodeView->next)
        {
            nodeView->next->last = view;
            view->next = nodeView->next;
        }
        else
        {
            vsParent->lastView = view;
            view->next = NULL;
        }
        nodeView->next = view;
        view->last = nodeView;
    }
    //整理下序号
    if (vsParent->view)
    {
        vsTemp = vsParent->view;
        vsTemp->number = 1;
        while (vsTemp->next)
        {
            vsTemp->next->number = vsTemp->number + 1;
            vsTemp = vsTemp->next;
        }
        vsParent->total = vsParent->lastView->number;
    }
    else
    {
        vsParent->lastView = NULL;
        vsParent->total = 0;
    }
    //参数继承
    view->parent = vsParent;
}

/*
 *  把 view 从其所在链表移除,但并不释放内存
 */
void view_remove(View_Struct *view)
{
    View_Struct *vsTemp, *vsParent;

    if (!view || !view->parent)
        return;

    vsParent = view->parent;

    //互斥保护
    // while(vsParent->callBackForbid)
    //     view_delayms(5);

    if (vsParent->jumpView == view)
    {
        vsParent->jumpView = NULL;
        vsParent->jumpViewOn = false;
        vsParent->jumpViewKeepOn = false;
    }
    else
    {
        if (view->last)
            view->last->next = view->next;
        else
            vsParent->view = view->next;
        if (view->next)
            view->next->last = view->last;
        else
            vsParent->lastView = view->last;
        //整理下序号
        if (vsParent->view)
        {
            vsTemp = vsParent->view;
            vsTemp->number = 1;
            while (vsTemp->next)
            {
                vsTemp->next->number = vsTemp->number + 1;
                vsTemp = vsTemp->next;
            }
            vsParent->total = vsParent->lastView->number;
        }
        else
        {
            vsParent->lastView = NULL;
            vsParent->total = 0;
        }
    }
    //削干净
    view->parent = NULL;
    view->jumpView = NULL;
    view->next = view->last = NULL;
}

/*
 *  把 view 从其所在链表移除,并添加到 viewTrash 垃圾桶链表中
 */
void view_delete(View_Struct *view)
{
    view_remove(view);
    //扔到垃圾桶
    while (viewTrash.callBackForbid)
        view_delayms(5);
    view_add(&viewTrash, view, false);
}

bool view_isChild(View_Struct *parent, View_Struct *child)
{
    if (!parent || !child)
        return false;
    if (parent == child->parent)
        return true;
    return view_isChild(parent, child->parent);
}

#define FIND_LINK(o, a, n)      \
    if (a->number > n)          \
    {                           \
        while (a)               \
        {                       \
            if (a->number == n) \
                return a;       \
            a = a->last;        \
        }                       \
    }                           \
    else if (a->number < n)     \
    {                           \
        while (a)               \
        {                       \
            if (a->number == n) \
                return a;       \
            a = a->next;        \
        }                       \
    }                           \
    return o

//定位到 view 链表中的一员
View_Struct *view_num(View_Struct *view, int n)
{
    View_Struct *vsTemp = view;
    int tN;
    if (n < 0)
    {
        if (n < VRNT_NEXT)
            tN = view->number + (VRNT_NEXT - n);
        else if (n == VRNT_NEXT)
            tN = view->number + 1;
        else if (n < VRNT_LAST)
            tN = view->number - (VRNT_LAST - n);
        else
            tN = view->number - 1;
        FIND_LINK(view, vsTemp, tN);
    }
    else
    {
        FIND_LINK(view, vsTemp, n);
    }
}

//--------------------  viewTool focus --------------------

View_Focus *view_focusInit(View_Struct *topView, View_Struct *cView, uint32_t color)
{
    View_Focus *focus;
    if (topView)
    {
        focus = (View_Focus *)calloc(1, sizeof(View_Focus));
        focus->topView = topView;
        focus->color = color;
        focus->lineSize = 3;
        view_focusNote(focus, cView);
        return focus;
    }
    return NULL;
}

void view_focusRecover(View_Focus *focus)
{
    if (!focus)
        return;
    if (focus->view)
    {
        focus->view->focus = NULL;
        focus->view = NULL;
    }
}

void view_focusNote(View_Focus *focus, View_Struct *view)
{
    if (!focus)
        return;
    view_focusRecover(focus);
    if (view)
    {
        focus->view = view;
        focus->view->focus = focus;
    }
}

void view_focusJump(View_Focus *focus, View_Struct *jumpView)
{
    if (focus && jumpView &&
        !jumpView->disable &&
        !jumpView->invisible &&
        !jumpView->hide)
        view_focusNote(focus, jumpView);
}

View_Struct *view_focusSearch(View_Struct *view)
{
    View_Struct *vsTemp = NULL, *vsTemp2 = NULL;

    if (!view || view->disable || view->invisible)
        return NULL;

    vsTemp = view;
    //优先跳转 view
    while (vsTemp->jumpView &&
           vsTemp->jumpViewOn &&
           !vsTemp->jumpViewKeepOn &&
           !vsTemp->jumpView->disable &&
           !vsTemp->jumpView->invisible)
        vsTemp = vsTemp->jumpView;
    //本地
    if (vsTemp->focusStop)
        return vsTemp;
    //同时绘制的跳转 view
    if (vsTemp->jumpView &&
        vsTemp->jumpViewKeepOn &&
        !vsTemp->jumpView->disable &&
        !vsTemp->jumpView->invisible)
    {
        vsTemp->jumpViewOn = true;
        return view_focusSearch(vsTemp->jumpView);
    }
    //子 view
    if (vsTemp->view)
    {
        vsTemp = vsTemp->view;
        while (vsTemp)
        {
            if ((vsTemp2 = view_focusSearch(vsTemp)))
                return vsTemp2;
            vsTemp = vsTemp->next;
        }
    }
    return NULL;
}

ViewCallBack view_focusEvent(View_Focus *focus, ViewFocus_Event event)
{
    View_Struct *vsTemp = NULL, *vsTemp2 = NULL;

    if (!focus || !focus->topView)
        return NULL;
    if (!focus->view)
    {
        focus->view = focus->topView->view;
        goto FOCUS_ENENT_ENTER;
    }

    //当前 view 接收任何事件
    if (focus->view->callBack && focus->view->enAnyInput)
        return focus->view->callBack;

    if (event == VFE_RETURN)
    {
        // printf("view_focusEvent: view/%s VFE_RETURN\r\n", focus->view->name);
        vsTemp = focus->view;
        while (vsTemp != focus->topView)
        {
            //jumpView
            if (vsTemp->backView && view_isChild(vsTemp->parent, vsTemp->backView))
            {
                vsTemp->parent->jumpViewOn = false;
                vsTemp2 = vsTemp->backView;
            }
            else
                vsTemp2 = vsTemp->parent;
            //顺手规整以下数据
            vsTemp2->disable = false;
            vsTemp2->invisible = false;
            vsTemp2->jumpViewOn = false;

            if (!vsTemp2->hide && vsTemp2->focusStop)
            {
                view_focusNote(focus, vsTemp2);
                return NULL;
            }

            vsTemp = vsTemp2;
        }

        return NULL;
    }
    else if (event == VFE_ENTER)
    {
        // printf("view_focusEvent: view/%s VFE_ENTER\r\n", focus->view->name);
        if (focus->view->callBack)
        {
            if (!focus->view->lock)
                return focus->view->callBack;
            return NULL;
        }
        goto FOCUS_ENENT_ENTER;
    }
    else if (event == VFE_NEXT)
    {
        // printf("view_focusEvent: view/%s VFE_NEXT\r\n", focus->view->name);
        if (focus->view->next)
            vsTemp = focus->view->next;
        else
        {
            vsTemp = focus->view;
            while (vsTemp->last)
                vsTemp = vsTemp->last;
        }
        while (vsTemp && vsTemp != focus->view)
        {
            if (vsTemp->disable || vsTemp->invisible ||
                vsTemp->hide || (vsTemp->jumpViewOn && !vsTemp->jumpViewKeepOn))
                ;
            else if (vsTemp->focusStop)
            {
                view_focusNote(focus, vsTemp);
                return NULL;
            }

            if (!vsTemp->next)
            {
                while (vsTemp->last)
                    vsTemp = vsTemp->last;
            }
            else
                vsTemp = vsTemp->next;
        }
        if (!focus->view->focusStop)
            return view_focusEvent(focus, VFE_ENTER);
        return NULL;
    }
    else if (event == VFE_LAST)
    {
        // printf("view_focusEvent: view/%s VFE_LAST\r\n", focus->view->name);

        if (focus->view->last)
            vsTemp = focus->view->last;
        else
        {
            vsTemp = focus->view;
            while (vsTemp->next)
                vsTemp = vsTemp->next;
        }
        while (vsTemp && vsTemp != focus->view)
        {
            if (vsTemp->disable || vsTemp->invisible ||
                vsTemp->hide || (vsTemp->jumpViewOn && !vsTemp->jumpViewKeepOn))
                ;
            else if (vsTemp->focusStop)
            {
                view_focusNote(focus, vsTemp);
                return NULL;
            }

            if (!vsTemp->last)
            {
                while (vsTemp->next)
                    vsTemp = vsTemp->next;
            }
            else
                vsTemp = vsTemp->last;
        }
        if (!focus->view->focusStop)
            return view_focusEvent(focus, VFE_ENTER);
        return NULL;
    }
    else if (event == VFE_HOME)
    {
        // printf("view_focusEvent: view/%s VFE_HOME\r\n", focus->view->name);
        while (focus->view && focus->view->parent != focus->topView)
            view_focusEvent(focus, VFE_RETURN);
        view_focusNote(focus, focus->topView->view);
        return NULL;
    }

    return NULL;

FOCUS_ENENT_ENTER:

    vsTemp = focus->view;

    if (vsTemp->jumpView &&
        (vsTemp->jumpViewKeepOn || vsTemp->jumpViewOn) &&
        !vsTemp->jumpView->disable && !vsTemp->jumpView->invisible)
    {
        vsTemp->jumpViewOn = true;
        if ((vsTemp2 = view_focusSearch(vsTemp->jumpView)))
        {
            view_focusNote(focus, vsTemp2);
            return NULL;
        }
    }

    if (vsTemp->view)
    {
        vsTemp = vsTemp->view;
        while (vsTemp)
        {
            if ((vsTemp2 = view_focusSearch(vsTemp)))
            {
                view_focusNote(focus, vsTemp2);
                return NULL;
            }
            vsTemp = vsTemp->next;
        }
    }

    return NULL;
}

//--------------------  viewTool draw --------------------

void _viewTool_viewLocal(uint32_t tickOfDraw, View_Struct *view, int width, int height, int xy[2][2])
{
    View_Struct *rView = view;
    int intTemp = 0;

    //相对布局: 宽计算
    if (view->width < 0)
    {
        if (view->width <= VWHT_FULL)
        {
            if ((intTemp = view->width % VWHT_FULL) == 0)
                view->absWidth = VIEW_X_SIZE / (view->width / VWHT_FULL);
            else
                view->absWidth = VIEW_X_SIZE * (-intTemp) / (view->width / VWHT_FULL);
        }
        else if (view->width <= VWHT_MATCH)
        {
            if ((intTemp = view->width % VWHT_MATCH) == 0)
                view->absWidth = width / (view->width / VWHT_MATCH);
            else
                view->absWidth = width * (-intTemp) / (view->width / VWHT_MATCH);
        }
        else
            view->absWidth = -view->width;
    }
    else
        view->absWidth = view->width;

    //相对布局: 高计算
    if (view->height < 0)
    {
        if (view->height <= VWHT_FULL)
        {
            if ((intTemp = view->height % VWHT_FULL) == 0)
                view->absHeight = VIEW_Y_SIZE / (view->height / VWHT_FULL);
            else
                view->absHeight = VIEW_Y_SIZE * (-intTemp) / (view->height / VWHT_FULL);
        }
        else if (view->height <= VWHT_MATCH)
        {
            if ((intTemp = view->height % VWHT_MATCH) == 0)
                view->absHeight = height / (view->height / VWHT_MATCH);
            else
                view->absHeight = height * (-intTemp) / (view->height / VWHT_MATCH);
        }
        else
            view->absHeight = -view->height;
    }
    else
        view->absHeight = view->height;

    //相对布局: 找到相对对象
    if (view->rNumber)
    {
        rView = view_num(view, view->rNumber);
        if (rView != view &&
            rView->tickOfDraw != tickOfDraw)
            _viewTool_viewLocal(tickOfDraw, rView, width, height, xy);
    }

    //center时必定相对于父控件
    if (view->centerX)
    {
        view->absXY[0][0] = xy[0][0] + (width - view->absWidth) / 2 - 1;
        //在子控件小于等于父控件尺寸时,尽量保留自控比例
        if (width < view->absWidth)
            view->absXY[1][0] = xy[0][0] + (width + view->absWidth) / 2 - 1;
        else
            view->absXY[1][0] = view->absXY[0][0] + view->absWidth - 1;
    }
    //不等于自己,表示有相对对象
    else if (rView != view)
    {
        if ((view->rType & 0xF0) == VRT_RIGHT)
        {
            view->absXY[0][0] = rView->absXY[1][0] + view->rLeftRightErr + 1;
            view->absXY[1][0] = rView->absXY[1][0] + view->rLeftRightErr + view->absWidth;
        }
        else if ((view->rType & 0xF0) == VRT_LEFT)
        {
            view->absXY[0][0] = rView->absXY[0][0] - view->rLeftRightErr - view->absWidth;
            view->absXY[1][0] = rView->absXY[0][0] - view->rLeftRightErr - 1;
        }
        else
        {
            if (view->rLeftRightErr)
                view->absXY[0][0] = rView->absXY[0][0] + view->rLeftRightErr;
            else
                view->absXY[0][0] = rView->absXY[0][0] + rView->absWidth / 2 - view->absWidth / 2;
            view->absXY[1][0] = view->absXY[0][0] + view->absWidth - 1;
        }
    }
    //后面都是相对父控件
    else if ((view->rType & 0xF0) == VRT_RIGHT)
    {
        view->absXY[0][0] = xy[1][0] - view->rLeftRightErr - view->absWidth + 1;
        view->absXY[1][0] = xy[1][0] - view->rLeftRightErr;
    }
    else if ((view->rType & 0xF0) == VRT_LEFT)
    {
        view->absXY[0][0] = xy[0][0] + view->rLeftRightErr;
        view->absXY[1][0] = xy[0][0] + view->rLeftRightErr + view->absWidth - 1;
    }
    else
    {
        view->absXY[0][0] = xy[0][0] + view->rLeftRightErr;
        view->absXY[1][0] = view->absXY[0][0] + view->absWidth - 1;
    }

    //center时必定相对于父控件
    if (view->centerY)
    {
        view->absXY[0][1] = xy[0][1] + (height - view->absHeight) / 2 - 1;
        //在子控件小于等于父控件尺寸时,尽量保留自控比例
        if (height < view->absHeight)
            view->absXY[1][1] = xy[0][1] + (height + view->absHeight) / 2 - 1;
        else
            view->absXY[1][1] = view->absXY[0][1] + view->absHeight - 1;
    }
    //不等于自己,表示有相对对象
    else if (rView != view)
    {
        if ((view->rType & 0x0F) == VRT_BOTTOM)
        {
            view->absXY[0][1] = rView->absXY[1][1] + view->rTopBottomErr + 1;
            view->absXY[1][1] = rView->absXY[1][1] + view->rTopBottomErr + view->absHeight;
        }
        else if ((view->rType & 0x0F) == VRT_TOP)
        {
            view->absXY[0][1] = rView->absXY[0][1] - view->rTopBottomErr - view->absHeight;
            view->absXY[1][1] = rView->absXY[0][1] - view->rTopBottomErr - 1;
        }
        else
        {
            if (view->rTopBottomErr)
                view->absXY[0][1] = rView->absXY[0][1] + view->rTopBottomErr;
            else
                view->absXY[0][1] = rView->absXY[0][1] + rView->absHeight / 2 - view->absHeight / 2;
            view->absXY[1][1] = view->absXY[0][1] + view->absHeight - 1;
        }
    }
    //后面都是相对父控件
    else if ((view->rType & 0x0F) == VRT_BOTTOM)
    {
        view->absXY[0][1] = xy[1][1] - view->rTopBottomErr - view->absHeight + 1;
        view->absXY[1][1] = xy[1][1] - view->rTopBottomErr;
    }
    else if ((view->rType & 0x0F) == VRT_TOP)
    {
        view->absXY[0][1] = xy[0][1] + view->rTopBottomErr;
        view->absXY[1][1] = xy[0][1] + view->rTopBottomErr + view->absHeight - 1;
    }
    else
    {
        view->absXY[0][1] = xy[0][1] + view->rTopBottomErr;
        view->absXY[1][1] = view->absXY[0][1] + view->absHeight - 1;
    }

    view->tickOfDraw = tickOfDraw;
}

char *_viewTool_textPrint(ViewValue_Format *value, ViewPrint_Struct *vps)
{
    int i, count, ret;
    char strDemoIntArray[] = "%01d,";
    char strDemoDouble[] = "%.6lf";
    char strDemoDoubleArray[] = "%.6lf,";
    char strDemoStrArray[] = "%s,";

    //数组数据有变动,重新初始化结构体
    if (value->type != vps->type ||
        value->vSize != vps->vSize ||
        !vps->textOutput)
    {
        if (value->vSize == 0)
            return NULL;

        vps->type = value->type;
        vps->vSize = value->vSize;
        if (vps->textOutput)
            free(vps->textOutput);
        vps->textOutput = NULL;

        switch (value->type)
        {
        case VT_INT_ARRAY:
            vps->textOutput = (char *)calloc(value->vSize / sizeof(int) * 32, sizeof(char));
            break;
        case VT_DOUBLE_ARRAY:
            vps->textOutput = (char *)calloc(value->vSize / sizeof(double) * 32, sizeof(char));
            break;
        case VT_BOOL_ARRAY:
            vps->textOutput = (char *)calloc(value->vSize / sizeof(bool) * 8, sizeof(char));
            break;
        case VT_STRING_ARRAY:
            for (i = 0, count = 0; i < value->vSize / sizeof(char *); i += 1)
                count += strlen(value->value.StringArray[i]) + 1;
            vps->textOutputLen = count;
            vps->textOutput = (char *)calloc(count, sizeof(char));
            break;
        default:
            vps->textOutput = (char *)calloc(64, sizeof(char));
            break;
        }
    }

    switch (value->type)
    {
    case VT_CHAR:
        vps->textOutput[0] = value->value.Char;
        break;
    case VT_STRING:
    case VT_POINT:
        return value->value.String;
    case VT_INT:
        sprintf(vps->textOutput, "%d", value->value.Int);
        break;
    case VT_DOUBLE:
        if (value->zero)
            strDemoDouble[2] = (value->zero % 10) + '0'; //指定保留小数位数
        sprintf(vps->textOutput, strDemoDouble, value->value.Double);
        break;
    case VT_BOOL:
        sprintf(vps->textOutput, "%s", value->value.Bool ? "true" : "false");
        break;
    case VT_INT_ARRAY:
        if (value->sep && value->sep != '%')
            strDemoIntArray[4] = value->sep; //指定分隔符
        if (value->zero)
            strDemoIntArray[2] = (value->zero % 10) + '0'; //指定高位补0数量
        for (i = 0, count = 0; i < value->vSize / sizeof(int); i += 1)
        {
            ret = sprintf(&vps->textOutput[count], strDemoIntArray, value->value.IntArray[i]);
            count += ret;
        }
        //当数组只有一个元素时,补全最后的分隔符,否则丢弃
        vps->textOutput[count - 1] =
            (i == 1 && value->sep) ? value->sep : 0;
        break;
    case VT_DOUBLE_ARRAY:
        if (value->sep && value->sep != '%')
            strDemoDoubleArray[5] = value->sep; //指定分隔符
        if (value->zero)
            strDemoDoubleArray[2] = (value->zero % 10) + '0'; //指定保留小数位数
        for (i = 0, count = 0; i < value->vSize / sizeof(double); i += 1)
        {
            ret = sprintf(&vps->textOutput[count], strDemoDoubleArray, value->value.DoubleArray[i]);
            count += ret;
        }
        //当数组只有一个元素时,补全最后的分隔符,否则丢弃
        vps->textOutput[count - 1] =
            (i == 1 && value->sep) ? value->sep : 0;
        break;
    case VT_BOOL_ARRAY:
        if (value->sep && value->sep != '%')
            strDemoStrArray[2] = value->sep; //指定分隔符
        for (i = 0, count = 0; i < value->vSize / sizeof(bool); i += 1)
        {
            ret = sprintf(&vps->textOutput[count], strDemoStrArray, value->value.BoolArray[i] ? "true" : "false");
            count += ret;
        }
        //当数组只有一个元素时,补全最后的分隔符,否则丢弃
        vps->textOutput[count - 1] =
            (i == 1 && value->sep) ? value->sep : 0;
        break;
    case VT_STRING_ARRAY:
    case VT_POINT_ARRAY:
        if (value->sep && value->sep != '%')
            strDemoStrArray[2] = value->sep; //指定分隔符
        for (i = 0, count = 0; i < value->vSize / sizeof(char *); i += 1)
            count += strlen(value->value.StringArray[i]) + 1;
        if (count > vps->textOutputLen)
        {
            free(vps->textOutput);
            vps->textOutput = (char *)calloc(count, sizeof(char));
        }
        for (i = 0, count = 0; i < value->vSize / sizeof(char *); i += 1)
        {
            ret = sprintf(&vps->textOutput[count], strDemoStrArray, value->value.StringArray[i]);
            count += ret;
        }
        //当数组只有一个元素时,补全最后的分隔符,否则丢弃
        vps->textOutput[count - 1] =
            (i == 1 && value->sep) ? value->sep : 0;
        break;
    default:
        return NULL;
    }

    return vps->textOutput;
}

//控件锁定(失能)时的颜色处理
static uint32_t _viewTool_lockColor(uint32_t color)
{
    uint8_t *p = (uint8_t *)&color;
    p[0] >>= 1;
    p[1] >>= 1;
    p[2] >>= 1;
    return *((uint32_t *)p);
}

static void _view_draw(View_Struct *view, int xyLimit[2][2])
{
    int widthTemp = 0, heightTemp = 0;
    uint32_t colorTemp = 0, colorTemp2 = 0;

    int absXYTemp[2][2] = {{0, 0}, {0, 0}};
    int intTemp = 0, intTemp2 = 0, intTemp3 = 0, intTemp4 = 0;

    View_Focus *focus = view->focus;

    //不具备焦点条件
    if (focus && !view->focusStop)
        focus = NULL;
    //focus 附加绘制
    if (focus && view->focusCallBackFront) //使用自定义的光标方法
        view->focusCallBackFront(view, focus, xyLimit);
    //自定义绘制方法
    if (view->drawStart)
        view->drawStart(view, xyLimit);
    //背景色
    if (view->backGroundColor)
    {
        view_rectangle(
            view->backGroundColor,
            view->absXY[0][0], view->absXY[0][1],
            view->absXY[1][0], view->absXY[1][1],
            0, view->backGroundRad,
            xyLimit[0][0], xyLimit[0][1], xyLimit[1][0], xyLimit[1][1]);
    }
    //形状绘制
    view->shapeAbsXY[0][0] = view->absXY[0][0] + view->shapeEdgeLeft;
    view->shapeAbsXY[1][0] = view->absXY[1][0] - view->shapeEdgeRight;
    view->shapeAbsXY[0][1] = view->absXY[0][1] + view->shapeEdgeTop;
    view->shapeAbsXY[1][1] = view->absXY[1][1] - view->shapeEdgeBottom;
    widthTemp = view->shapeAbsXY[1][0] - view->shapeAbsXY[0][0] + 1;
    heightTemp = view->shapeAbsXY[1][1] - view->shapeAbsXY[0][1] + 1;
    if (view->shapeColorPrint)
    {
        if (view->lock)
            colorTemp = _viewTool_lockColor(view->shapeColorPrint);
        else
            colorTemp = view->shapeColorPrint;
    }
    if (view->shapeColorBackground)
    {
        if (view->lock)
            colorTemp2 = _viewTool_lockColor(view->shapeColorBackground);
        else
            colorTemp2 = view->shapeColorBackground;
    }
    switch (view->shapeType)
    {
    case VST_RECT:
        if (view->shape.rect.lineSize > 0 && view->shapeColorBackground) //背景色
            view_rectangle(colorTemp2,
                           view->shapeAbsXY[0][0], view->shapeAbsXY[0][1],
                           view->shapeAbsXY[1][0], view->shapeAbsXY[1][1],
                           0, view->shape.rect.rad,
                           xyLimit[0][0], xyLimit[0][1], xyLimit[1][0], xyLimit[1][1]);
        if (view->shapeColorPrint)
            view_rectangle(colorTemp,
                           view->shapeAbsXY[0][0], view->shapeAbsXY[0][1],
                           view->shapeAbsXY[1][0], view->shapeAbsXY[1][1],
                           view->shape.rect.lineSize, view->shape.rect.rad,
                           xyLimit[0][0], xyLimit[0][1], xyLimit[1][0], xyLimit[1][1]);
        break;
    case VST_PARA:
        if (view->shapeColorPrint)
        {
            if (view->shape.para.mode == 1) //向外扩充(控件宽不变)
            {
                intTemp = view->shape.para.err / 2;
                intTemp2 = -view->shape.para.err / 2;
                intTemp3 = 0;
            }
            else if (view->shape.para.mode == 2) //向外扩充(控件宽变大)
            {
                intTemp = 0;
                intTemp2 = 0;
                intTemp3 = view->shape.para.err;
            }
            else //向内缩进(控件宽减少)
            {
                intTemp = view->shape.para.err;
                intTemp2 = -view->shape.para.err;
                intTemp3 = -view->shape.para.err;
            }
            if (view->shape.para.lineSize > 0 && view->shapeColorBackground) //背景色
                view_parallelogram(
                    colorTemp2,
                    view->shapeAbsXY[0][0] + view->shape.para.lineSize + intTemp, view->shapeAbsXY[0][1] + view->shape.para.lineSize,
                    view->shapeAbsXY[1][0] - view->shape.para.lineSize + intTemp2, view->shapeAbsXY[1][1] - view->shape.para.lineSize,
                    0, widthTemp - view->shape.para.lineSize * 2 + intTemp3,
                    xyLimit[0][0], xyLimit[0][1], xyLimit[1][0], xyLimit[1][1]);
            view_parallelogram(
                colorTemp,
                view->shapeAbsXY[0][0] + intTemp, view->shapeAbsXY[0][1],
                view->shapeAbsXY[1][0] + intTemp2, view->shapeAbsXY[1][1],
                view->shape.para.lineSize, widthTemp + intTemp3,
                xyLimit[0][0], xyLimit[0][1], xyLimit[1][0], xyLimit[1][1]);
        }
        break;
    case VST_LINE:
        if (view->shapeColorPrint)
        {
            if (view->shape.line.mode == 0)
            {
                absXYTemp[0][0] = view->shapeAbsXY[0][0];
                absXYTemp[0][1] = view->shapeAbsXY[0][1];
                absXYTemp[1][0] = view->shapeAbsXY[1][0];
                absXYTemp[1][1] = view->shapeAbsXY[1][1];
            }
            else if (view->shape.line.mode == 1)
            {
                absXYTemp[0][0] = view->shapeAbsXY[0][0];
                absXYTemp[0][1] = view->shapeAbsXY[1][1];
                absXYTemp[1][0] = view->shapeAbsXY[1][0];
                absXYTemp[1][1] = view->shapeAbsXY[0][1];
            }
            else if (view->shape.line.mode == 2)
            {
                absXYTemp[0][0] = view->shapeAbsXY[0][0];
                absXYTemp[0][1] = view->shapeAbsXY[0][1];
                absXYTemp[1][0] = view->shapeAbsXY[1][0];
                absXYTemp[1][1] = view->shapeAbsXY[0][1];
            }
            else if (view->shape.line.mode == 3)
            {
                absXYTemp[0][0] = view->shapeAbsXY[1][0];
                absXYTemp[0][1] = view->shapeAbsXY[0][1];
                absXYTemp[1][0] = view->shapeAbsXY[1][0];
                absXYTemp[1][1] = view->shapeAbsXY[1][1];
            }
            else if (view->shape.line.mode == 4)
            {
                absXYTemp[0][0] = view->shapeAbsXY[0][0];
                absXYTemp[0][1] = view->shapeAbsXY[1][1];
                absXYTemp[1][0] = view->shapeAbsXY[1][0];
                absXYTemp[1][1] = view->shapeAbsXY[1][1];
            }
            else if (view->shape.line.mode == 5)
            {
                absXYTemp[0][0] = view->shapeAbsXY[0][0];
                absXYTemp[0][1] = view->shapeAbsXY[0][1];
                absXYTemp[1][0] = view->shapeAbsXY[0][0];
                absXYTemp[1][1] = view->shapeAbsXY[1][1];
            }

            // absXYTemp[0][0] += view->shape.line.sErr[0];
            // absXYTemp[0][1] += view->shape.line.sErr[1];
            // absXYTemp[1][0] += view->shape.line.eErr[0];
            // absXYTemp[1][1] += view->shape.line.eErr[1];

            view_line(
                colorTemp,
                absXYTemp[0][0], absXYTemp[0][1], absXYTemp[1][0], absXYTemp[1][1],
                view->shape.line.lineSize, view->shape.line.space);
        }
        break;
    case VST_CIRCLE:
        if (view->shapeColorPrint)
        {
            //圆/圆环
            if (view->shape.circle.div < 2)
            {
                //打底
                if (view->shapeColorBackground && view->shape.circle.rad2 > 0)
                    view_circle(
                        colorTemp2,
                        view->shapeAbsXY[0][0] + widthTemp / 2 - 1,
                        view->shapeAbsXY[0][1] + heightTemp / 2 - 1,
                        view->shape.circle.rad2,
                        view->shape.circle.rad2,
                        xyLimit[0][0], xyLimit[0][1], xyLimit[1][0], xyLimit[1][1]);
                view_circle(
                    colorTemp,
                    view->shapeAbsXY[0][0] + widthTemp / 2 - 1,
                    view->shapeAbsXY[0][1] + heightTemp / 2 - 1,
                    view->shape.circle.rad,
                    view->shape.circle.rad - view->shape.circle.rad2,
                    xyLimit[0][0], xyLimit[0][1], xyLimit[1][0], xyLimit[1][1]);
            }
            //分段圆环
            else
            {
                //打底
                if (view->shapeColorBackground)
                    view_circle(
                        colorTemp2,
                        view->shapeAbsXY[0][0] + widthTemp / 2 - 1,
                        view->shapeAbsXY[0][1] + heightTemp / 2 - 1,
                        view->shape.circle.rad,
                        view->shape.circle.rad - view->shape.circle.rad2,
                        xyLimit[0][0], xyLimit[0][1], xyLimit[1][0], xyLimit[1][1]);
                view_circleLoop(
                    colorTemp,
                    view->shapeAbsXY[0][0] + widthTemp / 2 - 1,
                    view->shapeAbsXY[0][1] + heightTemp / 2 - 1,
                    view->shape.circle.rad,
                    view->shape.circle.rad - view->shape.circle.rad2,
                    view->shape.circle.div,
                    view->shape.circle.divStart, view->shape.circle.divEnd);
            }
        }
        break;
    case VST_SECTOR:
        if (view->shape.sector.angle > 0)
        {
            //分块数
            intTemp = 360;
            //当前块号
            if (view->shape.sector.angle > 359)
                view->shape.sector.angle -= 360;
            intTemp2 = view->shape.sector.angle + 1;
            //起始块
            intTemp3 = (view->shape.sector.rotary - view->shape.sector.angle / 2) + 1;
            if (intTemp3 < 1)
                intTemp3 += 360;
            //结束块
            intTemp4 = (view->shape.sector.rotary + view->shape.sector.angle / 2) + 1;
            if (intTemp4 > 360)
                intTemp4 -= 360;

            //外经到内径部分
            if (view->shapeColorPrint)
                view_circleLoop(
                    colorTemp,
                    view->shapeAbsXY[0][0] + widthTemp / 2 - 1,
                    view->shapeAbsXY[0][1] + heightTemp / 2 - 1,
                    view->shape.sector.rad,
                    view->shape.sector.rad - view->shape.sector.rad2,
                    intTemp, intTemp3, intTemp4);
            //内径部分
            if (view->shapeColorBackground)
                view_circleLoop(
                    colorTemp2,
                    view->shapeAbsXY[0][0] + widthTemp / 2 - 1,
                    view->shapeAbsXY[0][1] + heightTemp / 2 - 1,
                    view->shape.sector.rad2,
                    view->shape.sector.rad2,
                    intTemp, intTemp3, intTemp4);
        }
        break;
    case VST_PROGRESS_BAR:
        //画边框线
        if (view->shapeColorBackground && view->shape.processBar.lineSize == 0)
        {
            view_rectangle(
                colorTemp2,
                view->shapeAbsXY[0][0], view->shapeAbsXY[0][1],
                view->shapeAbsXY[1][0], view->shapeAbsXY[1][1],
                view->shape.processBar.lineSize,
                view->shape.processBar.rad,
                xyLimit[0][0], xyLimit[0][1], xyLimit[1][0], xyLimit[1][1]);
        }
        if (view->shapeColorPrint && view->shape.processBar.percent > 0)
        {
            if (view->shape.processBar.edge > 0)
                intTemp3 = view->shape.processBar.edge + view->shape.processBar.lineSize / 2 + 1;
            else
                intTemp3 = 0;

            absXYTemp[0][0] = view->shapeAbsXY[0][0] + intTemp3;
            absXYTemp[0][1] = view->shapeAbsXY[0][1] + intTemp3;
            absXYTemp[1][0] = view->shapeAbsXY[1][0] - intTemp3;
            absXYTemp[1][1] = view->shapeAbsXY[1][1] - intTemp3;
            switch (view->shape.processBar.mode)
            {
            case 0: //从左到右
                view_rectangle(
                    colorTemp,
                    absXYTemp[0][0],
                    absXYTemp[0][1],
                    absXYTemp[0][0] + (absXYTemp[1][0] - absXYTemp[0][0]) * view->shape.processBar.percent / 100,
                    absXYTemp[1][1],
                    0,
                    view->shape.processBar.rad - view->shape.processBar.edge,
                    xyLimit[0][0], xyLimit[0][1], xyLimit[1][0], xyLimit[1][1]);
                break;
            case 2: //从下到上
                view_rectangle(
                    colorTemp,
                    absXYTemp[0][0],
                    absXYTemp[1][1] - (absXYTemp[1][1] - absXYTemp[0][1]) * view->shape.processBar.percent / 100,
                    absXYTemp[1][0],
                    absXYTemp[1][1],
                    0,
                    view->shape.processBar.rad - view->shape.processBar.edge,
                    xyLimit[0][0], xyLimit[0][1], xyLimit[1][0], xyLimit[1][1]);
                break;
            case 1: //从右到左
                view_rectangle(
                    colorTemp,
                    absXYTemp[1][0] - (absXYTemp[1][0] - absXYTemp[0][0]) * view->shape.processBar.percent / 100,
                    absXYTemp[0][1],
                    absXYTemp[1][0],
                    absXYTemp[1][1],
                    0,
                    view->shape.processBar.rad - view->shape.processBar.edge,
                    xyLimit[0][0], xyLimit[0][1], xyLimit[1][0], xyLimit[1][1]);
                break;
            case 3: //从上到下
                view_rectangle(
                    colorTemp,
                    absXYTemp[0][0],
                    absXYTemp[0][1],
                    absXYTemp[1][0],
                    absXYTemp[0][1] + (absXYTemp[1][1] - absXYTemp[0][1]) * view->shape.processBar.percent / 100,
                    0,
                    view->shape.processBar.rad - view->shape.processBar.edge,
                    xyLimit[0][0], xyLimit[0][1], xyLimit[1][0], xyLimit[1][1]);
                break;
            case 4: //左右同时缩进
                view_rectangle(
                    colorTemp,
                    absXYTemp[0][0] + (absXYTemp[1][0] - absXYTemp[0][0]) * (100 - view->shape.processBar.percent) / 100 / 2,
                    absXYTemp[0][1],
                    absXYTemp[1][0] - (absXYTemp[1][0] - absXYTemp[0][0]) * (100 - view->shape.processBar.percent) / 100 / 2,
                    absXYTemp[1][1],
                    0,
                    view->shape.processBar.rad - view->shape.processBar.edge,
                    xyLimit[0][0], xyLimit[0][1], xyLimit[1][0], xyLimit[1][1]);
                break;
            case 5: //上下同时缩进
                view_rectangle(
                    colorTemp,
                    absXYTemp[0][0],
                    absXYTemp[0][1] + (absXYTemp[1][1] - absXYTemp[0][1]) * (100 - view->shape.processBar.percent) / 100 / 2,
                    absXYTemp[1][0],
                    absXYTemp[1][1] - (absXYTemp[1][1] - absXYTemp[0][1]) * (100 - view->shape.processBar.percent) / 100 / 2,
                    0,
                    view->shape.processBar.rad - view->shape.processBar.edge,
                    xyLimit[0][0], xyLimit[0][1], xyLimit[1][0], xyLimit[1][1]);
                break;
            case 6: //上下左右同时缩进
                view_rectangle(
                    colorTemp,
                    absXYTemp[0][0] + (absXYTemp[1][0] - absXYTemp[0][0]) * (100 - view->shape.processBar.percent) / 100 / 2,
                    absXYTemp[0][1] + (absXYTemp[1][1] - absXYTemp[0][1]) * (100 - view->shape.processBar.percent) / 100 / 2,
                    absXYTemp[1][0] - (absXYTemp[1][0] - absXYTemp[0][0]) * (100 - view->shape.processBar.percent) / 100 / 2,
                    absXYTemp[1][1] - (absXYTemp[1][1] - absXYTemp[0][1]) * (100 - view->shape.processBar.percent) / 100 / 2,
                    0,
                    view->shape.processBar.rad - view->shape.processBar.edge,
                    xyLimit[0][0], xyLimit[0][1], xyLimit[1][0], xyLimit[1][1]);
            }
        }
        if (view->shapeColorBackground && view->shape.processBar.lineSize > 0)
        {
            view_rectangle(
                colorTemp2,
                view->shapeAbsXY[0][0], view->shapeAbsXY[0][1],
                view->shapeAbsXY[1][0], view->shapeAbsXY[1][1],
                view->shape.processBar.lineSize,
                view->shape.processBar.rad,
                xyLimit[0][0], xyLimit[0][1], xyLimit[1][0], xyLimit[1][1]);
        }
        break;
    case VST_SCROLL_BAR:
        if (view->shapeColorPrint)
            view_rectangle(
                colorTemp,
                view->shapeAbsXY[0][0], view->shapeAbsXY[0][1],
                view->shapeAbsXY[1][0], view->shapeAbsXY[1][1],
                view->shape.scrollBar.lineSize, view->shape.scrollBar.rad,
                xyLimit[0][0], xyLimit[0][1], xyLimit[1][0], xyLimit[1][1]);
        if (view->shapeColorBackground && view->shape.scrollBar.percentOfTotal > 0)
        {
            switch (view->shape.scrollBar.mode)
            {
            case 0: //左右
                intTemp = view->shape.scrollBar.percentOfTotal * widthTemp / 100;
                intTemp2 = (100 - view->shape.scrollBar.percentOfTotal) * widthTemp / 100 * view->shape.scrollBar.percent / 100;
                view_rectangle(
                    colorTemp2,
                    view->shapeAbsXY[0][0] + intTemp2 - 1,
                    view->shapeAbsXY[0][1],
                    view->shapeAbsXY[0][0] + intTemp2 + intTemp - 1,
                    view->shapeAbsXY[1][1],
                    0, view->shape.scrollBar.rad,
                    xyLimit[0][0], xyLimit[0][1], xyLimit[1][0], xyLimit[1][1]);
                break;
            case 1: //上下
                intTemp = view->shape.scrollBar.percentOfTotal * heightTemp / 100;
                intTemp2 = (100 - view->shape.scrollBar.percentOfTotal) * heightTemp / 100 * view->shape.scrollBar.percent / 100;
                view_rectangle(
                    colorTemp2,
                    view->shapeAbsXY[0][0],
                    view->shapeAbsXY[0][1] + intTemp2 - 1,
                    view->shapeAbsXY[1][0],
                    view->shapeAbsXY[0][1] + intTemp2 + intTemp - 1,
                    0, view->shape.scrollBar.rad,
                    xyLimit[0][0], xyLimit[0][1], xyLimit[1][0], xyLimit[1][1]);
                break;
            }
        }
        break;
    case VST_SWITCH:
        if (view->shape.sw.printType < 160)
            view->shape.sw.printType = 160;
        intTemp = view->shape.sw.printType / 10;
        //线宽为0时作打底
        if (view->shapeColorPrint && view->shape.sw.lineSize == 0)
            view_rectangle(
                colorTemp,
                view->shapeAbsXY[0][0], view->shapeAbsXY[0][1],
                view->shapeAbsXY[1][0], view->shapeAbsXY[1][1],
                view->shape.sw.lineSize, view->shape.sw.rad,
                xyLimit[0][0], xyLimit[0][1], xyLimit[1][0], xyLimit[1][1]);
        if (view->shape.sw.status) //开
        {
            view_rectangle(
                ViewColor.Green,
                view->shapeAbsXY[1][0] - widthTemp / 2,
                view->shapeAbsXY[0][1],
                view->shapeAbsXY[1][0],
                view->shapeAbsXY[1][1],
                0, view->shape.sw.rad,
                xyLimit[0][0], xyLimit[0][1], xyLimit[1][0], xyLimit[1][1]);
            view_string_rectangle(
                ViewColor.Content, -1, "开",
                view->shapeAbsXY[0][0] + widthTemp / 4 - intTemp / 2,
                view->shapeAbsXY[0][1] + heightTemp / 2 - intTemp / 2,
                intTemp, intTemp,
                xyLimit[0][0], xyLimit[0][1], xyLimit[1][0], xyLimit[1][1],
                view->shape.sw.printType, 0);
        }
        else //关
        {
            view_rectangle(
                ViewColor.Red,
                view->shapeAbsXY[0][0],
                view->shapeAbsXY[0][1],
                view->shapeAbsXY[0][0] + widthTemp / 2,
                view->shapeAbsXY[1][1],
                0, view->shape.sw.rad,
                xyLimit[0][0], xyLimit[0][1], xyLimit[1][0], xyLimit[1][1]);
            view_string_rectangle(
                ViewColor.Content, -1, "关",
                view->shapeAbsXY[1][0] - widthTemp / 4 - intTemp / 4 * 3,
                view->shapeAbsXY[1][1] - heightTemp / 2 - intTemp / 2,
                intTemp, intTemp,
                xyLimit[0][0], xyLimit[0][1], xyLimit[1][0], xyLimit[1][1],
                view->shape.sw.printType, 0);
        }
        //框
        if (view->shapeColorPrint && view->shape.sw.lineSize)
            view_rectangle(
                colorTemp,
                view->shapeAbsXY[0][0], view->shapeAbsXY[0][1],
                view->shapeAbsXY[1][0], view->shapeAbsXY[1][1],
                view->shape.sw.lineSize, view->shape.sw.rad,
                xyLimit[0][0], xyLimit[0][1], xyLimit[1][0], xyLimit[1][1]);
        break;
    default:
        break;
    }

    //图片输出
    if (view->picPath)
    {
        //获取图片数据
        if (view->picPathBakup != view->picPath)
        {
            //释放现在的内存
            if (view->pic)
                free(view->pic);
            //读取新图片
            view->pic = view_getPic(view->picPath, &view->picWidth, &view->picHeight, &view->picPB);
            //获取指针网格
            if (view->pic)
                view->picPathBakup = view->picPath;
            //找不到图片,下次不找了...
            else
                view->picPathBakup = view->picPath = NULL;
        }
        //输出范围计算
        view->picAbsXY[0][0] = view->absXY[0][0] + view->picEdgeLeft;
        view->picAbsXY[1][0] = view->absXY[1][0] - view->picEdgeRight;
        view->picAbsXY[0][1] = view->absXY[0][1] + view->picEdgeTop;
        view->picAbsXY[1][1] = view->absXY[1][1] - view->picEdgeBottom;
        //拉伸/缩放输出
        if (view->picUseReplaceColor && view->picReplaceColorBy)
            view_rectangle_padding(
                view->pic,
                view->picAbsXY[0][0], view->picAbsXY[0][1],
                view->picAbsXY[1][0], view->picAbsXY[1][1],
                view->picWidth, view->picHeight,
                view->picUseReplaceColor, view->picReplaceColor, view->picReplaceColorBy,
                xyLimit[0][0], xyLimit[0][1], xyLimit[1][0], xyLimit[1][1]);
        else
            view_rectangle_padding(
                view->pic,
                view->picAbsXY[0][0], view->picAbsXY[0][1],
                view->picAbsXY[1][0], view->picAbsXY[1][1],
                view->picWidth, view->picHeight,
                false, 0, 0,
                xyLimit[0][0], xyLimit[0][1], xyLimit[1][0], xyLimit[1][1]);
    }

    //内容输出
    if (view->text && view->textColor)
    {
        if (view->lock)
            colorTemp = _viewTool_lockColor(view->textColor);
        else
            colorTemp = view->textColor;
        //指定输出指针 *textOutput
        view->textOutput = _viewTool_textPrint(view->text, &view->textPrint);
        //输出范围计算
        view->textAbsXY[0][0] = view->absXY[0][0] + view->textEdgeLeft;
        view->textAbsXY[1][0] = view->absXY[1][0] - view->textEdgeRight;
        view->textAbsXY[0][1] = view->absXY[0][1] + view->textEdgeTop;
        view->textAbsXY[1][1] = view->absXY[1][1] - view->textEdgeBottom;
        widthTemp = view->textAbsXY[1][0] - view->textAbsXY[0][0] + 1;
        heightTemp = view->textAbsXY[1][1] - view->textAbsXY[0][1] + 1;

        if (view->textOutput)
        {
            //----- 输出方式一: 自动换行输出 -----
            if (view->textEdgeY > 0)
            {
                //计算实际输出时的宽/高
                ttf_getSizeByUtf8_multiLine(
                    ViewTTF,
                    view->textOutput,
                    view->textSize,
                    view->textEdgeX, view->textEdgeY,
                    view->textAbsXY[1][0] - view->textAbsXY[0][0],
                    &intTemp, &intTemp2);
                //左右居中处理
                if (view->textSideX == VTST_CENTER) //居中
                {
                    view->textAbsXY[0][0] = view->textAbsXY[0][0] + (widthTemp - intTemp) / 2;
                    view->textAbsXY[1][0] = view->textAbsXY[0][0] + intTemp - 1;
                }
                else if (view->textSideX == VTST_LEFT_TOP) //向左对齐
                    ;
                else //向右对齐
                    view->textAbsXY[0][0] = view->textAbsXY[0][0] + (widthTemp - intTemp);
                //上下居中处理
                if (view->textSideY == VTST_CENTER)
                {
                    view->textAbsXY[0][1] = view->textAbsXY[0][1] + (heightTemp - intTemp2) / 2;
                    view->textAbsXY[1][1] = view->textAbsXY[0][1] + intTemp2 - 1;
                }
                else if (view->textSideY == VTST_LEFT_TOP) //向上对齐
                    ;
                else //向下对齐
                    view->textAbsXY[0][1] = view->textAbsXY[0][1] + (heightTemp - intTemp2);

                view_string_rectangleLineWrap(
                    colorTemp, -1,
                    view->textOutput,
                    view->textAbsXY[0][0], view->textAbsXY[0][1],
                    intTemp, intTemp2,
                    xyLimit[0][0], xyLimit[0][1], xyLimit[1][0], xyLimit[1][1],
                    view->textSize, view->textEdgeX, view->textEdgeY,
                    NULL, NULL);
            }
            else
            {
                //计算实际输出时的宽/高
                intTemp = ttf_getSizeByUtf8(
                    ViewTTF,
                    view->textOutput,
                    view->textSize,
                    view->textEdgeX,
                    &intTemp2);
                //上下居中处理
                if (view->textSideY == VTST_CENTER)
                {
                    view->textAbsXY[0][1] = view->textAbsXY[0][1] + (heightTemp - intTemp2) / 2;
                    view->textAbsXY[1][1] = view->textAbsXY[0][1] + intTemp2 - 1;
                }
                else if (view->textSideY == VTST_LEFT_TOP) //向上对齐
                    ;
                else //向下对齐
                    view->textAbsXY[0][1] = view->textAbsXY[0][1] + (heightTemp - intTemp2);
                //----- 输出方式二: 滚动输出 -----
                if (view->scroll > 0 &&
                    intTemp > view->textAbsXY[1][0] - view->textAbsXY[0][0]) //内容超出方框才滚动
                {
                    if (view->textAbsXY[0][0] < xyLimit[0][0])
                        view->textAbsXY[0][0] = xyLimit[0][0];
                    if (view->textAbsXY[0][1] < xyLimit[0][1])
                        view->textAbsXY[0][1] = xyLimit[0][1];
                    if (view->textAbsXY[1][0] > xyLimit[1][0])
                        view->textAbsXY[1][0] = xyLimit[1][0];
                    if (view->textAbsXY[1][1] > xyLimit[1][1])
                        view->textAbsXY[1][1] = xyLimit[1][1];

                    view->scrollCount = view_string_rectangleCR(
                        colorTemp, -1,
                        view->textOutput,
                        view->textAbsXY[0][0],
                        view->textAbsXY[0][1],
                        view->textAbsXY[1][0] - view->textAbsXY[0][0], intTemp2,
                        view->textAbsXY[0][0], view->textAbsXY[0][1], view->textAbsXY[1][0], view->textAbsXY[1][1],
                        view->textSize, view->textEdgeX,
                        view->scrollCount);

                    view->scrollPeriodCount += 1;
                    if (view->scrollPeriodCount >= view->scrollPeriod)
                    {
                        view->scrollPeriodCount = 0;
                        view->scrollCount -= view->scroll; //注意偏移量是往负值增加,即字幕往左滚动
                    }
                }
                //----- 输出方式三: 正常输出 -----
                else
                {
                    //左右居中处理(上面滚动是用不着横向居中的)
                    if (view->textSideX == VTST_CENTER)
                    {
                        view->textAbsXY[0][0] = view->textAbsXY[0][0] + (widthTemp - intTemp) / 2;
                        view->textAbsXY[1][0] = view->textAbsXY[0][0] + intTemp - 1;
                    }
                    else if (view->textSideX == VTST_LEFT_TOP) //向左对齐
                        ;
                    else //向右对齐
                        view->textAbsXY[0][0] = view->textAbsXY[0][0] + (widthTemp - intTemp);
                    view_string_rectangle(
                        colorTemp, -1,
                        view->textOutput,
                        view->textAbsXY[0][0],
                        view->textAbsXY[0][1],
                        intTemp, intTemp2,
                        xyLimit[0][0], xyLimit[0][1], xyLimit[1][0], xyLimit[1][1],
                        view->textSize, view->textEdgeX);
                }
            }
        }
    }

    //下划线
    if (view->bottomLine > 0 && view->bottomLineColor)
    {
        if (view->absXY[1][1] < xyLimit[0][1] || view->absXY[1][1] > xyLimit[1][1])
            ;
        else
        {
            // if(view->textAbsXY[0][0] < xyLimit[0][0])
            //     intTemp = xyLimit[0][0];
            // else
            //     intTemp = view->textAbsXY[0][0];
            // //
            // if(view->textAbsXY[1][0] > xyLimit[1][0])
            //     intTemp2 = xyLimit[1][0];
            // else
            //     intTemp2 = view->textAbsXY[1][0];
            // //
            // view_line(
            //     view->bottomLineColor,
            //     intTemp, view->textAbsXY[1][1],
            //     intTemp2, view->textAbsXY[1][1],
            //     view->bottomLine, 0);
            //
            view_line(
                view->bottomLineColor,
                view->absXY[0][0], view->absXY[1][1],
                view->absXY[1][0], view->absXY[1][1],
                view->bottomLine, 0);
        }
    }

    //描边
    if (view->side && view->sideColor)
    {
        switch (view->shapeType)
        {
        case VST_RECT:
            view_rectangle(
                view->sideColor,
                view->shapeAbsXY[0][0], view->shapeAbsXY[0][1],
                view->shapeAbsXY[1][0], view->shapeAbsXY[1][1],
                view->side, view->shape.rect.rad,
                xyLimit[0][0], xyLimit[0][1], xyLimit[1][0], xyLimit[1][1]);
            break;
        case VST_PROGRESS_BAR:
            view_rectangle(
                view->sideColor,
                view->shapeAbsXY[0][0], view->shapeAbsXY[0][1],
                view->shapeAbsXY[1][0], view->shapeAbsXY[1][1],
                view->side, view->shape.processBar.rad,
                xyLimit[0][0], xyLimit[0][1], xyLimit[1][0], xyLimit[1][1]);
            break;
        case VST_SCROLL_BAR:
            view_rectangle(
                view->sideColor,
                view->shapeAbsXY[0][0], view->shapeAbsXY[0][1],
                view->shapeAbsXY[1][0], view->shapeAbsXY[1][1],
                view->side, view->shape.scrollBar.rad,
                xyLimit[0][0], xyLimit[0][1], xyLimit[1][0], xyLimit[1][1]);
            break;
        case VST_SWITCH:
            view_rectangle(
                view->sideColor,
                view->shapeAbsXY[0][0], view->shapeAbsXY[0][1],
                view->shapeAbsXY[1][0], view->shapeAbsXY[1][1],
                view->side, view->shape.sw.rad,
                xyLimit[0][0], xyLimit[0][1], xyLimit[1][0], xyLimit[1][1]);
            break;
        case VST_PARA:
            if (view->shape.para.mode == 1) //向外扩充(控件宽不变)
            {
                intTemp = view->shape.para.err / 2;
                intTemp2 = -view->shape.para.err / 2;
                intTemp3 = 0;
            }
            else if (view->shape.para.mode == 2) //向外扩充(控件宽变大)
            {
                intTemp = 0;
                intTemp2 = 0;
                intTemp3 = view->shape.para.err;
            }
            else //向内缩进(控件宽减少)
            {
                intTemp = view->shape.para.err;
                intTemp2 = -view->shape.para.err;
                intTemp3 = -view->shape.para.err;
            }
            view_parallelogram(
                view->sideColor,
                view->shapeAbsXY[0][0] + intTemp, view->shapeAbsXY[0][1],
                view->shapeAbsXY[1][0] + intTemp2, view->shapeAbsXY[1][1],
                view->side, widthTemp + intTemp3,
                xyLimit[0][0], xyLimit[0][1], xyLimit[1][0], xyLimit[1][1]);
            break;
        case VST_CIRCLE:
            view_circle(
                view->sideColor,
                view->shapeAbsXY[0][0] + widthTemp / 2 - 1,
                view->shapeAbsXY[0][1] + heightTemp / 2 - 1,
                view->shape.circle.rad, view->side,
                xyLimit[0][0], xyLimit[0][1], xyLimit[1][0], xyLimit[1][1]);
            break;
        case VST_SECTOR:
            view_circle(
                view->sideColor,
                view->shapeAbsXY[0][0] + widthTemp / 2 - 1,
                view->shapeAbsXY[0][1] + heightTemp / 2 - 1,
                view->shape.sector.rad, view->side,
                xyLimit[0][0], xyLimit[0][1], xyLimit[1][0], xyLimit[1][1]);
            break;
        default:
            view_rectangle(
                view->sideColor,
                view->absXY[0][0], view->absXY[0][1],
                view->absXY[1][0], view->absXY[1][1],
                view->side, 0,
                xyLimit[0][0], xyLimit[0][1], xyLimit[1][0], xyLimit[1][1]);
            break;
        }
    }

    //自定义绘制方法
    if (view->drawEnd)
        view->drawEnd(view, xyLimit);

    //focus 附加绘制
    if (focus)
    {
        if (view->focusCallBack) //使用自定义的光标方法
            view->focusCallBack(view, focus, xyLimit);
        else if (!view->focusCallBackFront) //使用默认的光标方法
        {
            widthTemp = view->shapeAbsXY[1][0] - view->shapeAbsXY[0][0] + 1;
            heightTemp = view->shapeAbsXY[1][1] - view->shapeAbsXY[0][1] + 1;
            //
            switch (view->shapeType)
            {
            case VST_RECT:
                view_rectangle(
                    focus->color,
                    view->shapeAbsXY[0][0], view->shapeAbsXY[0][1],
                    view->shapeAbsXY[1][0], view->shapeAbsXY[1][1],
                    focus->lineSize,
                    view->shape.rect.rad,
                    xyLimit[0][0], xyLimit[0][1], xyLimit[1][0], xyLimit[1][1]);
                break;
            case VST_PROGRESS_BAR:
                view_rectangle(
                    focus->color,
                    view->shapeAbsXY[0][0], view->shapeAbsXY[0][1],
                    view->shapeAbsXY[1][0], view->shapeAbsXY[1][1],
                    focus->lineSize,
                    view->shape.processBar.rad,
                    xyLimit[0][0], xyLimit[0][1], xyLimit[1][0], xyLimit[1][1]);
                break;
            case VST_SCROLL_BAR:
                view_rectangle(
                    focus->color,
                    view->shapeAbsXY[0][0], view->shapeAbsXY[0][1],
                    view->shapeAbsXY[1][0], view->shapeAbsXY[1][1],
                    focus->lineSize,
                    view->shape.scrollBar.rad,
                    xyLimit[0][0], xyLimit[0][1], xyLimit[1][0], xyLimit[1][1]);
                break;
            case VST_SWITCH:
                view_rectangle(
                    focus->color,
                    view->shapeAbsXY[0][0], view->shapeAbsXY[0][1],
                    view->shapeAbsXY[1][0], view->shapeAbsXY[1][1],
                    focus->lineSize,
                    view->shape.sw.rad,
                    xyLimit[0][0], xyLimit[0][1], xyLimit[1][0], xyLimit[1][1]);
                break;
            case VST_PARA:
                if (view->shape.para.mode == 1) //向外扩充(控件宽不变)
                {
                    intTemp = view->shape.para.err / 2;
                    intTemp2 = -view->shape.para.err / 2;
                    intTemp3 = 0;
                }
                else if (view->shape.para.mode == 2) //向外扩充(控件宽变大)
                {
                    intTemp = 0;
                    intTemp2 = 0;
                    intTemp3 = view->shape.para.err;
                }
                else //向内缩进(控件宽减少)
                {
                    intTemp = view->shape.para.err;
                    intTemp2 = -view->shape.para.err;
                    intTemp3 = -view->shape.para.err;
                }
                view_parallelogram(
                    focus->color,
                    view->shapeAbsXY[0][0] + intTemp, view->shapeAbsXY[0][1],
                    view->shapeAbsXY[1][0] + intTemp2, view->shapeAbsXY[1][1],
                    focus->lineSize,
                    widthTemp + intTemp3,
                    xyLimit[0][0], xyLimit[0][1], xyLimit[1][0], xyLimit[1][1]);
                break;
            case VST_CIRCLE:
                view_circle(
                    focus->color,
                    view->shapeAbsXY[0][0] + widthTemp / 2 - 1,
                    view->shapeAbsXY[0][1] + heightTemp / 2 - 1,
                    view->shape.circle.rad,
                    focus->lineSize,
                    xyLimit[0][0], xyLimit[0][1], xyLimit[1][0], xyLimit[1][1]);
                break;
            case VST_SECTOR:
                view_circle(
                    focus->color,
                    view->shapeAbsXY[0][0] + widthTemp / 2 - 1,
                    view->shapeAbsXY[0][1] + heightTemp / 2 - 1,
                    view->shape.sector.rad,
                    focus->lineSize,
                    xyLimit[0][0], xyLimit[0][1], xyLimit[1][0], xyLimit[1][1]);
                break;
            default:
                view_rectangle(
                    focus->color,
                    view->absXY[0][0], view->absXY[0][1],
                    view->absXY[1][0], view->absXY[1][1],
                    focus->lineSize,
                    0,
                    xyLimit[0][0], xyLimit[0][1], xyLimit[1][0], xyLimit[1][1]);
                break;
            }
        }
    }
}

int _viewTool_viewListDraw(
    void *object,
    View_Focus *focus,
    ViewButtonTouch_Event *event,
    View_Struct *viewParent,
    View_Struct *view,
    int xyLimit[2][2])
{
    int ret;
    View_Struct *vsThis = view, *vsNext = NULL;

    while (vsThis)
    {
        vsNext = vsThis->next;

        ret = view_draw2(object, focus, event, viewParent, vsThis, xyLimit);
        if (ret == CALLBACK_BREAK)
            return CALLBACK_BREAK;

        vsThis = vsNext;
    }
    return CALLBACK_OK;
}

int view_draw2(
    void *object,
    View_Focus *focus,
    ViewButtonTouch_Event *event,
    View_Struct *viewParent,
    View_Struct *view,
    int xyLimit[2][2])
{
    View_Struct *parent;
    int xyLimitVsTemp[2][2];
    int ret;

    if (!view)
        return CALLBACK_OK;

    // printf("view_draw2: %s \r\n", view->name?view->name:"NULL");

    //禁止回调函数检查
    view->callBackForbid = true;

    //找爸爸
    if (viewParent)
        parent = viewParent;
    else
        parent = &ViewCommonParent;

    //再次确认父子关系(防止爸爸为空)
    view->parent = parent;

    //现在绘制的是顶层view
    if (parent == &ViewCommonParent)
    {
        //避免多条线同时使用 ViewCommonParent 作父控件
        //造成 tickOfDraw 的跳变增加; 子 view 可以通过缓存
        //该值,比较是否每次+1来判断是否第一次进入自己的界面
        ViewCommonParent.tickOfDraw += 1;

        //更新一次 ViewCommonParent 的系统滴答时钟
        ViewCommonParent.tickOfTimeMs = view_tickMs();
    }

    //共享滴答时钟
    view->tickOfTimeMs = ViewCommonParent.tickOfTimeMs;

    //当前view是第一次绘制
    if (view->tickOfDraw != parent->tickOfDraw)
    {
        //absXY[2][2] 和 absWidth, absHeight 计算
        _viewTool_viewLocal(
            parent->tickOfDraw, view,
            parent->absWidth, parent->absHeight, parent->absXY);
    }

    //子 view 的限制范围在父 view 内且在 xyLimit 内
    if (view->overDraw || !xyLimit)
        memcpy(&xyLimitVsTemp, &ViewCommonParent.absXY, sizeof(xyLimitVsTemp));
    else
    {
        memcpy(&xyLimitVsTemp, &view->absXY, sizeof(xyLimitVsTemp));
        if (xyLimitVsTemp[0][0] < xyLimit[0][0])
            xyLimitVsTemp[0][0] = xyLimit[0][0];
        if (xyLimitVsTemp[0][1] < xyLimit[0][1])
            xyLimitVsTemp[0][1] = xyLimit[0][1];
        if (xyLimitVsTemp[1][0] > xyLimit[1][0])
            xyLimitVsTemp[1][0] = xyLimit[1][0];
        if (xyLimitVsTemp[1][1] > xyLimit[1][1])
            xyLimitVsTemp[1][1] = xyLimit[1][1];
    }

    // printf("view_draw2: %s %d[%d-%d](%d-%d) %d[%d-%d](%d-%d), sync: %d -- %d\r\n",
    //     view->name,
    //     view->absWidth,
    //     view->absXY[0][0], view->absXY[1][0],
    //     xyLimitVsTemp[0][0], xyLimitVsTemp[1][0],
    //     view->absHeight,
    //     view->absXY[0][1], view->absXY[1][1],
    //     xyLimitVsTemp[0][1], xyLimitVsTemp[1][1],
    //     view->tickOfDraw, ViewCommonParent.tickOfDraw);

    //开始绘图
    if (view->disable ||
        xyLimitVsTemp[0][0] > xyLimitVsTemp[1][0] ||
        xyLimitVsTemp[0][1] > xyLimitVsTemp[1][1])
        ;
    else
    {
        //isFirstIn ?
        view->tickOfDrawLast += 1;
        if (view->tickOfDrawLast != view->tickOfDraw)
            view->isFirstIn = true;
        else
            view->isFirstIn = false;
        view->tickOfDrawLast = view->tickOfDraw;

        //viewStart
        if (view->viewStart)
        {
            ret = view->viewStart(view, object, focus, event);
            if (ret == CALLBACK_OK)
                ;
            else if (ret == CALLBACK_IGNORE) //结束本 view 绘制
            {
                view->callBackForbid = false; //允许回调函数检查
                return CALLBACK_OK;
            }
            else if (ret == CALLBACK_BREAK) //结束整个 UI 绘制
            {
                view->callBackForbid = false; //允许回调函数检查
                return CALLBACK_BREAK;
            }
            else if (ret == CALLBACK_ERR) //异常错误 打印
                printf("view_draw2 : view %s viewStart() err !!\r\n", view->name ? view->name : "???");
        }

        //在 jumpView 无效或 jumpViewKeepOn 的情况下可以绘制: view, 子view
        if (!view->invisible)
        {
            if (!view->jumpView || view->jumpViewKeepOn || !view->jumpViewOn)
            {
                //本 view 绘制
                if (!view->hide)
                    _view_draw(view, xyLimitVsTemp);

                //子 view 绘制
                if (view->view && !view->hideView)
                {
                    ret = _viewTool_viewListDraw(object, focus, event, view, view->view, xyLimitVsTemp); //子 view 直接继承限制范围
                    if (ret == CALLBACK_BREAK)
                    {
                        view->callBackForbid = false; //允许回调函数检查
                        return CALLBACK_BREAK;
                    }
                }
            }
        }

        //跳转 view
        if (view->jumpView && (view->jumpViewOn || view->jumpViewKeepOn))
        {
            ret = view_draw2(object, focus, event, view, view->jumpView, xyLimit); //子 view 直接继承限制范围
            if (ret == CALLBACK_BREAK)
            {
                view->callBackForbid = false; //允许回调函数检查
                return CALLBACK_BREAK;
            }
        }

        //viewEnd
        if (view->viewEnd)
        {
            ret = view->viewEnd(view, object, focus, event);
            if (ret == CALLBACK_OK)
                ;
            else if (ret == CALLBACK_IGNORE) //结束本 view 绘制
            {
                view->callBackForbid = false; //允许回调函数检查
                return CALLBACK_OK;
            }
            else if (ret == CALLBACK_BREAK) //结束整个 UI 绘制
            {
                view->callBackForbid = false; //允许回调函数检查
                return CALLBACK_BREAK;
            }
            else if (ret == CALLBACK_ERR) //异常错误 打印
                printf("view_draw2 : view %s viewEnd() err !!\r\n", view->name ? view->name : "???");
        }
    }

    view->callBackForbid = false; //允许回调函数检查

    return CALLBACK_OK;
}

int view_draw(void *object, View_Struct *view)
{
    return view_draw2(object, NULL, NULL, NULL, view, NULL);
}

ViewCallBack view_touchLocal(int xy[2], View_Struct *view, View_Struct **retView)
{
    ViewCallBack ret = NULL;
    View_Struct *vsTemp = NULL;

    if (!xy || !view || view->disable)
        return NULL;

    //等待绘制结束
    while (view->callBackForbid)
        view_delayms(5);

    // printf("view_touchLocal: %s check\r\n", view->name?view->name:"NULL");

    //不在范围
    if (!view->overDraw &&
        (view->absXY[0][0] > xy[0] || view->absXY[0][1] > xy[1] ||
         view->absXY[1][0] < xy[0] || view->absXY[1][1] < xy[1]))
        return NULL;

    *retView = NULL;

    //跳转 view
    if (view->jumpView && (view->jumpViewOn || view->jumpViewKeepOn))
    {
        ret = view_touchLocal(xy, view->jumpView, retView); //递归
        if (*retView)
            return ret;
    }

    if (!view->invisible &&
        (!view->jumpView || view->jumpViewKeepOn || !view->jumpViewOn))
    {
        //子 view
        if (view->view && !view->hideView)
        {
            vsTemp = view->lastView;
            while (vsTemp)
            {
                ret = view_touchLocal(xy, vsTemp, retView); //递归
                if (*retView &&
                    (*retView)->callBack &&
                    (((*retView)->focusStop && !(*retView)->lock) || (*retView)->enMoveEvent))
                    return ret;

                vsTemp = vsTemp->last;
            }
        }

        //本 view
        if (view->hide ||
            view->absXY[0][0] > xy[0] ||
            view->absXY[0][1] > xy[1] ||
            view->absXY[1][0] < xy[0] ||
            view->absXY[1][1] < xy[1])
            ;
        else
        {
            *retView = view;
            if (view->callBack &&
                ((view->focusStop && !view->lock) || view->enMoveEvent))
                return view->callBack;
        }
    }

    return NULL;
}

//--------------------  viewTool multiInput --------------------

typedef struct
{
    ViewValue_Format *value;
    ViewValue_Format *candidate;
    View_Struct *backView;
    ViewCallBack callBack;
    int type;
    bool callBackNext;
    View_Struct *callBackNextView;
    int astrict; //限制输入长度(针对横向列表输入)

    int contentSize2, contentMinSize;
} _InputBackup_Struct;

int _input_comm_callBack(View_Struct *view, void *object, View_Focus *focus, ViewButtonTouch_Event *event)
{
    View_Struct *vsHead = NULL, *vsTail = NULL, *vsCurr = NULL, *vsStandard = NULL;
    View_Struct *vsParent, *vsTemp;
    int movMax = 0;
    _InputBackup_Struct *ibs;
    uint8_t alpha;

    // printf("-- FUN CALLBACK-- _input_comm_callBack : %d\r\n", event->move);

    //---------- 滚动页面通用封装 ----------
    if (focus && event)
    {
        vsParent = view->parent;
        ibs = vsParent->privateData;

        if (ibs->type == 0)
        {
            ;
        }
        else if (ibs->type == 1)
        {
            //找到头和尾
            vsStandard = vsParent->view->next;
            vsHead = vsParent->view->next->next;
            vsTail = vsParent->lastView;
            movMax = -(vsTail->number - vsHead->number) * (ibs->contentMinSize + 4);
        }
        else if (ibs->type == 2)
        {
            //找到头和尾
            vsStandard = vsParent->view->next->next;
            vsHead = vsParent->view->next->next->next;
            vsTail = vsParent->lastView->last;
            movMax = -(vsTail->number - vsHead->number) * (ibs->contentMinSize / 2 + 4);
        }

        if (event->type == VBTT_CLICK_UP)
        {
            ;
        }
        else if (ibs->type == 1 &&
                 (event->type == VBTT_M_UP || event->type == VBTT_M_DOWN))
        {
            //不要焦点,以免影响滚动
            view_focusJump(focus, vsParent);
            //平移
            if (event->type == VBTT_M_UP)
                vsHead->rTopBottomErr -= event->move;
            else
                vsHead->rTopBottomErr += event->move;
            //
            if (vsHead->rTopBottomErr > 0)
                vsHead->rTopBottomErr = 0;
            else if (vsHead->rTopBottomErr < movMax)
                vsHead->rTopBottomErr = movMax;
            //找到当前居中的 view
            vsCurr = view_num(vsParent->view, vsHead->number + (int)((-vsHead->rTopBottomErr) / (ibs->contentMinSize + 4)));
            //透明度 字体 和 控件高度 调整
            vsCurr->textSize = vsStandard->textSize;
            vsCurr->height = vsStandard->height;
            vsCurr->bottomLine = 2;
            for (alpha = 15, vsTemp = vsCurr->last;
                 vsTemp && vsTemp->number > 2 && alpha < 255;
                 alpha += 60, vsTemp = vsTemp->last)
            {
                vsTemp->textColor &= 0xFFFFFF00;
                vsTemp->textColor |= alpha;
                vsTemp->textSize = ibs->contentMinSize * 10;
                vsTemp->height = ibs->contentMinSize + 4;
                vsTemp->bottomLine = 0;
            }
            for (alpha = 15, vsTemp = vsCurr->next;
                 vsTemp && alpha < 255;
                 alpha += 60, vsTemp = vsTemp->next)
            {
                vsTemp->textColor &= 0xFFFFFF00;
                vsTemp->textColor |= alpha;
                vsTemp->textSize = ibs->contentMinSize * 10;
                vsTemp->height = ibs->contentMinSize + 4;
                vsTemp->bottomLine = 0;
            }
        }
        else if (ibs->type == 1 && event->status == VBTS_UP &&
                 (event->type == VBTT_M_LEFT || event->type == VBTT_M_RIGHT))
        {
            view_focusEvent(focus, VFE_RETURN);
        }
        else if (ibs->type == 2 &&
                 (event->type == VBTT_M_LEFT || event->type == VBTT_M_RIGHT))
        {
            //不要焦点,以免影响滚动
            view_focusJump(focus, vsParent);
            //平移
            if (event->type == VBTT_M_LEFT)
                vsHead->rLeftRightErr -= event->move;
            else
                vsHead->rLeftRightErr += event->move;

            if (vsHead->rLeftRightErr > 0)
                vsHead->rLeftRightErr = 0;
            else if (vsHead->rLeftRightErr < movMax)
                vsHead->rLeftRightErr = movMax;
            //找到当前居中的 view
            vsCurr = view_num(vsParent->view, vsHead->number + (int)((-vsHead->rLeftRightErr) / (ibs->contentMinSize / 2 + 4)));
            //字体 和 控件宽度 调整
            vsCurr->textSize = vsStandard->textSize;
            vsCurr->width = vsStandard->width;
            vsCurr->bottomLine = 2;
            for (vsTemp = vsCurr->last;
                 vsTemp && vsTemp->number > 3;
                 vsTemp = vsTemp->last)
            {
                vsTemp->textSize = ibs->contentMinSize * 10;
                vsTemp->width = ibs->contentMinSize / 2 + 4;
                vsTemp->bottomLine = 0;
            }
            for (vsTemp = vsCurr->next;
                 vsTemp && vsTemp->number < vsParent->lastView->number;
                 vsTemp = vsTemp->next)
            {
                vsTemp->textSize = ibs->contentMinSize * 10;
                vsTemp->width = ibs->contentMinSize / 2 + 4;
                vsTemp->bottomLine = 0;
            }
        }
    }
    //---------- 滚动页面通用封装 ----------

    return CALLBACK_OK;
}

// 弹窗的 确认键 回调函数
int _input_enter_callBack(View_Struct *view, void *object, View_Focus *focus, ViewButtonTouch_Event *event)
{
    int i;
    View_Struct *vsParent, *vsTemp;
    _InputBackup_Struct *ibs;
    char *backString = NULL, vidChar[2] = {0};

    if (focus && event)
    {
        //点击结束事件
        if (event->type == VBTT_CLICK_UP)
        {
            vsParent = view->parent;
            ibs = vsParent->privateData;
            //弹窗类型
            switch (ibs->type)
            {
            //仅提示弹窗
            case 0:
                //返回前赋值
                if (ibs->value && ibs->value->type == VT_BOOL)
                    ibs->value->value.Bool = !ibs->value->value.Bool;
                break;
            //纵向列表
            case 1:
                break;
            //横向列表
            case 2:
                //候选列表有删除符
                if (vsParent->view->next->next->text->value.String[0] == VIEW_DEL_CHAR)
                    vidChar[0] = VIEW_DEL_CHAR;
                //返回前赋值
                if (ibs->value)
                {
                    //把横向输入的内容整理成字符串 backString, 长度为 i
                    backString = (char *)calloc(
                        vsParent->lastView->last->number -
                            vsParent->view->next->next->number + 1,
                        sizeof(char));
                    for (i = 0, vsTemp = vsParent->view->next->next->next;
                         vsTemp && vsTemp != vsParent->lastView;
                         i++, vsTemp = vsTemp->next)
                        backString[i] = vsTemp->text->value.Char;

                    if (i > 0)
                    {
                        if (ibs->value->type == VT_INT)
                        {
                            if (i == 1 && backString[0] == vidChar[0])
                                ibs->value->value.Int = 0;
                            else
                            {
                                // if(backString[0] == '-' && strlen(backString) > 10)//限制整形数有效数字9位
                                //     backString[10] = 0;
                                // else if(strlen(backString) > 9)
                                //     backString[9] = 0;
                                sscanf(backString, "%d", &ibs->value->value.Int);
                            }
                        }
                        else if (ibs->value->type == VT_DOUBLE)
                        {
                            if (i == 1 && backString[0] == vidChar[0])
                                ibs->value->value.Double = 0;
                            else
                            {
                                // if(backString[0] == '-' && strlen(backString) > 17)//限制double有效数字16位
                                //     backString[17] = 0;
                                // else if(strlen(backString) > 16)
                                //     backString[16] = 0;
                                sscanf(backString, "%lf", &ibs->value->value.Double);
                            }
                        }
                        else if (ibs->value->type == VT_STRING)
                        {
                            if (i == 1 && backString[0] == vidChar[0])
                            {
                                viewValue_reset(ibs->value, NULL, ibs->value->type, 1, vidChar);
                                ibs->value->value.String[0] = 0; //断句
                            }
                            else
                            {
                                if (i > 0 && backString[i - 1] == vidChar[0])
                                    backString[i - 1] = 0;
                                viewValue_reset(ibs->value, NULL, ibs->value->type, 1, backString);
                            }
                        }
                    }

                    free(backString);
                }
                break;
            default:
                break;
            }

            if (ibs->callBack)
                ibs->callBack(ibs->backView, object, focus, event);

            //上面回调结束时 焦点位置没有变化
            if (focus && focus->view == view)
            {
                view_focusEvent(focus, VFE_RETURN);
                //返回后右移一位
                if (ibs->callBackNext)
                {
                    // printf("callBackView : %s\r\n", ibs->callBackNextView?ibs->callBackNextView->name:"NULL");
                    if (ibs->callBackNextView) //有指定跳转项
                        view_focusJump(focus, ibs->callBackNextView);
                    else
                        view_focusEvent(focus, VFE_NEXT);
                }
            }
        }
        else
            _input_comm_callBack(view, object, focus, event);
    }

    return CALLBACK_OK;
}

// 弹窗的 取消键 回调函数
int _input_returnOrCancel_callBack(View_Struct *view, void *object, View_Focus *focus, ViewButtonTouch_Event *event)
{
    if (focus && event)
    {
        if (event->type == VBTT_CLICK_UP)
            view_focusEvent(focus, VFE_RETURN);
        else
            _input_comm_callBack(view, object, focus, event);
    }

    return CALLBACK_OK;
}

// 纵向列表 回调函数
int _input_list_callBack(View_Struct *view, void *object, View_Focus *focus, ViewButtonTouch_Event *event)
{
    _InputBackup_Struct *ibs;

    if (focus && event)
    {
        //点击事件,拷贝数据后返回
        if (event->type == VBTT_CLICK_UP)
        {
            //缓存选中项在候选中的位置(从0数起)
            viewInputSelectNumber = view->number - 3;
            if (viewInputSelectNumber < 0)
                viewInputSelectNumber = 0;
            printf("order: %d\r\n", viewInputSelectNumber);

            ibs = ((View_Struct *)(view->parent))->privateData;
            //返回前赋值
            if (ibs->value)
            {
                //选中的是个删除符,拷贝删除符到返回缓存 ibs->value
                if (view->text == &ViewSrc.Api_Del_Char)
                    viewValue_reset(ibs->value, NULL, VT_CHAR, 1, VIEW_DEL_CHAR);
                //拷贝选中数据到返回缓存 ibs->value
                else
                    viewValue_copy(ibs->value, view->text);
            }
            //有输入结束回调
            if (ibs->callBack)
                ibs->callBack(ibs->backView, object, focus, event);
            //上面回调结束时 焦点位置没有变化
            if (focus && focus->view == view)
            {
                view_focusEvent(focus, VFE_RETURN);
                //返回后右移一位
                if (ibs->callBackNext)
                {
                    // printf("callBackView : %s\r\n", ibs->callBackNextView?ibs->callBackNextView->name:"NULL");
                    if (ibs->callBackNextView) //有指定跳转项
                        view_focusJump(focus, ibs->callBackNextView);
                    else
                        view_focusEvent(focus, VFE_NEXT);
                }
            }
        }
        else
            _input_comm_callBack(view, object, focus, event);
    }

    return CALLBACK_OK;
}

void view_input_focusCallBackFront(View_Struct *view, View_Focus *focus, int xyLimit[2][2])
{
    view_rectangle(
        focus->color,
        view->absXY[0][0], view->absXY[0][1],
        view->absXY[1][0], view->absXY[1][1],
        0, 0,
        xyLimit[0][0], xyLimit[0][1], xyLimit[1][0], xyLimit[1][1]);
}

// 横向列表 回调函数 -> 生成一个纵向列表
int _input_list2_callBack(View_Struct *view, void *object, View_Focus *focus, ViewButtonTouch_Event *event);

// 点击横向列表生成的纵向列表,其输入结束时的回调
int _input_list2_input_callBack(View_Struct *view, void *object, View_Focus *focus, ViewButtonTouch_Event *event)
{
    View_Struct *vsParent, *vsTemp;
    char vidChar = 0;
    _InputBackup_Struct *ibs;

    if (view)
    {
        vsParent = view->parent;
        if (vsParent->view->next->next->text->value.String[0] == VIEW_DEL_CHAR)
            vidChar = VIEW_DEL_CHAR;

        //使用了删除符号
        if (vidChar)
        {
            //当前输入为列表最后一个,且输入的不是"删除",则在列表后面再加一位
            if (view == vsParent->lastView->last && view->text->value.Char != vidChar)
            {
                ibs = (_InputBackup_Struct *)(vsParent->privateData);
                if (ibs->astrict > 0 &&
                    ibs->astrict < view->number - 2) //再加一位就超出限制输入范围了
                    ;
                else
                {
                    vsTemp = (View_Struct *)calloc(1, sizeof(View_Struct));
                    sprintf(vsTemp->name, "_input_content%d", view->number + 1);
                    vsTemp->width = ibs->contentMinSize / 2 + 4;
                    vsTemp->height = ibs->contentSize2 + 4;
                    vsTemp->rNumber = VRNT_LAST;
                    vsTemp->rType = VRT_RIGHT;
                    vsTemp->text = viewValue_init(vsTemp->name, VT_CHAR, 1, VIEW_DEL_CHAR);
                    vsTemp->textSize = ibs->contentMinSize * 10;
                    vsTemp->textColor = ViewColor.Tips;
                    vsTemp->bottomLineColor = focus->color;
                    vsTemp->focusStop = true;
                    vsTemp->callBack = (ViewCallBack)&_input_list2_callBack;
                    vsTemp->focusCallBackFront = (FocusCallBack)&view_input_focusCallBackFront;
                    view_insert(view, vsTemp, false); //使用插入方式
                }
            }
            //在任意位置输入了删除符号,移除当前 view 往后的 view
            else if (view->text->value.Char == vidChar)
            {
                while (vsParent->callBackForbid)
                    view_delayms(5);
                while (view->next != vsParent->lastView)
                    view_delete(view->next);
            }
        }
    }

    return CALLBACK_OK;
}

// 横向列表 回调函数 -> 生成一个纵向列表
int _input_list2_callBack(View_Struct *view, void *object, View_Focus *focus, ViewButtonTouch_Event *event)
{
    if (focus && event)
    {
        if (event->type == VBTT_CLICK_UP)
        {
            view_input(
                view->parent->view->text->value.String,
                view->text,
                view->parent->view->next->next->text,
                view,
                view->parent,
                focus,
                (ViewCallBack)&_input_list2_input_callBack, true, NULL, 0);
        }
        else
            _input_comm_callBack(view, object, focus, event);
    }

    return CALLBACK_OK;
}

int _input_viewStart(View_Struct *view, void *object, View_Focus *focus, ViewButtonTouch_Event *event)
{
    View_Struct *vsFocus, *vsTemp;
    uint8_t alpha;
    int textSize;
    _InputBackup_Struct *ibs;

    ibs = view->privateData;
    textSize = ibs->contentMinSize * 10;

    if (focus && !focus->view->backView && focus->view->parent == view)
    {
        vsFocus = focus->view;
        switch ((((_InputBackup_Struct *)(view->privateData)))->type)
        {
        //纵向列表
        case 1:
            if (vsFocus->absXY[0][1] != view->view->next->absXY[0][1])
            {
                //位置平移
                view->view->next->next->rTopBottomErr = -(vsFocus->number - view->view->next->next->number) * (ibs->contentMinSize + 4);
                //透明度 字体 和 控件高度 调整
                vsFocus->textColor |= 0xFF;
                vsFocus->textSize = view->view->next->textSize;
                vsFocus->height = view->view->next->height;
                // ->last
                for (alpha = 15, vsTemp = vsFocus->last;
                     vsTemp && vsTemp->number > 2 && alpha < 255;
                     alpha += 60, vsTemp = vsTemp->last)
                {
                    vsTemp->textColor &= 0xFFFFFF00;
                    vsTemp->textColor |= alpha;
                    vsTemp->textSize = textSize;
                    vsTemp->height = ibs->contentMinSize + 4;
                    vsTemp->bottomLine = 0;
                }
                for (; vsTemp && vsTemp->number > 2; vsTemp = vsTemp->last)
                {
                    vsTemp->textColor |= 0xFF;
                    vsTemp->textSize = textSize;
                    vsTemp->height = ibs->contentMinSize + 4;
                    vsTemp->bottomLine = 0;
                }
                // ->next
                for (alpha = 15, vsTemp = vsFocus->next;
                     vsTemp && alpha < 255;
                     alpha += 60, vsTemp = vsTemp->next)
                {
                    vsTemp->textColor &= 0xFFFFFF00;
                    vsTemp->textColor |= alpha;
                    vsTemp->textSize = textSize;
                    vsTemp->height = ibs->contentMinSize + 4;
                    vsTemp->bottomLine = 0;
                }
                for (; vsTemp; vsTemp = vsTemp->next)
                {
                    vsTemp->textColor |= 0xFF;
                    vsTemp->textSize = textSize;
                    vsTemp->height = ibs->contentMinSize + 4;
                    vsTemp->bottomLine = 0;
                }
            }
            break;
        //横向列表
        case 2:
            if (vsFocus->number > 3 &&
                vsFocus->number < view->lastView->number &&
                vsFocus->absXY[0][0] != view->view->next->next->absXY[0][0])
            {
                //位置平移
                view->view->next->next->next->rLeftRightErr = -(vsFocus->number - view->view->next->next->next->number) * (ibs->contentMinSize / 2 + 4);
                //字体 和 控件宽度 调整
                vsFocus->textSize = view->view->next->next->textSize;
                vsFocus->width = view->view->next->next->width;
                for (vsTemp = vsFocus->last;
                     vsTemp && vsTemp->number > 3;
                     vsTemp = vsTemp->last)
                {
                    vsTemp->textSize = ibs->contentMinSize * 10;
                    vsTemp->width = ibs->contentMinSize / 2 + 4;
                    vsTemp->bottomLine = 0;
                }
                for (vsTemp = vsFocus->next;
                     vsTemp && vsTemp->number < view->lastView->number;
                     vsTemp = vsTemp->next)
                {
                    vsTemp->textSize = ibs->contentMinSize * 10;
                    vsTemp->width = ibs->contentMinSize / 2 + 4;
                    vsTemp->bottomLine = 0;
                }
            }
            break;
        default:
            break;
        }
    }

    return CALLBACK_OK;
}

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
    int astrict)
{
    View_Struct *vs;
    View_Struct *vsTemp, *retView = NULL;
    _InputBackup_Struct *ibs;

    int labelSize = ViewSrc.Label_Size.value.Int;
    int contentSize = ViewSrc.Content_Type.value.Int;
    int contentSize2 = 4;
    int contentMinSize = 32;
    int lineSize = 2;
    int rad = ViewSrc.Shape_Rad.value.Int;

    int count;
    bool retBool = false;
    int retInt = 0;
    int valueNum;
    int intTemp[2];
    char *pointTemp = NULL;
    ViewValue_Format **vvfArray = NULL;

    char numberValueString[64] = {0};
    char *valueStringPoint = NULL;
    char strDemo[] = "%.8lf";

    if (!backView || !focus || !vsParent)
        return;
    //布置一个全屏的 view
    vs = (View_Struct *)calloc(1, sizeof(View_Struct));
    vs->privateData = ibs = (_InputBackup_Struct *)calloc(1, sizeof(_InputBackup_Struct));
    ibs->value = value;
    ibs->candidate = candidate;
    ibs->backView = backView;
    ibs->callBack = callBack; //备份数据
    ibs->callBackNext = callBackNext;
    ibs->callBackNextView = callBackNextView;
    ibs->astrict = astrict;
    ibs->contentSize2 = contentSize2;
    ibs->contentMinSize = contentMinSize;
    vs->width = VWHT_MATCH;
    vs->height = VWHT_MATCH;
    sprintf(vs->name, "_input_%d", backView->tickOfDraw);
    // vs->rLeftRightErr = -vsParent->absXY[0][0];
    // vs->rTopBottomErr = -vsParent->absXY[0][1];
    // vs->backGroundColor = ViewColor.BackGround;//暗幕
    // vs->overDraw = true;
    vs->viewStart = (ViewCallBack)&_input_viewStart;
    // vs->viewEnd = (ViewCallBack)&_input_viewEnd;

    //type 0: 仅提示(暂不支持数组类数据的输入)
    if (!value ||
        value->type == VT_BOOL ||
        value->type == VT_STRING_ARRAY ||
        value->type == VT_INT_ARRAY ||
        value->type == VT_DOUBLE_ARRAY ||
        value->type == VT_BOOL_ARRAY)
    {
        ibs->type = 0;

        //框 和 label
        vsTemp = (View_Struct *)calloc(1, sizeof(View_Struct));
        strcpy(vsTemp->name, "_input_frameContent");
        vsTemp->width = VWHT_MATCH;
        vsTemp->height = VWHT_MATCH * 4 - 3;
        vsTemp->shapeType = VST_RECT;
        vsTemp->shape.rect.rad = rad;
        vsTemp->shape.rect.lineSize = lineSize;
        vsTemp->shapeColorPrint = ViewColor.Button;
        vsTemp->shapeEdgeBottom = 1;
        vsTemp->shapeEdgeLeft = vsTemp->shapeEdgeRight = 1;
        vsTemp->text = viewValue_init("_input_frameContent", VT_STRING, 1, label);
        vsTemp->textSize = contentSize * 10;
        vsTemp->textColor = ViewColor.Tips;
        vsTemp->textEdgeTop = vsTemp->textEdgeBottom = 5;
        vsTemp->textEdgeLeft = vsTemp->textEdgeRight = 5; //四周保持5的间距
        vsTemp->textEdgeY = 6;                            //自动换行,行间距5

        ttf_getSizeByUtf8_multiLine(
            ViewTTF,
            label,
            vsTemp->textSize,
            0, vsTemp->textEdgeY,
            vsParent->absWidth - vsTemp->textEdgeLeft - vsTemp->textEdgeRight,
            NULL, &retInt);

        if (retInt > (vsParent->absHeight) * 3 / 4 - vsTemp->textEdgeTop - vsTemp->textEdgeBottom) //提示信息太长超出范围
        {
            vsTemp->textEdgeTop = vsTemp->textEdgeBottom = 3;
            vsTemp->textEdgeLeft = vsTemp->textEdgeRight = 3; //四周保持5的间距
            vsTemp->textEdgeY = 4;
            vsTemp->textSize = 200;

            ttf_getSizeByUtf8_multiLine(
                ViewTTF,
                label,
                vsTemp->textSize,
                0, vsTemp->textEdgeY,
                vsParent->absWidth - vsTemp->textEdgeLeft - vsTemp->textEdgeRight,
                NULL, &retInt);

            if (retInt > (vsParent->absHeight) * 3 / 4 - vsTemp->textEdgeTop - vsTemp->textEdgeBottom) //提示信息还是太长超出范围
            {
                vsTemp->textEdgeY = 2;
                vsTemp->textSize = 160;
            }
        }

        vsTemp->callBack = (ViewCallBack)&_input_comm_callBack;
        vsTemp->enMoveEvent = true;
        view_add(vs, vsTemp, false);

        //取消键
        vsTemp = (View_Struct *)calloc(1, sizeof(View_Struct));
        strcpy(vsTemp->name, "_input_buttonReturnCancel");
        vsTemp->width = VWHT_MATCH * 2;
        vsTemp->height = VWHT_MATCH * 4;
        vsTemp->rType = VRT_BOTTOM | VRT_LEFT;
        vsTemp->shapeType = VST_RECT;
        vsTemp->shape.rect.rad = rad;
        // vsTemp->shape.rect.lineSize = lineSize;
        vsTemp->shapeColorPrint = ViewColor.Gray;
        vsTemp->shapeEdgeTop = vsTemp->shapeEdgeBottom = 1;
        vsTemp->shapeEdgeLeft = vsTemp->shapeEdgeRight = 1;
        vsTemp->text = &ViewSrc.Api_Button_Cancel;
        vsTemp->textSize = contentSize * 10;
        vsTemp->textColor = ViewColor.ButtonValue;
        vsTemp->focusStop = true;
        vsTemp->callBack = (ViewCallBack)&_input_returnOrCancel_callBack;
        vsTemp->enMoveEvent = true;
        view_add(vs, vsTemp, false);
        retView = vsTemp;

        //确认键
        vsTemp = (View_Struct *)calloc(1, sizeof(View_Struct));
        strcpy(vsTemp->name, "_input_buttonEnter");
        vsTemp->width = VWHT_MATCH * 2;
        vsTemp->height = VWHT_MATCH * 4;
        vsTemp->rType = VRT_BOTTOM | VRT_RIGHT;
        vsTemp->shapeType = VST_RECT;
        vsTemp->shape.rect.rad = rad;
        // vsTemp->shape.rect.lineSize = lineSize;
        vsTemp->shapeColorPrint = ViewColor.Green;
        vsTemp->shapeEdgeTop = vsTemp->shapeEdgeBottom = 1;
        vsTemp->shapeEdgeLeft = vsTemp->shapeEdgeRight = 1;
        vsTemp->text = &ViewSrc.Api_Button_Enter;
        vsTemp->textSize = contentSize * 10;
        vsTemp->textColor = ViewColor.ButtonValue;
        vsTemp->focusStop = true;
        vsTemp->callBack = (ViewCallBack)&_input_enter_callBack;
        vsTemp->enMoveEvent = true;
        view_add(vs, vsTemp, false);
    }
    //type 1: 列表中选一个 -- 纵向列表
    else if (candidate &&
             ((value->type == VT_CHAR && candidate->type == VT_STRING) ||
              (value->type == VT_INT && candidate->type == VT_INT_ARRAY) ||
              (value->type == VT_DOUBLE && candidate->type == VT_DOUBLE_ARRAY) ||
              (value->type == VT_STRING && candidate->type == VT_STRING_ARRAY)))
    {
        ibs->type = 1;

        //框 和 label
        vsTemp = (View_Struct *)calloc(1, sizeof(View_Struct));
        strcpy(vsTemp->name, "_input_frameLabel");
        vsTemp->width = VWHT_MATCH;
        vsTemp->height = VWHT_MATCH;
        vsTemp->shapeType = VST_RECT;
        vsTemp->shape.rect.rad = rad;
        vsTemp->shape.rect.lineSize = lineSize;
        vsTemp->shapeColorPrint = ViewColor.Button;
        vsTemp->shapeEdgeBottom = 1;
        vsTemp->shapeEdgeLeft = vsTemp->shapeEdgeRight = 1;
        vsTemp->text = viewValue_init("multiInput_label", VT_STRING, 1, label);
        vsTemp->textSize = labelSize * 10;
        vsTemp->textColor = ViewColor.Label;
        vsTemp->textEdgeTop = lineSize * 2;
        vsTemp->textEdgeLeft = vsTemp->textEdgeRight = 5; //左右保持5的间距
        vsTemp->textSideY = VTST_LEFT_TOP;
        vsTemp->scroll = labelSize / 4; //自动滚动
        vsTemp->callBack = (ViewCallBack)&_input_comm_callBack;
        vsTemp->enMoveEvent = true;
        view_add(vs, vsTemp, false);

        if ((retBool = viewValue_compare(value, candidate, &retInt)) == false)
            retInt = -1;
        switch (candidate->type)
        {
        case VT_STRING:
            if ((valueNum = strlen(candidate->value.String)) > 0)
            {
                vvfArray = (ViewValue_Format **)calloc(valueNum, sizeof(ViewValue_Format *));
                count = 0;
                //是否是删除字符
                if (candidate->value.String[0] == VIEW_DEL_CHAR)
                {
                    vvfArray[0] = &ViewSrc.Api_Del_Char;
                    count = 1;
                }

                for (; count < valueNum; count += 1)
                {
                    if (count == retInt)
                        vvfArray[count] =
                            viewValue_init("_input_char", VT_CHAR, 1, value->value.Char);
                    else
                        vvfArray[count] =
                            viewValue_init("_input_char", VT_CHAR, 1, candidate->value.String[count]);
                }

                ibs->contentSize2 = 56;
            }
            break;
        case VT_INT_ARRAY:
            if ((valueNum = candidate->vSize / sizeof(int)) > 0)
            {
                vvfArray = (ViewValue_Format **)calloc(valueNum, sizeof(ViewValue_Format *));
                for (count = 0; count < valueNum; count += 1)
                {
                    if (count == retInt)
                        vvfArray[count] =
                            viewValue_init("_input_int", VT_INT, 1, value->value.Int);
                    else
                        vvfArray[count] =
                            viewValue_init("_input_int", VT_INT, 1, candidate->value.IntArray[count]);
                }

                ibs->contentSize2 = 56;
            }
            break;
        case VT_DOUBLE_ARRAY:
            if ((valueNum = candidate->vSize / sizeof(double)) > 0)
            {
                vvfArray = (ViewValue_Format **)calloc(valueNum, sizeof(ViewValue_Format *));
                for (count = 0; count < valueNum; count += 1)
                {
                    if (count == retInt)
                        vvfArray[count] =
                            viewValue_init("_input_double", VT_DOUBLE, 1, value->value.Double);
                    else
                        vvfArray[count] =
                            viewValue_init("_input_double", VT_DOUBLE, 1, candidate->value.DoubleArray[count]);
                }

                ibs->contentSize2 = 56;
            }
            break;
        case VT_STRING_ARRAY:
            if ((valueNum = candidate->vSize / sizeof(char *)) > 0)
            {
                intTemp[0] = 0; //当前长度
                intTemp[1] = 0; //最长
                vvfArray = (ViewValue_Format **)calloc(valueNum, sizeof(ViewValue_Format *));
                for (count = 0; count < valueNum; count += 1)
                {
                    if (count == retInt)
                        vvfArray[count] =
                            viewValue_init("_input_string", VT_STRING, 1, value->value.String);
                    else
                        vvfArray[count] =
                            viewValue_init("_input_string", VT_STRING, 1, candidate->value.StringArray[count]);
                    //更新最长
                    intTemp[0] = strlen(vvfArray[count]->value.String);
                    if (intTemp[0] > intTemp[1])
                    {
                        intTemp[1] = intTemp[0];
                        pointTemp = vvfArray[count]->value.String;
                    }
                }

                if (pointTemp)
                {
                    //获取尽可能大的字体
                    ibs->contentSize2 = view_getType(pointTemp, vsParent->absWidth, 0) / 10;
                    if (ibs->contentMinSize > ibs->contentSize2)
                        ibs->contentMinSize = ibs->contentSize2;
                }
            }
            break;
        default:
            break;
        }

        //居中位置标杆
        vsTemp = (View_Struct *)calloc(1, sizeof(View_Struct));
        strcpy(vsTemp->name, "_input_contentMain");
        vsTemp->width = VWHT_MATCH;
        vsTemp->height = ibs->contentSize2 + 4;
        vsTemp->centerX = true;
        vsTemp->centerY = true;
        vsTemp->textSize = ibs->contentSize2 * 10;
        view_add(vs, vsTemp, false);

        //当前值不在候选列表中 把当前值展示在列表最上面
        if (!retBool)
        {
            vsTemp = (View_Struct *)calloc(1, sizeof(View_Struct));
            sprintf(vsTemp->name, "_input_content%d", retInt);
            vsTemp->width = VWHT_MATCH;
            vsTemp->height = ibs->contentSize2 + 4;
            vsTemp->rNumber = VRNT_LAST; //和标杆位置重合
            vsTemp->shapeType = VST_RECT;
            // vsTemp->shape.rect.rad = rad;
            vsTemp->text = viewValue_copy(NULL, value);
            vsTemp->textSize = ibs->contentSize2 * 10;
            vsTemp->textColor = ViewColor.Tips;
            // vsTemp->textEdgeLeft = vsTemp->textEdgeRight = 5;//左右保持5的间距
            vsTemp->scroll = 4; //自动滚动
            vsTemp->bottomLineColor = focus->color;
            vsTemp->callBack = (ViewCallBack)&_input_list_callBack;
            vsTemp->focusCallBackFront = (FocusCallBack)&view_input_focusCallBackFront;
            vsTemp->enMoveEvent = true;
            vsTemp->focusStop = true;
            view_add(vs, vsTemp, false);
            retView = vsTemp;
        }

        //把候选项逐个添加到链表中
        if (vvfArray)
        {
            for (count = 0; count < valueNum; count += 1)
            {
                vsTemp = (View_Struct *)calloc(1, sizeof(View_Struct));
                sprintf(vsTemp->name, "_input_content%d", count);
                vsTemp->width = VWHT_MATCH;
                vsTemp->rNumber = VRNT_LAST;
                //是否是第一个
                if (count == 0 && retBool)
                {
                    vsTemp->rType = 0;
                    vsTemp->rTopBottomErr = (count - retInt) * (ibs->contentMinSize + 4);
                }
                else
                {
                    vsTemp->rType = VRT_BOTTOM;
                    vsTemp->rTopBottomErr = 0;
                }
                //是否是当前
                if (count == retInt)
                {
                    vsTemp->textSize = ibs->contentSize2 * 10;
                    vsTemp->height = ibs->contentSize2 + 4;

                    retView = vsTemp;
                }
                else
                {
                    vsTemp->height = ibs->contentMinSize + 4;
                    vsTemp->textSize = ibs->contentMinSize * 10;
                }
                //透明度
                vsTemp->textColor &= 0xFFFFFF00;
                if (count > retInt)
                    vsTemp->textColor |= (count - retInt > 4) ? 0xFF : ((count - retInt) * 60 + 15);
                else if (count < retInt)
                    vsTemp->textColor |= (retInt - count > 4) ? 0xFF : ((retInt - count) * 60 + 15);
                // vsTemp->shapeType = VST_RECT;
                // vsTemp->shape.rect.rad = rad;
                vsTemp->text = vvfArray[count];
                vsTemp->text->sep = value->sep;
                vsTemp->text->zero = value->zero;
                vsTemp->textColor = ViewColor.Tips;
                // vsTemp->textEdgeLeft = vsTemp->textEdgeRight = 5;//左右保持5的间距
                vsTemp->scroll = 4; //自动滚动
                vsTemp->bottomLineColor = focus->color;
                vsTemp->callBack = (ViewCallBack)&_input_list_callBack;
                vsTemp->focusCallBackFront = (FocusCallBack)&view_input_focusCallBackFront;
                vsTemp->enMoveEvent = true;
                vsTemp->focusStop = true;
                view_add(vs, vsTemp, false);
            }

            free(vvfArray);
        }
    }
    //type 2: 逐个输入 -- 横向列表
    else if (candidate &&
             candidate->type == VT_STRING &&
             strlen(candidate->value.String) > 0 &&
             (value->type == VT_INT ||
              value->type == VT_DOUBLE ||
              value->type == VT_STRING))
    {
        ibs->type = 2;
        ibs->contentSize2 = 64;

        //框 和 label
        vsTemp = (View_Struct *)calloc(1, sizeof(View_Struct));
        strcpy(vsTemp->name, "_input_frameLabel");
        vsTemp->width = VWHT_MATCH;
        vsTemp->height = VWHT_MATCH * 4 - 3;
        vsTemp->shapeType = VST_RECT;
        vsTemp->shape.rect.rad = rad;
        vsTemp->shape.rect.lineSize = lineSize;
        vsTemp->shapeColorPrint = ViewColor.Button;
        vsTemp->shapeEdgeLeft = vsTemp->shapeEdgeRight = 1;
        vsTemp->text = viewValue_init("multiInput_label", VT_STRING, 1, label);
        vsTemp->textSize = labelSize * 10;
        vsTemp->textColor = ViewColor.Label;
        vsTemp->textEdgeTop = lineSize * 2;
        vsTemp->textEdgeLeft = vsTemp->textEdgeRight = 5; //左右保持5的间距
        vsTemp->textSideY = VTST_LEFT_TOP;
        vsTemp->scroll = labelSize / 4; //自动滚动
        vsTemp->callBack = (ViewCallBack)&_input_comm_callBack;
        vsTemp->enMoveEvent = true;
        view_add(vs, vsTemp, false);

        //取消键
        vsTemp = (View_Struct *)calloc(1, sizeof(View_Struct));
        strcpy(vsTemp->name, "_input_buttonReturnCancel");
        vsTemp->width = VWHT_MATCH * 2;
        vsTemp->height = VWHT_MATCH * 4;
        vsTemp->rType = VRT_BOTTOM | VRT_LEFT;
        vsTemp->shapeType = VST_RECT;
        vsTemp->shape.rect.rad = rad;
        // vsTemp->shape.rect.lineSize = lineSize;
        vsTemp->shapeColorPrint = ViewColor.Gray;
        vsTemp->shapeEdgeTop = vsTemp->shapeEdgeBottom = 1;
        vsTemp->shapeEdgeLeft = vsTemp->shapeEdgeRight = 1;
        vsTemp->text = &ViewSrc.Api_Button_Cancel;
        vsTemp->textSize = contentSize * 10;
        vsTemp->textColor = ViewColor.ButtonValue;
        vsTemp->focusStop = true;
        vsTemp->callBack = (ViewCallBack)&_input_returnOrCancel_callBack;
        vsTemp->enMoveEvent = true;
        view_add(vs, vsTemp, false);
        retView = vsTemp;

        //居中位置标杆
        vsTemp = (View_Struct *)calloc(1, sizeof(View_Struct));
        strcpy(vsTemp->name, "_input_contentMain");
        vsTemp->width = ibs->contentSize2 / 2 + 4;
        vsTemp->height = ibs->contentSize2 + 4;
        vsTemp->centerX = true;
        // vsTemp->centerY = true;
        vsTemp->rTopBottomErr = (156 - ibs->contentSize2 + 4) / 2;
        vsTemp->text = viewValue_copy(NULL, candidate);
        vsTemp->textSize = ibs->contentSize2 * 10;
        vsTemp->bottomLine = 2;
        vsTemp->bottomLineColor = focus->color;
        view_add(vs, vsTemp, false);

        //把被输入的数据转为字符串 指针给 valueStringPoint
        valueStringPoint = NULL;
        switch (value->type)
        {
        case VT_INT:
            sprintf(numberValueString, "%d", value->value.Int);
            valueStringPoint = numberValueString;
            break;
        case VT_DOUBLE:
            if (value->zero > 0)
                strDemo[2] = (value->zero % 10) + '0';
            sprintf(numberValueString, strDemo, value->value.Double);
            valueStringPoint = numberValueString;
            break;
        case VT_STRING:
            valueStringPoint = value->value.String;
            break;
        default:
            break;
        }
        //从中间向右逐个排列
        if (valueStringPoint && valueStringPoint[0])
        {
            //第一个居中
            vsTemp = (View_Struct *)calloc(1, sizeof(View_Struct));
            strcpy(vsTemp->name, "_input_content0");
            vsTemp->width = ibs->contentSize2 / 2 + 4;
            vsTemp->height = ibs->contentSize2 + 4;
            vsTemp->rNumber = VRNT_LAST;
            vsTemp->text = viewValue_init("_input_content0", VT_CHAR, 1, valueStringPoint[0]);
            vsTemp->textSize = ibs->contentSize2 * 10;
            vsTemp->textColor = ViewColor.Tips;
            vsTemp->bottomLineColor = focus->color;
            vsTemp->focusStop = true;
            vsTemp->callBack = (ViewCallBack)&_input_list2_callBack;
            vsTemp->focusCallBackFront = (FocusCallBack)&view_input_focusCallBackFront;
            vsTemp->enMoveEvent = true;
            view_add(vs, vsTemp, false);

            for (count = 1; valueStringPoint[count]; count += 1)
            {
                vsTemp = (View_Struct *)calloc(1, sizeof(View_Struct));
                sprintf(vsTemp->name, "_input_content%d", count);
                vsTemp->width = ibs->contentMinSize / 2 + 4;
                vsTemp->height = ibs->contentSize2 + 4;
                vsTemp->rNumber = VRNT_LAST;
                vsTemp->rType = VRT_RIGHT;
                vsTemp->text = viewValue_init(vsTemp->name, VT_CHAR, 1, valueStringPoint[count]);
                vsTemp->textSize = ibs->contentMinSize * 10;
                vsTemp->textColor = ViewColor.Tips;
                vsTemp->bottomLineColor = focus->color;
                vsTemp->focusStop = true;
                vsTemp->callBack = (ViewCallBack)&_input_list2_callBack;
                vsTemp->focusCallBackFront = (FocusCallBack)&view_input_focusCallBackFront;
                vsTemp->enMoveEvent = true;
                view_add(vs, vsTemp, false);
            }
            if (candidate->value.String[0] == VIEW_DEL_CHAR &&      //候选列表存在 删除字符
                valueStringPoint[0] != VIEW_DEL_CHAR &&             //被编辑的字符串不为空
                (astrict == 0 || (astrict > 0 && count < astrict))) //现在已填入的长度还没有超出限制长度
            {
                //留个空位
                vsTemp = (View_Struct *)calloc(1, sizeof(View_Struct));
                sprintf(vsTemp->name, "_input_content%d", count);
                vsTemp->width = ibs->contentMinSize / 2 + 4;
                vsTemp->height = ibs->contentSize2 + 4;
                vsTemp->rNumber = VRNT_LAST;
                vsTemp->rType = VRT_RIGHT;
                vsTemp->text = viewValue_init(vsTemp->name, VT_CHAR, 1, VIEW_DEL_CHAR);
                vsTemp->textSize = ibs->contentMinSize * 10;
                vsTemp->textColor = ViewColor.Tips;
                vsTemp->bottomLineColor = focus->color;
                vsTemp->focusStop = true;
                vsTemp->callBack = (ViewCallBack)&_input_list2_callBack;
                vsTemp->focusCallBackFront = (FocusCallBack)&view_input_focusCallBackFront;
                vsTemp->enMoveEvent = true;
                view_add(vs, vsTemp, false);
            }
        }
        else // if(candidate->value.String[0] == VIEW_DEL_CHAR)
        {
            //留个空位
            vsTemp = (View_Struct *)calloc(1, sizeof(View_Struct));
            sprintf(vsTemp->name, "_input_content%d", 0);
            vsTemp->width = ibs->contentSize2 / 2 + 4;
            vsTemp->height = ibs->contentSize2 + 4;
            vsTemp->rNumber = VRNT_LAST;
            vsTemp->text = viewValue_init(vsTemp->name, VT_CHAR, 1, candidate->value.String[0]);
            vsTemp->textSize = ibs->contentSize2 * 10;
            vsTemp->textColor = ViewColor.Tips;
            vsTemp->bottomLineColor = focus->color;
            vsTemp->focusStop = true;
            vsTemp->callBack = (ViewCallBack)&_input_list2_callBack;
            vsTemp->focusCallBackFront = (FocusCallBack)&view_input_focusCallBackFront;
            vsTemp->enMoveEvent = true;
            view_add(vs, vsTemp, false);
        }

        //确认键
        vsTemp = (View_Struct *)calloc(1, sizeof(View_Struct));
        strcpy(vsTemp->name, "_input_buttonEnter");
        vsTemp->width = VWHT_MATCH * 2;
        vsTemp->height = VWHT_MATCH * 4;
        vsTemp->rType = VRT_BOTTOM | VRT_RIGHT;
        vsTemp->shapeType = VST_RECT;
        vsTemp->shape.rect.rad = rad;
        // vsTemp->shape.rect.lineSize = lineSize;
        vsTemp->shapeColorPrint = ViewColor.Green;
        vsTemp->shapeEdgeTop = vsTemp->shapeEdgeBottom = 1;
        vsTemp->shapeEdgeLeft = vsTemp->shapeEdgeRight = 1;
        vsTemp->text = &ViewSrc.Api_Button_Enter;
        vsTemp->textSize = contentSize * 10;
        vsTemp->textColor = ViewColor.ButtonValue;
        vsTemp->focusStop = true;
        vsTemp->callBack = (ViewCallBack)&_input_enter_callBack;
        vsTemp->enMoveEvent = true;
        view_add(vs, vsTemp, false);
    }

    //删掉之前驻留的
    if (vsParent->jumpView && strstr(vsParent->jumpView->name, "_input_"))
    {
        vsTemp = vsParent->jumpView->jumpView;
        while (vsTemp)
        {
            view_delete(vsTemp);
            vsTemp = vsTemp->jumpView;
        }
        view_delete(vsParent->jumpView);
    }

    vs->backView = backView;
    vs->parent = vsParent;
    vsParent->jumpView = vs;
    vsParent->jumpViewOn = true;

    if (retView)
        view_focusJump(focus, retView);
    else
    {
        view_focusJump(focus, vs);
        view_focusEvent(focus, VFE_ENTER);
    }
}

//根据txt长度自动推荐字体,最小返回280
int view_getType(char *text, int width, int xEdge)
{
    int retValueType = 240;
    int retWidth;
    retValueType = 480;
    retWidth = ttf_getSizeByUtf8(ViewTTF, text, 480, xEdge, NULL);
    if (retWidth > width)
    {
        retValueType = 400;
        retWidth = ttf_getSizeByUtf8(ViewTTF, text, 400, xEdge, NULL);
        if (retWidth > width)
        {
            retValueType = 320;
            retWidth = ttf_getSizeByUtf8(ViewTTF, text, 320, xEdge, NULL);
            if (retWidth > width)
            {
                retValueType = 280;
            }
        }
    }
    return retValueType;
}