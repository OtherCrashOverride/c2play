#pragma once

#include "Element.h"


class NullSinkElement : public Element
{
	InPinSPTR pin;


public:
	virtual void Initialize() override
	{
		ClearOutputPins();
		ClearInputPins();

		// Create a pin
		PinInfoSPTR info = std::make_shared<PinInfo>(MediaCategoryEnum::Unknown);

		ElementWPTR weakPtr = shared_from_this();
		pin = std::make_shared<InPin>(weakPtr, info);
		AddInputPin(pin);
	}

	virtual void DoWork() override
	{
		BufferSPTR buffer;
		while (pin->TryGetFilledBuffer(&buffer))
		{
			pin->PushProcessedBuffer(buffer);
		}

		pin->ReturnProcessedBuffers();
	}

	virtual void ChangeState(MediaState oldState, MediaState newState) override
	{
		Element::ChangeState(oldState, newState);
	}
};

