/*
 * libv_log.c
 *
 */

#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#include "libv_log.h"
#include "libavutil/log.h"
#include "codec_app_def.h"

static LIBV_LOG_FUN g_log_cbfun = NULL;
static LIBV_LOG_LEVEL_E g_log_level = E_LIBV_LOG_INFO;
static char g_log_file[256] = "./libv_log.log";

static char g_date_format[256] =  "%F %T";
static int g_printf_flg = 1;

/*
 如果log_cbfun不为空，则回调上层的日志函数。否则，写入到文件log_file中;printf_flg:是否打印至前台
 * */
void libv_log_set(LIBV_LOG_FUN log_cbfun, LIBV_LOG_LEVEL_E level, const char* log_file, int printf_flg)
{
    g_log_cbfun = log_cbfun;
    g_log_level = level;
    strncpy(g_log_file, log_file, sizeof(g_log_file) - 1);
    g_printf_flg = printf_flg;
}

static const char *get_level_str(LIBV_LOG_LEVEL_E eLevel)
{
    switch (eLevel)
    {
        case E_LIBV_LOG_ERROR:
            return "ERR";

        case E_LIBV_LOG_WARING:
            return "WAR";

        case E_LIBV_LOG_INFO:
            return "INF";
        case E_LIBV_LOG_DEBUG:
            return "DBG";
        default:
            return "INF";

    }
}

static void libv_log_v(LIBV_LOG_LEVEL_E level, const char* file_name, int line_no, const char* format, va_list va_Arg)
{
    char  content[2048] = "";
    static int last = 0;
    FILE* fp;
    va_list vl;

    if (level > g_log_level)
    {
        return;
    }

    if(strlen(format) > 1600)
    {
        return ;
    }

    va_copy(vl, va_Arg);
    vsnprintf(content,sizeof(content),format,va_Arg);
    va_end(vl);

//    va_start(va_Arg,format);
//    vsnprintf(content,sizeof(content),format,va_Arg);
//    va_end(va_Arg);

    char file_str[256] = "";
    snprintf(file_str, sizeof(file_str), "[%s-%d]",file_name, line_no );

    //加上时间
    char time_buf[256] = "";
    time_t t;
    struct tm tm;

    memset(&t, 0, sizeof(t));
    memset(&tm, 0, sizeof(tm));

    //获得时间
    time(&t);
    localtime_r(&t, &tm);
    strftime(time_buf, sizeof(time_buf), g_date_format, &tm);

    const char *plevel = get_level_str(level);

    if((last > 50000))
    {
        fp = fopen(g_log_file,"w");
        last = 1;
    }
    else
    {
        fp = fopen(g_log_file,"a");
        last ++;
    }

    if (NULL == fp)
    {
        perror("fopen err:[w]");
        printf("open file[%s] is error!\n", g_log_file);
        return;
    }

    fprintf(fp,"\n[%s][%s]%s    %s\n", plevel, time_buf, content, file_str);
    if (1 == g_printf_flg)
    {
        printf("\n[%s][%s]%s    %s\n", plevel, time_buf, content, file_str);
    }

    fclose(fp);
}

void libv_log(LIBV_LOG_LEVEL_E level, const char* file_name, int line_no, const char* format, ...)
{
    va_list vl;
    va_start(vl, format);
    if (g_log_cbfun != NULL)
    {
        g_log_cbfun(level, file_name, line_no, format, vl);
    }
    else
    {
        libv_log_v(level, file_name, line_no, format, vl);
    }
    va_end(vl);
}

//ffmpeg库的log回调。
void ffm_av_log_callback(void* ptr, int level, const char* fmt, va_list vl)
{
    LIBV_LOG_LEVEL_E loc_level = E_LIBV_LOG_DEBUG;

    switch(level)
    {
        case AV_LOG_QUIET      :
        case AV_LOG_ERROR      :
        case AV_LOG_FATAL      :
            loc_level = E_LIBV_LOG_ERROR;
            break;
        case AV_LOG_WARNING    :
            loc_level = E_LIBV_LOG_WARING;
            break;
        case AV_LOG_INFO       :
            loc_level = E_LIBV_LOG_INFO;
            break;
        case AV_LOG_VERBOSE      :
        case AV_LOG_DEBUG     :
            loc_level = E_LIBV_LOG_DEBUG;
            break;
        default:
            break;
    }

    if (g_log_cbfun != NULL)
    {
        g_log_cbfun(loc_level, "ffmpeg", 0, fmt, vl);
    }
    else
    {
        libv_log_v(loc_level, "ffmpeg", 0, fmt, vl);
    }
}

/* 库中级别的定义。
//openh264库的log回调。
//enum {
//  WELS_LOG_QUIET        = 0x00,     // Quiet mode
//  WELS_LOG_ERROR        = 1 << 0,   // Error log iLevel
//  WELS_LOG_WARNING  = 1 << 1,   // Warning log iLevel
//  WELS_LOG_INFO     = 1 << 2,   // Information log iLevel
//  WELS_LOG_DEBUG        = 1 << 3,   // Debug log, critical algo log
//  WELS_LOG_DETAIL       = 1 << 4,   // per packet/frame log
//  WELS_LOG_RESV     = 1 << 5,   // Resversed log iLevel
//  WELS_LOG_LEVEL_COUNT = 6,
//  WELS_LOG_DEFAULT  = WELS_LOG_DEBUG    // Default log iLevel in Wels codec
//};*/


void openh264_log_callback(void* ctx, int level, const char* str)
{
    LIBV_LOG_LEVEL_E loc_level = E_LIBV_LOG_DEBUG;


    switch(level)
    {
        case WELS_LOG_QUIET      :
        case WELS_LOG_ERROR      :
            loc_level = E_LIBV_LOG_ERROR;
            break;
        case WELS_LOG_WARNING    :
            loc_level = E_LIBV_LOG_WARING;
            break;
        case WELS_LOG_INFO       :
            loc_level = E_LIBV_LOG_INFO;
            break;
        case WELS_LOG_DEBUG      :
        case WELS_LOG_DETAIL     :
        case WELS_LOG_RESV       :
        case WELS_LOG_LEVEL_COUNT:
            loc_level = E_LIBV_LOG_DEBUG;
            break;
        default:
            break;
    }

    libv_log(loc_level, "openh264", 0, str);
}





