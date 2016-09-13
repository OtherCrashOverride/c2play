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

#include "Pin.h"
#include "Thread.h"
#include "WaitCondition.h"
#include "Event.h"
#include "EventArgs.h"


//class OutPin;
//typedef std::shared_ptr<OutPin> OutPinSPTR;

class InPin : public Pin
{
	friend class Element;

	OutPinSPTR source;	// TODO: WeakPointer
	Mutex sourceMutex;
	ThreadSafeQueue<BufferSPTR> filledBuffers;
	ThreadSafeQueue<BufferSPTR> processedBuffers;
	ThreadSPTR pinThread;
	WaitCondition waitCondition;



	void WorkThread();



protected:

	void ReturnAllBuffers();
	virtual void DoWork();



public:

	Event<EventArgs> BufferReceived;



	OutPinSPTR Source()
	{
		return source;
	}



	InPin(ElementWPTR owner, PinInfoSPTR info);

	virtual ~InPin();


	// From this element
	bool TryGetFilledBuffer(BufferSPTR* buffer);
	bool TryPeekFilledBuffer(BufferSPTR* buffer);
	void PushProcessedBuffer(BufferSPTR buffer);
	void ReturnProcessedBuffers();



	// From other element
	void AcceptConnection(OutPinSPTR source);
	void Disconnect(OutPinSPTR source);
	
	void ReceiveBuffer(BufferSPTR buffer);
	
	virtual void Flush() override;
};

//typedef std::shared_ptr<InPin> InPinSPTR;

class InPinCollection : public PinCollection<InPinSPTR>
{
public:
	InPinCollection()
		: PinCollection()
	{
	}
};


template <typename T>	//where T:PinInfo
class GenericInPin : public InPin
{
public:
	GenericInPin(ElementWPTR owner, std::shared_ptr<T> info)
		: InPin(owner, info)
	{
	}


	std::shared_ptr<T> InfoAs() const
	{
		return std::static_pointer_cast<T>(Info());
	}
};


class VideoInPin : public GenericInPin<VideoPinInfo>
{
public:
	VideoInPin(ElementWPTR owner, VideoPinInfoSPTR info)
		: GenericInPin(owner, info)
	{
	}
};

typedef std::shared_ptr<VideoInPin> VideoInPinSPTR;


class AudioInPin : public GenericInPin<AudioPinInfo>
{
public:
	AudioInPin(ElementWPTR owner, AudioPinInfoSPTR info)
		: GenericInPin(owner, info)
	{
	}
};

typedef std::shared_ptr<AudioInPin> AudioInPinSPTR;


class SubtitleInPin : public GenericInPin<SubtitlePinInfo>
{
public:
	SubtitleInPin(ElementWPTR owner, SubtitlePinInfoSPTR info)
		: GenericInPin(owner, info)
	{
	}
};

typedef std::shared_ptr<SubtitleInPin> SubtitleInPinSPTR;