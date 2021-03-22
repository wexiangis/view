#ifndef _HIREDISTYPE_H_
#define _HIREDISTYPE_H_

//如果Makefile没有定义则自行定义
#include "viewDef.h"
#ifndef MAKE_HIREDIS
#define MAKE_HIREDIS 1
#endif

#if(MAKE_HIREDIS)
#include <pthread.h>

typedef struct{
    void *p;
    pthread_mutex_t lock;
}RedisCom;

//ip = "127.0.0.1", port = 6379
RedisCom* redis_connect(char *ip, int port);

void redis_disconnect(RedisCom *rc);

// QStringList redis_getStrList(RedisCom *rc, QString key);

void redis_getStr(RedisCom *rc, char *key, char *defaultRet);

int redis_getInt(RedisCom *rc, char *key, int defaultRet);

double redis_getDouble(RedisCom *rc, char *key, double defaultRet);

void redis_setStr(RedisCom *rc, char *key, char *value);

void redis_setInt(RedisCom *rc, char *key, int value);

void redis_setDouble(RedisCom *rc, char *key, double value);

#endif // #if(MAKE_HIREDIS)

#endif // _HIREDISTYPE_H_

