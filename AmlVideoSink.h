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

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

extern "C"
{
	// aml_lib
#include <codec.h>
}

#include <vector>

#include "Codec.h"
#include "Element.h"
#include "InPin.h"
#include "Thread.h"


//class AmlVideoSinkElement;
//std::shared_ptr<AmlVideoSinkElement> AmlVideoSinkElementSPTR;


#include <signal.h>
#include <time.h>
#include <math.h>

class Timer
{
	timer_t timer_id;


	double interval = 0;
	bool isRunning = false;


	static void timer_thread(sigval arg)
	{
		Timer* timerPtr = (Timer*)arg.sival_ptr;
		timerPtr->Expired.Invoke(timerPtr, EventArgs::Empty());

		//printf("Timer: expired arg=%p.\n", arg.sival_ptr);
	}

public:
	Event<EventArgs> Expired;


	double Interval() const
	{
		return interval;
	}
	void SetInterval(double value)
	{
		interval = value;
	}



	Timer()
	{
		sigevent se = { 0 };
		
		se.sigev_notify = SIGEV_THREAD;
		se.sigev_value.sival_ptr = &timer_id;
		se.sigev_notify_function = &Timer::timer_thread;
		se.sigev_notify_attributes = NULL;
		se.sigev_value.sival_ptr = this;

		int ret = timer_create(CLOCK_REALTIME, &se, &timer_id);
		if (ret != 0)
		{
			throw Exception("Timer creation failed.");
		}
	}

	~Timer()
	{
		timer_delete(timer_id);
	}


	void Start()
	{
		if (isRunning)
			throw InvalidOperationException();


		itimerspec ts = { 0 };

		// arm value
		ts.it_value.tv_sec = floor(interval);
		ts.it_value.tv_nsec = floor((interval - floor(interval)) * 1e9);
		
		// re-arm value
		ts.it_interval.tv_sec = ts.it_value.tv_sec;
		ts.it_interval.tv_nsec = ts.it_value.tv_nsec;

		int ret = timer_settime(timer_id, 0, &ts, 0);
		if (ret != 0)
		{
			throw Exception("timer_settime failed.");
		}

		isRunning = true;
	}

	void Stop()
	{
		if (!isRunning)
			throw InvalidOperationException();


		itimerspec ts = { 0 };

		int ret = timer_settime(timer_id, 0, &ts, 0);
		if (ret != 0)
		{
			throw Exception("timer_settime failed.");
		}

		isRunning = false;
	}
};


class AmlVideoSinkClockInPin : public InPin
{
	const unsigned long PTS_FREQ = 90000;

	codec_para_t* codecContextPtr;
	double clock = 0;
	double frameRate = 0;	// TODO just read info from Owner()


	void ProcessClockBuffer(BufferSPTR buffer)
	{
		clock = buffer->TimeStamp();
		unsigned long pts = (unsigned long)(buffer->TimeStamp() * PTS_FREQ);


		int vpts = codec_get_vpts(codecContextPtr);

		if (clock < vpts * (double)PTS_FREQ)
		{
			clock = vpts * (double)PTS_FREQ;
		}


		//AmlVideoSinkClockSPTR

		int drift = vpts - pts;
		double driftTime = drift / (double)PTS_FREQ;
		double driftFrames = driftTime * frameRate;
		//printf("Clock drift = %f (frames=%f)\n", driftTime, driftFrames);

		//clockPts = pts;
		//lastClockPts = clockPts;
		//printf("AmlVideoSinkElement: clock=%f, pts=%lu, clockPts=%lu\n", buffer->TimeStamp(), pts, clockPts);

		// To minimize clock jitter, only adjust the clock if it
		// deviates more than +/- 2 frames
		if (driftFrames >= 2.0 || driftFrames <= -2.0)
		{
			if (pts > 0)
			{
				int codecCall = codec_set_pcrscr(codecContextPtr, (int)pts);
				if (codecCall != 0)
				{
					printf("codec_set_pcrscr failed.\n");
				}

				//long delta = (long)pts - (long)clockPts;
				printf("AmlVideoSink: codec_set_pcrscr - pts=%lu drift=%f (%f frames)\n", pts, driftTime, driftFrames);
			}
		}
	}

protected:

	virtual void DoWork() override
	{
		BufferSPTR buffer;
		while (TryGetFilledBuffer(&buffer))
		{
			ProcessClockBuffer(buffer);

			PushProcessedBuffer(buffer);
			ReturnProcessedBuffers();
		}
	}


public:

	double Clock() const
	{
		//return clock;
		int vpts = codec_get_vpts(codecContextPtr);
		
		return vpts * (double)PTS_FREQ;
	}

	double FrameRate() const
	{
		return frameRate;
	}
	void SetFrameRate(double value)
	{
		frameRate = value;
	}



	AmlVideoSinkClockInPin(ElementWPTR owner, PinInfoSPTR info, codec_para_t* codecContextPtr)
		: InPin(owner, info),
		codecContextPtr(codecContextPtr)
	{
		if (codecContextPtr == nullptr)
			throw ArgumentNullException();
	}
};

typedef std::shared_ptr<AmlVideoSinkClockInPin> AmlVideoSinkClockInPinSPTR;



class AmlVideoSinkElement : public Element
{
	const unsigned long PTS_FREQ = 90000;
	
	const long EXTERNAL_PTS = (1);
	const long SYNC_OUTSIDE = (2);
	const long USE_IDR_FRAMERATE = 0x04;
	const long UCODE_IP_ONLY_PARAM = 0x08;
	const long MAX_REFER_BUF = 0x10;
	const long ERROR_RECOVERY_MODE_IN = 0x20;

	std::vector<unsigned char> videoExtraData;
	//AVCodecID codec_id;
	//double frameRate;
	codec_para_t codecContext;
	//void* extraData = nullptr;
	//int extraDataSize = 0;
	/*AVRational time_base;*/
	double lastTimeStamp = -1;

	bool isFirstVideoPacket = true;
	bool isAnnexB = false;
	bool isExtraDataSent = false;
	long estimatedNextPts = 0;

	VideoFormatEnum videoFormat = VideoFormatEnum::Unknown;
	VideoInPinSPTR videoPin;
	bool isFirstData = true;
	std::vector<unsigned char> extraData;

	//InPinSPTR clockInPin;
	//Thread clockThread = Thread(std::function<void()>(std::bind(&AmlVideoSinkElement::ClockWorkThread, this)));
	unsigned long clockPts = 0;
	unsigned long lastClockPts = 0;
	double clock = 0;

	AmlVideoSinkClockInPinSPTR clockInPin;
	bool isEndOfStream = false;

	Timer timer;
	EventListenerSPTR<EventArgs> timerExpiredListener;
	Mutex timerMutex;
	bool doPauseFlag = false;
	bool doResumeFlag = false;
	Mutex playPauseMutex;


	void timer_Expired(void* sender, const EventArgs& args)
	{
		timerMutex.Lock();

		if (isEndOfStream)
		{
			buf_status bufferStatus;
			int api = codec_get_vbuf_state(&codecContext, &bufferStatus);
			if (api == 0)
			{
				//printf("AmlVideoSinkElement: codec_get_vbuf_state free_len=%d, size=%d, data_len=%d, read_pointer=%x, write_pointer=%x\n",
				//	bufferStatus.free_len, bufferStatus.size, bufferStatus.data_len, bufferStatus.read_pointer, bufferStatus.write_pointer);

				// Testing has shown this value does not reach zero
				if (bufferStatus.data_len < 512)
				{
					printf("AmlVideoSinkElement: timer_Expired - isEndOfStream=true (Pausing).\n");
					SetState(MediaState::Pause);
					isEndOfStream = false;
				}
			}
		}

		timerMutex.Unlock();
		
		//printf("AmlVideoSinkElement: timer expired.\n");
	}

	void SetupHardware()
	{
		int width = videoPin->InfoAs()->Width;
		int height = videoPin->InfoAs()->Height;
		double frameRate = videoPin->InfoAs()->FrameRate;


		memset(&codecContext, 0, sizeof(codecContext));

		codecContext.stream_type = STREAM_TYPE_ES_VIDEO;
		codecContext.has_video = 1;

		// Note: Without EXTERNAL_PTS | SYNC_OUTSIDE, the codec auto adjusts
		// frame-rate from PTS 
		codecContext.am_sysinfo.param = (void*)(EXTERNAL_PTS | SYNC_OUTSIDE); //USE_IDR_FRAMERATE
		

		//if (videoPin->InfoAs()->HasEstimatedPts)
		//{
		//	printf("AmlVideoSink: Using alternate time formula.\n");
		//	codecContext.am_sysinfo.rate = 96000.0 / frameRate;
		//}
		//else
		{
			printf("AmlVideoSink: Using standard time formula.\n");
			// Note: Testing has shown that the ALSA clock requires the +1
			codecContext.am_sysinfo.rate = 96000.0 / frameRate + 1;
			//codecContext.noblock = 1;
		}

		switch (videoPin->InfoAs()->Format)
		{
		case VideoFormatEnum::Mpeg2:
			printf("AmlVideoSink - VIDEO/MPEG2\n");

			codecContext.video_type = VFORMAT_MPEG12;
			codecContext.am_sysinfo.format = VIDEO_DEC_FORMAT_UNKNOW;
			break;

		case VideoFormatEnum::Mpeg4V3:
			printf("AmlVideoSink - VIDEO/MPEG4V3\n");

			codecContext.video_type = VFORMAT_MPEG4;
			codecContext.am_sysinfo.format = VIDEO_DEC_FORMAT_MPEG4_3;
			//VIDEO_DEC_FORMAT_MP4; //VIDEO_DEC_FORMAT_MPEG4_3; //VIDEO_DEC_FORMAT_MPEG4_4; //VIDEO_DEC_FORMAT_MPEG4_5;
			break;

		case VideoFormatEnum::Mpeg4:
			printf("AmlVideoSink - VIDEO/MPEG4\n");

			codecContext.video_type = VFORMAT_MPEG4;
			codecContext.am_sysinfo.format = VIDEO_DEC_FORMAT_MPEG4_5;
			//VIDEO_DEC_FORMAT_MP4; //VIDEO_DEC_FORMAT_MPEG4_3; //VIDEO_DEC_FORMAT_MPEG4_4; //VIDEO_DEC_FORMAT_MPEG4_5;
			break;

		case VideoFormatEnum::Avc:
		{
			if (width > 1920 ||	height > 1080)
			{
				printf("AmlVideoSink - VIDEO/H264_4K2K\n");

				codecContext.video_type = VFORMAT_H264_4K2K;
				codecContext.am_sysinfo.format = VIDEO_DEC_FORMAT_H264_4K2K;
				//codecContext.am_sysinfo.param = (void*)(EXTERNAL_PTS);
			}
			else
			{
				printf("AmlVideoSink - VIDEO/H264\n");

				codecContext.video_type = VFORMAT_H264;
				codecContext.am_sysinfo.format = VIDEO_DEC_FORMAT_H264;
			}
		}
		break;

		case VideoFormatEnum::Hevc:
			printf("AmlVideoSink - VIDEO/HEVC\n");

			codecContext.video_type = VFORMAT_HEVC;
			codecContext.am_sysinfo.format = VIDEO_DEC_FORMAT_HEVC;
			codecContext.am_sysinfo.param = (void*)(EXTERNAL_PTS | SYNC_OUTSIDE);
			break;


		case VideoFormatEnum::VC1:
			printf("AmlVideoSink - VIDEO/VC1\n");
			codecContext.video_type = VFORMAT_VC1;
			codecContext.am_sysinfo.format = VIDEO_DEC_FORMAT_WVC1;
			break;

		default:
			printf("AmlVideoSink - VIDEO/UNKNOWN(%d)\n", (int)videoFormat);
			throw NotSupportedException();
		}


		// Rotation
		//codecContext.am_sysinfo.param = (void*)((unsigned long)(codecContext.am_sysinfo.param) | 0x10000); //90
		//codecContext.am_sysinfo.param = (void*)((unsigned long)(codecContext.am_sysinfo.param) | 0x20000); //180
		//codecContext.am_sysinfo.param = (void*)((unsigned long)(codecContext.am_sysinfo.param) | 0x30000); //270


		// Debug info
		printf("\tw=%d h=%d ", width, height);

		printf("fps=%f ", frameRate);

		printf("am_sysinfo.rate=%d ",
			codecContext.am_sysinfo.rate);

		printf("\n");


		// Intialize the hardware codec
		int api = codec_init(&codecContext);
		if (api != 0)
		{
			printf("codec_init failed (%x).\n", api);
			throw Exception();
		}


		// This is needed because the codec remains paused
		// even after closing
		int ret = codec_resume(&codecContext);

		//codec_reset(&codecContext);

		//api = codec_set_syncenable(&codecContext, 1);
		//if (api != 0)
		//{
		//	printf("codec_set_syncenable failed (%x).\n", api);
		//	exit(1);
		//}

		WriteToFile("/sys/class/graphics/fb0/blank", "1");
		WriteToFile("/sys/module/amvdec_h265/parameters/dynamic_buf_num_margin", "16");
	}

	void ProcessBuffer(AVPacketBufferSPTR buffer)
	{
		playPauseMutex.Lock();

		if (doResumeFlag)
		{
			codec_resume(&codecContext);
			doResumeFlag = false;
		}


		//AVPacketBufferPTR buffer = std::static_pointer_cast<AVPacketBuffer>(buf);
		AVPacket* pkt = buffer->GetAVPacket();


		unsigned char* nalHeader = (unsigned char*)pkt->data;

#if 0
		printf("Header (pkt.size=%x):\n", pkt.size);
		for (int j = 0; j < 16; ++j)	//nalHeaderLength
		{
			printf("%02x ", nalHeader[j]);
		}
		printf("\n");
#endif

		if (isFirstVideoPacket)
		{
			printf("Header (pkt.size=%x):\n", pkt->size);
			for (int j = 0; j < 16; ++j)	//nalHeaderLength
			{
				printf("%02x ", nalHeader[j]);
			}
			printf("\n");

			if (nalHeader[0] == 0 && nalHeader[1] == 0 &&
				nalHeader[2] == 0 && nalHeader[3] == 1)
			{
				isAnnexB = true;
			}

			isFirstVideoPacket = false;

			printf("isAnnexB=%u\n", isAnnexB);

			//if (videoPin->InfoAs()->Format == VideoFormatEnum::Mpeg4V3)
			//{
			//	//Divx3Header(videoPin->InfoAs()->Width, videoPin->InfoAs()->Height);
			//	//SendCodecData(0, &videoExtraData[0], videoExtraData.size());
			//}
		}


		unsigned long pts = 0;
#if 1
		if (pkt->pts != AV_NOPTS_VALUE)
		{
			double timeStamp = av_q2d(buffer->TimeBase()) * pkt->pts;
			pts = (unsigned long)(timeStamp * PTS_FREQ);

			estimatedNextPts = pkt->pts + pkt->duration;
			lastTimeStamp = timeStamp;
		}
		else
		{
			//printf("WARNING: AV_NOPTS_VALUE for codec_checkin_pts (duration=%x).\n", pkt->duration);

			//// This happens for containers like AVI
			//if (pkt->duration > 0)
			//{
			//	// Estimate PTS
			//	double timeStamp = av_q2d(buffer->TimeBase()) * estimatedNextPts;
			//	pts = (unsigned long)(timeStamp * PTS_FREQ);

			//	estimatedNextPts += pkt->duration;
			//	lastTimeStamp = timeStamp;

			//	//printf("WARNING: Estimated PTS for codec_checkin_pts (timeStamp=%f).\n", timeStamp);
			//}
		}

#else

		//pts = (unsigned long)(buffer->TimeStamp() * PTS_FREQ);

#endif

		isExtraDataSent = false;

		if (isAnnexB)
		{
			SendCodecData(pts, pkt->data, pkt->size);
		}
		else if (!isAnnexB &&
			(videoFormat == VideoFormatEnum::Avc ||
			 videoFormat == VideoFormatEnum::Hevc))
		{
			//unsigned char* nalHeader = (unsigned char*)pkt.data;

			// Five least significant bits of first NAL unit byte signify nal_unit_type.
			int nal_unit_type;
			const int nalHeaderLength = 4;

			while (nalHeader < (pkt->data + pkt->size))
			{
				switch (videoFormat)
				{
					case VideoFormatEnum::Avc:
					//if (!isExtraDataSent)
					{
						// Copy AnnexB data if NAL unit type is 5
						nal_unit_type = nalHeader[nalHeaderLength] & 0x1F;

						if (nal_unit_type == 5)
						{
							ConvertH264ExtraDataToAnnexB();

							SendCodecData(pts, &videoExtraData[0], videoExtraData.size());
						}

						isExtraDataSent = true;
					}
					break;

				case VideoFormatEnum::Hevc:
					//if (!isExtraDataSent)
					{
						nal_unit_type = (nalHeader[nalHeaderLength] >> 1) & 0x3f;

						/* prepend extradata to IRAP frames */
						if (nal_unit_type >= 16 && nal_unit_type <= 23)
						{
							HevcExtraDataToAnnexB();

							SendCodecData(0, &videoExtraData[0], videoExtraData.size());
						}

						isExtraDataSent = true;
					}
					break;

				}


				// Overwrite header NAL length with startcode '0x00000001' in *BigEndian*
				int nalLength = nalHeader[0] << 24;
				nalLength |= nalHeader[1] << 16;
				nalLength |= nalHeader[2] << 8;
				nalLength |= nalHeader[3];

				if (nalLength < 0 || nalLength > pkt->size)
				{
					printf("Invalid NAL length=%d, pkt->size=%d\n", nalLength, pkt->size);
					throw Exception();
				}

				nalHeader[0] = 0;
				nalHeader[1] = 0;
				nalHeader[2] = 0;
				nalHeader[3] = 1;

				//unsigned char* temp[nalLength + 3];
				//memcpy(temp, nalHeader, nalLength + 3);

				//if (nalHeader == pkt->data)
				//{
				//	SendCodecData(pts, (unsigned char*)temp, nalLength + 3);
				//}
				//else
				//{
				//	SendCodecData(0, (unsigned char*)temp, nalLength + 3);
				//}

				nalHeader += nalLength + 4;
			}

			SendCodecData(pts, pkt->data, pkt->size);
			
			//isExtraDataSent = false;
		}
		else if (videoPin->InfoAs()->Format == VideoFormatEnum::Mpeg4V3)
		{
			//printf("Sending Divx3\n");
			Divx3Header(videoPin->InfoAs()->Width, videoPin->InfoAs()->Height, pkt->size);
			SendCodecData(pts, &videoExtraData[0], videoExtraData.size());

			SendCodecData(0, pkt->data, pkt->size);
		}
		else
		{
			SendCodecData(pts, pkt->data, pkt->size);
		}

#if 0
		if (!isAnnexB)
		{
			unsigned char* data = (unsigned char*)pkt->data;

			printf("Post AnnexB Header (pkt.size=%x):\n", pkt->size);
			for (int j = 0; j < 16; ++j)	//nalHeaderLength
			{
				printf("%02x ", data[j]);
			}
			printf("\n");
		}
#endif

		if (doPauseFlag)
		{
			codec_pause(&codecContext);
			doPauseFlag = false;
		}

		playPauseMutex.Unlock();
	}
	
	void SendCodecData(unsigned long pts, unsigned char* data, int length)
	{
		//printf("AmlVideoSink: SendCodecData - pts=%lu, data=%p, length=0x%x\n", pts, data, length);

		int api;

		if (pts > 0)
		{
			if (codec_checkin_pts(&codecContext, pts))
			{
				printf("codec_checkin_pts failed\n");
			}
		}


		int offset = 0;
		//bool rePauseFlag = false;
		//int remainingAttempts = 10;
		while (api == -EAGAIN || offset < length)
		{
			api = codec_write(&codecContext, data + offset, length - offset);
			if (api < 0)
			{
				//printf("codec_write error: %x\n", api);
				//codec_reset(&codecContext);

				//// Handle the case where the codec buffer is full but
				//// pause is requested and this routine needs to exit
				//if (State() == MediaState::Pause)
				//{
				//	rePauseFlag = true;
				//	codec_resume(&codecContext);

				//	if (remainingAttempts <= 0)
				//	{
				//		// This likely a seek condition and the buffer
				//		// will be flushed.
				//		break;
				//	}

				//	--remainingAttempts;
				//}
			}
			else
			{
				offset += api;
				//printf("codec_write send %x bytes of %x total.\n", api, pkt.size);
			}

			// Testing has show that for 4K video, this loop can NOT sleep without
			// disrupting playback
			//usleep(1);
		}

		//if (rePauseFlag)
		//{
		//	codec_pause(&codecContext);
		//}

		//if (clockPts > 0)
		//{
		//	int codecCall = codec_set_pcrscr(&codecContext, (int)clockPts);
		//	if (codecCall != 0)
		//	{
		//		printf("codec_set_pcrscr failed.\n");
		//	}

		//	long delta = (long)pts - (long)clockPts;
		//	Log("pts=%lu, clockPts=%lu (%ld=%f)\n", pts, clockPts, delta, delta / (double)PTS_FREQ);
		//}
	}

	//void ProcessClockBuffer(BufferSPTR buffer)
	//{
	//	clock = buffer->TimeStamp();
	//	unsigned long pts = (unsigned long)(buffer->TimeStamp() * PTS_FREQ);


	//	int vpts = codec_get_vpts(&codecContext);

	//	if (clock < vpts * (double)PTS_FREQ)
	//	{
	//		clock = vpts * (double)PTS_FREQ;
	//	}



	//	int drift = vpts - pts;
	//	double driftTime = drift / (double)PTS_FREQ;
	//	double driftFrames = driftTime * (double)videoPin->InfoAs()->FrameRate;
	//	//printf("Clock drift = %f (frames=%f)\n", driftTime, driftFrames);

	//	clockPts = pts;
	//	lastClockPts = clockPts;
	//	//printf("AmlVideoSinkElement: clock=%f, pts=%lu, clockPts=%lu\n", buffer->TimeStamp(), pts, clockPts);

	//	// To minimize clock jitter, only adjust the clock if it
	//	// deviates more than +/- 2 frames
	//	if (driftFrames >= 2.0 || driftFrames <= -2.0)
	//	{
	//		if (clockPts > 0)
	//		{
	//			int codecCall = codec_set_pcrscr(&codecContext, (int)clockPts);
	//			if (codecCall != 0)
	//			{
	//				printf("codec_set_pcrscr failed.\n");
	//			}

	//			long delta = (long)pts - (long)clockPts;
	//			printf("AmlVideoSink: codec_set_pcrscr - pts=%lu\n", pts);
	//		}
	//	}
	//	
	//}

	//void ClockWorkThread()
	//{
	//	while (ExecutionState() != ExecutionStateEnum::Terminating)
	//	{
	//		while (ExecutionState() == ExecutionStateEnum::Executing)
	//		{
	//			// Clock
	//			BufferSPTR buffer;
	//			while (clockInPin->TryGetFilledBuffer(&buffer))
	//			{
	//				ProcessClockBuffer(buffer);

	//				// delay updates
	//				//usleep(1000);

	//				clockInPin->PushProcessedBuffer(buffer);
	//				clockInPin->ReturnProcessedBuffers();
	//			}

	//			usleep(1);
	//		}

	//		usleep(1);
	//	}
	//}


protected:

	//virtual void Idling() override
	//{
	//	//int ret = codec_pause(&codecContext);
	//	printf("AmlVideoSinkElement: Idling.\n");
	//}

	//virtual void Idled() override
	//{
	//	//int ret = codec_resume(&codecContext);
	//	printf("AmlVideoSinkElement: Idled.\n");
	//}

	
public:

	double Clock() 
	{
		//return clock;
		//return clockInPin->Clock();

		int vpts = codec_get_vpts(&codecContext);

		return vpts / (double)PTS_FREQ;
	}



	virtual void Initialize() override
	{
		ClearOutputPins();
		ClearInputPins();

		// TODO: Pin format negotiation

		{
			// Create a video pin
			VideoPinInfoSPTR info = std::make_shared<VideoPinInfo>();
			info->Format = VideoFormatEnum::Unknown;
			info->FrameRate = 0;

			ElementWPTR weakPtr = shared_from_this();
			videoPin = std::make_shared<VideoInPin>(weakPtr, info);
			AddInputPin(videoPin);
		}

		{
			// Create a clock pin
			PinInfoSPTR info = std::make_shared<PinInfo>(MediaCategoryEnum::Clock);

			ElementWPTR weakPtr = shared_from_this();
			clockInPin = std::make_shared<AmlVideoSinkClockInPin>(weakPtr, info, &codecContext);
			AddInputPin(clockInPin);			
		}


		// Event handlers
		timerExpiredListener = std::make_shared<EventListener<EventArgs>>(
			std::bind(&AmlVideoSinkElement::timer_Expired, this, std::placeholders::_1, std::placeholders::_2));

		timer.Expired.AddListener(timerExpiredListener);
		timer.SetInterval(0.25f);
		timer.Start();
	}

	virtual void DoWork() override
	{
		// TODO: Refactor to thread each input pin
		
		// TODO: Investigate merging PushProcessedBuffer and ReturnProcessedBuffer
		//	     into the latter.



		BufferSPTR buffer;
		
		//if (isEndOfStream)
		//{
		//	buf_status bufferStatus;
		//	int api = codec_get_vbuf_state(&codecContext, &bufferStatus);
		//	if (api == 0)
		//	{
		//		printf("AmlVideoSinkElement: codec_get_vbuf_state free_len=%d, size=%d, data_len=%d, read_pointer=%x, write_pointer=%x\n",
		//			bufferStatus.free_len, bufferStatus.size, bufferStatus.data_len, bufferStatus.read_pointer, bufferStatus.write_pointer);

		//		//if (bufferStatus.free_len == bufferStatus.size)
		//		if (bufferStatus.data_len <= 512)
		//		{
		//			SetState(MediaState::Pause);
		//			//isEndOfStream = false;
		//		}
		//	}
		//}

		if (videoPin->TryPeekFilledBuffer(&buffer))
		{
			AVPacketBufferSPTR avPacketBuffer = std::static_pointer_cast<AVPacketBuffer>(buffer);

			// Video
			if (videoPin->TryGetFilledBuffer(&buffer))
			{
				if (isFirstData)
				{
					OutPinSPTR otherPin = videoPin->Source();
					if (otherPin)
					{
						if (otherPin->Info()->Category() != MediaCategoryEnum::Video)
						{
							throw InvalidOperationException("AmlVideoSink: Not connected to a video pin.");
						}

						VideoPinInfoSPTR info = std::static_pointer_cast<VideoPinInfo>(otherPin->Info());
						videoFormat = info->Format;
						//frameRate = info->FrameRate;
						extraData = *(info->ExtraData);

						// TODO: This information should be copied
						//       as part of pin negotiation
						videoPin->InfoAs()->Format = info->Format;
						videoPin->InfoAs()->Width = info->Width;
						videoPin->InfoAs()->Height = info->Height;
						videoPin->InfoAs()->FrameRate = info->FrameRate;
						videoPin->InfoAs()->ExtraData = info->ExtraData;
						videoPin->InfoAs()->HasEstimatedPts = info->HasEstimatedPts;

						clockInPin->SetFrameRate(info->FrameRate);

						printf("AmlVideoSink: ExtraData size=%ld\n", extraData.size());

						SetupHardware();

						//clockThread.Start();

						isFirstData = false;
					}
				}


				switch (buffer->Type())
				{
					case BufferTypeEnum::Marker:
					{
						MarkerBufferSPTR markerBuffer = std::static_pointer_cast<MarkerBuffer>(buffer);
						printf("AmlVideoSinkElement: got marker buffer Marker=%d\n", (int)markerBuffer->Marker());

						switch (markerBuffer->Marker())
						{
						case MarkerEnum::EndOfStream:
							//SetExecutionState(ExecutionStateEnum::Idle);

							// TODO: This causes the codec to pause
							//  even though there are still buffers in
							//  the kernel driver being processed
							//SetState(MediaState::Pause);
							isEndOfStream = true;
							break;

						case MarkerEnum::Discontinue:
							//codec_reset(&codecContext);
							break;

						//case MarkerEnum::Pause:
						//	SetState(MediaState::Pause);
						//	break;

						default:
							// ignore unknown 
							break;
						}

						break;
					}

					case BufferTypeEnum::AVPacket:
					{
						//printf("AmlVideoSink: Got a buffer.\n");

						ProcessBuffer(avPacketBuffer);
						break;
					}
				}

				videoPin->PushProcessedBuffer(buffer);
				videoPin->ReturnProcessedBuffers();
			}

			//if (ExecutionState() != ExecutionStateEnum::Executing)
			//	break;
		}
	}

	virtual void ChangeState(MediaState oldState, MediaState newState) override
	{
		// TODO: pause video

		Element::ChangeState(oldState, newState);

		//if (ExecutionState() == ExecutionStateEnum::Executing)
		{
			switch (newState)
			{
			case MediaState::Play:
			{
				playPauseMutex.Lock();

				int ret = codec_resume(&codecContext);
				//doPauseFlag = false;
				//doResumeFlag = true;
				isEndOfStream = false;
				//timer.Start();

				playPauseMutex.Unlock();
				break;
			}

			case MediaState::Pause:
			{
				playPauseMutex.Lock();

				//doResumeFlag = false;
				//doPauseFlag = true;
				int ret = codec_pause(&codecContext);
				//timer.Stop();

				playPauseMutex.Unlock();
				break;
			}

			default:
				break;
			}
		}
	}


	virtual void Flush() override
	{
		Element::Flush();

		
		//codec_set_dec_reset(&codecContext);
		codec_reset(&codecContext);
	}

private:
	static void WriteToFile(const char* path, const char* value)
	{
		int fd = open(path, O_RDWR | O_TRUNC, 0644);
		if (fd < 0)
		{
			printf("WriteToFile open failed: %s = %s\n", path, value);
			throw Exception();
		}

		if (write(fd, value, strlen(value)) < 0)
		{
			printf("WriteToFile write failed: %s = %s\n", path, value);
			throw Exception();
		}

		close(fd);
	}

	void Divx3Header(int width, int height, int packetSize)
	{
		// Bitstream info from Kodi

		videoExtraData.clear();

		videoExtraData.push_back(0x00);
		videoExtraData.push_back(0x00);
		videoExtraData.push_back(0x00);
		videoExtraData.push_back(0x01);

		unsigned i = (width << 12) | (height & 0xfff);
		videoExtraData.push_back(0x20);
		videoExtraData.push_back((i >> 16) & 0xff);
		videoExtraData.push_back((i >> 8) & 0xff);
		videoExtraData.push_back(i & 0xff);
		videoExtraData.push_back(0x00);
		videoExtraData.push_back(0x00);

		const unsigned char divx311_chunk_prefix[] =
		{
			0x00, 0x00, 0x00, 0x01,
			0xb6, 'D', 'I', 'V', 'X', '3', '.', '1', '1'
		};

		for (size_t i = 0; i < sizeof(divx311_chunk_prefix); ++i)
		{
			videoExtraData.push_back(divx311_chunk_prefix[i]);
		}

		videoExtraData.push_back((packetSize >> 24) & 0xff);
		videoExtraData.push_back((packetSize >> 16) & 0xff);
		videoExtraData.push_back((packetSize >> 8) & 0xff);
		videoExtraData.push_back(packetSize & 0xff);
	}

	void ConvertH264ExtraDataToAnnexB()
	{
		void* video_extra_data = &extraData[0];
		int video_extra_data_size = extraData.size();

		videoExtraData.clear();

		if (video_extra_data_size > 0)
		{
			unsigned char* extraData = (unsigned char*)video_extra_data;

			// http://aviadr1.blogspot.com/2010/05/h264-extradata-partially-explained-for.html

			const int spsStart = 6;
			int spsLen = extraData[spsStart] * 256 + extraData[spsStart + 1];

			videoExtraData.push_back(0);
			videoExtraData.push_back(0);
			videoExtraData.push_back(0);
			videoExtraData.push_back(1);

			for (int i = 0; i < spsLen; ++i)
			{
				videoExtraData.push_back(extraData[spsStart + 2 + i]);
			}


			int ppsStart = spsStart + 2 + spsLen + 1; // 2byte sbs len, 1 byte pps start code
			int ppsLen = extraData[ppsStart] * 256 + extraData[ppsStart + 1];

			videoExtraData.push_back(0);
			videoExtraData.push_back(0);
			videoExtraData.push_back(0);
			videoExtraData.push_back(1);

			for (int i = 0; i < ppsLen; ++i)
			{
				videoExtraData.push_back(extraData[ppsStart + 2 + i]);
			}

		}

#if 0
		printf("EXTRA DATA = ");

		for (int i = 0; i < videoExtraData.size(); ++i)
		{
			printf("%02x ", videoExtraData[i]);

		}

		printf("\n");
#endif
	}

	void HevcExtraDataToAnnexB()
	{
		//void* video_extra_data = ExtraData();
		//int video_extra_data_size = ExtraDataSize();
		void* video_extra_data = &extraData[0];
		int video_extra_data_size = extraData.size();


		videoExtraData.clear();

		if (video_extra_data_size > 0)
		{
			unsigned char* extraData = (unsigned char*)video_extra_data;

			// http://fossies.org/linux/ffmpeg/libavcodec/hevc_mp4toannexb_bsf.c

			int offset = 21;
			int length_size = (extraData[offset++] & 3) + 1;
			int num_arrays = extraData[offset++];

			//printf("HevcExtraDataToAnnexB: length_size=%d, num_arrays=%d\n", length_size, num_arrays);


			for (int i = 0; i < num_arrays; i++)
			{
				int type = extraData[offset++] & 0x3f;
				int cnt = extraData[offset++] << 8 | extraData[offset++];

				for (int j = 0; j < cnt; j++)
				{
					videoExtraData.push_back(0);
					videoExtraData.push_back(0);
					videoExtraData.push_back(0);
					videoExtraData.push_back(1);

					int nalu_len = extraData[offset++] << 8 | extraData[offset++];
					for (int k = 0; k < nalu_len; ++k)
					{
						videoExtraData.push_back(extraData[offset++]);
					}
				}
			}

#if 0
			printf("EXTRA DATA = ");

			for (int i = 0; i < videoExtraData.size(); ++i)
			{
				printf("%02x ", videoExtraData[i]);
			}

			printf("\n");
#endif
		}
	}

};

typedef std::shared_ptr<AmlVideoSinkElement> AmlVideoSinkElementSPTR;



//class AmlVideoSink : public Sink, public virtual IClockSink
//{
//	const unsigned long PTS_FREQ = 90000;
//	const long EXTERNAL_PTS = (1);
//	const long SYNC_OUTSIDE = (2);
//
//	std::vector<unsigned char> videoExtraData;
//	AVCodecID codec_id;
//	double frameRate;
//	codec_para_t codecContext;
//	void* extraData = nullptr;
//	int extraDataSize = 0;
//	AVRational time_base;
//	double lastTimeStamp = -1;
//
//public:
//
//	void* ExtraData() const
//	{
//		return extraData;
//	}
//	void SetExtraData(void* value)
//	{
//		extraData = value;
//	}
//
//	int ExtraDataSize() const
//	{
//		return extraDataSize;
//	}
//	void SetExtraDataSize(int value)
//	{
//		extraDataSize = value;
//	}
//
//	double GetLastTimeStamp() const
//	{
//		return lastTimeStamp;
//	}
//
//
//
//	AmlVideoSink(AVCodecID codec_id, double frameRate, AVRational time_base)
//		: Sink(),
//		codec_id(codec_id),
//		frameRate(frameRate),
//		time_base(time_base)
//	{
//		memset(&codecContext, 0, sizeof(codecContext));
//
//		codecContext.stream_type = STREAM_TYPE_ES_VIDEO;
//		codecContext.has_video = 1;
//		codecContext.am_sysinfo.param = (void*)(0 ); // EXTERNAL_PTS | SYNC_OUTSIDE
//		codecContext.am_sysinfo.rate = 96000.5 * (1.0 / frameRate);
//
//		switch (codec_id)
//		{
//		case CODEC_ID_MPEG2VIDEO:
//			printf("AmlVideoSink - VIDEO/MPEG2\n");
//
//			codecContext.video_type = VFORMAT_MPEG12;
//			codecContext.am_sysinfo.format = VIDEO_DEC_FORMAT_UNKNOW;
//			break;
//
//		case CODEC_ID_MPEG4:
//			printf("AmlVideoSink - VIDEO/MPEG4\n");
//
//			codecContext.video_type = VFORMAT_MPEG4;
//			codecContext.am_sysinfo.format = VIDEO_DEC_FORMAT_MPEG4_5;
//			//VIDEO_DEC_FORMAT_MP4; //VIDEO_DEC_FORMAT_MPEG4_3; //VIDEO_DEC_FORMAT_MPEG4_4; //VIDEO_DEC_FORMAT_MPEG4_5;
//			break;
//
//		case CODEC_ID_H264:
//		{
//			printf("AmlVideoSink - VIDEO/H264\n");
//
//			codecContext.video_type = VFORMAT_H264_4K2K;
//			codecContext.am_sysinfo.format = VIDEO_DEC_FORMAT_H264_4K2K;
//		}
//		break;
//
//		case AV_CODEC_ID_HEVC:
//			printf("AmlVideoSink - VIDEO/HEVC\n");
//
//			codecContext.video_type = VFORMAT_HEVC;
//			codecContext.am_sysinfo.format = VIDEO_DEC_FORMAT_HEVC;
//			break;
//
//
//		//case CODEC_ID_VC1:
//		//	printf("stream #%d - VIDEO/VC1\n", i);
//		//	break;
//
//		default:
//			printf("AmlVideoSink - VIDEO/UNKNOWN(%x)\n", codec_id);
//			throw NotSupportedException();
//		}
//
//		printf("\tfps=%f ", frameRate);
//
//		printf("am_sysinfo.rate=%d ",
//			codecContext.am_sysinfo.rate);
//
//
//		//int width = codecCtxPtr->width;
//		//int height = codecCtxPtr->height;
//
//		//printf("SAR=(%d/%d) ",
//		//	streamPtr->sample_aspect_ratio.num,
//		//	streamPtr->sample_aspect_ratio.den);
//
//		// TODO: DAR
//
//		printf("\n");
//
//	
//
//
//		int api = codec_init(&codecContext);
//		if (api != 0)
//		{
//			printf("codec_init failed (%x).\n", api);
//			exit(1);
//		}
//
//		//api = codec_set_syncenable(&codecContext, 1);
//		//if (api != 0)
//		//{
//		//	printf("codec_set_syncenable failed (%x).\n", api);
//		//	exit(1);
//		//}
//
//
//		WriteToFile("/sys/class/graphics/fb0/blank", "1");
//	}
//
//	~AmlVideoSink()
//	{
//		if (IsRunning())
//		{
//			Stop();
//		}
//
//		codec_close(&codecContext);
//
//		WriteToFile("/sys/class/graphics/fb0/blank", "0");
//	}
//
//
//
//	// Inherited via IClockSink
//	virtual void SetTimeStamp(double value) override
//	{
//		unsigned long pts = (unsigned long)(value * PTS_FREQ);
//
//#if 0
//		int vpts = codec_get_vpts(&codecContext);
//		int drift = vpts - pts;
//
//		printf("Clock drift = %f\n", drift / (double)PTS_FREQ);
//#endif
//
//		int codecCall = codec_set_pcrscr(&codecContext, (int)pts);
//		if (codecCall != 0)
//		{
//			printf("codec_set_pcrscr failed.\n");
//		}
//	}
//
//
//	virtual void Flush() override
//	{
//		Sink::Flush();
//
//		codec_reset(&codecContext);
//	}
//
//	// Member
//private:
//	void ConvertH264ExtraDataToAnnexB()
//	{
//		void* video_extra_data = ExtraData();
//		int video_extra_data_size = ExtraDataSize();
//
//		videoExtraData.clear();
//
//		if (video_extra_data_size > 0)
//		{
//			unsigned char* extraData = (unsigned char*)video_extra_data;
//
//			// http://aviadr1.blogspot.com/2010/05/h264-extradata-partially-explained-for.html
//
//			const int spsStart = 6;
//			int spsLen = extraData[spsStart] * 256 + extraData[spsStart + 1];
//
//			videoExtraData.push_back(0);
//			videoExtraData.push_back(0);
//			videoExtraData.push_back(0);
//			videoExtraData.push_back(1);
//
//			for (int i = 0; i < spsLen; ++i)
//			{
//				videoExtraData.push_back(extraData[spsStart + 2 + i]);
//			}
//
//
//			int ppsStart = spsStart + 2 + spsLen + 1; // 2byte sbs len, 1 byte pps start code
//			int ppsLen = extraData[ppsStart] * 256 + extraData[ppsStart + 1];
//
//			videoExtraData.push_back(0);
//			videoExtraData.push_back(0);
//			videoExtraData.push_back(0);
//			videoExtraData.push_back(1);
//
//			for (int i = 0; i < ppsLen; ++i)
//			{
//				videoExtraData.push_back(extraData[ppsStart + 2 + i]);
//			}
//
//		}
//
//#if 0
//		printf("EXTRA DATA = ");
//
//		for (int i = 0; i < videoExtraData.size(); ++i)
//		{
//			printf("%02x ", videoExtraData[i]);
//
//		}
//
//		printf("\n");
//#endif
//	}
//
//	void HevcExtraDataToAnnexB()
//	{
//		void* video_extra_data = ExtraData();
//		int video_extra_data_size = ExtraDataSize();
//
//
//		videoExtraData.clear();
//
//		if (video_extra_data_size > 0)
//		{
//			unsigned char* extraData = (unsigned char*)video_extra_data;
//
//			// http://fossies.org/linux/ffmpeg/libavcodec/hevc_mp4toannexb_bsf.c
//
//			int offset = 21;
//			int length_size = (extraData[offset++] & 3) + 1;
//			int num_arrays = extraData[offset++];
//
//			//printf("HevcExtraDataToAnnexB: length_size=%d, num_arrays=%d\n", length_size, num_arrays);
//
//
//			for (int i = 0; i < num_arrays; i++)
//			{
//				int type = extraData[offset++] & 0x3f;
//				int cnt = extraData[offset++] << 8 | extraData[offset++];
//
//				for (int j = 0; j < cnt; j++)
//				{
//					videoExtraData.push_back(0);
//					videoExtraData.push_back(0);
//					videoExtraData.push_back(0);
//					videoExtraData.push_back(1);
//
//					int nalu_len = extraData[offset++] << 8 | extraData[offset++];
//					for (int k = 0; k < nalu_len; ++k)
//					{
//						videoExtraData.push_back(extraData[offset++]);
//					}
//				}
//			}
//
//#if 0
//			printf("EXTRA DATA = ");
//
//			for (int i = 0; i < videoExtraData.size(); ++i)
//			{
//				printf("%02x ", videoExtraData[i]);
//			}
//
//			printf("\n");
//#endif
//		}
//	}
//
//	static void WriteToFile(const char* path, const char* value)
//	{
//		int fd = open(path, O_RDWR | O_TRUNC, 0644);
//		if (fd < 0)
//		{
//			printf("WriteToFile open failed: %s = %s\n", path, value);
//			throw Exception();
//		}
//
//		if (write(fd, value, strlen(value)) < 0)
//		{
//			printf("WriteToFile write failed: %s = %s\n", path, value);
//			throw Exception();
//		}
//
//		close(fd);
//	}
//
//
//protected:
//	virtual void WorkThread() override
//	{
//		printf("AmlVideoSink entering running state.\n");
//
//
//		bool isFirstVideoPacket = true;
//		bool isAnnexB = false;
//		bool isExtraDataSent = false;
//		long estimatedNextPts = 0;
//		MediaState lastState = State();
//
//
//		codec_resume(&codecContext);
//
//		while (IsRunning())
//		{
//			MediaState state = State();
//
//			if (state != MediaState::Play)
//			{
//				if (lastState == MediaState::Play)
//				{
//					int ret = codec_pause(&codecContext);
//				}
//
//				usleep(1);
//			}
//			else
//			{
//				if (lastState == MediaState::Pause)
//				{
//					int ret = codec_resume(&codecContext);
//				}
//
//				AVPacketBufferPtr buffer;
//
//				if (!TryGetBuffer(&buffer))
//				{
//					usleep(1);
//				}
//				else
//				{
//					AVPacket* pkt = buffer->GetAVPacket();
//
//
//					unsigned char* nalHeader = (unsigned char*)pkt->data;
//
//#if 0
//					printf("Header (pkt.size=%x):\n", pkt.size);
//					for (int j = 0; j < 16; ++j)	//nalHeaderLength
//					{
//						printf("%02x ", nalHeader[j]);
//					}
//					printf("\n");
//#endif
//
//					if (isFirstVideoPacket)
//					{
//						printf("Header (pkt.size=%x):\n", pkt->size);
//						for (int j = 0; j < 16; ++j)	//nalHeaderLength
//						{
//							printf("%02x ", nalHeader[j]);
//						}
//						printf("\n");
//
//
//						//// DEBUG
//						//printf("Codec ExtraData=%p ExtraDataSize=%x\n", video_extra_data, video_extra_data_size);
//						//
//						//unsigned char* ptr = (unsigned char*)video_extra_data;
//						//printf("ExtraData :\n");
//						//for (int j = 0; j < video_extra_data_size; ++j)
//						//{					
//						//	printf("%02x ", ptr[j]);
//						//}
//						//printf("\n");
//						//
//
//						//int extraDataCall = codec_write(&codecContext, video_extra_data, video_extra_data_size);
//						//if (extraDataCall == -EAGAIN)
//						//{
//						//	printf("ExtraData codec_write failed.\n");
//						//}
//						////
//
//
//						if (nalHeader[0] == 0 && nalHeader[1] == 0 &&
//							nalHeader[2] == 0 && nalHeader[3] == 1)
//						{
//							isAnnexB = true;
//						}
//
//						isFirstVideoPacket = false;
//
//						printf("isAnnexB=%u\n", isAnnexB);
//					}
//
//
//					if (!isAnnexB &&
//						(codec_id == CODEC_ID_H264 ||
//						 codec_id == AV_CODEC_ID_HEVC))
//					{
//						//unsigned char* nalHeader = (unsigned char*)pkt.data;
//
//						// Five least significant bits of first NAL unit byte signify nal_unit_type.
//						int nal_unit_type;
//						const int nalHeaderLength = 4;
//
//						while (nalHeader < (pkt->data + pkt->size))
//						{
//							switch (codec_id)
//							{
//							case CODEC_ID_H264:
//								//if (!isExtraDataSent)
//							{
//								// Copy AnnexB data if NAL unit type is 5
//								nal_unit_type = nalHeader[nalHeaderLength] & 0x1F;
//
//								if (nal_unit_type == 5)
//								{
//									ConvertH264ExtraDataToAnnexB();
//
//									int api = codec_write(&codecContext, &videoExtraData[0], videoExtraData.size());
//									if (api <= 0)
//									{
//										printf("extra data codec_write error: %x\n", api);
//									}
//									else
//									{
//										//printf("extra data codec_write OK\n");
//									}
//								}
//
//								isExtraDataSent = true;
//							}
//							break;
//
//							case AV_CODEC_ID_HEVC:
//								//if (!isExtraDataSent)
//							{
//								nal_unit_type = (nalHeader[nalHeaderLength] >> 1) & 0x3f;
//
//								/* prepend extradata to IRAP frames */
//								if (nal_unit_type >= 16 && nal_unit_type <= 23)
//								{
//									HevcExtraDataToAnnexB();
//
//									int attempts = 10;
//									int api;
//									while (attempts > 0)
//									{
//										api = codec_write(&codecContext, &videoExtraData[0], videoExtraData.size());
//										if (api <= 0)
//										{
//											usleep(1);
//											--attempts;
//										}
//										else
//										{
//											//printf("extra data codec_write OK\n");
//											break;
//										}
//
//									}
//
//									if (attempts == 0)
//									{
//										printf("extra data codec_write error: %x\n", api);
//									}
//								}
//
//								isExtraDataSent = true;
//							}
//							break;
//
//							}
//
//
//							// Overwrite header NAL length with startcode '0x00000001' in *BigEndian*
//							int nalLength = nalHeader[0] << 24;
//							nalLength |= nalHeader[1] << 16;
//							nalLength |= nalHeader[2] << 8;
//							nalLength |= nalHeader[3];
//
//							nalHeader[0] = 0;
//							nalHeader[1] = 0;
//							nalHeader[2] = 0;
//							nalHeader[3] = 1;
//
//							nalHeader += nalLength + 4;
//						}
//					}
//
//
//					if (pkt->pts != AV_NOPTS_VALUE)
//					{
//						double timeStamp = av_q2d(time_base) * pkt->pts;
//						unsigned long pts = (unsigned long)(timeStamp * PTS_FREQ);
//						//printf("pts = %lu, timeStamp = %f\n", pts, timeStamp);
//						if (codec_checkin_pts(&codecContext, pts))
//						{
//							printf("codec_checkin_pts failed\n");
//						}
//
//						estimatedNextPts = pkt->pts + pkt->duration;
//						lastTimeStamp = timeStamp;
//					}
//					else
//					{
//						//printf("WARNING: AV_NOPTS_VALUE for codec_checkin_pts (duration=%x).\n", pkt.duration);
//
//						if (pkt->duration > 0)
//						{
//							// Estimate PTS
//							double timeStamp = av_q2d(time_base) * estimatedNextPts;
//							unsigned long pts = (unsigned long)(timeStamp * PTS_FREQ);
//
//							if (codec_checkin_pts(&codecContext, pts))
//							{
//								printf("codec_checkin_pts failed\n");
//							}
//
//							estimatedNextPts += pkt->duration;
//							lastTimeStamp = timeStamp;
//
//							//printf("WARNING: Estimated PTS for codec_checkin_pts (timeStamp=%f).\n", timeStamp);
//						}
//					}
//
//
//					// Send the data to the codec
//					int api = 0;
//					int offset = 0;
//					do
//					{
//						api = codec_write(&codecContext, pkt->data + offset, pkt->size - offset);
//						if (api == -EAGAIN)
//						{
//							usleep(1);
//						}
//						else if (api == -1)
//						{
//							// TODO: Sometimes this error is returned.  Ignoring it
//							// does not seem to have any impact on video display
//						}
//						else if (api < 0)
//						{
//							printf("codec_write error: %x\n", api);
//							//codec_reset(&codecContext);
//						}
//						else if (api > 0)
//						{
//							offset += api;
//							//printf("codec_write send %x bytes of %x total.\n", api, pkt.size);
//						}
//
//					} while (api == -EAGAIN || offset < pkt->size);
//				}
//
//			}
//
//			lastState = state;
//		}
//
//		printf("AmlVideoSink exiting running state.\n");
//	}
//
//};
//
//
//typedef std::shared_ptr<AmlVideoSink> AmlVideoSinkPtr;
