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

#include "MediaPlayer.h"



double MediaPlayer::Position() const
{
	double result;

	if (videoSink)
	{
		result = videoSink->Clock();
	}
	else if (audioSink)
	{
		result = audioSink->Clock();
	}
	else
	{
		result = 0;
	}

	return result;
}

MediaState MediaPlayer::State() const
{
	return state;
}
void MediaPlayer::SetState(MediaState value)
{
	if (value != state)
	{
		state = value;

		if (audioSink)
		{
			audioSink->SetState(state);
		}

		if (videoSink)
		{
			videoSink->SetState(state);
		}
	}

}

bool MediaPlayer::IsEndOfStream()
{
	bool result;

	bool audioIsIdle = true;
	if (audioSink)
	{
		if (audioSink->State() != MediaState::Pause)
		{
			audioIsIdle = false;
		}

	}

	bool videoIsIdle = true;
	if (videoSink)
	{
		if (videoSink->State() != MediaState::Pause)
		{
			videoIsIdle = false;
		}
	}

	if (State() != MediaState::Pause && audioIsIdle && videoIsIdle)
	{
		result = true;
	}
	else
	{
		result = false;
	}

	return result;
}

const ChapterListSPTR MediaPlayer::Chapters() const
{
	return source->Chapters();
}


MediaPlayer::MediaPlayer(std::string url)
	:url(url)
{
	source = std::make_shared<MediaSourceElement>(url);
	source->SetName(std::string("Source"));
	source->Execute();
	source->WaitForExecutionState(ExecutionStateEnum::Idle);


	// Connections
	OutPinSPTR sourceVideoPin = std::static_pointer_cast<OutPin>(
		source->Outputs()->FindFirst(MediaCategoryEnum::Video));
	if (sourceVideoPin)
	{
		videoSink = std::make_shared<AmlVideoSinkElement>();
		videoSink->SetName(std::string("VideoSink"));
		videoSink->Execute();
		videoSink->WaitForExecutionState(ExecutionStateEnum::Idle);

		sourceVideoPin->Connect(videoSink->Inputs()->Item(0));
	}

	OutPinSPTR sourceAudioPin = std::static_pointer_cast<OutPin>(
		source->Outputs()->FindFirst(MediaCategoryEnum::Audio));
	if (sourceAudioPin)
	{
		audioCodec = std::make_shared<AudioCodecElement>();
		audioCodec->SetName(std::string("AudioCodec"));
		audioCodec->Execute();
		audioCodec->WaitForExecutionState(ExecutionStateEnum::Idle);

		audioSink = std::make_shared<AlsaAudioSinkElement>();
		audioSink->SetName(std::string("AudioSink"));
		audioSink->Execute();
		audioSink->WaitForExecutionState(ExecutionStateEnum::Idle);

		sourceAudioPin->Connect(audioCodec->Inputs()->Item(0));

		audioCodec->Outputs()->Item(0)->Connect(audioSink->Inputs()->Item(0));
	}


	if (audioSink && videoSink)
	{
		// Clock
		audioSink->Outputs()->Item(0)->Connect(videoSink->Inputs()->Item(1));
	}
}
MediaPlayer::~MediaPlayer()
{
	// Tear down
	if (audioSink)
	{
		printf("MediaPlayer: terminating audioSink.\n");
		audioSink->Terminate();
		audioSink->WaitForExecutionState(ExecutionStateEnum::WaitingForExecute);
	}

	if (audioCodec)
	{
		printf("MediaPlayer: terminating audioCodec.\n");
		audioCodec->Terminate();
		audioCodec->WaitForExecutionState(ExecutionStateEnum::WaitingForExecute);
	}

	if (videoSink)
	{
		printf("MediaPlayer: terminating videoSink.\n");
		videoSink->Terminate();
		videoSink->WaitForExecutionState(ExecutionStateEnum::WaitingForExecute);
	}

	printf("MediaPlayer: terminating source.\n");
	source->Terminate();
	source->WaitForExecutionState(ExecutionStateEnum::WaitingForExecute);

	printf("MediaPlayer: destructed.\n");
}



void MediaPlayer::Seek(double timeStamp)
{
	source->SetState(MediaState::Pause);

	if (audioCodec)
	{
		audioCodec->SetState(MediaState::Pause);
	}

	if (audioSink)
	{
		audioSink->SetState(MediaState::Pause);
	}

	if (videoSink)
	{
		videoSink->SetState(MediaState::Pause);
	}


	source->Flush();


	if (audioCodec)
	{
		audioCodec->Flush();
	}

	if (audioSink)
	{
		audioSink->Flush();
	}

	if (videoSink)
	{
		videoSink->Flush();
	}


	source->Seek(timeStamp);


	if (videoSink)
	{
		videoSink->SetState(MediaState::Play);
	}

	if (audioCodec)
	{
		audioCodec->SetState(MediaState::Play);
	}

	if (audioSink)
	{
		audioSink->SetState(MediaState::Play);
	}

	source->SetState(MediaState::Play);
}

