/*
 * Copyright (C) 2017 leon
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 */


/* FILE   : h264reader.c
 * AUTHOR : leon
 * DATE   : Dec, 9 2015
 * DESC   :
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "h264reader.h"

#define DEF_H264_DELIMITER      0x01000000
typedef struct tH264FrameBuf
    {
    void* data;
    int   curSize;
    int   maxSize;
    FILE  *pFile;
    }tH264FrameBuf;

/*
 * PURPOSE : Export 1 H264 frame from reader
 * INPUT   : [reader] - Reader pointer
 * OUTPUT  : [data]   - Frame pointer
 *           [size]   - Frame size
 * RETURN  : Frame size
 * DESCRIPT: None
 */
static int
H264FileReaderFrameExport(void* reader, void* data, int* size)
    {
    int            i = 0;
    tH264FrameBuf  *r = (tH264FrameBuf*)reader;
    char           *buf = (char*)(r->data);
    int            syncWord = 0;
    char           *pSOF = NULL;
    char           *pEOF = NULL;
    int            iFrameSize = 0;

    /* Find first sync word */
    for(i = 0; i < r->curSize - 4; i++)
        {
        memcpy(&syncWord, buf + i, 4);
        if(syncWord == DEF_H264_DELIMITER)
            {
            pSOF = buf + i;
            break;
            }
        }

    /* Exit if not found */
    if(pSOF == NULL)
        return 0;
    else if(pSOF != buf)
        {
        /* Move to index 0 */
        memmove(r->data, pEOF, ((buf + r->curSize) - pSOF));
        r->curSize = r->curSize - (pSOF - buf);
        pSOF = buf;
        }

    /* Search next sync word */
    for(i = 4;i < r->curSize - 4; i++)
        {
        memcpy(&syncWord, buf + i, 4);
        if(syncWord == DEF_H264_DELIMITER)
            {
            pEOF = buf + i;
            iFrameSize = pEOF - pSOF;
            if(iFrameSize < 100)
                continue;
            else
                break;
            }
        }

    /* Move data out */
    if(pEOF != NULL)
        {
        iFrameSize = pEOF - pSOF;
        if(size)
            {
            if(*size < iFrameSize)
                return 0;
            *size = iFrameSize;
            }
        if(data)
            memmove(data, pSOF, iFrameSize);
        memmove(r->data, pEOF, r->curSize - iFrameSize);
        r->curSize = r->curSize - iFrameSize;
        return iFrameSize;
        }
    return 0;
    }

/*
 * PURPOSE : Remove H264 file reader
 * INPUT   : [reader] - Reader pointer
 * OUTPUT  : None
 * RETURN  : None
 * DESCRIPT: None
 */
void
H264FileReaderRemove(void* reader)
    {
    tH264FrameBuf* r = NULL;
    if(reader)
        {
        r = (tH264FrameBuf*)reader;
        if(r->pFile)
            {
            fclose(r->pFile);
            r->pFile = NULL;
            }
        if(r->data)
            {
            free(r->data);
            r->data = NULL;
            }
        free(reader);
        reader = NULL;
        }
    }

/*
 * PURPOSE : Create new H264 file reader
 * INPUT   : [fileName] - File name
 * OUTPUT  : None
 * RETURN  : Reader pointer
 * DESCRIPT: None
 */
void*
H264FileReaderCreate(char* fileName)
    {
    if(fileName == NULL)
        return NULL;

    tH264FrameBuf* reader = malloc(sizeof(tH264FrameBuf));
    if(reader == NULL)
        return NULL;

    reader->pFile = fopen(fileName, "rb");
    if(reader->pFile == NULL)
        {
        H264FileReaderRemove(reader);
        return NULL;
        }

    reader->curSize = 0;
    reader->maxSize = (1024*1024);
    reader->data = (void*)malloc(sizeof(char) * reader->maxSize);
    if(reader->data == NULL)
        {
        H264FileReaderRemove(reader);
        return NULL;
        }
    return reader;
    }

/*
 * PURPOSE : Read 1 H264 frame from file
 * INPUT   : [reader] - Reader pointer
 * OUTPUT  : [data]   - Frame pointer
 *           [size]   - Frame size
 * RETURN  : Frame size. 0 if EOF
 * DESCRIPT: None
 */
int
H264FileReaderGetFrame(void* reader, void* data, int* size)
    {
    tH264FrameBuf* r = NULL;
    int            readSize = 0;
    int            iRetCode = 0;

    /* Check input */
    if(reader == NULL)
        return -1;
    r = (tH264FrameBuf*)reader;
    do
        {
        readSize = fread(r->data + r->curSize, sizeof(char), r->maxSize - r->curSize, r->pFile);
        if(readSize < 0)
            {
            iRetCode = -2;
            break;
            }
        else if(readSize == 0)
            {
            iRetCode = 0;
            break;
            }
        else
            {
            r->curSize += readSize;
            iRetCode = H264FileReaderFrameExport(reader, data, size);
            if(iRetCode > 0)
                break;
            else
                {
                r->data = realloc(r->data, r->maxSize + 100*1024);
                if(r->data)
                    {
                    r->maxSize += 100*1024;
                    continue;
                    }
                else
                    {
                    iRetCode = 0;
                    break;
                    }
                }
            }
        }while(!feof(r->pFile));
    return iRetCode;
    }

/*
 * PURPOSE : Read all data of file
 * INPUT   : [reader] - Reader pointer
 * OUTPUT  : [data]   - Frame pointer
 *           [size]   - Frame size
 * RETURN  : Frame size. 0 if EOF
 * DESCRIPT: None
 */
int
H264FileReaderGetAll(void* reader, void* data, int* size)
    {
    tH264FrameBuf* r = NULL;
    long int       readSize = 0;

    /* Check input */
    if(reader == NULL)
        return -1;
    r = (tH264FrameBuf*)reader;
    fseek(r->pFile, 0, SEEK_END);
    readSize = ftell(r->pFile);
    rewind(r->pFile);
    if(size)
        *size = readSize;
    return fread(data, sizeof(char), readSize, r->pFile);
    }
