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

#include <cstdio>

#include "MediaPlayer.h"



double MediaPlayer::Position() const
{
	double result;

	if (audioSink)
	{
		result = audioSink->Clock();
	}
	else if (videoSink)
	{
		result = videoSink->Clock();
	}
	else
	{
		result = 0;
	}

	return result;
}

double MediaPlayer::Duration() const
{
	if (source)
	{
		return source->Duration();
	}
	else
	{
		return -1;
	}
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

		if (subtitleCodec)
		{
			subtitleCodec->SetState(state);
		}

		if (subtitleCodec)
		{
			subtitleCodec->SetState(state);
		}

		if (subtitleRender)
		{
			subtitleRender->SetState(state);
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


MediaPlayer::MediaPlayer(std::string url, std::string avOptions, CompositorSPTR compositor, int videoStream, int audioStream, int subtitleStream)
	:url(url), avOptions(avOptions), compositor(compositor)
{
	if (!compositor)
		throw ArgumentNullException();


	source = std::make_shared<MediaSourceElement>(url, avOptions);
	source->SetName(std::string("Source"));
	source->Execute();
	source->WaitForExecutionState(ExecutionStateEnum::Idle);


	// Connections
	OutPinSPTR sourceVideoPin = std::static_pointer_cast<OutPin>(
		source->Outputs()->Find(MediaCategoryEnum::Video, videoStream));
	if (sourceVideoPin)
	{
		videoSink = std::make_shared<AmlVideoSinkElement>();
		videoSink->SetName(std::string("VideoSink"));
		videoSink->Execute();
		videoSink->WaitForExecutionState(ExecutionStateEnum::Idle);

		sourceVideoPin->Connect(videoSink->Inputs()->Item(0));
	}

	OutPinSPTR sourceAudioPin = std::static_pointer_cast<OutPin>(
		source->Outputs()->Find(MediaCategoryEnum::Audio, audioStream));
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

	//OutPinSPTR sourceSubtitlePin;
	OutPinSPTR sourceSubtitlePin = source->Outputs()->Find(MediaCategoryEnum::Subtitle, subtitleStream);
	if (sourceSubtitlePin)
	{
		subtitleCodec = std::make_shared<SubtitleDecoderElement>();
		subtitleCodec->SetName("SubtitleDecoderElement");
		subtitleCodec->Execute();
		subtitleCodec->WaitForExecutionState(ExecutionStateEnum::Idle);

		sourceSubtitlePin->Connect(subtitleCodec->Inputs()->Item(0));


		subtitleRender = std::make_shared<SubtitleRenderElement>(compositor);
		subtitleRender->SetName("SubtitleRenderElement");
		subtitleRender->Execute();
		subtitleRender->WaitForExecutionState(ExecutionStateEnum::Idle);

		subtitleCodec->Outputs()->Item(0)->Connect(subtitleRender->Inputs()->Item(0));

		if (audioSink)
		{
			audioSink->ClockSinks()->Add(subtitleRender);
		}

		printf("MediaPlayer: Connected subtitle decoder.\n");
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

	if (subtitleCodec)
	{
		printf("MediaPlayer: terminating subtitleCodec.\n");
		subtitleCodec->Terminate();
		subtitleCodec->WaitForExecutionState(ExecutionStateEnum::WaitingForExecute);

		printf("MediaPlayer: terminating subtitleRender.\n");
		subtitleRender->Terminate();
		subtitleRender->WaitForExecutionState(ExecutionStateEnum::WaitingForExecute);
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
		//printf("Seek: audioCodec pause.\n");
		audioCodec->SetState(MediaState::Pause);
	}

	if (audioSink)
	{
		//printf("Seek: audioSink pause.\n");
		audioSink->SetState(MediaState::Pause);
	}

	if (videoSink)
	{
		//printf("Seek: videoSink pause.\n");
		videoSink->SetState(MediaState::Pause);
	}

	if (subtitleCodec)
	{
		//printf("Seek: subtitle pause.\n");
		subtitleCodec->SetState(MediaState::Pause);
		subtitleRender->SetState(MediaState::Pause);
	}


	/*source->Flush();*/


	if (audioCodec)
	{
		//printf("Seek: audioCodec flush.\n");
		audioCodec->Flush();
	}

	if (audioSink)
	{
		//printf("Seek: audioSink flush.\n");
		audioSink->Flush();
	}

	if (videoSink)
	{
		//printf("Seek: videoSink flush.\n");
		videoSink->Flush();
	}

	if (subtitleCodec)
	{
		//printf("Seek: subtitle flush.\n");
		subtitleCodec->Flush();
		subtitleRender->Flush();
	}


	//printf("Seek: source flush.\n");
	source->Flush();

	//printf("Seek: source seek.\n");
	source->Seek(timeStamp);


	if (videoSink)
	{
		//printf("Seek: videoSink play.\n");
		videoSink->SetState(MediaState::Play);
	}

	if (audioCodec)
	{
		//printf("Seek: audioCodec play.\n");
		audioCodec->SetState(MediaState::Play);
	}

	if (audioSink)
	{
		//printf("Seek: audioSink play.\n");
		audioSink->SetState(MediaState::Play);
	}

	if (subtitleCodec)
	{
		//printf("Seek: subitile play.\n");
		subtitleCodec->SetState(MediaState::Play);
		subtitleRender->SetState(MediaState::Play);
	}


	//printf("Seek: source play.\n");
	source->SetState(MediaState::Play);
}

