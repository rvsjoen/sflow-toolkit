#ifndef __logger_h__
#define __logger_h__

#include <stdio.h>
#include <stdarg.h>
#include <time.h>

static const int LOGLEVEL_ERROR = 0;
static const int LOGLEVEL_INFO	= 1;
static const int LOGLEVEL_DEBUG	= 2;

/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  logMessage
 *  Description:  This function provides logging
 * =====================================================================================
 */
void logmsg(int severity, const char* format, ...);

#endif
