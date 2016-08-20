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


void Thread::Start()
{
	int result_code = pthread_create(&thread, NULL, ThreadTrampoline, (void*)this);
	if (result_code != 0)
	{
		throw Exception("Sink pthread_create failed.\n");
	}
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
