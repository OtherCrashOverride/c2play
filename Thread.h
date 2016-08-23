#pragma once

#include <pthread.h>
#include <functional>
#include <memory>



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

