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
#include <unistd.h>

#include "logger.h"

// This is where we place the rrd file
extern char* cwd;

void get_HZ();
void init_stats();
void update_stats(uint32_t samples, uint32_t seconds, uint32_t bytes_written);

#endif
