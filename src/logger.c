#include "logger.h"
#include <syslog.h>

uint32_t log_level = 0;

void initLogger()
{
	openlog("stcollectd", LOG_NDELAY, LOG_DAEMON);
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
			fprintf(stderr, "[%s] (ERR) ", c);
			vfprintf(stderr, format, args);
			fprintf(stderr, "\n");
			fflush(stderr);

			vsyslog(LOG_ERR, format, args);
		} 

		if (severity == LOGLEVEL_INFO && log_level >= LOGLEVEL_INFO){
			fprintf(stdout, "[%s] (INF) ", c);
			vfprintf(stdout, format, args);
			fprintf(stdout, "\n");
			fflush(stdout);

			vsyslog(LOG_INFO, format, args);
		} 

		if (severity == LOGLEVEL_DEBUG && log_level >= LOGLEVEL_DEBUG){
			fprintf(stdout, "[%s] (DBG) ", c);
			vfprintf(stdout, format, args);
			fprintf(stdout, "\n");
			fflush(stdout);
			
			vsyslog(LOG_DEBUG, format, args);
		}

		va_end( args );
	}
}
