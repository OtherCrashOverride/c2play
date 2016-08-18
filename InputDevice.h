#pragma once

#include <pthread.h>
#include <queue>
#include <string>
#include <linux/input.h>
#include <linux/uinput.h>


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


	static void* ThreadTrampoline(void* argument)
	{
		InputDevice* ptr = (InputDevice*)argument;
		ptr->WorkThread();
	}


public:

	//bool IsRunning() const
	//{
	//	return isRunning;
	//}
	//void SetIsRunning(bool value)
	//{
	//	isRunning = value;
	//}

	const std::string& FileName() const
	{
		return fileName;
	}

	const std::string& Name() const
	{
		return name;
	}


	InputDevice(std::string fileName)
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

	~InputDevice()
	{
		isRunning = false;
		
		pthread_cancel(thread);

		pthread_join(thread, NULL);

		close(fd);
	}


	bool TryGetKeyPress(int* outValue)
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

	void EnqueKey(int code)
	{
		pthread_mutex_lock(&mutex);

		if (keyQueue.size() >= MAX_QUEUE_SIZE)
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

	void WorkThread()
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
							printf("KeyReleased (%x)\n", keycode);
						}
						break;

						case 1:
						{
							//RaiseKeyPressed(keyType);
							printf("KeyPressed (%x)\n", keycode);
							EnqueKey(keycode);
						}
						break;

						case 2:
						{
							//RaiseKeyRepeated(keyType);
							printf("KeyRepeated (%x)\n", keycode);
							EnqueKey(keycode);
						}
						break;

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
};

typedef std::shared_ptr<InputDevice> InputDevicePtr;
