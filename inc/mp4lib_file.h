/*
 * Copyright (C) 2017 leon
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 */

/* FILE   : mp4lib_file.h
 * AUTHOR : leon
 * DATE   : OCT 13, 2017
 * DESC   :
 */

#ifndef __LIB_MP4_FILE_H__
#define __LIB_MP4_FILE_H__

void*
LibMp4RecFileCreate(char* file_name);

int
LibMp4RecFileWrite(void* fdes, void* data, int size);

int
LibMp4RecFileSeek(void* fdes, int offset, int whence);

int
LibMp4RecFileClose(void* fdes);

#endif /* __LIB_MP4_FILE_H__ */
