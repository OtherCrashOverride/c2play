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
}

#include "Exception.h"
#include "PacketBuffer.h"
#include "AlsaAudioSink.h"
#include "AmlVideoSink.h"




AVFormatContext* ctx = NULL;


int video_stream_idx = -1;
void* video_extra_data;
int video_extra_data_size = 0;
AVCodecID video_codec_id;
double frameRate;
AVRational time_base;


int audio_stream_idx = -1;
AVCodecID audio_codec_id;
int audio_sample_rate = 0;
int audio_channels = 0;


bool isRunning = true;


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

				video_codec_id = codec_id;


				frameRate = av_q2d(streamPtr->avg_frame_rate);
				time_base = streamPtr->time_base;
			}

			

			switch (codec_id)
			{
			case CODEC_ID_MPEG2VIDEO:
				printf("stream #%d - VIDEO/MPEG2\n", i);
				break;

			case CODEC_ID_MPEG4:
				printf("stream #%d - VIDEO/MPEG4\n", i);
				break;

			case CODEC_ID_H264:
			{
				printf("stream #%d - VIDEO/H264\n", i);
			}
			break;

			case AV_CODEC_ID_HEVC:
				printf("stream #%d - VIDEO/HEVC\n", i);
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


/*
# Disable alpha layer (hide video)
echo "d01068b4 0x7fc0" | sudo tee /sys/kernel/debug/aml_reg/paddr

# Enable alpha layer (show video)
echo "d01068b4 0x3fc0" | sudo tee /sys/kernel/debug/aml_reg/paddr
*/


// Signal handler for Ctrl-C
void SignalHandler(int s)
{
	isRunning = false;
}


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


int main(int argc, char** argv)
{
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

	AlsaAudioSink* audioSink = new AlsaAudioSink(audio_codec_id, audio_sample_rate);
	audioSink->Start();
	audioSink->SetState(MediaState::Play);


	std::shared_ptr<AmlVideoSink> videoSink = std::make_shared<AmlVideoSink>(video_codec_id, frameRate, time_base);
	videoSink->Start();
	videoSink->SetState(MediaState::Play);
	
	audioSink->SetClockSink(videoSink);


	AVStream* streamPtr = ctx->streams[video_stream_idx];
	AVCodecContext* codecCtxPtr = streamPtr->codec;

	videoSink->SetExtraData(codecCtxPtr->extradata);
	videoSink->SetExtraDataSize(codecCtxPtr->extradata_size);


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
			videoSink->AddBuffer(buffer);
		}
		else if (pkt->stream_index == audio_stream_idx)
		{
			audioSink->AddBuffer(buffer);
		}
	}


	// If not terminating, wait until all buffers are
	// finished playing
	audioSink->Stop();
	videoSink->Stop();


	return 0;
}