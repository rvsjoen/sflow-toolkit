#ifndef __logger_h__
#define __logger_h__

#include <stdio.h>
#include <stdint.h>
#include <stdarg.h>
#include <time.h>

static const uint32_t LOGLEVEL_ERROR 	= 0;
static const uint32_t LOGLEVEL_INFO		= 1;
static const uint32_t LOGLEVEL_DEBUG	= 2;

/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  logMessage
 *  Description:  This function provides logging, it takes a variable number of
 *  			  arguments like the printf family of functions.
 * =====================================================================================
 */
void logmsg(uint32_t severity, const char* format, ...);

#endif
