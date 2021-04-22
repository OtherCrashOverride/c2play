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

#include "AmlCodec.h"

#include "Exception.h"

#include <sys/ioctl.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>

#include "codec_type.h"
#include "amports/amstream.h"

#define VIDEO_WIDEOPTION_NORMAL (0)
#define VIDEO_DISABLE_NONE    (0)

AmlCodec::AmlCodec()
{
	int fd = open(CODEC_VIDEO_ES_DEVICE, O_WRONLY);
	if (fd < 0)
	{
		throw Exception("AmlCodec open failed.");
	}


	apiLevel = ApiLevel::S805;

	int version;
	int r = ioctl(fd, AMSTREAM_IOC_GET_VERSION, &version);
	if (r == 0)
	{
		printf("AmlCodec: amstream version : %d.%d\n", (version & 0xffff0000) >> 16, version & 0xffff);

		if (version >= 0x20000)
		{
			apiLevel = ApiLevel::S905;
		}
	}

	close(fd);
}


void AmlCodec::InternalOpen(VideoFormatEnum format, int width, int height, double frameRate)
{
	if (apiLevel < ApiLevel::S905)
	{
		if (width > 1920 || height > 1080)
		{
			throw NotSupportedException("The video resolution is not supported on this platform.");
		}
	}


	this->format = format;
	this->width = width;
	this->height = height;
	this->frameRate = frameRate;


	// Open codec
	int flags = O_WRONLY;
	switch (format)
	{
		case VideoFormatEnum::Hevc:
			//case VideoFormatEnum::VP9:
			handle = open(CODEC_VIDEO_ES_HEVC_DEVICE, flags);
			break;

		default:
			handle = open(CODEC_VIDEO_ES_DEVICE, flags);
			break;
	}

	if (handle < 0)
	{
		codecMutex.Unlock();
		throw Exception("AmlCodec open failed.");
	}


	// Set video format
	vformat_t amlFormat = (vformat_t)0;
	dec_sysinfo_t am_sysinfo = { 0 };

	//codec.stream_type = STREAM_TYPE_ES_VIDEO;
	//codec.has_video = 1;
	////codec.noblock = 1;

	// Note: Without EXTERNAL_PTS | SYNC_OUTSIDE, the codec auto adjusts
	// frame-rate from PTS 
	//am_sysinfo.param = (void*)(EXTERNAL_PTS | SYNC_OUTSIDE);
	//am_sysinfo.param = (void*)(EXTERNAL_PTS | SYNC_OUTSIDE | USE_IDR_FRAMERATE | UCODE_IP_ONLY_PARAM);
	//am_sysinfo.param = (void*)(SYNC_OUTSIDE | USE_IDR_FRAMERATE);
	//am_sysinfo.param = (void*)(EXTERNAL_PTS | SYNC_OUTSIDE | USE_IDR_FRAMERATE);
	am_sysinfo.param = (void*)(SYNC_OUTSIDE);

	// Rotation (clockwise)
	//am_sysinfo.param = (void*)((unsigned long)(am_sysinfo.param) | 0x10000); //90
	//am_sysinfo.param = (void*)((unsigned long)(am_sysinfo.param) | 0x20000); //180
	//am_sysinfo.param = (void*)((unsigned long)(am_sysinfo.param) | 0x30000); //270


	// Note: Testing has shown that the ALSA clock requires the +1
	am_sysinfo.rate = 96000.0 / frameRate + 0.5;

	//am_sysinfo.width = width;
	//am_sysinfo.height = height;
	//am_sysinfo.ratio64 = (((int64_t)width) << 32) | ((int64_t)height);


	switch (format)
	{
		case VideoFormatEnum::Mpeg2:
			printf("AmlVideoSink - VIDEO/MPEG2\n");

			amlFormat = VFORMAT_MPEG12;
			am_sysinfo.format = VIDEO_DEC_FORMAT_UNKNOW;
			break;

		case VideoFormatEnum::Mpeg4V3:
			printf("AmlVideoSink - VIDEO/MPEG4V3\n");

			amlFormat = VFORMAT_MPEG4;
			am_sysinfo.format = VIDEO_DEC_FORMAT_MPEG4_3;
			break;

		case VideoFormatEnum::Mpeg4:
			printf("AmlVideoSink - VIDEO/MPEG4\n");

			amlFormat = VFORMAT_MPEG4;
			am_sysinfo.format = VIDEO_DEC_FORMAT_MPEG4_5;
			break;

		case VideoFormatEnum::Avc:
		{
			if (width > 1920 || height > 1080)
			{
				printf("AmlVideoSink - VIDEO/H264_4K2K\n");

				amlFormat = VFORMAT_H264_4K2K;
				am_sysinfo.format = VIDEO_DEC_FORMAT_H264_4K2K;
			}
			else
			{
				printf("AmlVideoSink - VIDEO/H264\n");

				amlFormat = VFORMAT_H264;
				am_sysinfo.format = VIDEO_DEC_FORMAT_H264;
			}
		}
		break;

		case VideoFormatEnum::Hevc:
			printf("AmlVideoSink - VIDEO/HEVC\n");

			amlFormat = VFORMAT_HEVC;
			am_sysinfo.format = VIDEO_DEC_FORMAT_HEVC;
			break;


		case VideoFormatEnum::VC1:
			printf("AmlVideoSink - VIDEO/VC1\n");

			amlFormat = VFORMAT_VC1;
			am_sysinfo.format = VIDEO_DEC_FORMAT_WVC1;
			break;

		default:
			printf("AmlVideoSink - VIDEO/UNKNOWN(%d)\n", (int)format);

			codecMutex.Unlock();
			throw NotSupportedException();
	}


	// S905
	am_ioctl_parm parm = { 0 };
	int r;

	if (apiLevel >= ApiLevel::S905) // S905
	{
		parm.cmd = AMSTREAM_SET_VFORMAT;
		parm.data_vformat = amlFormat;

		r = ioctl(handle, AMSTREAM_IOC_SET, (unsigned long)&parm);
		if (r < 0)
		{
			codecMutex.Unlock();
			throw Exception("AMSTREAM_IOC_SET failed.");
		}
	}
	else //S805
	{
		r = ioctl(handle, AMSTREAM_IOC_VFORMAT, amlFormat);
		if (r < 0)
		{
			codecMutex.Unlock();
			throw Exception("AMSTREAM_IOC_VFORMAT failed.");
		}
	}


	r = ioctl(handle, AMSTREAM_IOC_SYSINFO, (unsigned long)&am_sysinfo);
	if (r < 0)
	{
		codecMutex.Unlock();
		throw Exception("AMSTREAM_IOC_SYSINFO failed.");
	}


	// Control device
	cntl_handle = open(CODEC_CNTL_DEVICE, O_RDWR);
	if (cntl_handle < 0)
	{
		codecMutex.Unlock();
		throw Exception("open CODEC_CNTL_DEVICE failed.");
	}

	/*
	if (pcodec->vbuf_size > 0)
	{
	r = codec_h_ioctl(pcodec->handle, AMSTREAM_IOC_SET, AMSTREAM_SET_VB_SIZE, pcodec->vbuf_size);
	if (r < 0)
	{
	return system_error_to_codec_error(r);
	}
	}
	*/

	if (apiLevel >= ApiLevel::S905)	//S905
	{
		parm = { 0 };
		parm.cmd = AMSTREAM_PORT_INIT;

		r = ioctl(handle, AMSTREAM_IOC_SET, (unsigned long)&parm);
		if (r != 0)
		{
			codecMutex.Unlock();
			throw Exception("AMSTREAM_PORT_INIT failed.");
		}
	}
	else	//S805
	{
		r = ioctl(handle, AMSTREAM_IOC_PORT_INIT, 0);
		if (r != 0)
		{
			codecMutex.Unlock();
			throw Exception("AMSTREAM_IOC_PORT_INIT failed.");
		}
	}


	//codec_h_control(pcodec->cntl_handle, AMSTREAM_IOC_SYNCENABLE, (unsigned long)enable);
	r = ioctl(cntl_handle, AMSTREAM_IOC_SYNCENABLE, (unsigned long)1);
	if (r != 0)
	{
		codecMutex.Unlock();
		throw Exception("AMSTREAM_IOC_SYNCENABLE failed.");
	}

#if 0
	// Restore settings that Kodi tramples
	r = ioctl(cntl_handle, AMSTREAM_IOC_SET_VIDEO_DISABLE, (unsigned long)VIDEO_DISABLE_NONE);
	if (r != 0)
	{
		throw Exception("AMSTREAM_IOC_SET_VIDEO_DISABLE VIDEO_DISABLE_NONE failed.");
	}
#endif

	uint32_t screenMode = (uint32_t)VIDEO_WIDEOPTION_NORMAL;
	r = ioctl(cntl_handle, AMSTREAM_IOC_SET_SCREEN_MODE, &screenMode);
	if (r != 0)
	{
		std::string err = "AMSTREAM_IOC_SET_SCREEN_MODE VIDEO_WIDEOPTION_NORMAL failed (" + std::to_string(r) + ").";
		throw Exception(err.c_str());
	}


	// Debug info
	printf("\tw=%d h=%d ", width, height);

	printf("fps=%f ", frameRate);

	printf("am_sysinfo.rate=%d ",
		am_sysinfo.rate);

	printf("\n");


	//// Intialize the hardware codec
	//int api = codec_init(&codec);
	////int api = codec_init_no_modeset(&codecContext);
	//if (api != 0)
	//{
	//	printf("codec_init failed (%x).\n", api);

	//	codecMutex.Unlock();
	//	throw Exception();
	//}

#if 0
	// Set video size
	int video_fd = open("/dev/amvideo", O_RDWR);
	if (video_fd < 0)
	{
		throw Exception("open /dev/amvideo failed.");
	}

	
	int params[4]{ 0, 0, -1, -1 };

	int ret = ioctl(video_fd, AMSTREAM_IOC_SET_VIDEO_AXIS, &params);
	if (ret < 0)
	{
		throw Exception("ioctl AMSTREAM_IOC_SET_VIDEO_AXIS failed.");
	}

	close(video_fd);
#endif

	isOpen = true;
}

void AmlCodec::InternalClose()
{
	int r;

	r = ioctl(cntl_handle, AMSTREAM_IOC_CLEAR_VIDEO, 0);
	if (r < 0)
	{
		codecMutex.Unlock();
		throw Exception("AMSTREAM_IOC_CLEAR_VIDEO failed.");
	}

	r = close(cntl_handle);
	if (r < 0)
	{
		codecMutex.Unlock();
		throw Exception("close cntl_handle failed.");
	}


	r = close(handle);
	if (r < 0)
	{
		codecMutex.Unlock();
		throw Exception("close handle failed.");
	}

	cntl_handle = -1;
	handle = -1;

	isOpen = false;
}

void AmlCodec::Open(VideoFormatEnum format, int width, int height, double frameRate)
{
	if (width < 1)
		throw ArgumentOutOfRangeException("width");

	if (height < 1)
		throw ArgumentOutOfRangeException("height");

	if (frameRate < 1)
		throw ArgumentOutOfRangeException("frameRate");


	codecMutex.Lock();

	if (isOpen)
	{
		codecMutex.Unlock();
		throw InvalidOperationException("The codec is already open.");
	}	

	InternalOpen(format, width, height, frameRate);

	codecMutex.Unlock();

	//// This is needed because the codec remains paused
	//// even after closing
	//int ret = codec_resume(&codec);
	//if (ret < 0)
	//{
	//	printf("codec_resume failed (%x).\n", ret);
	//}

	Resume();
}

void AmlCodec::Close()
{
	codecMutex.Lock();

	if (!isOpen)
	{
		codecMutex.Unlock();
		throw InvalidOperationException("The codec is not open.");
	}


	InternalClose();

	codecMutex.Unlock();
}

void AmlCodec::Reset()
{
	codecMutex.Lock();

	if (!isOpen)
	{
		codecMutex.Unlock();
		throw InvalidOperationException("The codec is not open.");
	}

	//codec_reset(&codec);

	//codecMutex.Unlock();
	VideoFormatEnum format = this->format;
	int width = this->width ;
	int height = this->height ;
	double frameRate = this->frameRate;

	//Close();
	//Open(format, width, height, frameRate);


	InternalClose();
	InternalOpen(format, width, height, frameRate);

	codecMutex.Unlock();
}

double AmlCodec::GetCurrentPts()
{
	codecMutex.Lock();

	if (!isOpen)
	{
		codecMutex.Unlock();
		throw InvalidOperationException("The codec is not open.");
	}


	unsigned int vpts;
	int ret;
	if (apiLevel >= ApiLevel::S905)	// S905
	{
		//int vpts = codec_get_vpts(&codec);
		//unsigned int vpts;
		am_ioctl_parm parm = { 0 };

		parm.cmd = AMSTREAM_GET_VPTS;
		//parm.data_32 = &vpts;

		ret = ioctl(handle, AMSTREAM_IOC_GET, (unsigned long)&parm);
		if (ret < 0)
		{
			codecMutex.Unlock();
			throw Exception("AMSTREAM_GET_VPTS failed.");
		}

		vpts = parm.data_32;
		//unsigned long vpts = parm.data_64;

		//printf("AmlCodec::GetCurrentPts() parm.data_32=%u parm.data_64=%llu\n",
		//	parm.data_32, parm.data_64);
	}
	else	// S805
	{
		ret = ioctl(handle, AMSTREAM_IOC_VPTS, (unsigned long)&vpts);
		if (ret < 0)
		{
			codecMutex.Unlock();
			throw Exception("AMSTREAM_IOC_VPTS failed.");
		}
	}

	codecMutex.Unlock();

	return vpts / (double)PTS_FREQ;
}

void AmlCodec::SetCurrentPts(double value)
{
	codecMutex.Lock();
#if 1
	if (!isOpen)
	{
		codecMutex.Unlock();
		throw InvalidOperationException("The codec is not open.");
	}

	//unsigned int pts = value * PTS_FREQ;

	//int codecCall = codec_set_pcrscr(&codec, (int)pts);
	//if (codecCall != 0)
	//{
	//	printf("codec_set_pcrscr failed.\n");
	//}

	// truncate to 32bit
	unsigned long pts = (unsigned long)(value * PTS_FREQ);
	pts &= 0xffffffff;

	if (apiLevel >= ApiLevel::S905)	// S905
	{
		am_ioctl_parm parm = { 0 };

		parm.cmd = AMSTREAM_SET_PCRSCR;
		parm.data_32 = (unsigned int)(pts);
		//parm.data_64 = (unsigned long)(value * PTS_FREQ);

		int ret = ioctl(handle, AMSTREAM_IOC_SET, (unsigned long)&parm);
		if (ret < 0)
		{
			codecMutex.Unlock();
			throw Exception("AMSTREAM_SET_PCRSCR failed.");
		}
		else
		{
			//printf("AmlCodec::SetCurrentPts - parm.data_32=%u\n", parm.data_32);
		}
	}
	else	// S805
	{
		int ret = ioctl(handle, AMSTREAM_IOC_SET_PCRSCR, pts);
		if (ret < 0)
		{
			codecMutex.Unlock();
			throw Exception("AMSTREAM_IOC_SET_PCRSCR failed.");
		}
	}
#endif
	codecMutex.Unlock();
}

void AmlCodec::Pause()
{
	codecMutex.Lock();

	if (!isOpen)
	{
		codecMutex.Unlock();
		throw InvalidOperationException("The codec is not open.");
	}

	
	//codec_pause(&codec);
	int ret = ioctl(cntl_handle, AMSTREAM_IOC_VPAUSE, 1);
	if (ret < 0)
	{
		codecMutex.Unlock();
		throw Exception("AMSTREAM_IOC_VPAUSE (1) failed.");
	}

	codecMutex.Unlock();
}

void AmlCodec::Resume()
{
	codecMutex.Lock();

	if (!isOpen)
	{
		codecMutex.Unlock();
		throw InvalidOperationException("The codec is not open.");
	}

	
	//codec_resume(&codec);
	int ret = ioctl(cntl_handle, AMSTREAM_IOC_VPAUSE, 0);
	if (ret < 0)
	{
		codecMutex.Unlock();
		throw Exception("AMSTREAM_IOC_VPAUSE (0) failed.");
	}

	codecMutex.Unlock();
}

buf_status AmlCodec::GetBufferStatus()
{
	codecMutex.Lock();

	if (!isOpen)
	{
		codecMutex.Unlock();
		throw InvalidOperationException("The codec is not open.");
	}

	
	buf_status status;
	if (apiLevel >= ApiLevel::S905)	// S905
	{
		am_ioctl_parm_ex parm = { 0 };
		parm.cmd = AMSTREAM_GET_EX_VB_STATUS;
		
		int r = ioctl(handle, AMSTREAM_IOC_GET_EX, (unsigned long)&parm);

		codecMutex.Unlock();

		if (r < 0)
		{
			throw Exception("AMSTREAM_GET_EX_VB_STATUS failed.");
		}

		memcpy(&status, &parm.status, sizeof(status));
	}
	else	// S805
	{
		am_io_param am_io;

		int r = ioctl(handle, AMSTREAM_IOC_VB_STATUS, (unsigned long)&am_io);

		codecMutex.Unlock();

		if (r < 0)
		{
			throw Exception("AMSTREAM_IOC_VB_STATUS failed.");
		}

		memcpy(&status, &am_io.status, sizeof(status));
	}

	return status;
}

vdec_status AmlCodec::GetVdecStatus()
{
	codecMutex.Lock();

	if (!isOpen)
	{
		codecMutex.Unlock();
		throw InvalidOperationException("The codec is not open.");
	}



	am_ioctl_parm_ex parm = { 0 };
	parm.cmd = AMSTREAM_GET_EX_VDECSTAT;

	int r = ioctl(handle, AMSTREAM_IOC_GET_EX, (unsigned long)&parm);

	codecMutex.Unlock();

	if (r < 0)
	{
		throw Exception("AMSTREAM_GET_EX_VB_STATUS failed.");
	}


	vdec_status status;
	memcpy(&status, &parm.status, sizeof(status));

	return status;
}

//bool AmlCodec::SendData(unsigned long pts, unsigned char* data, int length)
//{
//	if (!isOpen)
//		throw InvalidOperationException("The codec is not open.");
//
//	if (data == nullptr)
//		throw ArgumentNullException("data");
//
//	if (length < 1)
//		throw ArgumentOutOfRangeException("length");
//
//
//	bool result = true;
//
//	//printf("AmlVideoSink: SendCodecData - pts=%lu, data=%p, length=0x%x\n", pts, data, length);
//
//	// truncate to 32bit
//	pts &= 0xffffffff;
//
//	int api;
//
//	if (pts > 0)
//	{
//		codecMutex.Lock();
//
//		//if (codec_checkin_pts(&codec, pts))
//		//{
//		//	printf("codec_checkin_pts failed\n");
//		//}
//
//		//return codec_h_ioctl(pcodec->handle, AMSTREAM_IOC_SET, AMSTREAM_SET_TSTAMP, pts);
//
//		am_ioctl_parm parm = { 0 };
//		
//#if 1
//		parm.cmd = AMSTREAM_SET_TSTAMP;
//		parm.data_32 = (unsigned int)pts;
//#else
//		// This gets converted to a 32bit value
//		// https://github.com/hardkernel/linux/blob/odroidc2-3.14.y/drivers/amlogic/amports/ptsserv.c#L525
//		parm.cmd = AMSTREAM_SET_TSTAMP_US64;
//		parm.data_64 = pts; // todo microseconds
//#endif
//
//		int r = ioctl(handle, AMSTREAM_IOC_SET, (unsigned long)&parm);
//		if (r < 0)
//		{
//			printf("codec_checkin_pts failed\n");
//		}
//		else
//		{
//			//printf("AmlCodec::SendData - pts=%lu\n", pts);
//		}
//
//		codecMutex.Unlock();
//	}
//
//
//	int offset = 0;
//	int maxAttempts = 150;
//	while (api == -EAGAIN || offset < length)
//	{
//		//codecMutex.Lock();
//
//		//api = codec_write(&codec, data + offset, length - offset);
//		api = write(handle, data + offset, length - offset);
//
//		//codecMutex.Unlock();
//
//		if (api > 0)
//		{
//			offset += api;
//			//printf("codec_write send %x bytes of %x total.\n", api, pkt.size);
//		}
//		else //if(api != -EAGAIN && api != -1)
//		{
//			//printf("codec_write failed (%x).\n", api);
//
//			maxAttempts -= 1;
//			if (maxAttempts <= 0)
//			{
//				printf("codec_write max attempts exceeded.\n");
//				Reset();
//				result = false;
//				break;
//			}
//			
//		}
//
//		//if (ExecutionState() == ExecutionStateEnum::Terminating)
//		//	break;
//	}
//
//	return result;
//}

void AmlCodec::SetVideoAxis(Int32Rectangle rectangle)
{
	codecMutex.Lock();

	if (!isOpen)
	{
		codecMutex.Unlock();
		throw InvalidOperationException("The codec is not open.");
	}

	int params[4]{ rectangle.X,
		rectangle.Y,
		rectangle.X + rectangle.Width,
		rectangle.Y + rectangle.Height };

	int ret = ioctl(cntl_handle, AMSTREAM_IOC_SET_VIDEO_AXIS, &params);

	codecMutex.Unlock();

	if (ret < 0)
	{
		throw Exception("AMSTREAM_IOC_SET_VIDEO_AXIS failed.");
	}
}

Int32Rectangle AmlCodec::GetVideoAxis()
{
	codecMutex.Lock();

	if (!isOpen)
	{
		codecMutex.Unlock();
		throw InvalidOperationException("The codec is not open.");
	}

	int params[4] = { 0 };

	int ret = ioctl(cntl_handle, AMSTREAM_IOC_GET_VIDEO_AXIS, &params);

	codecMutex.Unlock();

	if (ret < 0)
	{
		throw Exception("AMSTREAM_IOC_GET_VIDEO_AXIS failed.");
	}

	
	Int32Rectangle result(params[0],
		params[1],
		params[2] - params[0],
		params[3] - params[1]);
	
	return result;
}

void AmlCodec::SetSyncThreshold(unsigned long pts)
{
	codecMutex.Lock();

	if (!isOpen)
	{
		codecMutex.Unlock();
		throw InvalidOperationException("The codec is not open.");
	}

	//return codec_h_control(pcodec->cntl_handle, AMSTREAM_IOC_SYNCTHRESH, (unsigned long)syncthresh);
	int ret = ioctl(cntl_handle, AMSTREAM_IOC_SYNCTHRESH, pts);

	codecMutex.Unlock();


	if (ret < 0)
	{
		throw Exception("AMSTREAM_IOC_SYNCTHRESH failed.");
	}

}

void AmlCodec::CheckinPts(unsigned long pts)
{
	codecMutex.Lock();


	if (!isOpen)
	{
		codecMutex.Unlock();
		throw InvalidOperationException("The codec is not open.");
	}


	// truncate to 32bit
	//pts &= 0xffffffff;

	if (apiLevel >= ApiLevel::S905)	// S905
	{
		am_ioctl_parm parm = { 0 };
		parm.cmd = AMSTREAM_SET_TSTAMP;
		parm.data_32 = (unsigned int)pts;

		int r = ioctl(handle, AMSTREAM_IOC_SET, (unsigned long)&parm);
		if (r < 0)
		{
			codecMutex.Unlock();
			throw Exception("AMSTREAM_SET_TSTAMP failed\n");
		}
	}
	else	// S805
	{
		int r = ioctl(handle, AMSTREAM_IOC_TSTAMP, pts);
		if (r < 0)
		{
			codecMutex.Unlock();
			throw Exception("AMSTREAM_IOC_TSTAMP failed\n");
		}
	}

	codecMutex.Unlock();
}

int AmlCodec::WriteData(unsigned char* data, int length)
{
	if (data == nullptr)
		throw ArgumentNullException("data");

	if (length < 1)
		throw ArgumentOutOfRangeException("length");


	// This is done unlocked because it blocks
	// int written = 0;
	// while (written < length)
	// {
		int ret = write(handle, data, length);
	// 	if (ret < 0)
	// 	{
	// 		if (errno == EAGAIN)
	// 		{
	// 			//printf("EAGAIN.\n");
	// 			sleep(0);

	// 			ret = 0;
	// 		}
	// 		else
	// 		{
	// 			printf("write failed. (%d)(%d)\n", ret, errno);
	// 			abort();
	// 		}
	// 	}

	// 	written += ret;
	// }

	return ret; //written;
}