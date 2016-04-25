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
    case SIGHUP   ://  1      �ڿ����ն��ϼ�⵽�Ҷϻ�����߳�����    ��
    	return "SIGHUP";
    case SIGINT   ://  2      ����ע���ź�                            ��
    	return "SIGINT";
    case SIGQUIT  ://  3      ������ֹ�ź�                            ��
    	return "SIGQUIT";
    case SIGILL   ://  4      ��⵽�Ƿ�Ӳ����ָ��                    ��
    	return "SIGILL";
    case SIGTRAP  ://  5      �������л�˷                            ��
    	return "SIGTRAP";
    case SIGABRT  ://  6      �쳣��ֹ�ź�                            ��
    	return "SIGABRT";
    case SIGFPE   ://  8      ����ȷ�����������ź�                    ��
    	return "SIGFPE";
    case SIGKILL  ://  9      ��ֹ�ź�                                ��
    	return "SIGKILL";
    case SIGBUS   ://  10     ���ߴ���                                ��
    	return "SIGBUS";
    case SIGSEGV  ://  11     ��⵽�Ƿ����ڴ����                    ��
    	return "SIGSEGV";
    case SIGSYS   ://  12     ϵͳcall�Ĵ������                      ��
    	return "SIGSYS";
    case SIGPIPE  ://  13     ���޶��ߵĹܵ���д                      ��
    	return "SIGPIPE";
    case SIGALRM  ://  14     ��ʱ�ź�                                ��
    	return "SIGALRM";
    case SIGTERM  ://  15     ��ֹ�ź�                                ��
    	return "SIGTERM";
    case SIGURG   ://  16     IO�ŵ������ź�                          ��
    	return "SIGURG";
    case SIGSTOP  ://  17     ��ͣ�ź�                                ��
    	return "SIGSTOP";
    case SIGTSTP  ://  18     ������ͣ�ź�                            ��
    	return "SIGTSTP";
    case SIGCONT  ://  19     �����ͣ�����                          ��
    	return "SIGCONT";
    case SIGCHLD  ://  20     ���߳���ֹ����ͣ                        ��
    	return "SIGCHLD";
    case SIGTTIN  ://  21     ��̨�߳���һ��Ա��ͼ�ӿ����ն��϶���    ��
    	return "SIGTTIN";
    case SIGTTOU  ://  22     ��̨�߳���ĳ�Ա��ͼд�������ն���      ��
    	return "SIGTTOU";
    case SIGIO    ://  23     ����I/O�ź�                             ��
    	return "SIGIO";
    case SIGXCPU  ://  24     ����CPUʱ��                             ��
    	return "SIGXCPU";
    case SIGXFSZ  ://  25     �����ļ���С����                        ��
    	return "SIGXFSZ";
    case SIGVTALRM://  26     ��ʱ�侯����                            ��
    	return "SIGVTALRM";
    case SIGPROF  ://  27     ����ʱ�侯����                          ��
    	return "SIGPROF";
    case SIGWINCH ://  28     ���ڴ�С�ĸ���                          ��
    	return "SIGWINCH";

    case SIGUSR1  ://  30     ������Ϊ�û��Զ�����ź�1               ��
    	return "SIGUSR1";
    case SIGUSR2  ://  31     ������Ϊ�û��Զ�����ź�                �� 
    	return "SIGUSR2";
	}    

	return "Unkown";
}

