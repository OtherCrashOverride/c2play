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


		int drift = vpts - pts;
		double driftTime = drift / (double)PTS_FREQ;
		double driftFrames = driftTime * frameRate;

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

	codec_para_t codecContext;
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


	void timer_Expired(void* sender, const EventArgs& args);
	void SetupHardware();
	void ProcessBuffer(AVPacketBufferSPTR buffer);	
	void SendCodecData(unsigned long pts, unsigned char* data, int length);


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
