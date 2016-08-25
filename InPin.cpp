#include "InPin.h"

#include "OutPin.h"
#include "Element.h"


	void InPin::ReturnAllBuffers()
	{
		ElementSPTR parent = Owner().lock();

		sourceMutex.Lock();

		//if (!source)
		//	throw InvalidOperationException("Can not return buffers to a null source");

		if (source)
		{
			BufferSPTR buffer;
			while (processedBuffers.TryPop(&buffer))
			{
				source->AcceptProcessedBuffer(buffer);
			}

			while (filledBuffers.TryPop(&buffer))
			{
				source->AcceptProcessedBuffer(buffer);
			}
		}

		sourceMutex.Unlock();

		parent->Log("InPin::ReturnAllBuffers completed\n");
	}


	InPin::InPin(ElementWPTR owner, PinInfoSPTR info)
		: Pin(PinDirectionEnum::In, owner, info)
	{
	}

	InPin::~InPin()
	{
		if (source)
		{
			ReturnAllBuffers();
		}
	}


	void InPin::ReturnProcessedBuffers()
	{
		ElementSPTR parent = Owner().lock();

		sourceMutex.Lock();

		//if (!source)
		//	throw InvalidOperationException("Can not return buffers to a null source");

		if (source)
		{
			BufferSPTR buffer;
			while (processedBuffers.TryPop(&buffer))
			{
				source->AcceptProcessedBuffer(buffer);

				if (Owner().expired() || parent.get() == nullptr)
				{
					printf("InPin::ReturnProcessedBuffers: WARNING parent expired.\n");
				}
				else
				{
					parent->Log("InPin::ReturnProcessedBuffers returned a buffer\n");
				}
			}
		}

		sourceMutex.Unlock();

		parent->Log("InPin::ReturnProcessedBuffers completed\n");
	}



	void InPin::AcceptConnection(OutPinSPTR source)
	{
		if (!source)
			throw ArgumentNullException();

		if (this->source)
		{
			throw InvalidOperationException("A source is already connected.");
		}


		ElementSPTR parent = Owner().lock();
		//ElementSPTR parent = Owner().lock();
		//if (parent->IsExecuting())
		//	throw InvalidOperationException("Can not connect while executing.");


		sourceMutex.Lock();
		this->source = source;
		sourceMutex.Unlock();


		pinThread = std::make_shared<Thread>(std::function<void()>(std::bind(&InPin::WorkThread, this)));
		pinThread->Start();

		parent->Wake();

		parent->Log("InPin::AcceptConnection completed\n");
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


		ElementSPTR parent = Owner().lock();
		//ElementSPTR parent = Owner().lock();
		//if (parent->IsExecuting())
		//	throw InvalidOperationException("Can not disconnect while executing.");


		pinThread->Cancel();
		pinThread->Join();
		pinThread = nullptr;


		ReturnAllBuffers();


		sourceMutex.Lock();
		source = nullptr;
		sourceMutex.Unlock();

		parent->Log("InPin::Disconnect completed\n");
	}

	void InPin::ReceiveBuffer(BufferSPTR buffer)
	{
		if (!buffer)
			throw ArgumentNullException();


		ElementSPTR parent = Owner().lock();


		filledBuffers.Push(buffer);


		if (parent)
		{
			parent->Wake();
		}


		// Wake the work thread
		waitCondition.Signal();


		BufferReceived.Invoke(this, EventArgs::Empty());

		parent->Log("InPin::ReceiveBuffer completed\n");
	}


	void InPin::Flush()
	{
		ElementSPTR parent = Owner().lock();


		ReturnAllBuffers();

		parent->Log("InPin::Flush completed\n");
	}
