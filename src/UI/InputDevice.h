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

#include "Exception.h"

#include <pthread.h>
#include <queue>
#include <string>
#include <linux/input.h>
#include <linux/uinput.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <memory>



class InputDevice
{
	const int MAX_QUEUE_SIZE = 16;


	std::string fileName;
	int fd = -1;
	std::string name;
	pthread_t thread;
	bool isRunning = false;
	pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
	std::queue<int> keyQueue;


	static void* ThreadTrampoline(void* argument);



public:


	const std::string& FileName() const;
	const std::string& Name() const;



	InputDevice(std::string fileName);
	~InputDevice();



	bool TryGetKeyPress(int* outValue);
	void EnqueKey(int code);
	void WorkThread();
};

typedef std::shared_ptr<InputDevice> InputDevicePtr;
