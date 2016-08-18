#pragma once

extern "C"
{
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
}

#include <pthread.h>
#include <queue>


#include "Exception.h"


enum class MediaState
{
	Pause = 0,
	Play
};


// abstract base class
class Sink
{
	const int MAX_BUFFERS = 128;

	std::queue<PacketBufferPtr> buffers;
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


	virtual bool TryGetBuffer(PacketBufferPtr* outValue)
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


	virtual void AddBuffer(PacketBufferPtr buffer)
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