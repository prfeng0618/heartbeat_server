#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <assert.h>

#include <sys/time.h>
#include <sys/types.h>

#include <signal.h>
#include <time.h>

#include <execinfo.h>

FILE *CreateCrashFile(char *path)
{
	char file[200] = "";
	//char path[100] = "";
	FILE *fpCrash;
	time_t ti;

	if(path == NULL || *path == 0)
		return NULL;
	
	//GetSysJpegPath(path);
	ti = time(NULL);
	{
	struct tm *local;
	local=localtime(&ti);

	sprintf(file, "%s/crh%04d%02d%02d_%02d%02d%02d.txt", path, local->tm_year+1900, local->tm_mon+1, local->tm_mday, local->tm_hour, local->tm_min, local->tm_sec);
	}

	fpCrash = fopen(file, "wb");
	if(fpCrash == NULL)
		return NULL;

	return fpCrash;
}

static void StdSetColor(void)
{
	//fprintf(stderr, "\033[31;47m");
	//fprintf(stderr, "\033[34m");
	fprintf(stderr, "\033[1;31m");
}

static void StdRestoreColor(void)
{
	fprintf(stderr, "\e[0m");
}


#define FP_NUM 	2	
void ShowSegvAddr(void *context, FILE *fp)
{
}

static char szCrashPath[200];
void PrintStack(FILE *fp)
{
	void * array[50];
	int nSize = backtrace(array, 50);
	char ** symbols = backtrace_symbols(array, nSize);
	int i;
	int k;

	FILE *fpOut[FP_NUM];
	fpOut[0] = stderr;
	fpOut[1] = fp;
	StdSetColor();
	for(k=0; k<FP_NUM; k++)
	{
		FILE *fpCrash = fpOut[k];
		if(fpCrash == NULL)
			continue;

		fprintf(fpCrash, "\n\n---------- Call Stack ----------\n");
		for (i = 0; i < nSize; i++)
		{
			fprintf(fpCrash, "%s\n",symbols[i]); 
		}
		fprintf(fpCrash, "--------------------------------\n");
	}
	StdRestoreColor();
	fprintf(stderr, "\n");

 	free(symbols);
}



void segv_handler( int signal, siginfo_t *si, void *context )
{
	FILE *fp;
	//fpOut[0] = stderr;
	fp = CreateCrashFile(szCrashPath);
	
	if( signal == SIGSEGV )
	{
		ShowSegvAddr(context, fp);
	}

	PrintStack(fp);
	exit(1);
}


int InitSegv(char *pCrashFilePath)
{
	struct sigaction sigact;
	sigact.sa_sigaction = segv_handler;
	sigemptyset( &sigact.sa_mask );
	sigact.sa_flags = SA_SIGINFO;
	sigaction( SIGSEGV, &sigact, NULL );

	sigact.sa_sigaction = segv_handler; //abrt_handler;
	sigemptyset( &sigact.sa_mask );
	sigact.sa_flags = SA_SIGINFO;
	sigaction( SIGABRT, &sigact, NULL );

	if(pCrashFilePath != NULL && *pCrashFilePath != 0)
		strcpy(szCrashPath, pCrashFilePath);
	return 0;
}

char *GetSignalStr(int signal)
{
	switch(signal)
	{
    case SIGHUP   ://  1      在控制终端上检测到挂断或控制线程死亡    是
    	return "SIGHUP";
    case SIGINT   ://  2      交互注意信号                            是
    	return "SIGINT";
    case SIGQUIT  ://  3      交互中止信号                            是
    	return "SIGQUIT";
    case SIGILL   ://  4      检测到非法硬件的指令                    是
    	return "SIGILL";
    case SIGTRAP  ://  5      从陷阱中回朔                            否
    	return "SIGTRAP";
    case SIGABRT  ://  6      异常终止信号                            是
    	return "SIGABRT";
    case SIGFPE   ://  8      不正确的算术操作信号                    是
    	return "SIGFPE";
    case SIGKILL  ://  9      终止信号                                是
    	return "SIGKILL";
    case SIGBUS   ://  10     总线错误                                否
    	return "SIGBUS";
    case SIGSEGV  ://  11     检测到非法的内存调用                    是
    	return "SIGSEGV";
    case SIGSYS   ://  12     系统call的错误参数                      否
    	return "SIGSYS";
    case SIGPIPE  ://  13     在无读者的管道上写                      是
    	return "SIGPIPE";
    case SIGALRM  ://  14     报时信号                                是
    	return "SIGALRM";
    case SIGTERM  ://  15     终止信号                                是
    	return "SIGTERM";
    case SIGURG   ://  16     IO信道紧急信号                          否
    	return "SIGURG";
    case SIGSTOP  ://  17     暂停信号                                是
    	return "SIGSTOP";
    case SIGTSTP  ://  18     交互暂停信号                            是
    	return "SIGTSTP";
    case SIGCONT  ://  19     如果暂停则继续                          是
    	return "SIGCONT";
    case SIGCHLD  ://  20     子线程终止或暂停                        是
    	return "SIGCHLD";
    case SIGTTIN  ://  21     后台线程组一成员试图从控制终端上读出    是
    	return "SIGTTIN";
    case SIGTTOU  ://  22     后台线程组的成员试图写到控制终端上      是
    	return "SIGTTOU";
    case SIGIO    ://  23     允许I/O信号                             否
    	return "SIGIO";
    case SIGXCPU  ://  24     超出CPU时限                             否
    	return "SIGXCPU";
    case SIGXFSZ  ://  25     超出文件大小限制                        否
    	return "SIGXFSZ";
    case SIGVTALRM://  26     虚时间警报器                            否
    	return "SIGVTALRM";
    case SIGPROF  ://  27     侧面时间警报器                          否
    	return "SIGPROF";
    case SIGWINCH ://  28     窗口大小的更改                          否
    	return "SIGWINCH";

    case SIGUSR1  ://  30     保留作为用户自定义的信号1               是
    	return "SIGUSR1";
    case SIGUSR2  ://  31     保留作为用户自定义的信号                是 
    	return "SIGUSR2";
	}    

	return "Unkown";
}

