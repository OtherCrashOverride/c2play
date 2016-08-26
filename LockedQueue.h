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

#include <pthread.h>
#include <queue>
#include <unistd.h>



template <typename T>
class ThreadSafeQueue
{
	std::queue<T> queue;
	Mutex mutex;


public:
	int Count() const
	{
		return queue.size();
	}



	ThreadSafeQueue() {}



	void Push(T value)
	{
		mutex.Lock();
		queue.push(value);
		mutex.Unlock();
	}

	bool TryPop(T* outValue)
	{
		bool result;

		mutex.Lock();

		if (queue.size() < 1)
		{
			result = false;
		}
		else
		{
			*outValue = queue.front();
			queue.pop();

			result = true;
		}

		mutex.Unlock();

		return result;
	}

	bool TryPeek(T* outValue)
	{
		bool result;

		mutex.Lock();

		if (queue.size() < 1)
		{
			result = false;
		}
		else
		{
			*outValue = queue.front();
			result = true;
		}

		mutex.Unlock();

		return result;
	}

	void Clear()
	{
		mutex.Lock();
		
		while (queue.size() > 0)
		{
			queue.pop();
		}

		mutex.Unlock();
	}
};



template <typename T>
class LockedQueue
{
	const int limit = 0x7fffffff;
	std::queue<T> queue;
	pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

public:

	int Limit() const
	{
		return limit;
	}
	void SetLimit(int value)
	{
		if (value < 1)
			throw ArgumentOutOfRangeException();

		pthread_mutex_lock(&mutex);

		if (queue.size() > value)
		{
			pthread_mutex_unlock(&mutex);
			throw InvalidOperationException("The queue has more entries than the limit.");
		}
		else
		{
			limit = value;
			pthread_mutex_unlock(&mutex);
		}
	}

	LockedQueue()
	{
	}

	LockedQueue(int limit)
		: limit(limit)
	{
		if (limit < 1)
		{
			throw ArgumentOutOfRangeException();
		}
	}


	void Push(T value)
	{
		pthread_mutex_lock(&mutex);

		while (queue.size() >= limit)
		{
			pthread_mutex_unlock(&mutex);
			usleep(1);
			pthread_mutex_lock(&mutex);
		}

		queue.push(value);

		pthread_mutex_unlock(&mutex);
	}

	bool TryPush(T value)
	{
		bool result;

		pthread_mutex_lock(&mutex);

		if (queue.size() >= limit)
		{
			result = false;
		}
		else
		{
			queue.push(value);
			result = true;
		}

		pthread_mutex_unlock(&mutex);

		return result;
	}

	T Pop()
	{
		pthread_mutex_lock(&mutex);

		while (queue.size() < 1)
		{
			pthread_mutex_unlock(&mutex);
			usleep(1);
			pthread_mutex_lock(&mutex);
		}

		T result = queue.front();
		queue.pop();

		pthread_mutex_unlock(&mutex);
	}

	bool TryPop(T* outValue)
	{
		bool result;

		pthread_mutex_lock(&mutex);

		if (queue.size() < 1)
		{
			//memset(outValue, 0, sizeof(*outValue));
			result = false;
		}
		else
		{
			*outValue = queue.front();
			queue.pop();
			result = true;
		}

		pthread_mutex_unlock(&mutex);

		return result;
	}

	void Flush()
	{
		pthread_mutex_lock(&mutex);

		while (queue.size() > 0)
		{
			queue.pop();
		}

		pthread_mutex_unlock(&mutex);
	}
};
