/*
 * Copyright (C) 2017 leon
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 */

/* FILE   : mp4lib_file.c
 * AUTHOR : leon
 * DATE   : OCT 13, 2017
 * DESC   :
 */

#include <stdio.h>
#include <unistd.h>
#include "mp4lib_file.h"

void*
LibMp4RecFileCreate(char* file_name)
    {
    return (void*)fopen(file_name, "wb");
    }

int
LibMp4RecFileWrite(void* fdes, void* data, int size)
    {
    FILE *pFile = (FILE*)fdes;
    if(pFile)
        return fwrite(data, sizeof(char), size, pFile);
    else
        return -1;
    }


int
LibMp4RecFileSeek(void* fdes, int offset, int whence)
    {
    FILE *pFile = (FILE*)fdes;
    if(pFile)
        return fseek(pFile, offset, whence);
    else
        return -1;
    }

int
LibMp4RecFileClose(void* fdes)
    {
    FILE *pFile = (FILE*)fdes;
    if(pFile)
        return fclose(pFile);
    else
        return -1;
    }
