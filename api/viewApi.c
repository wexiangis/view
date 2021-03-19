
#include <sys/time.h>
#include <unistd.h>

#include "viewApi.h"

void view_delayms(uint32_t ms)
{
    struct timeval delay;
    if (ms > 1000)
    {
        delay.tv_sec = ms / 1000;
        delay.tv_usec = (ms % 1000) * 1000;
    }
    else
    {
        delay.tv_sec = 0;
        delay.tv_usec = ms * 1000;
    }
    select(0, NULL, NULL, NULL, &delay);
}

int view_tickMs(void)
{
    struct timeval tv = {0};
    gettimeofday(&tv, NULL);
    return (int)(tv.tv_sec * 1000 + tv.tv_usec / 1000);
}

struct tm *view_time(void)
{
    static time_t now;
    time(&now);
    return localtime(&now);
}

//根据RGB内存块制作二维指针数组
uint8_t ***bmp_mapInit(uint8_t *pic, int width, int height, int pb)
{
    uint8_t ***map;
    int xC, yC;
    if (pic == NULL || width < 1 || height < 1 || pb < 1)
        return NULL;
    map = (uint8_t ***)calloc(height, sizeof(uint8_t **));
    for (yC = 0; yC < height; yC++)
    {
        map[yC] = (uint8_t **)calloc(width, sizeof(uint8_t *));
        for (xC = 0; xC < width; xC++)
            map[yC][xC] = &pic[yC * width * pb + xC * pb];
    }
    return map;
}
void bmp_mapRelease(uint8_t ***picMap, int width, int height)
{
    int i;
    if (picMap == NULL)
        return;
    for (i = 0; i < height; i++)
        free(picMap[i]);
    free(picMap);
}

//公共顶view
static View_Struct ViewCommonParent = {
    .name = "commonParent",
    .width = VIEW_X_SIZE,
    .height = VIEW_Y_SIZE,
    .absWidth = VIEW_X_SIZE,
    .absHeight = VIEW_Y_SIZE,
    .absXY = {{0, 0}, {VIEW_X_END, VIEW_Y_END}},
    .drawSync = 1,
};

//纵向输入列表最后选中项的序号(从0数起)
static int viewInputSelectNumber = 0;

//垃圾桶
static View_Struct viewTrash = {
    .name = "viewTrash",
};

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

        //颜色资源释放
        if (vsThis->backGroundColor && !viewColor_compare(vsThis->backGroundColor))
        {
            viewValue_release(vsThis->backGroundColor);
            vsThis->backGroundColor = NULL;
        }
        if (vsThis->shapeColorPrint && !viewColor_compare(vsThis->shapeColorPrint))
        {
            viewValue_release(vsThis->shapeColorPrint);
            vsThis->shapeColorPrint = NULL;
        }
        if (vsThis->shapeColorBackground && !viewColor_compare(vsThis->shapeColorBackground))
        {
            viewValue_release(vsThis->shapeColorBackground);
            vsThis->shapeColorBackground = NULL;
        }
        if (vsThis->picReplaceColorBy && !viewColor_compare(vsThis->picReplaceColorBy))
        {
            viewValue_release(vsThis->picReplaceColorBy);
            vsThis->picReplaceColorBy = NULL;
        }
        if (vsThis->valueColor && !viewColor_compare(vsThis->valueColor))
        {
            viewValue_release(vsThis->valueColor);
            vsThis->valueColor = NULL;
        }
        if (vsThis->bottomLineColor && !viewColor_compare(vsThis->bottomLineColor))
        {
            viewValue_release(vsThis->bottomLineColor);
            vsThis->bottomLineColor = NULL;
        }
        if (vsThis->sideColor && !viewColor_compare(vsThis->sideColor))
        {
            viewValue_release(vsThis->sideColor);
            vsThis->sideColor = NULL;
        }
        //数据资源释放
        if (vsThis->value && !viewSource_compare(vsThis->value))
        {
            viewValue_release(vsThis->value);
            vsThis->value = NULL;
        }
        if (vsThis->valueBackup && !viewSource_compare(vsThis->valueBackup))
        {
            viewValue_release(vsThis->valueBackup);
            vsThis->valueBackup = NULL;
        }
        //图片内存释放
        if (vsThis->picMap)
        {
            bmp_mapRelease(vsThis->picMap, vsThis->picWidth, vsThis->picHeight);
            vsThis->picMap = NULL;
        }
        if (vsThis->pic)
        {
            free(vsThis->pic);
            vsThis->pic = NULL;
        }
        //文字输出缓冲区释放
        if (vsThis->valuePrint.valueOutput)
        {
            free(vsThis->valuePrint.valueOutput);
            vsThis->valuePrint.valueOutput = NULL;
        }
        //含有子 view
        if (vsThis->view)
        {
            if (vsNext == NULL)
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
 *      color : 颜色
 *      xStart :
 *      yStart :
 *      rad : 半径(外经)
 *      size : 半径向里画环的圈数  0:完全填充, >0: 画环
 *  返回: 无
 */
void view_circle(
    int color,
    int xStart, int yStart,
    int rad, int size,
    float alpha,
    int minX, int minY,
    int maxX, int maxY)
{
    int circle_a, circle_b;
    int circle_di;
    int circle_rad = rad;
    int circle_size = size;
    //
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
    if (alpha == 0)
    {
        for (yC = 0; yC < xySize; yC++)
        {
            yC2 = yS + yC;
            if (yC2 < minY || yC2 > maxY)
                continue;
            for (xC = 0; xC < xySize; xC++)
            {
                xC2 = xS + xC;
                if (xC2 < minX || xC2 > maxX)
                    continue;
                else if (memBUff[yC][xC])
                    PRINT_DOT(xC2, yC2, color);
            }
        }
    }
    else
    {
        for (yC = 0; yC < xySize; yC++)
        {
            yC2 = yS + yC;
            if (yC2 < minY || yC2 > maxY)
                continue;
            for (xC = 0; xC < xySize; xC++)
            {
                xC2 = xS + xC;
                if (xC2 < minX || xC2 > maxX)
                    continue;
                else if (memBUff[yC][xC])
                    PRINT_DOT2(xC2, yC2, color, alpha);
            }
        }
    }
}

/*
 *  功能: 画圆环, 扇形, 扇形圆环
 *  参数:
 *      color : 颜色
 *      xStart :
 *      yStart :
 *      rad : 半径(外经)
 *      size : 半径向里画环的圈数  0:完全填充, >0: 画环
 *      div : 把圆拆分多少分  0或1 : 画整圆, >1: 拆分多份(此时 divStart, divEnd 参数有效)
 *      divStart, divEnd : 只画 divStart ~ divEnd 的圆环
 *  返回: 无
 *  说明:
 */
void view_circleLoop(int color, int xStart, int yStart, int rad, int size, int div, int divStart, int divEnd)
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
                PRINT_DOT(xStart + circle_a, yStart - circle_b, color); //1
                PRINT_DOT(xStart + circle_b, yStart - circle_a, color); //2
                PRINT_DOT(xStart + circle_b, yStart + circle_a, color); //3
                PRINT_DOT(xStart + circle_a, yStart + circle_b, color); //4
                PRINT_DOT(xStart - circle_a, yStart + circle_b, color); //5
                PRINT_DOT(xStart - circle_b, yStart + circle_a, color); //6
                PRINT_DOT(xStart - circle_b, yStart - circle_a, color); //7
                PRINT_DOT(xStart - circle_a, yStart - circle_b, color); //8
                //if(circle_size > 1)
                {
                    PRINT_DOT(xStart + circle_a, yStart - circle_b + 1, color); //补点
                    PRINT_DOT(xStart + circle_b - 1, yStart - circle_a, color); //补点
                    PRINT_DOT(xStart + circle_b - 1, yStart + circle_a, color); //补点
                    PRINT_DOT(xStart + circle_a, yStart + circle_b - 1, color); //补点
                    PRINT_DOT(xStart - circle_a, yStart + circle_b - 1, color); //补点
                    PRINT_DOT(xStart - circle_b + 1, yStart + circle_a, color); //补点
                    PRINT_DOT(xStart - circle_b + 1, yStart - circle_a, color); //补点
                    PRINT_DOT(xStart - circle_a, yStart - circle_b + 1, color); //补点
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
            for (i = 0, sumCount2 = sumCount - 1; i < sumCount; i++)
            {
                circle_a = intArray[i][0];
                circle_b = intArray[i][1];

                if (circle_a > rangeArray[0][0] && circle_a < rangeArray[0][1])
                {
                    PRINT_DOT(xStart + circle_a, yStart - circle_b, color);     //1
                    PRINT_DOT(xStart + circle_a, yStart - circle_b + 1, color); //补点
                }
                if (sumCount2 - circle_a > rangeArray[1][0] && sumCount2 - circle_a < rangeArray[1][1])
                {
                    PRINT_DOT(xStart + circle_b, yStart - circle_a, color);     //2
                    PRINT_DOT(xStart + circle_b - 1, yStart - circle_a, color); //补点
                }
                if (circle_a > rangeArray[2][0] && circle_a < rangeArray[2][1])
                {
                    PRINT_DOT(xStart + circle_b, yStart + circle_a, color);     //3
                    PRINT_DOT(xStart + circle_b - 1, yStart + circle_a, color); //补点
                }
                if (sumCount2 - circle_a > rangeArray[3][0] && sumCount2 - circle_a < rangeArray[3][1])
                {
                    PRINT_DOT(xStart + circle_a, yStart + circle_b, color);     //4
                    PRINT_DOT(xStart + circle_a, yStart + circle_b - 1, color); //补点
                }
                if (circle_a > rangeArray[4][0] && circle_a < rangeArray[4][1])
                {
                    PRINT_DOT(xStart - circle_a, yStart + circle_b, color);     //5
                    PRINT_DOT(xStart - circle_a, yStart + circle_b - 1, color); //补点
                }
                if (sumCount2 - circle_a > rangeArray[5][0] && sumCount2 - circle_a < rangeArray[5][1])
                {
                    PRINT_DOT(xStart - circle_b, yStart + circle_a, color);     //6
                    PRINT_DOT(xStart - circle_b + 1, yStart + circle_a, color); //补点
                }
                if (circle_a > rangeArray[6][0] && circle_a < rangeArray[6][1])
                {
                    PRINT_DOT(xStart - circle_b, yStart - circle_a, color);     //7
                    PRINT_DOT(xStart - circle_b + 1, yStart - circle_a, color); //补点
                }
                if (sumCount2 - circle_a > rangeArray[7][0] && sumCount2 - circle_a < rangeArray[7][1])
                {
                    PRINT_DOT(xStart - circle_a, yStart - circle_b, color);     //8
                    PRINT_DOT(xStart - circle_a, yStart - circle_b + 1, color); //补点
                }
            }
        }
    }
    //输出
}

/*
 *  功能: 画点函数
 *  参数:
 *      color : 颜色
 *      xStart :
 *      yStart :
 *      size : 1~2
 *  返回: 无
 */
void view_dot(int color, int xStart, int yStart, int size, float alpha)
{
    if (size == 1)
    {
        if (alpha > 0)
            PRINT_DOT2(xStart, yStart, color, alpha);
        else
            PRINT_DOT(xStart, yStart, color);
    }
    else if (size == 2)
    {
        if (alpha > 0)
        {
            //mid
            PRINT_DOT2(xStart, yStart, color, alpha);
            //up
            PRINT_DOT2(xStart, yStart + 1, color, alpha);
            //down
            PRINT_DOT2(xStart, yStart - 1, color, alpha);
            //left
            PRINT_DOT2(xStart + 1, yStart, color, alpha);
            //right
            PRINT_DOT2(xStart - 1, yStart, color, alpha);
        }
        else
        {
            //mid
            PRINT_DOT(xStart, yStart, color);
            //up
            PRINT_DOT(xStart, yStart + 1, color);
            //down
            PRINT_DOT(xStart, yStart - 1, color);
            //left
            PRINT_DOT(xStart + 1, yStart, color);
            //right
            PRINT_DOT(xStart - 1, yStart, color);
        }
    }
    else if (size > 2)
        view_circle(color, xStart, yStart, size, 0, alpha, 0, 0, 9999, 9999);
}

//-------------------- 线 --------------------

/*
 *  功能: 指定起止坐标, 返回两点间画线的Y坐标数组
 *  参数:
 *      xStart, yStart, xEnd, yEnd : 起止坐标
 *      dotX, dotY : Y坐标数组起始地址, 需要自己先分配好内存再传入
 *  返回: 点数
 */
int view_getDot(int xStart, int yStart, int xEnd, int yEnd, int *dotX, int *dotY)
{
    unsigned short t;
    int xerr = 0, yerr = 0;
    int delta_x, delta_y;
    int distance;
    int incx, incy, xCount, yCount;

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

    for (t = 0; t <= distance + 1; t++) //画线输出
    {
        *dotX++ = xCount;
        *dotY++ = yCount;

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
    return distance + 2;
}

/*
 *  功能: 划线函数
 *  参数:
 *      xStart, yStart, xEnd, yEnd : 起止坐标
 *      size : 线宽
 *      space : 不为0时画的是虚线, 其值代表虚线的点密度
 *  返回: 无
 */
void view_line(int color, int xStart, int yStart, int xEnd, int yEnd, int size, int space, float alpha)
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
        printf("%d - %d , %d - %d , %d too large\n",
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

    for (t = 0; t <= distance + 1; t++) //画线输出
    {
        if (spaceVal == 0 || spaceCount < 0)
        {
            spaceCount += 1;
            view_dot(color, xCount, yCount, size, alpha);
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
 *      color : 颜色
 *      xStart, yStart, xEnd, yEnd : 起止坐标
 *      size : 线宽
 *      rad : 圆角半径
 *      alpha : 0:实心填充  1:半透填充
 *      minY, maxY : 超出上下 Y 坐标部分不绘制
 *  返回: 无
 */
void view_rectangle(
    int color,
    int xStart, int yStart,
    int xEnd, int yEnd,
    int size, int rad, float alpha,
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
    for (i = 0; i < ySize; i++)
        outPutArray[i] = (char *)calloc(xSize, sizeof(char));

    //腰两竖
    if (ySize > circle_rad)
    {
        if (sSize > 0)
        {
            for (i = circle_rad; i < ySize - circle_rad; i++)
            {
                for (j = 0; j < sSize; j++)
                    outPutArray[i][j] = 1;
                for (j = xSize - sSize; j < xSize; j++)
                    outPutArray[i][j] = 1;
            }
        }
        else
        {
            for (i = circle_rad; i < ySize - circle_rad; i++)
            {
                for (j = 0; j < xSize; j++)
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
        for (i = 0; i < temp; i++)
        {
            for (j = circle_rad; j < xSize - circle_rad; j++)
                outPutArray[i][j] = 1;
        }
        //下横
        for (i = ySize - temp; i < ySize; i++)
        {
            for (j = circle_rad; j < xSize - circle_rad; j++)
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
                            for (; xC < k; xC++)
                                outPutArray[yC][xC] = 1;
                        }
                        if (circle_localCentre[3][1] + circle_b < circle_compare[3]) //4, 5  y
                        {
                            circle_compare[3] = circle_localCentre[3][1] + circle_b;

                            xC = circle_localCentre[2][0] - circle_a; //5 x
                            k = circle_localCentre[3][0] + circle_a;  //4 x
                            yC = circle_compare[3];
                            for (; xC < k; xC++)
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
                            for (; xC < k; xC++)
                                outPutArray[yC][xC] = 1;
                        }
                        if (circle_localCentre[3][1] + circle_a > circle_compare[2]) //3, 6  y
                        {
                            circle_compare[2] = circle_localCentre[3][1] + circle_a;

                            xC = circle_localCentre[2][0] - circle_b; //6 x
                            k = circle_localCentre[3][0] + circle_b;  //3 x
                            yC = circle_compare[2];
                            for (; xC <= k; xC++)
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
    if (alpha)
    {
        for (i = 0, yC = yS; i < ySize; i++, yC++)
        {
            if (yC < minY || yC > maxY)
                continue;
            for (j = 0, xC = xS, k; j < xSize; j++, xC++)
            {
                if (xC < minX || xC > maxX)
                    continue;
                else if (outPutArray[i][j])
                    PRINT_DOT2(xC, yC, color, alpha);
            }
        }
    }
    else
    {
        for (i = 0, yC = yS; i < ySize; i++, yC++)
        {
            if (yC < minY || yC > maxY)
                continue;
            for (j = 0, xC = xS, k; j < xSize; j++, xC++)
            {
                if (xC < minX || xC > maxX)
                    continue;
                else if (outPutArray[i][j])
                    PRINT_DOT(xC, yC, color);
            }
        }
    }

    for (i = 0; i < ySize; i++)
        free(outPutArray[i]);
    free(outPutArray);
}

/*
 *  用rgb数据填充矩形
 */
void view_rectangle_padding(
    uint8_t *pic,
    int xStart, int yStart,
    int xEnd, int yEnd,
    int picWidth, int picHeight,
    int picPB,
    bool picUseReplaceColor,
    int picReplaceColor,
    int picReplaceColorBy,
    bool picUseInvisibleColor,
    int picInvisibleColor,
    float alpha,
    int xMin, int yMin,
    int xMax, int yMax)
{
    int xS = xStart, yS = yStart, xE = xEnd, yE = yEnd;
    int xC, yC, temp;

    uint8_t replaceColor[3] = {0};
    uint8_t invisibleColor[3] = {0};

    int xPC, yPC;
    float xPPC, yPPC, xCVal, yCVal;

    uint8_t *charP;

    if (pic == NULL)
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

    xCVal = (float)picWidth / (xE - xS);
    yCVal = (float)picHeight / (yE - yS);

    if (picUseReplaceColor)
    {
        replaceColor[0] = (picReplaceColor >> 16) & 0xFF;
        replaceColor[1] = (picReplaceColor >> 8) & 0xFF;
        replaceColor[2] = picReplaceColor & 0xFF;
    }
    if (picUseInvisibleColor)
    {
        invisibleColor[0] = (picInvisibleColor >> 16) & 0xFF;
        invisibleColor[1] = (picInvisibleColor >> 8) & 0xFF;
        invisibleColor[2] = picInvisibleColor & 0xFF;
    }

    if (alpha == 0)
    {
        for (yC = yS, yPC = 0, yPPC = 0; yC < yE; yC++, yPPC += yCVal)
        {
            if (yC < yMin)
                continue;
            else if (yC > yMax)
                break;

            for (xC = xS, yPC = yPPC, xPC = 0, xPPC = 0; xC < xE; xC++, xPPC += xCVal)
            {
                if (xC < xMin)
                    continue;
                else if (xC > xMax)
                    break;

                xPC = xPPC;
                charP = &pic[yPC * picWidth * picPB + xPC * picPB];

                if (picUseReplaceColor &&
                    charP[0] == replaceColor[0] &&
                    charP[1] == replaceColor[1] &&
                    charP[2] == replaceColor[2])
                    PRINT_DOT(xC, yC, picReplaceColorBy);
                else if (picUseInvisibleColor &&
                         charP[0] == invisibleColor[0] &&
                         charP[1] == invisibleColor[1] &&
                         charP[2] == invisibleColor[2])
                    ;
                else
                    PRINT_DOT(xC, yC, ((charP[0] << 16) | (charP[1] << 8) | charP[2]));
            }
        }
    }
    else
    {
        for (yC = yS, yPC = 0, yPPC = 0; yC < yE; yC++, yPPC += yCVal)
        {
            if (yC < yMin)
                continue;
            else if (yC > yMax)
                break;

            for (xC = xS, yPC = yPPC, xPC = 0, xPPC = 0; xC < xE; xC++, xPPC += xCVal)
            {
                if (xC < xMin)
                    continue;
                else if (xC > xMax)
                    break;

                xPC = xPPC;
                charP = &pic[yPC * picWidth * picPB + xPC * picPB];

                if (picUseReplaceColor &&
                    charP[0] == replaceColor[0] &&
                    charP[1] == replaceColor[1] &&
                    charP[2] == replaceColor[2])
                    PRINT_DOT(xC, yC, picReplaceColorBy);
                else if (picUseInvisibleColor &&
                         charP[0] == invisibleColor[0] &&
                         charP[1] == invisibleColor[1] &&
                         charP[2] == invisibleColor[2])
                    ;
                else
                    PRINT_DOT2(xC, yC, ((charP[0] << 16) | (charP[1] << 8) | charP[2]), alpha);
            }
        }
    }
}

/*
 *  功能: 画矩形并填充图片
 *  参数:
 *      pic : 图片数据
 *      xStart, yStart, xEnd, yEnd : 起止坐标
 *      pW : 像素字节数
 *  返回: 无
 */
void view_rectangle_padding2(
    uint8_t ***picMap,
    int xStart, int yStart,
    int xEnd, int yEnd,
    int picWidth, int picHeight,
    int picPB,
    bool picUseReplaceColor,
    int picReplaceColor,
    int picReplaceColorBy,
    bool picUseInvisibleColor,
    int picInvisibleColor,
    float alpha,
    int xMin, int yMin,
    int xMax, int yMax)
{
    int xS = xStart, yS = yStart, xE = xEnd, yE = yEnd;
    int xC, yC, temp;

    uint8_t replaceColor[3] = {0};
    uint8_t invisibleColor[3] = {0};

    int xPC, yPC;
    float xPPC, yPPC, xCVal, yCVal;

    if (picMap == NULL)
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

    xCVal = (float)picWidth / (xE - xS);
    yCVal = (float)picHeight / (yE - yS);

    if (picUseReplaceColor)
    {
        replaceColor[0] = (picReplaceColor >> 16) & 0xFF;
        replaceColor[1] = (picReplaceColor >> 8) & 0xFF;
        replaceColor[2] = picReplaceColor & 0xFF;
    }
    if (picUseInvisibleColor)
    {
        invisibleColor[0] = (picInvisibleColor >> 16) & 0xFF;
        invisibleColor[1] = (picInvisibleColor >> 8) & 0xFF;
        invisibleColor[2] = picInvisibleColor & 0xFF;
    }

    if (alpha == 0)
    {
        for (yC = yS, yPC = 0, yPPC = 0; yC < yE; yC++, yPPC += yCVal)
        {
            if (yC < yMin)
                continue;
            else if (yC > yMax)
                break;

            for (xC = xS, yPC = yPPC, xPC = 0, xPPC = 0; xC < xE; xC++, xPPC += xCVal)
            {
                if (xC < xMin)
                    continue;
                else if (xC > xMax)
                    break;

                xPC = xPPC;

                if (picUseReplaceColor &&
                    picMap[yPC][xPC][0] == replaceColor[0] &&
                    picMap[yPC][xPC][1] == replaceColor[1] &&
                    picMap[yPC][xPC][2] == replaceColor[2])
                    PRINT_DOT(xC, yC, picReplaceColorBy);
                else if (picUseInvisibleColor &&
                         picMap[yPC][xPC][0] == invisibleColor[0] &&
                         picMap[yPC][xPC][1] == invisibleColor[1] &&
                         picMap[yPC][xPC][2] == invisibleColor[2])
                    ;
                else
                    PRINT_DOT(xC, yC, ((picMap[yPC][xPC][0] << 16) | (picMap[yPC][xPC][1] << 8) | picMap[yPC][xPC][2]));
            }
        }
    }
    else
    {
        for (yC = yS, yPC = 0, yPPC = 0; yC < yE; yC++, yPPC += yCVal)
        {
            if (yC < yMin)
                continue;
            else if (yC > yMax)
                break;

            for (xC = xS, yPC = yPPC, xPC = 0, xPPC = 0; xC < xE; xC++, xPPC += xCVal)
            {
                if (xC < xMin)
                    continue;
                else if (xC > xMax)
                    break;

                xPC = xPPC;

                if (picUseReplaceColor &&
                    picMap[yPC][xPC][0] == replaceColor[0] &&
                    picMap[yPC][xPC][1] == replaceColor[1] &&
                    picMap[yPC][xPC][2] == replaceColor[2])
                    PRINT_DOT2(xC, yC, picReplaceColorBy, alpha);
                else if (picUseInvisibleColor &&
                         picMap[yPC][xPC][0] == invisibleColor[0] &&
                         picMap[yPC][xPC][1] == invisibleColor[1] &&
                         picMap[yPC][xPC][2] == invisibleColor[2])
                    ;
                else
                    PRINT_DOT2(xC, yC, ((picMap[yPC][xPC][0] << 16) | (picMap[yPC][xPC][1] << 8) | picMap[yPC][xPC][2]), alpha);
            }
        }
    }
}

//-------------------- 平行四边形 --------------------

/*
 *  功能: 画平行四边形
 *  参数:
 *      color : 颜色
 *      xStart, yStart, xEnd, yEnd : 起止坐标   //平行四边形 左上 和 右下 的坐标
 *      size : 线宽
 *      width : 平行四边形上边长度
 *      alpha : 0:实心填充  1:半透填充
 *      minY, maxY : 超出上下 Y 坐标部分不绘制
 *  返回: 无
 */
void view_parallelogram(
    int color,
    int xStart, int yStart,
    int xEnd, int yEnd,
    int size, int width,
    float alpha,
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

    for (t = 0; t <= distance + 1; t++) //画线输出
    {
        // xCount, yCount
        if (yCount < minY || yCount > maxY)
            ;
        else
        {
            if (alpha)
            {
                for (xC = xCount, yC = yCount; xC <= xCount + width; xC++)
                {
                    if (xC < minX || xC > maxX)
                        continue;
                    if (sSize == 0)
                        PRINT_DOT2(xC, yC, color, alpha);
                    else if (yC < yStart + sSize || xC < xCount + sSize || yC > yEnd2 - sSize || xC > xCount + width - sSize)
                        PRINT_DOT2(xC, yC, color, alpha);
                }
            }
            else
            {
                for (xC = xCount, yC = yCount; xC <= xCount + width; xC++)
                {
                    if (xC < minX || xC > maxX)
                        continue;
                    if (sSize == 0)
                        PRINT_DOT(xC, yC, color);
                    else if (yC < yStart + sSize || xC < xCount + sSize || yC > yEnd2 - sSize || xC > xCount + width - sSize)
                        PRINT_DOT(xC, yC, color);
                }
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
        if (alpha)
        {
            for (xC = xEnd2, yC = yEnd2; xC <= xCount + width; xC++)
            {
                if (xC < minX || xC > maxX)
                    continue;
                if (sSize == 0)
                    PRINT_DOT2(xC, yC, color, alpha);
                else if (yC < yStart + sSize || xC < xCount + sSize ||
                         yC > yEnd2 - sSize || xC > xCount + width - sSize)
                    PRINT_DOT2(xC, yC, color, alpha);
            }
        }
        else
        {
            for (xC = xEnd2, yC = yEnd2; xC <= xCount + width; xC++)
            {
                if (xC < minX || xC > maxX)
                    continue;
                if (sSize == 0)
                    PRINT_DOT(xC, yC, color);
                else if (yC < yStart + sSize || xC < xCount + sSize ||
                         yC > yEnd2 - sSize || xC > xCount + width - sSize)
                    PRINT_DOT(xC, yC, color);
            }
        }
    }
}

//-------------------- 写字符串 --------------------

/*
 *  根据 ttf_map 画点阵
 */
void view_printBitMap(
    int fColor, int bColor,
    int xStart, int yStart,
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

    int xLeft = xStart + map.bitLeft;
    int yTop = yStart + map.bitTop;

    int xEnd = xStart + map.width;
    int yEnd = yStart + map.height;

    //纵向起始跳过行数
    for (y = yStart; y < yTop; y++)
    {
        for (x = xStart; x < xEnd; x++)
            PRINT_DOT(x, y, bColor);
    }
    //正文
    for (byteTotalCount = 0; byteTotalCount < byteTotal && y < yEnd; y++)
    {
        //横向起始跳过点数
        for (x = xStart; x < xLeft; x++)
            PRINT_DOT(x, y, bColor);
        //正文
        for (lineByteCount = 0; lineByteCount < map.lineByte; lineByteCount++, byteTotalCount++)
        {
            //画一行的点(注意 map.lineByte 有盈余,需判断 x < xEnd 保证不多画)
            for (i = 0, bit = map.bitMap[byteTotalCount]; i < 8 && x < xEnd; i++, x++)
            {
                if (bit & 0x80)
                    PRINT_DOT(x, y, fColor);
                else
                    PRINT_DOT(x, y, bColor);
                bit <<= 1;
            }
        }
        //横向补足点数
        for (; x < xEnd; x++)
            PRINT_DOT(x, y, bColor);
    }
    //纵向补足行数
    for (; y < yEnd; y++)
    {
        for (x = xStart; x < xEnd; x++)
            PRINT_DOT(x, y, bColor);
    }
}

/*
 *  根据 ttf_map 画点阵,增加范围限制和透明度参数
 */
void view_printBitMap2(
    int fColor, int bColor,
    int xStart, int yStart,
    int xScreenStart, int yScreenStart,
    int xScreenEnd, int yScreenEnd,
    float alpha,
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

    int xLeft = xStart + map.bitLeft;
    int yTop = yStart + map.bitTop;

    int xEnd = xStart + map.width;
    int yEnd = yStart + map.height;

    //纵向起始跳过行数
    for (y = yStart; y < yTop; y++)
    {
        for (x = xStart; x < xEnd; x++)
            PRINT_DOT2(x, y, bColor, alpha);
    }
    //正文
    for (byteTotalCount = 0; byteTotalCount < byteTotal && y < yEnd; y++)
    {
        //横向起始跳过点数
        for (x = xStart; x < xLeft; x++)
            PRINT_DOT(x, y, bColor);
        //正文
        for (lineByteCount = 0; lineByteCount < map.lineByte; lineByteCount++, byteTotalCount++)
        {
            //画一行的点(注意 map.lineByte 有盈余,需判断 x < xEnd 保证不多画)
            for (i = 0, bit = map.bitMap[byteTotalCount]; i < 8 && x < xEnd; i++, x++)
            {
                if (bit & 0x80)
                    PRINT_DOT2(x, y, fColor, alpha);
                else
                    PRINT_DOT(x, y, bColor);
                bit <<= 1;
            }
        }
        //横向补足点数
        for (; x < xEnd; x++)
            PRINT_DOT(x, y, bColor);
    }
    //纵向补足行数
    for (; y < yEnd; y++)
    {
        for (x = xStart; x < xEnd; x++)
            PRINT_DOT(x, y, bColor);
    }
}

/*
 *  功能: 字符串输出
 *  参数:
 *      fColor : 打印颜色
 *      bColor : 背景颜色,-1时使用原图填充(也就是透明)
 *      str : 字符串
 *      xStart, yStart : 矩阵的左上角定位坐标
 *      type : 字体, 例如 160, 240, 320, 400, 480, 560, 640, 前两位标识像素尺寸, 后1位表示字体
 *      space : 字符间隔, 正常输出为0
 *  返回: 无
 */
void view_string(int fColor, int bColor, char *str, int xStart, int yStart, int type, int space)
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
            view_printBitMap(fColor, bColor, xStart, yStart, map);
            xStart += map.width + space;
            str += ret;
        }
    }
}

/*
 *  功能: 字符串输出, 带范围限制
 *  参数:
 *      fColor : 打印颜色
 *      bColor : 背景颜色,-1时使用原图填充(也就是透明)
 *      str : 字符串
 *      xStart, yStart : 矩阵的左上角定位坐标
 *      strWidth, strHight : 相对左上角定位坐标, 限制宽, 高的矩阵内输出字符串
 *      type : 字体, 例如 160, 240, 320, 400, 480, 560, 640, 前两位标识像素尺寸, 后1位表示字体
 *      space : 字符间隔, 正常输出为0
 *      alpha : 0 正常打印, 1 半透打印
 *  返回: 无
 */
void view_string_rectangle(
    int fColor, int bColor,
    char *str,
    int xStart, int yStart,
    int strWidth, int strHight,
    int xScreenStart, int yScreenStart,
    int xScreenEnd, int yScreenEnd,
    int type, int space, float alpha)
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
            view_printBitMap2(
                fColor, bColor,
                xStart, yStart,
                xScreenStart, yScreenStart,
                xScreenEnd, yScreenEnd,
                alpha,
                map);
            xStart += map.width + space;
            str += ret;
        }
    }
}

/*
 *  功能: 字符串输出, 带范围限制, 自动换行
 *  参数:
 *      fColor : 打印颜色
 *      bColor : 背景颜色,-1时使用原图填充(也就是透明)
 *      str : 字符串
 *      xStart, yStart : 矩阵的左上角定位坐标
 *      strWidth, strHight : 相对左上角定位坐标, 限制宽, 高的矩阵内输出字符串
 *      type : 字体, 例如 160, 240, 320, 400, 480, 560, 640, 前两位标识像素尺寸, 后1位表示字体
 *      xSpace, ySpace : 字符间隔, 正常输出为0
 *      alpha : 0 正常打印, 1 半透打印
 *      lineSpace : 上下行间隔
 *      retWordPerLine : 传入记录每行占用字节数的数组指针, 不用可置NULL
 *      retLine : 传入记录占用行数的指针, 不用可置NULL
 * 返回: 成功输出的字符数
 */
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
                //
                if (*str == 0 && rwpl) //返回前记得先保存行占用字节数
                    *rwpl = str - strOld;
            }
            else if (*str == '\n' || *str == '\r')
            {
                yC += (typeHeight + ySpace); //换行
                xC = xStart;
                if (*str == '\r')
                    str += 2;
                else
                    str += 1;

                if (rl && yC < yE)
                    *rl += 1;
                if (rwpl) //行占用字节数记录
                {
                    *rwpl = str - strOld; //计算偏移量得到一行字节数
                    rwpl += 1;
                    strOld = str; //更新偏移指针
                }
            }

            if (str == 0 || yC > yE)
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
            view_printBitMap2(
                fColor, bColor,
                xC, yC,
                xScreenStart, yScreenStart,
                xScreenEnd, yScreenEnd,
                alpha,
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
 *      fColor : 打印颜色
 *      bColor : 背景颜色,-1时使用原图填充(也就是透明)
 *      str : 字符串
 *      xStart, yStart : 矩阵的左上角定位坐标
 *      strWidth, strHight : 相对左上角定位坐标, 限制宽, 高的矩阵内输出字符串
 *      type : 字体, 例如 160, 240, 320, 400, 480, 560, 640, 前两位标识像素尺寸, 后1位表示字体
 *      space : 字符间隔, 正常输出为0
 *      xErr : 相对 xStart 坐标, 字符串输出前先按xErr的 负/正的量 进行 左/右偏移一定像素
 * 返回: 返回此次绘制的偏差值, 以便后续无缝衔接
 */
int view_string_rectangleCR(
    int fColor, int bColor,
    char *str,
    int xStart, int yStart,
    int strWidth, int strHight,
    int xScreenStart, int yScreenStart,
    int xScreenEnd, int yScreenEnd,
    int type, int space,
    int xErr, float alpha)
{
    int xMov = xErr, typeHeight = type / 10;
    int ret, strCount = 0, retVal = xErr;

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
                view_printBitMap2(
                    fColor, bColor,
                    xStart + xMov, yStart,
                    xScreenStart, yScreenStart,
                    xScreenEnd, yScreenEnd,
                    alpha,
                    map);
            xMov += map.width + space;
            strCount += ret;
        }
        //字符串内容循环输出
        if (str[strCount] == '\0')
        {
            //自动填充空格分开字符串
            xMov += (typeHeight + space);

            if (xMov <= 0)     //字符串遍历完还未抵达开始绘制的位置
                retVal = xMov; //返回此次绘制的偏差值, 以便后续无缝衔接
            strCount = 0;
        }
    }

    return retVal;
}

//--------------------  UI系统初始化 --------------------

void view_init(void)
{
    //配置文件初始化
    viewConfig_init();
    //viewSource初始化
    viewSource_init();
    //viewColor初始化
    viewColor_init();
    //平台系统初始化
    PLAT_INIT();
}

//--------------------  viewTool add/insert/remove --------------------

/*
 *  把 view 添加到 parentView 的子view链表
 *  参数:
 *      front: 是否添加到前面
 */
void view_add(View_Struct *parentView, View_Struct *view, bool front)
{
    View_Struct *vsTemp;

    if (parentView == NULL || view == NULL)
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

    if (nodeView == NULL || view == NULL || nodeView->parent == NULL)
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

    if (view == NULL || view->parent == NULL)
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

//--------------------  viewTool is ? --------------------

bool view_isChild(View_Struct *parent, View_Struct *child)
{
    if (parent == NULL || child == NULL)
        return false;
    if (parent == child->parent)
        return true;
    return view_isChild(parent, child->parent);
}

//--------------------  viewTool focus --------------------

View_Focus *view_focusInit(View_Struct *topView, View_Struct *cView, ViewValue_Format *color)
{
    View_Focus *focus;
    if (topView)
    {
        focus = (View_Focus *)calloc(1, sizeof(View_Focus));
        focus->topView = topView;
        focus->color = color;
        focus->lineSize = 3;
        focus->alpha = 0;
        view_focusNote(focus, cView);
        return focus;
    }
    return NULL;
}

void view_focusRecover(View_Focus *focus)
{
    if (focus == NULL)
        return;
    if (focus->view)
    {
        focus->view->focus = NULL;
        focus->view = NULL;
    }
}

void view_focusNote(View_Focus *focus, View_Struct *view)
{
    if (focus == NULL)
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

    if (view == NULL || view->disable || view->invisible)
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

    if (focus == NULL || focus->topView == NULL)
        return NULL;
    if (focus->view == NULL)
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

            if (vsTemp->next == NULL)
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

            if (vsTemp->last == NULL)
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

#define F_NEXT(a) a = a->next
#define F_LAST(a) a = a->last
#define FIND_LINK(o, a, n)      \
    if (a->number > n)          \
    {                           \
        while (a)               \
        {                       \
            if (a->number == n) \
                return a;       \
            F_LAST(a);          \
        }                       \
    }                           \
    else if (a->number < n)     \
    {                           \
        while (a)               \
        {                       \
            if (a->number == n) \
                return a;       \
            F_NEXT(a);          \
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

void _viewTool_viewLocal(char drawSync, View_Struct *view, int width, int height, int xy[2][2])
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
            rView->drawSync != drawSync)
            _viewTool_viewLocal(drawSync, rView, width, height, xy);
    }

    if (view->centerHor)
    {
        view->absXY[0][0] = xy[0][0] + (width - view->absWidth) / 2 - 1;
        view->absXY[1][0] = xy[0][0] + (width + view->absWidth) / 2 - 1;
    }
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
            view->absXY[0][0] = rView->absXY[0][0] + view->rLeftRightErr;
            view->absXY[1][0] = view->absXY[0][0] + view->absWidth - 1;
        }
    }
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

    if (view->centerVer)
    {
        view->absXY[0][1] = xy[0][1] + (height - view->absHeight) / 2 - 1;
        view->absXY[1][1] = xy[0][1] + (height + view->absHeight) / 2 - 1;
    }
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
            view->absXY[0][1] = rView->absXY[0][1] + view->rTopBottomErr;
            view->absXY[1][1] = view->absXY[0][1] + view->absHeight - 1;
        }
    }
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

    view->drawSync = drawSync;
}

char *_viewTool_valuePrint(ViewValue_Format *value, ViewPrint_Struct *vps)
{
    int i, count, ret;
    char strDemoIntArray[] = "%01d,";
    char strDemoDouble[] = "%.6lf";
    char strDemoDoubleArray[] = "%.6lf,";
    char strDemoStrArray[] = "%s,";

    //数组数据有变动,重新初始化结构体
    if (value->type != vps->type ||
        value->vSize != vps->vSize ||
        vps->valueOutput == NULL)
    {
        if (value->vSize == 0)
            return NULL;

        vps->type = value->type;
        vps->vSize = value->vSize;
        if (vps->valueOutput)
            free(vps->valueOutput);
        vps->valueOutput = NULL;

        switch (value->type)
        {
        case VT_INT_ARRAY:
            vps->valueOutput = (char *)calloc(value->vSize / sizeof(int) * 32, sizeof(char));
            break;
        case VT_DOUBLE_ARRAY:
            vps->valueOutput = (char *)calloc(value->vSize / sizeof(double) * 32, sizeof(char));
            break;
        case VT_BOOL_ARRAY:
            vps->valueOutput = (char *)calloc(value->vSize / sizeof(bool) * 8, sizeof(char));
            break;
        case VT_STRING_ARRAY:
            for (i = 0, count = 0; i < value->vSize / sizeof(char *); i++)
                count += strlen(value->value.StringArray[i]) + 1;
            vps->valueOutputLen = count;
            vps->valueOutput = (char *)calloc(count, sizeof(char));
            break;
        default:
            vps->valueOutput = (char *)calloc(64, sizeof(char));
            break;
        }
    }

    switch (value->type)
    {
    case VT_CHAR:
        vps->valueOutput[0] = value->value.Char;
        break;
    case VT_STRING:
        return value->value.String;
    case VT_INT:
        sprintf(vps->valueOutput, "%d", value->value.Int);
        break;
    case VT_DOUBLE:
        if (value->param[1])
            strDemoDouble[2] = (value->param[1] % 10) + '0'; //指定保留小数位数
        sprintf(vps->valueOutput, strDemoDouble, value->value.Double);
        break;
    case VT_BOOL:
        sprintf(vps->valueOutput, "%s", value->value.Bool ? "true" : "false");
        break;
    case VT_INT_ARRAY:
        if (value->param[0] && value->param[0] != '%')
            strDemoIntArray[4] = value->param[0]; //指定分隔符
        if (value->param[1])
            strDemoIntArray[2] = (value->param[1] % 10) + '0'; //指定高位补0数量
        for (i = 0, count = 0; i < value->vSize / sizeof(int); i++)
        {
            ret = sprintf(&vps->valueOutput[count], strDemoIntArray, value->value.IntArray[i]);
            count += ret;
        }
        //当数组只有一个元素时,补全最后的分隔符,否则丢弃
        vps->valueOutput[count - 1] = 
            (i == 1 && value->param[0]) ? value->param[0] : 0;
        break;
    case VT_DOUBLE_ARRAY:
        if (value->param[0] && value->param[0] != '%')
            strDemoDoubleArray[5] = value->param[0]; //指定分隔符
        if (value->param[1])
            strDemoDoubleArray[2] = (value->param[1] % 10) + '0'; //指定保留小数位数
        for (i = 0, count = 0; i < value->vSize / sizeof(double); i++)
        {
            ret = sprintf(&vps->valueOutput[count], strDemoDoubleArray, value->value.DoubleArray[i]);
            count += ret;
        }
        //当数组只有一个元素时,补全最后的分隔符,否则丢弃
        vps->valueOutput[count - 1] = 
            (i == 1 && value->param[0]) ? value->param[0] : 0;
        break;
    case VT_BOOL_ARRAY:
        if (value->param[0] && value->param[0] != '%')
            strDemoStrArray[2] = value->param[0]; //指定分隔符
        for (i = 0, count = 0; i < value->vSize / sizeof(bool); i++)
        {
            ret = sprintf(&vps->valueOutput[count], strDemoStrArray, value->value.BoolArray[i] ? "true" : "false");
            count += ret;
        }
        //当数组只有一个元素时,补全最后的分隔符,否则丢弃
        vps->valueOutput[count - 1] = 
            (i == 1 && value->param[0]) ? value->param[0] : 0;
        break;
    case VT_STRING_ARRAY:
        if (value->param[0] && value->param[0] != '%')
            strDemoStrArray[2] = value->param[0]; //指定分隔符
        for (i = 0, count = 0; i < value->vSize / sizeof(char *); i++)
            count += strlen(value->value.StringArray[i]) + 1;
        if (count > vps->valueOutputLen)
        {
            free(vps->valueOutput);
            vps->valueOutput = (char *)calloc(count, sizeof(char));
        }
        for (i = 0, count = 0; i < value->vSize / sizeof(char *); i++)
        {
            ret = sprintf(&vps->valueOutput[count], strDemoStrArray, value->value.StringArray[i]);
            count += ret;
        }
        //当数组只有一个元素时,补全最后的分隔符,否则丢弃
        vps->valueOutput[count - 1] = 
            (i == 1 && value->param[0]) ? value->param[0] : 0;
        break;
    default:
        return NULL;
    }

    return vps->valueOutput;
}

//控件锁定(失能)时的颜色处理
static int _viewTool_lockColor(int color)
{
    uint8_t R = ((color & 0xFF0000) >> 16);
    uint8_t G = ((color & 0x00FF00) >> 8);
    uint8_t B = (color & 0x0000FF);

    R = (R + 0xFF) >> 1;
    G = (G + 0xFF) >> 1;
    B = (B + 0xFF) >> 1;

    return ((R << 16) | (G << 8) | B);
}

void _view_draw(View_Struct *view, int xyLimit[2][2])
{
    int widthTemp = 0, heightTemp = 0;
    int colorTemp = 0, colorTemp2 = 0;

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
            view->backGroundColor->value.Int,
            view->absXY[0][0], view->absXY[0][1],
            view->absXY[1][0], view->absXY[1][1],
            0, view->backGroundRad, view->backGroundAlpha,
            xyLimit[0][0], xyLimit[0][1], xyLimit[1][0], xyLimit[1][1]);
    }
    //形状绘制
    view->shapeAbsXY[0][0] = view->absXY[0][0] + view->shapeLeftEdge;
    view->shapeAbsXY[1][0] = view->absXY[1][0] - view->shapeRightEdge;
    view->shapeAbsXY[0][1] = view->absXY[0][1] + view->shapeTopEdge;
    view->shapeAbsXY[1][1] = view->absXY[1][1] - view->shapeBottomEdge;
    widthTemp = view->shapeAbsXY[1][0] - view->shapeAbsXY[0][0] + 1;
    heightTemp = view->shapeAbsXY[1][1] - view->shapeAbsXY[0][1] + 1;
    if (view->shapeColorPrint)
    {
        if (view->lock)
            colorTemp = _viewTool_lockColor(view->shapeColorPrint->value.Int);
        // colorTemp = (((view->shapeColorPrint->value.Int&0xFF0000)>>1)&0xFF0000) |
        //     (((view->shapeColorPrint->value.Int&0x00FF00)>>1)&0x00FF00) |
        //     (((view->shapeColorPrint->value.Int&0x0000FF)>>1)&0x0000FF);
        else
            colorTemp = view->shapeColorPrint->value.Int;
    }
    if (view->shapeColorBackground)
    {
        if (view->lock)
            colorTemp2 = _viewTool_lockColor(view->shapeColorBackground->value.Int);
        // colorTemp2 = (((view->shapeColorBackground->value.Int&0xFF0000)>>1)&0xFF0000) |
        //     (((view->shapeColorBackground->value.Int&0x00FF00)>>1)&0x00FF00) |
        //     (((view->shapeColorBackground->value.Int&0x0000FF)>>1)&0x0000FF);
        else
            colorTemp2 = view->shapeColorBackground->value.Int;
    }
    switch (view->shapeType)
    {
    case VST_RECT:
        if (view->shape.rect.lineSize > 0 && view->shapeColorBackground) //背景色
            view_rectangle(colorTemp2,
                           view->shapeAbsXY[0][0], view->shapeAbsXY[0][1],
                           view->shapeAbsXY[1][0], view->shapeAbsXY[1][1],
                           0, view->shape.rect.rad, view->shapeAlpha,
                           xyLimit[0][0], xyLimit[0][1], xyLimit[1][0], xyLimit[1][1]);
        if (view->shapeColorPrint)
            view_rectangle(colorTemp,
                           view->shapeAbsXY[0][0], view->shapeAbsXY[0][1],
                           view->shapeAbsXY[1][0], view->shapeAbsXY[1][1],
                           view->shape.rect.lineSize, view->shape.rect.rad, view->shapeAlpha,
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
                    0, widthTemp - view->shape.para.lineSize * 2 + intTemp3, view->shapeAlpha,
                    xyLimit[0][0], xyLimit[0][1], xyLimit[1][0], xyLimit[1][1]);
            view_parallelogram(
                colorTemp,
                view->shapeAbsXY[0][0] + intTemp, view->shapeAbsXY[0][1],
                view->shapeAbsXY[1][0] + intTemp2, view->shapeAbsXY[1][1],
                view->shape.para.lineSize, widthTemp + intTemp3, view->shapeAlpha,
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
                view->shape.line.lineSize, view->shape.line.space, view->shapeAlpha);
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
                        view->shapeAlpha,
                        xyLimit[0][0], xyLimit[0][1], xyLimit[1][0], xyLimit[1][1]);
                view_circle(
                    colorTemp,
                    view->shapeAbsXY[0][0] + widthTemp / 2 - 1,
                    view->shapeAbsXY[0][1] + heightTemp / 2 - 1,
                    view->shape.circle.rad,
                    view->shape.circle.rad - view->shape.circle.rad2,
                    view->shapeAlpha,
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
                        view->shapeAlpha,
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
        if (view->shapeColorPrint && view->shape.processBar.lineSize == 0)
        {
            view_rectangle(
                colorTemp,
                view->shapeAbsXY[0][0], view->shapeAbsXY[0][1],
                view->shapeAbsXY[1][0], view->shapeAbsXY[1][1],
                view->shape.processBar.lineSize,
                view->shape.processBar.rad,
                view->shapeAlpha,
                xyLimit[0][0], xyLimit[0][1], xyLimit[1][0], xyLimit[1][1]);
        }
        if (view->shapeColorBackground && view->shape.processBar.percent > 0)
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
                    colorTemp2,
                    absXYTemp[0][0],
                    absXYTemp[0][1],
                    absXYTemp[0][0] + (absXYTemp[1][0] - absXYTemp[0][0]) * view->shape.processBar.percent / 100,
                    absXYTemp[1][1],
                    0,
                    view->shape.processBar.rad - view->shape.processBar.edge,
                    view->shapeAlpha,
                    xyLimit[0][0], xyLimit[0][1], xyLimit[1][0], xyLimit[1][1]);
                break;
            case 2: //从下到上
                view_rectangle(
                    colorTemp2,
                    absXYTemp[0][0],
                    absXYTemp[1][1] - (absXYTemp[1][1] - absXYTemp[0][1]) * view->shape.processBar.percent / 100,
                    absXYTemp[1][0],
                    absXYTemp[1][1],
                    0,
                    view->shape.processBar.rad - view->shape.processBar.edge,
                    view->shapeAlpha,
                    xyLimit[0][0], xyLimit[0][1], xyLimit[1][0], xyLimit[1][1]);
                break;
            case 1: //从右到左
                view_rectangle(
                    colorTemp2,
                    absXYTemp[1][0] - (absXYTemp[1][0] - absXYTemp[0][0]) * view->shape.processBar.percent / 100,
                    absXYTemp[0][1],
                    absXYTemp[1][0],
                    absXYTemp[1][1],
                    0,
                    view->shape.processBar.rad - view->shape.processBar.edge,
                    view->shapeAlpha,
                    xyLimit[0][0], xyLimit[0][1], xyLimit[1][0], xyLimit[1][1]);
                break;
            case 3: //从上到下
                view_rectangle(
                    colorTemp2,
                    absXYTemp[0][0],
                    absXYTemp[0][1],
                    absXYTemp[1][0],
                    absXYTemp[0][1] + (absXYTemp[1][1] - absXYTemp[0][1]) * view->shape.processBar.percent / 100,
                    0,
                    view->shape.processBar.rad - view->shape.processBar.edge,
                    view->shapeAlpha,
                    xyLimit[0][0], xyLimit[0][1], xyLimit[1][0], xyLimit[1][1]);
                break;
            case 4: //左右同时缩进
                view_rectangle(
                    colorTemp2,
                    absXYTemp[0][0] + (absXYTemp[1][0] - absXYTemp[0][0]) * (100 - view->shape.processBar.percent) / 100 / 2,
                    absXYTemp[0][1],
                    absXYTemp[1][0] - (absXYTemp[1][0] - absXYTemp[0][0]) * (100 - view->shape.processBar.percent) / 100 / 2,
                    absXYTemp[1][1],
                    0,
                    view->shape.processBar.rad - view->shape.processBar.edge,
                    view->shapeAlpha,
                    xyLimit[0][0], xyLimit[0][1], xyLimit[1][0], xyLimit[1][1]);
                break;
            case 5: //上下同时缩进
                view_rectangle(
                    colorTemp2,
                    absXYTemp[0][0],
                    absXYTemp[0][1] + (absXYTemp[1][1] - absXYTemp[0][1]) * (100 - view->shape.processBar.percent) / 100 / 2,
                    absXYTemp[1][0],
                    absXYTemp[1][1] - (absXYTemp[1][1] - absXYTemp[0][1]) * (100 - view->shape.processBar.percent) / 100 / 2,
                    0,
                    view->shape.processBar.rad - view->shape.processBar.edge,
                    view->shapeAlpha,
                    xyLimit[0][0], xyLimit[0][1], xyLimit[1][0], xyLimit[1][1]);
                break;
            case 6: //上下左右同时缩进
                view_rectangle(
                    colorTemp2,
                    absXYTemp[0][0] + (absXYTemp[1][0] - absXYTemp[0][0]) * (100 - view->shape.processBar.percent) / 100 / 2,
                    absXYTemp[0][1] + (absXYTemp[1][1] - absXYTemp[0][1]) * (100 - view->shape.processBar.percent) / 100 / 2,
                    absXYTemp[1][0] - (absXYTemp[1][0] - absXYTemp[0][0]) * (100 - view->shape.processBar.percent) / 100 / 2,
                    absXYTemp[1][1] - (absXYTemp[1][1] - absXYTemp[0][1]) * (100 - view->shape.processBar.percent) / 100 / 2,
                    0,
                    view->shape.processBar.rad - view->shape.processBar.edge,
                    view->shapeAlpha,
                    xyLimit[0][0], xyLimit[0][1], xyLimit[1][0], xyLimit[1][1]);
            }
        }
        if (view->shapeColorPrint && view->shape.processBar.lineSize > 0)
        {
            view_rectangle(
                colorTemp,
                view->shapeAbsXY[0][0], view->shapeAbsXY[0][1],
                view->shapeAbsXY[1][0], view->shapeAbsXY[1][1],
                view->shape.processBar.lineSize,
                view->shape.processBar.rad,
                view->shapeAlpha,
                xyLimit[0][0], xyLimit[0][1], xyLimit[1][0], xyLimit[1][1]);
        }
        break;
    case VST_SCROLL_BAR:
        if (view->shapeColorPrint)
            view_rectangle(
                colorTemp,
                view->shapeAbsXY[0][0], view->shapeAbsXY[0][1],
                view->shapeAbsXY[1][0], view->shapeAbsXY[1][1],
                view->shape.scrollBar.lineSize, view->shape.scrollBar.rad, view->shapeAlpha,
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
                    0, view->shape.scrollBar.rad, view->shapeAlpha,
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
                    0, view->shape.scrollBar.rad, view->shapeAlpha,
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
                view->shape.sw.lineSize, view->shape.sw.rad, view->shapeAlpha,
                xyLimit[0][0], xyLimit[0][1], xyLimit[1][0], xyLimit[1][1]);
        if (view->shape.sw.status) //开
        {
            view_rectangle(
                ViewColor.Green.value.Int,
                view->shapeAbsXY[1][0] - widthTemp / 2,
                view->shapeAbsXY[0][1],
                view->shapeAbsXY[1][0],
                view->shapeAbsXY[1][1],
                0, view->shape.sw.rad, view->shapeAlpha,
                xyLimit[0][0], xyLimit[0][1], xyLimit[1][0], xyLimit[1][1]);
            view_string_rectangle(
                ViewColor.Content.value.Int, -1, "开",
                view->shapeAbsXY[0][0] + widthTemp / 4 - intTemp / 2,
                view->shapeAbsXY[0][1] + heightTemp / 2 - intTemp / 2,
                intTemp, intTemp,
                xyLimit[0][0], xyLimit[0][1], xyLimit[1][0], xyLimit[1][1],
                view->shape.sw.printType, 0, view->shapeAlpha);
        }
        else //关
        {
            view_rectangle(
                ViewColor.Red.value.Int,
                view->shapeAbsXY[0][0],
                view->shapeAbsXY[0][1],
                view->shapeAbsXY[0][0] + widthTemp / 2,
                view->shapeAbsXY[1][1],
                0, view->shape.sw.rad, view->shapeAlpha,
                xyLimit[0][0], xyLimit[0][1], xyLimit[1][0], xyLimit[1][1]);
            view_string_rectangle(
                ViewColor.Content.value.Int, -1, "关",
                view->shapeAbsXY[1][0] - widthTemp / 4 - intTemp / 4 * 3,
                view->shapeAbsXY[1][1] - heightTemp / 2 - intTemp / 2,
                intTemp, intTemp,
                xyLimit[0][0], xyLimit[0][1], xyLimit[1][0], xyLimit[1][1],
                view->shape.sw.printType, 0, view->shapeAlpha);
        }
        //框
        if (view->shapeColorPrint && view->shape.sw.lineSize)
            view_rectangle(
                colorTemp,
                view->shapeAbsXY[0][0], view->shapeAbsXY[0][1],
                view->shapeAbsXY[1][0], view->shapeAbsXY[1][1],
                view->shape.sw.lineSize, view->shape.sw.rad, view->shapeAlpha,
                xyLimit[0][0], xyLimit[0][1], xyLimit[1][0], xyLimit[1][1]);
        break;
    default:
        break;
    }

    //图片输出
    if (view->picPath)
    {
        //获取图片数据
        if (view->picPath_bak != view->picPath)
        {
            if (view->picMap)
                bmp_mapRelease(view->picMap, view->picWidth, view->picHeight);
            if (view->pic)
                free(view->pic);
            view->pic = bmp_get(view->picPath, &view->picWidth, &view->picHeight, &view->picPB);
            if (view->pic)
            {
                view->picMap = bmp_mapInit(view->pic, view->picWidth, view->picHeight, view->picPB);
                view->picPath_bak = view->picPath;
            }
            else //找不到图片,下次不找了...
                view->picPath_bak = view->picPath = NULL;
        }
        //输出范围计算
        view->picAbsXY[0][0] = view->absXY[0][0] + view->picLeftEdge;
        view->picAbsXY[1][0] = view->absXY[1][0] - view->picRightEdge;
        view->picAbsXY[0][1] = view->absXY[0][1] + view->picTopEdge;
        view->picAbsXY[1][1] = view->absXY[1][1] - view->picBottomEdge;
        //拉伸/缩放输出
        if (view->picUseReplaceColor && view->picReplaceColorBy)
            view_rectangle_padding2(
                view->picMap,
                view->picAbsXY[0][0], view->picAbsXY[0][1],
                view->picAbsXY[1][0], view->picAbsXY[1][1],
                view->picWidth, view->picHeight, view->picPB,
                view->picUseReplaceColor, view->picReplaceColor, view->picReplaceColorBy->value.Int,
                view->picUseInvisibleColor, view->picInvisibleColor,
                view->picAlpha,
                xyLimit[0][0], xyLimit[0][1], xyLimit[1][0], xyLimit[1][1]);
        else
            view_rectangle_padding2(
                view->picMap,
                view->picAbsXY[0][0], view->picAbsXY[0][1],
                view->picAbsXY[1][0], view->picAbsXY[1][1],
                view->picWidth, view->picHeight, view->picPB,
                false, 0, 0,
                view->picUseInvisibleColor, view->picInvisibleColor,
                view->picAlpha,
                xyLimit[0][0], xyLimit[0][1], xyLimit[1][0], xyLimit[1][1]);
    }

    //内容输出
    if (view->value && view->valueColor)
    {
        if (view->lock)
            colorTemp = _viewTool_lockColor(view->valueColor->value.Int);
        // colorTemp = (((view->valueColor->value.Int&0xFF0000)>>1)&0xFF0000) |
        //     (((view->valueColor->value.Int&0x00FF00)>>1)&0x00FF00) |
        //     (((view->valueColor->value.Int&0x0000FF)>>1)&0x0000FF);
        else
            colorTemp = view->valueColor->value.Int;

        //指定输出指针 *valueOutput
        view->valueOutput = _viewTool_valuePrint(view->value, &view->valuePrint);
        //输出范围计算
        view->valueAbsXY[0][0] = view->absXY[0][0] + view->valueLeftEdge;
        view->valueAbsXY[1][0] = view->absXY[1][0] - view->valueRightEdge;
        view->valueAbsXY[0][1] = view->absXY[0][1] + view->valueTopEdge;
        view->valueAbsXY[1][1] = view->absXY[1][1] - view->valueBottomEdge;
        widthTemp = view->valueAbsXY[1][0] - view->valueAbsXY[0][0] + 1;
        heightTemp = view->valueAbsXY[1][1] - view->valueAbsXY[0][1] + 1;

        if (view->valueOutput)
        {
            //----- 输出方式一: 自动换行输出 -----
            if (view->valueYEdge > 0)
            {
                //计算实际输出时的宽/高
                ttf_getSizeByUtf8_multiLine(
                    ViewTTF,
                    view->valueOutput,
                    view->valueType,
                    view->valueXEdge, view->valueYEdge,
                    view->valueAbsXY[1][0] - view->valueAbsXY[0][0],
                    &intTemp, &intTemp2);
                //左右居中处理
                if (view->valueHorType == 0) //居中
                {
                    view->valueAbsXY[0][0] = view->valueAbsXY[0][0] + (widthTemp - intTemp) / 2;
                    view->valueAbsXY[1][0] = view->valueAbsXY[0][0] + intTemp - 1;
                }
                else if (view->valueHorType == 1) //向左对齐
                    ;
                else //向右对齐
                    view->valueAbsXY[0][0] = view->valueAbsXY[0][0] + (widthTemp - intTemp);
                //上下居中处理
                if (view->valueVerType == 0)
                {
                    view->valueAbsXY[0][1] = view->valueAbsXY[0][1] + (heightTemp - intTemp2) / 2;
                    view->valueAbsXY[1][1] = view->valueAbsXY[0][1] + intTemp2 - 1;
                }
                else if (view->valueVerType == 1) //向上对齐
                    ;
                else //向下对齐
                    view->valueAbsXY[0][1] = view->valueAbsXY[0][1] + (heightTemp - intTemp2);

                view_string_rectangleLineWrap(
                    colorTemp, -1,
                    view->valueOutput,
                    view->valueAbsXY[0][0], view->valueAbsXY[0][1],
                    intTemp, intTemp2,
                    xyLimit[0][0], xyLimit[0][1], xyLimit[1][0], xyLimit[1][1],
                    view->valueType, view->valueXEdge, view->valueYEdge,
                    view->valueAlpha, NULL, NULL);
            }
            else
            {
                //计算实际输出时的宽/高
                intTemp = ttf_getSizeByUtf8(
                    ViewTTF,
                    view->valueOutput,
                    view->valueType,
                    view->valueXEdge,
                    &intTemp2);
                //上下居中处理
                if (view->valueVerType == 0)
                {
                    view->valueAbsXY[0][1] = view->valueAbsXY[0][1] + (heightTemp - intTemp2) / 2;
                    view->valueAbsXY[1][1] = view->valueAbsXY[0][1] + intTemp2 - 1;
                }
                else if (view->valueVerType == 1) //向上对齐
                    ;
                else //向下对齐
                    view->valueAbsXY[0][1] = view->valueAbsXY[0][1] + (heightTemp - intTemp2);
                //----- 输出方式二: 滚动输出 -----
                if (view->scroll > 0 &&
                    intTemp > view->valueAbsXY[1][0] - view->valueAbsXY[0][0]) //内容超出方框才滚动
                {
                    if (view->valueAbsXY[0][0] < xyLimit[0][0])
                        view->valueAbsXY[0][0] = xyLimit[0][0];
                    if (view->valueAbsXY[0][1] < xyLimit[0][1])
                        view->valueAbsXY[0][1] = xyLimit[0][1];
                    if (view->valueAbsXY[1][0] > xyLimit[1][0])
                        view->valueAbsXY[1][0] = xyLimit[1][0];
                    if (view->valueAbsXY[1][1] > xyLimit[1][1])
                        view->valueAbsXY[1][1] = xyLimit[1][1];
                    //
                    view->scrollCount = view_string_rectangleCR(
                        colorTemp, -1,
                        view->valueOutput,
                        view->valueAbsXY[0][0],
                        view->valueAbsXY[0][1],
                        view->valueAbsXY[1][0] - view->valueAbsXY[0][0], intTemp2,
                        view->valueAbsXY[0][0], view->valueAbsXY[0][1], view->valueAbsXY[1][0], view->valueAbsXY[1][1],
                        view->valueType, view->valueXEdge,
                        view->scrollCount, view->valueAlpha);
                    view->scrollCount2 += 1;
                    if (view->scrollCount2 >= view->scrollPeriod)
                    {
                        view->scrollCount2 = 0;
                        view->scrollCount -= view->scroll; //注意偏移量是往负值增加,即字幕往左滚动
                    }
                }
                //----- 输出方式三: 正常输出 -----
                else
                {
                    //左右居中处理(上面滚动是用不着横向居中的)
                    if (view->valueHorType == 0)
                    {
                        view->valueAbsXY[0][0] = view->valueAbsXY[0][0] + (widthTemp - intTemp) / 2;
                        view->valueAbsXY[1][0] = view->valueAbsXY[0][0] + intTemp - 1;
                    }
                    else if (view->valueHorType == 1) //向左对齐
                        ;
                    else //向右对齐
                        view->valueAbsXY[0][0] = view->valueAbsXY[0][0] + (widthTemp - intTemp);
                    view_string_rectangle(
                        colorTemp, -1,
                        view->valueOutput,
                        view->valueAbsXY[0][0],
                        view->valueAbsXY[0][1],
                        intTemp, intTemp2,
                        xyLimit[0][0], xyLimit[0][1], xyLimit[1][0], xyLimit[1][1],
                        view->valueType, view->valueXEdge, view->valueAlpha);
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
            // if(view->valueAbsXY[0][0] < xyLimit[0][0])
            //     intTemp = xyLimit[0][0];
            // else
            //     intTemp = view->valueAbsXY[0][0];
            // //
            // if(view->valueAbsXY[1][0] > xyLimit[1][0])
            //     intTemp2 = xyLimit[1][0];
            // else
            //     intTemp2 = view->valueAbsXY[1][0];
            // //
            // view_line(
            //     view->bottomLineColor->value.Int,
            //     intTemp, view->valueAbsXY[1][1],
            //     intTemp2, view->valueAbsXY[1][1],
            //     view->bottomLine, 0, view->bottomLineAlpha);
            //
            view_line(
                view->bottomLineColor->value.Int,
                view->absXY[0][0], view->absXY[1][1],
                view->absXY[1][0], view->absXY[1][1],
                view->bottomLine, 0, view->bottomLineAlpha);
        }
    }

    //描边
    if (view->side && view->sideColor)
    {
        switch (view->shapeType)
        {
        case VST_RECT:
            view_rectangle(
                view->sideColor->value.Int,
                view->shapeAbsXY[0][0], view->shapeAbsXY[0][1],
                view->shapeAbsXY[1][0], view->shapeAbsXY[1][1],
                view->side, view->shape.rect.rad, 0,
                xyLimit[0][0], xyLimit[0][1], xyLimit[1][0], xyLimit[1][1]);
            break;
        case VST_PROGRESS_BAR:
            view_rectangle(
                view->sideColor->value.Int,
                view->shapeAbsXY[0][0], view->shapeAbsXY[0][1],
                view->shapeAbsXY[1][0], view->shapeAbsXY[1][1],
                view->side, view->shape.processBar.rad, 0,
                xyLimit[0][0], xyLimit[0][1], xyLimit[1][0], xyLimit[1][1]);
            break;
        case VST_SCROLL_BAR:
            view_rectangle(
                view->sideColor->value.Int,
                view->shapeAbsXY[0][0], view->shapeAbsXY[0][1],
                view->shapeAbsXY[1][0], view->shapeAbsXY[1][1],
                view->side, view->shape.scrollBar.rad, 0,
                xyLimit[0][0], xyLimit[0][1], xyLimit[1][0], xyLimit[1][1]);
            break;
        case VST_SWITCH:
            view_rectangle(
                view->sideColor->value.Int,
                view->shapeAbsXY[0][0], view->shapeAbsXY[0][1],
                view->shapeAbsXY[1][0], view->shapeAbsXY[1][1],
                view->side, view->shape.sw.rad, 0,
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
                view->sideColor->value.Int,
                view->shapeAbsXY[0][0] + intTemp, view->shapeAbsXY[0][1],
                view->shapeAbsXY[1][0] + intTemp2, view->shapeAbsXY[1][1],
                view->side, widthTemp + intTemp3, 0,
                xyLimit[0][0], xyLimit[0][1], xyLimit[1][0], xyLimit[1][1]);
            break;
        case VST_CIRCLE:
            view_circle(
                view->sideColor->value.Int,
                view->shapeAbsXY[0][0] + widthTemp / 2 - 1,
                view->shapeAbsXY[0][1] + heightTemp / 2 - 1,
                view->shape.circle.rad, view->side, 0,
                xyLimit[0][0], xyLimit[0][1], xyLimit[1][0], xyLimit[1][1]);
            break;
        case VST_SECTOR:
            view_circle(
                view->sideColor->value.Int,
                view->shapeAbsXY[0][0] + widthTemp / 2 - 1,
                view->shapeAbsXY[0][1] + heightTemp / 2 - 1,
                view->shape.sector.rad, view->side, 0,
                xyLimit[0][0], xyLimit[0][1], xyLimit[1][0], xyLimit[1][1]);
            break;
        default:
            view_rectangle(
                view->sideColor->value.Int,
                view->absXY[0][0], view->absXY[0][1],
                view->absXY[1][0], view->absXY[1][1],
                view->side, 0, 0,
                xyLimit[0][0], xyLimit[0][1], xyLimit[1][0], xyLimit[1][1]);
            break;
        }
    }

    //mark
    ;

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
                    focus->color->value.Int,
                    view->shapeAbsXY[0][0], view->shapeAbsXY[0][1],
                    view->shapeAbsXY[1][0], view->shapeAbsXY[1][1],
                    focus->lineSize,
                    view->shape.rect.rad,
                    focus->alpha,
                    xyLimit[0][0], xyLimit[0][1], xyLimit[1][0], xyLimit[1][1]);
                break;
            case VST_PROGRESS_BAR:
                view_rectangle(
                    focus->color->value.Int,
                    view->shapeAbsXY[0][0], view->shapeAbsXY[0][1],
                    view->shapeAbsXY[1][0], view->shapeAbsXY[1][1],
                    focus->lineSize,
                    view->shape.processBar.rad,
                    focus->alpha,
                    xyLimit[0][0], xyLimit[0][1], xyLimit[1][0], xyLimit[1][1]);
                break;
            case VST_SCROLL_BAR:
                view_rectangle(
                    focus->color->value.Int,
                    view->shapeAbsXY[0][0], view->shapeAbsXY[0][1],
                    view->shapeAbsXY[1][0], view->shapeAbsXY[1][1],
                    focus->lineSize,
                    view->shape.scrollBar.rad,
                    focus->alpha,
                    xyLimit[0][0], xyLimit[0][1], xyLimit[1][0], xyLimit[1][1]);
                break;
            case VST_SWITCH:
                view_rectangle(
                    focus->color->value.Int,
                    view->shapeAbsXY[0][0], view->shapeAbsXY[0][1],
                    view->shapeAbsXY[1][0], view->shapeAbsXY[1][1],
                    focus->lineSize,
                    view->shape.sw.rad,
                    focus->alpha,
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
                    focus->color->value.Int,
                    view->shapeAbsXY[0][0] + intTemp, view->shapeAbsXY[0][1],
                    view->shapeAbsXY[1][0] + intTemp2, view->shapeAbsXY[1][1],
                    focus->lineSize,
                    widthTemp + intTemp3,
                    focus->alpha,
                    xyLimit[0][0], xyLimit[0][1], xyLimit[1][0], xyLimit[1][1]);
                break;
            case VST_CIRCLE:
                view_circle(
                    focus->color->value.Int,
                    view->shapeAbsXY[0][0] + widthTemp / 2 - 1,
                    view->shapeAbsXY[0][1] + heightTemp / 2 - 1,
                    view->shape.circle.rad,
                    focus->lineSize,
                    focus->alpha,
                    xyLimit[0][0], xyLimit[0][1], xyLimit[1][0], xyLimit[1][1]);
                break;
            case VST_SECTOR:
                view_circle(
                    focus->color->value.Int,
                    view->shapeAbsXY[0][0] + widthTemp / 2 - 1,
                    view->shapeAbsXY[0][1] + heightTemp / 2 - 1,
                    view->shape.sector.rad,
                    focus->lineSize,
                    focus->alpha,
                    xyLimit[0][0], xyLimit[0][1], xyLimit[1][0], xyLimit[1][1]);
                break;
            default:
                view_rectangle(
                    focus->color->value.Int,
                    view->absXY[0][0], view->absXY[0][1],
                    view->absXY[1][0], view->absXY[1][1],
                    focus->lineSize,
                    0,
                    focus->alpha,
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

        ret = view_draw(object, focus, event, viewParent, vsThis, xyLimit);
        if (ret == CALLBACK_BREAK)
            return CALLBACK_BREAK;

        vsThis = vsNext;
    }
    return CALLBACK_OK;
}

int view_draw(
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

    if (view == NULL)
        return CALLBACK_OK;

    // printf("view_draw: %s \r\n", view->name?view->name:"NULL");

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
        //造成 drawSync 的跳变增加; 子 view 可以通过缓存
        //该值,比较是否每次+1来判断是否第一次进入自己的界面
        ViewCommonParent.drawSync = view->drawSync + 1;

        //更新一次 ViewCommonParent 的系统滴答时钟
        ViewCommonParent.tickMs = view_tickMs();
    }

    //共享滴答时钟
    view->tickMs = ViewCommonParent.tickMs;

    //当前view是第一次绘制
    if (view->drawSync != parent->drawSync)
    {
        //absXY[2][2] 和 absWidth, absHeight 计算
        _viewTool_viewLocal(
            parent->drawSync, view,
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

    // printf("view_draw: %s %d[%d-%d](%d-%d) %d[%d-%d](%d-%d), sync: %d -- %d\r\n",
    //     view->name,
    //     view->absWidth,
    //     view->absXY[0][0], view->absXY[1][0],
    //     xyLimitVsTemp[0][0], xyLimitVsTemp[1][0],
    //     view->absHeight,
    //     view->absXY[0][1], view->absXY[1][1],
    //     xyLimitVsTemp[0][1], xyLimitVsTemp[1][1],
    //     view->drawSync, ViewCommonParent.drawSync);

    //开始绘图
    if (view->disable ||
        xyLimitVsTemp[0][0] > xyLimitVsTemp[1][0] ||
        xyLimitVsTemp[0][1] > xyLimitVsTemp[1][1])
        ;
    else
    {
        //firstIn ?
        view->drawSyncOld += 1;
        if (view->drawSyncOld != view->drawSync)
            view->firstIn = true;
        else
            view->firstIn = false;
        view->drawSyncOld = view->drawSync;

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
                printf("view_draw : view %s viewStart() err !!\r\n", view->name ? view->name : "???");
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
            ret = view_draw(object, focus, event, view, view->jumpView, xyLimit); //子 view 直接继承限制范围
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
                printf("view_draw : view %s viewEnd() err !!\r\n", view->name ? view->name : "???");
        }
    }

    view->callBackForbid = false; //允许回调函数检查

    return CALLBACK_OK;
}

ViewCallBack view_touchLocal(int xy[2], View_Struct *view, View_Struct **retView)
{
    ViewCallBack ret = NULL;
    View_Struct *vsTemp = NULL;

    if (xy == NULL || view == NULL || view->disable)
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

    int contentType2, contentMinType;
} _InputBackup_Struct;

int _input_comm_callBack(View_Struct *view, void *object, View_Focus *focus, ViewButtonTouch_Event *event)
{
    View_Struct *vsHead = NULL, *vsTail = NULL, *vsCurr = NULL, *vsStandard = NULL;
    View_Struct *vsParent, *vsTemp;
    int movMax = 0;
    _InputBackup_Struct *ibs;
    float alpha;

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
            movMax = -(vsTail->number - vsHead->number) * (ibs->contentMinType + 4);
        }
        else if (ibs->type == 2)
        {
            //找到头和尾
            vsStandard = vsParent->view->next->next;
            vsHead = vsParent->view->next->next->next;
            vsTail = vsParent->lastView->last;
            movMax = -(vsTail->number - vsHead->number) * (ibs->contentMinType / 2 + 4);
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
            vsCurr = view_num(vsParent->view, vsHead->number + (int)((-vsHead->rTopBottomErr) / (ibs->contentMinType + 4)));
            //透明度 字体 和 控件高度 调整
            vsCurr->valueAlpha = 0;
            vsCurr->valueType = vsStandard->valueType;
            vsCurr->height = vsStandard->height;
            vsCurr->bottomLine = 2;
            for (alpha = 0.2, vsTemp = vsCurr->last;
                 vsTemp && vsTemp->number > 2 && alpha < 1;
                 alpha += 0.2, vsTemp = vsTemp->last)
            {
                vsTemp->valueAlpha = alpha;
                vsTemp->valueType = ibs->contentMinType * 10;
                vsTemp->height = ibs->contentMinType + 4;
                vsTemp->bottomLine = 0;
            }
            for (alpha = 0.2, vsTemp = vsCurr->next;
                 vsTemp && alpha < 1;
                 alpha += 0.2, vsTemp = vsTemp->next)
            {
                vsTemp->valueAlpha = alpha;
                vsTemp->valueType = ibs->contentMinType * 10;
                vsTemp->height = ibs->contentMinType + 4;
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
            vsCurr = view_num(vsParent->view, vsHead->number + (int)((-vsHead->rLeftRightErr) / (ibs->contentMinType / 2 + 4)));
            //字体 和 控件宽度 调整
            vsCurr->valueType = vsStandard->valueType;
            vsCurr->width = vsStandard->width;
            vsCurr->bottomLine = 2;
            for (vsTemp = vsCurr->last;
                 vsTemp && vsTemp->number > 3;
                 vsTemp = vsTemp->last)
            {
                vsTemp->valueType = ibs->contentMinType * 10;
                vsTemp->width = ibs->contentMinType / 2 + 4;
                vsTemp->bottomLine = 0;
            }
            for (vsTemp = vsCurr->next;
                 vsTemp && vsTemp->number < vsParent->lastView->number;
                 vsTemp = vsTemp->next)
            {
                vsTemp->valueType = ibs->contentMinType * 10;
                vsTemp->width = ibs->contentMinType / 2 + 4;
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
                if (vsParent->view->next->next->value->value.String[0] == VIEW_DEL_CHAR)
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
                        backString[i] = vsTemp->value->value.Char;

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
                if (view->value == &ViewSrc.Api_Del_Char)
                    viewValue_reset(ibs->value, NULL, VT_CHAR, 1, VIEW_DEL_CHAR);
                //拷贝选中数据到返回缓存 ibs->value
                else
                    viewValue_copy(ibs->value, view->value);
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
        focus->color->value.Int,
        view->absXY[0][0], view->absXY[0][1],
        view->absXY[1][0], view->absXY[1][1],
        0, 0, 0.7,
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
        if (vsParent->view->next->next->value->value.String[0] == VIEW_DEL_CHAR)
            vidChar = VIEW_DEL_CHAR;

        //使用了删除符号
        if (vidChar)
        {
            //当前输入为列表最后一个,且输入的不是"删除",则在列表后面再加一位
            if (view == vsParent->lastView->last && view->value->value.Char != vidChar)
            {
                ibs = (_InputBackup_Struct *)(vsParent->privateData);
                if (ibs->astrict > 0 &&
                    ibs->astrict < view->number - 2) //再加一位就超出限制输入范围了
                    ;
                else
                {
                    vsTemp = (View_Struct *)calloc(1, sizeof(View_Struct));
                    sprintf(vsTemp->name, "_input_content%d", view->number + 1);
                    vsTemp->width = ibs->contentMinType / 2 + 4;
                    vsTemp->height = ibs->contentType2 + 4;
                    vsTemp->rNumber = VRNT_LAST;
                    vsTemp->rType = VRT_RIGHT;
                    vsTemp->value = viewValue_init(vsTemp->name, VT_CHAR, 1, VIEW_DEL_CHAR);
                    vsTemp->valueType = ibs->contentMinType * 10;
                    vsTemp->valueColor = &ViewColor.Tips;
                    vsTemp->bottomLineColor = focus->color;
                    vsTemp->focusStop = true;
                    vsTemp->callBack = (ViewCallBack)&_input_list2_callBack;
                    vsTemp->focusCallBackFront = (FocusCallBack)&view_input_focusCallBackFront;
                    view_insert(view, vsTemp, false); //使用插入方式
                }
            }
            //在任意位置输入了删除符号,移除当前 view 往后的 view
            else if (view->value->value.Char == vidChar)
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
                view->parent->view->value->value.String,
                view->value,
                view->parent->view->next->next->value,
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
    float alpha;
    int valueType;
    _InputBackup_Struct *ibs;

    ibs = view->privateData;
    valueType = ibs->contentMinType * 10;

    if (focus && focus->view->backView == NULL && focus->view->parent == view)
    {
        vsFocus = focus->view;
        switch ((((_InputBackup_Struct *)(view->privateData)))->type)
        {
        //纵向列表
        case 1:
            if (vsFocus->absXY[0][1] != view->view->next->absXY[0][1])
            {
                //位置平移
                view->view->next->next->rTopBottomErr = -(vsFocus->number - view->view->next->next->number) * (ibs->contentMinType + 4);
                //透明度 字体 和 控件高度 调整
                vsFocus->valueAlpha = 0;
                vsFocus->valueType = view->view->next->valueType;
                vsFocus->height = view->view->next->height;
                // ->last
                for (alpha = 0.2, vsTemp = vsFocus->last;
                     vsTemp && vsTemp->number > 2 && alpha < 1;
                     alpha += 0.2, vsTemp = vsTemp->last)
                {
                    vsTemp->valueAlpha = alpha;
                    vsTemp->valueType = valueType;
                    vsTemp->height = ibs->contentMinType + 4;
                    vsTemp->bottomLine = 0;
                }
                for (; vsTemp && vsTemp->number > 2; vsTemp = vsTemp->last)
                {
                    vsTemp->valueAlpha = 1;
                    vsTemp->valueType = valueType;
                    vsTemp->height = ibs->contentMinType + 4;
                    vsTemp->bottomLine = 0;
                }
                // ->next
                for (alpha = 0.2, vsTemp = vsFocus->next;
                     vsTemp && alpha < 1;
                     alpha += 0.2, vsTemp = vsTemp->next)
                {
                    vsTemp->valueAlpha = alpha;
                    vsTemp->valueType = valueType;
                    vsTemp->height = ibs->contentMinType + 4;
                    vsTemp->bottomLine = 0;
                }
                for (; vsTemp; vsTemp = vsTemp->next)
                {
                    vsTemp->valueAlpha = 1;
                    vsTemp->valueType = valueType;
                    vsTemp->height = ibs->contentMinType + 4;
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
                view->view->next->next->next->rLeftRightErr = -(vsFocus->number - view->view->next->next->next->number) * (ibs->contentMinType / 2 + 4);
                //字体 和 控件宽度 调整
                vsFocus->valueType = view->view->next->next->valueType;
                vsFocus->width = view->view->next->next->width;
                for (vsTemp = vsFocus->last;
                     vsTemp && vsTemp->number > 3;
                     vsTemp = vsTemp->last)
                {
                    vsTemp->valueType = ibs->contentMinType * 10;
                    vsTemp->width = ibs->contentMinType / 2 + 4;
                    vsTemp->bottomLine = 0;
                }
                for (vsTemp = vsFocus->next;
                     vsTemp && vsTemp->number < view->lastView->number;
                     vsTemp = vsTemp->next)
                {
                    vsTemp->valueType = ibs->contentMinType * 10;
                    vsTemp->width = ibs->contentMinType / 2 + 4;
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

    int labelType = ViewSrc.Common_LabelType.value.Int;
    int contentType = ViewSrc.Common_ContentType.value.Int, contentType2 = 48, contentMinType = 32, lineSize = 2;
    int rad = ViewSrc.Common_Rad.value.Int;

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

    if (backView == NULL || focus == NULL || vsParent == NULL)
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
    ibs->contentType2 = contentType2;
    ibs->contentMinType = contentMinType;
    vs->width = VWHT_MATCH;
    vs->height = VWHT_MATCH;
    sprintf(vs->name, "_input_%d", backView->drawSync);
    // vs->rLeftRightErr = -vsParent->absXY[0][0];
    // vs->rTopBottomErr = -vsParent->absXY[0][1];
    // vs->backGroundColor = &ViewColor.BackGround;//暗幕
    vs->backGroundAlpha = 0;
    // vs->overDraw = true;
    vs->viewStart = (ViewCallBack)&_input_viewStart;
    // vs->viewEnd = (ViewCallBack)&_input_viewEnd;

    //type 0: 仅提示(暂不支持数组类数据的输入)
    if (value == NULL ||
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
        vsTemp->shapeColorPrint = &ViewColor.Button;
        vsTemp->shapeBottomEdge = 1;
        vsTemp->shapeLeftEdge = vsTemp->shapeRightEdge = 1;
        vsTemp->value = viewValue_init("_input_frameContent", VT_STRING, 1, label);
        vsTemp->valueType = contentType * 10;
        vsTemp->valueColor = &ViewColor.Tips;
        vsTemp->valueTopEdge = vsTemp->valueBottomEdge = 5;
        vsTemp->valueLeftEdge = vsTemp->valueRightEdge = 5; //四周保持5的间距
        vsTemp->valueYEdge = 6;                             //自动换行,行间距5

        ttf_getSizeByUtf8_multiLine(
            ViewTTF,
            label,
            vsTemp->valueType,
            0, vsTemp->valueYEdge,
            vsParent->absWidth - vsTemp->valueLeftEdge - vsTemp->valueRightEdge,
            NULL, &retInt);

        if (retInt > (vsParent->absHeight) * 3 / 4 - vsTemp->valueTopEdge - vsTemp->valueBottomEdge) //提示信息太长超出范围
        {
            vsTemp->valueTopEdge = vsTemp->valueBottomEdge = 3;
            vsTemp->valueLeftEdge = vsTemp->valueRightEdge = 3; //四周保持5的间距
            vsTemp->valueYEdge = 4;
            vsTemp->valueType = 200;

            ttf_getSizeByUtf8_multiLine(
                ViewTTF,
                label,
                vsTemp->valueType,
                0, vsTemp->valueYEdge,
                vsParent->absWidth - vsTemp->valueLeftEdge - vsTemp->valueRightEdge,
                NULL, &retInt);

            if (retInt > (vsParent->absHeight) * 3 / 4 - vsTemp->valueTopEdge - vsTemp->valueBottomEdge) //提示信息还是太长超出范围
            {
                vsTemp->valueYEdge = 2;
                vsTemp->valueType = 160;
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
        vsTemp->shapeColorPrint = &ViewColor.Gray2;
        vsTemp->shapeTopEdge = vsTemp->shapeBottomEdge = 1;
        vsTemp->shapeLeftEdge = vsTemp->shapeRightEdge = 1;
        vsTemp->value = &ViewSrc.Api_Button_Cancel;
        vsTemp->valueType = contentType * 10;
        vsTemp->valueColor = &ViewColor.ButtonValue;
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
        vsTemp->shapeColorPrint = &ViewColor.Green3;
        vsTemp->shapeTopEdge = vsTemp->shapeBottomEdge = 1;
        vsTemp->shapeLeftEdge = vsTemp->shapeRightEdge = 1;
        vsTemp->value = &ViewSrc.Api_Button_Enter;
        vsTemp->valueType = contentType * 10;
        vsTemp->valueColor = &ViewColor.ButtonValue;
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
        vsTemp->shapeColorPrint = &ViewColor.Button;
        vsTemp->shapeBottomEdge = 1;
        vsTemp->shapeLeftEdge = vsTemp->shapeRightEdge = 1;
        vsTemp->value = viewValue_init("multiInput_label", VT_STRING, 1, label);
        vsTemp->valueType = labelType * 10;
        vsTemp->valueColor = &ViewColor.Label;
        vsTemp->valueTopEdge = lineSize * 2;
        vsTemp->valueLeftEdge = vsTemp->valueRightEdge = 5; //左右保持5的间距
        vsTemp->valueVerType = 1;
        vsTemp->scroll = labelType / 4; //自动滚动
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

                for (; count < valueNum; count++)
                {
                    if (count == retInt)
                        vvfArray[count] =
                            viewValue_init("_input_char", VT_CHAR, 1, value->value.Char);
                    else
                        vvfArray[count] =
                            viewValue_init("_input_char", VT_CHAR, 1, candidate->value.String[count]);
                }

                ibs->contentType2 = 56;
            }
            break;
        case VT_INT_ARRAY:
            if ((valueNum = candidate->vSize / sizeof(int)) > 0)
            {
                vvfArray = (ViewValue_Format **)calloc(valueNum, sizeof(ViewValue_Format *));
                for (count = 0; count < valueNum; count++)
                {
                    if (count == retInt)
                        vvfArray[count] =
                            viewValue_init("_input_int", VT_INT, 1, value->value.Int);
                    else
                        vvfArray[count] =
                            viewValue_init("_input_int", VT_INT, 1, candidate->value.IntArray[count]);
                }

                ibs->contentType2 = 56;
            }
            break;
        case VT_DOUBLE_ARRAY:
            if ((valueNum = candidate->vSize / sizeof(double)) > 0)
            {
                vvfArray = (ViewValue_Format **)calloc(valueNum, sizeof(ViewValue_Format *));
                for (count = 0; count < valueNum; count++)
                {
                    if (count == retInt)
                        vvfArray[count] =
                            viewValue_init("_input_double", VT_DOUBLE, 1, value->value.Double);
                    else
                        vvfArray[count] =
                            viewValue_init("_input_double", VT_DOUBLE, 1, candidate->value.DoubleArray[count]);
                }

                ibs->contentType2 = 56;
            }
            break;
        case VT_STRING_ARRAY:
            if ((valueNum = candidate->vSize / sizeof(char *)) > 0)
            {
                intTemp[0] = 0; //当前长度
                intTemp[1] = 0; //最长
                vvfArray = (ViewValue_Format **)calloc(valueNum, sizeof(ViewValue_Format *));
                for (count = 0; count < valueNum; count++)
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
                    ibs->contentType2 = view_getType(pointTemp, vsParent->absWidth, 0) / 10;
                    if (ibs->contentMinType > ibs->contentType2)
                        ibs->contentMinType = ibs->contentType2;
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
        vsTemp->height = ibs->contentType2 + 4;
        vsTemp->centerHor = true;
        vsTemp->centerVer = true;
        vsTemp->valueType = ibs->contentType2 * 10;
        view_add(vs, vsTemp, false);

        //当前值不在候选列表中 把当前值展示在列表最上面
        if (!retBool)
        {
            vsTemp = (View_Struct *)calloc(1, sizeof(View_Struct));
            sprintf(vsTemp->name, "_input_content%d", retInt);
            vsTemp->width = VWHT_MATCH;
            vsTemp->height = ibs->contentType2 + 4;
            vsTemp->rNumber = VRNT_LAST; //和标杆位置重合
            vsTemp->shapeType = VST_RECT;
            // vsTemp->shape.rect.rad = rad;
            vsTemp->value = viewValue_copy(NULL, value);
            vsTemp->valueType = ibs->contentType2 * 10;
            vsTemp->valueColor = &ViewColor.Tips;
            // vsTemp->valueLeftEdge = vsTemp->valueRightEdge = 5;//左右保持5的间距
            vsTemp->valueAlpha = 0;
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
            for (count = 0; count < valueNum; count++)
            {
                vsTemp = (View_Struct *)calloc(1, sizeof(View_Struct));
                sprintf(vsTemp->name, "_input_content%d", count);
                vsTemp->width = VWHT_MATCH;
                vsTemp->rNumber = VRNT_LAST;
                //是否是第一个
                if (count == 0 && retBool)
                {
                    vsTemp->rType = 0;
                    vsTemp->rTopBottomErr = (count - retInt) * (ibs->contentMinType + 4);
                }
                else
                {
                    vsTemp->rType = VRT_BOTTOM;
                    vsTemp->rTopBottomErr = 0;
                }
                //是否是当前
                if (count == retInt)
                {
                    vsTemp->valueType = ibs->contentType2 * 10;
                    vsTemp->height = ibs->contentType2 + 4;

                    retView = vsTemp;
                }
                else
                {
                    vsTemp->height = ibs->contentMinType + 4;
                    vsTemp->valueType = ibs->contentMinType * 10;
                }
                //透明度
                if (count > retInt)
                    vsTemp->valueAlpha = ((count - retInt > 4) ? 1 : ((count - retInt) * 0.2));
                else if (count < retInt)
                    vsTemp->valueAlpha = ((retInt - count > 4) ? 1 : ((retInt - count) * 0.2));
                else
                    vsTemp->valueAlpha = 0;
                // vsTemp->shapeType = VST_RECT;
                // vsTemp->shape.rect.rad = rad;
                vsTemp->value = vvfArray[count];
                vsTemp->value->param[0] = value->param[0];
                vsTemp->value->param[1] = value->param[1];
                vsTemp->valueColor = &ViewColor.Tips;
                // vsTemp->valueLeftEdge = vsTemp->valueRightEdge = 5;//左右保持5的间距
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
        ibs->contentType2 = 64;

        //框 和 label
        vsTemp = (View_Struct *)calloc(1, sizeof(View_Struct));
        strcpy(vsTemp->name, "_input_frameLabel");
        vsTemp->width = VWHT_MATCH;
        vsTemp->height = VWHT_MATCH * 4 - 3;
        vsTemp->shapeType = VST_RECT;
        vsTemp->shape.rect.rad = rad;
        vsTemp->shape.rect.lineSize = lineSize;
        vsTemp->shapeColorPrint = &ViewColor.Button;
        vsTemp->shapeLeftEdge = vsTemp->shapeRightEdge = 1;
        vsTemp->value = viewValue_init("multiInput_label", VT_STRING, 1, label);
        vsTemp->valueType = labelType * 10;
        vsTemp->valueColor = &ViewColor.Label;
        vsTemp->valueTopEdge = lineSize * 2;
        vsTemp->valueLeftEdge = vsTemp->valueRightEdge = 5; //左右保持5的间距
        vsTemp->valueVerType = 1;
        vsTemp->scroll = labelType / 4; //自动滚动
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
        vsTemp->shapeColorPrint = &ViewColor.Gray2;
        vsTemp->shapeTopEdge = vsTemp->shapeBottomEdge = 1;
        vsTemp->shapeLeftEdge = vsTemp->shapeRightEdge = 1;
        vsTemp->value = &ViewSrc.Api_Button_Cancel;
        vsTemp->valueType = contentType * 10;
        vsTemp->valueColor = &ViewColor.ButtonValue;
        vsTemp->focusStop = true;
        vsTemp->callBack = (ViewCallBack)&_input_returnOrCancel_callBack;
        vsTemp->enMoveEvent = true;
        view_add(vs, vsTemp, false);
        retView = vsTemp;

        //居中位置标杆
        vsTemp = (View_Struct *)calloc(1, sizeof(View_Struct));
        strcpy(vsTemp->name, "_input_contentMain");
        vsTemp->width = ibs->contentType2 / 2 + 4;
        vsTemp->height = ibs->contentType2 + 4;
        vsTemp->centerHor = true;
        // vsTemp->centerVer = true;
        vsTemp->rTopBottomErr = (156 - ibs->contentType2 + 4) / 2;
        vsTemp->value = viewValue_copy(NULL, candidate);
        vsTemp->valueType = ibs->contentType2 * 10;
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
            if (value->param[1] > 0)
                strDemo[2] = (value->param[1] % 10) + '0';
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
            vsTemp->width = ibs->contentType2 / 2 + 4;
            vsTemp->height = ibs->contentType2 + 4;
            vsTemp->rNumber = VRNT_LAST;
            vsTemp->value = viewValue_init("_input_content0", VT_CHAR, 1, valueStringPoint[0]);
            vsTemp->valueType = ibs->contentType2 * 10;
            vsTemp->valueColor = &ViewColor.Tips;
            vsTemp->bottomLineColor = focus->color;
            vsTemp->focusStop = true;
            vsTemp->callBack = (ViewCallBack)&_input_list2_callBack;
            vsTemp->focusCallBackFront = (FocusCallBack)&view_input_focusCallBackFront;
            vsTemp->enMoveEvent = true;
            view_add(vs, vsTemp, false);

            for (count = 1; valueStringPoint[count]; count++)
            {
                vsTemp = (View_Struct *)calloc(1, sizeof(View_Struct));
                sprintf(vsTemp->name, "_input_content%d", count);
                vsTemp->width = ibs->contentMinType / 2 + 4;
                vsTemp->height = ibs->contentType2 + 4;
                vsTemp->rNumber = VRNT_LAST;
                vsTemp->rType = VRT_RIGHT;
                vsTemp->value = viewValue_init(vsTemp->name, VT_CHAR, 1, valueStringPoint[count]);
                vsTemp->valueType = ibs->contentMinType * 10;
                vsTemp->valueColor = &ViewColor.Tips;
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
                vsTemp->width = ibs->contentMinType / 2 + 4;
                vsTemp->height = ibs->contentType2 + 4;
                vsTemp->rNumber = VRNT_LAST;
                vsTemp->rType = VRT_RIGHT;
                vsTemp->value = viewValue_init(vsTemp->name, VT_CHAR, 1, VIEW_DEL_CHAR);
                vsTemp->valueType = ibs->contentMinType * 10;
                vsTemp->valueColor = &ViewColor.Tips;
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
            vsTemp->width = ibs->contentType2 / 2 + 4;
            vsTemp->height = ibs->contentType2 + 4;
            vsTemp->rNumber = VRNT_LAST;
            vsTemp->value = viewValue_init(vsTemp->name, VT_CHAR, 1, candidate->value.String[0]);
            vsTemp->valueType = ibs->contentType2 * 10;
            vsTemp->valueColor = &ViewColor.Tips;
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
        vsTemp->shapeColorPrint = &ViewColor.Green3;
        vsTemp->shapeTopEdge = vsTemp->shapeBottomEdge = 1;
        vsTemp->shapeLeftEdge = vsTemp->shapeRightEdge = 1;
        vsTemp->value = &ViewSrc.Api_Button_Enter;
        vsTemp->valueType = contentType * 10;
        vsTemp->valueColor = &ViewColor.ButtonValue;
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
    int retWidth;
    int retValueType = 240;

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