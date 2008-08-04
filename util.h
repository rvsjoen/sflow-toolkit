#ifndef __util_h__
#define __util_h__

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <termios.h>
#include <fcntl.h>
#include <unistd.h>

#define UNUSED_ARGUMENT(x) (void)x

/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  disable_echo
 *  Description:  Disable echo to the terminal to remove output of ctrl-characters
 * =====================================================================================
 */
void disable_echo(bool b);

/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  exit_collector
 *  Description:  This function helps do a clean shutdown of the collector with a
 *  			  specific return value
 * =====================================================================================
 */
void exit_collector(int r);

#endif
