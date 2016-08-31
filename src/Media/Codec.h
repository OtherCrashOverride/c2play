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

extern "C"
{
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
}

#include <pthread.h>
#include <queue>
#include <memory>

#include "Exception.h"
#include "Thread.h"
#include "LockedQueue.h"
#include "Buffer.h"



enum class MediaState
{
	Pause = 0,
	Play
};


// Forward Declarations
class Element;
typedef std::shared_ptr<Element> ElementSPTR;
typedef std::weak_ptr<Element> ElementWPTR;

class Pin;
typedef std::shared_ptr<Pin> PinSPTR;

class InPin;
typedef std::shared_ptr<InPin> InPinSPTR;

class OutPin;
typedef std::shared_ptr<OutPin> OutPinSPTR;


//template<typename T>
//using EventFunction = std::function<void(void* sender, const T& args)>;
//
//
//
//
//typedef std::shared_ptr<EventArgs> EventArgsSPTR;
//
//
//class Event;
//typedef std::shared_ptr<Event> EventSPTR;
//typedef std::weak_ptr<Event> EventWPTR;
//
//





//class IElement
//{
//protected:
//	IElement() {}
//
//
//	virtual void Flush() = 0;
//
//
//public:
//	virtual ~IElement() {}
//
//
//	virtual void Execute() = 0;
//	virtual void Terminate() = 0;
//
//	//virtual MediaState PreviousState() = 0;
//
//	virtual MediaState State() = 0;
//	virtual void SetState(MediaState value) = 0;
//
//};
//typedef std::shared_ptr<IElement> IElementPTR;








// ----------------


//// Forward declartions to break cylclic dependencies
//class ISinkElement;
//typedef std::shared_ptr<ISinkElement> ISinkElementPTR;
//
//class ISourceElement;
//typedef std::shared_ptr<ISourceElement> ISourceElementPTR;

// -----

//class ISinkElement : public virtual IElement
//{
//protected:
//	ISinkElement() {}
//
//public:
//	virtual void AcceptConnection(ISourceElement* source) = 0;
//	virtual void Disconnect(ISourceElement* source) = 0;
//
//	virtual void ProcessBuffer(BufferPTR buffer) = 0;
//};



// ---------------

//class ISourceElement : public virtual IElement
//{
//protected:
//	ISourceElement() {}
//
//public:
//	//virtual BufferPTR GetAvailableBuffer() = 0;
//	virtual void AcceptProcessedBuffer(BufferPTR buffer) = 0;
//};




//class OutPin;
//typedef std::shared_ptr<OutPin> OutPinSPTR;

//class InPin;
//typedef std::shared_ptr<InPin> InPinSPTR;







////template <typename T>
//class SourceElement : public Element, public virtual ISourceElement
//{
//	ThreadSafeQueue<BufferPTR> filledBuffers;
//	ThreadSafeQueue<BufferPTR> availableBuffers;
//	ISinkElementPTR sink;
//	Mutex sinkMutex;
//
//protected:
//	
//	SourceElement()
//	{
//	}
//
//
//	void AddAvailableBuffer(BufferPTR buffer)
//	{
//		if (!buffer)
//			throw ArgumentNullException();
//
//		if (buffer->Owner() != (void*)this)
//		{
//			throw InvalidOperationException("The buffer being added does not belong to this object.");
//		}
//
//		availableBuffers.Push(buffer);
//	}
//
//	bool TryGetAvailableBuffer(BufferPTR* outValue)
//	{
//		return availableBuffers.TryPop(outValue);
//	}
//
//
//	void AddFilledBuffer(BufferPTR buffer)
//	{
//		if (!buffer)
//			throw ArgumentNullException();
//
//		if (buffer->Owner() != (void*)this)
//		{
//			throw InvalidOperationException("The buffer being added does not belong to this object.");
//		}
//
//		filledBuffers.Push(buffer);
//	}
//
//	virtual void Flush() override
//	{
//		BufferPTR buffer;
//		while (filledBuffers.TryPop(&buffer))
//		{
//			availableBuffers.Push(buffer);
//		}
//	}
//
//	virtual bool DoWork() override
//	{
//		sinkMutex.Lock();
//
//		if (sink)
//		{
//			BufferPTR buffer;
//			if (filledBuffers.TryPop(&buffer))
//			{
//				sink->ProcessBuffer(buffer);
//			}
//		}
//
//		sinkMutex.Unlock();
//	}
//
//
//	// ISourceElement
//	virtual void AcceptProcessedBuffer(BufferPTR buffer) override
//	{
//		if (!buffer)
//			throw ArgumentNullException();
//
//		if (buffer->Owner() != (void*)this)
//		{
//			throw InvalidOperationException("The buffer being returned does not belong to this object.");
//		}
//
//		availableBuffers.Push(buffer);
//	}
//
//public:
//	virtual ~SourceElement()
//	{
//		if (sink)
//		{
//			sink->Disconnect(this);
//		}
//	}
//
//
//	void Connect(ISinkElementPTR sink)
//	{
//		if (!sink)
//			throw ArgumentNullException();
//
//		if (IsExecuting())
//			throw InvalidOperationException();
//
//		
//		sinkMutex.Lock();
//
//		this->sink = sink;
//
//
//		// The interface does not use a smart pointer
//		// due to 'this' being required.
//		// TODO: Refactor to allow use of smart pointer
//		sink->AcceptConnection(this);
//
//		sinkMutex.Unlock();
//	}
//};
//
//
//// ----------
//
//class SinkElement : public Element, public virtual ISinkElement
//{
//	ISourceElement* source = nullptr;
//	Mutex sourceMutex;
//	ThreadSafeQueue<BufferPTR> filledBuffers;
//	ThreadSafeQueue<BufferPTR> processedBuffers;
//
//
//	void ReturnAllBuffers()
//	{
//		sourceMutex.Lock();
//
//		if (!source)
//			throw InvalidOperationException("Can not return buffers to a null source");
//
//		BufferPTR buffer;
//		while (processedBuffers.TryPop(&buffer))
//		{
//			source->AcceptProcessedBuffer(buffer);
//		}
//
//		while (filledBuffers.TryPop(&buffer))
//		{
//			source->AcceptProcessedBuffer(buffer);
//		}
//
//		sourceMutex.Unlock();
//	}
//
//protected:
//	SinkElement()
//	{
//
//	}
//
//	virtual ~SinkElement()
//	{
//		if (source)
//		{
//			ReturnAllBuffers();
//		}
//	}
//
//
//#if 1// Element
//	
//	virtual bool DoWork() override
//	{
//		sourceMutex.Lock();
//
//		if (source)
//		{
//			BufferPTR buffer;
//			while (processedBuffers.TryPop(&buffer))
//			{
//				source->AcceptProcessedBuffer(buffer);
//			}
//		}
//
//		sourceMutex.Unlock();
//	}
//
//#endif
//
//
//#if 1// ISinkElement
//
//	virtual void AcceptConnection(ISourceElement* source) override
//	{
//		if (!source)
//			throw ArgumentNullException();
//
//		if (IsExecuting())
//			throw InvalidOperationException("Can not connect while executing.");
//
//
//		ISourceElement* safeSource = this->source;
//		if (safeSource)
//		{
//			throw InvalidOperationException("A source is already connected.");
//		}
//
//		sourceMutex.Lock();
//		this->source = source;
//		sourceMutex.Unlock();
//	}
//	virtual void Disconnect(ISourceElement* source) override
//	{
//		ISourceElement* safeSource = this->source;
//
//		if (!source)
//			throw ArgumentNullException();
//
//		if (!safeSource)
//			throw InvalidOperationException("No source is connected.");
//
//		if (safeSource != source)
//			throw InvalidOperationException("Attempt to disconnect a source that is not connected.");
//
//		if (IsExecuting())
//			throw InvalidOperationException("Can not disconnect while executing.");
//
//		
//		ReturnAllBuffers();
//
//		sourceMutex.Lock();
//		source = nullptr;
//		sourceMutex.Unlock();
//	}
//
//	virtual void ProcessBuffer(BufferPTR buffer) override
//	{
//		if (!buffer)
//			throw ArgumentNullException();
//
//		filledBuffers.Push(buffer);
//	}
//
//#endif
//
//};

// ----------

// abstract base class
class Sink
{
	const int MAX_BUFFERS = 128;

	std::queue<AVPacketBufferPtr> buffers;
	pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
	pthread_t thread;
	bool isThreadStarted = false;
	bool isRunning = false;
	MediaState state = MediaState::Pause;


	static void* ThreadTrampoline(void* argument)
	{
		Sink* ptr = (Sink*)argument;
		ptr->WorkThread();
	}

protected:
	
	bool IsRunning() const
	{
		return isRunning;
	}
	

	
	Sink() {}


	virtual bool TryGetBuffer(AVPacketBufferPtr* outValue)
	{
		bool result;

		pthread_mutex_lock(&mutex);
		size_t count = buffers.size();

		if (count < 1)
		{
			pthread_mutex_unlock(&mutex);
			
			(*outValue).reset();
			result = false;

			//printf("TryGetBuffer failed.\n");
		}
		else
		{
			*outValue = buffers.front();
			buffers.pop();
			result = true;

			pthread_mutex_unlock(&mutex);

			//printf("TryGetBuffer success.\n");
		}

		return result;
	}

	virtual void WorkThread() = 0;


public:

	virtual ~Sink()
	{
		isRunning = false;

		if (isThreadStarted)
		{
			pthread_join(thread, NULL);
		}
	}


	virtual void AddBuffer(AVPacketBufferPtr buffer)
	{
		if (!isThreadStarted)
			throw InvalidOperationException();


		size_t count;

		while (isRunning)
		{
			pthread_mutex_lock(&mutex);
			count = buffers.size();

			if (count >= MAX_BUFFERS)
			{
				pthread_mutex_unlock(&mutex);

				usleep(1);
			}
			else
			{
				buffers.push(buffer);

				pthread_mutex_unlock(&mutex);
				break;
			}
		}
	}

	virtual void Start()
	{
		if (isThreadStarted)
			throw InvalidOperationException();

		// ----- start decoder -----
		isRunning = true;

		int result_code = pthread_create(&thread, NULL, ThreadTrampoline, (void*)this);
		if (result_code != 0)
		{
			throw Exception("Sink pthread_create failed.\n");
		}

		isThreadStarted = true;
	}

	virtual void Stop()
	{
		if (!isThreadStarted)
			throw InvalidOperationException();

		Flush();

		isRunning = false;

		pthread_join(thread, NULL);

		isThreadStarted = false;
	}


	virtual MediaState State()
	{
		//if (!isRunning)
		//	throw InvalidOperationException();

		return state;
	}
	virtual void SetState(MediaState state)
	{
		//if (!isRunning)
		//	throw InvalidOperationException();

		this->state = state;
	}

	virtual void Flush()
	{
		pthread_mutex_lock(&mutex);

		while (buffers.size() > 0)
		{
			buffers.pop();
		}

		pthread_mutex_unlock(&mutex);
	}
};

typedef std::shared_ptr<Sink> SinkPtr;


class IClockSource
{
protected:
	IClockSource() {}

public:
	virtual ~IClockSource() {}

	virtual double GetTimeStamp() = 0;
};

typedef std::shared_ptr<IClockSource> IClockSourcePtr;


class IClockSink
{
protected:
	IClockSink() {}

public:
	virtual ~IClockSink() {}

	virtual void SetTimeStamp(double value) = 0;
};

typedef std::shared_ptr<IClockSink> IClockSinkPtr;



//class Clock
//{
//
//public:
//	Clock(IClockSink)
//};


//class TestClockSink : public virtual IClockSink
//{
//	const unsigned long PTS_FREQ = 90000;
//
//	codec_para_t* codecContext;
//
//
//public:
//	TestClockSink(codec_para_t* codecContext)
//		: codecContext(codecContext)
//	{
//		if (codecContext == nullptr)
//			throw ArgumentNullException();
//	}
//
//
//	// Inherited via IClockSink
//	virtual void SetTimeStamp(double value) override
//	{
//		unsigned long pts = (unsigned long)(value * PTS_FREQ);
//
//		int codecCall = codec_set_pcrscr(codecContext, (int)pts);
//		if (codecCall != 0)
//		{
//			printf("codec_set_pcrscr failed.\n");
//		}
//	}
//};