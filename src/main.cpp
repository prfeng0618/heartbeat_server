#include "network.h"
#include "socket_util.h"
#include "timers.h"


#include "readconfig.h"
#include "debug.h"
#include "hashlist.h"


int sv[MAX_CPU][2] = {};

struct accepted_fd *g_fd = NULL;

pthread_mutex_t dispatch_mutex;
pthread_cond_t dispatch_cond;

PFdProcess gFdProcess[MAX_FD];/*连接管理*/



struct listhash * mac_cache_name;
pthread_mutex_t mac_cache_mutex;
pthread_cond_t mac_cache_cond;

int globalListenPort;//是为了向redis中存储端口号
char globalHostIp[32] = {0};

void InitAbortHandler(char *pCrashFilePath);



int main()
{
    setvbuf(stdout, NULL, _IONBF, 0);

    pthread_mutex_init(&dispatch_mutex, NULL);
    pthread_cond_init(&dispatch_cond, NULL);

    pthread_mutex_init(&mac_cache_mutex, NULL);
    pthread_cond_init(&mac_cache_cond, NULL);

    signal(SIGPIPE, SIG_IGN);

    InitAbortHandler(NULL);

    mac_cache_name = initializetable(TABLESIZE);

    dbgTrace("each connection size =%ld bytes\n", sizeof(FdProcess));
    dbgTrace("all  alloc %ld bytes\n", MAX_FD * sizeof(FdProcess));

    PFdProcess p = (PFdProcess)malloc(MAX_FD * sizeof(FdProcess));

    assert(p);

    //memset(p, 0, MAX_FD * sizeof(FdProcess));

    for(int i = 0; i < MAX_FD; ++i)
    {
        p[i].fd = i;
        p[i].gbuf = NULL;
        p[i].maxLen = 0;
        p[i].echoFlag = 0;
        gFdProcess[i] = &p[i];
    }

    int cpu_num = 1;

    cpu_num = sysconf(_SC_NPROCESSORS_CONF) * 1;

    if(cpu_num < 2)
    {
        cpu_num = 2;
    }

    for(int i = 1; i < cpu_num; ++i)
    {
        pipe(sv[i]);
        dbgTrace(" cpu %d, pipe %d %d\n", i, sv[i][0], sv[i][1]);
    }

    pthread_t t_all[MAX_CPU] = {0};
    cpu_set_t mask[MAX_CPU];

    for(int i = 1; i < cpu_num; ++i)
    {
        CPU_ZERO(&mask[i]);
        CPU_SET(i, &mask[i]);

        struct worker_thread_arg *pagr = new worker_thread_arg();

        pagr->cpuid = i % (sysconf(_SC_NPROCESSORS_CONF) - 1) + 1;
        pagr->orderfd = sv[i][0];

        pthread_create(&t_all[i], NULL, worker_thread, (void *)pagr);
        pthread_setaffinity_np(t_all[i], sizeof(cpu_set_t), (const cpu_set_t *) & (mask[i]));
    }

    pthread_t dispatch_t;
    pthread_create(&dispatch_t, NULL, dispatch_conn, (void *)&cpu_num);

    pthread_t t_main;
    cpu_set_t mask_main;

    CPU_ZERO(&mask_main);
    CPU_SET(0, &mask_main);

    pthread_create(&t_main, NULL, main_thread, NULL);
    pthread_setaffinity_np(t_main, sizeof(cpu_set_t), (const cpu_set_t *)&mask_main);

    pthread_join(t_main, NULL);

    return 0;
}

void *main_thread(void *arg)
{
    int epollfd;
    int rt = 0;
    epollfd = epoll_create(MAXEPOLLEVENT);
    struct sockaddr_in servaddr;

    int listenfd = socket(AF_INET, SOCK_STREAM, IPPROTO_IP);

    if(listenfd <= 0)
    {
        perror("socket");
        return 0;
    }

    rt = set_reused(listenfd);

    if(rt < 0)
    {
        perror("setsockopt");
        exit(1);
    }

    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    int sListenPort = getListenPort();
	getHostIp(globalHostIp);
    servaddr.sin_port = htons(sListenPort);
    globalListenPort = sListenPort;
    infoTrace("listenport = %d\n", sListenPort,globalHostIp);

    rt = bind(listenfd, (struct sockaddr *)&servaddr, sizeof(servaddr));

    if(rt < 0)
    {
        perror("setsockopt");
        exit(1);
    }

    rt = listen(listenfd, 50000);

    if(rt < 0)
    {
        perror("listen");
        exit(1);
    }

    struct epoll_event ev = {0};

    ev.events = EPOLLIN | EPOLLET;

    ev.data.fd = listenfd;

    set_noblock(listenfd);

    gFdProcess[listenfd]->m_readfun = accept_readfun;

    if(epoll_ctl(epollfd, EPOLL_CTL_ADD, listenfd, &ev) == -1)
    {
        perror("epoll_ctl: listen_sock");
        exit(-1);
    }

    struct epoll_event *m_events;

    m_events = (struct epoll_event *)malloc(MAXEPOLLEVENT * sizeof(struct epoll_event));

    timer_link global_timer;

    int outime = 1000;

    while(true)
    {
        process_event(epollfd, m_events, outime, &global_timer);

        if(global_timer.get_arg_time_size() > 0)
        {
            outime = global_timer.get_mintimer();
        }
        else
        {
            outime = 1000;
        }
    }
}

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
void getHostIp(char *hostIp)
{
    static int flag = 0;
    GetProfileString("./initConf.conf", "heartbeat_server", "hostIp", hostIp);
    if(flag == 0)
    {
        dbgTrace("%s:%s\n", __FUNCTION__ , hostIp);
        flag = 1;
    }
    if(strlen(hostIp) <= 0)
    {
        strcpy(hostIp, "10.9.200.12");
		dbgTrace("%s:error:hostIp=%s\n", __FUNCTION__ , hostIp);
		exit(-1);
    }
}
