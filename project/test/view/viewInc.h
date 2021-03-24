
/*
 *  全部 view_xxx 公用包含头文件
 */
#ifndef _VIEWINC_H_
#define _VIEWINC_H_

#include "ui/viewApi.h"

//源文件路径
#define SRC_PATH "./project/test/src"

//刷屏周期
#define REFRESH_MS 500

//状态栏宽高
#define BAR_WIDTH  VIEW_X_SIZE
#define BAR_HEIGHT 30

//状态栏 - 日期
#define BAR_DATE_WIDTH (VIEW_X_SIZE * 3 / 5)
//状态栏 - 电量
#define BAR_BATT_WIDTH (VIEW_X_SIZE / 5)
//状态栏 - 电量图标
#define BAR_ICON_WIDTH (VIEW_X_SIZE / 5)

//内容窗体宽高
#define WINDOW_WIDTH VIEW_X_SIZE
#define WINDOW_HEIGHT (VIEW_Y_SIZE - BAR_HEIGHT)

//view列表
View_Struct *view_main_init(void);
View_Struct *view_bar_init(void);
View_Struct *view_win1_init(void);
View_Struct *view_win2_init(void);

#endif