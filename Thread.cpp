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

#include "Thread.h"

#include "Exception.h"




void* Thread::ThreadTrampoline(void* argument)
{
	Thread* ptr = (Thread*)argument;
	ptr->function();
}



Thread::Thread(std::function<void()> function)
	: function(function)
{
}
Thread::~Thread()
{
	if (isCreated)
		pthread_detach(thread);
}



void Thread::Start()
{
	if (isCreated)
		throw InvalidOperationException();

	int result_code = pthread_create(&thread, NULL, ThreadTrampoline, (void*)this);
	if (result_code != 0)
	{
		throw Exception("Sink pthread_create failed.\n");
	}

	isCreated = true;
}

void Thread::Cancel()
{
	pthread_cancel(thread);
}

void Thread::Join()
{
	pthread_join(thread, NULL);
}


void Thread::SetCancelEnabled(bool value)
{
	if (value)
	{
		pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
	}
	else
	{
		pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);
	}
}

void Thread::SetCancelTypeDeferred(bool value)
{
	if (value)
	{
		pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, NULL);
	}

	else
	{
		pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);
	}
}
