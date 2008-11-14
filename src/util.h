#ifndef __util_h__
#define __util_h__

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
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

/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  exit_on_error
 *  Description:  Exits with an error status code
 * =====================================================================================
 */
void exit_on_error();

/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  daemonize
 *  Description:  Makes the calling thread fork into background 
 * =====================================================================================
 */
void daemonize_me();

/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  printInHex
 *  Description:  Print an unsigned char array using hex
 * =====================================================================================
 */
void printInHex(unsigned char* pkt, uint32_t len);

/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  printSingleLineHex
 *  Description:  Used to print the array of unsigned chars on a single line in hex
 * =====================================================================================
 */
void printSingleLineHex(unsigned char* pkt, uint32_t len);

void num_to_ip(uint32_t num, char* buf);
#endif
