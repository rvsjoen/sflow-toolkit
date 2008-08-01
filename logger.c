#include "logger.h"

int log_level = 0;

void logmsg(int severity, const char* format, ...)
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
		} 

		if (severity == LOGLEVEL_INFO && log_level >= LOGLEVEL_INFO){
			fprintf(stdout, "[%s] (INF) ", c);
			vfprintf(stdout, format, args);
			fprintf(stdout, "\n");
			fflush(stdout);
		} 

		if (severity == LOGLEVEL_DEBUG && log_level >= LOGLEVEL_DEBUG){
			
			fprintf(stdout, "[%s] (DBG) ", c);
			vfprintf(stdout, format, args);
			fprintf(stdout, "\n");
			fflush(stdout);
		}
		va_end( args );
	}
}
