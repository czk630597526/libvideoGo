/*
 * stat.c
 *  时间统计库，用于计算多个函数调用总时间。
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

#include "libv_stat.h"

void stat_time_start(STAT_TIME_ST *pstat)
{
    gettimeofday(&(pstat->moldtime), NULL);
}

void stat_time_stop(STAT_TIME_ST *pstat)
{
    struct timeval now_time;
    gettimeofday(&(now_time), NULL);

    int delaytime_us = now_time.tv_usec - pstat->moldtime.tv_usec;
    int delaytime_s = now_time.tv_sec - pstat->moldtime.tv_sec;

    pstat->total_tv_usec += delaytime_s * 1000000 + delaytime_us;
}























