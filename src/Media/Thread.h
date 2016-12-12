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
#include <functional>

#include <pthread.h>



class Thread
{
	pthread_t thread;
	std::function<void()> function;
	bool isCreated = false;


	static void* ThreadTrampoline(void* argument);


public:
	Thread(std::function<void()> function);
	virtual ~Thread();


	void Start();

	void Cancel();

	void Join();


	static void SetCancelEnabled(bool value);

	static void SetCancelTypeDeferred(bool value);

};

typedef std::shared_ptr<Thread> ThreadSPTR;

