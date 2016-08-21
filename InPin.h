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



protected:
	void ReturnAllBuffers();


public:

	OutPinSPTR Source()
	{
		return source;
	}



	InPin(ElementWPTR owner, PinInfoSPTR info);

	virtual ~InPin();


	// From this element
	//ThreadSafeQueue<BufferPTR>* FilledBuffers()
	//{
	//	return &filledBuffers;
	//}

	bool TryGetFilledBuffer(BufferSPTR* buffer)
	{
		return filledBuffers.TryPop(buffer);
	}

	bool TryPeekFilledBuffer(BufferSPTR* buffer)
	{
		return filledBuffers.TryPeek(buffer);
	}

	void PushProcessedBuffer(BufferSPTR buffer)
	{
		processedBuffers.Push(buffer);
	}

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
