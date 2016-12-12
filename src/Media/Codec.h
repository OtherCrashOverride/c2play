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

#include <memory>



enum class MediaState
{
	Pause = 0,
	Play
};


// Forward Declarations
class Element;
typedef std::shared_ptr<Element> ElementSPTR;
typedef std::weak_ptr<Element> ElementWPTR;

class Pin;
typedef std::shared_ptr<Pin> PinSPTR;

class InPin;
typedef std::shared_ptr<InPin> InPinSPTR;

class OutPin;
typedef std::shared_ptr<OutPin> OutPinSPTR;


