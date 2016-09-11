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
#include <cstring>

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
#include "Timer.h"

//#include <signal.h>
//#include <time.h>
//#include <math.h>
//
//
//
//class Timer
//{
//	timer_t timer_id;
//
//
//	double interval = 0;
//	bool isRunning = false;
//
//
//	static void timer_thread(sigval arg)
//	{
//		Timer* timerPtr = (Timer*)arg.sival_ptr;
//		timerPtr->Expired.Invoke(timerPtr, EventArgs::Empty());
//
//		//printf("Timer: expired arg=%p.\n", arg.sival_ptr);
//	}
//
//public:
//	Event<EventArgs> Expired;
//
//
//	double Interval() const
//	{
//		return interval;
//	}
//	void SetInterval(double value)
//	{
//		interval = value;
//	}
//
//
//
//	Timer()
//	{
//		sigevent se = { 0 };
//		
//		se.sigev_notify = SIGEV_THREAD;
//		se.sigev_value.sival_ptr = &timer_id;
//		se.sigev_notify_function = &Timer::timer_thread;
//		se.sigev_notify_attributes = NULL;
//		se.sigev_value.sival_ptr = this;
//
//		int ret = timer_create(CLOCK_REALTIME, &se, &timer_id);
//		if (ret != 0)
//		{
//			throw Exception("Timer creation failed.");
//		}
//	}
//
//	~Timer()
//	{
//		timer_delete(timer_id);
//	}
//
//
//	void Start()
//	{
//		if (isRunning)
//			throw InvalidOperationException();
//
//
//		itimerspec ts = { 0 };
//
//		// arm value
//		ts.it_value.tv_sec = floor(interval);
//		ts.it_value.tv_nsec = floor((interval - floor(interval)) * 1e9);
//		
//		// re-arm value
//		ts.it_interval.tv_sec = ts.it_value.tv_sec;
//		ts.it_interval.tv_nsec = ts.it_value.tv_nsec;
//
//		int ret = timer_settime(timer_id, 0, &ts, 0);
//		if (ret != 0)
//		{
//			throw Exception("timer_settime failed.");
//		}
//
//		isRunning = true;
//	}
//
//	void Stop()
//	{
//		if (!isRunning)
//			throw InvalidOperationException();
//
//
//		itimerspec ts = { 0 };
//
//		int ret = timer_settime(timer_id, 0, &ts, 0);
//		if (ret != 0)
//		{
//			throw Exception("timer_settime failed.");
//		}
//
//		isRunning = false;
//	}
//};


class AmlCodec
{
	const unsigned long PTS_FREQ = 90000;

	const long EXTERNAL_PTS = (1);
	const long SYNC_OUTSIDE = (2);
	const long USE_IDR_FRAMERATE = 0x04;
	const long UCODE_IP_ONLY_PARAM = 0x08;
	const long MAX_REFER_BUF = 0x10;
	const long ERROR_RECOVERY_MODE_IN = 0x20;

	codec_para_t codec = { 0 };
	bool isOpen = false;
	Mutex codecMutex;


public:

	bool IsOpen() const
	{
		return isOpen;
	}


	void Open(VideoFormatEnum format, int width, int height, double frameRate)
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


		codec.stream_type = STREAM_TYPE_ES_VIDEO;
		codec.has_video = 1;
		//codec.noblock = 1;

		// Note: Without EXTERNAL_PTS | SYNC_OUTSIDE, the codec auto adjusts
		// frame-rate from PTS 
		codec.am_sysinfo.param = (void*)(EXTERNAL_PTS | SYNC_OUTSIDE); //USE_IDR_FRAMERATE

																	   // Note: Testing has shown that the ALSA clock requires the +1
		codec.am_sysinfo.rate = 96000.0 / frameRate + 1;


		switch (format)
		{
			case VideoFormatEnum::Mpeg2:
				printf("AmlVideoSink - VIDEO/MPEG2\n");

				codec.video_type = VFORMAT_MPEG12;
				codec.am_sysinfo.format = VIDEO_DEC_FORMAT_UNKNOW;
				break;

			case VideoFormatEnum::Mpeg4V3:
				printf("AmlVideoSink - VIDEO/MPEG4V3\n");

				codec.video_type = VFORMAT_MPEG4;
				codec.am_sysinfo.format = VIDEO_DEC_FORMAT_MPEG4_3;
				break;

			case VideoFormatEnum::Mpeg4:
				printf("AmlVideoSink - VIDEO/MPEG4\n");

				codec.video_type = VFORMAT_MPEG4;
				codec.am_sysinfo.format = VIDEO_DEC_FORMAT_MPEG4_5;
				break;

			case VideoFormatEnum::Avc:
			{
				if (width > 1920 || height > 1080)
				{
					printf("AmlVideoSink - VIDEO/H264_4K2K\n");

					codec.video_type = VFORMAT_H264_4K2K;
					codec.am_sysinfo.format = VIDEO_DEC_FORMAT_H264_4K2K;
				}
				else
				{
					printf("AmlVideoSink - VIDEO/H264\n");

					codec.video_type = VFORMAT_H264;
					codec.am_sysinfo.format = VIDEO_DEC_FORMAT_H264;
				}
			}
			break;

			case VideoFormatEnum::Hevc:
				printf("AmlVideoSink - VIDEO/HEVC\n");

				codec.video_type = VFORMAT_HEVC;
				codec.am_sysinfo.format = VIDEO_DEC_FORMAT_HEVC;
				break;


			case VideoFormatEnum::VC1:
				printf("AmlVideoSink - VIDEO/VC1\n");

				codec.video_type = VFORMAT_VC1;
				codec.am_sysinfo.format = VIDEO_DEC_FORMAT_WVC1;
				break;

			default:
				printf("AmlVideoSink - VIDEO/UNKNOWN(%d)\n", (int)format);

				codecMutex.Unlock();
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
			codec.am_sysinfo.rate);

		printf("\n");


		// Intialize the hardware codec
		int api = codec_init(&codec);
		//int api = codec_init_no_modeset(&codecContext);
		if (api != 0)
		{
			printf("codec_init failed (%x).\n", api);

			codecMutex.Unlock();
			throw Exception();
		}


		// This is needed because the codec remains paused
		// even after closing
		int ret = codec_resume(&codec);


		isOpen = true;
		codecMutex.Unlock();
	}

	void Close()
	{
		if (!isOpen)
			throw InvalidOperationException("The codec is not open.");


		codecMutex.Lock();

		codec_close(&codec);

		codecMutex.Unlock();
	}

	void Reset()
	{
		if (!isOpen)
			throw InvalidOperationException("The codec is not open.");


		codecMutex.Lock();

		codec_reset(&codec);

		codecMutex.Unlock();
	}

	double GetCurrentPts()
	{
		if (!isOpen)
			throw InvalidOperationException("The codec is not open.");


		codecMutex.Lock();

		int vpts = codec_get_vpts(&codec);

		codecMutex.Unlock();

		return vpts / (double)PTS_FREQ;
	}

	void SetCurrentPts(double value)
	{
		if (!isOpen)
			throw InvalidOperationException("The codec is not open.");


		codecMutex.Lock();

		unsigned long pts = value * PTS_FREQ;

		int codecCall = codec_set_pcrscr(&codec, (int)pts);
		if (codecCall != 0)
		{
			printf("codec_set_pcrscr failed.\n");
		}

		codecMutex.Unlock();
	}

	void Pause()
	{
		if (!isOpen)
			throw InvalidOperationException("The codec is not open.");


		codecMutex.Lock();
		codec_pause(&codec);
		codecMutex.Unlock();
	}

	void Resume()
	{
		if (!isOpen)
			throw InvalidOperationException("The codec is not open.");


		codecMutex.Lock();
		codec_resume(&codec);
		codecMutex.Unlock();
	}

	buf_status GetBufferStatus()
	{
		if (!isOpen)
			throw InvalidOperationException("The codec is not open.");

		throw NotImplementedException();
	}

	void SendData(unsigned long pts, unsigned char* data, int length)
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

			if (codec_checkin_pts(&codec, pts))
			{
				printf("codec_checkin_pts failed\n");
			}

			codecMutex.Unlock();
		}


		int offset = 0;
		while (api == -EAGAIN || offset < length)
		{
			//codecMutex.Lock();

			api = codec_write(&codec, data + offset, length - offset);

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

};



class AmlVideoSinkClockInPin : public InPin
{
	const unsigned long PTS_FREQ = 90000;

	//codec_para_t* codecContextPtr;
	AmlCodec* codecPTR;
	double clock = 0;
	double frameRate = 0;	// TODO just read info from Owner()


	void ProcessClockBuffer(BufferSPTR buffer)
	{
		clock = buffer->TimeStamp();
		unsigned long pts = (unsigned long)(buffer->TimeStamp() * PTS_FREQ);


		//int vpts = codec_get_vpts(codecContextPtr);
		int vpts = codecPTR->GetCurrentPts() * PTS_FREQ;

		if (clock < (vpts / (double)PTS_FREQ))
		{
			clock = (vpts / (double)PTS_FREQ);
		}


		int drift = vpts - pts;
		double driftTime = drift / (double)PTS_FREQ;
		double driftFrames = driftTime * frameRate;

		// To minimize clock jitter, only adjust the clock if it
		// deviates more than +/- 2 frames
		if (driftFrames >= 2.0 || driftFrames <= -2.0)
		{
			if (pts > 0)
			{
				//int codecCall = codec_set_pcrscr(codecContextPtr, (int)pts);
				//if (codecCall != 0)
				//{
				//	printf("codec_set_pcrscr failed.\n");
				//}
				codecPTR->SetCurrentPts(pts / (double)PTS_FREQ);

				printf("AmlVideoSink: codec_set_pcrscr - pts=%lu drift=%f (%f frames)\n", pts, driftTime, driftFrames);
			}
		}
	}

protected:

	virtual void DoWork() override
	{
		BufferSPTR buffer;
		if (TryGetFilledBuffer(&buffer))
		{
			ElementSPTR owner = Owner().lock();
			if (owner && owner->State() == MediaState::Play)
			{
				ProcessClockBuffer(buffer);
			}

			PushProcessedBuffer(buffer);
			ReturnProcessedBuffers();
		}
	}


public:

	//double Clock() const
	//{
	//	//return clock;
	//	int vpts = codec_get_vpts(codecContextPtr);
	//	
	//	return vpts / (double)PTS_FREQ;
	//}

	double FrameRate() const
	{
		return frameRate;
	}
	void SetFrameRate(double value)
	{
		frameRate = value;
	}



	AmlVideoSinkClockInPin(ElementWPTR owner, PinInfoSPTR info, AmlCodec* codecPTR)
		: InPin(owner, info),
		codecPTR(codecPTR)
	{
		if (codecPTR == nullptr)
			throw ArgumentNullException();
	}
};

typedef std::shared_ptr<AmlVideoSinkClockInPin> AmlVideoSinkClockInPinSPTR;



class AmlVideoSinkClockOutPin : public OutPin
{
	const unsigned long PTS_FREQ = 90000;

	codec_para_t* codecContextPtr;
	Timer timer;
	Mutex mutex;
	EventListenerSPTR<EventArgs> timerExpiredListener;


	void timer_Expired(void* sender, const EventArgs& args)
	{
		mutex.Lock();

		ElementSPTR element = Owner().lock();
		if (element)
		{
			if (element->State() == MediaState::Play)
			{
				BufferSPTR buffer;
				if (TryGetAvailableBuffer(&buffer))
				{
					if (buffer->Type() != BufferTypeEnum::ClockData)
						throw InvalidOperationException();

					ClockDataBufferSPTR clockBuffer = std::static_pointer_cast<ClockDataBuffer>(buffer);

					int vpts = codec_get_vpts(codecContextPtr);
					double clock = vpts / (double)PTS_FREQ;

					clockBuffer->SetTimeStamp(clock);
					SendBuffer(clockBuffer);

					//printf("AmlVideoSinkClockOutPin: TimeStamp=%f\n", clockBuffer->TimeStamp());
				}
			}
		}

		mutex.Unlock();
	}


protected:

	//virtual void DoWork() override
	//{

	//}

public:

	AmlVideoSinkClockOutPin(ElementWPTR owner, PinInfoSPTR info, codec_para_t* codecContextPtr)
		: OutPin(owner, info),
		codecContextPtr(codecContextPtr)
	{
		ElementSPTR element = owner.lock();
		if (!element)
			throw InvalidOperationException();

		ClockDataBufferSPTR clockBuffer = std::make_shared<ClockDataBuffer>(element);
		AddAvailableBuffer(clockBuffer);


		timerExpiredListener = std::make_shared<EventListener<EventArgs>>(
			std::bind(&AmlVideoSinkClockOutPin::timer_Expired, this, std::placeholders::_1, std::placeholders::_2));

		timer.Expired.AddListener(timerExpiredListener);
		timer.SetInterval(1.0 / (60.0 * 2.0));
		timer.Start();
	}
};

typedef std::shared_ptr<AmlVideoSinkClockOutPin> AmlVideoSinkClockOutPinSPTR;



class AmlVideoSinkElement : public Element
{
	const unsigned long PTS_FREQ = 90000;
	//
	//const long EXTERNAL_PTS = (1);
	//const long SYNC_OUTSIDE = (2);
	//const long USE_IDR_FRAMERATE = 0x04;
	//const long UCODE_IP_ONLY_PARAM = 0x08;
	//const long MAX_REFER_BUF = 0x10;
	//const long ERROR_RECOVERY_MODE_IN = 0x20;

	std::vector<unsigned char> videoExtraData;

	//codec_para_t codecContext;
	double lastTimeStamp = -1;

	bool isFirstVideoPacket = true;
	bool isAnnexB = false;
	bool isExtraDataSent = false;
	long estimatedNextPts = 0;

	VideoFormatEnum videoFormat = VideoFormatEnum::Unknown;
	VideoInPinSPTR videoPin;
	bool isFirstData = true;
	std::vector<unsigned char> extraData;

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

	AmlVideoSinkClockOutPinSPTR clockOutPin;
	AmlCodec amlCodec;



	void timer_Expired(void* sender, const EventArgs& args);
	void SetupHardware();
	void ProcessBuffer(AVPacketBufferSPTR buffer);	
	//void SendCodecData(unsigned long pts, unsigned char* data, int length);


protected:


	virtual void Initialize() override;
	virtual void DoWork() override;
	virtual void ChangeState(MediaState oldState, MediaState newState) override;

	virtual void Terminating() override;

public:

	double Clock();

	virtual void Flush() override;

private:

	//static void WriteToFile(const char* path, const char* value);

	void Divx3Header(int width, int height, int packetSize);
	void ConvertH264ExtraDataToAnnexB();
	void HevcExtraDataToAnnexB();

};

typedef std::shared_ptr<AmlVideoSinkElement> AmlVideoSinkElementSPTR;
