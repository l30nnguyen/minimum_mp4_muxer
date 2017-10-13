/*
 * Copyright (C) 2017 leon
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 */

/* FILE   : main.c
 * AUTHOR : leon
 * DATE   : OCT 13, 2017
 * DESC   :
 */

#include <string.h>
#include "mp4lib.h"
#include "h264reader.h"

#ifndef RECORD_AUDIO_FORMAT_AAC
#define RECORD_AUDIO_FORMAT_AAC     0x41414300
#endif

#ifndef AAC_CODEC_TYPE
#define AAC_CODEC_TYPE              0
#endif

#define AAC_DEBUG_TO_SDCARD         0
#define PCM_FRAME_SIZE              320

#undef clog_error
#undef clog_info

#define __FILENAME__ (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)
#define clog_info(fmt, ...)    printf("<%s:%d> "fmt "\n", __FILENAME__, __LINE__, ##__VA_ARGS__)
#define clog_error(fmt, ...)   printf("<%s:%d> "fmt "\n", __FILENAME__, __LINE__, ##__VA_ARGS__)

typedef struct tAvMP4FrameInfo
    {
    long tag;
    long size;
    } tAvMP4FrameInfo;

/*****************************************************
 *                   MP4 FILE
 *****************************************************/
static void*
Mp4FileOpen(char *file_name)
    {
    void *pRecHandle = NULL;
    tMp4VidInfo video;
    tMp4AudInfo audio;

    video.width = 1920;
    video.height = 1080;
    video.fps = 15;

    audio.channels = 1;
    audio.rate = 8000;
    audio.bits = 16;
    audio.format = RECORD_AUDIO_FORMAT_AAC;
    audio.avg_bitrate = 0;

    pRecHandle = ln_mmp4m_create(file_name, &video, &audio);
    if (!pRecHandle)
        clog_info("Create file %s error", file_name);
    return pRecHandle;
    }

static int Mp4FileFrameAVWrite(void* pRec, void* data, int size)
    {
    int iRetCode = 0;
    iRetCode = ln_mmp4m_write_frame(pRec, (char*) data, 1, size);
    if (iRetCode != 0)
        {
        clog_error("Write data failed. Error %d", iRetCode);
        return -1;
        }
    return iRetCode;
    }

static void Mp4FileClose(void* pRec)
    {
    if (ln_mmp4m_close(pRec) != 0)
        clog_error("ln_mmp4m_close error");
    }

int main(int argc, char** argv)
    {
    char  cFileName[1024] = {0};
    char  *av_frame = NULL;
    int   av_frame_size = 0;
    int   av_max_frame_size = 500*1024;
    void  *pRec = NULL;
    char  *pMemAddr = NULL;
    int   iRetCode = 0;
    int   iFrameCount = 0;
    void  *pH264Reader = NULL;

    memset(cFileName, 0, sizeof(cFileName));
    sprintf(cFileName, "test_lib.mp4");
    if (argc >= 2 && argv[1] != NULL)
        strcpy(cFileName, argv[1]);

    pH264Reader = H264FileReaderCreate("raw.h264");
    if(pH264Reader == NULL)
        return -1;

    pMemAddr = malloc(sizeof(char)* av_max_frame_size);
    if(pMemAddr == NULL)
        {
        H264FileReaderRemove(pH264Reader);
        return -2;
        }

    av_frame = malloc(sizeof(char)* av_max_frame_size);
    if(av_frame == NULL)
        {
        free(pMemAddr);
        H264FileReaderRemove(pH264Reader);
        return -3;
        }

    ln_mmp4m_mem_init(pMemAddr);
    pRec = Mp4FileOpen(cFileName);
    if (pRec == NULL)
        {
        free(pMemAddr);
        free(av_frame);
        H264FileReaderRemove(pH264Reader);
        return -4;
        }

    do
        {
        av_frame_size = av_max_frame_size;
        memset(av_frame, 0, av_max_frame_size);
        iRetCode = H264FileReaderGetFrame(pH264Reader, av_frame, &av_frame_size);
        if(iRetCode == 0)
            break;
        iRetCode = Mp4FileFrameAVWrite(pRec, av_frame, av_frame_size);
        if (iRetCode < 0)
            break;
        iFrameCount++;
        }while(1);
    Mp4FileClose(pRec);
    free(pMemAddr);
    free(av_frame);
    H264FileReaderRemove(pH264Reader);
    clog_info("Record %s done. Total frame %d", cFileName, iFrameCount);
    return 0;
    }
