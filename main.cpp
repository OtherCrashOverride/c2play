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

#include <stdio.h>

#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <vector>
#include <alsa/asoundlib.h>
#include <string> 
#include <queue>
#include <pthread.h>
#include <signal.h>
#include <unistd.h>
#include <getopt.h>

extern "C"
{
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>

#include <codec.h>
}

#include "Exception.h"
#include "PacketBuffer.h"
#include <memory>


//class Exception : std::exception
//{
//public:
//	Exception()
//	{}
//
//	Exception(const char* message)
//	{
//		printf("%s\n", message);
//	}
//};
//
//class NotSupportedException : Exception
//{
//
//};


//class Buffer
//{
//private:
//	const int DEFAULT_ALIGNMENT = 16;
//
//
//	void* actualPtr = nullptr;
//	void* dataPtr = nullptr;
//	int dataLength = 0;
//	int usedLength = 0;
//	double timeStamp = -1.0;
//
//
//	void SetData(void* value)
//	{
//		dataPtr = value;
//	}
//
//	void SetDataLength(int value)
//	{
//		dataLength = value;
//	}
//
//
//
//	static bool IsPowerOfTwo(int value)
//	{
//		return (value > 0) && (((value - 1) & value) == 0);
//	}
//
//	static void* AllocAligned(int length, int alignment, void** out_actualPtr)
//	{
//		if (length < 1)
//			throw Exception();
//
//		if (!IsPowerOfTwo(alignment))
//			throw Exception();
//
//
//		int actualAlignment = alignment - 1;
//		*out_actualPtr = malloc(length + actualAlignment);
//
//
//		long address = (long)*out_actualPtr;
//		void* result = (void*)((address + actualAlignment) & (~actualAlignment));
//		
//		return result;
//	}
//
//
//public:
//	double GetTimeStamp()
//	{
//		return timeStamp;
//	}
//	void SetTimeStamp(double value)
//	{
//		timeStamp = value;
//	}
//
//	void* GetData()
//	{
//		return dataPtr;
//	}
//	
//	int GetDataLength()
//	{
//		return dataLength;
//	}
//	
//	int GetUsedLength()
//	{
//		return usedLength;
//	}
//	void SetUsedLength(int value)
//	{
//		if (value < 0)
//			throw Exception();
//
//		usedLength = value;
//	}
//
//
//
//	Buffer()
//	{
//
//	}
//	Buffer(int dataLength)
//	{
//		if (dataLength < 1)
//			throw Exception();
//
//
//		SetData(AllocAligned(dataLength, DEFAULT_ALIGNMENT, &actualPtr));
//		SetDataLength(dataLength);
//	}
//
//	~Buffer()
//	{
//		free(actualPtr);
//	}
//
//
//	void Resize(int dataLength)
//	{
//		if (dataLength < 1)
//			throw Exception();
//
//
//		free(actualPtr);
//
//
//		SetData(AllocAligned(dataLength, DEFAULT_ALIGNMENT, &actualPtr));
//		SetDataLength(dataLength);
//		SetUsedLength(0);
//	}
//
//};



const unsigned long PTS_FREQ = 90000;
const int EXTERNAL_PTS = (1);
const int SYNC_OUTSIDE = (2);


AVFormatContext* ctx = NULL;
codec_para_t codecContext = { 0 };
int video_stream_idx = -1;
void* video_extra_data;
int video_extra_data_size = 0;
std::vector<unsigned char> videoExtraData;
AVCodecID video_codec_id;
std::vector<AVPacket> videoPackets;


static const char *device = "default"; //default   //plughw                     /* playback device */
snd_pcm_t *handle;
snd_pcm_sframes_t frames;

int audio_stream_idx = -1;
AVCodecID audio_codec_id;
int audio_sample_rate = 0;
int audio_channels = 0;
//std::queue<Buffer*> audioQueue;
//std::queue<Buffer*> audioFreeQueue;

typedef std::shared_ptr<PacketBuffer> PacketBufferPtr;

pthread_mutex_t audioMutex = PTHREAD_MUTEX_INITIALIZER;
std::queue<PacketBufferPtr> audioBuffers;

pthread_mutex_t videoMutex = PTHREAD_MUTEX_INITIALIZER;
std::queue<PacketBufferPtr> videoBuffers;



void WriteToFile(const char* path, const char* value)
{
	int fd = open(path, O_RDWR | O_TRUNC, 0644);
	if (fd < 0)
	{
		printf("WriteToFile open failed: %s = %s\n", path, value);
		exit(1);
	}

	if (write(fd, value, strlen(value)) < 0)
	{
		printf("WriteToFile write failed: %s = %s\n", path, value);
		exit(1);
	}

	close(fd);
}

void SetupPins()
{
	int ret = avformat_find_stream_info(ctx, NULL);
	if (ret < 0)
	{
		throw Exception();
	}

	int streamCount = ctx->nb_streams;
	if (streamCount < 1)
	{
		throw Exception("No streams found");
	}

	printf("Streams (count=%d):\n", streamCount);

	for (int i = 0; i < streamCount; ++i)
	{
		AVStream* streamPtr = ctx->streams[i];
		AVCodecContext* codecCtxPtr = streamPtr->codec;
		AVMediaType mediaType = codecCtxPtr->codec_type;		
		AVCodecID codec_id = codecCtxPtr->codec_id;

		switch (mediaType)
		{
		case AVMEDIA_TYPE_VIDEO:
		{
			double fps = av_q2d(streamPtr->avg_frame_rate);


			if (video_stream_idx < 0)
			{
				video_stream_idx = i;
				video_extra_data = codecCtxPtr->extradata;
				video_extra_data_size = codecCtxPtr->extradata_size;

				codecContext.stream_type = STREAM_TYPE_ES_VIDEO;
				codecContext.has_video = 1;
				//codecContext.am_sysinfo.param = (void*)(SYNC_OUTSIDE);
				codecContext.am_sysinfo.param = (void*)(EXTERNAL_PTS | SYNC_OUTSIDE);
				//codecContext.am_sysinfo.rate = (int)(96000.5 / fps);	// 0.5 is used by kodi
				codecContext.am_sysinfo.rate = 96000.5 * ((double)streamPtr->avg_frame_rate.den / (double)streamPtr->avg_frame_rate.num);
				//printf("codecContext.am_sysinfo.rate = %d (%d num : %d dem)\n",
					//codecContext.am_sysinfo.rate,
					//streamPtr->avg_frame_rate.num,
					//streamPtr->avg_frame_rate.den);

				video_codec_id = codec_id;
			}

			

			switch (codec_id)
			{
			case CODEC_ID_MPEG2VIDEO:
				printf("stream #%d - VIDEO/MPEG2\n", i);

				codecContext.video_type = VFORMAT_MPEG12;
				codecContext.am_sysinfo.format = VIDEO_DEC_FORMAT_UNKNOW;
				break;

			case CODEC_ID_MPEG4:
				printf("stream #%d - VIDEO/MPEG4\n", i);

				codecContext.video_type = VFORMAT_MPEG4;
				codecContext.am_sysinfo.format = VIDEO_DEC_FORMAT_MPEG4_5;
				//VIDEO_DEC_FORMAT_MP4; //VIDEO_DEC_FORMAT_MPEG4_3; //VIDEO_DEC_FORMAT_MPEG4_4; //VIDEO_DEC_FORMAT_MPEG4_5;
				break;

			case CODEC_ID_H264:
			{
				printf("stream #%d - VIDEO/H264\n", i);

				codecContext.video_type = VFORMAT_H264_4K2K;
				codecContext.am_sysinfo.format = VIDEO_DEC_FORMAT_H264_4K2K;
			}
			break;

			case AV_CODEC_ID_HEVC:
				printf("stream #%d - VIDEO/HEVC\n", i);

				codecContext.video_type = VFORMAT_HEVC;
				codecContext.am_sysinfo.format = VIDEO_DEC_FORMAT_HEVC;
				break;


			case CODEC_ID_VC1:
				printf("stream #%d - VIDEO/VC1\n", i);
				break;

			default:
				printf("stream #%d - VIDEO/UNKNOWN(%x)\n", i, codec_id);
				//throw NotSupportedException();
			}

			printf("\tfps=%f(%d/%d) ", fps,
				streamPtr->avg_frame_rate.num,
				streamPtr->avg_frame_rate.den);

			printf("am_sysinfo.rate=%d ",
				codecContext.am_sysinfo.rate);


			int width = codecCtxPtr->width;
			int height = codecCtxPtr->height;

			printf("SAR=(%d/%d) ",
				streamPtr->sample_aspect_ratio.num,
				streamPtr->sample_aspect_ratio.den);

			// TODO: DAR

			printf("\n");

		}
		break;

		case AVMEDIA_TYPE_AUDIO:
		{
			// Use the first audio stream
			if (audio_stream_idx == -1)
			{
				audio_stream_idx = i;
				audio_codec_id = codec_id;
			}


			switch (codec_id)
			{
			case CODEC_ID_MP3:
				printf("stream #%d - AUDIO/MP3\n", i);
				break;

			case CODEC_ID_AAC:
				printf("stream #%d - AUDIO/AAC\n", i);
				break;

			case CODEC_ID_AC3:
				printf("stream #%d - AUDIO/AC3\n", i);
				break;

			case CODEC_ID_DTS:
				printf("stream #%d - AUDIO/DTS\n", i);
				break;

				//case AVCodecID.CODEC_ID_WMAV2:
				//    break;

			default:
				printf("stream #%d - AUDIO/UNKNOWN (0x%x)\n", i, codec_id);
				//throw NotSupportedException();
				break;
			}

			audio_channels = codecCtxPtr->channels;
			audio_sample_rate = codecCtxPtr->sample_rate;

		}
		break;


		case AVMEDIA_TYPE_SUBTITLE:
		{
			switch (codec_id)
			{
			case  CODEC_ID_DVB_SUBTITLE:
				printf("stream #%d - SUBTITLE/DVB_SUBTITLE\n", i);
				break;

			case  CODEC_ID_TEXT:
				printf("stream #%d - SUBTITLE/TEXT\n", i);
				break;

			case  CODEC_ID_XSUB:
				printf("stream #%d - SUBTITLE/XSUB\n", i);
				break;

			case  CODEC_ID_SSA:
				printf("stream #%d - SUBTITLE/SSA\n", i);
				break;

			case  CODEC_ID_MOV_TEXT:
				printf("stream #%d - SUBTITLE/MOV_TEXT\n", i);
				break;

			case  CODEC_ID_HDMV_PGS_SUBTITLE:
				printf("stream #%d - SUBTITLE/HDMV_PGS_SUBTITLE\n", i);
				break;

			case  CODEC_ID_DVB_TELETEXT:
				printf("stream #%d - SUBTITLE/DVB_TELETEXT\n", i);
				break;

			case  CODEC_ID_SRT:
				printf("stream #%d - SUBTITLE/SRT\n", i);
				break;


			default:
				printf("stream #%d - SUBTITLE/UNKNOWN (0x%x)\n", i, codec_id);
				break;
			}
		}
		break;


		case AVMEDIA_TYPE_DATA:
			printf("stream #%d - DATA\n", i);
			break;

		default:
			printf("stream #%d - Unknown mediaType (%x)\n", i, mediaType);
			//throw NotSupportedException();
		}

	}
}

void ConvertH264ExtraDataToAnnexB()
{
	videoExtraData.clear();

	if (video_extra_data_size > 0)
	{
		unsigned char* extraData = (unsigned char*)video_extra_data;

		// http://aviadr1.blogspot.com/2010/05/h264-extradata-partially-explained-for.html

		const int spsStart = 6;
		int spsLen = extraData[spsStart] * 256 + extraData[spsStart + 1];

		videoExtraData.push_back(0);
		videoExtraData.push_back(0);
		videoExtraData.push_back(0);
		videoExtraData.push_back(1);

		for (int i = 0; i < spsLen; ++i)
		{
			videoExtraData.push_back(extraData[spsStart + 2 + i]);
		}


		int ppsStart = spsStart + 2 + spsLen + 1; // 2byte sbs len, 1 byte pps start code
		int ppsLen = extraData[ppsStart] * 256 + extraData[ppsStart + 1];

		videoExtraData.push_back(0);
		videoExtraData.push_back(0);
		videoExtraData.push_back(0);
		videoExtraData.push_back(1);

		for (int i = 0; i < ppsLen; ++i)
		{
			videoExtraData.push_back(extraData[ppsStart + 2 + i]);
		}

	}


	printf("EXTRA DATA = ");

	for (int i = 0; i < videoExtraData.size(); ++i)
	{
		printf("%02x ", videoExtraData[i]);

	}

	printf("\n");
}


void HevcExtraDataToAnnexB()
{
	videoExtraData.clear();

	if (video_extra_data_size > 0)
	{
		unsigned char* extraData = (unsigned char*)video_extra_data;

		// http://fossies.org/linux/ffmpeg/libavcodec/hevc_mp4toannexb_bsf.c

		int offset = 21;
		int length_size = (extraData[offset++] & 3) + 1;
		int num_arrays = extraData[offset++];

		printf("HevcExtraDataToAnnexB: length_size=%d, num_arrays=%d\n", length_size, num_arrays);


		for (int i = 0; i < num_arrays; i++)
		{
			int type = extraData[offset++] & 0x3f;
			int cnt = extraData[offset++] << 8 | extraData[offset++];

			for (int j = 0; j < cnt; j++)
			{
				videoExtraData.push_back(0);
				videoExtraData.push_back(0);
				videoExtraData.push_back(0);
				videoExtraData.push_back(1);

				int nalu_len = extraData[offset++] << 8 | extraData[offset++];
				for (int k = 0; k < nalu_len; ++k)
				{
					videoExtraData.push_back(extraData[offset++]);
				}
			}
		}

		printf("EXTRA DATA = ");

		for (int i = 0; i < videoExtraData.size(); ++i)
		{
			printf("%02x ", videoExtraData[i]);
		}

		printf("\n");
	}
}


/*
# Disable alpha layer (hide video)
echo "d01068b4 0x7fc0" | sudo tee /sys/kernel/debug/aml_reg/paddr

# Enable alpha layer (show video)
echo "d01068b4 0x3fc0" | sudo tee /sys/kernel/debug/aml_reg/paddr
*/


bool isRunning = true;

// Signal handler for Ctrl-C
void SignalHandler(int s)
{
	isRunning = false;
}

void* VideoDecoderThread(void* argument)
{
	WriteToFile("/sys/class/graphics/fb0/blank", "1");


	int api = codec_init(&codecContext);
	if (api != 0)
	{
		printf("codec_init failed (%x).\n", api);
		exit(1);
	}

	api = codec_set_syncenable(&codecContext, 1);
	if (api != 0)
	{
		printf("codec_set_syncenable failed (%x).\n", api);
		exit(1);
	}


	switch (video_codec_id)
	{
	case CODEC_ID_H264:
		ConvertH264ExtraDataToAnnexB();
		break;

	case AV_CODEC_ID_HEVC:
		HevcExtraDataToAnnexB();
		break;
	}


	bool isFirstVideoPacket = true;
	bool isAnnexB = false;
	bool isExtraDataSent = false;
	long estimatedNextPts = 0;

	while (isRunning)
	{
		pthread_mutex_lock(&videoMutex);
		size_t count = videoBuffers.size();

		if (count < 1)
		{
			pthread_mutex_unlock(&videoMutex);
			usleep(1);
		}
		else
		{
			PacketBufferPtr buffer = videoBuffers.front();
			videoBuffers.pop();

			pthread_mutex_unlock(&videoMutex);

			AVPacket* pkt = buffer->GetAVPacket();


			unsigned char* nalHeader = (unsigned char*)pkt->data;

#if 0
			printf("Header (pkt.size=%x):\n", pkt.size);
			for (int j = 0; j < 16; ++j)	//nalHeaderLength
			{
				printf("%02x ", nalHeader[j]);
			}
			printf("\n");
#endif

			if (isFirstVideoPacket)
			{
				printf("Header (pkt.size=%x):\n", pkt->size);
				for (int j = 0; j < 16; ++j)	//nalHeaderLength
				{
					printf("%02x ", nalHeader[j]);
				}
				printf("\n");


				//// DEBUG
				//printf("Codec ExtraData=%p ExtraDataSize=%x\n", video_extra_data, video_extra_data_size);
				//
				//unsigned char* ptr = (unsigned char*)video_extra_data;
				//printf("ExtraData :\n");
				//for (int j = 0; j < video_extra_data_size; ++j)
				//{					
				//	printf("%02x ", ptr[j]);
				//}
				//printf("\n");
				//

				//int extraDataCall = codec_write(&codecContext, video_extra_data, video_extra_data_size);
				//if (extraDataCall == -EAGAIN)
				//{
				//	printf("ExtraData codec_write failed.\n");
				//}
				////


				if (nalHeader[0] == 0 && nalHeader[1] == 0 &&
					nalHeader[2] == 0 && nalHeader[3] == 1)
				{
					isAnnexB = true;
				}

				isFirstVideoPacket = false;

				printf("isAnnexB=%u\n", isAnnexB);
			}
		

			if (!isAnnexB &&
				(video_codec_id == CODEC_ID_H264 ||
					video_codec_id == AV_CODEC_ID_HEVC))
			{
				//unsigned char* nalHeader = (unsigned char*)pkt.data;

				// Five least significant bits of first NAL unit byte signify nal_unit_type.
				int nal_unit_type;
				const int nalHeaderLength = 4;

				while (nalHeader < (pkt->data + pkt->size))
				{
					switch (video_codec_id)
					{
					case CODEC_ID_H264:
						//if (!isExtraDataSent)
						{
							// Copy AnnexB data if NAL unit type is 5
							nal_unit_type = nalHeader[nalHeaderLength] & 0x1F;

							if (nal_unit_type == 5)
							{
								api = codec_write(&codecContext, &videoExtraData[0], videoExtraData.size());
								if (api <= 0)
								{
									printf("extra data codec_write error: %x\n", api);
								}
								else
								{
									//printf("extra data codec_write OK\n");
								}
							}

							isExtraDataSent = true;
						}
						break;

					case AV_CODEC_ID_HEVC:
						//if (!isExtraDataSent)
						{
							nal_unit_type = (nalHeader[nalHeaderLength] >> 1) & 0x3f;

							/* prepend extradata to IRAP frames */
							if (nal_unit_type >= 16 && nal_unit_type <= 23)
							{
								int attempts = 10;

								while (attempts > 0)
								{
									api = codec_write(&codecContext, &videoExtraData[0], videoExtraData.size());
									if (api <= 0)
									{
										usleep(1);
										--attempts;
									}
									else
									{
										//printf("extra data codec_write OK\n");
										break;
									}

								}

								if (attempts == 0)
								{
									printf("extra data codec_write error: %x\n", api);
								}
							}

							isExtraDataSent = true;
						}
						break;

					}


					// Overwrite header NAL length with startcode '0x00000001' in *BigEndian*
					int nalLength = nalHeader[0] << 24;
					nalLength |= nalHeader[1] << 16;
					nalLength |= nalHeader[2] << 8;
					nalLength |= nalHeader[3];

					nalHeader[0] = 0;
					nalHeader[1] = 0;
					nalHeader[2] = 0;
					nalHeader[3] = 1;

					nalHeader += nalLength + 4;
				}
			}


			if (pkt->pts != AV_NOPTS_VALUE)
			{
				AVStream* streamPtr = ctx->streams[video_stream_idx];
				double timeStamp = av_q2d(streamPtr->time_base) * pkt->pts;
				unsigned long pts = (unsigned long)(timeStamp * PTS_FREQ);
				//printf("pts = %lu, timeStamp = %f\n", pts, timeStamp);
				if (codec_checkin_pts(&codecContext, pts))
				{
					printf("codec_checkin_pts failed\n");
				}

				estimatedNextPts = pkt->pts + pkt->duration;
			}
			else
			{
				//printf("WARNING: AV_NOPTS_VALUE for codec_checkin_pts (duration=%x).\n", pkt.duration);

				if (pkt->duration > 0)
				{
					// Estimate PTS
					AVStream* streamPtr = ctx->streams[video_stream_idx];
					double timeStamp = av_q2d(streamPtr->time_base) * estimatedNextPts;
					unsigned long pts = (unsigned long)(timeStamp * PTS_FREQ);

					if (codec_checkin_pts(&codecContext, pts))
					{
						printf("codec_checkin_pts failed\n");
					}

					estimatedNextPts += pkt->duration;

					//printf("WARNING: Estimated PTS for codec_checkin_pts (timeStamp=%f).\n", timeStamp);
				}
			}


			// Send the data to the codec
			int api = 0;
			int offset = 0;
			do
			{
				api = codec_write(&codecContext, pkt->data + offset, pkt->size - offset);
				if (api == -EAGAIN)
				{
					usleep(1);
				}
				else if (api == -1)
				{
					// TODO: Sometimes this error is returned.  Ignoring it
					// does not seem to have any impact on video display
				}
				else if (api < 0)
				{
					printf("codec_write error: %x\n", api);
					//codec_reset(&codecContext);
				}
				else if (api > 0)
				{
					offset += api;
					//printf("codec_write send %x bytes of %x total.\n", api, pkt.size);
				}

			} while (api == -EAGAIN || offset < pkt->size);
		}

	}

	codec_close(&codecContext);

	WriteToFile("/sys/class/graphics/fb0/blank", "0");

	return nullptr;
}

void* AudioDecoderThread(void* argument)
{
	const double AUDIO_ADJUST_SECONDS = (-0.30);



	AVCodec* soundCodec = avcodec_find_decoder(audio_codec_id);
	if (!soundCodec) {
		fprintf(stderr, "codec not found\n");
		exit(1);
	}

	AVCodecContext* soundCodecContext = avcodec_alloc_context3(soundCodec);
	if (!soundCodecContext) {
		fprintf(stderr, "avcodec_alloc_context3 failed.\n");
		exit(1);
	}

	if (audio_channels == 0)
		audio_channels = 2;

	if (audio_sample_rate == 0)
		audio_sample_rate = 48000;

	soundCodecContext->channels = audio_channels;
	soundCodecContext->sample_rate = audio_sample_rate;
	//soundCodecContext->sample_fmt = AV_SAMPLE_FMT_S16P; //AV_SAMPLE_FMT_FLTP; //AV_SAMPLE_FMT_S16P
	soundCodecContext->request_sample_fmt = AV_SAMPLE_FMT_FLTP; // AV_SAMPLE_FMT_S16P; //AV_SAMPLE_FMT_FLTP;
	soundCodecContext->request_channel_layout = AV_CH_LAYOUT_STEREO;

	/* open it */
	if (avcodec_open2(soundCodecContext, soundCodec, NULL) < 0) {
		fprintf(stderr, "could not open codec\n");
		exit(1);
	}



	AVFrame* decoded_frame = av_frame_alloc();
	if (!decoded_frame) {
		fprintf(stderr, "av_frame_alloc failed.\n");
		exit(1);
	}


	int err;
	if ((err = snd_pcm_open(&handle, device, SND_PCM_STREAM_PLAYBACK, 0)) < 0) //SND_PCM_NONBLOCK
	{
		printf("snd_pcm_open error: %s\n", snd_strerror(err));
		exit(EXIT_FAILURE);
	}

	int alsa_channels = 2;
	const int FRAME_SIZE = 1536; // 1536;
	snd_pcm_hw_params_t *hw_params;
	snd_pcm_sw_params_t *sw_params;
	snd_pcm_uframes_t period_size = FRAME_SIZE * alsa_channels * 2;
	snd_pcm_uframes_t buffer_size = 12 * period_size;
	unsigned int sampleRate = soundCodecContext->sample_rate;
	if (sampleRate == 0)
		sampleRate = 48000;

	printf("sampleRate = %d\n", sampleRate);

	(snd_pcm_hw_params_malloc(&hw_params));
	(snd_pcm_hw_params_any(handle, hw_params));
	(snd_pcm_hw_params_set_access(handle, hw_params, SND_PCM_ACCESS_RW_INTERLEAVED));
	(snd_pcm_hw_params_set_format(handle, hw_params, SND_PCM_FORMAT_S16_LE));
	(snd_pcm_hw_params_set_rate_near(handle, hw_params, &sampleRate, NULL));
	(snd_pcm_hw_params_set_channels(handle, hw_params, alsa_channels));
	(snd_pcm_hw_params_set_buffer_size_near(handle, hw_params, &buffer_size));
	(snd_pcm_hw_params_set_period_size_near(handle, hw_params, &period_size, NULL));
	(snd_pcm_hw_params(handle, hw_params));
	snd_pcm_hw_params_free(hw_params);

	snd_pcm_prepare(handle);


	//if ((err = snd_pcm_set_params(handle,
	//	SND_PCM_FORMAT_S16,
	//	SND_PCM_ACCESS_RW_INTERLEAVED, //SND_PCM_ACCESS_RW_NONINTERLEAVED,
	//	1, //soundCodecContext->channels,
	//	soundCodecContext->sample_rate,
	//	0,
	//	10000000)) < 0) {   /* required overall latency in us */
	//	printf("snd_pcm_set_params error: %s\n", snd_strerror(err));
	//	exit(EXIT_FAILURE);
	//}

	int totalAudioFrames = 0;
	double pcrElapsedTime = 0;
	double frameTimeStamp = -1.0;

	while (isRunning)
	{
		pthread_mutex_lock(&audioMutex);
		size_t count = audioBuffers.size();

		if (count < 1)
		{
			pthread_mutex_unlock(&audioMutex);
			usleep(1);
		}
		else
		{
			PacketBufferPtr buffer = audioBuffers.front();
			audioBuffers.pop();

			pthread_mutex_unlock(&audioMutex);



			if (frameTimeStamp < 0)
			{
				frameTimeStamp = buffer->GetTimeStamp();				
			}


			// ---

			//AVPacket pkt;
			//av_init_packet(&pkt);
			//pkt.data = (uint8_t*)buffer->GetData();
			//pkt.size = buffer->GetUsedLength();
			//pkt.dts = pkt.pts = AV_NOPTS_VALUE;
			////printf("pkt.data=%p, pkt.size=%x\n", pkt.data, pkt.size);


			int got_frame = 0;
			int len = avcodec_decode_audio4(soundCodecContext,
				decoded_frame,
				&got_frame,
				buffer->GetAVPacket());
			//printf("avcodec_decode_audio4 len=%d\n", len);

			if (len < 0) 
			{
				char errmsg[1024] = { 0 };
				av_strerror(len, errmsg, 1024);

				fprintf(stderr, "Error while decoding: %s\n", errmsg);
				//exit(1);
			}

			//printf("decoded audio frame OK (len=%x, pkt.size=%x)\n", len, pkt.size);

			if (got_frame)
			{
				int leftChannelIndex = av_get_channel_layout_channel_index(decoded_frame->channel_layout, AV_CH_FRONT_LEFT);
				int rightChannelIndex = av_get_channel_layout_channel_index(decoded_frame->channel_layout, AV_CH_FRONT_RIGHT);
				//int centerChannelIndex = av_get_channel_layout_channel_index(decoded_frame->channel_layout, AV_CH_FRONT_CENTER);


				//printf("leftChannelIndex=%d, rightChannelIndex=%d, centerChannelIndex=%d\n",
				//	leftChannelIndex, rightChannelIndex, centerChannelIndex);

				short data[alsa_channels * decoded_frame->nb_samples];

				if (soundCodecContext->sample_fmt == AV_SAMPLE_FMT_S16P)
				{
					short* channels[alsa_channels] = { 0 };
					channels[0] = (short*)decoded_frame->data[leftChannelIndex];
					channels[1] = (short*)decoded_frame->data[rightChannelIndex];
					//channels[2] = (short*)decoded_frame->data[centerChannelIndex];

					int index = 0;
					for (int i = 0; i < decoded_frame->nb_samples; ++i)
					{
						for (int j = 0; j < alsa_channels; ++j)
						{
							short* samples = channels[j];
							data[index++] = samples[i];
						}
					}
				}
				else if (soundCodecContext->sample_fmt == AV_SAMPLE_FMT_FLTP)
				{
					float* channels[alsa_channels] = { 0 };
					channels[0] = (float*)decoded_frame->data[leftChannelIndex];
					channels[1] = (float*)decoded_frame->data[rightChannelIndex];
					//channels[2] = (float*)decoded_frame->data[centerChannelIndex];

					int index = 0;
					for (int i = 0; i < decoded_frame->nb_samples; ++i)
					{
						for (int j = 0; j < alsa_channels; ++j)
						{
							float* samples = channels[j];
							
							float sample = samples[i];
							short value;
							if (sample >= 0)
							{
								value = (sample >= 1) ? 0x7fff : (short)(sample * 0x7fff);
							}
							else
							{
								value = (sample <= -1) ? 0x8000 : (short)(sample * 0x8000);
							}

							//printf("sample = %d\n", value);

							data[index++] = value;
						}
					}
				}
				else
				{
					throw NotSupportedException();
				}




				snd_pcm_sframes_t frames = snd_pcm_writei(handle,
					(void*)data,
					decoded_frame->nb_samples);

				if (frames < 0) 
				{
					printf("snd_pcm_writei failed: %s\n", snd_strerror(frames));

					if (frames == -EPIPE)
					{
						snd_pcm_recover(handle, frames, 1);
						printf("snd_pcm_recover\n");
					}
				}


				{
					if (frameTimeStamp < 0)
					{
						printf("WARNING: frameTimeStamp not set.\n");
					}

					unsigned long pts = (unsigned long)((frameTimeStamp + AUDIO_ADJUST_SECONDS) * PTS_FREQ);
					frameTimeStamp = -1;


					int codecCall = codec_set_pcrscr(&codecContext, (int)pts);
					if (codecCall != 0)
					{
						printf("codec_set_pcrscr failed.\n");
					}
				}



			}

			//pthread_mutex_lock(&audioMutex);
			//audioFreeQueue.push(buffer);
			//pthread_mutex_unlock(&audioMutex);

		}

	}

	return nullptr;
}

//void FlushAudioQueue()
//{
//	pthread_mutex_lock(&audioMutex);
//
//	while (audioQueue.size() > 0)
//	{
//		Buffer* buffer = audioQueue.front();
//		audioQueue.pop();
//
//		audioFreeQueue.push(buffer);
//	}
//
//	pthread_mutex_unlock(&audioMutex);
//}


void PrintDictionary(AVDictionary* dictionary)
{
	int count = av_dict_count(dictionary);

	AVDictionaryEntry* prevEntry = nullptr;

	for (int i = 0; i < count; ++i)
	{
		AVDictionaryEntry* entry = av_dict_get(dictionary, "", prevEntry, AV_DICT_IGNORE_SUFFIX);

		if (entry != nullptr)
		{
			printf("\tkey=%s, value=%s\n", entry->key, entry->value);
		}

		prevEntry = entry;
	}
}

struct option longopts[] = {
	{ "time",         required_argument,  NULL,          't' },
	{ "chapter",      required_argument,  NULL,          'c' },
	{ 0, 0, 0, 0 }
};

#if 0
void TestAudio()
{


	/*
	typedef struct {
    int valid;               ///< audio extradata valid(1) or invalid(0), set by dsp
    int sample_rate;         ///< audio stream sample rate
    int channels;            ///< audio stream channels
    int bitrate;             ///< audio stream bit rate
    int codec_id;            ///< codec format id
    int block_align;         ///< audio block align from ffmpeg
    int extradata_size;      ///< extra data size
    char extradata[AUDIO_EXTRA_DATA_SIZE];;   ///< extra data information for decoder
} audio_info_t;
*/

	/*
	typedef struct {
	CODEC_HANDLE handle;        ///< codec device handler
	CODEC_HANDLE cntl_handle;   ///< video control device handler
	CODEC_HANDLE sub_handle;    ///< subtile device handler
	CODEC_HANDLE audio_utils_handle;  ///< audio utils handler
	stream_type_t stream_type;  ///< stream type(es, ps, rm, ts)
	unsigned int has_video:
	1;                          ///< stream has video(1) or not(0)
	unsigned int  has_audio:
	1;                          ///< stream has audio(1) or not(0)
	unsigned int has_sub:
	1;                          ///< stream has subtitle(1) or not(0)
	unsigned int noblock:
	1;                          ///< codec device is NONBLOCK(1) or not(0)
	int video_type;             ///< stream video type(H264, VC1...)
	int audio_type;             ///< stream audio type(PCM, WMA...)
	int sub_type;               ///< stream subtitle type(TXT, SSA...)
	int video_pid;              ///< stream video pid
	int audio_pid;              ///< stream audio pid
	int sub_pid;                ///< stream subtitle pid
	int audio_channels;         ///< stream audio channel number
	int audio_samplerate;       ///< steram audio sample rate
	int vbuf_size;              ///< video buffer size of codec device
	int abuf_size;              ///< audio buffer size of codec device
	dec_sysinfo_t am_sysinfo;   ///< system information for video
	audio_info_t audio_info;    ///< audio information pass to audiodsp
	int packet_size;            ///< data size per packet
	int avsync_threshold;    ///<for adec in ms>
	void * adec_priv;          ///<for adec>
	int SessionID;
	int dspdec_not_supported;//check some profile that audiodsp decoder can not support,we switch to arm decoder
	int switch_audio_flag;		//<switch audio flag switching(1) else(0)
	} codec_para_t;
	*/

	codec_para_t audioCodec = { 0 };

	audioCodec.stream_type = STREAM_TYPE_ES_AUDIO;
	audioCodec.has_audio = 1;
	audioCodec.audio_type = AFORMAT_PCM_S16LE;
	audioCodec.audio_channels = 2;
	audioCodec.audio_samplerate = 48000;

	audioCodec.audio_info.sample_rate = 48000;
	audioCodec.audio_info.channels = 2;
		//audioCodec.audio_info.bitrate
	audioCodec.audio_info.codec_id = AFORMAT_PCM_S16LE;
		//audioCodec.audio_info.extradata_size
		//audioCodec.audio_info.extradata


	int api = codec_init(&audioCodec);
	if (api != 0)
	{
		printf("audioCodec codec_init failed (%x=%d).\n", api, api);
		exit(1);
	}

	int length = audioCodec.audio_channels * audioCodec.audio_samplerate * sizeof(short);
	short buffer[length];
	
	for (int i = 0; i < length; ++i)
	{
		buffer[i] = (short)(rand() % 0xffff);
		//printf("%04x", buffer[i]);
	}

	unsigned char* data = (unsigned char*)buffer;
	data[0] = 0x00;
	data[1] = 0x00;
	data[2] = 0x01;

	data[3] = 0xc0;

	int remainingLength = length - 6;
	data[4] = (remainingLength & 0xff00) >> 8;
	data[5] = remainingLength & 0xff;


	while (true)
	{
		api = codec_write(&audioCodec, buffer, length);
		if (api <= 0)
		{
			printf("audioCodec codec_write error: %x\n", api);
		}
		usleep(1);
	}
}
#endif

int main(int argc, char** argv)
{
#if 0
	TestAudio();
	return 0;
#endif


	if (argc < 2)
	{
		// TODO: Usage
		printf("TODO: Usage\n");
		return 0;
	}


	// options
	int c;
	double optionStartPosition = 0;
	int optionChapter = -1;

	while ((c = getopt_long(argc, argv, "t:c:", longopts, NULL)) != -1)
	{
		switch (c)
		{
		case 't':
		{
			if (strchr(optarg, ':'))
			{
				unsigned int h;
				unsigned int m;
				double s;
				if (sscanf(optarg, "%u:%u:%lf", &h, &m, &s) == 3)
				{
					optionStartPosition = h * 3600 + m * 60 + s;
				}
				else
				{
					printf("invalid time specification.\n");
					throw Exception();
				}
			}
			else
			{
				optionStartPosition = atof(optarg);
			}

			printf("startPosition=%f\n", optionStartPosition);
		}
		break;

		case 'c':
			optionChapter = atoi(optarg);
			printf("optionChapter=%d\n", optionChapter);
			break;

		default:
			throw NotSupportedException();

			//printf("?? getopt returned character code 0%o ??\n", c);
			//break;
		}
	}


	const char* url = nullptr;
	if (optind < argc) 
	{
		//printf("non-option ARGV-elements: ");
		while (optind < argc)
		{
			url = argv[optind++];
			//printf("%s\n", url);
			break;
		}		
	}


	if (url == nullptr)
	{
		// TODO: Usage
		printf("TODO: Usage\n");
		return 0;
	}

	//return 0;


	// Trap signal to clean up
	signal(SIGINT, SignalHandler);


	// Initialize libav
	av_log_set_level(AV_LOG_VERBOSE);
	av_register_all();
	avformat_network_init();


	//const char* url = argv[1];

	AVDictionary* options_dict = NULL;
	
	/*
	Set probing size in bytes, i.e. the size of the data to analyze to get
	stream information. A higher value will enable detecting more information
	in case it is dispersed into the stream, but will increase latency. Must
	be an integer not lesser than 32. It is 5000000 by default.
	*/
	av_dict_set(&options_dict, "probesize", "10000000", 0);

	/*
	Specify how many microseconds are analyzed to probe the input. A higher
	value will enable detecting more accurate information, but will increase
	latency. It defaults to 5,000,000 microseconds = 5 seconds.
	*/
	av_dict_set(&options_dict, "analyzeduration", "10000000", 0);

	int ret = avformat_open_input(&ctx, url, NULL, &options_dict);
	if (ret < 0)
	{
		printf("avformat_open_input failed.\n");
	}

	
	printf("Source Metadata:\n");
	PrintDictionary(ctx->metadata);
	

	SetupPins();
	
	
	// Chapters
	int chapterCount = ctx->nb_chapters;
	printf("Chapters (count=%d):\n", chapterCount);

	AVChapter** chapters = ctx->chapters;
	for (int i = 0; i < chapterCount; ++i)
	{
		AVChapter* avChapter = chapters[i];

		int index = i + 1;
		double start = avChapter->start * avChapter->time_base.num / (double)avChapter->time_base.den;
		double end = avChapter->end * avChapter->time_base.num / (double)avChapter->time_base.den;
		AVDictionary* metadata = avChapter->metadata;

		printf("Chapter #%02d: %f -> %f\n", index, start, end);
		PrintDictionary(metadata);

		if (optionChapter > -1 && optionChapter == index)
		{
			optionStartPosition = start;
		}
	}


	if (optionStartPosition > 0)
	{
		if (av_seek_frame(ctx, -1, (long)(optionStartPosition * AV_TIME_BASE), 0) < 0)
		{
			printf("av_seek_frame (%f) failed\n", optionStartPosition);
		}
	}

	// ---




	pthread_t audioThread;
	bool isAudioThreadStarted = false;

	if (audio_stream_idx >= 0)
	{
		//// Sound
		//for (int i = 0; i < 256; ++i)
		//{
		//	Buffer* buffer = new Buffer();
		//	audioFreeQueue.push(buffer);
		//}

		// ----- start decoder -----
		int result_code = pthread_create(&audioThread, NULL, AudioDecoderThread, NULL);
		if (result_code != 0)
		{
			printf("AudioDecoderThread pthread_create failed.\n");
			exit(1);
		}

		isAudioThreadStarted = true;
	}


	pthread_t videoThread;
	bool isVideoThreadStarted = false;

	if (video_stream_idx >= 0)
	{
		// ----- start decoder -----
		int result_code = pthread_create(&videoThread, NULL, VideoDecoderThread, NULL);
		if (result_code != 0)
		{
			printf("VideoDecoderThread pthread_create failed.\n");
			exit(1);
		}

		isVideoThreadStarted = true;
	}



	// Demux
	AVPacket pkt;
	av_init_packet(&pkt);
	pkt.data = NULL;
	pkt.size = 0;
	

	while (isRunning )
	{
		//printf("Read packet.\n");

		PacketBufferPtr buffer = std::make_shared<PacketBuffer>();

		if (av_read_frame(ctx, buffer->GetAVPacket()) < 0)
			break;

		AVPacket* pkt = buffer->GetAVPacket();

		if (pkt->pts != AV_NOPTS_VALUE)
		{
			AVStream* streamPtr = ctx->streams[pkt->stream_index];
			buffer->SetTimeStamp(av_q2d(streamPtr->time_base) * pkt->pts);
		}


		if (pkt->stream_index == video_stream_idx)
		{
			const int MAX_VIDEO_BUFFERS = 64;

			size_t count;

			while (isRunning)
			{
				pthread_mutex_lock(&videoMutex);
				count = videoBuffers.size();

				if (count >= MAX_VIDEO_BUFFERS)
				{
					pthread_mutex_unlock(&videoMutex);

					usleep(1);
				}
				else
				{
					videoBuffers.push(buffer);

					pthread_mutex_unlock(&videoMutex);
					break;
				}
			}

		}
		else if (pkt->stream_index == audio_stream_idx)
		{
			const int MAX_AUDIO_BUFFERS = 128;

			size_t count;
			
			while(isRunning)
			{
				pthread_mutex_lock(&audioMutex);
				count = audioBuffers.size();

				if (count >= MAX_AUDIO_BUFFERS)
				{
					pthread_mutex_unlock(&audioMutex);
					
					usleep(1);
				}
				else
				{
					audioBuffers.push(buffer);
					
					pthread_mutex_unlock(&audioMutex);
					break;
				}
			}

			//Buffer* buffer = nullptr;

			//while (buffer == nullptr)
			//{
			//	pthread_mutex_lock(&audioMutex);

			//	if (audioFreeQueue.size() > 0)
			//	{
			//		buffer = audioFreeQueue.front();
			//		audioFreeQueue.pop();
			//	}

			//	pthread_mutex_unlock(&audioMutex);

			//	if (buffer == nullptr)
			//		usleep(1);
			//}

			//if (buffer->GetDataLength() < pkt.size)
			//{
			//	buffer->Resize(pkt.size);
			//}

			//memcpy(buffer->GetData(), pkt.data, pkt.size);

			//buffer->SetUsedLength(pkt.size);




			//pthread_mutex_lock(&audioMutex);
			//audioQueue.push(buffer);
			//pthread_mutex_unlock(&audioMutex);
		}


		//av_free_packet(&pkt);

		//av_init_packet(&pkt);
		//pkt.data = NULL;
		//pkt.size = 0;

	}


	// If not terminating, wait until all buffers are
	// finished playing
	while (isRunning)
	{
		pthread_mutex_lock(&audioMutex);
		size_t count = audioBuffers.size();
		pthread_mutex_unlock(&audioMutex);

		if (count < 1)
			isRunning = false;
	}
	
	while (isRunning)
	{
		pthread_mutex_lock(&videoMutex);
		size_t count = videoBuffers.size();
		pthread_mutex_unlock(&videoMutex);

		if (count < 1)
			isRunning = false;
	}



	//isRunning = false;

	if (isAudioThreadStarted)
	{
		void *retval;
		pthread_join(audioThread, &retval);
	}
	if (isVideoThreadStarted)
	{
		void *retval;
		pthread_join(videoThread, &retval);
	}


	return 0;
}