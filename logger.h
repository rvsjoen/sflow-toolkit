#ifndef __logger_h__
#define __logger_h__

#include <stdio.h>
#include <stdint.h>
#include <stdarg.h>
#include <time.h>

static const uint32_t LOGLEVEL_ERROR 	= 0;
static const uint32_t LOGLEVEL_INFO		= 1;
static const uint32_t LOGLEVEL_DEBUG	= 2;

// Declare this prototype to avoid an implicit declaration warning
void vsyslog(int priority, const char *format, va_list ap);

/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  initLogger
 *  Description:  Open up the log handle
 * =====================================================================================
 */
void initLogger();

/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  destroyLogger
 *  Description:  Closes the log handle
 * =====================================================================================
 */
void destroyLogger();

/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  logMessage
 *  Description:  This function provides logging, it takes a variable number of
 *  			  arguments like the printf family of functions.
 * =====================================================================================
 */
void logmsg(uint32_t severity, const char* format, ...);

#endif
