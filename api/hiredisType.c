#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "hiredisType.h"

#if(MAKE_HIREDIS)
#include "hiredis.h"

RedisCom* redis_connect(char *ip, int port)
{
    RedisCom *rc = (RedisCom*)calloc(1, sizeof(RedisCom));
    //rc->p = redisConnect(ip, port);
    struct timeval tv = {
        .tv_sec = 1,
        .tv_usec = 0
    };
    rc->p = redisConnectWithTimeout(ip, port, tv);
    if (!rc->p || ((redisContext*)rc->p)->err)
    {
        if(rc->p)
            printf("redisConnect Error: %s\n", ((redisContext*)rc)->errstr);
        else
            printf("redisConnect Error: NULL\n");
        free(rc);
        return NULL;
    }
    pthread_mutex_init(&rc->lock, NULL);
    return rc;
}

void redis_disconnect(RedisCom *rc)
{
    if(!rc)
        return;
    redisFree((redisContext*)rc->p);
    pthread_mutex_destroy(&rc->lock);
}

// QStringList redis_getStrList(RedisCom *rc, QString key)
// {
//     QStringList ret;
//     int len = 0;
//     char cmd[128] = {0};
//     //
//     if(!rc || !rc->p)
//         return ret;
//     //
//     pthread_mutex_lock(&rc->lock);
//     //
//     sprintf (cmd, "LLEN %s", key.toLocal8Bit ().data ());
//     redisReply *reply = (redisReply*)redisCommand((redisContext*)rc->p, cmd);
//     if(reply->type == REDIS_REPLY_INTEGER)
//         len = reply->integer;
//     freeReplyObject(reply);
//     if(len > 0)
//     {
//         char cmd2[128] = {0};
//         sprintf (cmd2, "LRANGE %s 0 %d", key.toLocal8Bit ().data (), len-1);
//         redisReply *reply2 = (redisReply*)redisCommand((redisContext*)rc->p, cmd2);
//         if(reply2->type == REDIS_REPLY_ARRAY)
//         {
//             for(size_t i = 0; i < reply2->elements; i++)
//                 ret << reply2->element[i]->str;
//         }
//         // else
//         //     qDebug() << "redis_getStr: type err " << reply->type << " (" << reply->str << ") cmd: " << cmd;
//         freeReplyObject(reply2);
//     }
//     //
//     pthread_mutex_unlock(&rc->lock);
//     //
//     return ret;
// }

void redis_getStr(RedisCom *rc, char *key, char *defaultRet)
{
    char cmd[128] = {0};

    if(!rc || !rc->p)
        return;

    pthread_mutex_lock(&rc->lock);

    sprintf (cmd, "GET %s", key);
    redisReply *reply = (redisReply*)redisCommand((redisContext*)rc->p, cmd);
    if(reply->type == REDIS_REPLY_STRING)
        strcpy(defaultRet, reply->str);
    else if(reply->type == REDIS_REPLY_INTEGER)
        sprintf(defaultRet, "%ld", (long int)(reply->integer));
    // else
    //     qDebug() << "redis_getStr: type err " << reply->type << " (" << reply->str << ") cmd: " << cmd;
    freeReplyObject(reply);

    pthread_mutex_unlock(&rc->lock);

    defaultRet[strlen(defaultRet)] = '\0';
}

int redis_getInt(RedisCom *rc, char *key, int defaultRet)
{
    char result[64] = {0};
    int resultInt = defaultRet;
    sprintf(result, "%d", defaultRet);
    redis_getStr(rc, key, result);
    sscanf(result, "%d", &resultInt);
    return resultInt;
}

double redis_getDouble(RedisCom *rc, char *key, double defaultRet)
{
    char result[64] = {0};
    double resultDouble = defaultRet;
    sprintf(result, "%lf", defaultRet);
    redis_getStr(rc, key, result);
    sscanf(result, "%lf", &resultDouble);
    return resultDouble;
}

void redis_setStr(RedisCom *rc, char *key, char *value)
{
    char cmd[128] = {0};

    if(!rc || !rc->p)
        return;

    pthread_mutex_lock(&rc->lock);

    sprintf (cmd, "SET %s %s", key, value);
    redisReply *reply = (redisReply*)redisCommand((redisContext*)rc->p, cmd);
    freeReplyObject(reply);

    pthread_mutex_unlock(&rc->lock);
}

void redis_setInt(RedisCom *rc, char *key, int value)
{
    char cmd[128] = {0};

    if(!rc || !rc->p)
        return;

    pthread_mutex_lock(&rc->lock);

    sprintf (cmd, "SET %s %d", key, value);
    redisReply *reply = (redisReply*)redisCommand((redisContext*)rc->p, cmd);
    freeReplyObject(reply);

    pthread_mutex_unlock(&rc->lock);
}

void redis_setDouble(RedisCom *rc, char *key, double value)
{
    char cmd[128] = {0};

    if(!rc || !rc->p)
        return;

    pthread_mutex_lock(&rc->lock);

    sprintf (cmd, "SET %s %lf", key, value);
    redisReply *reply = (redisReply*)redisCommand((redisContext*)rc->p, cmd);
    freeReplyObject(reply);

    pthread_mutex_unlock(&rc->lock);
}

#endif // #if(MAKE_HIREDIS)