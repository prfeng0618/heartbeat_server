#ifndef _PROTYPE_H
#define _PROTYPE_H


typedef  char i8_t;
typedef  short i16_t;
typedef  int i32_t;
typedef  long i64_t;

typedef unsigned char u8_t;
typedef unsigned short u16_t;
typedef unsigned int u32_t;
typedef unsigned long u64_t;


#define PKT_VERSION             1

#define PKT_CHALLENGE_MAGIC     0xaa
#define PKT_HDR_MAGIC           0xab

#define CHANLLENGE_KEY  ((char *)"CVNCHINA")

#define PKT_CHALLENGE_REQUEST       101
#define PKT_CHALLENGE_RESPONSE      102

#define PKT_ECHO_REQUEST            103
#define PKT_ECHO_RESPONSE           104

#define PKT_NOTIFY_REQUEST          105
#define PKT_NOTIFY_RESPONSE         106

#define PKT_REPORT_REQUEST          111
#define PKT_REPORT_RESPONSE         112

#define PKT_ISSUE_REQUEST           113
#define PKT_ISSUE_RESPONSE          114


#define ONLINE 1
#define OFFLINE 0

struct header
{
    u16_t flag;
    u16_t pktlen;
    u8_t  version;
    u8_t  pktType;
    u16_t sn;
    u32_t ext;
};

struct challenge_request
{
    u32_t magic;
    u32_t key;
    u8_t  u8res[8];
};

struct challenge_response
{
    u16_t client_sn;
    u8_t magic[4];
    u8_t key[4];
    u8_t  u6res[6];
};

struct echo_request
{
    i8_t equipmentSn[6];

};

struct echo_response
{
    u16_t client_sn;
};

struct notify_request
{
    i8_t equipmentSn[6];
    u16_t command;
    u32_t sendTime;
};

struct notify_response
{
    u16_t returnSn;
    i16_t  returnCode;
};

struct report_request
{
	u32_t vendor;
};

struct report_response
{
    u16_t client_sn;
};

struct issue_request
{
	u32_t vendor;
	i8_t equipmentSn[6];
};

struct issue_response
{
    u16_t client_sn;
    u16_t response_code;
};


struct mac_info
{
    i32_t sessionFd;
    i8_t equipmentSn[6];
    u16_t onLineStatus;
};

enum return_code
{
    OK = 0,
    DEVICE_NOT_EXIST = 1,
    SESSION_NOT_EXIST = 2,
    SEND_FAILED_SESSION_EXIST = 3
};

typedef struct header THDR;
typedef struct challenge_request TCHALREQ;
typedef struct challenge_response TCHALRESP;
typedef struct echo_request  TECHOREQ;
typedef struct echo_response  TECHORESP;
typedef struct notify_request  TNOTIFYREQ;
typedef struct notify_response  TNOTIFYRESP;
typedef struct report_request TREPORTREQ;
typedef struct report_response TREPORTRESP;
typedef struct issue_request TISSUEREQ;
typedef struct issue_response TISSUERESP;


#endif
