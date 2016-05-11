#ifndef NETWORK_H
#define NETWORK_H

#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/tcp.h>
#include <netdb.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <signal.h>
#include <assert.h>
#include <string>
#include <iostream>
#include <sched.h>
#include <sys/types.h> /* See NOTES */
#include <sys/socket.h>


#include "timers.h"
#include "protype.h"
#include "socket_util.h"

const int K = 1024;

// 100 ��������
const int MAX_FD = 100 * 10 * K ;

#define TABLESIZE 100 * 10 * K


// ÿ�δ���event ��Ŀ
const int MAXEPOLLEVENT = 10000;

const int MAXACCEPTSIZE = 1024;

#define MAX_CPU 128

typedef int (*readfun)(int epoll, int fd, timer_link *);
typedef int (*writefun)(int epoll, int fd, timer_link *);
typedef int (*closefun)(int epoll, int fd, timer_link *);
typedef int (*timeoutfun)(int epoll, int fd, timer_link *, time_value tnow);

int accept_readfun(int epoll, int listenfd, timer_link *);
int accept_write(int epoll, int listenfd, timer_link *);

void *dispatch_conn(void *);

int fdsend_writefun(int epollfd, int listenfd, timer_link *timerlink);

int client_readfun(int epoll, int fd, timer_link *timer);
int client_writefun(int epoll, int fd, timer_link *timer);
int client_closefun(int epoll, int fd, timer_link *timer);
int client_timeoutfun(int epoll, int fd, timer_link *, time_value tnow);

int order_readfun(int epoll, int listenfd, timer_link *);

struct worker_thread_arg
{
    int orderfd;  // listen �̻߳����������fd����
    int cpuid;
};

struct accepted_fd
{
    int len;
    char buff[MAXACCEPTSIZE * 4];
    struct accepted_fd *next;
};

struct FdProcess
{
    readfun m_readfun;
    writefun m_writefun;
    closefun m_closefun;
    timeoutfun m_timeoutfun;
    time_value m_timeout;  // timeout��ʱ��
    int fd;//�����׽���
    int m_sended;
    time_value m_lastecho;
    bool m_activeclose;


    u32_t client_key; //�ͻ��˺ͷ���������Կ
    u32_t server_key;
    u16_t recvsn;//��Ϣ���к�
    u8_t equipmentSn[6];//�豸��mac
    char * gbuf; //���ӻ���
    int dataLen;//��ǰ�����е����ݳ���
    int maxLen;//��ǰ����Ĵ�С
    int echoFlag;//�ж��Ƿ��ǵ�һ�η���������Ϣ


    FdProcess()
    {
        init();
    }
    void init()
    {
        m_readfun = NULL;
        m_writefun = NULL;
        m_closefun = NULL;
        m_timeoutfun = NULL;
        m_lastecho = get_now();
        m_sended = 0;
        m_activeclose = false;


        client_key = 0;
        server_key = 0;
		recvsn = 0;
		
        memset(equipmentSn, 0, sizeof(equipmentSn));
        
        dataLen = 0;
        echoFlag = 0;
    }
};

struct sendfdbuff
{
    int len;
    char buff[4 * 30000];
};

typedef struct FdProcess FdProcess;
typedef struct FdProcess *PFdProcess;
#define Log std::cerr

bool connect(int &fd, std::string host, int port);

int getListenPort();
void getHostIp(char *hostIp);


const int listenport = 20000;

bool addtimer(timer_link *, int fd, time_value);

bool checkontimer(timer_link *, int *fd, time_value *);

bool stoptimer(timer_link *, int fd, time_value);

time_value getnexttimer(timer_link *);

void process_event(int epollfd, struct epoll_event *m_events,
                   time_value timeout, timer_link *timers);

void *worker_thread(void *arg);

void *main_thread(void *arg);

void *distpatch_thread(void *arg);

#endif
