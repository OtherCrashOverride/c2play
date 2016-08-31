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

#include <vector>

#include "Codec.h"
#include "Element.h"
#include "InPin.h"


class AudioCodecElement : public Element
{
	const int alsa_channels = 2;

	InPinSPTR audioInPin;
	OutPinSPTR audioOutPin;
	AudioPinInfoSPTR outInfo;

	AVCodec* soundCodec = nullptr;
	AVCodecContext* soundCodecContext = nullptr;
	bool isFirstData = true;
	AudioFormatEnum audioFormat = AudioFormatEnum::Unknown;

	int streamChannels = 0;
	int outputChannels = 0;
	int sampleRate = 0;
	AVFrameBufferSPTR frame;


	void SetupCodec();
	void ProcessBuffer(AVPacketBufferSPTR buffer, AVFrameBufferSPTR frame);


public:

	virtual void Initialize() override;
	virtual void DoWork() override;
	virtual void ChangeState(MediaState oldState, MediaState newState) override;
	virtual void Flush() override;
};


typedef std::shared_ptr<AudioCodecElement> AudioCodecElementSPTR;
