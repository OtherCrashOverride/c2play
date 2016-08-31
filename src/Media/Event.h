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

#include "Mutex.h"
#include "Exception.h"
#include "EventListener.h"

#include <vector>


template<typename T>  // where T : EventArgs
class Event
{
	Mutex mutex;
	std::vector<EventListenerWPTR<T>> listeners;

public:

	~Event()
	{
		//for (auto item : listeners)
		//{
		//	item->InvalidateSource();
		//}
	}

	void Invoke(void *sender, const T& args)
	{
		if (sender == nullptr)
			throw ArgumentNullException();


		mutex.Lock();

		for (auto item : listeners)
		{
			auto sptr = item.lock();
			if (sptr)
			{
				sptr->Invoke(sender, args);
			}
			//printf("Event: Invoked a listener (%p).\n", *item);
		}

		//TODO: Clean up dead objects
		
		mutex.Unlock();
	}

	void AddListener(EventListenerWPTR<T> listener)
	{
		auto listenerSPTR = listener.lock();

		if (!listenerSPTR)
			throw ArgumentNullException();


		//printf("Event: Attempting to add a listener (%p).\n", listener);

		mutex.Lock();

		bool isDuplicate = false;
		for (auto item : listeners)
		{
			auto sptr = item.lock();
			if (sptr == listenerSPTR)
			{
				isDuplicate = true;
				break;
			}
		}

		if (!isDuplicate)
		{
			listeners.push_back(listener);
			//printf("Event: Added a listener (%p).\n", listener);
		}

		mutex.Unlock();
	}

	void RemoveListener(EventListenerSPTR<T> listener)
	{
		if (listener == nullptr)
			throw ArgumentNullException();


		mutex.Lock();

		for (auto item : listeners)
		{
			auto sptr = item.lock();
			if (sptr == listener)
			{
				listeners.erase(item);
				break;
			}
		}

		mutex.Unlock();
	}
};
