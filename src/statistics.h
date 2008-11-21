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
#include <mqueue.h>

#include "logger.h"

// This is where we place the rrd files
extern char* cwd;

/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  get_HZ
 *  Description:  Get the timer frequency
 * =====================================================================================
 */
void get_HZ();

/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  update_realtime_stats
 *  Description:  Write some realtime stats to /<storagedir>/rstats
 * =====================================================================================
 */
void stats_update_stcollectd_realtime(uint32_t time_start, uint32_t num_agents, uint64_t total_datagrams, 
		uint64_t total_samples_flow, uint64_t total_samples_cntr, uint64_t total_bytes_written);

/*
 * ===  FUNCTION  ======================================================================
 *         Name:  stats_init_stprocessd
 *  Description:  Prepare for collecting statistics from stprocessd
 * =====================================================================================
 */
void stats_init_stprocessd();

/*
 * ===  FUNCTION  ======================================================================
 *         Name:  stats_init_stcollectd
 *  Description:  Prepare for collecting statistics from stcollectd
 * =====================================================================================
 */
void stats_init_stcollectd();

/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  stats_update_stprocessd
 *  Description:  Update the statistics for stprocessd
 * =====================================================================================
 */
void stats_update_stprocessd(uint32_t seconds, mqd_t queue);

/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  stats_update_stcollectd
 *  Description:  Update the statistics for stcollectd
 * =====================================================================================
 */
void stats_update_stcollectd(uint32_t seconds, uint32_t num_agents, uint64_t total_datagrams, 
		uint64_t total_samples_flow, uint64_t total_samples_cntr, uint64_t total_bytes_written);

#endif
