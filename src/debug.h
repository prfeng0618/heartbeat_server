#ifndef __DEBUG_H
#define __DEBUG_H

#ifdef __cplusplus
extern "C"
{
#endif 

enum
{
	DBG_LVL_FATAL = 1,
	DBG_LVL_ERROR,
	DBG_LVL_WARN,
	
	DBG_LVL_KEYINFO = 5,
	DBG_LVL_INFO = 6,
	DBG_LVL_DBG,
};


#ifndef TRACE
#ifdef _DEBUG
#define TRACE trace
#else
#define TRACE (1)? (void)0 : trace
#endif
#endif

#ifdef _DEBUG
	#define dbgTrace		(i32DbgLvl<DBG_LVL_DBG) ? (void)0 : trace
	#define infoTrace		(i32DbgLvl<DBG_LVL_INFO) ? (void)0 : trace
	#define keyTrace		(i32DbgLvl<DBG_LVL_KEYINFO) ? (void)0 : trace
	#define warnTrace		(i32DbgLvl<DBG_LVL_WARN) ? (void)0 : trace
	#define errTrace		(i32DbgLvl<DBG_LVL_ERROR) ? (void)0 : trace
	#define fatalTrace		(i32DbgLvl<DBG_LVL_FATAL) ? (void)0 : trace

#else
	#define dbgTrace		(1) ? (void)0 : trace
	#define infoTrace		(1) ? (void)0 : trace
	#define keyTrace		(1) ? (void)0 : trace
	#define warnTrace		(1) ? (void)0 : trace
	#define errTrace		(1) ? (void)0 : trace
	#define fatalTrace		(1) ? (void)0 : trace
#endif

#define dbgCall 		if(i32DbgLvl>=DBG_LVL_DBG)
#define infoCall		if(i32DbgLvl>=DBG_LVL_INFO)
#define keyCall 		if(i32DbgLvl>=DBG_LVL_KEYINFO)
#define warnCall		if(i32DbgLvl>=DBG_LVL_WARN)
#define errCall 		if(i32DbgLvl>=DBG_LVL_ERROR)
#define fatalCall		if(i32DbgLvl>=DBG_LVL_FATAL)

#define dbgEna			i32DbgLvl>=DBG_LVL_DBG
#define infoEna 		i32DbgLvl>=DBG_LVL_INFO
#define keyEna			i32DbgLvl>=DBG_LVL_KEYINFO
#define warnEna 		i32DbgLvl>=DBG_LVL_WARN
#define errEna			i32DbgLvl>=DBG_LVL_ERROR
#define fatalEna		i32DbgLvl>=DBG_LVL_FATAL

#define Trace	infoTrace


#define trace(format...) _trace(__FILE__, __LINE__, format)
void _trace(char *filename, int line, const char* lpszFormat, ...);

#if 0
#define critTrace(_fmt, ...)						\
	do {							\
		va_list vlist;				\
		trace(_fmt); 				\
		openlog("heartbeat_server ", LOG_PID|LOG_CONS, LOG_USER); 		\
		va_start(vlist, _fmt); \
		vsyslog(LOG_CRIT, _fmt, vlist);                 \
		va_end(vlist);								\
		closelog();												\
        } while (0)
#else
//void critTrace(const char *_fmt, ...);
#define critTrace(format...)									\
	do {														\
		_trace(__FILE__, __LINE__,format);				 	    \
		_critTrace(format);										\
	} while (0)
	
void _critTrace(const char *_fmt, ...);
#endif

void dump(unsigned char *p, int n);

extern int i32DbgLvl;

#ifdef __cplusplus
}
#endif 

#endif

