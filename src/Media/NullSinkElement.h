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

