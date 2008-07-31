#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <time.h>
#include "logger.h"

int log_level = 0;

void logMessage(const char* msg, int severity){
	if(severity>= log_level)
		printf("%s\n", msg);
}

void log(int severity, const char* format, ...)
{
	time_t timestamp = time(NULL);
	struct tm* t;
	t = localtime(&timestamp);
	char c[16];
	strftime(c, 16, "%b %d %H:%M:%S", t);

	FILE* f;
	f=fopen("collector.log", "a");
	if(f==NULL)
		exit(1);

	if(severity <= log_level){
		va_list args;
		va_start( args, format );

		if (severity == LOGLEVEL_ERROR) {
			fprintf(stderr, "[%s] (ERR) ", c);
			fprintf(f, "[%s] (ERR) ", c);
			
			vfprintf(stderr, format, args);
			vfprintf(f, format, args);
			
			fprintf(stderr, "\n");
			fprintf(f, "\n");

			fflush(stderr);
			fflush(f);
		} 

		if (severity == LOGLEVEL_INFO && log_level >= LOGLEVEL_INFO){
			fprintf(stdout, "[%s] (INF) ", c);
			fprintf(f, "[%s] (INF) ", c);

			vfprintf(stdout, format, args);
			vfprintf(f, format, args);
			
			fprintf(stdout, "\n");
			fprintf(f, "\n");

			fflush(stdout);
			fflush(f);
		} 

		if (severity == LOGLEVEL_DEBUG && log_level >= LOGLEVEL_DEBUG){
			
			fprintf(stdout, "[%s] (DBG) ", c);
			fprintf(f, "[%s] (DBG) ", c);
			
			vfprintf(stdout, format, args);
			vfprintf(f, format, args);
			
			fprintf(stdout, "\n");
			fprintf(f, "\n");

			fflush(stdout);
			fflush(f);
		}
		va_end( args );
	}

	fclose(f);
}
