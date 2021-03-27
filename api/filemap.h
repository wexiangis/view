/*
 *  文件内存映射工具,创建或获取指定文件的内存块
 *  支持文件、framebuffer、video设备的映射
 */
#ifndef _FILEMAP_H_
#define _FILEMAP_H_

#include <stdlib.h>
#include <stdint.h>
#include <linux/fb.h>

#ifdef __cplusplus
extern "C"
{
#endif

typedef enum
{
    FMT_R = 0, // 只读模式得到的mem指针绝对不能写,否则段错误
    FMT_RW,
    FMT_NEW,
} FileMap_Type;

typedef struct
{
    FileMap_Type type;
    int fd;
    int size;
    uint8_t *mem;
} FileMap_Struct;

typedef struct
{
    int fd;
    uint8_t *fb;
    size_t size;
    //获取屏幕宽高 fbInfo.xres fbInfo.yres
    struct fb_var_screeninfo fbInfo;
    //每像素字节数
    int bpp;
    //bytes of width, height
    int bw, bh;
} FbMap_Struct;

typedef struct
{
    uint8_t *buff;
    size_t size;
} CameraMap_Frame;

typedef struct CameraMapStruct
{
    int fd;
    int width;
    int height;
    int runFlag;        // 线程运行同步标志: 0/关闭 1/正在运行 2/请求关闭
    uint32_t timeStamp; // 时间戳毫秒(ms)
    char format[4];     // 图像数据格式""
    int memSize;        // 包含多少块内存
    CameraMap_Frame *mem;
    void *obj;
    void (*callback)(struct CameraMapStruct *, uint8_t *, int);
} CameraMap_Struct;

/*
 *  对文件进行内存映射
 *  参数:
 *      file: 目标文件
 *      type: 读写权限类型
 *      size: type=FMT_NEW 时传入创建文件内存大小,其它时候无效
 *  返回: 返回结构体指针,失败NULL
 */
FileMap_Struct *fileMap_open(char *file, FileMap_Type type, int size);

/*
 *  framebuffer设备内存获取
 *  参数:
 *      fbDev: fb设备,例如 /dev/fb0
 *  返回: 指针结构体,NULL失败
 */
FbMap_Struct *fbMap_open(char *fbDev);

/*
 *  /dev/videoX设备内存映射,顺便作数据流读取回调
 *  参数:
 *      videoDev: 目标设备,示例"/dev/video0"
 *      type: 指定数据格式,如:"JPEG" "H264" "MJPG" "BGR3" ...
 *      obj: 私有参数
 *      callback: 数据就绪回调
 *  返回: 返回结构体指针,失败NULL
 */
CameraMap_Struct *cameraMap_open(
    char *videoDev,
    char type[4],
    int width,
    int height,
    void *obj,
    void (*callback)(CameraMap_Struct *, uint8_t *, int));

/*
 *  关闭内存映射和文件
 */
void fileMap_close(FileMap_Struct *fms);
void fbMap_close(FbMap_Struct *fms);
void cameraMap_close(CameraMap_Struct *cms);

#ifdef __cplusplus
}
#endif

#endif
