#include "network.h"
#include "debug.h"
#include "xorcode.h"
#include "des.h"
#include "hashlist.h"
#include "operateredis.h"
#include "readconfig.h"
#include "business.h"
#include "receive.h"


#include <stdarg.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <stdlib.h>

/* rabbit mq */
#include <amqp_tcp_socket.h>
#include <amqp.h>
#include <amqp_framing.h>


extern pthread_mutex_t mac_cache_mutex;

char globalMqIp[16] = {0};
int globalPort = 0;
char globalExchangeName[64] = {0};

void die(const char *fmt, ...)
{
  va_list ap;
  va_start(ap, fmt);
  vfprintf(stderr, fmt, ap);
  va_end(ap);
  fprintf(stderr, "\n");
  exit(1);
}

void die_on_error(int x, char const *context)
{
  if (x < 0) {
    fprintf(stderr, "%s: %s\n", context, amqp_error_string2(x));
    exit(1);
  }
}

void die_on_amqp_error(amqp_rpc_reply_t x, char const *context)
{
  switch (x.reply_type) {
  case AMQP_RESPONSE_NORMAL:
    return;

  case AMQP_RESPONSE_NONE:
    fprintf(stderr, "%s: missing RPC reply type!\n", context);
    break;

  case AMQP_RESPONSE_LIBRARY_EXCEPTION:
    fprintf(stderr, "%s: %s\n", context, amqp_error_string2(x.library_error));
    break;

  case AMQP_RESPONSE_SERVER_EXCEPTION:
    switch (x.reply.id) {
    case AMQP_CONNECTION_CLOSE_METHOD: {
      amqp_connection_close_t *m = (amqp_connection_close_t *) x.reply.decoded;
      fprintf(stderr, "%s: server connection error %d, message: %.*s\n",
              context,
              m->reply_code,
              (int) m->reply_text.len, (char *) m->reply_text.bytes);
      break;
    }
    case AMQP_CHANNEL_CLOSE_METHOD: {
      amqp_channel_close_t *m = (amqp_channel_close_t *) x.reply.decoded;
      fprintf(stderr, "%s: server channel error %d, message: %.*s\n",
              context,
              m->reply_code,
              (int) m->reply_text.len, (char *) m->reply_text.bytes);
      break;
    }
    default:
      fprintf(stderr, "%s: unknown server error, method id 0x%08X\n", context, x.reply.id);
      break;
    }
    break;
  }

  exit(1);
}



extern PFdProcess gFdProcess[MAX_FD];

static void print_reportreq(THDR  *tHdr, TREPORTREQ  *reportReq)
{

	dbgTrace("<<-- [report resquest] [hdr]:{flag(0x%04x),pktlen(%d),version(%d),pktType(%d),sn(%d),ext(0x%08x)} \
[data]:{vendor(0x%08x)}\n", 
		tHdr->flag,
		tHdr->pktlen,
		tHdr->version,
		tHdr->pktType,
		tHdr->sn,
		tHdr->ext,
		reportReq->vendor);
}


static void print_reportresp(THDR  *tHdr, TREPORTRESP *reportResp)
{
	dbgTrace("[report response] -->> [hdr]:{flag(0x%04x),pktlen(%d),version(%d),pktType(%d),sn(%d),ext(0x%08x)} \
[data]:{client_sn(%d)}\n", 
		tHdr->flag,
		tHdr->pktlen,
		tHdr->version,
		tHdr->pktType,
		tHdr->sn,
		tHdr->ext,
		reportResp->client_sn);
}


static void print_issuereq(THDR  *tHdr, TISSUEREQ  *issueReq)
{
	dbgTrace("[issue resquest] -->> [hdr]:{flag(0x%04x),pktlen(%d),version(%d),pktType(%d),sn(%d),ext(0x%08x)} \
[data]:{vendor(0x%08x),equipmentSn(0x%02x%02x%02x%02x%02x%02x)}\n", 
		tHdr->flag,
		tHdr->pktlen,
		tHdr->version,
		tHdr->pktType,
		tHdr->sn,
		tHdr->ext,
		issueReq->vendor,
		issueReq->equipmentSn[0],issueReq->equipmentSn[1],issueReq->equipmentSn[2],
		issueReq->equipmentSn[3],issueReq->equipmentSn[4],issueReq->equipmentSn[5]
	);
}

static void print_issueresp(THDR  *tHdr, TISSUERESP*issueResp)
{
	dbgTrace("<<-- [issue response] [hdr]:{flag(0x%04x),pktlen(%d),version(%d),pktType(%d),sn(%d),ext(0x%08x)} \
[data]:{client_sn(%d),response_code(%d)}\n", 
		tHdr->flag,
		tHdr->pktlen,
		tHdr->version,
		tHdr->pktType,
		tHdr->sn,
		tHdr->ext,
		issueResp->client_sn,
		issueResp->response_code);
}

#if 0
int getListenPort()
{
    int sListenPort = listenport;//如果配置文件没有配置，listenport是默认值
    char listenPort[16] = {0};
    GetProfileString("./initConf.conf", "heartbeat_server", "listenport", listenPort);

    if(strlen(listenPort) > 0)
    {
        sListenPort = atoi(listenPort);
    }

    return sListenPort;

}
#endif

int getMqIp(char *mqIp)
{
    GetProfileString("./initConf.conf", "heartbeat_server", "rabbitMqIp", mqIp);
    dbgTrace("%s:%s\n", __FUNCTION__ , mqIp);
	
    if(strlen(mqIp) <= 0)
    {
        strcpy(mqIp, "182.254.146.223");
		exit(-1);
    }
}

int getMqPort()
{
    int iMqPort = 5672;
    char sMqPort[16] = {0};
    static int flag = 0;

    GetProfileString("./initConf.conf", "heartbeat_server", "rabbitMqPort", sMqPort);

    if(flag == 0)
    {
        dbgTrace("%s:%s\n", __FUNCTION__, sMqPort);
        flag = 1;
    }

    if(strlen(sMqPort) > 0)
    {
        iMqPort = atoi(sMqPort);

    }

    return iMqPort;

}



int getMqExchange(char *mqExchange)
{
    GetProfileString("./initConf.conf", "heartbeat_server", "rabbitMqExchange", mqExchange);
    dbgTrace("%s:%s\n", __FUNCTION__ , mqExchange);
	
    if(strlen(mqExchange) <= 0)
    {
        strcpy(mqExchange,"reportInstruction");
		exit(-1);
    }
}


int getMqExchangeType(char *mqExchangeType)
{
    GetProfileString("./initConf.conf", "heartbeat_server", "rabbitMqExchangeType", mqExchangeType);
    dbgTrace("%s:%s\n", __FUNCTION__ , mqExchangeType);
	
    if(strlen(mqExchangeType) <= 0)
    {
        strcpy(mqExchangeType,"fanout");
		exit(-1);
    }
}

int getMqRoutingKey(char *mqRoutingKey)
{
    GetProfileString("./initConf.conf", "heartbeat_server", "rabbitMqRoutingKey", mqRoutingKey);
    dbgTrace("%s:%s\n", __FUNCTION__ , mqRoutingKey);
	
    if(strlen(mqRoutingKey) <= 0)
    {
        strcpy(mqRoutingKey,"test");
		exit(-1);
    }
}




static int send_data_to_mq(char *data, int datalen)
{
#if 0
	char mqIp[16] = "182.254.146.223";
	char const exchange[32] = "reportInstruction";
	const char *exchangetype = "fanout"; /*direct, fanout, topic*/
	char const routingkey[32] = "test";
	int port = 5672; 
#else
	char mqIp[16] = {0};
	int port = 0; 
	char exchange[32] = {0};
	char exchangetype[10] = {0}; /*direct, fanout, topic*/
	char routingkey[32] = {0};
#endif
	int status;
	amqp_bytes_t messagebody = amqp_empty_bytes;
	amqp_socket_t *socket = NULL;
	amqp_connection_state_t conn;


	//hostname = strdup();
	//port = atoi(argv[2]);
	//exchange = argv[3];
	//routingkey = argv[4];

	getMqIp(mqIp);
	port = getMqPort();
	getMqExchange(exchange);
	getMqExchangeType(exchangetype);
	getMqRoutingKey(routingkey);
	
	messagebody = amqp_bytes_malloc(datalen);
	memcpy(messagebody.bytes,data,datalen);
	
	conn = amqp_new_connection();
	
	socket = amqp_tcp_socket_new(conn);
	if (!socket) {
	  die("creating TCP socket");
	}
	
	status = amqp_socket_open(socket, mqIp, port);
	if (status) {
	  die("opening TCP socket");
	}
	
	die_on_amqp_error(amqp_login(conn, "/", 0, 131072, 0, AMQP_SASL_METHOD_PLAIN, "admin", "admin"),
					  "Logging in");
	amqp_channel_open(conn, 1);

	/* pengruofeng declare exchange to fanout */
	amqp_exchange_declare(conn,1,amqp_cstring_bytes(exchange),amqp_cstring_bytes(exchangetype),
						   0, 1, 0, 0, amqp_empty_table);

	die_on_amqp_error(amqp_get_rpc_reply(conn), "Opening channel");
	
	{
	  amqp_basic_properties_t props;
	  props._flags = AMQP_BASIC_CONTENT_TYPE_FLAG | AMQP_BASIC_DELIVERY_MODE_FLAG;
	  props.content_type = amqp_cstring_bytes("text/plain");
	  props.delivery_mode = 2; /* persistent delivery mode */
#if 0
	  die_on_error(amqp_basic_publish(conn,
									  1,
									  amqp_cstring_bytes(exchange),
									  amqp_cstring_bytes(routingkey),
									  0,
									  0,
									  &props,
									  amqp_cstring_bytes((const char*)data)),
				   "Publishing");
#else
	die_on_error(amqp_basic_publish(conn,
									1,
									amqp_cstring_bytes(exchange),
									amqp_cstring_bytes(routingkey),
									0,
									0,
									&props,
									messagebody),
				 "Publishing");
#endif
	}
	
	die_on_amqp_error(amqp_channel_close(conn, 1, AMQP_REPLY_SUCCESS), "Closing channel");
	die_on_amqp_error(amqp_connection_close(conn, AMQP_REPLY_SUCCESS), "Closing connection");
	die_on_error(amqp_destroy_connection(conn), "Ending connection");

	return 0;

}

int send_business_report_response(int fd, TREPORTRESP *preportRespMsg)
{
    FdProcess *pCnxt;
    pCnxt = gFdProcess[fd];

    pCnxt->recvsn++;
    u32_t key = pCnxt->client_key;

    THDR hdr;
    THDR *pHdr = &hdr;
    memset(pHdr, 0, sizeof(*pHdr));

    pHdr->flag = PKT_HDR_MAGIC;
    pHdr->version = PKT_VERSION;
    pHdr->pktType = PKT_REPORT_RESPONSE;
    pHdr->sn = pCnxt->recvsn;

	TREPORTRESP  sendReportRespMsg;
    pHdr->pktlen = sizeof(sendReportRespMsg);

	print_reportresp(pHdr,preportRespMsg);
	
    dbgTrace("%s:len=%d\n", __FUNCTION__, pHdr->pktlen);
    XORencode(preportRespMsg, &sendReportRespMsg, key, pHdr->pktlen);

    char sendMessage[24] = {0};
    int hdrLen = sizeof(THDR);
    int sendLen = hdrLen + pHdr->pktlen;

    memcpy(sendMessage, pHdr, hdrLen);
    memcpy(sendMessage + hdrLen, &sendReportRespMsg, pHdr->pktlen);

    send(fd, sendMessage, sendLen, 0);

    return 1;
}

int recv_business_report_request(int fd, char *pbuff)
{
    THDR *pHdr;
    pHdr = (THDR *)pbuff;
    int clientSn = pHdr->sn;
    char pMsg[512] = {0};
	//u8_t *businessData;
	char *businessData;
	int businessDataLen = 0;

    FdProcess *pCnxt;
    pCnxt = gFdProcess[fd];
    u32_t serverKey = pCnxt->server_key;

    //TREPORTREQ reportReqMsg;
    //memset(&reportReqMsg, 0, sizeof(reportReqMsg));
	TREPORTREQ* pReportReq;


	dbgTrace("[%s]:############# pbuff = %s \n", __FUNCTION__,pbuff + sizeof(THDR));
	XORencode((void *)pbuff + sizeof(THDR), pMsg, serverKey, pHdr->pktlen);
	pReportReq = (TREPORTREQ*)pMsg;

	dbgTrace("[%s]:*********** pMsg(%d)=%s\n", __FUNCTION__,pHdr->pktlen,pMsg);

	print_reportreq(pHdr,pReportReq);
	//proc_report_data(reportReqMsg.vendor);

	/* 将med指令发送到mq队列  */
	businessData = pMsg + 4;
	businessDataLen = pHdr->pktlen -4;
#if 0
	businessData = (u8_t *)malloc(businessDataLen + 1);
	memcpy(businessData,pMsg + 4,businessDataLen);
	businessData[businessDataLen] = '\0';
	dbgTrace("[%s]:businessData(%d)=%s\n", __FUNCTION__,businessDataLen,businessData);
	send_data_to_mq(businessData,businessDataLen);
	free(businessData);
#else
	dbgTrace("[%s]:businessData(%d)=%s\n", __FUNCTION__,businessDataLen,businessData);
	send_data_to_mq(businessData,businessDataLen);
#endif

	/* 组织report response */
    TREPORTRESP reportRespMsg;
    memset(&reportRespMsg, 0, sizeof(reportRespMsg));
	reportRespMsg.client_sn = clientSn;

    send_business_report_response(fd, &reportRespMsg);

    return 1;
}


int send_business_issue_request(int fd, char *issueReqMsg,int issueReqMsgLen)
{
	FdProcess *pCnxt;
	pCnxt = gFdProcess[fd];

	pCnxt->recvsn++;
	u32_t key = pCnxt->client_key;

	THDR hdr;
	THDR *pHdr = &hdr;
	memset(pHdr, 0, sizeof(*pHdr));

	pHdr->flag = PKT_HDR_MAGIC;
	pHdr->version = PKT_VERSION;
	pHdr->pktType = PKT_ISSUE_REQUEST;
	pHdr->sn = pCnxt->recvsn;
	pHdr->pktlen = issueReqMsgLen;

#if 0

	TISSUEREQ sendIssueReqMsg;
	pHdr->pktlen = sizeof(sendIssueReqMsg);

	char *vendor = "myed";
	TISSUEREQ issueReqMsg;
	memcpy((char *)&issueReqMsg.vendor,(char *)vendor,4);
#endif
	TISSUEREQ *pIssueReqMsg;
	pIssueReqMsg = (TISSUEREQ *)issueReqMsg;
	print_issuereq(pHdr,pIssueReqMsg);

	char sendIssueReqMsg[1024] = {0};
	dbgTrace("%s:len=%d\n", __FUNCTION__, pHdr->pktlen);
	XORencode(issueReqMsg, sendIssueReqMsg, key, pHdr->pktlen);

	int hdrLen = sizeof(THDR);
	send(fd, pHdr, hdrLen, 0);
	send(fd, sendIssueReqMsg, pHdr->pktlen, 0);

	return 1;
}


int recv_business_issue_response(int fd,char* pbuff)
{
	THDR *pHdr;
    pHdr = (THDR *)pbuff;

    FdProcess *pCnxt;
    pCnxt = gFdProcess[fd];
    u32_t key = pCnxt->server_key;

    TISSUERESP issueRespMsg;

    XORencode((void *)pbuff + sizeof(THDR), &issueRespMsg, key, pHdr->pktlen);

	print_issueresp(pHdr,&issueRespMsg);
    return 1;
}

int send_business_issue_response(int fd, TISSUERESP *pIssueResp)
{
    FdProcess *pCnxt;
    pCnxt = gFdProcess[fd];

    pCnxt->recvsn++;
    u32_t key = pCnxt->client_key;

    THDR hdr;
    THDR *pHdr = &hdr;
    memset(pHdr, 0, sizeof(*pHdr));

    pHdr->flag = PKT_HDR_MAGIC;
    pHdr->version = PKT_VERSION;
    pHdr->pktType = PKT_ISSUE_RESPONSE;
    pHdr->sn = pCnxt->recvsn;

	TISSUERESP  sendIssueResp;
    pHdr->pktlen = sizeof(sendIssueResp);

	print_issueresp(pHdr,pIssueResp);
	
    dbgTrace("%s:len=%d\n", __FUNCTION__, pHdr->pktlen);
    XORencode(pIssueResp, &sendIssueResp, key, pHdr->pktlen);

    char sendMessage[24] = {0};
    int hdrLen = sizeof(THDR);
    int sendLen = hdrLen + pHdr->pktlen;

    memcpy(sendMessage, pHdr, hdrLen);
    memcpy(sendMessage + hdrLen, &sendIssueResp, pHdr->pktlen);

    send(fd, sendMessage, sendLen, 0);

    return 1;
}


int recv_business_issue_request(int fd,char* pbuff)
{
    THDR *pHdr;
    pHdr = (THDR *)pbuff;
    int clientSn = pHdr->sn;
	char issueReqMsg[1024] = {0};

    FdProcess *pCnxt;
    pCnxt = gFdProcess[fd];
    u32_t serverKey = pCnxt->server_key;

	TISSUEREQ *pIssueReq;

	dbgTrace("%s:len=%d\n", __FUNCTION__, pHdr->pktlen);
    XORencode((void *)pbuff + sizeof(THDR), (void *)issueReqMsg, serverKey, pHdr->pktlen);
	pIssueReq = (TISSUEREQ*)issueReqMsg;
	print_issuereq(pHdr,pIssueReq);

    char equipmentSn[8] = {0};

    //strncpy(equipmentSn, pIssueReq->equipmentSn, 6);
	memcpy(equipmentSn, pIssueReq->equipmentSn, 6);

    dbgTrace("%s:%d  equipmentSn:", __FUNCTION__, __LINE__);
    printf_equipmentsn(equipmentSn, 6);
    pthread_mutex_lock(&mac_cache_mutex);
    int sfd = getFdFromCache(equipmentSn);
    pthread_mutex_unlock(&mac_cache_mutex);

	TISSUERESP issueRespMsg;
    memset(&issueRespMsg, 0, sizeof(issueRespMsg));
    issueRespMsg.client_sn= clientSn;

    if(-1 == sfd || -2 == sfd)
    {
        issueRespMsg.response_code = DEVICE_NOT_EXIST;
        dbgTrace("DEVICE_NOT_EXIST [%s:%d]\n", __FUNCTION__, __LINE__);
    }
    else if(-3 == sfd)
    {
        issueRespMsg.response_code = SESSION_NOT_EXIST;
        dbgTrace("SESSION_NOT_EXIST [%s:%d]\n", __FUNCTION__, __LINE__);
    }
    else if(sfd > 0)
    {
        int sendFlag;
		sendFlag = send_business_issue_request(sfd, issueReqMsg,pHdr->pktlen);
        dbgTrace("OK [%s:%d]\n", __FUNCTION__, __LINE__);

        if(sendFlag > 0)
        {
            issueRespMsg.response_code = OK;
        }
        else
        {
            issueRespMsg.response_code = SEND_FAILED_SESSION_EXIST;
        }
    }

	send_business_issue_response(fd, &issueRespMsg);
    return 1;
}


