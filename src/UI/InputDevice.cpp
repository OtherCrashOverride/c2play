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

#include "InputDevice.h"



void* InputDevice::ThreadTrampoline(void* argument)
{
	InputDevice* ptr = (InputDevice*)argument;
	ptr->WorkThread();

	return nullptr;
}



const std::string& InputDevice::FileName() const
{
	return fileName;
}

const std::string& InputDevice::Name() const
{
	return name;
}


InputDevice::InputDevice(std::string fileName)
	: fileName(fileName)
{
	fd = open(fileName.c_str(), O_RDONLY);
	if (fd < 0)
		throw Exception("device open failed.");

	input_id id;
	int io = ioctl(fd, EVIOCGID, &id);
	if (io < 0)
	{
		throw Exception("EVIOCGID failed.\n");
	}
	else
	{
		printf("\tbustype=%d, vendor=%d, product=%d, version=%d\n",
			id.bustype, id.vendor, id.product, id.version);
	}

	char name[255];
	io = ioctl(fd, EVIOCGNAME(255), name);
	if (io < 0)
	{
		throw Exception("EVIOCGNAME failed.\n");
	}
	else
	{
		printf("\tname=%s\n", name);
	}

	this->name = std::string(name);

	long arg = 1;
	io = ioctl(fd, EVIOCGRAB, arg);
	if (io < 0)
	{
		throw Exception("EVIOCGRAB failed.\n");
	}


	// Threading
	isRunning = true;

	int result_code = pthread_create(&thread, NULL, ThreadTrampoline, (void*)this);
	if (result_code != 0)
	{
		throw Exception("InputDevice pthread_create failed.\n");
	}
}

InputDevice::~InputDevice()
{
	isRunning = false;

	pthread_cancel(thread);

	pthread_join(thread, NULL);

	close(fd);
}


bool InputDevice::TryGetKeyPress(int* outValue)
{
	bool result;

	pthread_mutex_lock(&mutex);
	size_t count = keyQueue.size();

	if (count < 1)
	{
		pthread_mutex_unlock(&mutex);

		*outValue = 0;
		result = false;
	}
	else
	{
		*outValue = keyQueue.front();
		keyQueue.pop();
		result = true;

		pthread_mutex_unlock(&mutex);
	}

	return result;
}

void InputDevice::EnqueKey(int code)
{
	pthread_mutex_lock(&mutex);

	if (keyQueue.size() >= (size_t)MAX_QUEUE_SIZE)
	{
		pthread_mutex_unlock(&mutex);
		printf("InputDevice: EnqueKey failed (buffer full).\n");
	}
	else
	{
		keyQueue.push(code);
		pthread_mutex_unlock(&mutex);
	}
}

void InputDevice::WorkThread()
{
	input_event ev;

	printf("InputDevice entering running state.\n");

	int ret;

	ret = pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);
	if (ret != 0)
		throw Exception("pthread_setcanceltype failed.");

	ret = pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
	if (ret != 0)
		throw Exception("pthread_setcancelstate failed.");


	while (isRunning)
	{
		int count = read(fd, &ev, sizeof(ev));

		if (count != sizeof(ev))
		{
			// EINTR will happen when threaded                    
			if (count != -(EINTR))
				break;
		}
		else
		{
			//int value = ev.value;

			switch (ev.type)
			{
				case EV_SYN:
				{
					//unsigned short sync = ev.code;
				}
				break;


				case EV_KEY:
				{
					unsigned short keycode = ev.code;

					switch (ev.value)
					{
						case 0:
						{
							//RaiseKeyReleased(keyType);
							printf("%s: KeyReleased %d (0x%x)\n", name.c_str(), keycode, keycode);
						}
						break;

						case 1:
						{
							//RaiseKeyPressed(keyType);
							printf("%s: KeyPressed %d (0x%x)\n", name.c_str(), keycode, keycode);
							EnqueKey(keycode);
						}
						break;

#if 0
						case 2:
						{
							//RaiseKeyRepeated(keyType);
							printf("%s: KeyRepeated %d (0x%x)\n", name.c_str(), keycode, keycode);
							EnqueKey(keycode);
						}
						break;
#endif

						default:
						{
							// Ignore
						}
						break;
					}
				}
				break;


				case EV_REL:
				{
					//RelativeAxisType axis = (RelativeAxisType)evMarshalArray[0].code;
					//RaiseRelativeMoved(axis, value);
				}
				break;


				case EV_ABS:
				{
					//AbsoluteAxisType axis = (AbsoluteAxisType)evMarshalArray[0].code;
					//RaiseAbsoluteMoved(axis, value);
				}
				break;


				default:
					// Do nothing
					break;
			}
		}
	}

	printf("InputDevice exited running state.\n");
}
