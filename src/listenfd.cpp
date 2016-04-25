#include "network.h"
#include "debug.h"


extern int sv[MAX_CPU][2];

extern pthread_mutex_t dispatch_mutex;
extern pthread_cond_t dispatch_cond;
extern struct accepted_fd *g_fd;

int accept_readfun(int epollfd, int listenfd, timer_link *timerlink)
{
    struct sockaddr_in servaddr;
    int len = sizeof(servaddr);
    int newacceptfd = 0;

    char buff[MAXACCEPTSIZE * 4] = {0};//store 1024 fd
    int acceptindex = 0;//the number of accepted fd
    int *pbuff = (int *)buff;

    while(true)
    {
        memset(&servaddr, 0, sizeof(servaddr));
        newacceptfd =
            accept(listenfd, (struct sockaddr *)&servaddr, (socklen_t *)&len);

        if(newacceptfd > 0)
        {
            infoTrace("\naccept: %d\n", newacceptfd);
        }

        if(newacceptfd > 0)
        {
            /*too many connected client,forbidden any client to connect */
            if(newacceptfd >= MAX_FD)
            {
                keyTrace("accept fd  close fd: %d\n", newacceptfd);

                close(newacceptfd);
                continue;
            }

            /*store every clientfd in array buff*/
            *(pbuff + acceptindex) = newacceptfd;
            acceptindex++;

            if(acceptindex < MAXACCEPTSIZE)
            {
                //  dbgTrace("%s,%d\n", __FUNCTION__, __LINE__);
                continue;
            }

            //if there is 1024 fd,memcpy them in p
            struct accepted_fd *p = new accepted_fd();

            p->len = acceptindex * 4;

            memcpy(p->buff, buff, p->len);

            p->next = NULL;

            pthread_mutex_lock(&dispatch_mutex);

            if(g_fd)
            {
                p->next = g_fd;
                g_fd = p;
            }
            else
            {
                g_fd = p;
            }

            pthread_cond_broadcast(&dispatch_cond);
            pthread_mutex_unlock(&dispatch_mutex);
            acceptindex = 0;

        }
        else if(newacceptfd == -1)
        {
            if(errno != EAGAIN && errno != ECONNABORTED && errno != EPROTO &&
                    errno != EINTR)
            {
                errTrace("errno == %d\n", errno);
                perror("read error1 errno");
                exit(0);
            }
            else
            {

                if(acceptindex)
                {
                    struct accepted_fd *p = new accepted_fd();

                    p->len = acceptindex * 4;
                    memcpy(p->buff, buff, p->len);
                    p->next = NULL;

                    pthread_mutex_lock(&dispatch_mutex);

                    if(g_fd)
                    {
                        p->next = g_fd;
                        g_fd = p;
                    }
                    else
                    {
                        g_fd = p;
                    }

                    pthread_cond_broadcast(&dispatch_cond);
                    pthread_mutex_unlock(&dispatch_mutex);
                    acceptindex = 0;
                }

                break;
            }
        }
        else
        {
            errTrace("errno 3==%d\n", errno);
            exit(1);
            break;
        }
    }

    return 1;
}

int accept_write(int fd)
{
    return 1;
}

struct sendfdbuff order_list[MAX_CPU];

int movefddata(int fd, char *buff, int len)
{
    memmove(order_list[fd].buff + order_list[fd].len, buff, len);
    order_list[fd].len += len;
    return order_list[fd].len;
}

int sendbuff(int fd)
{
    int needsend = order_list[fd].len;//need to send
    int sended = 0;//already send
    int sendlen = 0;
    char *buff = order_list[fd].buff;

    while(true)
    {
        if(needsend - sended > 0)
        {
            sendlen = write(fd, buff + sended, needsend - sended);

            //sendlen = send(fd, buff + sended, needsend - sended, 0);
            //dbgTrace("sendfd = %d, needsend - sended = %d, ", fd, needsend - sended);
            //dbgTrace("len = %d\n", sendlen);

            if(sendlen > 0)
            {
                sended += sendlen;
            }
            else
            {
                break;
            }
        }
        else
        {
            break;
        }
    }

    //send all the data success
    if(needsend == sended)
    {
    }
    else
    {
        memmove(buff, buff + sended, needsend - sended);
    }

    order_list[fd].len = needsend - sended;
    return sended;
}

int fdsend_writefun(int epollfd, int sendfd, timer_link *timerlink)
{
    return sendbuff(sendfd);
}

void *dispatch_conn(void *arg)
{
    static long sum = 0;
    struct accepted_fd *paccept = NULL;
    int cpu_num = *(int *)arg;

    if(cpu_num < 2)
    {
        cpu_num = 2;
    }

    for(;;)
    {
        pthread_mutex_lock(&dispatch_mutex);

        if(g_fd)
        {
            paccept = g_fd;
            g_fd = NULL;

        }
        else
        {
            pthread_cond_wait(&dispatch_cond, &dispatch_mutex);
            paccept = g_fd;
            g_fd = NULL;
        }

        pthread_mutex_unlock(&dispatch_mutex);

        while(paccept)
        {
            int index = sum % (cpu_num - 1) + 1;
            int sendfd = sv[index][1];

            movefddata(sendfd, paccept->buff, paccept->len);
            sendbuff(sendfd);
            ++sum;
            struct accepted_fd *next = paccept->next;
            delete paccept;
            paccept = next;
        }
    }

    return NULL;
}

