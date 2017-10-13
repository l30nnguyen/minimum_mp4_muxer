/*
 * Copyright (C) 2017 leon
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 */

/* FILE   : mp4lib.c
 * AUTHOR : leon
 * DATE   : OCT 13, 2017
 * DESC   :
 */

#include "mp4lib_os.h"
#include "mp4lib_file.h"
#include "mp4lib.h"

typedef struct
    {
    tMp4AudInfo    audio;
    tMp4VidInfo    video;
    void *fdes;
    u32  mdata_size;            //box size
    u32  frame_start;           //for stco calculate

    u32  i_count;
    u32  *video_stss_table;     //for stss
    u32  frame_count;
    u32  *video_stco_table;     //for stco
    u32  *video_stsz_table;     //for stsz

    u32  audio_enable;
    u32  sample_count;
    u32  *audio_stco_table;     //for stco
    u32  *audio_stsz_table;     //for stsz

    //for avcC & H264 header handle
    u32  sps_size;
    u8   *sps_info;
    u32  pps_size;
    u8   *pps_info;
    } mp4_t;

///////////////////////
//Default Header Box///
///////////////////////
typedef struct
    {
    u8 size[4]; //0x1C
    u8 name[4];
    u8 version[4];
    u8 entry_count[4];
    u8 first_chunk[4];
    u8 samples_per_chunk[4];
    u8 sample_description_index[4];
    } stsc_t;

typedef struct
    {
    u8 size[4]; //0x18
    u8 name[4];
    u8 version[4];
    u8 entry_count[4];
    u8 counts[4];
    u8 duration[4];
    } stts_t;

typedef struct
    {
    u8 size[4]; //0x18
    u8 name[4];
    u8 version[4];
    u8 entry_count[4];
    u32 modifyinfo[2];
    } stts_modify_t;

typedef struct
    {
    u8 size[4]; //0x22
    u8 name[4];
    u8 version;
    u8 profile;
    u8 profile_compat;
    u8 level;
    u8 nal[2];
    u8 spspps_info[64]; //include pps
    } avcC_t;

typedef struct
    {
    u8 size[4]; //0x78
    u8 name[4];
    u8 version[4];
    u8 reserved[20];
    u8 width[2];
    u8 height[2];
    u8 h_res[4];
    u8 v_res[4];
    u8 reserved_1[42];
    avcC_t avcC;
    } avc1_t;

typedef struct
    {
    u8 size[4]; //0x88
    u8 name[4];
    u8 version[4];
    u8 entry_count[4];
    avc1_t avc1;
    } video_stsd_t;

typedef struct   //0x08(STBL) + 0x88(STSD) + 0x24(STTS) + 0x1C(STSC) + STSS,STSZ,STCO
    {
    u8 size[4];
    u8 name[4];
    video_stsd_t stsd;
    stsc_t stsc;
    stts_t stts;
    } video_stbl_t;

typedef struct
    {
    u8 size[4];
    u8 name[4];
    u8 dref[0x1C];
    } dinf_t;

typedef struct
    {
    u8 size[4];
    u8 name[4];
    u8 version[4];
    u8 graphic_mode[4];
    u8 opcolor[4];
    } vmhd_t;

typedef struct   //0x08(MINF) + 0x14(VMHD) + 0x24(DINF) + Size(STBL)
    {
    u8 size[4];
    u8 name[4];
    vmhd_t vmhd;
    dinf_t dinf;
    video_stbl_t stbl;
    } video_minf_t;

typedef struct
    {
    u8 size[4];
    u8 name[4];
    u8 version[4];
    u8 predefined[4];
    u8 handler_type[4];
    u8 reserved[12];
    u8 handler_name[16];
    } hdlr_t;

typedef struct
    {
    u8 size[4];
    u8 name[4];
    u8 version[4];
    u8 creation_time[4];
    u8 modification_time[4];
    u8 timescale[4];
    u8 duration[4];
    u8 language[2];
    u8 reserved[2];
    } mdhd_t;

typedef struct   //0x08(MDIA) + 0x20(MDHD) + 0x30(HDLR) + Size(MINF)
    {
    u8 size[4];
    u8 name[4];
    mdhd_t mdhd;
    hdlr_t hdlr;
    video_minf_t minf;
    } video_mdia_t;

typedef struct
    {
    u8 size[4];
    u8 name[4];
    u8 elst_sz[4];
    u8 elst_name[4];
    u8 elst_version[4];
    u8 elst_entrycount[4];
    u8 elst_delay[4];
    u8 elst_start_ct[4];
    u8 reserved[4];
    } edts_t;

typedef struct
    {
    u8 size[4];
    u8 name[4];
    u8 version[4];
    u8 creation_time[4];
    u8 modification_time[4];
    u8 track_id[4];
    u8 reserved[4];
    u8 duration[4];
    u8 reserved_1[8];
    u8 layer[2];
    u8 alternate_group[2];
    u8 volume[2];
    u8 reserved_2[38];
    u8 width[4];
    u8 height[4];
    } tkhd_t;

typedef struct	//0x08(TRAK) + 0x5C(TKHD) + 0x24(EDTS) + Size(MDIA)
    {
    u8 size[4];		//Addr:0x20
    u8 name[4];
    tkhd_t tkhd;		//Addr:0x28
    edts_t edts;
    video_mdia_t mdia;
    } ln_mmp4m_vid_trak_t;

typedef struct
    {
    u8 size[4];
    u8 name[4];
    u8 version[4];
    u8 creation_time[4];
    u8 modification_time[4];
    u8 timescale[4];
    u8 duration[4];
    u8 rate[4];
    u8 volume[2];
    u8 reserved[70];
    u8 next_id[4];
    } mvhd_t;

typedef struct tMp4Moov
    {
    u8 moov_sz[4];		//Addr:0x20
    u8 moov[4];
    mvhd_t mvhd;		//Addr:0x28
    ln_mmp4m_vid_trak_t vtrak;
    } ln_mmp4m_moov_t;

u8 u8FtypeBox[] =
    {
    /* FTYP:0x000 */
    0x00, 0x00, 0x00, 0x20,
    'f', 't', 'y', 'p', 'i', 's', 'o', 'm',
    0x00, 0x00, 0x02, 0x00,
    'i', 's', 'o', 'm', 'i', 's', 'o', '2', 'a', 'v', 'c', '1', 'm', 'p', '4', '1',
    };

u8 u8FreeBox[] =
    {
    /* FTYP:0x000 */
    0x00, 0x00, 0x00, 0x08, 'f', 'r', 'e', 'e'
    };

u8 u8MdatBox[] =
    {
    0x00, 0x00, 0x00, 0x08, 'm', 'd', 'a', 't'
    };

u8 u8Mp4VidBox[] =
    {
    /* MOOV:0x020 */
    0xBF, 0xFF, 0xFF, 0xFF, 'm', 'o', 'o', 'v',	//0x08(MOOV) + 0x6C(MVHD) + Size(Video TRAK) + Size(Audio TRAK)

    /* MVHD:0x028 */
    //box size			4		box大小
    0x00, 0x00, 0x00, 0x6C,
    //box type			4		box類型
    'm', 'v', 'h', 'd',
    //version			1		box版本，0或1，一般為0。（以下位元組數均按version=0）
    0x00,
    //flags				3
    0x00, 0x00, 0x00,
    //creation time		4		創建時間（相對於UTC時間1904-01-01零點的秒數）
    0x00, 0x00, 0x00,
    0x00,		//M:
    //modification time	4		修改時間
    0x00, 0x00, 0x00,
    0x00,		//M:
    //time scale		4		檔媒體在1秒時間內的刻度值，可以理解為1秒長度的時間單元數
    0x00, 0x00, 0x03,
    0xE8,    //1000
    //duration			4		該track的時間長度，用duration和time scale值可以計算track時長，比如audio track的time scale = 8000, duration = 560128，時長為70.016，video track的time scale = 600, duration = 42000，時長為70
    0x00, 0x00, 0x03,
    0xE8,    //1000	//M:
    //rate				4		推薦播放速率，高16位元和低16位元分別為小數點整數部分和小數部分，即[16.16] 格式，該值為1.0（0x00010000）表示正常前向播放
    0x00, 0x01, 0x00,
    0x00,    //1.0
    //volume			2		與rate類似，[8.8] 格式，1.0（0x0100）表示最大音量
    0x01,
    0x00,			//1.0
    //reserved			10		保留位
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    //matrix			36		視頻變換矩陣
    0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x00,
    //pre-defined		24
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    //Next id
    0x00, 0x00, 0x00,
    0x02,	//M: (if audio,change to 3)

    /* TRAK:0x094 */
    0xBF, 0xFF, 0xFF, 0xFF, 't', 'r', 'a',
    'k',	//0x08(TRAK) + 0x5C(TKHD) + 0x24(EDTS) + Size(MDIA)

    /* TKHD:0x09C */
    //box size	4	box大小
    0x00, 0x00, 0x00, 0x5C,
//box type	4	box類型
    't', 'k', 'h', 'd',
//version	1	box版本，0或1，一般為0。（以下位元組數均按version=0）
    0x00,
    /*flags		3	按位或操作結果值，預定義如下：
     0x000001 track_enabled，否則該track不被播放；
     0x000002 track_in_movie，表示該track在播放中被引用；
     0x000004 track_in_preview，表示該track在預覽時被引用。
     一般該值為7，如果一個媒體所有track均未設置track_in_movie和track_in_preview，將被理解為所有track均設置了這兩項；對於hint track，該值為0
     */
    0x00, 0x00,
    0x03,			//M:
//creation time		4	創建時間（相對於UTC時間1904-01-01零點的秒數）
    0x00, 0x00, 0x00,
    0x00,	//M:
//modification time	4	修改時間
    0x00, 0x00, 0x00,
    0x00,	//M:
//track id	4	id號，不能重複且不能為0
    0x00, 0x00, 0x00, 0x01,
//reserved	4	保留位
    0x00, 0x00, 0x00, 0x00,
//duration	4	track的時間長度
    0x00, 0x00, 0x03,
    0xE8,	//M:
//reserved	8	保留位
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
//layer	2	視頻層，預設為0，值小的在上層
    0x00, 0x00,
//alternate group	2	track分組資訊，預設為0表示該track未與其他track有群組關係
    0x00, 0x00,
//volume	2	[8.8] 格式，如果為音訊track，1.0（0x0100）表示最大音量；否則為0
    0x00, 0x00,
//reserved	2	保留位
    0x00, 0x00,
//matrix	36	視頻變換矩陣
    0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x00,
//width		4	寬
    0x07, 0x80, 0x00,
    0x00,	//M:
//height	4	高，均為 [16.16] 格式值，與sample描述中的實際畫面大小比值，用於播放時的展示寬高
    0x04, 0x38, 0x00,
    0x00,	//M:

    /* EDTS:0x0F8 */
    0x00, 0x00, 0x00, 0x24, 'e', 'd', 't', 's', 0x00, 0x00, 0x00, 0x1C, 'e', 'l', 's', 't',
    //version
    0x00,
    //flags
    0x00, 0x00, 0x00,
    //entry count
    0x00, 0x00, 0x00, 0x01,
    //delay
    0x00, 0x00, 0x03,
    0xE8,	//M:
    //start_ct
    0x00, 0x00, 0x00, 0x00,
    //
    0x00, 0x01, 0x00, 0x00,

    /* MDIA:0x011C */
    0xBF, 0xFF, 0xFF, 0xFF, 'm', 'd', 'i', 'a',	//0x08(MDIA) + 0x20(MDHD) + 0x30(HDLR) + Size(MINF)

    /* MDHD:0x0124 */
    //box size	4	box大小
    0x00, 0x00, 0x00, 0x20,
//box type	4	box類型
    'm', 'd', 'h', 'd',
//version	1	box版本，0或1，一般為0。（以下位元組數均按version=0）
    0x00,
//flags	3
    0x00, 0x00, 0x00,
//creation time	4	創建時間（相對於UTC時間1904-01-01零點的秒數）
    0x00, 0x00, 0x00,
    0x00,	//M:
//modification time	4	修改時間
    0x00, 0x00, 0x00,
    0x00,	//M:
//time scale	4	同前表
    0x00, 0x00, 0x2E,
    0xE0,	//12000
//duration	4	track的時間長度
    0x00, 0x00, 0x2E,
    0xE0,	//12000	//M:
//language	2	媒體語言碼。最高位為0，後面15位元為3個字元（見ISO 639-2/T標準中定義）
    0x55, 0xC4,
//pre-defined	2
    0x00, 0x00,

    /* HDLR:0x0144 */
//box size	4	box大小
    0x00, 0x00, 0x00, 0x30,
//box type	4	box類型
    'h', 'd', 'l', 'r',
//version	1	box版本，0或1，一般為0。（以下位元組數均按version=0）
    0x00,
//flags	3
    0x00, 0x00, 0x00,
//pre-defined	4
    0x00, 0x00, 0x00, 0x00,
//handler type	4	在media box中，該值為4個字元：
//“vide”— video track
//“soun”— audio track
//“hint”— hint track
    'v', 'i', 'd', 'e',
//reserved	12
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
//name	不定	track type name，以‘\0’結尾的字串
    'l', 'l', 'm', 'i', 'n', 'i', 'm', 'u', 'm', 'm', 'p', '4', 'm', 'u', 'x', 0x00,

    /* MINF:0x0164 */
    0xBF, 0xFF, 0xFF, 0xFF, 'm', 'i', 'n',
    'f',	//0x08(MINF) + 0x14(VMHD) + 0x24(DINF) + Size(STBL)

    /* VMHD:0x016C */
//box size	4	box大小
    0x00, 0x00, 0x00, 0x14,
//box type	4	box類型
    'v', 'm', 'h', 'd',
//version	1	box版本，0或1，一般為0。（以下位元組數均按version=0）
    0x00,
//flags	3
    0x00, 0x00, 0x01,
//graphics mode	4	視頻合成模式，為0時拷貝原始圖像，否則與opcolor進行合成
    0x00, 0x00, 0x00, 0x00,
//opcolor	size according to graphics mode	｛red，green，blue｝
    0x00, 0x00, 0x00, 0x00,

    /* DINF:0x0180 */
//box size	4	box大小
    0x00, 0x00, 0x00, 0x24,
//box type	4	box類型
    'd', 'i', 'n', 'f',
    //box size	4	box大小
    0x00, 0x00, 0x00, 0x1C,
    //box type	4	box類型
    'd', 'r', 'e', 'f',
    //version	1	box版本，0或1，一般為0。（以下位元組數均按version=0）
    0x00,
    //flags	3
    0x00, 0x00, 0x00,
    //entry count	4	“url”或“urn”表的元素個數
    0x00, 0x00, 0x00, 0x01,
    //“url”或“urn”列表	不定,
    //box size	4	box大小
    0x00, 0x00, 0x00, 0x0C,
    //box type	4	box類型
    'u', 'r', 'l', ' ',
    //box flag，值1，表明“url”中的字符串為空，表示track數據已包含在文件中
    0x00, 0x00, 0x00, 0x01,

    /* STBL:0x01B4 */
    0xBF, 0xFF, 0xFF, 0xFF, 's', 't', 'b',
    'l',		//0x08(STBL) + 0x88(STSD) + 0x24(STTS) + Size(STBL)

    /* STSD:0x01BC */
//box size	4	box大小
    0x00, 0x00, 0x00, 0x88,
//box type	4	box類型
    's', 't', 's', 'd',
//version	1	box版本，0或1，一般為0。（以下位元組數均按version=0）
    0x00,
//flags	3
    0x00, 0x00, 0x00,
    //entry count	1
    0x00, 0x00, 0x00, 0x01,
    //box size	4	box大小
    0x00, 0x00, 0x00, 0x78,
    //box type	4	box類型
    'a', 'v', 'c', '1',
    //version & flags
    0x00, 0x00, 0x00, 0x00,
    //Reserved
    0x00, 0x00,
    //Data-reference index
    0x00, 0x01,
    //Codec stream version
    0x00, 0x00,
    //Codec stream revision (=0)
    0x00, 0x00,
    //Reserved
    0x00, 0x00, 0x00, 0x00,
    //Reserved
    0x00, 0x00, 0x00, 0x00,
    //Reserved
    0x00, 0x00, 0x00, 0x00,
    //Video width
    0x07,
    0x80,					//M:
    //Video height
    0x04,
    0x38,					//M:
    //Horizontal resolution 72dpi
    0x00, 0x48, 0x00, 0x00,
    //Vertical resolution 72dpi
    0x00, 0x48, 0x00, 0x00,
    //Data size (= 0)
    0x00, 0x00, 0x00, 0x00,
    //Frame count (= 1)
    0x00, 0x01,
    //compressor_name len
    0x00,
    //compressor_name
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    //Reserved
    0x00, 0x18,
    //Reserved
    0xFF, 0xFF,

    //box size	4	box大小
    0x00, 0x00, 0x00, 0x22,
    //box type	4	box類型
    'a', 'v', 'c', 'C',	//SPS、PPS
    //version
    0x01,
    //profile
    0x64,//100
    //profile compat
    0x00,
    //level
    0x28,//40
    //6 bits reserved (111111) + 2 bits nal size length - 1 (11)
    0xFF,
    //3 bits reserved (111) + 5 bits number of sps (00001)
    0xE1,
    /*---------------fill 64 bytes here-------------------*/
    //sps size  sps data
    0x00, 0x0B, 0x67, 0x64, 0x00, 0x28, 0xAC, 0xE8, 0x07, 0x80, 0x22, 0x7E, 0x54,
    //number of pps
    0x08,
    //pps size  pps data
    0x00, 0x04, 0x68, 0xEE, 0x3C, 0x30,
    0x00, 0x04, 0x68, 0xEE, 0x3C, 0x30,
    0x00, 0x04, 0x68, 0xEE, 0x3C, 0x30,
    0x00, 0x04, 0x68, 0xEE, 0x3C, 0x30,
    0x00, 0x04, 0x68, 0xEE, 0x3C, 0x30,
    0x00, 0x04, 0x68, 0xEE, 0x3C, 0x30,
    0x00, 0x04, 0x68, 0xEE, 0x3C, 0x30,
    0x00, 0x06, 0x68, 0xEB, 0xE3, 0xCB, 0x22, 0xC0,
    /*---------------end of 64 bytes-----------------------*/

    /* STSC:0x25C */
//固定1sample 1chunk就不用update，如Novatek.
//box size	4	box大小	//size = entry count * 12 + 0x10 = 0x70
    0x00, 0x00, 0x00, 0x1C,
//box type	4	box類型
    's', 't', 's', 'c',
//version & flags
    0x00, 0x00, 0x00, 0x00,
//entry count
    0x00, 0x00, 0x00, 0x01,
// first chunk、samples per chunk、sample description index(0x00000001)
    0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01,
    /* STTS:0x244 */
    //box size	4	box大小
    0x00, 0x00, 0x00, 0x18,
//box type	4	box類型
    's', 't', 't', 's',
//version & flags
    0x00, 0x00, 0x00, 0x00,
//entry count
    0x00, 0x00, 0x00, 0x01,
//counts
    0x00, 0x00, 0x00, 0x1E,	//M:
//duration
    0x00, 0x00, 0x01, 0x90,
    //30fps: 12000/30 = 400

    /* STSS:0x278 */
    //STSS
    //STSZ
    //STCO
    };

typedef struct
    {
    u8 size[4];    //0x33
    u8 name[4];
    u8 version[4];
    u8 es_descriptor[8];
    u8 decoderconfig_descriptor[5];
    u8 object_type;
    u8 codec_type;
    u8 buffer_size[3];
    u8 max_bitrate[4];
    u8 avg_bitrate[4];
    u8 decoderspecific_info_descriptor[7];
    u8 sl_descriptor[6];
    } esds_t;

typedef struct
    {
    u8 size[4];    //0x57
    u8 name[4];
    u8 version[4];
    u8 reserved[12];
    u8 channel_count[2];
    u8 sample_size[2];
    u8 reserved_1[4];
    u8 sample_rate[2];
    u8 reserved_2[2];
    esds_t esds;
    } mp4a_t;

typedef struct
    {
    u8 size[4];    //0x67
    u8 name[4];
    u8 version[4];
    u8 entry_count[4];
    mp4a_t mp4a;
    } audio_stsd_t;

typedef struct   //0x08(STBL) + 0x67(STSD) + 0x18(STTS) + 0x1C(STSC) + STSZ,STCO
    {
    u8 size[4];
    u8 name[4];
    audio_stsd_t stsd;
    stts_t stts;
    stsc_t stsc;
    } audio_stbl_t;

typedef struct
    {
    u8 size[4];
    u8 name[4];
    u8 version[4];
    u8 balance[2];
    u8 reserved[2];
    } smhd_t;

typedef struct   //0x08(MINF) + 0x14(VMHD) + 0x24(DINF) + Size(STBL)
    {
    u8 size[4];
    u8 name[4];
    smhd_t smhd;
    dinf_t dinf;
    audio_stbl_t stbl;
    } audio_minf_t;

typedef struct
    {
    u8 size[4];
    u8 name[4];
    u8 version[4];
    u8 predefined[4];
    u8 handler_type[4];
    u8 reserved[12];
    u8 handler_name[17];
    } audio_hdlr_t;

typedef struct   //0x08(MDIA) + 0x20(MDHD) + 0x30(HDLR) + Size(MINF)
    {
    u8 size[4];
    u8 name[4];
    mdhd_t mdhd;
    audio_hdlr_t hdlr;
    audio_minf_t minf;
    } audio_mdia_t;

typedef struct	//0x08(TRAK) + 0x5C(TKHD) + 0x24(EDTS) + Size(MDIA)
    {
    u8 size[4];
    u8 name[4];
    tkhd_t tkhd;
    audio_mdia_t mdia;
    } ln_mmp4m_aud_trak_t;

u8 u8Mp4AudBox[] =
    {
    /* TRAK:0x000 */
    0xBF, 0xFF, 0xFF, 0xFF, 't', 'r', 'a',
    'k',	//0x08(TRAK) + 0x5C(TKHD) + Size(MDIA)

    /* TKHD:0x008 */
//box size	4	box大小
    0x00, 0x00, 0x00, 0x5C,
//box type	4	box類型
    't', 'k', 'h', 'd',
//version	1	box版本，0或1，一般為0。（以下位元組數均按version=0）
    0x00,
    /*flags		3	按位或操作結果值，預定義如下：
     0x000001 track_enabled，否則該track不被播放；
     0x000002 track_in_movie，表示該track在播放中被引用；
     0x000004 track_in_preview，表示該track在預覽時被引用。
     一般該值為7，如果一個媒體所有track均未設置track_in_movie和track_in_preview，將被理解為所有track均設置了這兩項；對於hint track，該值為0
     */
    0x00, 0x00, 0x03,
//creation time		4	創建時間（相對於UTC時間1904-01-01零點的秒數）
    0x00, 0x00, 0x00, 0x00,
//modification time	4	修改時間
    0x00, 0x00, 0x00, 0x00,
//track id	4	id號，不能重複且不能為0
    0x00, 0x00, 0x00, 0x02,
//reserved	4	保留位
    0x00, 0x00, 0x00, 0x00,
//duration	4	track的時間長度
    0x00, 0x00, 0x03,
    0xE8,			//M:
//reserved	8	保留位
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
//layer	2	視頻層，預設為0，值小的在上層
    0x00, 0x00,
//alternate group	2	track分組資訊，預設為0表示該track未與其他track有群組關係
    0x00, 0x01,
//volume	2	[8.8] 格式，如果為音訊track，1.0（0x0100）表示最大音量；否則為0
    0x01, 0x00,
//reserved	2	保留位
    0x00, 0x00,
//matrix	36	視頻變換矩陣
    0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x00,
//width		4	寬
    0x00, 0x00, 0x00, 0x00,
//height	4	高，均為 [16.16] 格式值，與sample描述中的實際畫面大小比值，用於播放時的展示寬高
    0x00, 0x00, 0x00, 0x00,

    /* MDIA:0x064 */
    0xBF, 0xFF, 0xFF, 0xFF, 'm', 'd', 'i', 'a',	//0x08(MDIA) + 0x20(MDHD) + 0x31(HDLR) + Size(MINF)

    /* MDHD:0x06C */
//box size	4	box大小
    0x00, 0x00, 0x00, 0x20,
//box type	4	box類型
    'm', 'd', 'h', 'd',
//version	1	box版本，0或1，一般為0。（以下位元組數均按version=0）
    0x00,
//flags	3
    0x00, 0x00, 0x00,
//creation time	4	創建時間（相對於UTC時間1904-01-01零點的秒數）
    0x00, 0x00, 0x00, 0x00,
//modification time	4	修改時間
    0x00, 0x00, 0x00, 0x00,
//time scale	4	同前表
    0x00, 0x00, 0x1F, 0x40,		//8000
//duration	4	track的時間長度
    0x00, 0x00, 0x1F, 0x40,		//M:
//language	2	媒體語言碼。最高位為0，後面15位元為3個字元（見ISO 639-2/T標準中定義）
    0x55, 0xC4,
//pre-defined	2
    0x00, 0x00,

    /* HDLR:0x08C */
//box size	4	box大小
    0x00, 0x00, 0x00, 0x31,
//box type	4	box類型
    'h', 'd', 'l', 'r',
//version	1	box版本，0或1，一般為0。（以下位元組數均按version=0）
    0x00,
//flags	3
    0x00, 0x00, 0x00,
//pre-defined	4
    0x00, 0x00, 0x00, 0x00,
//handler type	4	在media box中，該值為4個字元：
//“vide”— video track
//“soun”— audio track
//“hint”— hint track
    's', 'o', 'u', 'n',
//reserved	12
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
//name	不定	track type name，以‘\0’結尾的字串
    'l', 'l', 'm', 'i', 'n', 'i', 'm', 'u', 'm', 'm', 'p', '4', 'm', 'u', 'x', ' ', 0x00,

    /* MINF:0x0BD */
    0xBF, 0xFF, 0xFF, 0xFF, 'm', 'i', 'n', 'f',	//0x08(MINF) + 0x10(SMHD) + 0x24(DINF) + Size(STBL)

    /* SMHD:0x0C5 */
//box size	4	box大小
    0x00, 0x00, 0x00, 0x10,
//box type	4	box類型
    's', 'm', 'h', 'd',
//version	1	box版本，0或1，一般為0。（以下位元組數均按version=0）
    0x00,
//flags	3
    0x00, 0x00, 0x00,
//balance
    0x00, 0x00,
//Reserved
    0x00, 0x00,

    /* DINF:0x0D5 */
//box size	4	box大小
    0x00, 0x00, 0x00, 0x24,
//box type	4	box類型
    'd', 'i', 'n', 'f',
    //box size	4	box大小
    0x00, 0x00, 0x00, 0x1C,
    //box type	4	box類型
    'd', 'r', 'e', 'f',
    //version	1	box版本，0或1，一般為0。（以下位元組數均按version=0）
    0x00,
    //flags	3
    0x00, 0x00, 0x00,
    //entry count	4	“url”或“urn”表的元素個數
    0x00, 0x00, 0x00, 0x01,
    //“url”或“urn”列表	不定,
    //box size	4	box大小
    0x00, 0x00, 0x00, 0x0C,
    //box type	4	box類型
    'u', 'r', 'l', ' ',
    //box flag，值1，表明“url”中的字符串為空，表示track數據已包含在文件中
    0x00, 0x00, 0x00, 0x01,

    /* STBL:0x0F9 */
    0xBF, 0xFF, 0xFF, 0xFF, 's', 't', 'b', 'l',	//0x08(STBL) + 0x67(STSD) + 0x18(STTS) + Size(STBL)

    /* STSD:0x101*/
//box size	4	box大小
    0x00, 0x00, 0x00, 0x67,
//box type	4	box類型
    's', 't', 's', 'd',
//version	1	box版本，0或1，一般為0。（以下位元組數均按version=0）
    0x00,
//flags	3
    0x00, 0x00, 0x00,
    //entry count	1
    0x00, 0x00, 0x00, 0x01,

    //box size	4	box大小
    0x00, 0x00, 0x00, 0x57,
    //box type	4	box類型
    'm', 'p', '4', 'a',
    //version & flags
    0x00, 0x00, 0x00, 0x00,
    //Reserved
    0x00, 0x00,
    //Data-reference index
    0x00, 0x01,
    //Codec stream version
    0x00, 0x00,
    //Codec stream revision (=0)
    0x00, 0x00,
    //Reserved
    0x00, 0x00, 0x00, 0x00,
    //channel count
    0x00, 0x02,			//M:
    //sample size
    0x00, 0x10,
    //Reserved
    0x00, 0x00,
    //Reserved
    0x00, 0x00,
    //sample rate
    0x1F, 0x40,			//M:
    //Reserved
    0x00, 0x00,
    //esds
    //box size	4	box大小
    0x00, 0x00, 0x00, 0x33,
    //box type	4	box類型
    'e', 's', 'd', 's',
    //version & flags
    0x00, 0x00, 0x00, 0x00,
    //ES descriptor
    0x03, 0x80, 0x80, 0x80, 0x22,
    //track_id
    0x00, 0x01,
    //Reserved
    0x00,
    //DecoderConfig descriptor
    0x04, 0x80, 0x80, 0x80, 0x14,
    //Object type indication
    0x40,//64:MPEG-4 audio
    //codec_type
    0x15,//0x5:audio ,0x10:reserved set to 1
    //Buffersize
    0x00, 0x00, 0x00,
    //max_bitrate
    0x00, 0x00, 0x00, 0x00,		//M:
    //avg_bitrate
    0x00, 0x00, 0x00, 0x00,		//M:
    //DecoderSpecific info descriptor
    0x05, 0x80, 0x80, 0x80, 0x02, 0x15, 0x88,
    //SL descriptor
    0x06, 0x80, 0x80, 0x80, 0x01, 0x02,

    /* STTS:0x168 */
//box size	4	box大小
    0x00, 0x00, 0x00, 0x18,
//box type	4	box類型
    's', 't', 't', 's',
//version & flags
    0x00, 0x00, 0x00, 0x00,
//entry count
    0x00, 0x00, 0x00, 0x01,
//counts
    0x00, 0x00, 0x01, 0x2B,    //M:AAC Encode count
//duration
    0x00, 0x00, 0x04, 0x00,	//1024 for sample

    /* STSC:0x180 */
//固定1sample 1chunk就不用update，如Novatek.
//box size	4	box大小	//size = entry count * 12 + 0x10 = 0x70
    0x00, 0x00, 0x00, 0x1C,
//box type	4	box類型
    's', 't', 's', 'c',
//version & flags
    0x00, 0x00, 0x00, 0x00,
//entry count
    0x00, 0x00, 0x00, 0x01,
// first chunk、samples per chunk、sample description index(0x00000001)
    0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01,

    /* STSZ:0x19C */
    //STSZ
    //STCO
    };

typedef struct
    {
    u8 size[4];
    u8 name[4];
    u8 version[4];
    u8 iframe_count[4];
    u32 iframe_pos[1]; //need to big2little
    } stss_t;

//STSS:Sync Sample Box,I frame position
u8 u8VideoSTSSBox[] =
    {
//box size	4	box大小
    0x00, 0x00, 0x00, 0x38,
//box type	4	box類型
    's', 't', 's', 's',
//version & flags
    0x00, 0x00, 0x00, 0x00,
//I frame entry count
    0x00, 0x00, 0x00, 0x0A,
//I frame position
//	.............
//	0x00,0x00,0x00,0x01,//every 30 frames
//	0x00,0x00,0x00,0x1F,...
    };

typedef struct
    {
    u8 size[4];
    u8 name[4];
    u8 version[4];
    u8 reserved[4];
    u8 item_count[4];
    u32 item_size[1]; //need to big2little
    } stsz_t;

//STSZ:Sample Size Box
u8 u8STSZBox[] =
    {
//box size	4	box大小	//size = sample count * 4 + 0x14 = 0x12B*4+0x14 = 0x4C0
    0x00, 0x00, 0x04, 0xC0,
//box type	4	box類型
    's', 't', 's', 'z',
//
    0x00, 0x00, 0x00, 0x00,
//
    0x00, 0x00, 0x00, 0x00,
//sample count
    0x00, 0x00, 0x01, 0x2B,
//sample sizes, every 4 bytes
//...........................
    };

typedef struct
    {
    u8 size[4];
    u8 name[4];
    u8 version[4];
    u8 chunk_count[4];
    u32 chunk_offset[1]; //need to big2little
    } stco_t;

//STCO:Chunk Offset Box
u8 u8STCOBox[] =
    {
//box size	4	box大小	//size = chunk count * 4 + 0x10 = 0x0C*4+0x10 = 0x40
    0x00, 0x00, 0x00, 0x40,
//box type	4	box類型
    's', 't', 'c', 'o',
//
    0x00, 0x00, 0x00, 0x00,
//chunk count
    0x00, 0x00, 0x00, 0x0C,
//chunk offsets, every 4 bytes
//...........................
    };

static void *gMP4_MOOV_BOX = NULL;

/*************************************************************************
 *                      STATITC FUNCTION
 *************************************************************************/
static inline void
ln_hex_dump(char* key, void* value, int value_size)
    {
    char    str[1000] = {0};
    int i;
    int length = 0;
    u8 *data = (u8*) value;

    if (sizeof(str) / 3 > value_size)
        length = value_size;
    else
        length = sizeof(str) / 3 - 1;

    for (i = 0; i < length; i++)
        sprintf(str + strlen(str), "%02X ", data[i]);

    if (key)
        ln_log_info("%s (%d) = %s", key, value_size, str);
    else
        ln_log_info("(%d) : %s", value_size, str);
    }

static void blong2str(void* buff, int32_t n)
    {
    unsigned char *dst = (unsigned char *)buff;
    dst[0] = (n >> 24) & 0xff;
    dst[1] = (n >> 16) & 0xff;
    dst[2] = (n >> 8) & 0xff;
    dst[3] = (n) & 0xff;
    }

static void bshort2str(void* buff, int32_t n)
    {
    unsigned char *dst = (unsigned char *)buff;
    dst[0] = (n >> 8) & 0xff;
    dst[1] = (n) & 0xff;
    }

static void check_iframe_spspps(mp4_t *MP4, u8 *ptr)
    {
    u32 j = 0;
    do
        {
        if ((ptr[0] == 0) && (ptr[1] == 0) && (ptr[2] == 0))
            {
            j = 0;
            if (ptr[3] == 0x01) //H264 frame 00 00 00 01
                {
                do
                    {
                    if ((ptr[j + 4] == 0) && (ptr[j + 5] == 0) && (ptr[j + 6] == 0))
                        break;
                    else
                        {
                        if (!MP4->sps_size)
                            MP4->sps_info[j] = ptr[j + 4];
                        else
                            MP4->pps_info[j] = ptr[j + 4];
                        j++;
                        }
                    }
                while (1);
                }
            else if (ptr[3] < 0x20) //1080P:0x0B,480P:0x09
                {
                //MP4 frame 00 00 00 len,just copy
                do
                    {
                    if (!MP4->sps_size)
                        MP4->sps_info[j] = ptr[j + 4];
                    else
                        MP4->pps_info[j] = ptr[j + 4];
                    j++;
                    }
                while (j < ptr[3]);
                }
            else
                {
                ln_log_error("\n\n\nstream format fail");
                return;
                }
            if (!MP4->sps_size)
                MP4->sps_size = j;
            else
                {
                MP4->pps_size = j;
                break;
                }
            ptr += (j + 4);
            }
        else
            ptr++;
        }
    while (1);
    ln_hex_dump("SPS", MP4->sps_info,  MP4->sps_size);
    ln_hex_dump("PPS", MP4->pps_info,  MP4->pps_size);
    ptr += MP4->pps_size + 4;
    }

static void
mp4v_add_index_entry(mp4_t *MP4, u8 isIFrame, u32 len, u8 *bs_buf)
    {
    u8 *ptr;
    ptr = bs_buf;

    //update mdata size
    MP4->mdata_size += len;

    /* Add index entry */
    blong2str((u8*) &MP4->video_stco_table[MP4->frame_count], MP4->frame_start);
    blong2str((u8*) &MP4->video_stsz_table[MP4->frame_count], len);
    MP4->frame_start += len;

    if (ptr[4] == 0x67) // I frame
        {
        if (!MP4->sps_size)
            check_iframe_spspps(MP4, ptr);
        }

    /* Update counter */
    MP4->frame_count++;
    if (isIFrame)
        {
        blong2str(&(MP4->video_stss_table[MP4->i_count]), MP4->frame_count);
        MP4->i_count++;
        }
    }

//bitstream_data
static void
mp4a_add_index_entry(mp4_t *MP4, unsigned long len)
    {
    u32 sample_size;

    //update mdata size
    MP4->mdata_size += len;
    sample_size = len - 7;
    /* Add index entry */
    MP4->frame_start += (0x08 + 0x07);
    blong2str((u8*) &MP4->audio_stco_table[MP4->sample_count], MP4->frame_start);
    blong2str((u8*) &MP4->audio_stsz_table[MP4->sample_count], sample_size);

    MP4->frame_start += (len - 0x07);

    /* Update counter */
    MP4->sample_count++;
    }

static void
ln_mmp4m_free(mp4_t *mp4)
    {
    if (mp4->sps_info)
        ln_free(mp4->sps_info);
    if (mp4->pps_info)
        ln_free(mp4->pps_info);
    if (mp4->video_stss_table)
        ln_free(mp4->video_stss_table);
    if (mp4->video_stco_table)
        ln_free(mp4->video_stco_table);
    if (mp4->video_stsz_table)
        ln_free(mp4->video_stsz_table);
    if (mp4->audio_stco_table)
        ln_free(mp4->audio_stco_table);
    if (mp4->audio_stsz_table)
        ln_free(mp4->audio_stsz_table);
    ln_free(mp4);
    }

static mp4_t *ln_mmp4m_alloc(char *filename)
    {
    mp4_t *mp4 = NULL;
    int   iErrCnt = 0;

    mp4 = ln_zalloc(sizeof(mp4_t));
    if (!mp4)
        return mp4;

    mp4->fdes = LibMp4RecFileCreate(filename);
    if (mp4->fdes == NULL)
        {
        ln_free(mp4);
        return NULL;
        }

    mp4->video_stss_table = ln_zalloc(3000);    //300*2(GOP=15)*4+spare
    if (mp4->video_stss_table == NULL)
        iErrCnt++;

    mp4->video_stco_table = ln_zalloc(40000);    //300*30*4+spare
    if (mp4->video_stco_table == NULL)
        iErrCnt++;

    mp4->video_stsz_table = ln_zalloc(40000);    //300*30*4+spare
    if (mp4->video_stsz_table == NULL)
        iErrCnt++;

    mp4->sps_info = ln_zalloc(0x20);
    if (mp4->sps_info == NULL)
        iErrCnt++;

    mp4->pps_info = ln_zalloc(0x10);
    if (mp4->pps_info == NULL)
        iErrCnt++;

    if(iErrCnt)
        {
        ln_mmp4m_free(mp4);
        return NULL;
        }

    return mp4;
    }

static u32 table_size_audio(mp4_t *mp4)
    {
    u32 size;
    size = sizeof(stsz_t) + 4 * (mp4->sample_count - 1) 	//STSZ
           + sizeof(stco_t) + 4 * (mp4->sample_count - 1);	//STCO
    return size;
    }

static u32 stbl_size_audio(mp4_t *mp4)
    {
    u32 size;
    size = sizeof(audio_stbl_t) + table_size_audio(mp4);
    //ln_log_error("audio stbl_size = %x",size);
    return size;
    }

static u32 trak_size_audio(mp4_t *mp4)
    {
    return (sizeof(ln_mmp4m_aud_trak_t) + table_size_audio(mp4));
    }

static u32 mdia_size_audio(mp4_t *mp4)
    {
    return (sizeof(audio_mdia_t) + table_size_audio(mp4));
    }

static u32 minf_size_audio(mp4_t *mp4)
    {
    return (sizeof(audio_minf_t) + table_size_audio(mp4));
    }

static u32 table_size(mp4_t *mp4)
    {
    u32 size;
    size = sizeof(stss_t)
           + 4 * (mp4->i_count - 1)			//STSS
           + sizeof(stsz_t) + 4 * (mp4->frame_count - 1) 	//STSZ
           + sizeof(stco_t) + 4 * (mp4->frame_count - 1);	//STCO
    //ln_log_error("table_size = %x",size);
    return size;
    }

static u32 stbl_size(mp4_t *mp4)
    {
    u32 size;
    size = sizeof(video_stbl_t) + table_size(mp4);
    //ln_log_info("stbl_size = %x", size);
    return size;
    }

static u32 trak_size(mp4_t *mp4)
    {
    return (sizeof(ln_mmp4m_vid_trak_t) + table_size(mp4));
    }

static u32 moov_size(mp4_t *mp4)
    {
    return (8 + sizeof(mvhd_t) + trak_size(mp4) + (mp4->audio_enable ? trak_size_audio(mp4) : 0));
    }

static u32 mdia_size(mp4_t *mp4)
    {
    return (sizeof(video_mdia_t) + table_size(mp4));
    }

static u32 minf_size(mp4_t *mp4)
    {
    return (sizeof(video_minf_t) + table_size(mp4));
    }

static u32
update_video_table(ln_mmp4m_vid_trak_t *vtrak, mp4_t *mp4, int fps)
    {
    stts_t *stts = NULL;
    stss_t *stss = NULL;
    stsz_t *stsz = NULL;
    stco_t *stco = NULL;
    u32 size = 0;
    u32 offset = sizeof(ln_mmp4m_vid_trak_t) - sizeof(stts_t);

    //update stts
    stts = (stts_t *) ((u32) vtrak + offset);
    size = sizeof(stts_t);
    offset += size;
    blong2str(stts->size, size);
    blong2str(stts->entry_count, 1);

    stss = (stss_t *) ((u32) vtrak + offset);
    memcpy((u8 *) stss, u8VideoSTSSBox, sizeof(u8VideoSTSSBox));
    size = sizeof(stss_t) + (mp4->i_count - 1) * 4;
    offset += size;
    blong2str(stss->size, size);
    blong2str(stss->iframe_count, mp4->i_count);
    memcpy((u8*) stss->iframe_pos, mp4->video_stss_table, mp4->i_count * 4);

    stsz = (stsz_t *) ((u32) vtrak + offset);
    memcpy((u8 *) stsz, u8STSZBox, sizeof(u8STSZBox));
    size = sizeof(stsz_t) + (mp4->frame_count - 1) * 4;
    offset += size;
    blong2str(stsz->size, size);
    blong2str(stsz->item_count, mp4->frame_count);
    memcpy((u8 *) stsz->item_size, mp4->video_stsz_table, mp4->frame_count * 4);
    //STCO
    stco = (stco_t *) ((u32) vtrak + offset);
    memcpy((u8*) stco, u8STCOBox, sizeof(u8STCOBox));
    size = sizeof(stco_t) + (mp4->frame_count - 1) * 4;
    offset += size;
    blong2str(stco->size, size);
    blong2str(stco->chunk_count, mp4->frame_count);
    memcpy((u8*) stco->chunk_offset, mp4->video_stco_table, mp4->frame_count * 4);

    return offset;
    }

static long
update_avcc_table(avcC_t *avcC, mp4_t *mp4)
    {
    long avcc_size = 0;

    avcC->profile = mp4->sps_info[1];
    avcC->profile_compat = mp4->sps_info[2];
    avcC->level = mp4->sps_info[3];
    memset(avcC->spspps_info, 0, sizeof(avcC->spspps_info));

    /* Update SPS */
    bshort2str(avcC->spspps_info, mp4->sps_size);
    memcpy(&(avcC->spspps_info[2]), mp4->sps_info, mp4->sps_size);

    /* Update PPS */
    avcC->spspps_info[2 + mp4->sps_size] = 0x01; //only has 1 PPS

    bshort2str(&(avcC->spspps_info[2 + mp4->sps_size + 1]), mp4->pps_size);
    memcpy(&(avcC->spspps_info[2 + mp4->sps_size + 1 + 2]), mp4->pps_info, mp4->pps_size);
    avcc_size = sizeof(avcC_t) - (sizeof(avcC->spspps_info));
    avcc_size += (2 + mp4->sps_size) + 1 + (2 + mp4->pps_size);
    //blong2str(avcC->size, avcc_size);
    blong2str(avcC->size, sizeof(avcC_t));
    return avcc_size;
    }

static u32
update_video_trak(mp4_t *mp4, ln_mmp4m_vid_trak_t *vtrak)
    {
    u32  offset = 0;

    //1000 us mvhd timescale
    blong2str(vtrak->tkhd.duration, mp4->frame_count * 1000 / mp4->video.fps);
    bshort2str(vtrak->tkhd.width, mp4->video.width);
    bshort2str(vtrak->tkhd.height, mp4->video.height);
    blong2str(vtrak->edts.elst_delay, mp4->frame_count * 1000 / mp4->video.fps);

    //12000 is mdhd time_scale
    blong2str(vtrak->mdia.mdhd.duration, mp4->frame_count * 12000 / mp4->video.fps);
    bshort2str(vtrak->mdia.minf.stbl.stsd.avc1.width, mp4->video.width);
    bshort2str(vtrak->mdia.minf.stbl.stsd.avc1.height, mp4->video.height);

    //TRAK
    blong2str(vtrak->size, trak_size(mp4));
    //MDIA
    blong2str(vtrak->mdia.size, mdia_size(mp4));
    //MINF
    blong2str(vtrak->mdia.minf.size, minf_size(mp4));
    //STBL
    blong2str(vtrak->mdia.minf.stbl.size, stbl_size(mp4));
    //STTS
    blong2str(vtrak->mdia.minf.stbl.stts.counts, mp4->frame_count);
    //STSS,STSZ,STCO
    offset = update_video_table(vtrak, mp4, mp4->video.fps);
    return offset;
    }

static u32
update_audio_table(ln_mmp4m_aud_trak_t *atrak, mp4_t *mp4)
    {
    stsz_t *stsz;
    stco_t *stco;
    u32 size, offset = sizeof(ln_mmp4m_aud_trak_t);

    stsz = (stsz_t *) ((u32) atrak + offset);
    memcpy((u8 *) stsz, u8STSZBox, sizeof(u8STSZBox));
    size = sizeof(stsz_t) + (mp4->sample_count - 1) * 4;
    offset += size;
    blong2str(stsz->size, size);
    blong2str(stsz->item_count, mp4->sample_count);
    memcpy((u8 *) stsz->item_size, mp4->audio_stsz_table, mp4->sample_count * 4);
    //STCO
    stco = (stco_t *) ((u32) atrak + offset);
    memcpy((u8*) stco, u8STCOBox, sizeof(u8STCOBox));
    size = sizeof(stco_t) + (mp4->sample_count - 1) * 4;
    offset += size;
    blong2str(stco->size, size);
    blong2str(stco->chunk_count, mp4->sample_count);
    memcpy((u8*) stco->chunk_offset, mp4->audio_stco_table, mp4->sample_count * 4);
    return offset;
    }

u16 esds_dec_config_descr[2] =
    {
    0x1588,    //8K 1ch
    0x1408,
    //16K 1ch
    };

static u32
update_audio_trak(mp4_t *mp4, ln_mmp4m_aud_trak_t *atrak)
    {
    u32 offset;

    memcpy((u8*) atrak, u8Mp4AudBox, sizeof(u8Mp4AudBox));

    //1000 us mvhd timescale,128ms 1 sample
    blong2str(atrak->tkhd.duration, mp4->sample_count * 2048 / (mp4->audio.rate * 2 / 1000));    //8K samplerate
    //8000 is mdhd time_scale
    blong2str(atrak->mdia.mdhd.duration, mp4->sample_count * 2048 / (mp4->audio.rate * 2 / 8000));
    //TRAK
    blong2str(atrak->size, trak_size_audio(mp4));
    //MDIA
    blong2str(atrak->mdia.size, mdia_size_audio(mp4));
    //MINF
    blong2str(atrak->mdia.minf.size, minf_size_audio(mp4));
    //STBL
    blong2str(atrak->mdia.minf.stbl.size, stbl_size_audio(mp4));
    //update STBL Info
    //STTS
    blong2str(atrak->mdia.minf.stbl.stts.counts, mp4->sample_count);
    blong2str(atrak->mdia.minf.stbl.stts.duration, 2048 / (mp4->audio.rate * 2 / 8000));

    //update Audio track
    bshort2str(atrak->mdia.minf.stbl.stsd.mp4a.channel_count, mp4->audio.channels);
    bshort2str(atrak->mdia.minf.stbl.stsd.mp4a.sample_size, mp4->audio.bits);
    bshort2str(atrak->mdia.minf.stbl.stsd.mp4a.sample_rate, mp4->audio.rate);
    blong2str(atrak->mdia.minf.stbl.stsd.mp4a.esds.max_bitrate, mp4->audio.avg_bitrate);
    blong2str(atrak->mdia.minf.stbl.stsd.mp4a.esds.avg_bitrate, mp4->audio.avg_bitrate);
    if (mp4->audio.rate == 8000)
        bshort2str(&atrak->mdia.minf.stbl.stsd.mp4a.esds.decoderspecific_info_descriptor[5], esds_dec_config_descr[0]);
    else if (mp4->audio.rate == 16000)
        bshort2str(&atrak->mdia.minf.stbl.stsd.mp4a.esds.decoderspecific_info_descriptor[5], esds_dec_config_descr[1]);
    else
        ln_log_error("Not supported samplerate");
    //STSZ,STCO
    offset = update_audio_table(atrak, mp4);

    return offset;
    }

static u32
prepare_header(mp4_t *mp4, u8 *MP4_header)
    {
    u32 offset = 0;
    ln_mmp4m_moov_t *mp4header = NULL;

    mp4header = (ln_mmp4m_moov_t *) (MP4_header);

    /* Need to compute AVCC length here and move to correct offset */
    update_avcc_table(&(mp4header->vtrak.mdia.minf.stbl.stsd.avc1.avcC), mp4);

    /* Update all box size size */
    blong2str(&(mp4header->vtrak.mdia.minf.stbl.stsd.avc1.size), sizeof(avc1_t));
    blong2str(&(mp4header->vtrak.mdia.minf.stbl.stsd.size), sizeof(video_stsd_t));
    blong2str(&(mp4header->vtrak.mdia.minf.stbl.stsc.size), sizeof(stsc_t));
    blong2str(&(mp4header->vtrak.mdia.minf.stbl.stts.size), sizeof(stts_t));
    blong2str(&(mp4header->vtrak.mdia.minf.stbl.size), sizeof(video_stbl_t));
    blong2str(&(mp4header->vtrak.mdia.minf.size), sizeof(video_minf_t));
    blong2str(&(mp4header->vtrak.mdia.size), sizeof(video_mdia_t));
    blong2str(&(mp4header->vtrak.size), sizeof(ln_mmp4m_vid_trak_t));

    //update parameter
    blong2str(mp4header->mvhd.duration, mp4->frame_count * 1000 / mp4->video.fps);
    if (mp4->audio_enable)
        blong2str(mp4header->mvhd.next_id, 0x03);

    offset = update_video_trak(mp4, &mp4header->vtrak);
    // audio TRAK
    if (mp4->audio_enable)
        offset += update_audio_trak(mp4, (ln_mmp4m_aud_trak_t *) ((u32) &mp4header->vtrak + offset));

    //update size
    //MOOV
    blong2str(mp4header->moov_sz, moov_size(mp4));
    blong2str(u8MdatBox, mp4->mdata_size + 8);
    if(mp4->audio_enable)
        return offset + sizeof(ln_mmp4m_vid_trak_t) + sizeof(ln_mmp4m_aud_trak_t);
    else
        return offset + sizeof(ln_mmp4m_vid_trak_t);
    }

static int
ln_mmp4m_write_moov(mp4_t *mp4)
    {
    u32 u32Size = 0;
    int iRetCode = 0;
    unsigned char *MP4_header = gMP4_MOOV_BOX;

    u32Size = prepare_header(mp4, MP4_header);

    /* Write MOOV box */
    ln_log_info("Updating MOOV box with size %u", u32Size);
    iRetCode = LibMp4RecFileWrite(mp4->fdes, (char *) MP4_header, u32Size);
    if(iRetCode != u32Size)
        ln_log_error("Write MOOV box size %u failed with error %d", u32Size, iRetCode);

    /* Update mdat box */
    if (LibMp4RecFileSeek(mp4->fdes, 0, SEEK_SET) < 0 )
        return -1;

    iRetCode = LibMp4RecFileWrite(mp4->fdes, u8FtypeBox, sizeof(u8FtypeBox));
    if (iRetCode != sizeof(u8FtypeBox))
        {
        ln_log_error("Write FTYP box size %u failed with error %d", sizeof(u8FtypeBox), iRetCode);
        return -2;
        }

    iRetCode = LibMp4RecFileWrite(mp4->fdes, u8FreeBox, sizeof(u8FreeBox));
    if (iRetCode != sizeof(u8FreeBox))
        {
        ln_log_error("Write FREE box size %u failed with error %d", sizeof(u8FreeBox), iRetCode);
        return -2;
        }

    iRetCode = LibMp4RecFileWrite(mp4->fdes, u8MdatBox, sizeof(u8MdatBox));
    if (iRetCode != sizeof(u8MdatBox))
        {
        ln_log_error("Write FTYP box size %u failed with error %d", sizeof(u8MdatBox), iRetCode);
        return -2;
        }
    return 0;
    }

/***************************************************************
 *                        PUCLIC FUNCTION
 ***************************************************************/
int
ln_mmp4m_write_frame(void *handler, void *data, eMp4FrameType type, int size)
    {
    int   iRetCode = 0;
    u8    *frame_data = (u8*)data;
    u8    isIFrame = 0x00;
    mp4_t *mp4 = NULL;

    if (handler == NULL)
        return -1;

    if (!data)
        return -2;

    mp4 = (mp4_t*)handler;
    switch (type)
        {
        case eMp4FrameVideo:
            if(frame_data[4] == 0x67 && frame_data[5] == 0x64)
                isIFrame = 0x10;
            mp4v_add_index_entry(mp4, isIFrame, size, data);
            break;
        case eMp4FrameAudio:
            mp4a_add_index_entry(mp4, size);
            break;
        }

    iRetCode = LibMp4RecFileWrite(mp4->fdes, (char *) data, size);
    if (iRetCode != size)
        {
        ln_log_error("LibMp4RecFileWrite return %d", iRetCode);
        return -3;
        }
    return 0;
    }


int
ln_mmp4m_close(void *handler)
    {
    int   ret = 0;
    mp4_t *mp4 = (mp4_t*)handler;

    ret = ln_mmp4m_write_moov(mp4);
    LibMp4RecFileClose(mp4->fdes);
    ln_mmp4m_free(mp4);
    return ret;
    }

void*
ln_mmp4m_create(char *filename, tMp4VidInfo *video, tMp4AudInfo *audio)
    {
    int iRetCode = 0;
    mp4_t *mp4 = NULL;
    ln_mmp4m_moov_t *mp4header = NULL;

    mp4 = ln_mmp4m_alloc(filename);
    if (!mp4)
        return NULL;

    /* Write out HEADERBYTES bytes, the header will be updated
       when we are finished with writing */

    iRetCode = LibMp4RecFileWrite(mp4->fdes, u8FtypeBox, sizeof(u8FtypeBox));
    if (iRetCode != sizeof(u8FtypeBox))
        {
        LibMp4RecFileClose(mp4->fdes);
        ln_free(mp4);
        return NULL;
        }

    iRetCode = LibMp4RecFileWrite(mp4->fdes, u8FreeBox, sizeof(u8FreeBox));
    if (iRetCode != sizeof(u8FreeBox))
        {
        LibMp4RecFileClose(mp4->fdes);
        ln_free(mp4);
        return NULL;
        }

    iRetCode = LibMp4RecFileWrite(mp4->fdes, u8MdatBox, sizeof(u8MdatBox));
    if (iRetCode != sizeof(u8MdatBox))
        {
        LibMp4RecFileClose(mp4->fdes);
        ln_free(mp4);
        return NULL;
        }

    memcpy(gMP4_MOOV_BOX, u8Mp4VidBox, sizeof(u8Mp4VidBox));
    mp4header = (ln_mmp4m_moov_t *) (gMP4_MOOV_BOX);

    //update video track
    memcpy(&(mp4->video), video, sizeof(tMp4VidInfo));
    bshort2str(mp4header->vtrak.tkhd.width, video->width);
    bshort2str(mp4header->vtrak.tkhd.height, video->height);
    bshort2str(mp4header->vtrak.mdia.minf.stbl.stsd.avc1.width, video->width);
    bshort2str(mp4header->vtrak.mdia.minf.stbl.stsd.avc1.height, video->height);
    blong2str(mp4header->vtrak.mdia.minf.stbl.stts.duration, 12000 / video->fps);

    //update Audio track
    if (audio)
        {
        mp4->audio_enable = 1;
        mp4->audio_stco_table = ln_zalloc(20000);    //300*16*4+spare
        mp4->audio_stsz_table = ln_zalloc(20000);    //300*16*4+spare
        memcpy(&(mp4->audio), audio, sizeof(tMp4AudInfo));
        if (mp4->audio_enable == 1)
            {
            ln_mmp4m_aud_trak_t *atrak;
            blong2str(mp4header->mvhd.next_id, 0x03);
            memcpy((u8*) ((u32) mp4header + sizeof(u8Mp4VidBox)), u8Mp4AudBox, sizeof(u8Mp4AudBox));
            atrak = (ln_mmp4m_aud_trak_t *) ((u32) mp4header + sizeof(u8Mp4VidBox));
            bshort2str(atrak->mdia.minf.stbl.stsd.mp4a.channel_count, audio->channels);
            bshort2str(atrak->mdia.minf.stbl.stsd.mp4a.sample_size, audio->bits);
            bshort2str(atrak->mdia.minf.stbl.stsd.mp4a.sample_rate, audio->rate);
            blong2str(atrak->mdia.minf.stbl.stsd.mp4a.esds.max_bitrate, audio->avg_bitrate);
            blong2str(atrak->mdia.minf.stbl.stsd.mp4a.esds.avg_bitrate, audio->avg_bitrate);
            blong2str(atrak->mdia.minf.stbl.stts.duration, 2048 / (audio->rate * 2 / 8000));
            }
        }
    else
        mp4->audio_enable = 0;
    mp4->frame_start = sizeof(u8FtypeBox) + sizeof(u8FreeBox) + sizeof(u8MdatBox);
    return (void*)mp4;
    }

void ln_mmp4m_mem_init(void *tmpBuf)
    {
    gMP4_MOOV_BOX = tmpBuf;
    }
