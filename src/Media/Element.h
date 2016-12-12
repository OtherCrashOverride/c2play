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





	void SetExecutionState(ExecutionStateEnum newState);


protected:

	bool IsRunning() const
	{
		return isRunning;
	}



	Element();


	
	virtual void Initialize();
	virtual void DoWork();
	virtual void Idling();
	virtual void Idled();
	virtual void Terminating();


	void State_Executing();
	void InternalWorkThread();
	void AddInputPin(InPinSPTR pin);
	void ClearInputPins();
	void AddOutputPin(OutPinSPTR pin);
	void ClearOutputPins();



public:

	InPinCollection* Inputs();
	OutPinCollection* Outputs();
	ExecutionStateEnum ExecutionState() const;
	bool IsExecuting() const;
	std::string Name() const;
	void SetName(std::string name);
	bool LogEnabled() const;
	void SetLogEnabled(bool value);
	virtual MediaState State() const;
	virtual void SetState(MediaState value);



	virtual ~Element();



	virtual void Execute();
	virtual void Wake();
	virtual void Terminate();
	virtual void ChangeState(MediaState oldState, MediaState newState);
	void WaitForExecutionState(ExecutionStateEnum state);
	virtual void Flush();

	// DEBUG
	void Log(const char* message, ...);
};


typedef std::shared_ptr<Element> ElementSPTR;
typedef std::weak_ptr<Element> ElementWPTR;
