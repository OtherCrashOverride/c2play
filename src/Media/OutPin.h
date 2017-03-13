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


//class InPin;
//typedef std::shared_ptr<InPin> InPinSPTR;

class BufferEventArgs : public EventArgs
{
	BufferSPTR buffer;


public:
	BufferSPTR Buffer() const
	{
		return buffer;
	}


	BufferEventArgs(BufferSPTR buffer)
		: buffer(buffer)
	{
	}

};



class OutPin : public Pin
{
	friend class Element;

	InPinSPTR sink;
	Mutex sinkMutex;
	//ThreadSafeQueue<BufferSPTR> filledBuffers;
	ThreadSafeQueue<BufferSPTR> availableBuffers;
	ThreadSPTR pinThread;
	WaitCondition waitCondition;


	void WorkThread();


protected:

	void AddAvailableBuffer(BufferSPTR buffer);
	virtual void DoWork();


public:
	Event<EventArgs> BufferReturned;

	bool IsConnected() const
	{
		return sink != nullptr;
	}


	OutPin(ElementWPTR owner, PinInfoSPTR info);

	virtual ~OutPin();

	
	void Wake();

	bool TryGetAvailableBuffer(BufferSPTR* outValue);
	bool TryPeekAvailableBuffer(BufferSPTR* buffer);
	void SendBuffer(BufferSPTR buffer);
	virtual void Flush() override;


	void Connect(InPinSPTR sink);
	void AcceptProcessedBuffer(BufferSPTR buffer);
};

//typedef std::shared_ptr<OutPin> OutPinSPTR;


class OutPinCollection : public PinCollection<OutPinSPTR>
{
public:
	OutPinCollection()
		: PinCollection()
	{
	}
};
