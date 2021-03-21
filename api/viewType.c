
#include <viewType.h>

//========== 基本数据的 生成/重设/释放 方法 ==========

/*
 *  通用数据类型初始化
 *  参数:
 *      name: 最大 VIEW_VALUE_NAME_LEN 字节的view名称存储区
 *      type: 数据类型
 *      valueNum: 告知后面的变长参数的数据个数
 *      ...: 根据 type 类型填入特定类型的数据,注意浮点一定要加小数点,如3写成3.0或3lf
 */
ViewValue_Format *viewValue_init(char *name, ViewValue_Type type, int valueNum, ...)
{
    ViewValue_Format *vvf;
    va_list ap;
    int i, j;
    char *charP;

    if (name == NULL)
    {
        fprintf(stderr, "viewValue_init: err, name = NULL !!\r\n");
        return NULL;
    }
    else if (!(type > VT_NULL && type < VT_END))
    {
        fprintf(stderr, "viewValue_init: %s err, unknow type !!\r\n", name);
        return NULL;
    }
    else if (strlen(name) > VIEW_VALUE_NAME_LEN)
    {
        fprintf(stderr, "viewValue_init: %s err, name strlen > %d !!\r\n", name, VIEW_VALUE_NAME_LEN);
        return NULL;
    }

    vvf = (ViewValue_Format *)calloc(1, sizeof(ViewValue_Format));
    strcpy(vvf->name, name);

    vvf->type = type;
    va_start(ap, valueNum);
    switch (type)
    {
    case VT_CHAR:
        vvf->value.Char = (char)va_arg(ap, int);
        vvf->vSize = sizeof(char);
        break;
    case VT_STRING:
        charP = va_arg(ap, char *);
        if (charP) //防止传入NULL
        {
            vvf->value.String = (char *)calloc(strlen(charP) + 1, sizeof(char));
            strcpy(vvf->value.String, charP);
            vvf->vSize = strlen(vvf->value.String);
        }
        else
        {
            vvf->value.String = (char *)calloc(1, sizeof(char));
            vvf->vSize = 0;
        }
        break;
    case VT_STRING_ARRAY:
        vvf->value.StringArray = (char **)calloc(valueNum + 1, sizeof(char *));
        vvf->vSize = valueNum * sizeof(char *);
        for (i = 0, j = 0; i < valueNum; i++)
        {
            charP = va_arg(ap, char *);
            if (charP)
            {
                vvf->value.StringArray[j] = (char *)calloc(strlen(charP) + 1, sizeof(char));
                strcpy(vvf->value.StringArray[j], charP);
                j += 1;
            }
        }
        break;
    case VT_INT:
        vvf->value.Int = va_arg(ap, int);
        vvf->vSize = sizeof(int);
        break;
    case VT_INT_ARRAY:
        vvf->value.IntArray = (int *)calloc(valueNum + 1, sizeof(int));
        vvf->vSize = valueNum * sizeof(int);
        for (i = 0; i < valueNum; i++)
            vvf->value.IntArray[i] = va_arg(ap, int);
        break;
    case VT_DOUBLE:
        vvf->value.Double = va_arg(ap, double);
        vvf->vSize = sizeof(double);
        break;
    case VT_DOUBLE_ARRAY:
        vvf->value.DoubleArray = (double *)calloc(valueNum + 1, sizeof(double));
        vvf->vSize = valueNum * sizeof(double);
        for (i = 0; i < valueNum; i++)
            vvf->value.DoubleArray[i] = va_arg(ap, double);
        break;
    case VT_BOOL:
        vvf->value.Bool = (va_arg(ap, int) ? true : false); //... 没有提供bool类型,可用int代替
        vvf->vSize = sizeof(bool);
        break;
    case VT_BOOL_ARRAY:
        vvf->value.BoolArray = (bool *)calloc(valueNum + 1, sizeof(bool));
        vvf->vSize = valueNum * sizeof(bool);
        for (i = 0; i < valueNum; i++)
            vvf->value.BoolArray[i] = (va_arg(ap, int) ? true : false);
        break;
    case VT_POINT:
        vvf->value.Point = va_arg(ap, void *);
        vvf->vSize = sizeof(void *);
        break;
    case VT_POINT_ARRAY:
        vvf->value.PointArray = (void **)calloc(valueNum + 1, sizeof(void *));
        vvf->vSize = valueNum * sizeof(void *);
        for (i = 0; i < valueNum; i++)
            vvf->value.PointArray[i] = va_arg(ap, void *);
        break;
    default:
        free(vvf);
        vvf = NULL;
        break;
    }
    va_end(ap);
    //
    // pthread_mutex_init(&vvf->lock, NULL);
    //
    return vvf;
}

///
void _viewValue_value_release(ViewValue_Format *vvf)
{
    int i;
    //
    if (vvf == NULL)
        return;
    //
    switch (vvf->type)
    {
    case VT_CHAR:
        vvf->value.Char = 0;
        vvf->vSize = 0;
        break;
    case VT_STRING:
        if (vvf->value.String)
            free(vvf->value.String);
        vvf->value.String = NULL;
        vvf->vSize = 0;
        break;
    case VT_STRING_ARRAY:
        if (vvf->value.StringArray)
        {
            for (i = 0; i < vvf->vSize / sizeof(char *); i++)
            {
                if (vvf->value.StringArray[i])
                    free(vvf->value.StringArray[i]);
            }
            free(vvf->value.StringArray);
        }
        vvf->value.StringArray = NULL;
        vvf->vSize = 0;
        break;
    case VT_INT:
        vvf->value.Int = 0;
        vvf->vSize = 0;
        break;
    case VT_INT_ARRAY:
        if (vvf->value.IntArray)
            free(vvf->value.IntArray);
        vvf->value.IntArray = NULL;
        vvf->vSize = 0;
        break;
    case VT_DOUBLE:
        vvf->value.Double = 0;
        vvf->vSize = 0;
        break;
    case VT_DOUBLE_ARRAY:
        if (vvf->value.DoubleArray)
            free(vvf->value.DoubleArray);
        vvf->value.DoubleArray = NULL;
        vvf->vSize = 0;
        break;
    case VT_BOOL:
        vvf->value.Bool = false; //... 没有提供bool类型,可用int代替
        vvf->vSize = 0;
        break;
    case VT_BOOL_ARRAY:
        if (vvf->value.BoolArray)
            free(vvf->value.BoolArray);
        vvf->value.BoolArray = NULL;
        vvf->vSize = 0;
        break;
    case VT_POINT:
        vvf->value.Point = NULL;
        vvf->vSize = 0;
        break;
    case VT_POINT_ARRAY:
        if (vvf->value.PointArray)
            free(vvf->value.PointArray);
        vvf->value.PointArray = NULL;
        vvf->vSize = 0;
        break;
    default:
        break;
    }
}

///
void viewValue_release(ViewValue_Format *vvf)
{
    if (vvf == NULL)
        return;
    //
    memset(vvf->name, 0, sizeof(vvf->name));
    //
    _viewValue_value_release(vvf);
    //
    viewValue_release(vvf->next);
    //
    // pthread_mutex_destroy(&vvf->lock);
}

///
ViewValue_Format *viewValue_reset(ViewValue_Format *vvf, char *name, ViewValue_Type type, int valueNum, ...)
{
    va_list ap;
    int i, j;
    char *charP;
    //
    if (vvf == NULL)
        return vvf;
    //
    _viewValue_value_release(vvf);
    //
    if (name)
    {
        memset(vvf->name, 0, sizeof(vvf->name));
        strcpy(vvf->name, name);
    }
    //
    vvf->type = type;
    va_start(ap, valueNum);
    switch (type)
    {
    case VT_CHAR:
        vvf->value.Char = (char)va_arg(ap, int);
        vvf->vSize = sizeof(char);
        break;
    case VT_STRING:
        charP = va_arg(ap, char *);
        if (charP) //防止传入NULL
        {
            vvf->value.String = (char *)calloc(strlen(charP) + 1, sizeof(char));
            strcpy(vvf->value.String, charP);
            vvf->vSize = strlen(vvf->value.String);
        }
        else
        {
            vvf->value.String = (char *)calloc(1, sizeof(char));
            vvf->vSize = 0;
        }
        break;
    case VT_STRING_ARRAY:
        vvf->value.StringArray = (char **)calloc(valueNum + 1, sizeof(char *));
        vvf->vSize = valueNum * sizeof(char *);
        for (i = 0, j = 0; i < valueNum; i++)
        {
            charP = va_arg(ap, char *);
            if (charP)
            {
                vvf->value.StringArray[j] = (char *)calloc(strlen(charP) + 1, sizeof(char));
                strcpy(vvf->value.StringArray[j], charP);
                j += 1;
            }
        }
        break;
    case VT_INT:
        vvf->value.Int = va_arg(ap, int);
        vvf->vSize = sizeof(int);
        break;
    case VT_INT_ARRAY:
        vvf->value.IntArray = (int *)calloc(valueNum + 1, sizeof(int));
        vvf->vSize = valueNum * sizeof(int);
        for (i = 0; i < valueNum; i++)
            vvf->value.IntArray[i] = va_arg(ap, int);
        break;
    case VT_DOUBLE:
        vvf->value.Double = va_arg(ap, double);
        vvf->vSize = sizeof(double);
        break;
    case VT_DOUBLE_ARRAY:
        vvf->value.DoubleArray = (double *)calloc(valueNum + 1, sizeof(double));
        vvf->vSize = valueNum * sizeof(double);
        for (i = 0; i < valueNum; i++)
            vvf->value.DoubleArray[i] = va_arg(ap, double);
        break;
    case VT_BOOL:
        vvf->value.Bool = (va_arg(ap, int) ? true : false); //... 没有提供bool类型,可用int代替
        vvf->vSize = sizeof(bool);
        break;
    case VT_BOOL_ARRAY:
        vvf->value.BoolArray = (bool *)calloc(valueNum + 1, sizeof(bool));
        vvf->vSize = valueNum * sizeof(bool);
        for (i = 0; i < valueNum; i++)
            vvf->value.BoolArray[i] = (va_arg(ap, int) ? true : false);
        break;
    case VT_POINT:
        vvf->value.Point = va_arg(ap, void *);
        vvf->vSize = sizeof(void *);
        break;
    case VT_POINT_ARRAY:
        vvf->value.PointArray = (void **)calloc(valueNum + 1, sizeof(void *));
        vvf->vSize = valueNum * sizeof(void *);
        for (i = 0; i < valueNum; i++)
            vvf->value.PointArray[i] = va_arg(ap, void *);
        break;
    default:
        break;
    }
    va_end(ap);
    //
    return vvf;
}

///
bool _viewValue_compare(char *str1, char *str2)
{
    int i;
    if (!str1 || !str2)
        return false;
    for (i = 0; str1[i] && str2[i]; i++)
    {
        if (str1[i] != str2[i])
            break;
    }
    if (str1[i] == 0 && str2[i] == 0)
        return true;
    return false;
}

///
bool viewValue_compare(ViewValue_Format *vvf, ViewValue_Format *vvfArray, int *retNum)
{
    int i;
    //
    if (vvf == NULL || vvfArray == NULL)
        return false;
    //
    if (retNum)
        *retNum = 0;
    //
    switch (vvf->type)
    {
    case VT_CHAR:
        if (vvfArray->type == VT_CHAR &&
            vvf->value.Char == vvfArray->value.Char)
            return true;
        else if (vvfArray->type == VT_STRING)
        {
            for (i = 0; i < vvfArray->vSize / sizeof(char); i++)
            {
                if (vvf->value.Char == vvfArray->value.String[i])
                {
                    if (retNum)
                        *retNum = i;
                    return true;
                }
            }
        }
        break;
    case VT_STRING:
        if (vvfArray->type == VT_STRING &&
            _viewValue_compare(vvf->value.String, vvfArray->value.String))
            return true;
        else if (vvfArray->type == VT_STRING_ARRAY)
        {
            for (i = 0; i < vvfArray->vSize / sizeof(char *); i++)
            {
                if (_viewValue_compare(vvf->value.String, vvfArray->value.StringArray[i]))
                {
                    if (retNum)
                        *retNum = i;
                    return true;
                }
            }
        }
        break;
    case VT_INT:
        if (vvfArray->type == VT_INT &&
            vvf->value.Int == vvfArray->value.Int)
            return true;
        else if (vvfArray->type == VT_INT_ARRAY)
        {
            for (i = 0; i < vvfArray->vSize / sizeof(int); i++)
            {
                if (vvf->value.Int == vvfArray->value.IntArray[i])
                {
                    if (retNum)
                        *retNum = i;
                    return true;
                }
            }
        }
        break;
    case VT_DOUBLE:
        if (vvfArray->type == VT_DOUBLE &&
            vvf->value.Double == vvfArray->value.Double)
            return true;
        else if (vvfArray->type == VT_DOUBLE_ARRAY)
        {
            for (i = 0; i < vvfArray->vSize / sizeof(double); i++)
            {
                if (vvf->value.Double == vvfArray->value.DoubleArray[i])
                {
                    if (retNum)
                        *retNum = i;
                    return true;
                }
            }
        }
        break;
    case VT_BOOL:
        if (vvfArray->type == VT_BOOL &&
            vvf->value.Bool == vvfArray->value.Bool)
            return true;
        else if (vvfArray->type == VT_BOOL_ARRAY)
        {
            for (i = 0; i < vvfArray->vSize / sizeof(bool); i++)
            {
                if (vvf->value.Bool == vvfArray->value.BoolArray[i])
                {
                    if (retNum)
                        *retNum = i;
                    return true;
                }
            }
        }
        break;
    case VT_POINT:
        if (vvfArray->type == VT_POINT &&
            vvf->value.Point == vvfArray->value.Point)
            return true;
        else if (vvfArray->type == VT_POINT_ARRAY)
        {
            for (i = 0; i < vvfArray->vSize / sizeof(void *); i++)
            {
                if (vvf->value.Point == vvfArray->value.PointArray[i])
                {
                    if (retNum)
                        *retNum = i;
                    return true;
                }
            }
        }
        break;
    case VT_STRING_ARRAY:
    case VT_INT_ARRAY:
    case VT_DOUBLE_ARRAY:
    case VT_BOOL_ARRAY:
    case VT_POINT_ARRAY:
    default:
        break;
    }
    //
    return false;
}

int viewValue_find(ViewValue_Format *vvfArray, ...)
{
    va_list ap;
    int i;
    int ret = -1;
    void *vp;
    //
    if (vvfArray == NULL)
        return ret;
    //
    va_start(ap, vvfArray);
    switch (vvfArray->type)
    {
    case VT_CHAR:
        if (vvfArray->value.Char == va_arg(ap, int))
            ret = 0;
        break;
    case VT_STRING:
        if (_viewValue_compare(vvfArray->value.String, va_arg(ap, char *)))
            ret = 0;
        break;
    case VT_INT:
        if (vvfArray->value.Int == va_arg(ap, int))
            ret = 0;
        break;
    case VT_DOUBLE:
        if (vvfArray->value.Double == va_arg(ap, double))
            ret = 0;
        break;
    case VT_BOOL:
        if (vvfArray->value.Bool && va_arg(ap, int))
            ret = 0;
        else if (!vvfArray->value.Bool && !va_arg(ap, int))
            ret = 0;
        break;
    case VT_POINT:
        if (vvfArray->value.Point == va_arg(ap, void *))
            ret = 0;
        break;
    case VT_STRING_ARRAY:
        vp = va_arg(ap, void *);
        for (i = 0; i < vvfArray->vSize / sizeof(char *); i++)
        {
            if (_viewValue_compare(vvfArray->value.StringArray[i], vp))
            {
                ret = i;
                break;
            }
        }
        break;
    case VT_INT_ARRAY:
        vp = va_arg(ap, void *);
        for (i = 0; i < vvfArray->vSize / sizeof(int *); i++)
        {
            if (vvfArray->value.IntArray[i] == *((int *)vp))
            {
                ret = i;
                break;
            }
        }
        break;
    case VT_DOUBLE_ARRAY:
        vp = va_arg(ap, void *);
        for (i = 0; i < vvfArray->vSize / sizeof(double *); i++)
        {
            if (vvfArray->value.DoubleArray[i] == *((double *)vp))
            {
                ret = i;
                break;
            }
        }
        break;
    case VT_BOOL_ARRAY:
        vp = va_arg(ap, void *);
        for (i = 0; i < vvfArray->vSize / sizeof(bool *); i++)
        {
            if (vvfArray->value.BoolArray[i] == *((bool *)vp))
            {
                ret = i;
                break;
            }
        }
        break;
    case VT_POINT_ARRAY:
        vp = va_arg(ap, void *);
        for (i = 0; i < vvfArray->vSize / sizeof(bool *); i++)
        {
            if (vvfArray->value.PointArray[i] == vp)
            {
                ret = i;
                break;
            }
        }
        break;
    default:
        break;
    }
    va_end(ap);
    //
    return ret;
}

/// 把 vvf2 拷贝到 vvf1 并返回 vvf1
ViewValue_Format *viewValue_copy(ViewValue_Format *vvf1, ViewValue_Format *vvf2)
{
    ViewValue_Format *vvfRet;
    int i, len;
    //
    if (vvf2 == NULL)
        return vvf1;
    //
    if (vvf1)
    {
        _viewValue_value_release(vvf1);
        vvfRet = vvf1;
    }
    else
        vvfRet = (ViewValue_Format *)calloc(1, sizeof(ViewValue_Format));
    //
    memcpy(vvfRet->name, vvf2->name, sizeof(vvfRet->name));
    vvfRet->type = vvf2->type;
    vvfRet->vSize = vvf2->vSize;
    vvfRet->param[0] = vvf2->param[0];
    vvfRet->param[1] = vvf2->param[1];
    switch (vvf2->type)
    {
    case VT_CHAR:
    case VT_INT:
    case VT_DOUBLE:
    case VT_BOOL:
    case VT_POINT:
        memcpy(&vvfRet->value, &vvf2->value, sizeof(vvfRet->value));
        break;
    case VT_STRING:
        vvfRet->value.String = (char *)calloc(vvf2->vSize / sizeof(char) + 1, sizeof(char));
        memcpy(vvfRet->value.String, vvf2->value.String, vvf2->vSize / sizeof(char));
        break;
    case VT_INT_ARRAY:
        vvfRet->value.IntArray = (int *)calloc(vvf2->vSize / sizeof(int) + 1, sizeof(int));
        memcpy(vvfRet->value.IntArray, vvf2->value.IntArray, vvf2->vSize / sizeof(int));
        break;
    case VT_DOUBLE_ARRAY:
        vvfRet->value.DoubleArray = (double *)calloc(vvf2->vSize / sizeof(double) + 1, sizeof(double));
        memcpy(vvfRet->value.DoubleArray, vvf2->value.DoubleArray, vvf2->vSize / sizeof(double));
        break;
    case VT_BOOL_ARRAY:
        vvfRet->value.BoolArray = (bool *)calloc(vvf2->vSize / sizeof(bool) + 1, sizeof(bool));
        memcpy(vvfRet->value.BoolArray, vvf2->value.BoolArray, vvf2->vSize / sizeof(bool));
        break;
    case VT_STRING_ARRAY:
        vvfRet->value.StringArray = (char **)calloc(vvf2->vSize / sizeof(char *) + 1, sizeof(char *));
        for (i = 0; i < vvf2->vSize / sizeof(char *); i++)
        {
            len = strlen(vvf2->value.StringArray[i]);
            vvfRet->value.StringArray[i] = (char *)calloc(len + 1, sizeof(char));
            memcpy(vvfRet->value.StringArray[i], vvf2->value.StringArray[i], len);
        }
        break;
    case VT_POINT_ARRAY:
        vvfRet->value.PointArray = (void **)calloc(vvf2->vSize / sizeof(void *) + 1, sizeof(void *));
        memcpy(vvfRet->value.PointArray, vvf2->value.PointArray, vvf2->vSize / sizeof(void *));
        break;
    default:
        break;
    }
    return vvfRet;
}

ViewValue_Format *viewValue_arrayAdd(ViewValue_Format *vvf, ...)
{
    va_list ap;
    void **tempArray;
    void *tempPoi;
    int len;
    //
    if (vvf == NULL)
        return NULL;
    //
    va_start(ap, vvf);
    switch (vvf->type)
    {
    case VT_STRING_ARRAY:
        len = vvf->vSize / sizeof(char *);
        tempArray = (void **)calloc(len + 1 + 1, sizeof(char *));
        //拷贝原有值
        memcpy(tempArray, vvf->value.PointArray, len * sizeof(char *));
        //
        tempPoi = va_arg(ap, char *);
        if (tempPoi)
        {
            tempArray[len] = (char *)calloc(strlen(tempPoi) + 1, sizeof(char));
            strcpy(tempArray[len], tempPoi);
        }
        else
            tempArray[len] = (char *)calloc(1, sizeof(char));
        //移花接木
        free(vvf->value.PointArray);
        vvf->value.PointArray = tempArray;
        vvf->vSize = (len + 1) * sizeof(char *);
        break;
    case VT_POINT_ARRAY:
        len = vvf->vSize / sizeof(void *);
        tempArray = (void **)calloc(len + 1 + 1, sizeof(void *));
        //拷贝原有值
        memcpy(tempArray, vvf->value.PointArray, len * sizeof(void *));
        //
        tempPoi = va_arg(ap, void *);
        tempArray[len] = tempPoi;
        //移花接木
        free(vvf->value.PointArray);
        vvf->value.PointArray = tempArray;
        vvf->vSize = (len + 1) * sizeof(void *);
        break;
    case VT_INT_ARRAY:
        len = vvf->vSize / sizeof(int);
        tempPoi = (int *)calloc(len + 1 + 1, sizeof(int));
        //拷贝原有值
        memcpy(tempPoi, vvf->value.IntArray, len * sizeof(int));
        *(((int *)tempPoi) + len) = va_arg(ap, int);
        //移花接木
        free(vvf->value.IntArray);
        vvf->value.IntArray = (int *)tempPoi;
        vvf->vSize = (len + 1) * sizeof(int);
        break;
    case VT_DOUBLE_ARRAY:
        len = vvf->vSize / sizeof(double);
        tempPoi = (double *)calloc(len + 1 + 1, sizeof(double));
        //拷贝原有值
        memcpy(tempPoi, vvf->value.DoubleArray, len * sizeof(double));
        *(((double *)tempPoi) + len) = va_arg(ap, double);
        //移花接木
        free(vvf->value.DoubleArray);
        vvf->value.DoubleArray = (double *)tempPoi;
        vvf->vSize = (len + 1) * sizeof(double);
        break;
    case VT_BOOL_ARRAY:
        len = vvf->vSize / sizeof(bool);
        tempPoi = (bool *)calloc(len + 1 + 1, sizeof(bool));
        //拷贝原有值
        memcpy(tempPoi, vvf->value.BoolArray, len * sizeof(bool));
        *(((bool *)tempPoi) + len) = (va_arg(ap, int) ? true : false);
        //移花接木
        free(vvf->value.BoolArray);
        vvf->value.BoolArray = (bool *)tempPoi;
        vvf->vSize = (len + 1) * sizeof(bool);
        break;
    default:
        break;
    }
    va_end(ap);
    //
    return vvf;
}

ViewValue_Format *viewValue_arrayRemoveByNum(ViewValue_Format *vvf, int num)
{
    void *tempPoi;
    int count, len;
    //
    if (vvf == NULL || num < 0)
        return NULL;
    //
    switch (vvf->type)
    {
    case VT_STRING_ARRAY:
    case VT_POINT_ARRAY:
        len = vvf->vSize / sizeof(void *);
        if (len > num)
        {
            free(vvf->value.StringArray[num]);
            for (count = num; count < len; count++)
                vvf->value.StringArray[count] = vvf->value.StringArray[count + 1];
            vvf->vSize = (len - 1) * sizeof(void *);
        }
        break;
    case VT_INT_ARRAY:
        len = vvf->vSize / sizeof(int);
        if (len > num)
        {
            tempPoi = (int *)calloc(len, sizeof(int));
            for (count = 0; count < num; count++)
                ((int *)tempPoi)[count] = vvf->value.IntArray[count];
            for (; count < len; count++)
                ((int *)tempPoi)[count] = vvf->value.IntArray[count + 1];
            //移花接木
            free(vvf->value.IntArray);
            vvf->value.IntArray = (int *)tempPoi;
            vvf->vSize = (len - 1) * sizeof(int);
        }
        break;
    case VT_DOUBLE_ARRAY:
        len = vvf->vSize / sizeof(double);
        if (len > num)
        {
            tempPoi = (double *)calloc(len, sizeof(double));
            for (count = 0; count < num; count++)
                ((double *)tempPoi)[count] = vvf->value.DoubleArray[count];
            for (; count < len; count++)
                ((double *)tempPoi)[count] = vvf->value.DoubleArray[count + 1];
            //移花接木
            free(vvf->value.DoubleArray);
            vvf->value.DoubleArray = (double *)tempPoi;
            vvf->vSize = (len - 1) * sizeof(double);
        }
        break;
    case VT_BOOL_ARRAY:
        len = vvf->vSize / sizeof(bool);
        if (len > num)
        {
            tempPoi = (bool *)calloc(len, sizeof(bool));
            for (count = 0; count < num; count++)
                ((bool *)tempPoi)[count] = vvf->value.BoolArray[count];
            for (; count < len; count++)
                ((bool *)tempPoi)[count] = vvf->value.BoolArray[count + 1];
            //移花接木
            free(vvf->value.BoolArray);
            vvf->value.BoolArray = (bool *)tempPoi;
            vvf->vSize = (len - 1) * sizeof(bool);
        }
        break;
    default:
        break;
    }
    //
    return vvf;
}

//============ ViewValue_Format 的文件 读/写 =================

#define VVF_SL_DIV_CHAR '\\' //用反斜杠作为文件内容的分隔符和结束符

/*

[name1\type\size]
value\

[volume\VT_BOOL\1]
0\

[volume\VT_INT\4]
10\

[viewAlpha\VT_DOUBLE\8]
0.70000000\

[battArray\VT_INT_ARRAY\16]
1\2\3\4\

[picName\VT_STRING\9]
cover.bmp\

[picList\VT_STRING_ARRAY\9]
cover.bmp\log.jpg\roverCover.jpg\

*/

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#define VVF_SL_SHOW_HEX 0

//
void _viewValue_save(FILE *fd, ViewValue_Format *vvf)
{
    int j;
    //
    switch (vvf->type)
    {
    case VT_CHAR:
        fprintf(fd, "# %s VT_CHAR %d\n%c\\\n",
                vvf->name, 1, vvf->value.Char);
        break;
    case VT_BOOL:
        fprintf(fd, "# %s VT_BOOL %d\n%c\\\n",
                vvf->name, 1, vvf->value.Bool ? 'T' : 'F');
        break;
    case VT_INT:
#if (VVF_SL_SHOW_HEX)
        fprintf(fd, "# %s VT_INT %d\n%06X\\\n",
                vvf->name, 1, vvf->value.Int);
#else
        fprintf(fd, "# %s VT_INT %d\n%d\\\n",
                vvf->name, 1, vvf->value.Int);
#endif
        break;
    case VT_DOUBLE:
        fprintf(fd, "# %s VT_DOUBLE %d\n%.8lf\\\n",
                vvf->name, 1, vvf->value.Double);
        break;
    case VT_STRING:
        fprintf(fd, "# %s VT_STRING %d\n%s\\\n",
                vvf->name, (int)strlen(vvf->value.String), vvf->value.String);
        break;
    case VT_BOOL_ARRAY:
        fprintf(fd, "# %s VT_BOOL_ARRAY %d\n",
                vvf->name, (int)(vvf->vSize / sizeof(bool)));
        for (j = 0; j < vvf->vSize / sizeof(bool); j++)
            fprintf(fd, "%c\\\n", vvf->value.BoolArray[j] ? 'T' : 'F');
        break;
    case VT_INT_ARRAY:
        fprintf(fd, "# %s VT_INT_ARRAY %d\n",
                vvf->name, (int)(vvf->vSize / sizeof(int)));
        for (j = 0; j < vvf->vSize / sizeof(int); j++)
#if (VVF_SL_SHOW_HEX)
            fprintf(fd, "%06X\\\n", vvf->value.IntArray[j]);
#else
            fprintf(fd, "%d\\\n", vvf->value.IntArray[j]);
#endif
        break;
    case VT_DOUBLE_ARRAY:
        fprintf(fd, "# %s VT_DOUBLE_ARRAY %d\n",
                vvf->name, (int)(vvf->vSize / sizeof(double)));
        for (j = 0; j < vvf->vSize / sizeof(double); j++)
            fprintf(fd, "%.8lf\\\n", vvf->value.DoubleArray[j]);
        break;
    case VT_STRING_ARRAY:
        fprintf(fd, "# %s VT_STRING_ARRAY %d\n",
                vvf->name, (int)(vvf->vSize / sizeof(void *)));
        for (j = 0; j < vvf->vSize / sizeof(void *); j++)
            fprintf(fd, "%s\\\n", vvf->value.StringArray[j]);
        break;
    default:
        break;
    }
    //
    if (vvf->next)
        _viewValue_save(fd, vvf->next);
}

//成功放回 arrayLen
int viewValue_save(char *filePath, ViewValue_Format *array, int arrayLen)
{
    ViewValue_Format *vvf = array;
    //
    if (!filePath || !array || arrayLen < 1)
        return 0;
    //
    FILE *fd;
    if ((fd = fopen(filePath, "w")) == NULL)
    {
        fprintf(stderr, "open write failed ! %s %d\n", filePath, arrayLen);
        return 0;
    }
    //
    int i;
    for (i = 0; i < arrayLen && vvf; i++)
        _viewValue_save(fd, vvf++);
    //
    fclose(fd);
    //
    return arrayLen;
}

//
typedef struct VvfLoadStat
{
    ViewValue_Format *vvf;
    char flag;
    struct VvfLoadStat *next;
} VvfLoadStat_Struct;

//
VvfLoadStat_Struct *VvfLoadStat_browse(ViewValue_Format *array, int *count)
{
    VvfLoadStat_Struct *vlss = NULL;
    //
    if (array)
    {
        *count += 1;
        vlss = (VvfLoadStat_Struct *)calloc(1, sizeof(VvfLoadStat_Struct));
        vlss->vvf = array;
        vlss->next = VvfLoadStat_browse(array->next, count);
    }
    //
    return vlss;
}
//
void VvfLoadStat_release(VvfLoadStat_Struct *vlss)
{
    VvfLoadStat_Struct *current, *next;
    current = vlss;
    while (current)
    {
        if (!current->flag)
            printf("load ignore: %s\n", current->vvf->name);
        next = current->next;
        free(current);
        current = next;
    }
}

#define LOAD_LINE_SIZE 10240

//
int _viewValue_load_string(FILE *fd, char *line, char **str)
{
    int ret = strlen(line);
    //结尾至少会有'\'和'\n'两个字符 此处太短
    if (ret < 3)
        ret = 2;
    //判断字符串是否在该行就结束了 否则继续读行 直至结束或长度达 LOAD_LINE_SIZE
    else if (ret < LOAD_LINE_SIZE)
    {
        while (line[ret - 1] != '\n' || line[ret - 2] != '\\')
        {
            if (fgets(&line[ret], LOAD_LINE_SIZE - ret, fd) == NULL)
            {
                ret += 2; //手动加结束符'\'和'\n'
                break;
            }
            ret += strlen(&line[ret]);
            //
            if (ret >= LOAD_LINE_SIZE)
                ret = LOAD_LINE_SIZE + 2;
        }
    }
    else
        ret = LOAD_LINE_SIZE + 2;
    //要拷贝的字符串总长
    ret -= 2;
    //拷贝字符串
    if (str)
    {
        if (*str)
            free(*str);
        *str = (char *)calloc(ret + 1, 1);
        strncpy(*str, line, ret);
    }
    //
    return ret;
}

//返回成功加载条数 理论为arrayLen
int viewValue_load(char *filePath, ViewValue_Format *array, int arrayLen)
{
    if (!filePath || !array || arrayLen < 1)
        return 0;
    //
    int i, hit = 0, count = 0; //匹配计数
    //指针统计成链表 方便遍历和跳过已匹配的项
    VvfLoadStat_Struct *vlss = NULL, *vlssTemp;
    if ((vlss = vlssTemp = VvfLoadStat_browse(array, &count))) //第一个
    {
        for (i = 1; i < arrayLen; i++)
        {
            while (vlssTemp->next)
                vlssTemp = vlssTemp->next;
            vlssTemp->next = VvfLoadStat_browse(&array[i], &count);
        }
    }
    //
    FILE *fd;
    if ((fd = fopen(filePath, "r")) == NULL)
    {
        fprintf(stderr, "open read failed ! %s %d\n", filePath, arrayLen);
        return 0;
    }
    //行缓存
    char line[LOAD_LINE_SIZE + 1], typeStr[16];
    //存储临时解析的数据解析数据
    ViewValue_Format vvfTemp, *vvfPoint;
    //用于比较
    int vSize;
    //
    while (1)
    {
        if (feof(fd))
            break;
        //获取一行数据
        memset(line, 0, LOAD_LINE_SIZE);
        if (fgets(line, LOAD_LINE_SIZE, fd) == NULL)
            break;
        //判断是否是数据头 //如:[name1\type\size]
        if (line[0] == '#')
        {
            memset(&vvfTemp, 0, sizeof(ViewValue_Format));
            memset(typeStr, 0, sizeof(typeStr));
            if (sscanf(line, "%*s %s %s %d",
                       vvfTemp.name, typeStr, &vvfTemp.vSize) == 3)
            {
                //
                if (typeStr[3] == 'C')
                {
                    vvfTemp.type = VT_CHAR;
                    vSize = vvfTemp.vSize * sizeof(char);
                }
                else if (typeStr[3] == 'S')
                {
                    if (typeStr[10] == 'A')
                    {
                        vvfTemp.type = VT_STRING_ARRAY;
                        vSize = vvfTemp.vSize * sizeof(void *);
                    }
                    else
                    {
                        vvfTemp.type = VT_STRING;
                        vSize = vvfTemp.vSize * 1;
                    }
                }
                else if (typeStr[3] == 'I')
                {
                    if (typeStr[7] == 'A')
                    {
                        vvfTemp.type = VT_INT_ARRAY;
                        vSize = vvfTemp.vSize * sizeof(void *);
                    }
                    else
                    {
                        vvfTemp.type = VT_INT;
                        vSize = vvfTemp.vSize * sizeof(int);
                    }
                }
                else if (typeStr[3] == 'D')
                {
                    if (typeStr[10] == 'A')
                    {
                        vvfTemp.type = VT_DOUBLE_ARRAY;
                        vSize = vvfTemp.vSize * sizeof(void *);
                    }
                    else
                    {
                        vvfTemp.type = VT_DOUBLE;
                        vSize = vvfTemp.vSize * sizeof(double);
                    }
                }
                else if (typeStr[3] == 'B')
                {
                    if (typeStr[8] == 'A')
                    {
                        vvfTemp.type = VT_BOOL_ARRAY;
                        vSize = vvfTemp.vSize * sizeof(void *);
                    }
                    else
                    {
                        vvfTemp.type = VT_BOOL;
                        vSize = vvfTemp.vSize * sizeof(bool);
                    }
                }
                else
                {
                    vvfTemp.type = VT_NULL;
                    vSize = 0;
                }

                //匹配 name 和 type 定位到目标变量指针
                vlssTemp = vlss;
                while (vlssTemp)
                {
                    //比较
                    if (!vlssTemp->flag &&
                        vvfTemp.type == vlssTemp->vvf->type &&
                        memcmp(vvfTemp.name, vlssTemp->vvf->name, VIEW_VALUE_NAME_LEN - 1) == 0)
                    {
                        if (vvfTemp.type != VT_STRING_ARRAY &&
                            vvfTemp.type != VT_POINT_ARRAY)
                            break;
                        //对于指针数组类,其条目不得小于默认条目
                        else if (vSize >= vlssTemp->vvf->vSize)
                            break;
                        else
                        {
                            printf("viewValue_load: name/%s, type/%d, vSize/%d < defalut vSize/%d\n",
                                   vvfTemp.name, vvfTemp.type, vSize, vlssTemp->vvf->vSize);
                        }
                    }
                    //
                    vlssTemp = vlssTemp->next;
                }
                //成功匹配
                if (vlssTemp)
                {
                    hit += 1;           //计数加
                    vlssTemp->flag = 1; //已匹配
                    vvfPoint = vlssTemp->vvf;
                    //读文件数据部分
                    memset(line, 0, LOAD_LINE_SIZE);
                    //分类型赋值
                    switch (vvfPoint->type)
                    {
                    case VT_CHAR:
                        if (fgets(line, LOAD_LINE_SIZE, fd) == NULL)
                            break;
                        vvfPoint->value.Char = line[0];
                        vvfPoint->vSize = 1;
                        break;
                    case VT_BOOL:
                        if (fgets(line, LOAD_LINE_SIZE, fd) == NULL)
                            break;
                        vvfPoint->value.Bool = line[0] == 'T' ? true : false;
                        vvfPoint->vSize = sizeof(bool);
                        break;
                    case VT_INT:
                        if (fgets(line, LOAD_LINE_SIZE, fd) == NULL)
                            break;
#if (VVF_SL_SHOW_HEX)
                        sscanf(line, "%X", &vvfPoint->value.Int);
#else
                        sscanf(line, "%d", &vvfPoint->value.Int);
#endif
                        vvfPoint->vSize = sizeof(int);
                        break;
                    case VT_DOUBLE:
                        if (fgets(line, LOAD_LINE_SIZE, fd) == NULL)
                            break;
                        sscanf(line, "%lf", &vvfPoint->value.Double);
                        vvfPoint->vSize = sizeof(double);
                        break;
                    case VT_BOOL_ARRAY:
                        //重新分配内存
                        if (vvfPoint->value.BoolArray)
                            free(vvfPoint->value.BoolArray);
                        vvfPoint->value.BoolArray = (bool *)calloc(vvfTemp.vSize + 1, sizeof(bool));
                        //每行读数据
                        vvfPoint->vSize = vvfTemp.vSize * sizeof(bool);
                        for (i = 0; i < vvfTemp.vSize; i++)
                        {
                            //获取一行数据
                            memset(line, 0, LOAD_LINE_SIZE);
                            if (fgets(line, LOAD_LINE_SIZE, fd) == NULL)
                                break;
                            vvfPoint->value.BoolArray[i] = line[0] == 'T' ? true : false;
                        }
                        break;
                    case VT_INT_ARRAY:
                        //重新分配内存
                        if (vvfPoint->value.IntArray)
                            free(vvfPoint->value.IntArray);
                        vvfPoint->value.IntArray = (int *)calloc(vvfTemp.vSize + 1, sizeof(int));
                        vvfPoint->vSize = vvfTemp.vSize * sizeof(int);
                        //每行读数据
                        for (i = 0; i < vvfTemp.vSize; i++)
                        {
                            //获取一行数据
                            memset(line, 0, LOAD_LINE_SIZE);
                            if (fgets(line, LOAD_LINE_SIZE, fd) == NULL)
                                break;
#if (VVF_SL_SHOW_HEX)
                            sscanf(line, "%X", &vvfPoint->value.IntArray[i]);
#else
                            sscanf(line, "%d", &vvfPoint->value.IntArray[i]);
#endif
                        }
                        break;
                    case VT_DOUBLE_ARRAY:
                        //重新分配内存
                        if (vvfPoint->value.DoubleArray)
                            free(vvfPoint->value.DoubleArray);
                        vvfPoint->value.DoubleArray = (double *)calloc(vvfTemp.vSize + 1, sizeof(double));
                        vvfPoint->vSize = vvfTemp.vSize * sizeof(double);
                        //每行读数据
                        for (i = 0; i < vvfTemp.vSize; i++)
                        {
                            //获取一行数据
                            memset(line, 0, LOAD_LINE_SIZE);
                            if (fgets(line, LOAD_LINE_SIZE, fd) == NULL)
                                break;
                            sscanf(line, "%lf", &vvfPoint->value.DoubleArray[i]);
                        }
                        break;
                    case VT_STRING:
                        if (fgets(line, LOAD_LINE_SIZE, fd) == NULL)
                            break;
                        vvfPoint->vSize = _viewValue_load_string(fd, line, &vvfPoint->value.String);
                        break;
                    case VT_STRING_ARRAY:
                        //重新分配内存
                        if (vvfPoint->value.StringArray)
                            free(vvfPoint->value.StringArray);
                        vvfPoint->value.StringArray = (char **)calloc(vvfTemp.vSize + 1, sizeof(void *));
                        vvfPoint->vSize = vvfTemp.vSize * sizeof(void *);
                        //每行读数据
                        for (i = 0; i < vvfTemp.vSize; i++)
                        {
                            //获取一行数据
                            memset(line, 0, LOAD_LINE_SIZE);
                            if (fgets(line, LOAD_LINE_SIZE, fd) == NULL)
                                break;
                            _viewValue_load_string(fd, line, &vvfPoint->value.StringArray[i]);
                        }
                        break;
                    default:
                        break;
                    }
                }
            }
        }
    }
    //
    fclose(fd);
    VvfLoadStat_release(vlss);
    //
    return hit;
}
