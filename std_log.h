#ifndef __STD_LOG_H__
#define __STD_LOG_H__

#include <stdio.h>
#include <stdarg.h>
#include <string.h>


#if 0

#define std_log_info(format, ...)
#define std_log_info_pos(format, ...)
#define std_log_debug(format, ...)
#define std_log_debug_pos(format, ...)
#define std_log_warn(format, ...)
#define std_log_error(format, ...)

#else

#define std_log_info(format, ...)       __log_info( "[INFO]  " format "\n", ## __VA_ARGS__)
#define std_log_info_pos(format, ...)   __log_info( "[INFO]  %s::%s,\t line %llu:\t " format "\n", __FILE__, __func__, __LINE__, ## __VA_ARGS__)

#define std_log_debug(format, ...)      __log_debug("[DEBUG] " format "\n", ## __VA_ARGS__)
#define std_log_debug_pos(format, ...)  __log_debug("[DEBUG] %s::%s,\t line %llu:\t " format "\n", __FILE__, __func__, __LINE__, ## __VA_ARGS__)

#define std_log_warn(format, ...)       __log_warn( "[WARN]  %s::%s,\t line %llu:\t " format "\n", __FILE__, __func__, __LINE__, ## __VA_ARGS__)

#define std_log_error(format, ...)      __log_error("[ERROR] %s::%s,\t line %llu:\t " format "\n", __FILE__, __func__, __LINE__, ## __VA_ARGS__)

#endif


#define INFO_STREAM     stdout
#define DEBUG_STREAM    stdout
#define WARN_STREAM     stdout
#define ERROR_STREAM    stderr


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


static void
__log_info(const char * format, ...)
{
    va_list args;
    va_start(args, format);
    vfprintf(INFO_STREAM, format, args);
    va_end(args);
}

static void
__log_debug(const char * format, ...)
{
    va_list args;
    va_start(args, format);
    vfprintf(DEBUG_STREAM, format, args);
    va_end(args);
}

static void
__log_warn(const char * format, ...)
{
    va_list args;
    va_start(args, format);
    vfprintf(WARN_STREAM, format, args);
    va_end(args);
}

static void
__log_error(const char * format, ...)
{
    va_list args;
    va_start(args, format);
    vfprintf(ERROR_STREAM, format, args);
    va_end(args);

    if (errno != 0) {
        fprintf(ERROR_STREAM, "\tCause by: %s\n", strerror(errno));
        errno = 0;
    }
}


#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif /* __STD_LOG_H__ */