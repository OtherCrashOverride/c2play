#include "OutPin.h"

#include "InPin.h"
#include "Element.h"

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
			pin->ReceiveBuffer(buffer);
		}
		else
		{
			AddAvailableBuffer(buffer);
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

		sinkMutex.Unlock();
	}

	void OutPin::AcceptProcessedBuffer(BufferSPTR buffer)
	{
		//if (!buffer)
		//	throw ArgumentNullException();

		//if (buffer->Owner() != (void*)this)
		//{
		//	throw InvalidOperationException("The buffer being returned does not belong to this object.");
		//}

		//availableBuffers.Push(buffer);
		AddAvailableBuffer(buffer);
	}

