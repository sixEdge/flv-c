#ifndef __UTIL_H__
#define __UTIL_H__


#include <stdlib.h>
#include <stdio.h>
#include <string.h>


/* error statuses */
typedef char flv_code;  /* use char for type byte haven't been defined */
#define FLV_OK                          0
#define FLV_ERROR_OPEN                  1
#define FLV_ERROR_OPEN_READ             2
#define FLV_ERROR_OPEN_WRITE            3
#define FLV_ERROR_NO_FLV                4
#define FLV_ERROR_EOF                   5
#define FLV_ERROR_MEMORY                6
#define FLV_ERROR_EMPTY_TAG             7
#define FLV_ERROR_INVALID_METADATA_NAME 8
#define FLV_ERROR_INVALID_METADATA      9




typedef unsigned long long  int64;
typedef unsigned long long  u_int64;
typedef unsigned int        u_int;
typedef unsigned short      u_short;
typedef unsigned char       u_char;
typedef u_char              u_byte;
typedef char                byte;


#define swap64_be(val) (((val&0x00000000000000ff) << 56) | ((val&0x000000000000ff00) << 40) \
                    |   ((val&0x0000000000ff0000) << 24) | ((val&0x00000000ff000000) <<  8) \
                    |   ((val&0x000000ff00000000) >>  8) | ((val&0x0000ff0000000000) >> 24) \
                    |   ((val&0x00ff000000000000) >> 40) | ((val&0xff00000000000000) >> 56))

#define swap32_be(val) (((val & 0x000000ff) << 24) | ((val & 0x0000ff00) <<  8) \
                    |   ((val & 0x00ff0000) >>  8) | ((val & 0xff000000) >> 24))

#define swap16_be(val) (((val & 0x00ff) << 8) | ((val & 0xff00) >> 8))

#define u_int24_be2u_int32(val) (((val & 0x0000ff) << 16) | ((val & 0xff0000) >> 16))




#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

void * std_calloc(size_t size);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __UTIL_H__ */