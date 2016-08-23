#pragma once

#include <pthread.h>


class WaitCondition
{
	pthread_cond_t waitCondition = PTHREAD_COND_INITIALIZER;
	pthread_mutex_t waitMutex = PTHREAD_MUTEX_INITIALIZER;
	bool flag = false;


public:

	//void Lock()
	//{
	//	pthread_mutex_lock(&waitMutex);
	//}

	//void Unlock()
	//{
	//	pthread_mutex_unlock(&waitMutex);
	//}

	void Signal()
	{
		pthread_mutex_lock(&waitMutex);

		flag = true;
		pthread_cond_broadcast(&waitCondition);

		pthread_mutex_unlock(&waitMutex);
	}

	void WaitForSignal()
	{
		pthread_mutex_lock(&waitMutex);

		while (flag == false)
		{
			pthread_cond_wait(&waitCondition, &waitMutex);
		}

		flag = false;

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
