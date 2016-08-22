#pragma once

#include "Exception.h"
//#include "Codec.h"

extern "C"
{
#include <libavformat/avformat.h>
}

#include <memory>


enum class BufferTypeEnum
{
	Unknown = 0,
	Marker,
	ClockData,
	PcmData,
	AVPacket,
	AVFrame
};


class Element;
typedef std::shared_ptr<Element> ElementSPTR;


// abstract
class Buffer
{
	BufferTypeEnum type;
	ElementSPTR owner;


protected:
	//Buffer() {}
	
	Buffer(BufferTypeEnum type, ElementSPTR owner)
		: type(type), owner(owner)
	{
	}

public:
	BufferTypeEnum Type()
	{
		return type;
	}

	ElementSPTR Owner() const
	{
		return owner;
	}


	virtual ~Buffer() {};


	// TODO: Remove DataPtr/DataLength
	//		 since they only make sense if
	//		 the buffer payload is understood
	virtual void* DataPtr() = 0;
	virtual int DataLength() = 0;

	virtual double TimeStamp() = 0;

};

//typedef std::shared_ptr<Buffer> BufferPTR;	// TODO: Change type to Buffer*
typedef std::shared_ptr<Buffer> BufferSPTR;
typedef std::unique_ptr<Buffer> BufferUPTR;
typedef std::weak_ptr<Buffer> BufferWPTR;



enum class MarkerEnum
{
	Unknown = 0,
	EndOfStream,
	//Play,
	//Pause
};

class MarkerBuffer : public Buffer
{
	//double timeStamp = 0;
	MarkerEnum status;

public:
	
	MarkerEnum Marker() const
	{
		return status;
	}
	
	
	MarkerBuffer(ElementSPTR owner, MarkerEnum status)
		: Buffer(BufferTypeEnum::Marker, owner),
		status(status)
	{
	}


	virtual void* DataPtr() override
	{
		return nullptr;
	}

	virtual int DataLength() override
	{
		return 0;
	}

	virtual double TimeStamp() override
	{
		// Time stamps are not significant
		// since the buffer is temporally located
		// in the stream
		return 0;
	}
	//void SetTimeStamp(double value)
	//{
	//	timeStamp = value;
	//}
};

typedef std::shared_ptr<MarkerBuffer> MarkerBufferSPTR;



template<typename T>
class GenericBuffer : public Buffer
{
	T payload;

protected:
	//GenericBuffer(T payload)
	//	: payload(payload)
	//{
	//}
	GenericBuffer(BufferTypeEnum type, ElementSPTR owner, T payload)
		: Buffer(type, owner), payload(payload)
	{
	}

	virtual T Payload()
	{
		return payload;
	}

public:


};


class ClockData
{
public:
	double TimeStamp = -1;

};

class ClockDataBuffer : public GenericBuffer<ClockData*>
{
	ClockData clockData;

public:
	ClockDataBuffer(ElementSPTR owner)
		: GenericBuffer(BufferTypeEnum::ClockData, owner, &clockData)
	{
	}


	virtual void* DataPtr() override
	{
		return Payload();
	}

	virtual int DataLength() override
	{
		return sizeof(*Payload());
	}

	virtual double TimeStamp() override
	{
		return clockData.TimeStamp;
	}
	void SetTimeStamp(double value)
	{
		clockData.TimeStamp = value;
	}
};

typedef std::shared_ptr<ClockDataBuffer> ClockDataBufferSPTR;


enum class PcmFormat
{
	Unknown = 0,
	Int16,
	Int16Planes,
	Int32,
	Int32Planes,
	Float32,
	Float32Planes
};

class PcmData
{
public:
	static const int MAX_CHANNELS = 8;


	int Channels = 0;
	PcmFormat Format = PcmFormat::Unknown;
	int Samples = 0;
	void* Channel[MAX_CHANNELS] = { 0 };
	int ChannelSize = 0;
};

class PcmDataBuffer : public GenericBuffer<PcmData*>
{
	PcmData pcmData;
	double timeStamp = -1;


	static int GetSampleSize(PcmFormat format)
	{
		int sampleSize;

		switch (format)
		{
		case PcmFormat::Int16:
		case PcmFormat::Int16Planes:
			sampleSize = 2;
			break;

		case PcmFormat::Int32:
		case PcmFormat::Int32Planes:
		case PcmFormat::Float32:
		case PcmFormat::Float32Planes:
			sampleSize = 4;
			break;

		default:
			throw NotSupportedException();
			break;
		}

		return sampleSize;
	}

	static bool IsInterleaved(PcmFormat format)
	{
		int result;

		switch (format)
		{
		case PcmFormat::Int16:
		case PcmFormat::Int32:
		case PcmFormat::Float32:		
			result = true;
			break;
		
		case PcmFormat::Int16Planes:
		case PcmFormat::Int32Planes:
		case PcmFormat::Float32Planes:
			result = false;
			break;

		default:
			throw NotSupportedException();
			break;
		}

		return result;
	}


public:
	//PcmDataBuffer( PcmFormat format, int channels, int samples)
	//	: PcmDataBuffer(nullptr, format, channels, samples)
	//{
	//}

	PcmDataBuffer(ElementSPTR owner, PcmFormat format, int channels, int samples)
		: GenericBuffer(BufferTypeEnum::PcmData, owner, &pcmData)

	{
		//printf("PcmDataBuffer ctor: format=%d, channels=%d, samples=%d\n",
		//	(int)format, channels, samples);

		if (channels < 1 ||
			channels > PcmData::MAX_CHANNELS)
		{
			throw ArgumentOutOfRangeException();
		}

		if (samples < 0)
			throw ArgumentOutOfRangeException();


		pcmData.Format = (format);
		pcmData.Channels = channels;
		pcmData.Samples = (samples);


		if (IsInterleaved(format))
		{
			//printf("PcmDataBuffer ctor: IsInterleaved=true\n");

			int size = samples * GetSampleSize(format) * channels;
			pcmData.Channel[0] = malloc(size);
			pcmData.ChannelSize = size;
		}
		else
		{
			//printf("PcmDataBuffer ctor: IsInterleaved=false\n");

			int size = samples * GetSampleSize(format);
			pcmData.ChannelSize = size;
			//printf("PcmDataBuffer ctor: pcmData.ChannelSize=%d\n", pcmData.ChannelSize);

			for (int i = 0; i < channels; ++i)
			{
				pcmData.Channel[i] = malloc(size);

				//printf("PcmDataBuffer ctor: pcmData.Channel[%d]=%p\n", i, pcmData.Channel[i]);
			}
		}
	}
	~PcmDataBuffer()
	{
		for (int i = 0; i < PcmData::MAX_CHANNELS; ++i)
		{
			free(pcmData.Channel[i]);
		}
	}


	virtual void* DataPtr() override
	{
		return pcmData.Channel;
	}

	virtual int DataLength() override
	{
		return pcmData.Channels * sizeof(void*);
	}

	virtual double TimeStamp() override
	{
		return timeStamp;
	}
	void SetTimeStamp(double value)
	{
		timeStamp = value;
	}


	PcmData* GetPcmData()
	{
		return Payload();
	}
};

typedef std::shared_ptr<PcmDataBuffer> PcmDataBufferPtr;
typedef std::shared_ptr<PcmDataBuffer> PcmDataBufferSPTR;



class AVPacketBuffer : public GenericBuffer<AVPacket*>
{
	double timeStamp = -1;
	AVRational time_base;


	static AVPacket* CreatePayload()
	{
		AVPacket* avpkt;
		avpkt = (AVPacket*)calloc(1, sizeof(*avpkt));
		
		av_init_packet(avpkt);


		return avpkt;
	}


public:
	//AVPacketBuffer()
	//	: GenericBuffer(CreatePayload())
	//{
	//	time_base.num = 1;
	//	time_base.den = 1;
	//}
	AVPacketBuffer(ElementSPTR owner)
		: GenericBuffer(BufferTypeEnum::AVPacket, owner, CreatePayload())
	{
		time_base.num = 1;
		time_base.den = 1;
	}

	~AVPacketBuffer()
	{
		AVPacket* avpkt = Payload();

		av_free_packet(avpkt);
		free(avpkt);
	}


	virtual void* DataPtr() override
	{
		return Payload()->data;
	}

	virtual int DataLength() override
	{
		return Payload()->size;
	}

	virtual double TimeStamp() override
	{
		return timeStamp;
	}
	void SetTimeStamp(double value)
	{
		timeStamp = value;
	}


	// Named GetAVPacket instead of AVPacket
	// to avoid compiler name collisions
	AVPacket* GetAVPacket()
	{
		return Payload();
	}

	void Reset()
	{
		AVPacket* avpkt = Payload();

		av_free_packet(avpkt);
		av_init_packet(avpkt);
		avpkt->data = NULL;
		avpkt->size = 0;

		time_base.num = 1;
		time_base.den = 1;

		timeStamp = -1;
	}

	AVRational TimeBase()
	{
		return time_base;
	}
	void SetTimeBase(AVRational value)
	{
		time_base = value;
	}

};

typedef std::shared_ptr<AVPacketBuffer> AVPacketBufferPtr;
typedef std::shared_ptr<AVPacketBuffer> AVPacketBufferPTR;
typedef std::shared_ptr<AVPacketBuffer> AVPacketBufferSPTR;


class AVFrameBuffer : public GenericBuffer<AVFrame*>
{
	//AVRational timeBase;
	double timeStamp = -1;


	static AVFrame* CreatePayload()
	{
		AVFrame* frame = av_frame_alloc();
		if (!frame)
		{
			throw Exception("av_frame_alloc failed.\n");			
		}

		return frame;
	}


public:
	//AVFrameBuffer(AVRational timeBase)
	//	: GenericBuffer(CreatePayload()),
	//	timeBase(timeBase)
	//{
	//}
	AVFrameBuffer(ElementSPTR owner)
		: GenericBuffer(BufferTypeEnum::AVFrame, owner, CreatePayload()),
		timeStamp(timeStamp)
	{
		//printf("AVFrameBuffer ctor: payload=%p, timeStamp=%f\n", Payload(), timeStamp);
	}
	~AVFrameBuffer()
	{
		AVFrame* frame = Payload();
		av_frame_free(&frame);
	}


	virtual void* DataPtr() override
	{
		return Payload()->data;
	}

	virtual int DataLength() override
	{
		// DataPtr is an array of pointers.
		// DataLength is the bytelength of that array
		// not the actual data elements.
		return AV_NUM_DATA_POINTERS * sizeof(void*);
	}

	virtual double TimeStamp() override
	{
		return timeStamp;
	}
	void SetTimeStamp(double value)
	{
		timeStamp = value;
	}

	// Named GetAVFrame() instead of AVFrame()
	// to avoid compiler name collisions
	AVFrame* GetAVFrame()
	{
		return Payload();
	}
};

typedef std::shared_ptr<AVFrameBuffer> AVFrameBufferPtr;
typedef std::shared_ptr<AVFrameBuffer> AVFrameBufferSPTR;


//class PacketBuffer
//{
//private:
//	AVPacket* avpkt;
//	double timeStamp = -1.0;
//
//protected:
//
//
//
//public:
//
//
//	void* GetDataPtr() const
//	{
//		return avpkt->data;
//	}
//
//	int GetDataLength() const
//	{
//		return avpkt->size;
//	}
//
//	AVPacket* GetAVPacket() const
//	{
//		return avpkt;
//	}
//
//	double GetTimeStamp()
//	{
//		return timeStamp;
//	}
//	void SetTimeStamp(double value)
//	{
//		timeStamp = value;
//	}
//
//
//	PacketBuffer()
//	{
//		avpkt = (AVPacket*)calloc(1, sizeof(*avpkt));
//		av_init_packet(avpkt);
//		
//		//pkt.data = NULL;
//		//pkt.size = 0;
//	}
//
//	~PacketBuffer()
//	{
//		av_free_packet(avpkt);
//		free(avpkt);
//	}
//};
//
//
//typedef std::shared_ptr<PacketBuffer> PacketBufferPtr;
//
