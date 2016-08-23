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
//	-->	 Waiting
//	|	    |
//	|	    V
//	|   Initializing
//	|	    |
//	|	    V
//	|	Executing  <-->  Idle
//	|	    |             |
//	|	    V             |
//	---	Terminated <-------


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

	pthread_cond_t waitCondition = PTHREAD_COND_INITIALIZER;
	pthread_mutex_t waitMutex = PTHREAD_MUTEX_INITIALIZER;
	bool canSleep = true;

	std::string name = "Element";

	//pthread_cond_t executionWaitCondition = PTHREAD_COND_INITIALIZER;
	//pthread_mutex_t executionWaitMutex = PTHREAD_MUTEX_INITIALIZER;

	bool logEnabled = false;

	WaitCondition executionStateWaitCondition;



protected:

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
				if (newState != ExecutionStateEnum::Executing)
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
				throw InvalidOperationException();
				break;

			default:
				break;
			}


			//pthread_mutex_lock(&executionWaitMutex);

			executionState = newState;

			//pthread_cond_signal(&executionWaitCondition);

			//pthread_mutex_unlock(&executionWaitMutex);

			executionStateWaitCondition.Signal();
		}
	}



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



	void InternalWorkThread()
	{
		Log("Element (%s) InternalWorkThread entered.\n", name.c_str());

		SetExecutionState(ExecutionStateEnum::Initializing);
		Initialize();

		SetExecutionState(ExecutionStateEnum::Executing);
		while (executionState == ExecutionStateEnum::Executing)
		{			
			while (executionState == ExecutionStateEnum::Executing)
			{
				if (state == MediaState::Play)
				{
					DoWork();
				}


				if (executionState == ExecutionStateEnum::Executing)
				{
					Log("Element (%s) InternalWorkThread sleeping.\n", name.c_str());

					pthread_mutex_lock(&waitMutex);

					while (canSleep)
					{
						pthread_cond_wait(&waitCondition, &waitMutex);
					}

					canSleep = true;

					pthread_mutex_unlock(&waitMutex);

					Log("Element (%s) InternalWorkThread woke.\n", name.c_str());
				}
			}

			// Idle State
			while (executionState == ExecutionStateEnum::Idle)
			{
				executionStateWaitCondition.WaitForSignal();
			}
		}


		// Termination
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
		return executionState;
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

	virtual MediaState State()
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
	}



	virtual void Execute()
	{
		if (executionState != ExecutionStateEnum::WaitingForExecute &&
			executionState != ExecutionStateEnum::Idle)
		{
			throw InvalidOperationException();
		}

		if (executionState == ExecutionStateEnum::WaitingForExecute)
		{
			thread.Start();
		}
		else
		{
			SetExecutionState(ExecutionStateEnum::Executing);
		}

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

		SetExecutionState(ExecutionStateEnum::Terminating);
		Flush();

		thread.Cancel();
		thread.Join();

		Log("Element (%s) Terminate.\n", name.c_str());
	}


	virtual void ChangeState(MediaState oldState, MediaState newState)
	{
		// TODO: Allow to abort change?
		state = newState;

		Wake();

		Log("Element (%s) ChangeState oldState=%d newState=%d.\n", name.c_str(), (int)oldState, (int)newState);
	}



	void WaitForExecutionState(ExecutionStateEnum state)
	{
		while (executionState != state)
		{
			executionStateWaitCondition.WaitForSignal();
		}
	}

	virtual void Flush()
	{
		inputs.Flush();
		outputs.Flush();

		Log("Element (%s) Flush exited.\n", name.c_str());
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
