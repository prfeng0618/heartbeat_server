#include "network.h"
#include "receive.h"
#include "debug.h"
#include "xorcode.h"
#include "des.h"
#include "hashlist.h"
#include "operateredis.h"
#include "readconfig.h"
#include "business.h"

#include <sys/ioctl.h>
#include <net/if.h>
#include <stdlib.h>


extern PFdProcess gFdProcess[MAX_FD];
extern struct listhash *mac_cache_name;
extern int globalListenPort;
extern char globalHostIp[];
extern pthread_mutex_t mac_cache_mutex;
extern pthread_cond_t mac_cache_cond;


#define ETH_NAME        "eth0"

static void print_chalreq(THDR  *tHdr, TCHALREQ  *chalReq)
{

	dbgTrace("<<-- [challange resquest] [hdr]:{flag(0x%04x),pktlen(%d),version(%d),pktType(%d),sn(%d),ext(0x%08x)} \
[data]:{magic(0x%08x),key(%d),res(%02x%02x%02x%02x%02x%02x%02x%02x)}\n", 
		tHdr->flag,
		tHdr->pktlen,
		tHdr->version,
		tHdr->pktType,
		tHdr->sn,
		tHdr->ext,
		chalReq->magic,
		chalReq->key,
		chalReq->u8res[0],chalReq->u8res[1],chalReq->u8res[2],chalReq->u8res[3],
		chalReq->u8res[4],chalReq->u8res[5],chalReq->u8res[6],chalReq->u8res[7]);

}


static void print_chalresp(THDR  *tHdr, TCHALRESP  *chalResp)
{
	u32_t server_key;
    memcpy(&server_key, chalResp->key, sizeof(server_key));
	u32_t magic;
    memcpy(&magic, chalResp->magic, sizeof(magic));

	dbgTrace("[challange response] -->> [hdr]:{flag(0x%04x),pktlen(%d),version(%d),pktType(%d),sn(%d),ext(0x%08x)} \
[data]:{sn(%d),magic(%08x),key(%d),res(0x%02x%02x%02x%02x%02x%02x)}\n", 
		tHdr->flag,
		tHdr->pktlen,
		tHdr->version,
		tHdr->pktType,
		tHdr->sn,
		tHdr->ext,
		chalResp->client_sn,
		magic,
		server_key,
		chalResp->u6res[0],chalResp->u6res[1],chalResp->u6res[2],chalResp->u6res[3],chalResp->u6res[4],chalResp->u6res[5]);
}



static void print_echoreq(THDR  *tHdr, TECHOREQ  *echoReq)
{
	dbgTrace("<<-- [echo resquest] [hdr]:{flag(0x%04x),pktlen(%d),version(%d),pktType(%d),sn(%d),ext(0x%08x)} \
[data]:{equipmentSn(0x%02x%02x%02x%02x%02x%02x)}\n", 
		tHdr->flag,
		tHdr->pktlen,
		tHdr->version,
		tHdr->pktType,
		tHdr->sn,
		tHdr->ext,
		(unsigned char)echoReq->equipmentSn[0],(unsigned char)echoReq->equipmentSn[1],(unsigned char)echoReq->equipmentSn[2],
		(unsigned char)echoReq->equipmentSn[3],(unsigned char)echoReq->equipmentSn[4],(unsigned char)echoReq->equipmentSn[5]);
}



static void print_echoresp(THDR  *tHdr, TECHORESP *echoResp)
{
	dbgTrace("[echo response] -->> [hdr]:{flag(0x%04x),pktlen(%d),version(%d),pktType(%d),sn(%d),ext(0x%08x)} \
[data]:{client_sn(%d)}\n", 
		tHdr->flag,
		tHdr->pktlen,
		tHdr->version,
		tHdr->pktType,
		tHdr->sn,
		tHdr->ext,
		echoResp->client_sn);
}


static void print_notifyreq(THDR  *tHdr,TNOTIFYREQ  *notifyReq)
{
	dbgTrace("[notify resquest] -->> [hdr]:{flag(0x%04x),pktlen(%d),version(%d),pktType(%d),sn(%d),ext(0x%08x)} \
[data]:{equipmentSn(0x%c%c%c%c%c%c), command(%d), sendtime(%d)}\n", 
		tHdr->flag,
		tHdr->pktlen,
		tHdr->version,
		tHdr->pktType,
		tHdr->sn,
		tHdr->ext,
		notifyReq->equipmentSn[0],notifyReq->equipmentSn[1],notifyReq->equipmentSn[2],
		notifyReq->equipmentSn[3],notifyReq->equipmentSn[4],notifyReq->equipmentSn[5],
		notifyReq->command,notifyReq->sendTime);
}


static void print_notifyresp(THDR  *tHdr,TNOTIFYRESP* notifyResp)
{
	dbgTrace("<<-- [notify response] [hdr]:{flag(0x%04x),pktlen(%d),version(%d),pktType(%d),sn(%d),ext(0x%08x)} \
[data]:{returnSn(%d), returnCode(%d)}\n", 
		tHdr->flag,
		tHdr->pktlen,
		tHdr->version,
		tHdr->pktType,
		tHdr->sn,
		tHdr->ext,
		notifyResp->returnSn,notifyResp->returnCode);
}



/*
  *判断是否是一个完整的包
  *通过判断包头长度
  *包头标志
  *包体长度
  *如果三个条件均满足，取出包处理
  *形参是接收到的数据和数据长度
*/
int ishdrValid(char *pBuff, u32_t len)
{
    THDR *pHdr;

    if(len < sizeof(THDR))
    {
        return -1;
    }

    pHdr = (THDR *)pBuff;

    unsigned int needlen = sizeof(THDR) + pHdr->pktlen;
	dbgTrace("[ishdrValid] needlen = %d(%d + %d) , bufflen = %d\n", needlen,sizeof(THDR), pHdr->pktlen,len);

    if(len < needlen)
    {
        return -2;
    }

    if(pHdr->flag != PKT_HDR_MAGIC)
    {
        dbgTrace("recv invalid pkt, recv magic=0x%X, right=0x%X\n", pHdr->flag, PKT_HDR_MAGIC);
        return -3;
    }

    return 1;
}

/*
  *处理接收到的数据包
  *每次处理一个数据包
*/
int proc_packet(int fd, char *pBuff, int readLen)
{
    THDR *pHdr;
    pHdr = (THDR *)pBuff;

    /*本次处理的数据长度*/
    int packetLen = sizeof(THDR) + pHdr->pktlen;

    dbgTrace("recv packet, type = %d,packetlen = %d\n", pHdr->pktType,packetLen);

    if(pHdr->pktType == PKT_CHALLENGE_REQUEST)
    {
        recv_chalfun(fd, pBuff);
    }
    else if(pHdr->pktType == PKT_ECHO_REQUEST)
    {
        recv_echofun(fd, pBuff);
    }
    else if(pHdr->pktType == PKT_NOTIFY_REQUEST)
    {
        recv_notifyreq_fun(fd, pBuff);
    }
    else if(pHdr->pktType == PKT_NOTIFY_RESPONSE)
    {
        recv_notifyresp_fun(fd, pBuff);
    }
    else if(pHdr->pktType == PKT_REPORT_REQUEST)
    {
        recv_business_report_request(fd, pBuff);
		//send_business_issue_request(fd,pBuff);
    }
    else if(pHdr->pktType == PKT_ISSUE_REQUEST)
    {
        recv_business_issue_request(fd, pBuff);
    }
    else if(pHdr->pktType == PKT_ISSUE_RESPONSE)
    {
        recv_business_issue_response(fd, pBuff);
    }
    else
    {
        infoTrace("invalid pocket\n");
    }

    int remainData = readLen - packetLen;

    if(remainData > 0)
    {
        return remainData;
    }
    else
    {
        return 0;
    }
}

/*
  *接收echo消息
  *心跳并且回复echo消息
*/
int recv_echofun(int fd, char *pbuff)
{
    THDR *pHdr;
    pHdr = (THDR *)pbuff;

    u16_t clientSn = pHdr->sn;

    FdProcess *pCnxt;
    pCnxt = gFdProcess[fd];

    u32_t key = pCnxt->server_key;

    TECHOREQ echoReqMsg;
    XORencode(pbuff + sizeof(THDR), &echoReqMsg, key, pHdr->pktlen);

	/*pengruofeng debug mips*/
	print_echoreq(pHdr,&echoReqMsg);
		
    char judgeDevice[8] = {0};
#if 0
    strncpy(judgeDevice, echoReqMsg.equipmentSn, 6);
    int recvLen = strlen(judgeDevice) ;
#endif
    char formatMac[24] = {0};

#if 0
    if(recvLen != 6)
    {
        dbgTrace("%s:%d  equipmentSn:", __FUNCTION__, __LINE__);
        printf_equipmentsn(judgeDevice, 6);
    }

    if(pCnxt->echoFlag == 0 && 6 == recvLen)
#endif
	if(pCnxt->echoFlag == 0)
    {
        //strncpy(pCnxt->equipmentSn, echoReqMsg.equipmentSn, 6);
		memcpy(pCnxt->equipmentSn, echoReqMsg.equipmentSn, 6);
		
        struct listnode macInfoNode;
        memset(&macInfoNode, 0, sizeof(macInfoNode));
        struct mac_info *pmac;
        pmac = &(macInfoNode.macInfo);
        //strncpy(pmac->equipmentSn, echoReqMsg.equipmentSn, 6);
		memcpy(pmac->equipmentSn, echoReqMsg.equipmentSn, 6);
        pmac->onLineStatus = ONLINE;
        pmac->sessionFd = fd;
        macInfoNode.next = NULL;

        pthread_mutex_lock(&mac_cache_mutex);

        insertNode(&macInfoNode, mac_cache_name);

        pthread_mutex_unlock(&mac_cache_mutex);

        pCnxt->echoFlag = 1;

        char command[64] = "set online-";
        changeMac(pmac->equipmentSn, formatMac, 6);
        strcat(command, formatMac);
        strcat(command, "  1");
        int setFlag;

        setFlag = setKeyValueToRedis(command);

        if(setFlag == 0)
            dbgTrace("success set the key\n");

        setMacAndIP(formatMac);

    }

    pCnxt->m_lastecho = get_now();

    send_echofun(fd, clientSn);

    return 1;
}


int send_echofun(int fd, u16_t clientSn)
{
    dbgTrace("%s  clientSn=%d\n", __FUNCTION__, clientSn);
    FdProcess *pCnxt;
    pCnxt = gFdProcess[fd];

    pCnxt->recvsn++;
    u32_t key = pCnxt->client_key;

    THDR echoRespHdr;
    THDR *pHdr = &echoRespHdr;
    memset(pHdr, 0, sizeof(*pHdr));

    pHdr->flag = PKT_HDR_MAGIC;
    pHdr->version = PKT_VERSION;
    pHdr->pktType = PKT_ECHO_RESPONSE;
    pHdr->sn = pCnxt->recvsn;

    TECHORESP echoRespMsg;
    TECHORESP *pechoRespMsg = &echoRespMsg;

    pechoRespMsg->client_sn = clientSn;

    pHdr->pktlen = sizeof(TECHORESP);

    TECHORESP sendEchoRespMsg;

	/*pengruofeng debug mips*/
	print_echoresp(pHdr,pechoRespMsg);

    XORencode(pechoRespMsg, &sendEchoRespMsg, key, pHdr->pktlen);

    char sendMessage[16] = {0};
    int hdrLen = sizeof(THDR);
    int sendLen = hdrLen + sizeof(TECHORESP);

    memcpy(sendMessage, pHdr, hdrLen);
    memcpy(sendMessage + hdrLen, &sendEchoRespMsg, pHdr->pktlen);

    send(fd, sendMessage, sendLen, 0);

    return 1;
}

/*
  *完成加密协商
  *1.获取客户端的密钥并保持
  *2.把服务器端密钥传递给客户端
  *3.记录第一次发送challenge消息的时间
*/
int recv_chalfun(int fd, char *pbuff)
{
    THDR *pHdr;
    pHdr = (THDR *)pbuff;

    u16_t client_sn = pHdr->sn;

    FdProcess *pCnxt;
    pCnxt = gFdProcess[fd];

    TCHALREQ  Rchal_reqmsg;
    TCHALREQ  *pReq = &Rchal_reqmsg;
    memset(pReq, 0, sizeof(*pReq));

    des_decode((void *)(pbuff + sizeof(THDR)), pReq, CHANLLENGE_KEY, sizeof(*pReq));
	
	/*pengruofeng debug mips*/
	print_chalreq(pHdr,pReq);

    pCnxt->client_key = Rchal_reqmsg.key;

    dbgTrace("client key %d\n", Rchal_reqmsg.key);

    send_chalfun(fd, client_sn);

    return 1;
}

/*
  *把服务器分配给客户端的密钥加密
  *把密钥发送给客户端
*/
int send_chalfun(int fd, u16_t client_sn)
{
    FdProcess *pCnxt;
    pCnxt = gFdProcess[fd];
    pCnxt->recvsn++;

    THDR chal_header;
    THDR *pHdr = &chal_header;
    memset(pHdr, 0, sizeof(*pHdr));

    pHdr->flag = PKT_HDR_MAGIC;
    pHdr->pktlen = sizeof(TCHALRESP);
    pHdr->version = PKT_VERSION;
    pHdr->pktType = PKT_CHALLENGE_RESPONSE;
    pHdr->sn = pCnxt->recvsn;

    TCHALRESP chal_respmsg;
    TCHALRESP *pResp = &chal_respmsg;
    memset(pResp, 0, sizeof(pResp));

    pResp->client_sn = client_sn;

    u32_t key = (u32_t)time(NULL);

    dbgTrace("server key %d\n", key);

    pCnxt->server_key = key;
    memcpy(pResp->key, &key, sizeof(key));

    u32_t magic = PKT_CHALLENGE_MAGIC;
    memcpy(pResp->magic, &magic, sizeof(magic));

    memset(pResp->u6res, sizeof(pResp->u6res), sizeof(pResp->u6res));

	/*pengruofeng debug mips*/
	print_chalresp(pHdr,pResp);

    DES_chalfun(fd, &chal_header, &chal_respmsg);

    return 1;
}

int DES_chalfun(int fd, THDR *pHdr, TCHALRESP *pChalresp)
{
    unsigned char Chaldata[64] = {0};

    int bytes = sizeof(*pChalresp);

    des_encode((const void *)pChalresp, Chaldata, CHANLLENGE_KEY, bytes);

    pHdr->pktlen = bytes;

    char sendMessage[32] = {0};

    int hdrLen = sizeof(THDR);
    int sendLen = hdrLen + bytes;

    memcpy(sendMessage, pHdr, hdrLen);
    memcpy(sendMessage + hdrLen, Chaldata, bytes);

    send(fd, sendMessage, sendLen, 0);
    return 1;
}
int recv_notifyreq_fun(int fd, char *pbuff)
{

    THDR *pHdr;
    pHdr = (THDR *)pbuff;
    int clientSn = pHdr->sn;

    FdProcess *pCnxt;
    pCnxt = gFdProcess[fd];
    u32_t serverKey = pCnxt->server_key;

    TNOTIFYREQ notifyReqMsg;
    memset(&notifyReqMsg, 0, sizeof(notifyReqMsg));

    XORencode(pbuff + sizeof(THDR), &notifyReqMsg, serverKey, pHdr->pktlen);

    char equipmentSn[8] = {0};

    //strncpy(equipmentSn, notifyReqMsg.equipmentSn, 6);
	memcpy(equipmentSn, notifyReqMsg.equipmentSn, 6);

    dbgTrace("%s:%d  equipmentSn:", __FUNCTION__, __LINE__);
    printf_equipmentsn(equipmentSn, 6);
    pthread_mutex_lock(&mac_cache_mutex);
    int sfd = getFdFromCache(equipmentSn);
    pthread_mutex_unlock(&mac_cache_mutex);

    TNOTIFYRESP notifyRespMsg;
    memset(&notifyRespMsg, 0, sizeof(notifyRespMsg));
    notifyRespMsg.returnSn = clientSn;

    if(-1 == sfd || -2 == sfd)
    {
        notifyRespMsg.returnCode = DEVICE_NOT_EXIST;
        dbgTrace("DEVICE_NOT_EXIST [%s:%d]\n", __FUNCTION__, __LINE__);
    }
    else if(-3 == sfd)
    {
        notifyRespMsg.returnCode = SESSION_NOT_EXIST;
        dbgTrace("SESSION_NOT_EXIST [%s:%d]\n", __FUNCTION__, __LINE__);
    }
    else if(sfd > 0)
    {
        int sendFlag;
        sendFlag = send_notifyreq_fun(sfd, &notifyReqMsg);
        dbgTrace("OK [%s:%d]\n", __FUNCTION__, __LINE__);

        if(sendFlag > 0)
        {
            notifyRespMsg.returnCode = OK;
        }
        else
        {
            notifyRespMsg.returnCode = SEND_FAILED_SESSION_EXIST;
        }

    }

    send_notifyresp_fun(fd, &notifyRespMsg);

    return 1;
}

int getFdFromCache(char *equipmentSn)
{
    int fd ;

    if(equipmentSn == NULL || strlen(equipmentSn) == 0)
    {
        return -1;
    }

    struct listnode * pmac;

    pmac = findNode(equipmentSn, mac_cache_name);

    if(NULL == pmac)
    {
        return -2;
    }
    else
    {
        if(pmac->macInfo.onLineStatus == ONLINE)
        {
            fd = pmac->macInfo.sessionFd;
            return fd;
        }
        else
        {
            return -3;
        }

    }
}

int send_notifyreq_fun(int fd, TNOTIFYREQ *pnotifyReqMsg)
{
    if(NULL == pnotifyReqMsg)
    {
        infoTrace("函数%s:第%d 行消息长度为0\n", __FUNCTION__, __LINE__);
        return -1;
    }

    FdProcess *pCnxt;
    pCnxt = gFdProcess[fd];
    pCnxt->recvsn++;
    u32_t key = pCnxt->client_key;

    THDR notifyRequestHdr;
    THDR *pHdr = &notifyRequestHdr;
    memset(pHdr, 0, sizeof(*pHdr));

    pHdr->flag = PKT_HDR_MAGIC;
    pHdr->version = PKT_VERSION;
    pHdr->pktType = PKT_NOTIFY_REQUEST;
    pHdr->sn = pCnxt->recvsn;

    TNOTIFYREQ sendNotifyReqMsg;

    pHdr->pktlen = sizeof(sendNotifyReqMsg);

    XORencode(pnotifyReqMsg, &sendNotifyReqMsg, key, pHdr->pktlen);

    int sendLen = 0;
    char sendMessage[32] = {0};
    int hdrLen = sizeof(THDR);
    int needLen = hdrLen + sizeof(sendNotifyReqMsg);

    memcpy(sendMessage, pHdr, hdrLen);
    memcpy(sendMessage + hdrLen, &sendNotifyReqMsg, pHdr->pktlen);

    sendLen = send(fd, sendMessage, needLen, 0);

    if(sendLen == needLen)
    {
        return 1;
    }
    else
    {
        return -2;
    }
}

int send_notifyresp_fun(int fd, TNOTIFYRESP *pnotifyRespMsg)
{
    FdProcess *pCnxt;
    pCnxt = gFdProcess[fd];

    pCnxt->recvsn++;
    u32_t key = pCnxt->client_key;

    THDR notifyRespHdr;
    THDR *pHdr = &notifyRespHdr;
    memset(pHdr, 0, sizeof(*pHdr));

    pHdr->flag = PKT_HDR_MAGIC;
    pHdr->version = PKT_VERSION;
    pHdr->pktType = PKT_NOTIFY_RESPONSE;
    pHdr->sn = pCnxt->recvsn;

    TNOTIFYRESP  sendNotifyRespMsg;

    pHdr->pktlen = sizeof(sendNotifyRespMsg);

    dbgTrace("%s:len=%d\n", __FUNCTION__, pHdr->pktlen);

    XORencode(pnotifyRespMsg, &sendNotifyRespMsg, key, pHdr->pktlen);

    char sendMessage[24] = {0};
    int hdrLen = sizeof(THDR);
    int sendLen = hdrLen + pHdr->pktlen;

    memcpy(sendMessage, pHdr, hdrLen);
    memcpy(sendMessage + hdrLen, &sendNotifyRespMsg, pHdr->pktlen);

    send(fd, sendMessage, sendLen, 0);

    return 1;
}


int recv_notifyresp_fun(int fd, char *pbuff)
{

    THDR *pHdr;
    pHdr = (THDR *)pbuff;

    FdProcess *pCnxt;
    pCnxt = gFdProcess[fd];
    u32_t key = pCnxt->server_key;

    TNOTIFYRESP  notifyReqMsg;

    XORencode(pbuff + sizeof(THDR), &notifyReqMsg, key, pHdr->pktlen);

    return 1;
}



int setMacAndIP(char *mac)
{
    int setFlag = -1;
    char command[64] = "set heartbeat-";
    strcat(command, mac);
    strcat(command, "  ");
    int ipLen = 0;
    char *localHostip = globalHostIp;
    char localport[8] = {0};

    dbgTrace("%s %d  %s\n", __FUNCTION__, __LINE__, globalHostIp);
        ipLen = strlen(localHostip);
        strncat(command, localHostip, ipLen);
        strcat(command, "-");
        sprintf(localport, "%d", globalListenPort);
        strcat(command, localport);

        setFlag = setKeyValueToRedis(command);

        if(setFlag == 0)
            dbgTrace("%s:%s\n", __FUNCTION__, command);
        else
            dbgTrace("%s:%s\n", __FUNCTION__ , command);


    return setFlag;
}

int changeMac(char *src, char *dest, int srcLen)
{
    int i;

    for(i = 0; i < srcLen; i++)
    {
        sprintf(dest + i * 3, "%02X-", (unsigned char)src[i]);
    }

    dest[i * 3 - 1] = '\0';
    printf("[%s]\n", dest);
    return 0;
}

int getInterval()
{
    int interval = 30;

    char readInterval[16] = {0};
    GetProfileString("./initConf.conf", "heartbeat_server", "heartbeatInterval", readInterval);

    if(strlen(readInterval) > 0)
    {
        interval = atoi(readInterval);
    }

    return interval;

}

int printf_equipmentsn(char *src, int len)
{
    int i;

    char dest[64] = {0};

    for(i = 0; i < len; i++)
    {
        sprintf(dest + i * 3, "%02X-", (unsigned char)src[i]);
    }

    dest[i * 3 - 1] = '\0';
    printf("[%s]\n", dest);

    return 0;

}
