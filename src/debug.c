#ifdef _WIN32
#include <windows.h>
#include <direct.h>
#else
#include <sys/stat.h>
#include <sys/types.h>
#endif

#include <stdio.h>
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
void trace (const char *format, ... )
{
	va_list args;
	int nBuf;
	char szBuffer[0x800];

	va_start(args, format);
	nBuf = vsnprintf(szBuffer, sizeof(szBuffer), format, args);

	OutputDbgString(szBuffer, nBuf);
	va_end(args);
}


