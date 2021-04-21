/*
*
* Copyright (C) 2016 OtherCrashOverride@users.noreply.github.com.
* All rights reserved.
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License version 2, as
* published by the Free Software Foundation.
*
* This program is distributed in the hope that it will be useful, but WITHOUT
* ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
* FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
* more details.
*
*/

#pragma once

//extern "C"
//{
//	// aml_lib
//#include <codec.h>
//}

#include "Mutex.h"
#include "Pin.h"
#include "Rectangle.h"

#include "amports/amstream.h"

#if 0
// codec_type.h
typedef struct {
	unsigned int    format;  ///< video format, such as H264, MPEG2...
	unsigned int    width;   ///< video source width
	unsigned int    height;  ///< video source height
	unsigned int    rate;    ///< video source frame duration
	unsigned int    extra;   ///< extra data information of video stream
	unsigned int    status;  ///< status of video stream
	unsigned int    ratio;   ///< aspect ratio of video source
	void *          param;   ///< other parameters for video decoder
	unsigned long long    ratio64;   ///< aspect ratio of video source
} dec_sysinfo_t;


// vformat.h
typedef enum {
	VIDEO_DEC_FORMAT_UNKNOW,
	VIDEO_DEC_FORMAT_MPEG4_3,
	VIDEO_DEC_FORMAT_MPEG4_4,
	VIDEO_DEC_FORMAT_MPEG4_5,
	VIDEO_DEC_FORMAT_H264,
	VIDEO_DEC_FORMAT_MJPEG,
	VIDEO_DEC_FORMAT_MP4,
	VIDEO_DEC_FORMAT_H263,
	VIDEO_DEC_FORMAT_REAL_8,
	VIDEO_DEC_FORMAT_REAL_9,
	VIDEO_DEC_FORMAT_WMV3,
	VIDEO_DEC_FORMAT_WVC1,
	VIDEO_DEC_FORMAT_SW,
	VIDEO_DEC_FORMAT_AVS,
	VIDEO_DEC_FORMAT_H264_4K2K,
	VIDEO_DEC_FORMAT_HEVC,
	VIDEO_DEC_FORMAT_VP9,
	VIDEO_DEC_FORMAT_MAX
} vdec_type_t;

typedef enum {
	VFORMAT_UNKNOWN = -1,
	VFORMAT_MPEG12 = 0,
	VFORMAT_MPEG4,
	VFORMAT_H264,
	VFORMAT_MJPEG,
	VFORMAT_REAL,
	VFORMAT_JPEG,
	VFORMAT_VC1,
	VFORMAT_AVS,
	VFORMAT_SW,
	VFORMAT_H264MVC,
	VFORMAT_H264_4K2K,
	VFORMAT_HEVC,
	VFORMAT_H264_ENC,
	VFORMAT_JPEG_ENC,
	VFORMAT_VP9,

	/*add new here before.*/
	VFORMAT_MAX,
	VFORMAT_UNSUPPORT = VFORMAT_MAX
} vformat_t;


// aformat.h
typedef enum {
	AFORMAT_UNKNOWN = -1,
	AFORMAT_MPEG = 0,
	AFORMAT_PCM_S16LE = 1,
	AFORMAT_AAC = 2,
	AFORMAT_AC3 = 3,
	AFORMAT_ALAW = 4,
	AFORMAT_MULAW = 5,
	AFORMAT_DTS = 6,
	AFORMAT_PCM_S16BE = 7,
	AFORMAT_FLAC = 8,
	AFORMAT_COOK = 9,
	AFORMAT_PCM_U8 = 10,
	AFORMAT_ADPCM = 11,
	AFORMAT_AMR = 12,
	AFORMAT_RAAC = 13,
	AFORMAT_WMA = 14,
	AFORMAT_WMAPRO = 15,
	AFORMAT_PCM_BLURAY = 16,
	AFORMAT_ALAC = 17,
	AFORMAT_VORBIS = 18,
	AFORMAT_AAC_LATM = 19,
	AFORMAT_APE = 20,
	AFORMAT_EAC3 = 21,
	AFORMAT_PCM_WIFIDISPLAY = 22,
	AFORMAT_DRA = 23,
	AFORMAT_SIPR = 24,
	AFORMAT_TRUEHD = 25,
	AFORMAT_MPEG1 = 26, //AFORMAT_MPEG-->mp3,AFORMAT_MPEG1-->mp1,AFROMAT_MPEG2-->mp2
	AFORMAT_MPEG2 = 27,
	AFORMAT_WMAVOI = 28,
	AFORMAT_UNSUPPORT,
	AFORMAT_MAX

} aformat_t;


// amstream.h
struct buf_status {
	int size;
	int data_len;
	int free_len;
	unsigned int read_pointer;
	unsigned int write_pointer;
};

struct am_ioctl_parm {
	union {
		unsigned int data_32;
		unsigned long long data_64;
		vformat_t data_vformat;
		aformat_t data_aformat;
		char data[8];
	};
	unsigned int cmd;
	char reserved[4];
};

struct vdec_status {
	unsigned int width;
	unsigned int height;
	unsigned int fps;
	unsigned int error_count;
	unsigned int status;
};

struct adec_status {
	unsigned int channels;
	unsigned int sample_rate;
	unsigned int resolution;
	unsigned int error_count;
	unsigned int status;
};

struct userdata_poc_info_t {

	unsigned int poc_info;

	unsigned int poc_number;
};

struct am_ioctl_parm_ex {
	union {
		struct buf_status status;
		struct vdec_status vstatus;
		struct adec_status astatus;

		struct userdata_poc_info_t data_userdata_info;
		char data[24];

	};
	unsigned int cmd;
	char reserved[4];
};

#define AMSTREAM_IOC_MAGIC 'S'
#define AMSTREAM_IOC_SYSINFO _IOW((AMSTREAM_IOC_MAGIC), 0x0a, int)
#define AMSTREAM_IOC_VPAUSE _IOW((AMSTREAM_IOC_MAGIC), 0x17, int)
#define AMSTREAM_IOC_SYNCTHRESH _IOW((AMSTREAM_IOC_MAGIC), 0x19, int)
#define AMSTREAM_IOC_CLEAR_VIDEO _IOW((AMSTREAM_IOC_MAGIC), 0x1f, int)
#define AMSTREAM_IOC_SYNCENABLE _IOW((AMSTREAM_IOC_MAGIC), 0x43, int)
#define AMSTREAM_IOC_SET_VIDEO_DISABLE _IOW((AMSTREAM_IOC_MAGIC), 0x49, int)
#define AMSTREAM_IOC_GET_VIDEO_AXIS   _IOR((AMSTREAM_IOC_MAGIC), 0x4b, int)
#define AMSTREAM_IOC_SET_VIDEO_AXIS   _IOW((AMSTREAM_IOC_MAGIC), 0x4c, int)
#define AMSTREAM_IOC_SET_SCREEN_MODE _IOW((AMSTREAM_IOC_MAGIC), 0x59, int)
#define AMSTREAM_IOC_GET _IOWR((AMSTREAM_IOC_MAGIC), 0xc1, struct am_ioctl_parm)
#define AMSTREAM_IOC_SET _IOW((AMSTREAM_IOC_MAGIC), 0xc2, struct am_ioctl_parm)
#define AMSTREAM_IOC_GET_EX _IOWR((AMSTREAM_IOC_MAGIC), 0xc3, struct am_ioctl_parm_ex)

#define AMSTREAM_SET_VFORMAT 0x105
#define AMSTREAM_SET_TSTAMP 0x10E
#define AMSTREAM_PORT_INIT 0x111
#define AMSTREAM_SET_PCRSCR 0x118
#define AMSTREAM_GET_VPTS 0x805

#define AMSTREAM_GET_EX_VB_STATUS 0x900
#define AMSTREAM_GET_EX_VDECSTAT 0x902

#define AMSTREAM_IOC_GET_VERSION _IOR((AMSTREAM_IOC_MAGIC), 0xc0, int)


// AMSTREAM_IOC_SET_VIDEO_DISABLE
#define VIDEO_DISABLE_NONE    0
#define VIDEO_DISABLE_NORMAL  1
#define VIDEO_DISABLE_FORNEXT 2


// AMSTREAM_IOC_SET_SCREEN_MODE
enum {
	VIDEO_WIDEOPTION_NORMAL = 0,
	VIDEO_WIDEOPTION_FULL_STRETCH = 1,
	VIDEO_WIDEOPTION_4_3 = 2,
	VIDEO_WIDEOPTION_16_9 = 3,
	VIDEO_WIDEOPTION_NONLINEAR = 4,
	VIDEO_WIDEOPTION_NORMAL_NOSCALEUP = 5,
	VIDEO_WIDEOPTION_4_3_IGNORE = 6,
	VIDEO_WIDEOPTION_4_3_LETTER_BOX = 7,
	VIDEO_WIDEOPTION_4_3_PAN_SCAN = 8,
	VIDEO_WIDEOPTION_4_3_COMBINED = 9,
	VIDEO_WIDEOPTION_16_9_IGNORE = 10,
	VIDEO_WIDEOPTION_16_9_LETTER_BOX = 11,
	VIDEO_WIDEOPTION_16_9_PAN_SCAN = 12,
	VIDEO_WIDEOPTION_16_9_COMBINED = 13,
	VIDEO_WIDEOPTION_MAX = 14
};


// S805
#define AMSTREAM_IOC_VFORMAT _IOW(AMSTREAM_IOC_MAGIC, 0x04, int)
#define AMSTREAM_IOC_PORT_INIT _IO(AMSTREAM_IOC_MAGIC, 0x11)
#define AMSTREAM_IOC_TSTAMP _IOW(AMSTREAM_IOC_MAGIC, 0x0e, unsigned long)
#define AMSTREAM_IOC_VPTS _IOR(AMSTREAM_IOC_MAGIC, 0x41, unsigned long)
#define AMSTREAM_IOC_SET_PCRSCR _IOW(AMSTREAM_IOC_MAGIC, 0x4a, unsigned long)
#define AMSTREAM_IOC_VB_STATUS _IOR(AMSTREAM_IOC_MAGIC, 0x08, unsigned long)

struct am_io_param {
	union {
		int data;
		int id;//get bufstatus? //or others
	};

	int len; //buffer size;

	union {
		char buf[1];
		struct buf_status status;
		struct vdec_status vstatus;
		struct adec_status astatus;
	};
};

#endif

enum class ApiLevel
{
	Unknown = 0,
	S805,
	S905
};


class AmlCodec
{
	const unsigned long PTS_FREQ = 90000;

	const long EXTERNAL_PTS = (1);
	const long SYNC_OUTSIDE = (2);
	const long USE_IDR_FRAMERATE = 0x04;
	const long UCODE_IP_ONLY_PARAM = 0x08;
	const long MAX_REFER_BUF = 0x10;
	const long ERROR_RECOVERY_MODE_IN = 0x20;

	const char* CODEC_VIDEO_ES_DEVICE = "/dev/amstream_vbuf";
	const char* CODEC_VIDEO_ES_HEVC_DEVICE = "/dev/amstream_hevc";
	const char* CODEC_CNTL_DEVICE = "/dev/amvideo";
	typedef int CODEC_HANDLE;


	//codec_para_t codec = { 0 };
	bool isOpen = false;
	Mutex codecMutex;
	CODEC_HANDLE handle;
	CODEC_HANDLE cntl_handle;
	VideoFormatEnum format;
	int width;
	int height;
	double frameRate;
	ApiLevel apiLevel;


	void InternalOpen(VideoFormatEnum format, int width, int height, double frameRate);
	void InternalClose();

public:

	bool IsOpen() const
	{
		return isOpen;
	}



	AmlCodec();
	virtual ~AmlCodec() { }



	void Open(VideoFormatEnum format, int width, int height, double frameRate);
	void Close();
	void Reset();
	double GetCurrentPts();
	void SetCurrentPts(double value);
	void Pause();
	void Resume();
	buf_status GetBufferStatus();
	vdec_status GetVdecStatus();
	//bool SendData(unsigned long pts, unsigned char* data, int length);
	void SetVideoAxis(Int32Rectangle rectangle);
	Int32Rectangle GetVideoAxis();
	void SetSyncThreshold(unsigned long pts);
	void CheckinPts(unsigned long pts);
	int WriteData(unsigned char* data, int length);
};
