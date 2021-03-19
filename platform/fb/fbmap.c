#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/mman.h>

#include <poll.h>
#include <pthread.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <linux/videodev2.h>

#include "fbmap.h"

//抛线程工具
static void new_thread(void *obj, void *callback)
{
    pthread_t th;
    pthread_attr_t attr;
    int ret;
    //禁用线程同步,线程运行结束后自动释放
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    //抛出线程
    ret = pthread_create(&th, &attr, callback, (void *)obj);
    if (ret != 0)
        printf("new_thread failed !! %s\r\n", strerror(ret));
    //attr destroy
    pthread_attr_destroy(&attr);
}

/*
 *
 */
void fbmapCamera_stream_on(CameraMap_Struct *cms)
{
    enum v4l2_buf_type type;
    type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (ioctl(cms->fd, VIDIOC_STREAMON, &type) < 0)
    {
        fprintf(stderr, "ioctl VIDIOC_STREAMON error (%d)\r\n", errno);
    }
}
void fbmapCamera_stream_off(CameraMap_Struct *cms)
{
    enum v4l2_buf_type type;
    type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (ioctl(cms->fd, VIDIOC_STREAMOFF, &type) < 0)
    {
        fprintf(stderr, "ioctl VIDIOC_STREAMON error (%d)\r\n", errno);
    }
}

/*
 *
 */
void fbmapCamera_read(CameraMap_Struct *cms)
{
    struct v4l2_buffer buffer = {0};

    buffer.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buffer.memory = V4L2_MEMORY_MMAP;

    // 检查是哪一块内存接收数据
    if (ioctl(cms->fd, VIDIOC_DQBUF, &buffer) < 0)
    {
        fprintf(stderr, "ioctl VIDIOC_DQBUF error (%d)\r\n", errno);
        return;
    }

    // 块有效检查
    if (buffer.index < cms->memSize)
    {
        // 解析时间戳
        cms->timeStamp =
            buffer.timestamp.tv_sec * 1000 + buffer.timestamp.tv_usec / 1000;

        // 得到内存,回调
        if (cms->callback)
            cms->callback(cms, cms->mem[buffer.index].buff, buffer.bytesused);

        // 投放一个空的视频缓冲区到视频缓冲区输入队列中
        if (ioctl(cms->fd, VIDIOC_QBUF, &buffer) < 0)
        {
            fprintf(stderr, "ioctl VIDIOC_QBUF error (%d)\r\n", errno);
            return;
        }
    }
}

/*
 *
 */
void fbmapCamera_callback(void *argv)
{
    CameraMap_Struct *cms = (CameraMap_Struct *)argv;
    struct v4l2_buffer buffer;
    int count;

#if 0 // select
    fd_set fds;
    struct timeval tv = {
        .tv_sec = 0,
        .tv_usec = 200000, // 200ms超时
    };
#else // poll
    struct pollfd fds[1];
    fds[0].fd = cms->fd;
    fds[0].events = POLLIN;
#endif

    // 投放一个空的视频缓冲区到视频缓冲区输入队列中
    for (count = 0; count < cms->memSize; count++)
    {
        memset(&buffer, 0, sizeof(buffer));
        buffer.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buffer.memory = V4L2_MEMORY_MMAP;
        buffer.index = count;
        if (ioctl(cms->fd, VIDIOC_QBUF, &buffer) < 0)
        {
            fprintf(stderr, "ioctl VIDIOC_QBUF error (%d)\r\n", errno);
        }
    }
    // 开始数据流
    fbmapCamera_stream_on(cms);

    while (cms->runFlag == 1)
    {
#if 0 // select
        FD_ZERO(&fds);
        FD_SET(cms->fd, &fds);
        if (select(cms->fd + 1, &fds, NULL, NULL, &tv) > 0)
        {
            //读取数据
            fbmapCamera_read(cms);
        }
#else // poll
        if ((count = poll(fds, 1, 200)) < 0)
        {
            fprintf(stderr, "poll error %d \r\n", count);
        }
        if (count > 0 && fds[0].revents)
        {
            //读取数据
            fbmapCamera_read(cms);
        }
#endif
    }

    // 关闭数据流
    fbmapCamera_stream_off(cms);

    //清运行状态
    cms->runFlag = 0;
}

/*
 *  对文件进行内存映射
 *  参数:
 *      file: 目标文件
 *      type: 读写权限类型
 *      size: type=FMT_NEW 时传入创建文件内存大小,其它时候无效
 *  返回: 返回结构体指针,失败NULL
 */
FileMap_Struct *fileMap_open(char *file, FileMap_Type type, int size)
{
    FileMap_Struct *fms;
    uint8_t *mem;
    int fd;
    int prot;
    struct stat info;

    int bytesCount;
    uint8_t bytes = 0;

    // 参数检查
    if (!file)
    {
        fprintf(stderr, "fileMap_open: param error !! \r\n");
        return NULL;
    }

    // 打开文件
    switch (type)
    {
    case FMT_R:
        prot = PROT_READ;
        fd = open(file, O_RDONLY);
        break;
    case FMT_RW:
        prot = PROT_READ | PROT_WRITE;
        fd = open(file, O_RDWR);
        break;
    case FMT_NEW:
        prot = PROT_READ | PROT_WRITE;
        fd = open(file, O_RDWR | O_CREAT | O_TRUNC, 0666);
        // 创建文件时,指定文件大小
        if (size < 1)
            size = 1;
        // 拓宽文件到目标大小
        if (fd > 0)
        {
            for (bytesCount = 0; bytesCount < size; bytesCount++)
                write(fd, &bytes, 1);
            // 指针位置恢复到文件头
            lseek(fd, 0, SEEK_CUR);
        }
        break;
    default:
        fd = -1;
        break;
    }

    // 检查
    if (fd < 1)
    {
        fprintf(stderr, "fileMap_open: can't open %s \r\n", file);
        return NULL;
    }

    // 检查文件大小
    fstat(fd, &info);
    size = info.st_size;
    printf("file size: %d \r\n", size);

    // 内存映射(第一个0表示不指定内存地址,最后的0表示从文件起始地址开始映射)
    mem = (uint8_t *)mmap(0, size, prot, MAP_SHARED, fd, 0);
    if (!mem)
    {
        fprintf(stderr, "fileMap_open: mmap failed \r\n");
        close(fd);
        return NULL;
    }

    // 返回参数
    fms = (FileMap_Struct *)calloc(1, sizeof(FileMap_Struct));
    fms->mem = mem;
    fms->fd = fd;
    fms->size = size;
    fms->type = type;

    return fms;
}

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
    void (*callback)(CameraMap_Struct *, uint8_t *, int))
{
    CameraMap_Struct *cms;
    int fd;
    int i;

    struct v4l2_capability cap;
    struct v4l2_format fmt;
    struct v4l2_requestbuffers reqBuff;
    struct v4l2_buffer buffer;
    struct v4l2_fmtdesc fmtdesc;

    // 参数检查
    if (!videoDev)
    {
        fprintf(stderr, "fileMap_open: param error !! \r\n");
        return NULL;
    }

    // 打开文件
    fd = open(videoDev, O_RDWR | O_NONBLOCK);
    if (fd < 1)
    {
        fprintf(stderr, "fileMap_open: can't open %s \r\n", videoDev);
        return NULL;
    }

    // 查询视频设备的功能
    memset(&cap, 0, sizeof(cap));
    if (ioctl(fd, VIDIOC_QUERYCAP, &cap) < 0)
    {
        fprintf(stderr, "ioctl VIDIOC_QUERYCAP error (%d)\r\n", errno);
        return NULL;
    }
    // printf("cap: driver %s, card %s, bus %s \r\n", cap.driver, cap.card, cap.bus_info);

    // 是否支持录像
    if (!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE))
    {
        fprintf(stderr, "not a capture device (0x%08X)\r\n", cap.capabilities);
        return NULL;
    }

    // 是否支持流模式
    if (!(cap.capabilities & V4L2_CAP_STREAMING))
    {
        fprintf(stderr, "not a stream device (0x%08X)\r\n", cap.capabilities);
        return NULL;
    }

    memset(&fmtdesc, 0, sizeof(fmtdesc));
    fmtdesc.index = 0;
    fmtdesc.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    while (ioctl(fd, VIDIOC_ENUM_FMT, &fmtdesc) == 0)
    {
        fmtdesc.index++;
        printf("support fmt: %4s, %s \r\n",
               (char *)&fmtdesc.pixelformat, fmtdesc.description);
    }

    // 格式信息
    memset(&fmt, 0, sizeof(fmt));
    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (ioctl(fd, VIDIOC_G_FMT, &fmt) < 0)
    {
        fprintf(stderr, "ioctl VIDIOC_G_FMT error (%d)\r\n", errno);
        return NULL;
    }
    // 设置格式
    if (type)
    {
        fmt.fmt.pix.pixelformat = v4l2_fourcc(type[0], type[1], type[2], type[3]);
        fmt.fmt.pix.width = width;
        fmt.fmt.pix.height = height;
        if (ioctl(fd, VIDIOC_S_FMT, &fmt) < 0)
        {
            fprintf(stderr, "ioctl VIDIOC_S_FMT error (%d)\r\n", errno);
            // return NULL;
        }
    }
    // 再次读取确认
    memset(&fmt, 0, sizeof(fmt));
    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (ioctl(fd, VIDIOC_G_FMT, &fmt) < 0)
    {
        fprintf(stderr, "ioctl VIDIOC_G_FMT error (%d)\r\n", errno);
        return NULL;
    }

    printf("FMT: %d x %d - type %4s - size %d\r\n",
           fmt.fmt.pix.width,
           fmt.fmt.pix.height,
           (char *)&fmt.fmt.pix.pixelformat,
           fmt.fmt.pix.sizeimage);

    // 申请视频流buff块信息
    memset(&reqBuff, 0, sizeof(reqBuff));
    reqBuff.count = 4;
    reqBuff.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    reqBuff.memory = V4L2_MEMORY_MMAP;
    if (ioctl(fd, VIDIOC_REQBUFS, &reqBuff) < 0)
    {
        fprintf(stderr, "ioctl VIDIOC_REQBUFS error (%d)\r\n", errno);
        return NULL;
    }
    if (reqBuff.count < 1)
    {
        fprintf(stderr, "Insufficient reqBuff.count %d\r\n", reqBuff.count);
        return NULL;
    }

    // 参数准备
    cms = (CameraMap_Struct *)calloc(1, sizeof(CameraMap_Struct));
    cms->fd = fd;
    cms->width = fmt.fmt.pix.width;
    cms->height = fmt.fmt.pix.height;
    cms->runFlag = 1;
    memcpy(cms->format, &fmt.fmt.pix.pixelformat, 4);
    cms->memSize = reqBuff.count;
    cms->mem = (CameraMap_Frame *)calloc(reqBuff.count, sizeof(CameraMap_Frame));
    cms->obj = obj;
    cms->callback = callback;

    // 查询已经分配视频流内存块信息(对应每帧图像,大小和起始地址一般是连续的)
    for (i = 0; i < reqBuff.count; i++)
    {
        memset(&buffer, 0, sizeof(buffer));
        buffer.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buffer.memory = V4L2_MEMORY_MMAP;
        buffer.index = i;

        // 获取单块(单帧)数据量和地址信息
        if (ioctl(fd, VIDIOC_QUERYBUF, &buffer) < 0)
        {
            fprintf(stderr, "ioctl VIDIOC_QUERYBUF error (%d)\r\n", errno);
            return NULL;
        }
        // printf("count %d: len %d, offset %d\r\n", i, buffer.length, buffer.m.offset);

        cms->mem[i].size = buffer.length;
        cms->mem[i].buff = (uint8_t *)mmap(
            0, buffer.length, PROT_READ | PROT_WRITE, MAP_SHARED, fd, buffer.m.offset);
    }

    // 开线程检查数据更新
    new_thread(cms, (void *)fbmapCamera_callback);

    return cms;
}

/*
 *  关闭内存映射和文件
 */
void fileMap_close(FileMap_Struct *fms)
{
    if (fms)
    {
        if (fms->mem)
            munmap(fms->mem, fms->size);
        if (fms->fd > 0)
            close(fms->fd);
        free(fms);
    }
}

void cameraMap_close(CameraMap_Struct *cms)
{
    int i;
    if (!cms)
        return;
    //请求关闭线程
    cms->runFlag = 2;
    for (i = 0; i < 1000 && cms->runFlag; i += 200)
        usleep(200000);
    //解除内存映射
    if (cms->mem)
    {
        for (i = 0; i < cms->memSize; i++)
            munmap(cms->mem[i].buff, cms->mem[i].size);
        free(cms->mem);
    }
    if (cms->fd > 0)
        close(cms->fd);
    free(cms);
}
