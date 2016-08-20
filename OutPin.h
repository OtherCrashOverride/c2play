#pragma once

#include "Pin.h"


//class InPin;
//typedef std::shared_ptr<InPin> InPinSPTR;

class OutPin : public Pin
{
	friend class Element;

	InPinSPTR sink;
	Mutex sinkMutex;
	//ThreadSafeQueue<BufferSPTR> filledBuffers;
	ThreadSafeQueue<BufferSPTR> availableBuffers;


protected:

	void AddAvailableBuffer(BufferSPTR buffer);



public:
	OutPin(ElementWPTR owner, PinInfoSPTR info);

	virtual ~OutPin();

	

	bool TryGetAvailableBuffer(BufferSPTR* outValue);
	void SendBuffer(BufferSPTR buffer);
	virtual void Flush() override;


	void Connect(InPinSPTR sink);
	void AcceptProcessedBuffer(BufferSPTR buffer);
};

//typedef std::shared_ptr<OutPin> OutPinSPTR;
