/*
 * Copyright (C) 2017 leon
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 */

/* FILE   : mp4lib_os.h
 * AUTHOR : leon
 * DATE   : OCT 13, 2017
 * DESC   :
 */

#ifndef __LIB_MP4_OS_H__
#define __LIB_MP4_OS_H__

#include <stdio.h>
#include <unistd.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

typedef unsigned char   u8;
typedef unsigned int    u32;
typedef unsigned short  u16;
typedef char            s8;
typedef int             s32;

#define ln_zalloc(x)      calloc(1, x)
#define ln_free(x)        free(x)

#define __FILENAME__ (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)
#define ln_log_info(fmt, ...)    printf("INFO  <%s:%d> "fmt"\n", __FILENAME__, __LINE__, ##__VA_ARGS__)
#define ln_log_error(fmt, ...)   printf("ERROR <%s:%d> "fmt"\n", __FILENAME__, __LINE__, ##__VA_ARGS__)
#endif
