#pragma once

extern "C"
{
	// aml_lib
#include <codec.h>
}

#include "Mutex.h"
#include "Pin.h"
#include "Rectangle.h"


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

public:

	bool IsOpen() const
	{
		return isOpen;
	}


	void Open(VideoFormatEnum format, int width, int height, double frameRate);
	void Close();
	void Reset();
	double GetCurrentPts();
	void SetCurrentPts(double value);
	void Pause();
	void Resume();
	buf_status GetBufferStatus();
	void SendData(unsigned long pts, unsigned char* data, int length);
	void SetVideoAxis(Int32Rectangle rectangle);
	Int32Rectangle GetVideoAxis();

};
