/*
 *  提示弹窗: 按指定滞留时间,显示指定颜色,指定的内容
 */
#include "viewapi.h"

static int view_tips_vStart(View_Struct *view, void *object, View_Focus *focus, ViewButtonTouch_Event *event)
{
    //超时检查
    if (view_tickMs() > view->textBakup->value.Int)
    {
        //删除当前view
        view_delete(view);
        //跳过本view绘制
        return CALLBACK_IGNORE;
    }
    return CALLBACK_OK;
}

/*
 *  清除提示弹窗
 *  参数:
 *      window: 目标窗口
 *      msg: 检索内容,为NULL时通配
 *      color: 检索文字颜色,为0时通配
 */
void view_tips_clean(View_Struct *window, char *msg, uint32_t color)
{
    View_Struct *view = window->view;
    //遍历链表
    while(view)
    {
        //匹配控件名称
        if (view->name[0] && strncmp(view->name, "tips", 4) == 0)
        {
            //匹配内容
            if (!msg || strncmp(view->text->value.String, msg, strlen(msg)) == 0)
            {
                //匹配颜色
                if (color == 0 || view->textColor == color)
                {
                    //超时置0
                    view->textBakup->value.Int = 0;
                }
            }
        }
        //下一个
        view = view->next;
    }
}

/*
 *  添加提示弹窗
 *  参数:
 *      window: 目标窗口
 *      msg: 内容
 *      color: 文字颜色
 *      delayms: 滞留时长
 */
void view_tips_add(View_Struct *window, char *msg, uint32_t color, uint32_t delayms)
{
    View_Struct *vs;
    //检查
    if (!msg)
        return;
    //占用半屏
    vs = view_init("tips", VWHT_MATCH, VWHT_MATCH * 2, 0, 0);
    vs->centerY = true;
    //半透明灰色色覆盖背景
    vs->backGroundColor = 0x20404040;
    vs->backGroundRad = 15;
    //文字
    vs->text = viewValue_init(NULL, VT_STRING, 1, msg);
    vs->textColor = color;
    vs->textEdgeX = 1;
    vs->textEdgeY = 1;
    vs->textSize = ViewSrc.Content_Type.value.Int * 10;
    //目标时间,到达这个时间则关闭窗口
    vs->textBakup = viewValue_init(NULL, VT_INT, 1, view_tickMs() + delayms);
    //绘制前回调
    vs->viewStart = (ViewCallBack)&view_tips_vStart;

    //添加链表最后面(即显示在最上面)
    view_add(window, vs, false);
}
