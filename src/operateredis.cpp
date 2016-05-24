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
	char redisPwd[32] = {0};

    getRedisHostIp(redisHostIp);
    redisPort = getRedisPort();
	getRedisPwd(redisPwd);

    //该对象将用于其后所有与Redis操作的函数。
    redisContext* c = redisConnect(redisHostIp, redisPort);

    if(c->err)
    {
        redisFree(c);
        infoTrace("%s:Failed to  connect redis server.\n", __FUNCTION__);
        return -1;
    }

    critTrace("%s:success connect redis server.\n", __FUNCTION__);
	
    //Authentication is done after a connection has been made, and should be done by the consumer of the API like so: 
    redisReply* reply= (redisReply*)redisCommand(c, "AUTH %s",redisPwd);
    if (reply->type == REDIS_REPLY_ERROR) {
        dbgTrace("%s:Authentication failed.\n", __FUNCTION__);
    }
    freeReplyObject(reply);

    redisReply* r = (redisReply*)redisCommand(c, command);

    if(NULL == r)
    {
        redisFree(c);
        return -1;
    }

    //字符串类型的set命令的返回值的类型是REDIS_REPLY_STATUS
    //后只有当返回信息是"OK"
    if(!(r->type == REDIS_REPLY_STATUS && strcasecmp(r->str, "OK") == 0))
    {
        infoTrace("Failed to execute command[%s].\n", command);
        freeReplyObject(r);
        redisFree(c);
        return -1;
    }

    freeReplyObject(r);
    critTrace("Succeed to execute command[%s].\n", command);
	printBackTrace();

    //不要忘记在退出前释放当前连接的上下文对象
    redisFree(c);
    return 0;
}

//alreay test right
int  getValueInRedis(const char *command)
{
    char redisHostIp[40] = {0};
    int redisPort;
	char redisPwd[32] = {0};

    getRedisHostIp(redisHostIp);
    redisPort = getRedisPort();
	getRedisPwd(redisPwd);

    redisContext* c = redisConnect(redisHostIp, redisPort);

    if(c->err)
    {
        redisFree(c);
        infoTrace("%s:Failed to  connect redis server.\n", __FUNCTION__);
        return -1;
    }

    critTrace("%s:success connect redis server.\n", __FUNCTION__);

    //Authentication is done after a connection has been made, and should be done by the consumer of the API like so: 
    redisReply* reply= (redisReply*)redisCommand(c, "AUTH %s",redisPwd);
    if (reply->type == REDIS_REPLY_ERROR) {
        dbgTrace("%s:Authentication failed.\n", __FUNCTION__);
    }
    freeReplyObject(reply);

    redisReply*  r = (redisReply*)redisCommand(c, command);

    if(r->type == REDIS_REPLY_NIL)
    {
        dbgTrace("the key is not exist command[%s].\n", command);
        freeReplyObject(r);
        redisFree(c);
        return -1;
    }
    else if(r->type == REDIS_REPLY_STRING)
    {
        critTrace("Succeed to execute command[%s].the value is %s.\n", command, r->str);
    }
    else
    {
        dbgTrace("Failed to execute command[%s].\n", command);
    }

    freeReplyObject(r);
    redisFree(c);
    return 0;

}

int  delKeyValueInRedis(const char *command)
{
    char redisHostIp[40] = {0};
    int redisPort;
	char redisPwd[32] = {0};

    getRedisHostIp(redisHostIp);
    redisPort = getRedisPort();
	getRedisPwd(redisPwd);

    redisContext* c = redisConnect(redisHostIp, redisPort);

    if(c->err)
    {
        redisFree(c);
        infoTrace("%s:Failed to  connect redis server.\n", __FUNCTION__);
        return -1;
    }

    critTrace("%s:success connect redis server.\n", __FUNCTION__);

    //Authentication is done after a connection has been made, and should be done by the consumer of the API like so: 
    redisReply* reply= (redisReply*)redisCommand(c, "AUTH %s",redisPwd);
    if (reply->type == REDIS_REPLY_ERROR) {
        dbgTrace("%s:Authentication failed.\n", __FUNCTION__);
    }
    freeReplyObject(reply);

    redisReply*  r = (redisReply*)redisCommand(c, command);

    if(r->type != REDIS_REPLY_INTEGER)
    {
        dbgTrace("Failed to execute command[%s].\n", command);
        freeReplyObject(r);
        redisFree(c);
        return -1;
    }

    int delNumber = r->integer;

    critTrace("Succeed to execute command[%s].the number=%d.\n", command, delNumber);

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

void getRedisPwd(char *redisPassword)
{
    static int flag = 0;

    GetProfileString("./initConf.conf", "heartbeat_server", "redisPassword", redisPassword);

    if(flag == 0)
    {
        dbgTrace("%s:%s\n", __FUNCTION__ , redisPassword);
        flag = 1;
    }

    if(strlen(redisPassword) <= 0)
    {
        strcpy(redisPassword, "mye2016");
    }
}



void * testRedis(void *arg)
{
    const char * command1 = "set testre 5";
    const char * command2 = "get testre";
    setKeyValueToRedis(command1);
    getValueInRedis(command2);
    return NULL;
}



