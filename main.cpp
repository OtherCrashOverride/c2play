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
#include "InputDevice.h"
#include "MediaSourceElement.h"
#include "AudioCodec.h"

#include <dirent.h>
#include <sys/stat.h>
#include <string>
#include <linux/input.h>
#include <linux/uinput.h>

#include <linux/kd.h>


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
std::vector<SinkPtr> sinks;
std::vector<InputDevicePtr> inputDevices;


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

			printf("\tfps=%f(%d/%d) ", frameRate,
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


std::vector<std::string> devices;




void GetDevices()
{
	inputDevices.clear();


	DIR *dpdf;
	struct dirent *epdf;

	std::string path = "/dev/input/";

	dpdf = opendir(path.c_str());
	if (dpdf != NULL)
	{
		while (epdf = readdir(dpdf))
		{
			//printf("Filename: %s\n", epdf->d_name);
			// std::cout << epdf->d_name << std::endl;
			
			std::string entryName = path + std::string(epdf->d_name);
			//printf("Filename: %s\n", entryName.c_str());

			struct stat sb;
			if (stat(entryName.c_str(), &sb) == -1) {
				perror("stat");
				exit(EXIT_FAILURE);
			}

			//printf("File type:");

			//switch (sb.st_mode & S_IFMT) {
			//case S_IFBLK:  printf("block device\n");            break;
			//case S_IFCHR:  printf("character device\n");        break;
			//case S_IFDIR:  printf("directory\n");               break;
			//case S_IFIFO:  printf("FIFO/pipe\n");               break;
			//case S_IFLNK:  printf("symlink\n");                 break;
			//case S_IFREG:  printf("regular file\n");            break;
			//case S_IFSOCK: printf("socket\n");                  break;
			//default:       printf("unknown?\n");                break;
			//}

			//printf("\n");

			if (sb.st_mode & S_IFCHR)
			{
				devices.push_back(entryName);
				printf("added device: %s\n", entryName.c_str());
			}
		}

		for(auto device : devices)
		{
			printf("Device: %s\n", device.c_str());

			int fd = open(device.c_str(), O_RDONLY);
			if (fd < 0)
				throw Exception("device open failed.");

			//input_id id;
			//int io = ioctl(fd, EVIOCGID, &id);
			//if (io < 0)
			//{
			//	printf("EVIOCGID failed.\n");
			//}
			//else
			//{
			//	printf("\tbustype=%d, vendor=%d, product=%d, version=%d\n",
			//		id.bustype, id.vendor, id.product, id.version);
			//}

			//char name[255];
			//io = ioctl(fd, EVIOCGNAME(255), name);
			//if (io < 0)
			//{
			//	printf("EVIOCGNAME failed.\n");
			//}
			//else
			//{
			//	printf("\tname=%s\n", name);
			//}

			
			int bitsRequired = EV_MAX;
			int bytesRequired = (bitsRequired + 1) / 8;

			unsigned char buffer[bytesRequired];
			int io = ioctl(fd, EVIOCGBIT(0, bytesRequired), buffer);
			if (io < 0)
			{
				printf("EVIOCGBIT failed.\n");
			}
			else
			{
				unsigned int events = *((unsigned int*)buffer);

				if (events & EV_KEY)
				{
					InputDevicePtr inputDevice = std::make_shared<InputDevice>(device);
					inputDevices.push_back(inputDevice);
				}
			}
			
			close(fd);
		}
	}
}

struct option longopts[] = {
	{ "time",         required_argument,  NULL,          't' },
	{ "chapter",      required_argument,  NULL,          'c' },
	{ 0, 0, 0, 0 }
};



void Seek(double time)
{
	if (time < 0)
		time = 0;

	for (SinkPtr sink : sinks)
	{
		sink->Stop();
	}

	if (av_seek_frame(ctx, -1, (long)(time * AV_TIME_BASE), 0) < 0)
	{
		printf("av_seek_frame (%f) failed\n", time);
}
	else
	{
		printf("Seeked to %f\n", time);
	}

	for (SinkPtr sink : sinks)
	{
		sink->Start();
	}
}

#if 0
int main_old(int argc, char** argv)
{
#if 1
	GetDevices();
#endif

#if 0
	InputDevicePtr inputDevice = std::make_shared<InputDevice>(std::string("/dev/input/event3"));

#endif

	for (InputDevicePtr dev : inputDevices)
	{
		printf("Using input device: %s\n", dev->Name().c_str());
	}


	// Set graphics mode
	int ttyfd = open("/dev/tty0", O_RDWR);
	if (ttyfd < 0)
	{
		printf("Could not open /dev/tty0\n");
	}
	else
	{
		int ret = ioctl(ttyfd, KDSETMODE, KD_GRAPHICS);
		if (ret < 0)
			throw Exception("KDSETMODE failed.");

		close(ttyfd);
	}


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
	//std::vector<SinkPtr> sinks;
	AlsaAudioSinkPtr audioSink;
	AmlVideoSinkPtr videoSink;

	if (audio_stream_idx > -1)
	{
		audioSink = std::make_shared<AlsaAudioSink>(audio_codec_id, audio_sample_rate);
		audioSink->Start();
		audioSink->SetState(MediaState::Play);

		sinks.push_back(audioSink);
	}

	if (video_stream_idx > -1)
	{
		videoSink = std::make_shared<AmlVideoSink>(video_codec_id, frameRate, time_base);
		videoSink->Start();
		videoSink->SetState(MediaState::Play);

		if (audioSink)
		{
			audioSink->SetClockSink(videoSink);
		}

		AVStream* streamPtr = ctx->streams[video_stream_idx];
		AVCodecContext* codecCtxPtr = streamPtr->codec;

		videoSink->SetExtraData(codecCtxPtr->extradata);
		videoSink->SetExtraDataSize(codecCtxPtr->extradata_size);

		sinks.push_back(videoSink);
	}


	// Demux
	/*AVPacket pkt;
	av_init_packet(&pkt);
	pkt.data = NULL;
	pkt.size = 0;*/
	
	bool isPaused = false;


	while (isRunning )
	{		
		if (isPaused)
		{
			usleep(1);
		}
		else
		{
			//printf("Read packet.\n");


			AVPacketBufferPtr buffer = std::make_shared<AVPacketBuffer>();

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


		// Process Input
		int keycode;

		double currentTime = -1;
		if (audioSink)
		{
			currentTime = audioSink->GetLastTimeStamp();
		}
		else if(videoSink)
		{
			currentTime = videoSink->GetLastTimeStamp();
		}

		double newTime;

		for (InputDevicePtr dev : inputDevices)
		{
			while (dev->TryGetKeyPress(&keycode))
			{
				switch (keycode)
				{
				case KEY_HOME:	// odroid remote
				case KEY_MUTE:
				case KEY_MENU:
				case KEY_BACK:
				case KEY_VOLUMEDOWN:
				case KEY_VOLUMEUP:
					break;

				case KEY_POWER:	// odroid remote
				case KEY_ESC:
					isRunning = false;
					break;

				case KEY_FASTFORWARD:
					//TODO
					break;

				case KEY_REWIND:
					//TODO
					break;

				case KEY_ENTER:	// odroid remote
				case KEY_SPACE:
				case KEY_PLAYPAUSE:
				{
					// Pause
					for (SinkPtr sink : sinks)
					{
						if (isPaused)
						{
							sink->SetState(MediaState::Play);
						}
						else
						{
							sink->SetState(MediaState::Pause);
						}
					}

					isPaused = !isPaused;
				}
				break;

				case KEY_LEFT:
					// Backward
					if (!isPaused)
					{
						newTime = currentTime - 60.0; // 1 minute
						Seek(newTime);
					}
					break;

				case KEY_RIGHT:
					// Forward
					if (!isPaused)
					{
						newTime = currentTime + 60.0; // 1 minute			
						Seek(newTime);
					}
					break;

				case KEY_UP:
					if (!isPaused)
					{
						newTime = currentTime - 10.0 * 60; // 10 minutes
						Seek(newTime);
					}
					break;

				case KEY_DOWN:
					if (!isPaused)
					{
						newTime = currentTime + 10.0 * 60; // 10 minutes
						Seek(newTime);
					}
					break;

				default:
					break;
				}
			}
		}
	}


	// If not terminating, wait until all buffers are
	// finished playing
	if (audioSink)
		audioSink->Stop();

	if (videoSink)
		videoSink->Stop();


	// Set text mode
	ttyfd = open("/dev/tty0", O_RDWR);
	if (ttyfd < 0)
	{
		printf("Could not open /dev/tty0\n");
	}
	else
	{
		int ret = ioctl(ttyfd, KDSETMODE, KD_TEXT);
		if (ret < 0)
			throw Exception("KDSETMODE failed.");

		close(ttyfd);
	}

	return 0;
}
#endif


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

	
	// Initialize libav
	av_log_set_level(AV_LOG_VERBOSE);
	av_register_all();
	avformat_network_init();


	auto source = std::make_shared<MediaSourceElement>(std::string(url));
	source->SetName(std::string("Source"));
	source->Execute();


	auto videoSink = std::make_shared<AmlVideoSinkElement>();
	videoSink->SetName(std::string("VideoSink"));
	//videoSink->SetLogEnabled(true);
	videoSink->Execute();


	auto audioCodec = std::make_shared<AudioCodecElement>();
	audioCodec->SetName(std::string("AudioCodec"));
	audioCodec->Execute();


	//auto nullSink = std::make_shared<NullSink>();
	//nullSink->SetName(std::string("NullSink"));
	//nullSink->Execute();

	
	auto audioSink = std::make_shared<AlsaAudioSinkElement>();
	audioSink->SetName(std::string("AudioSink"));
	audioSink->Execute();


	// Wait for the source to create the pins
	source->WaitForExecutionState(ExecutionState::Executing);
	videoSink->WaitForExecutionState(ExecutionState::Executing);
	audioCodec->WaitForExecutionState(ExecutionState::Executing);
	audioSink->WaitForExecutionState(ExecutionState::Executing);


	// Connections
	source->Outputs()->Item(0)->Connect(videoSink->Inputs()->Item(0));
	source->Outputs()->Item(1)->Connect(audioCodec->Inputs()->Item(0));
	
	audioCodec->Outputs()->Item(0)->Connect(audioSink->Inputs()->Item(0));
	
	audioSink->Outputs()->Item(0)->Connect(videoSink->Inputs()->Item(1)); //clock


	// Start feeding data
	audioSink->SetState(MediaState::Play);
	//nullSink->SetState(MediaState::Play);
	audioCodec->SetState(MediaState::Play);
	videoSink->SetState(MediaState::Play);
	source->SetState(MediaState::Play);
	

	while (true) //source->Status() == ExecutionState::Executing)
	{
		usleep(1000);
	}

	return 0;
}