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

extern "C"
{
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
}

#include <ass/ass.h>

#include "Element.h"
#include "InPin.h"
#include "OutPin.h"
#include "Image.h"
//#include "Egl.h"
//#include "QuadBatch.h"
#include "Compositor.h"
#include "Timer.h"
#include "IClock.h"
#include "../UI/Rectangle.h"


class SubtitleDecoderElement : public Element
{
	int width;
	int height;
	float xScale;
	float yScale;
	SubtitleInPinSPTR inPin;
	SubtitlePinInfoSPTR inInfo;
	OutPinSPTR outPin;
	PinInfoSPTR outInfo;


	AVCodec* avcodec = nullptr;
	AVCodecContext* avcodecContext = nullptr;
	AVSubtitle* avSubtitle = nullptr;
	bool isFirstData = true;
	

	ASS_Library *ass_library;
	ASS_Renderer *ass_renderer;
	ASS_Track * ass_track;
	bool isSaaHeaderSent = false;

	static void static_msg_callback(int level, const char* fmt, va_list va, void *data);

	void SetupCodec();
	void ProcessBuffer(AVPacketBufferSPTR buffer);


public:

	SubtitleDecoderElement(int width, int height);
	virtual ~SubtitleDecoderElement();



	virtual void Initialize() override;
	virtual void DoWork() override;
	virtual void ChangeState(MediaState oldState, MediaState newState) override;
	virtual void Flush() override;
};

typedef std::shared_ptr<SubtitleDecoderElement> SubtitleDecoderElementSPTR;




//--------------


struct SpriteEntry
{
public:
	double StartTime = 0;
	double StopTime = 0;
	SpriteSPTR Sprite;
	bool IsActive = false;


	inline bool operator==(const SpriteEntry& rhs)
	{
		return StartTime == rhs.StartTime &&
			StopTime == rhs.StopTime &&
			Sprite == rhs.Sprite &&
			IsActive == rhs.IsActive;
	}

	inline bool operator!=(const SpriteEntry& rhs)
	{
		return !(*this == rhs); 
	}
};


class SubtitleRenderElement : public Element, public virtual IClockSink
{
	//EGLDisplay eglDisplay;
	//EGLSurface surface;
	//EGLContext context;
	CompositorSPTR compositor;
	InPinSPTR inPin;
	PinInfoSPTR inInfo;
	bool isFirstData = true;
	//QuadBatchSPTR quadBatch;
	std::vector<SpriteEntry> spriteEntries;
	Mutex entriesMutex;
	Timer timer;
	EventListenerSPTR<EventArgs> timerExpiredListener;
	double currentTime = 0;


	void timer_Expired(void* sender, const EventArgs& args);		
	void SetupCodec();
	void ProcessBuffer(ImageListBufferSPTR buffer);

public:

	//SubtitleRenderElement(EGLDisplay eglDisplay, EGLSurface surface, EGLContext context);
	SubtitleRenderElement(CompositorSPTR compositor);
	virtual ~SubtitleRenderElement();



	virtual void Initialize() override;
	virtual void DoWork() override;
	virtual void ChangeState(MediaState oldState, MediaState newState) override;
	virtual void Flush() override;


	// Inherited via IClockSink
	virtual void SetTimeStamp(double value) override;
};

typedef std::shared_ptr<SubtitleRenderElement> SubtitleRenderElementSPTR;
