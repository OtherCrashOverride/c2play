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

#include <pthread.h>


class Mutex
{
	pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

public:

	Mutex() {}


	void Lock()
	{
		pthread_mutex_lock(&mutex);
	}

	void Unlock()
	{
		pthread_mutex_unlock(&mutex);
	}
};


