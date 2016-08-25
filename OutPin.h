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


	void WorkThread()
	{
		//printf("OutPin: WorkTread started.\n");

		while (true)
		{
			DoWork();

			waitCondition.WaitForSignal();
		}

		//printf("OutPin: WorkTread exited.\n");
	}


protected:

	void AddAvailableBuffer(BufferSPTR buffer);

	virtual void DoWork()
	{
		// Work should not block in the thread
	}


public:
	Event<EventArgs> BufferReturned;


	OutPin(ElementWPTR owner, PinInfoSPTR info);

	virtual ~OutPin();

	

	bool TryGetAvailableBuffer(BufferSPTR* outValue);
	bool TryPeekAvailableBuffer(BufferSPTR* buffer)
	{
		return availableBuffers.TryPeek(buffer);
	}
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
