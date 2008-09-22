#ifndef __statistics_h__
#define __statistics_h__

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <rrd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <time.h>
#include <getopt.h>
#include <asm/param.h> // The jiffies per second value HZ is defined in param.h

// This is where we place the rrd file
extern char* cwd;

void init_stats();
void update_stats();

#endif
