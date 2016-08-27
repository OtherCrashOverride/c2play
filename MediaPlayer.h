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

#include "MediaSourceElement.h"
#include "AlsaAudioSink.h"
#include "AmlVideoSink.h"
#include "AudioCodec.h"

#include <string>



class MediaPlayer
{
	std::string url;
	MediaSourceElementSPTR source;
	AmlVideoSinkElementSPTR videoSink;
	AudioCodecElementSPTR audioCodec;
	AlsaAudioSinkElementSPTR audioSink;
	MediaState state = MediaState::Pause;


public:

	double Position() const;

	MediaState State() const;
	void SetState(MediaState value);

	bool IsEndOfStream();

	const ChapterListSPTR Chapters() const;



	MediaPlayer(std::string url);
	~MediaPlayer();



	void Seek(double timeStamp);
};

typedef std::shared_ptr<MediaPlayer> MediaPlayerSPTR;
