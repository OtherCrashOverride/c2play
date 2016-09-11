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


#include <pthread.h>
#include <sys/time.h>

#include "Codec.h"
#include "InPin.h"
#include "OutPin.h"
#include "WaitCondition.h"


// Push model
//
// source.Execute()
// source.Connect(sink)
//										->	sink.AcceptConnection(source)
//
// while(IsRunning())
//										->	sink.ProcessBuffer(buffer)
//		source.AcceptProcessedBuffer()	<-
// loop
//
// source.Connect(nullptr)				->  sink.Disconnect(source)
//


// Execution Flow:
//	-->WaitingForExecute
//	|	    |
//	|	    V
//	|   Initializing
//	|	    |
//	|	    |<---------------------------------------------
//	|	    V                                             |
//	|	  Idle  --> [Play] -->  Executing --> [Pause] -----
//	|	    |                       |
//	|	    V                       |
//	---	Terminating <----------------


enum class ExecutionStateEnum
{
	WaitingForExecute = 0,
	Initializing,
	Executing,
	Idle,
	Terminating
};


class Element : public std::enable_shared_from_this<Element>
{
	Thread thread = Thread(std::function<void()>(std::bind(&Element::InternalWorkThread, this)));
	MediaState state = MediaState::Pause;
	InPinCollection inputs;
	OutPinCollection outputs;
	ExecutionStateEnum executionState = ExecutionStateEnum::WaitingForExecute;
	//ExecutionStateEnum desiredExecutionState = ExecutionStateEnum::WaitingForExecute;

	pthread_cond_t waitCondition = PTHREAD_COND_INITIALIZER;
	pthread_mutex_t waitMutex = PTHREAD_MUTEX_INITIALIZER;
	bool canSleep = true;

	std::string name = "Element";

	pthread_cond_t executionWaitCondition = PTHREAD_COND_INITIALIZER;
	pthread_mutex_t executionWaitMutex = PTHREAD_MUTEX_INITIALIZER;

	bool logEnabled = false;
	bool isRunning = false;

	//Mutex playPauseMutex;

	//WaitCondition executionStateWaitCondition;


	//void SignalExecutionWait(ExecutionStateEnum newState)
	//{
	//	pthread_mutex_lock(&executionWaitMutex);

	//	canSleep = false;

	//	if (pthread_cond_broadcast(&executionWaitCondition) != 0)
	//	{
	//		throw Exception("Element::SignalExecutionWait - pthread_cond_broadcast failed.");
	//	}

	//	pthread_mutex_unlock(&executionWaitMutex);
	//}

	void SetExecutionState(ExecutionStateEnum newState)
	{
		ExecutionStateEnum current = executionState;

		if (newState != current)
		{
			switch (current)
			{
			case ExecutionStateEnum::WaitingForExecute:
				if (newState != ExecutionStateEnum::Initializing)
				{
					throw InvalidOperationException();
				}
				break;

			case ExecutionStateEnum::Initializing:
				if (newState != ExecutionStateEnum::Idle)
				{
					throw InvalidOperationException();
				}
				break;

			case ExecutionStateEnum::Executing:
				if (newState != ExecutionStateEnum::Idle &&
					newState != ExecutionStateEnum::Terminating)
				{
					throw InvalidOperationException();
				}
				break;

			case ExecutionStateEnum::Idle:
				if (newState != ExecutionStateEnum::Executing &&
					newState != ExecutionStateEnum::Terminating)
				{
					throw InvalidOperationException();
				}
				break;

			case ExecutionStateEnum::Terminating:
				if (newState != ExecutionStateEnum::WaitingForExecute)
				{
					throw InvalidOperationException();
				}
				break;

			default:
				break;
			}

			//executionState = newState;
		}

		// Signal even if already set
		//executionStateWaitCondition.Signal();

		pthread_mutex_lock(&executionWaitMutex);

		executionState = newState;
		//desiredExecutionState = newState;

		if (pthread_cond_broadcast(&executionWaitCondition) != 0)
		{
			throw Exception("Element::SignalExecutionWait - pthread_cond_broadcast failed.");
		}

		pthread_mutex_unlock(&executionWaitMutex);

		//Wake();

		Log("Element: %s Set ExecutionState=%d\n", name.c_str(), (int)newState);
	}


protected:

	Element()
	{
	}


	
	virtual void Initialize()
	{
	}

	virtual void DoWork()
	{
		Log("Element (%s) DoWork exited.\n", name.c_str());
	}

	virtual void Idling()
	{
	}

	virtual void Idled()
	{
	}

	virtual void Terminating()
	{
	}


	//void ChangeExecutionState(ExecutionStateEnum state)
	//{
	//	pthread_mutex_lock(&executionWaitMutex);

	//	desiredExecutionState = state;

	//	if (pthread_cond_broadcast(&executionWaitCondition) != 0)
	//	{
	//		throw Exception("Element::SignalExecutionWait - pthread_cond_broadcast failed.");
	//	}

	//	pthread_mutex_unlock(&executionWaitMutex);

	//	Wake();
	//}


	void State_Executing()
	{
		// Executing state
		// state == MediaState::Play
		//while (desiredExecutionState == ExecutionStateEnum::Executing)
		while (isRunning)
		{
			//playPauseMutex.Lock();

			if (state == MediaState::Play)
			{
				if (ExecutionState() != ExecutionStateEnum::Executing)
				{
					//printf("Element (%s) InternalWorkThread woke.\n", name.c_str());
					SetExecutionState(ExecutionStateEnum::Executing);
				}

				DoWork();
			}

			//playPauseMutex.Unlock();


			if (!isRunning)
				break;


			pthread_mutex_lock(&waitMutex);

			while (canSleep)
			{
				if (ExecutionState() != ExecutionStateEnum::Idle)
				{
					//printf("Element (%s) InternalWorkThread sleeping.\n", name.c_str());
					SetExecutionState(ExecutionStateEnum::Idle);
				}

				pthread_cond_wait(&waitCondition, &waitMutex);
			}

			canSleep = true;

			pthread_mutex_unlock(&waitMutex);
		}
	}

	//void State_Idle()
	//{
	//	// Idle State
	//	pthread_mutex_lock(&executionWaitMutex);

	//	while (desiredExecutionState == ExecutionStateEnum::Idle)
	//	{
	//		pthread_cond_wait(&executionWaitCondition, &executionWaitMutex);
	//	}

	//	pthread_mutex_unlock(&executionWaitMutex);
	//}

	void InternalWorkThread()
	{
		Log("Element (%s) InternalWorkThread entered.\n", name.c_str());

		Thread::SetCancelTypeDeferred(false);
		Thread::SetCancelEnabled(true);

		SetExecutionState(ExecutionStateEnum::Initializing);
		Initialize();

		/*SetExecutionState(ExecutionStateEnum::Idle);*/
		
#if 0
		while (executionState == ExecutionStateEnum::Idle ||
			   executionState == ExecutionStateEnum::Executing)
		{
			// Executing state
			while (executionState == ExecutionStateEnum::Executing)
			{				
				DoWork();

				pthread_mutex_lock(&waitMutex);
				
				if (executionState == ExecutionStateEnum::Executing)
				{
					//printf("Element (%s) InternalWorkThread sleeping.\n", name.c_str());

					while (canSleep)
					{
						pthread_cond_wait(&waitCondition, &waitMutex);
					}

					canSleep = true;

					//printf("Element (%s) InternalWorkThread woke.\n", name.c_str());
				}

				pthread_mutex_unlock(&waitMutex);

			}
			
			
			// Idle State
			pthread_mutex_lock(&executionWaitMutex);

			if (executionState == ExecutionStateEnum::Idle)
			{
				//printf("Element %s Idling\n", name.c_str());

				Idling();
				
				while (executionState == ExecutionStateEnum::Idle)
				{
					//executionStateWaitCondition.WaitForSignal();
					pthread_cond_wait(&executionWaitCondition, &executionWaitMutex);
				}

				Idled();

				//printf("Element %s resumed from Idle\n", name.c_str());
			}
			
			pthread_mutex_unlock(&executionWaitMutex);

			
		}


		// Termination
		SetExecutionState(ExecutionStateEnum::WaitingForExecute);
		WaitForExecutionState(ExecutionStateEnum::WaitingForExecute);

		Terminating();
#endif

		//while (true)
		//{
		//	printf("Element (%s) InternalWorkThread - Idle\n", name.c_str());
		//	SetExecutionState(ExecutionStateEnum::Idle);
		//	State_Idle();

		//	if (desiredExecutionState == ExecutionStateEnum::Terminating)
		//		break;

		//	printf("Element (%s) InternalWorkThread - Executing\n", name.c_str());
		//	SetExecutionState(ExecutionStateEnum::Executing);
		//	State_Executing();

		//	if (desiredExecutionState == ExecutionStateEnum::Terminating)
		//		break;
		//}


		SetExecutionState(ExecutionStateEnum::Idle);
		State_Executing();


		printf("Element (%s) InternalWorkThread - Terminating\n", name.c_str());
		SetExecutionState(ExecutionStateEnum::Terminating);
		Terminating();

		SetExecutionState(ExecutionStateEnum::WaitingForExecute);

		printf("Element (%s) InternalWorkThread exited.\n", name.c_str());
	}


	void AddInputPin(InPinSPTR pin)
	{
		inputs.Add(pin);
	}
	void ClearInputPins()
	{
		inputs.Clear();
	}

	void AddOutputPin(OutPinSPTR pin)
	{
		outputs.Add(pin);
	}
	void ClearOutputPins()
	{
		outputs.Clear();
	}




public:

	InPinCollection* Inputs()
	{
		return &inputs;
	}
	OutPinCollection* Outputs()
	{
		return &outputs;
	}

	ExecutionStateEnum ExecutionState() const
	{
		ExecutionStateEnum result;

		// Probably not necessary to lock since
		// the read should be atomic at the hardware
		// level.
		//pthread_mutex_lock(&waitMutex);
		result = executionState;
		//pthread_mutex_unlock(&waitMutex);
		
		return result;
	}

	bool IsExecuting() const
	{
		//return desiredExecutionState == ExecutionStateEnum::Executing;
		return State() == MediaState::Play && 
			   ExecutionState() == ExecutionStateEnum::Executing;
	}

	std::string Name() const
	{
		return name;
	}
	void SetName(std::string name)
	{
		this->name = name;
	}

	bool LogEnabled() const
	{
		return logEnabled;
	}
	void SetLogEnabled(bool value)
	{
		logEnabled = value;
	}

	virtual MediaState State() const
	{
		return state;
	}
	virtual void SetState(MediaState value)
	{
		if (state != value)
		{
			ChangeState(state, value);
		}
	}



	virtual ~Element()
	{
		printf("Element %s destructed.\n", name.c_str());
	}



	virtual void Execute()
	{
		if (executionState != ExecutionStateEnum::WaitingForExecute)
		{
			throw InvalidOperationException();
		}


		isRunning = true;
		thread.Start();


		Log("Element (%s) Execute.\n", name.c_str());
	}

	virtual void Wake()
	{
		pthread_mutex_lock(&waitMutex);

		canSleep = false;

		//pthread_cond_signal(&waitCondition);
		if (pthread_cond_broadcast(&waitCondition) != 0)
		{
			throw Exception("Element::Wake - pthread_cond_broadcast failed.");
		}

		pthread_mutex_unlock(&waitMutex);

		Log("Element (%s) Wake.\n", name.c_str());
	}

	virtual void Terminate()
	{
		if (executionState != ExecutionStateEnum::Executing &&
			executionState != ExecutionStateEnum::Idle)
		{
			throw InvalidOperationException();
		}

		SetState(MediaState::Pause);
		WaitForExecutionState(ExecutionStateEnum::Idle);

		Flush();

		isRunning = false;
		Wake();

		WaitForExecutionState(ExecutionStateEnum::WaitingForExecute);
		

		//thread.Cancel();

		printf("Element (%s) thread.Join().\n", name.c_str());
		thread.Join();

		printf("Element (%s) Terminate.\n", name.c_str());
	}


	virtual void ChangeState(MediaState oldState, MediaState newState)
	{
		//playPauseMutex.Lock();

		state = newState;

		//playPauseMutex.Unlock();

		//if (ExecutionState() == ExecutionStateEnum::Executing ||
		//	ExecutionState() == ExecutionStateEnum::Idle)
		//{
		//	// Note: Can not WaitForExecutionState since its
		//	// called inside the thread making it block forever.
		//	switch (newState)
		//	{
		//		case MediaState::Pause:
		//			ChangeExecutionState(ExecutionStateEnum::Idle);
		//			//WaitForExecutionState(ExecutionStateEnum::Idle);
		//			break;

		//		case MediaState::Play:
		//			ChangeExecutionState(ExecutionStateEnum::Executing);
		//			//WaitForExecutionState(ExecutionStateEnum::Executing);
		//			break;

		//		default:
		//			throw NotSupportedException();
		//	}
		//}
		//else
		//{
		//	throw InvalidOperationException();
		//}

		Wake();

		printf("Element (%s) ChangeState oldState=%d newState=%d.\n", name.c_str(), (int)oldState, (int)newState);
	}



	void WaitForExecutionState(ExecutionStateEnum state)
	{
		//while (executionState != state)
		//{
			//printf("Element %s: WaitForExecutionState - executionState=%d, waitingFor=%d\n",
			//	name.c_str(), (int)executionState, (int)state);

		//	executionStateWaitCondition.WaitForSignal();
		//}

		//Wake();

		pthread_mutex_lock(&executionWaitMutex);

		while (executionState != state)
		{
			pthread_cond_wait(&executionWaitCondition, &executionWaitMutex);
		}

		pthread_mutex_unlock(&executionWaitMutex);

		//printf("Element %s: Finished WaitForExecutionState - executionState=%d\n",
		//	name.c_str(), (int)state);
	}

	virtual void Flush()
	{
		if (State() != MediaState::Pause)
			throw InvalidOperationException();

		WaitForExecutionState(ExecutionStateEnum::Idle);

		inputs.Flush();
		outputs.Flush();
		

		printf("Element (%s) Flush exited.\n", name.c_str());
	}

	// DEBUG
	void Log(const char* message, ...)
	{
		if (logEnabled)
		{
			struct timeval tp;
			gettimeofday(&tp, NULL);
			double ms = tp.tv_sec + tp.tv_usec * 0.0001;

			char text[1024];
			sprintf(text, "[%s : %f] %s", Name().c_str(), ms, message);


			va_list argptr;
			va_start(argptr, message);
			vfprintf(stderr, text, argptr);
			va_end(argptr);
		}
	}
};


typedef std::shared_ptr<Element> ElementSPTR;
typedef std::weak_ptr<Element> ElementWPTR;
