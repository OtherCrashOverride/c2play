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

#include "Codec.h"
#include "Thread.h"
#include "LockedQueue.h"

#include <alsa/asoundlib.h>


#include <vector>

#include "Codec.h"
#include "Element.h"
#include "InPin.h"
#include "IClock.h"



class AlsaAudioSinkElement : public Element
{
	InPinSPTR audioPin;
	OutPinSPTR clockOutPin;

	const int AUDIO_FRAME_BUFFERCOUNT = 8;
	const char* device = "default"; //default   //plughw                     /* playback device */
	const int alsa_channels = 2;

	AVCodecID codec_id = AV_CODEC_ID_NONE;
	unsigned int sampleRate = 0;
	snd_pcm_t* handle = nullptr;
	snd_pcm_sframes_t frames;
	//IClockSinkPtr clockSink;
	double lastTimeStamp = -1;
	bool canPause = true;
	LockedQueue<PcmDataBufferPtr> pcmBuffers = LockedQueue<PcmDataBufferPtr>(128);

	bool isFirstData = true;
	AudioFormatEnum audioFormat = AudioFormatEnum::Unknown;
	int streamChannels = 0;

	bool isFirstBuffer = true;
	snd_pcm_uframes_t period_size;
	snd_pcm_uframes_t buffer_size;
	double audioAdjustSeconds = 0.0;
	double clock = 0.0;

	int FRAME_SIZE = 0;
	bool doResumeFlag = false;
	bool doPauseFlag = false;
	Mutex playPauseMutex;
	//std::vector<IClockSinkSPTR> 
	ClockList clockSinks;

	void SetupAlsa(int frameSize);

	void ProcessBuffer(PcmDataBufferSPTR pcmBuffer);

public:

	double AudioAdjustSeconds() const;
	void SetAudioAdjustSeconds(double value);

	double Clock() const;

	ClockList* ClockSinks()
	{
		return &clockSinks;
	}


	virtual void Flush() override;

protected:
	virtual void Initialize() override;

	virtual void DoWork() override;

	virtual void Terminating() override;

	virtual void ChangeState(MediaState oldState, MediaState newState) override;
};

typedef std::shared_ptr<AlsaAudioSinkElement> AlsaAudioSinkElementSPTR;
