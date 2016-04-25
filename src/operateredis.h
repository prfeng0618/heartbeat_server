#ifndef  _OPERATEREDIS_H
#define  _OPERATEREDIS_H

int  setKeyValueToRedis(const char *command);

int  getValueInRedis(const char *command);

int  delKeyValueInRedis(const char *command);


void getRedisHostIp(char *redisHostIp);
int getRedisPort();


void * testRedis(void *arg);

#endif

