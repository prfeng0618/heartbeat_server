#ifdef _WIN32
#include <windows.h>
#include <direct.h>
#else
#include <sys/stat.h>
#include <sys/types.h>
#endif

#include <stdio.h>
#include <unistd.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>
#include "debug.h"


int i32DbgLvl = DBG_LVL_DBG;

void dump(unsigned char *p, int n)
{
	int i;
	for(i=0; i<n; i++)
	{
		trace("0x%02X ", p[i]);
		if(i%16 == 15)
			trace("\n");
	}
	if(i%16)
		trace("\n");
}

void OutputDbgString(char *buf, int len)
{
	const int block = 512;
	if(len < 0)
		len = strlen(buf);
	while(len > block)
	{
		char ch;

		ch = buf[block];
		buf[block] = 0;
#ifdef _WIN32
		OutputDebugString(buf);
#else
		printf(buf);
#endif
		//OutputDebugString("\n");
		buf[block] = ch;
		buf += block;
		len -= block;
	}

	if(len > 0)
	{
#ifdef _WIN32		
		OutputDebugString(buf);
#else
		printf(buf);
#endif
	}
}

#ifdef _WIN32
#define vsnprintf _vsnprintf
#endif

#define time_format tm_now->tm_year+1900,tm_now->tm_mon,tm_now->tm_mday,tm_now->tm_hour,tm_now->tm_min,tm_now->tm_sec

#if 0
void _trace (char *filename, int line, const char *format, ... )
{
	va_list args;
	int nBuf;
	char szBuffer[0x800];

	va_start(args, format);
	nBuf = vsnprintf(szBuffer, sizeof(szBuffer), format, args);

	OutputDbgString(szBuffer, nBuf);
	va_end(args);
}
#else
void _trace (char *filename, int line, const char *format, ... )
{
   va_list vlist;
/*
	struct tm {
			int tm_sec; 		  // �� �C ȡֵ����Ϊ[0,59] 
			int tm_min; 		  // �� - ȡֵ����Ϊ[0,59] 
			int tm_hour;		  // ʱ - ȡֵ����Ϊ[0,23] 
			int tm_mday;		// һ�����е����� - ȡֵ����Ϊ[1,31] 
			int tm_mon; 		 // �·ݣ���һ�¿�ʼ��0����һ�£� - ȡֵ����Ϊ[0,11] 
			int tm_year;		  // ��ݣ���ֵ����ʵ����ݼ�ȥ1900 
			int tm_wday;		// ���� �C ȡֵ����Ϊ[0,6]������0���������죬1��������һ 
			int tm_yday;		 // ��ÿ��1��1�տ�ʼ�������C ȡֵ����[0,365]������0����1��1�� 
			int tm_isdst;	//����ʱ��ʶ��������ʱtm_isdstΪ������ʵ������ʱtm_isdstΪ0��
	};
*/
	struct tm *tm_now;
	time_t now;
	/*gmtime�õ�����0ʱ��ʱ�䣬localtime��ȡ����ʱ��ʱ��*/
#if 0 
	time(&now);
	tm_now = gmtime(&now);
#else
	now = time(NULL);
	tm_now = localtime(&now);
#endif
	fprintf(stderr, "[%04d-%02d-%02d %d:%d:%d][%u](%s:%d) ",time_format,
			getpid(),filename, line);
	va_start(vlist, format);
	vfprintf(stderr, format, vlist);
	va_end(vlist);
	fputc('\n', stderr);
}

#endif

