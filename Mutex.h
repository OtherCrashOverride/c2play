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


class WaitCondition
{
	pthread_cond_t waitCondition = PTHREAD_COND_INITIALIZER;
	pthread_mutex_t waitMutex = PTHREAD_MUTEX_INITIALIZER;


public:

	void Signal()
	{
		pthread_mutex_lock(&waitMutex);
		pthread_cond_signal(&waitCondition);
		pthread_mutex_unlock(&waitMutex);
	}

	void Wait()
	{
		pthread_mutex_lock(&waitMutex);
		pthread_cond_wait(&waitCondition, &waitMutex);
		pthread_mutex_unlock(&waitMutex);
	}

	//bool WaitTimeout(double seconds)
	//{
	//	pthread_timestruc_t absoluteTime;
	//	absoluteTime.tv_sec = time(NULL) + TIMEOUT;
	//	absoluteTime.tv_nsec = 0;

	//	pthread_mutex_lock(&waitMutex);
	//	pthread_cond_timedwait(&waitCondition, &waitMutex, &abstime);
	//	pthread_mutex_unlock(&waitMutex);
	//}

	//TODO: WaitPredicate
};
