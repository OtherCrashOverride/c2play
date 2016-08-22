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


