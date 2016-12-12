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

#include <memory>
#include <string>
#include <vector>

extern "C"
{
#include <libavformat/avformat.h>
#include <libavutil/error.h>
}

#include "Element.h"
#include "OutPin.h"
#include "EventListener.h"
#include "Exception.h"



class AVException : public Exception
{
	static std::string GetErrorString(int error)
	{
		char buffer[AV_ERROR_MAX_STRING_SIZE + 1];
		char* message = av_make_error_string(buffer, AV_ERROR_MAX_STRING_SIZE, error);

		// Return a copy that survives after buffer goes out of scope
		return std::string(message);
	}

public:

	AVException(int error)
		: Exception(GetErrorString(error).c_str())
	{
	}
};



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
	std::vector<uint64_t> streamNextPts;

	ChapterListSPTR chapters = NewSPTR<ChapterList>();
	
	EventListenerSPTR<EventArgs> bufferReturnedListener;

	uint64_t lastPts = 0;
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


	MediaSourceElement(std::string url, std::string avOptions);


	virtual void Initialize() override;
	virtual void DoWork() override;

	void Seek(double timeStamp);

};

typedef std::shared_ptr<MediaSourceElement> MediaSourceElementSPTR;
