#include <math.h>
#include <stdio.h>
#include <sys/time.h>

#include "debug.h"
#include "receive.h"
#include "network.h"
#include "hashlist.h"
#include "operateredis.h"

extern pthread_mutex_t mac_cache_mutex;
extern pthread_cond_t mac_cache_cond;

extern struct listhash *mac_cache_name;

extern PFdProcess gFdProcess[MAX_FD];


void * recvMalloc(u32_t len)
{
    void *p = malloc(len);

    if(p == NULL)
    {
        return NULL;
    }
    else
    {
        memset(p, 0, len);
        return p;
    }
}

int client_readfun(int epoll, int fd, timer_link *timer)
{
    char buff[1024] = {0};

    char * oldBuff = gFdProcess[fd]->gbuf;
    int dataLen = gFdProcess[fd]->dataLen;
    int maxLen = gFdProcess[fd]->maxLen;

    while(true)
    {
        int len = read(fd, buff, 256);
		infoTrace("%s: read len =  %d bytes\n", __FUNCTION__,len);

        if(len < 0)
        {
            buff[0] = 0;

            if(errno == EINTR || errno == EWOULDBLOCK || errno == EAGAIN)
            {
                break;
            }
            else
            {
                infoTrace("%s: read< 0 bytes\n", __FUNCTION__);
                gFdProcess[fd]->m_activeclose = true;
                return -1;
            }
        }
        else if(len == 0)
        {
            infoTrace("%s: read= 0 bytes\n", __FUNCTION__);
            buff[len] = 0;
            gFdProcess[fd]->m_activeclose = true;
            return -1;
        }

        int totalLen = dataLen + len;
		infoTrace("%s: current bufflen = %d , totalLen(bufflen+len) = %d , maxLen = %d \n", __FUNCTION__,dataLen,totalLen,maxLen);

        if(oldBuff == NULL && maxLen == 0)
        {
            oldBuff = (char *)recvMalloc(512);

            if(oldBuff == 0)
            {
                gFdProcess[fd]->m_activeclose = true;
                return -1;
            }

            gFdProcess[fd]->gbuf = oldBuff ;
            gFdProcess[fd]->maxLen = 512;
            maxLen = 512;
            dbgTrace("first allocate  %d\n", maxLen);
        }

        if(totalLen <= maxLen)
        {
            memcpy(oldBuff + dataLen, buff, len);
            dataLen += len;
            gFdProcess[fd]->dataLen = dataLen;
            dbgTrace("recv datelen =  %d  don't alloc space\n", dataLen);
        }
        else
        {
            char *newBuff = (char *)recvMalloc(totalLen);

            if(newBuff == 0)
            {
                gFdProcess[fd]->m_activeclose = true;
                return -1;
            }

            gFdProcess[fd]->gbuf = newBuff ;
            gFdProcess[fd]->maxLen = totalLen;
            maxLen = totalLen;

            memcpy(newBuff, oldBuff, dataLen);
            memcpy(newBuff + dataLen, buff, len);

            dataLen += len;
            gFdProcess[fd]->dataLen = dataLen;
            dbgTrace("old buff not enough，new buff len =  %d ,recv datelen %d\n", maxLen, dataLen);
            free(oldBuff);
            oldBuff = newBuff;

        }

        char *recvBuff = gFdProcess[fd]->gbuf;

        while(true)
        {
            if(dataLen == 0)
            {
                break;
            }

            int invalidFlag = ishdrValid(recvBuff, dataLen);
			dbgTrace("invalidFlag %d after ishdrValid() \n", invalidFlag);

            if(invalidFlag < 0 && invalidFlag != -3)
            {
                break;
            }

            if(invalidFlag == -3)
            {
                THDR *pHdr;
                pHdr = (THDR *)recvBuff;
                int needLen = 0;
                dbgTrace("[invalidFlag == -3] recv datelen :%d\n", dataLen);
                needLen = sizeof(THDR) + pHdr->pktlen;
                dataLen = dataLen - needLen;
                dbgTrace("abandon invalid packet,remain datalen :%d\n", dataLen);
                memcpy(recvBuff, recvBuff + needLen, dataLen);
                gFdProcess[fd]->dataLen = dataLen;
                continue;
            }

            int remainLen = proc_packet(fd, recvBuff, dataLen);
			dbgTrace("remainLen %d after proc_packet() \n", remainLen);

            if(remainLen > 0)
            {
                memcpy(recvBuff, recvBuff + dataLen - remainLen, remainLen);
            }

            dataLen = remainLen;
            gFdProcess[fd]->dataLen = dataLen;

        }

    }

    return 1;
}

int client_writefun(int epoll, int fd, timer_link *timer)
{
    return 1;
}

int client_closefun(int epoll, int fd, timer_link *timer)
{
    dbgTrace("%s  %d close(%d)\n", __FUNCTION__, __LINE__, fd);

    timer->remote_timer(gFdProcess[fd]);

#if 0
    char *deviceName = (char *)gFdProcess[fd]->equipmentSn;

    char judgeDevice[8] = {0};

    char formatMac[24] = {0};

    strncpy(judgeDevice, deviceName, 6);
    int recvLen = strlen(judgeDevice) ;

    dbgTrace("%s:the equipmentSnlen is %d\n", __FUNCTION__ , recvLen);

#else
	char deviceName[8] = {0};
	
	memcpy(deviceName,gFdProcess[fd]->equipmentSn,6);
	dbgTrace("%s:%d gFdProcess[%d]->equipmentSn(%02x%02x%02x%02x%02x%02x)\n", __FUNCTION__, __LINE__,fd,
		gFdProcess[fd]->equipmentSn[0],gFdProcess[fd]->equipmentSn[1],gFdProcess[fd]->equipmentSn[2],
		gFdProcess[fd]->equipmentSn[3],gFdProcess[fd]->equipmentSn[4],gFdProcess[fd]->equipmentSn[5]);

#endif

	//dbgTrace("%s:the equipmentSnlen is %d\n", __FUNCTION__ , recvLen);

    pthread_mutex_lock(&mac_cache_mutex);

    struct listnode *pos = NULL;
    pos = findNode(deviceName, mac_cache_name);
    struct mac_info * pdevice = NULL;
	int closeTimeFd = 0;

    if(pos != NULL )
    {
        pdevice = &pos->macInfo;

        dbgTrace("%s  %d  fd=%d   online=%d\n", __FUNCTION__,
                 __LINE__, pdevice->sessionFd, pdevice->onLineStatus);


		/* 如果pdevice->sessionFd != fd 表示fd是因为心跳超时需要断开连接，但在判断超时的过程
		中，客户端已经建立了新的链接，这时候不能影响新连接的状态，但要关闭
		超时连接*/
        if(pdevice->sessionFd != fd)
        {
			closeTimeFd = 1;
        } else {
            pdevice->sessionFd = 0;
            pdevice->onLineStatus = OFFLINE;
        }
    } else {
		dbgTrace("%s:findNode(deviceName, mac_cache_name) is NULL \n", __FUNCTION__);
    }

    pthread_mutex_unlock(&mac_cache_mutex);

	if(closeTimeFd == 1) {
		critTrace("%s : pdevice->sessionFd(%d) != fd(%d),close timeout fd!\n", __FUNCTION__,pdevice->sessionFd,fd);
	}	

#if 0
    if(6 == recvLen)
    {
        int setFlag;
        char command[64] = "set online-";

        changeMac(deviceName, formatMac, recvLen);

        strcat(command, formatMac);
        strcat(command, "  0");

        setFlag = setKeyValueToRedis(command);

        if(setFlag == 0)
            dbgTrace("%s:success set the key\n", __FUNCTION__);


        char delCommade[64] = "del heartbeat-";
        strcat(delCommade, formatMac);
        delKeyValueInRedis(delCommade);
    }
#else
	if(pos != NULL && !closeTimeFd) {
		int setFlag;
		char command[64] = {0};
		
		
		//changeMac(deviceName, formatMac, 6);
		
		//strcat(command, formatMac);
		//strcat(command, "  0");
		
		sprintf(command,"set online-%02X-%02X-%02X-%02X-%02X-%02X 0",
				(u8_t)deviceName[0],(u8_t)deviceName[1],(u8_t)deviceName[2],
				(u8_t)deviceName[3],(u8_t)deviceName[4],(u8_t)deviceName[5]);
		
		setFlag = setKeyValueToRedis(command);
		
		if(setFlag == 0)
			dbgTrace("%s:success set the key\n", __FUNCTION__);
		
		
		char delCommade[64] = {0};
		//strcat(delCommade, formatMac);
		sprintf(delCommade,"del heartbeat-%02X-%02X-%02X-%02X-%02X-%02X",
				(u8_t)deviceName[0],(u8_t)deviceName[1],(u8_t)deviceName[2],
				(u8_t)deviceName[3],(u8_t)deviceName[4],(u8_t)deviceName[5]);
		delKeyValueInRedis(delCommade);
	}
#endif

    gFdProcess[fd]->init();
    close(fd);

    return 1;
}

int client_timeoutfun(int epoll, int fd, timer_link *timers, time_value tnow)
{
    // timers->remote_timer(gFdProcess[fd]);

    FdProcess *pCnxt;
    pCnxt = gFdProcess[fd];

    int interval = tnow - pCnxt->m_lastecho;

	int setInterval=getInterval();

	/* pengruofeng debug timeout*/
    dbgTrace("%s : fd(%d) setInterval(%ds) interval(%d=%d-%d)  \n",
    		__FUNCTION__,fd,setInterval,interval,tnow,pCnxt->m_lastecho);
    if(pCnxt->echoFlag == 1 && interval > setInterval * 1000)
    {
        client_closefun(epoll, fd, timers);
        dbgTrace("%s  heartbeat timeout(%ds)  %d\n", __FUNCTION__,setInterval,fd);
    }

    timers->add_timer(gFdProcess[fd], tnow + 10 * 1000);

    return 0;
}


