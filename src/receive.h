#ifndef _RECEIVE_H
#define _RECEIVE_H

#include "network.h"


int ishdrValid(char *pBuff, u32_t len);
int proc_packet(int fd, char *pBuff, int len);



int recv_chalfun(int fd, char *pbuff);
int send_chalfun(int fd, u16_t client_sn);
int DES_chalfun(int fd, THDR *pHdr, TCHALRESP *pChalresp);


int recv_echofun(int fd, char *pbuff);
int send_echofun(int fd, u16_t client_sn);


int recv_notifyreq_fun(int fd, char *pbuff);
int getFdFromCache(char *equipmentSn);


int send_notifyreq_fun(int fd, TNOTIFYREQ *pnotifyReqMsg);

int send_notifyresp_fun(int fd, TNOTIFYRESP *pnotifyRespMsg);

int recv_notifyresp_fun(int fd, char *pbuff);

int setMacAndIP(char *mac);
int changeMac(char *src, char *dest, int srcLen);

int getInterval();


#endif

