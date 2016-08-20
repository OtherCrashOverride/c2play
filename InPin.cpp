#include "InPin.h"

#include "OutPin.h"
#include "Element.h"


	void InPin::ReturnAllBuffers()
	{
		sourceMutex.Lock();

		if (!source)
			throw InvalidOperationException("Can not return buffers to a null source");

		BufferPTR buffer;
		while (processedBuffers.TryPop(&buffer))
		{
			source->AcceptProcessedBuffer(buffer);
		}

		while (filledBuffers.TryPop(&buffer))
		{
			source->AcceptProcessedBuffer(buffer);
		}

		sourceMutex.Unlock();
	}


	InPin::InPin(ElementWPTR owner, PinInfoSPTR info)
		: Pin(PinDirection::In, owner, info)
	{
	}

	InPin::~InPin()
	{
		if (source)
		{
			ReturnAllBuffers();
		}
	}


	void InPin::AcceptConnection(OutPinSPTR source)
	{
		if (!source)
			throw ArgumentNullException();

		if (this->source)
		{
			throw InvalidOperationException("A source is already connected.");
		}

		//ElementSPTR parent = Owner().lock();
		//if (parent->IsExecuting())
		//	throw InvalidOperationException("Can not connect while executing.");


		sourceMutex.Lock();
		this->source = source;
		sourceMutex.Unlock();
	}

	void InPin::Disconnect(OutPinSPTR source)
	{
		OutPinSPTR safeSource = this->source;

		if (!source)
			throw ArgumentNullException();

		if (!safeSource)
			throw InvalidOperationException("No source is connected.");

		if (safeSource != source)
			throw InvalidOperationException("Attempt to disconnect a source that is not connected.");

		//ElementSPTR parent = Owner().lock();
		//if (parent->IsExecuting())
		//	throw InvalidOperationException("Can not disconnect while executing.");


		ReturnAllBuffers();

		sourceMutex.Lock();
		source = nullptr;
		sourceMutex.Unlock();
	}

	void InPin::ReceiveBuffer(BufferSPTR buffer)
	{
		if (!buffer)
			throw ArgumentNullException();

		filledBuffers.Push(buffer);

		ElementSPTR parent = Owner().lock();
		if (parent)
		{
			parent->Wake();
		}
	}


	void InPin::Flush()
	{
		ReturnAllBuffers();
	}
