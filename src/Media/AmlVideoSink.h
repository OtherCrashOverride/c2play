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


#include <vector>

#include "Codec.h"
#include "Element.h"
#include "InPin.h"
#include "Thread.h"
#include "Timer.h"
#include "AmlCodec.h"



class AmlVideoSinkClockInPin : public InPin
{
	const uint64_t PTS_FREQ = 90000;

	//codec_para_t* codecContextPtr;
	AmlCodec* codecPTR;
	//double clock = 0;
	double frameRate = 0;	// TODO just read info from Owner()


	void ProcessClockBuffer(BufferSPTR buffer)
	{
		// truncate to 32bit
		uint64_t pts = (uint64_t)(buffer->TimeStamp() * PTS_FREQ);
		pts &= 0xffffffff;

		uint64_t vpts = (uint64_t)(codecPTR->GetCurrentPts() * PTS_FREQ);
		vpts &= 0xffffffff;

		//double pts = pts1 / (double)PTS_FREQ;

		//unsigned long pts = (unsigned long)(buffer->TimeStamp() * PTS_FREQ);


		//int vpts = codec_get_vpts(codecContextPtr);
		//double vpts = vpts1 / (double)PTS_FREQ;

		//if (clock < (vpts / (double)PTS_FREQ))
		//{
		//	clock = (vpts / (double)PTS_FREQ);
		//}


		double drift = ((double)vpts - (double)pts) / (double)PTS_FREQ;
		//double driftTime = drift / (double)PTS_FREQ;
		double driftFrames = drift * frameRate;

#if 1
		const int maxDelta = 60;
		// To minimize clock jitter, only adjust the clock if it
		// deviates more than +/- 2 frames
		if (driftFrames >= maxDelta || driftFrames <= -maxDelta)
		{
			{
				codecPTR->SetCurrentPts(buffer->TimeStamp());

				printf("AmlVideoSink: Adjust PTS - pts=%f vpts=%f drift=%f (%f frames)\n", pts / (double)PTS_FREQ, vpts / (double)PTS_FREQ, drift, driftFrames);
			}
		}
#endif
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



//class AmlVideoSinkClockOutPin : public OutPin
//{
//	const unsigned long PTS_FREQ = 90000;
//
//	codec_para_t* codecContextPtr;
//	Timer timer;
//	Mutex mutex;
//	EventListenerSPTR<EventArgs> timerExpiredListener;
//
//
//	void timer_Expired(void* sender, const EventArgs& args)
//	{
//		mutex.Lock();
//
//		ElementSPTR element = Owner().lock();
//		if (element)
//		{
//			if (element->State() == MediaState::Play)
//			{
//				BufferSPTR buffer;
//				if (TryGetAvailableBuffer(&buffer))
//				{
//					if (buffer->Type() != BufferTypeEnum::ClockData)
//						throw InvalidOperationException();
//
//					ClockDataBufferSPTR clockBuffer = std::static_pointer_cast<ClockDataBuffer>(buffer);
//
//					int vpts = codec_get_vpts(codecContextPtr);
//					double clock = vpts / (double)PTS_FREQ;
//
//					clockBuffer->SetTimeStamp(clock);
//					SendBuffer(clockBuffer);
//
//					//printf("AmlVideoSinkClockOutPin: TimeStamp=%f\n", clockBuffer->TimeStamp());
//				}
//			}
//		}
//
//		mutex.Unlock();
//	}
//
//
//protected:
//
//	//virtual void DoWork() override
//	//{
//
//	//}
//
//public:
//
//	AmlVideoSinkClockOutPin(ElementWPTR owner, PinInfoSPTR info, codec_para_t* codecContextPtr)
//		: OutPin(owner, info),
//		codecContextPtr(codecContextPtr)
//	{
//		ElementSPTR element = owner.lock();
//		if (!element)
//			throw InvalidOperationException();
//
//		ClockDataBufferSPTR clockBuffer = std::make_shared<ClockDataBuffer>(element);
//		AddAvailableBuffer(clockBuffer);
//
//
//		timerExpiredListener = std::make_shared<EventListener<EventArgs>>(
//			std::bind(&AmlVideoSinkClockOutPin::timer_Expired, this, std::placeholders::_1, std::placeholders::_2));
//
//		timer.Expired.AddListener(timerExpiredListener);
//		timer.SetInterval(1.0 / (60.0 * 2.0));
//		timer.Start();
//	}
//};
//
//typedef std::shared_ptr<AmlVideoSinkClockOutPin> AmlVideoSinkClockOutPinSPTR;



class AmlVideoSinkElement : public Element
{
	const uint64_t PTS_FREQ = 90000;
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
	bool isShortStartCode = false;
	bool isExtraDataSent = false;
	int64_t estimatedNextPts = 0;

	VideoFormatEnum videoFormat = VideoFormatEnum::Unknown;
	VideoInPinSPTR videoPin;
	bool isFirstData = true;
	std::vector<unsigned char> extraData;

	uint64_t clockPts = 0;
	uint64_t lastClockPts = 0;
	double clock = 0;

	AmlVideoSinkClockInPinSPTR clockInPin;
	bool isEndOfStream = false;
	double eosPts = -1;

	Timer timer;
	EventListenerSPTR<EventArgs> timerExpiredListener;
	Mutex timerMutex;
	bool doPauseFlag = false;
	bool doResumeFlag = false;
	Mutex playPauseMutex;

	//AmlVideoSinkClockOutPinSPTR clockOutPin;
	AmlCodec amlCodec;



	void timer_Expired(void* sender, const EventArgs& args);
	void SetupHardware();
	void ProcessBuffer(AVPacketBufferSPTR buffer);	
	bool SendCodecData(unsigned long pts, unsigned char* data, int length);


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
