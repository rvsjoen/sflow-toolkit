#ifndef __logger_h__
#define __logger_h__

static const int LOGLEVEL_ERROR = 0;
static const int LOGLEVEL_INFO	= 1;
static const int LOGLEVEL_DEBUG	= 2;

/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  logMessage
 *  Description:  This function provides logging
 * =====================================================================================
 */
void logMessage(const char* msg, int severity);
void log(int severity, const char* format, ...);

#endif
