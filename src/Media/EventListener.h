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

#include <functional>
#include <memory>



template<typename T>
using EventFunction = std::function<void(void* sender, const T& args)>;

template<typename T>
class Event;


template<typename T>
class EventListener
{
	friend class Event<T>;

	//Event<T>* source = nullptr;
	EventFunction<T> target;


	//void InvalidateSource()
	//{
	//	source = nullptr;
	//}

public:
	EventListener()
	{
	}

	EventListener(EventFunction<T> target)
		: target(target)
	{
	}

	//EventListener(Event<T>* source, EventFunction<T> target)
	//	: source(source), target(target)
	//{
	//	if (source == nullptr)
	//		throw ArgumentNullException();
	//}

	~EventListener()
	{
		//// Remove event
		//if (source)
		//{
		//	source->RemoveListener(this);
		//}
	}


	void Invoke(void *sender, const T& args)
	{
		if (target)
		{
			target(sender, args);
		}
	}
};

template<typename T>
using EventListenerSPTR = std::shared_ptr<EventListener<T>>;

template<typename T>
using EventListenerWPTR = std::weak_ptr<EventListener<T>>;
