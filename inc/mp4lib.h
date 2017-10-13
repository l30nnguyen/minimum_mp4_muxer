/*
 * Copyright (C) 2017 leon
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 */

/* FILE   : mp4lib.h
 * AUTHOR : leon
 * DATE   : OCT 13, 2017
 * DESC   :
 */

#ifndef __LIB_MP4_H__
#define __LIB_MP4_H__

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

typedef enum eMp4FrameType
    {
    eMp4FrameAudio,
    eMp4FrameVideo,
    }eMp4FrameType;

typedef struct tMp4VidInfo
    {
    int width;
    int height;
    int fps;
    } tMp4VidInfo;

typedef struct tMp4AudInfo
    {
    int channels;
    int rate;
    int bits;
    int format;
    int avg_bitrate;
    } tMp4AudInfo;

void
ln_mmp4m_mem_init(void *tmpBuf);

void*
ln_mmp4m_create(char *filename, tMp4VidInfo *, tMp4AudInfo *);

int
ln_mmp4m_close(void *mp4);

int
ln_mmp4m_write_frame(void *mp4, void *data, eMp4FrameType type, int size);

#endif /* __LIB_MP4_H__ */
