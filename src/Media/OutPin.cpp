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

#include "OutPin.h"

#include "InPin.h"
#include "Element.h"


void OutPin::WorkThread()
{
	//printf("OutPin: WorkTread started.\n");

	while (true)
	{
		// Scope the shared pointer
		{
			ElementSPTR owner = Owner().lock();
			if (owner && owner->State() == MediaState::Play)
			{
				DoWork();
			}
		}

		waitCondition.WaitForSignal();
	}

	//printf("OutPin: WorkTread exited.\n");
}


OutPin::OutPin(ElementWPTR owner, PinInfoSPTR info)
	: Pin(PinDirectionEnum::Out, owner, info)
{
}


void OutPin::AddAvailableBuffer(BufferSPTR buffer)
{
	if (!buffer)
		throw ArgumentNullException();

	ElementSPTR element = Owner().lock();
	if (buffer->Owner() != element)
	{
		throw InvalidOperationException("The buffer being added does not belong to this object.");
	}

	availableBuffers.Push(buffer);

	element->Wake();

	//printf("OutPin::AddAvailableBuffer (%s).\n", element->Name().c_str());
}

bool OutPin::TryGetAvailableBuffer(BufferSPTR* outValue)
{
	return availableBuffers.TryPop(outValue);
}

void OutPin::SendBuffer(BufferSPTR buffer)
{
	InPinSPTR pin = sink;

	if (pin)
	{
		auto owner = pin->Owner().lock();
		if (owner && (owner->ExecutionState() == ExecutionStateEnum::Executing ||
			owner->ExecutionState() == ExecutionStateEnum::Idle))
		{

			pin->ReceiveBuffer(buffer);
		}
		else
		{
			AcceptProcessedBuffer(buffer);
		}
	}
	else
	{
		//AddAvailableBuffer(buffer);

		// This is required to generate an event
		AcceptProcessedBuffer(buffer);
	}
}

void OutPin::Flush()
{
	// TODO: cascade to connected pin?
}


OutPin::~OutPin()
{
	if (sink)
	{
		OutPinSPTR thisPin = std::static_pointer_cast<OutPin>(shared_from_this());
		sink->Disconnect(thisPin);
	}
}


void OutPin::Connect(InPinSPTR sink)
{
	if (!sink)
		throw ArgumentNullException();

	//ElementSPTR parent = Owner().lock();

	//if (parent->IsExecuting())
	//	throw InvalidOperationException();


	sinkMutex.Lock();


	OutPinSPTR thisPin = std::static_pointer_cast<OutPin>(shared_from_this());

	// If a pin is connected, disconnect it
	if (this->sink)
	{
		this->sink->Disconnect(thisPin);
	}

	// Connect new pin
	this->sink = sink;
	this->sink->AcceptConnection(thisPin);

		
	pinThread = std::make_shared<Thread>(std::function<void()>(std::bind(&OutPin::WorkThread, this)));
	pinThread->Start();


	sinkMutex.Unlock();
}

void OutPin::AcceptProcessedBuffer(BufferSPTR buffer)
{
	if (!buffer)
		throw ArgumentNullException();

	//if (buffer->Owner() != (void*)this)
	//{
	//	throw InvalidOperationException("The buffer being returned does not belong to this object.");
	//}

		
	//availableBuffers.Push(buffer);
	AddAvailableBuffer(buffer);

	// Wake the work thread
	waitCondition.Signal();

	//BufferEventArgs args(buffer);
	BufferReturned.Invoke(this, EventArgs::Empty());
}

