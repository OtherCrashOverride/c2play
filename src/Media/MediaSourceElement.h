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
#include "Element.h"
#include "OutPin.h"
#include "EventListener.h"


#include <string>
#include <map>


extern "C"
{
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
}


#include <memory>


struct Chapter
{
	std::string Title;
	double TimeStamp;
};

typedef std::vector<Chapter> ChapterList;
typedef std::shared_ptr<ChapterList> ChapterListSPTR;


#define NewSPTR std::make_shared
#define NewUPTR std::make_unique


class MediaSourceElement : public Element
{
	const int BUFFER_COUNT = 64;  // enough for 4k video

	std::string url;
	AVFormatContext* ctx = nullptr;
	
	ThreadSafeQueue<BufferSPTR> availableBuffers;
	std::vector<OutPinSPTR> streamList;
	std::vector<unsigned long> streamNextPts;

	ChapterListSPTR chapters = NewSPTR<ChapterList>();
	
	EventListenerSPTR<EventArgs> bufferReturnedListener;

	unsigned long lastPts = 0;
	double duration = -1;


	void outPin_BufferReturned(void* sender, const EventArgs& args);

	static void PrintDictionary(AVDictionary* dictionary);

	void SetupPins();


public:

	const ChapterListSPTR Chapters() const;

	double Duration() const
	{
		return duration;
	}


	MediaSourceElement(std::string url);


	virtual void Initialize() override;
	virtual void DoWork() override;

	void Seek(double timeStamp);

};

typedef std::shared_ptr<MediaSourceElement> MediaSourceElementSPTR;
