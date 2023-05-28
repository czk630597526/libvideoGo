/*
 * stat.h
 *
 */

#ifndef STAT_H_
#define STAT_H_

#include <sys/time.h>
#include <stdint.h>
#include "libv_log.h"
#include "libvideo.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct __STAT_TIME_ST
{
    struct timeval moldtime;
    uint32_t total_tv_usec;      //运行总时间，微秒为单位。
}STAT_TIME_ST;

void stat_time_start(STAT_TIME_ST *pstat);
void stat_time_stop(STAT_TIME_ST *pstat);




#ifdef __cplusplus
}
#endif

#endif /* STAT_H_ */
