#pragma once

extern "C"
{
#include <libavformat/avformat.h>
}



class PacketBuffer
{
private:
	AVPacket* avpkt;
	double timeStamp = -1.0;

protected:



public:


	void* GetDataPtr() const
	{
		return avpkt->data;
	}

	int GetDataLength() const
	{
		return avpkt->size;
	}

	AVPacket* GetAVPacket() const
	{
		return avpkt;
	}

	double GetTimeStamp()
	{
		return timeStamp;
	}
	void SetTimeStamp(double value)
	{
		timeStamp = value;
	}


	PacketBuffer()
	{
		avpkt = (AVPacket*)calloc(1, sizeof(*avpkt));
		av_init_packet(avpkt);
		
		//pkt.data = NULL;
		//pkt.size = 0;
	}

	~PacketBuffer()
	{
		av_free_packet(avpkt);
		free(avpkt);
	}
};


