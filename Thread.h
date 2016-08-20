#pragma once

#include <pthread.h>
#include <functional>




class Thread
{
	pthread_t thread;
	std::function<void()> function;


	static void* ThreadTrampoline(void* argument);


public:
	Thread(std::function<void()> function);


	void Start();

	void Cancel();

	void Join();


	static void SetCancelEnabled(bool value);

	static void SetCancelTypeDeferred(bool value);

};
