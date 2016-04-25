#define _XOPEN_SOURCE  500
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <unistd.h>

#include <errno.h>

#include <sys/types.h>
#include <sys/stat.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <fcntl.h>
#include <sys/ioctl.h>
#include <pthread.h>

#define errTrace printf

extern void SysExit(void);

extern int InitSegv(char *pCrashFilePath);
extern void PrintStack(FILE *fp);
extern char *GetSignalStr(int signal);

void SysExit(void)
{

}

void handle_sigterm( int sig )
{
	if(sig != SIGTERM && sig != SIGINT)
	{
		PrintStack(NULL);
	}
	
	SysExit();
	errTrace("exiting due to signal %d(%s)\n", sig, GetSignalStr(sig));
	exit( 1 );
}

void handle_sighup( int sig )
{
	const int oerrno = errno;

	errTrace("handle_sighup\n");
	errno = oerrno;
}

void handle_socket_I( int sig )
{
	errTrace("Socket Abort\n");
}

void handle_bus_I( int signal)
{
	errTrace("Bus error!\n");
	handle_sigterm(signal);
}

void handle_Illegal( int signal)
{
	errTrace("Illegal instruction!\n");

	handle_sigterm(signal);
}

void InitAbortHandler(char *pCrashFilePath)
{
	(void) sigset( SIGTERM, handle_sigterm );
	(void) sigset( SIGABRT, handle_sigterm );
	(void) sigset( SIGUSR1, handle_sigterm );
	(void) sigset( SIGINT,  handle_sigterm );
	(void) sigset( SIGPIPE, handle_socket_I );
	(void) sigset( SIGBUS,  handle_bus_I );
	(void) sigset( SIGILL,  handle_Illegal );
	//(void) sigset( SIGTRAP,  handle_bus_I );
	//(void) sigset( SIGFPE,  handle_bus_I );

	signal(SIGPIPE, SIG_IGN);

	InitSegv(pCrashFilePath);
}

