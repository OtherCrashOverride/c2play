#pragma once

#include "Pin.h"

//class OutPin;
//typedef std::shared_ptr<OutPin> OutPinSPTR;

class InPin : public Pin
{
	friend class Element;

	OutPinSPTR source;	// TODO: WeakPointer
	Mutex sourceMutex;
	ThreadSafeQueue<BufferPTR> filledBuffers;
	ThreadSafeQueue<BufferPTR> processedBuffers;


	void ReturnAllBuffers();

protected:


public:
	InPin(ElementWPTR owner, PinInfoSPTR info);

	virtual ~InPin();


	void AcceptConnection(OutPinSPTR source);
	void Disconnect(OutPinSPTR source);
	
	void ReceiveBuffer(BufferSPTR buffer);
	
	virtual void Flush() override;
};

//typedef std::shared_ptr<InPin> InPinSPTR;
