#include "AmlCodec.h"

#include "Exception.h"

#include <sys/ioctl.h>
#include <sys/stat.h>
#include <fcntl.h>


void AmlCodec::Open(VideoFormatEnum format, int width, int height, double frameRate)
{
	if (width < 1)
		throw ArgumentOutOfRangeException("width");

	if (height < 1)
		throw ArgumentOutOfRangeException("height");

	if (frameRate < 1)
		throw ArgumentOutOfRangeException("frameRate");

	if (isOpen)
		throw InvalidOperationException("The codec is already open.");


	codecMutex.Lock();


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
	am_sysinfo.param = (void*)(EXTERNAL_PTS | SYNC_OUTSIDE); //USE_IDR_FRAMERATE

	// Note: Testing has shown that the ALSA clock requires the +1
	am_sysinfo.rate = 96000.0 / frameRate + 1;


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


	am_ioctl_parm parm = { 0 };

	parm.cmd = AMSTREAM_SET_VFORMAT;
	parm.data_vformat = amlFormat;

	int r = ioctl(handle, AMSTREAM_IOC_SET, (unsigned long)&parm);
	if (r < 0) 
	{
		codecMutex.Unlock();
		throw Exception("AMSTREAM_IOC_SET failed.");
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

	parm = { 0 };
	parm.cmd = AMSTREAM_PORT_INIT;
	
	r = ioctl(handle, AMSTREAM_IOC_SET, (unsigned long)&parm);
	if (r != 0)
	{
		codecMutex.Unlock();
		throw Exception("AMSTREAM_PORT_INIT failed.");
	}


	//// Rotation
	////codecContext.am_sysinfo.param = (void*)((unsigned long)(codecContext.am_sysinfo.param) | 0x10000); //90
	////codecContext.am_sysinfo.param = (void*)((unsigned long)(codecContext.am_sysinfo.param) | 0x20000); //180
	////codecContext.am_sysinfo.param = (void*)((unsigned long)(codecContext.am_sysinfo.param) | 0x30000); //270


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

	isOpen = true;
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
	if (!isOpen)
		throw InvalidOperationException("The codec is not open.");


	codecMutex.Lock();

	//codec_close(&codec);

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

	codecMutex.Unlock();
}

void AmlCodec::Reset()
{
	if (!isOpen)
		throw InvalidOperationException("The codec is not open.");


	//codecMutex.Lock();

	//codec_reset(&codec);

	//codecMutex.Unlock();
	VideoFormatEnum format = this->format;
	int width = this->width ;
	int height = this->height ;
	double frameRate = this->frameRate;

	Close();
	Open(format, width, height, frameRate);
}

double AmlCodec::GetCurrentPts()
{
	if (!isOpen)
		throw InvalidOperationException("The codec is not open.");


	codecMutex.Lock();


	//int vpts = codec_get_vpts(&codec);
	//unsigned int vpts;
	am_ioctl_parm parm = { 0 };

	parm.cmd = AMSTREAM_GET_VPTS;
	//parm.data_32 = &vpts;

	int ret = ioctl(handle, AMSTREAM_IOC_GET, (unsigned long)&parm);
	if (ret < 0)
	{
		codecMutex.Unlock();
		throw Exception("AMSTREAM_GET_VPTS failed.");
	}

	unsigned int vpts = parm.data_32;
	//unsigned long vpts = parm.data_64;

	//printf("AmlCodec::GetCurrentPts() parm.data_32=%u parm.data_64=%llu\n",
	//	parm.data_32, parm.data_64);

	codecMutex.Unlock();

	return vpts / (double)PTS_FREQ;
}

void AmlCodec::SetCurrentPts(double value)
{
	if (!isOpen)
		throw InvalidOperationException("The codec is not open.");


	codecMutex.Lock();

	//unsigned int pts = value * PTS_FREQ;

	//int codecCall = codec_set_pcrscr(&codec, (int)pts);
	//if (codecCall != 0)
	//{
	//	printf("codec_set_pcrscr failed.\n");
	//}

	am_ioctl_parm parm = { 0 };
	
	parm.cmd = AMSTREAM_SET_PCRSCR;
	parm.data_32 = (unsigned int)(value * PTS_FREQ);
	//parm.data_64 = value * PTS_FREQ;

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

	codecMutex.Unlock();
}

void AmlCodec::Pause()
{
	if (!isOpen)
		throw InvalidOperationException("The codec is not open.");


	codecMutex.Lock();
	
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
	if (!isOpen)
		throw InvalidOperationException("The codec is not open.");


	codecMutex.Lock();
	
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
	if (!isOpen)
		throw InvalidOperationException("The codec is not open.");

	
	//if (true) 
	{
		buf_status status;
		//r = codec_h_ioctl(p->handle, AMSTREAM_IOC_GET_EX, AMSTREAM_GET_EX_VB_STATUS, (unsigned long)&status);
		//memcpy(buf, &status, sizeof(*buf));


		am_ioctl_parm_ex parm = { 0 };

		parm.cmd = AMSTREAM_GET_EX_VB_STATUS;
		
		int r = ioctl(handle, AMSTREAM_IOC_GET_EX, (unsigned long)&parm);
		if (r < 0)
		{
			codecMutex.Unlock();
			throw Exception("AMSTREAM_GET_EX_VB_STATUS failed.");
		}

		memcpy(&status, &parm.status, sizeof(status));

		return status;
	}
	//else 
	//{
	//	struct am_io_param am_io;
	//	r = codec_h_control(p->handle, AMSTREAM_IOC_VB_STATUS, (unsigned long)&am_io);
	//	memcpy(buf, &am_io.status, sizeof(*buf));
	//}
}

void AmlCodec::SendData(unsigned long pts, unsigned char* data, int length)
{
	if (!isOpen)
		throw InvalidOperationException("The codec is not open.");

	if (data == nullptr)
		throw ArgumentNullException("data");

	if (length < 1)
		throw ArgumentOutOfRangeException("length");



	//printf("AmlVideoSink: SendCodecData - pts=%lu, data=%p, length=0x%x\n", pts, data, length);



	int api;

	if (pts > 0)
	{
		codecMutex.Lock();

		//if (codec_checkin_pts(&codec, pts))
		//{
		//	printf("codec_checkin_pts failed\n");
		//}

		//return codec_h_ioctl(pcodec->handle, AMSTREAM_IOC_SET, AMSTREAM_SET_TSTAMP, pts);

		am_ioctl_parm parm = { 0 };
		
		parm.cmd = AMSTREAM_SET_TSTAMP;
		parm.data_32 = (unsigned int)pts;
		//parm.data_64 = pts;

		int r = ioctl(handle, AMSTREAM_IOC_SET, (unsigned long)&parm);
		if (r < 0)
		{
			printf("codec_checkin_pts failed\n");
		}
		else
		{
			//printf("AmlCodec::SendData - pts=%lu\n", pts);
		}

		codecMutex.Unlock();
	}


	int offset = 0;
	while (api == -EAGAIN || offset < length)
	{
		//codecMutex.Lock();

		//api = codec_write(&codec, data + offset, length - offset);
		api = write(handle, data + offset, length - offset);

		//codecMutex.Unlock();

		if (api > 0)
		{
			offset += api;
			//printf("codec_write send %x bytes of %x total.\n", api, pkt.size);
		}
		else
		{
			//printf("codec_write failed (%x).\n", api);
		}

		//if (ExecutionState() == ExecutionStateEnum::Terminating)
		//	break;
	}
}

void AmlCodec::SetVideoAxis(Int32Rectangle rectangle)
{
	if (!isOpen)
		throw InvalidOperationException("The codec is not open.");

	codecMutex.Lock();

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
	if (!isOpen)
		throw InvalidOperationException("The codec is not open.");

	codecMutex.Lock();

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
