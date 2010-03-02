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

/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  get_HZ
 *  Description:  Get the timer frequency
 * =====================================================================================
 */
void get_HZ();

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
void stats_update_stcollectd(uint32_t seconds, uint64_t	total_datagrams, 
		uint64_t total_samples_flow, uint64_t total_samples_cntr);

#endif
