#include "logger.h"
#include <syslog.h>

uint32_t log_level = 0;

void initLogger(char* name)
{
	openlog(name, LOG_NDELAY, LOG_DAEMON);
}

void destroyLogger()
{
	closelog();
}

void logmsg(uint32_t severity, const char* format, ...)
{
	time_t timestamp = time(NULL);
	struct tm* t;
	t = localtime(&timestamp);
	char c[16];
	strftime(c, 16, "%b %d %H:%M:%S", t);

	if(severity <= log_level){
		va_list args;
		va_start( args, format );

		if (severity == LOGLEVEL_ERROR) {
			va_list args2;
			va_copy(args2, args);
			fprintf(stderr, "[%s] (ERR) ", c);
			vfprintf(stderr, format, args2);
			fprintf(stderr, "\n");
			fflush(stderr);

			va_copy(args2, args);
			vsyslog(LOG_ERR, format, args2);
			va_end(args2);
		} 

		if (severity == LOGLEVEL_WARNING && log_level >= LOGLEVEL_WARNING){
			va_list args2;
			va_copy(args2, args);
			fprintf(stdout, "[%s] (WRN) ", c);
			vfprintf(stdout, format, args2);
			fprintf(stdout, "\n");
			fflush(stdout);

			va_copy(args2, args);
			vsyslog(LOG_WARNING, format, args2);
			va_end(args2);
		} 

		if (severity == LOGLEVEL_INFO && log_level >= LOGLEVEL_INFO){
			va_list args2;
			va_copy(args2, args);
			fprintf(stdout, "[%s] (INF) ", c);
			vfprintf(stdout, format, args2);
			fprintf(stdout, "\n");
			fflush(stdout);

			va_copy(args2, args);
			vsyslog(LOG_INFO, format, args2);
			va_end(args2);
		} 

		if (severity == LOGLEVEL_DEBUG && log_level >= LOGLEVEL_DEBUG){
			va_list args2;
			va_copy(args2, args);
			fprintf(stdout, "[%s] (DBG) ", c);
			vfprintf(stdout, format, args2);
			fprintf(stdout, "\n");
			fflush(stdout);
			
			va_copy(args2, args);
			vsyslog(LOG_DEBUG, format, args2);
			va_end(args2);
		}

		va_end( args );
	}
}
