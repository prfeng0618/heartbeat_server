#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdarg.h>
#include <string.h>
#include <assert.h>
#include <hiredis/hiredis.h>

#include "debug.h"
#include "readconfig.h"
#include "operateredis.h"

//alreay test right
int  setKeyValueToRedis(const char *command)
{
    char redisHostIp[40] = {0};
    int redisPort;

    getRedisHostIp(redisHostIp);
    redisPort = getRedisPort();

    //�ö����������������Redis�����ĺ�����
    redisContext* c = redisConnect(redisHostIp, redisPort);

    if(c->err)
    {
        redisFree(c);
        infoTrace("%s:Failed to  connect redis server.\n", __FUNCTION__);
        return -1;
    }

    dbgTrace("%s:success connect redis server.\n", __FUNCTION__);

    redisReply* r = (redisReply*)redisCommand(c, command);

    if(NULL == r)
    {
        redisFree(c);
        return -1;
    }

    //�ַ������͵�set����ķ���ֵ��������REDIS_REPLY_STATUS
    //��ֻ�е�������Ϣ��"OK"
    if(!(r->type == REDIS_REPLY_STATUS && strcasecmp(r->str, "OK") == 0))
    {
        infoTrace("Failed to execute command[%s].\n", command);
        freeReplyObject(r);
        redisFree(c);
        return -1;
    }

    freeReplyObject(r);
    dbgTrace("Succeed to execute command[%s].\n", command);

    //��Ҫ�������˳�ǰ�ͷŵ�ǰ���ӵ������Ķ���
    redisFree(c);
    return 0;
}

//alreay test right
int  getValueInRedis(const char *command)
{
    char redisHostIp[40] = {0};
    int redisPort;

    getRedisHostIp(redisHostIp);
    redisPort = getRedisPort();

    redisContext* c = redisConnect(redisHostIp, redisPort);

    if(c->err)
    {
        redisFree(c);
        infoTrace("%s:Failed to  connect redis server.\n", __FUNCTION__);
        return -1;
    }

    dbgTrace("%s:success connect redis server.\n", __FUNCTION__);

    redisReply*  r = (redisReply*)redisCommand(c, command);

    if(r->type == REDIS_REPLY_NIL)
    {
        dbgTrace("the key is not exist command[%s].\n", command);
        dbgTrace("Succeed to execute command[%s].\n", command);
        freeReplyObject(r);
        redisFree(c);
        return -1;
    }
    else if(r->type == REDIS_REPLY_STRING)
    {
        dbgTrace("Succeed to execute command[%s].the value is %s.\n", command, r->str);
    }
    else
    {
        infoTrace("Failed to execute command[%s].\n", command);
    }

    freeReplyObject(r);
    redisFree(c);
    return 0;

}

int  delKeyValueInRedis(const char *command)
{
    char redisHostIp[40] = {0};
    int redisPort;

    getRedisHostIp(redisHostIp);
    redisPort = getRedisPort();

    redisContext* c = redisConnect(redisHostIp, redisPort);

    if(c->err)
    {
        redisFree(c);
        infoTrace("%s:Failed to  connect redis server.\n", __FUNCTION__);
        return -1;
    }

    dbgTrace("%s:success connect redis server.\n", __FUNCTION__);

    redisReply*  r = (redisReply*)redisCommand(c, command);

    if(r->type != REDIS_REPLY_INTEGER)
    {
        printf("Failed to execute command[%s].\n", command);
        freeReplyObject(r);
        redisFree(c);
        return -1;
    }

    int delNumber = r->integer;

    dbgTrace("Succeed to execute command[%s].the number=%d.\n", command, delNumber);

    freeReplyObject(r);
    redisFree(c);
    return 0;

}


void getRedisHostIp(char *redisHostIp)
{
    static int flag = 0;

    GetProfileString("./initConf.conf", "heartbeat_server", "redisIp", redisHostIp);

    if(flag == 0)
    {
        dbgTrace("%s:%s\n", __FUNCTION__ , redisHostIp);
        flag = 1;
    }

    if(strlen(redisHostIp) <= 0)
    {
        strcpy(redisHostIp, "10.9.200.12");
    }
}

int getRedisPort()
{
    int sRedisPort = 6379;
    char redisPort[16] = {0};
    static int flag = 0;

    GetProfileString("./initConf.conf", "heartbeat_server", "redisPort", redisPort);

    if(flag == 0)
    {
        dbgTrace("%s:%s\n", __FUNCTION__, redisPort);
        flag = 1;
    }

    if(strlen(redisPort) > 0)
    {
        sRedisPort = atoi(redisPort);

    }

    return sRedisPort;

}


void * testRedis(void *arg)
{
    const char * command1 = "set testre 5";
    const char * command2 = "get testre";
    setKeyValueToRedis(command1);
    getValueInRedis(command2);
    return NULL;
}



