#pragma once

#include "Codec.h"
#include "Element.h"
#include "OutPin.h"
#include "EventListener.h"


#include <string>
#include <map>


extern "C"
{
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
}


#include <memory>


struct Chapter
{
	std::string Title;
	double Start;
	double End;
};

typedef std::vector<Chapter> ChapterList;
typedef std::shared_ptr<ChapterList> ChapterListSPTR;


#define NewSPTR std::make_shared
#define NewUPTR std::make_unique


class MediaSourceElement : public Element
{
	const int BUFFER_COUNT = 64;  // enough for 4k video

	std::string url;
	AVFormatContext* ctx = nullptr;
	
	//int video_stream_idx = -1;
	//void* video_extra_data;
	//int video_extra_data_size = 0;
	//AVCodecID video_codec_id;
	//double frameRate;
	//AVRational time_base;
	//OutPinSPTR videoPin;


	//int audio_stream_idx = -1;
	//AVCodecID audio_codec_id;
	//int audio_sample_rate = 0;
	//int audio_channels = 0;
	//OutPinSPTR audioPin;

	//std::map<int, OutPinSPTR> streamMap;
	ThreadSafeQueue<BufferSPTR> availableBuffers;
	std::vector<OutPinSPTR> streamList;
	std::vector<unsigned long> streamNextPts;

	ChapterListSPTR chapters = NewSPTR<ChapterList>();
	
	EventListenerSPTR<EventArgs> bufferReturnedListener;

	unsigned long lastPts = 0;


	void outPin_BufferReturned(void* sender, const EventArgs& args)
	{
		//printf("MediaSourceElement: outPin_BufferReturned entered.\n");

		OutPin* outPin = (OutPin*)sender;
		BufferSPTR buffer;
		
		if (outPin->TryGetAvailableBuffer(&buffer))
		{
			switch (buffer->Type())
			{
			case BufferTypeEnum::AVPacket:
			{
				// Free the memory allocated to the buffers by libav
				AVPacketBufferPTR avbuffer = std::static_pointer_cast<AVPacketBuffer>(buffer);
				avbuffer->Reset();

				// Reuse the buffer
				availableBuffers.Push(buffer);
				Wake();

				break;
			}

			case BufferTypeEnum::Marker:
			default:
				break;
			}
		}
		else
		{
			throw Exception("Unexpected state: buffer returned but not available.");
		}

		//printf("MediaSourceElement: outPin_BufferReturned finished.\n");
	}



	static void PrintDictionary(AVDictionary* dictionary)
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

		for (int i = 0; i < streamCount; ++i)
		{
			streamList.push_back(nullptr);
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
				VideoPinInfoSPTR info = std::make_shared<VideoPinInfo>();
				info->FrameRate = av_q2d(streamPtr->avg_frame_rate);;
				info->Width = codecCtxPtr->width;
				info->Height = codecCtxPtr->height;

				ExtraDataSPTR ext = std::make_shared<ExtraData>();
				info->ExtraData = ext;

				// Copy codec extra data
				unsigned char* src = codecCtxPtr->extradata;
				int size = codecCtxPtr->extradata_size;

				for (int j = 0; j < size; ++j)
				{
					ext->push_back(src[j]);
				}

#if 1
				printf("EXTRA DATA = ");
				
				for (int j = 0; j < size; ++j)
				{
					printf("%02x ", src[j]);
				
				}
				
				printf("\n");
#endif

				if (url.compare(url.size() - 4, 4, ".avi") == 0)
				{
					info->HasEstimatedPts = true;
					//printf("MediaSourceElement: info->HasEstimatedPts = true\n");
				}

				ElementWPTR weakPtr = shared_from_this();
				OutPinSPTR videoPin = std::make_shared<OutPin>(weakPtr, info);
				videoPin->SetName("Video");

				AddOutputPin(videoPin);

				//streamMap[i] = videoPin;
				streamList[i] = videoPin;

//				if (video_stream_idx < 0)
//				{
//					video_stream_idx = i;
//					video_extra_data = codecCtxPtr->extradata;
//					video_extra_data_size = codecCtxPtr->extradata_size;
//
//					video_codec_id = codec_id;
//
//
//					frameRate = av_q2d(streamPtr->avg_frame_rate);
//					time_base = streamPtr->time_base;
//
//					info = std::make_shared<VideoPinInfo>();
//					info->FrameRate = frameRate;
//					info->Width = codecCtxPtr->width;
//					info->Height = codecCtxPtr->height;
//
//					
//					ExtraDataSPTR ext = std::make_shared<ExtraData>();
//					info->ExtraData = ext;
//
//					// Copy codec extra data
//					unsigned char* src = codecCtxPtr->extradata;
//					int size = codecCtxPtr->extradata_size;
//
//					for (int j = 0; j < size; ++j)
//					{
//						ext->push_back(src[j]);
//					}
//					printf("MediaSourceElement: copied extra data (size=%d)\n", size);
//#if 0
//					printf("EXTRA DATA = ");
//
//					for (int j = 0; j < size; ++j)
//					{
//						printf("%02x ", src[j]);
//
//					}
//
//					printf("\n");
//#endif
//
//					ElementWPTR weakPtr = shared_from_this();
//					videoPin = std::make_shared<OutPin>(weakPtr, info);
//					videoPin->SetName("Video");
//
//					AddOutputPin(videoPin);
//
//					//streamMap[i] = videoPin;
//					streamList[i] = videoPin;
//				}

				switch (codec_id)
				{
				case CODEC_ID_MPEG2VIDEO:
					printf("stream #%d - VIDEO/MPEG2\n", i);
					if (info)
						info->Format = VideoFormatEnum::Mpeg2;
					break;

				case AV_CODEC_ID_MSMPEG4V3:
					printf("stream #%d - VIDEO/MPEG4V3\n", i);
					if (info)
						info->Format = VideoFormatEnum::Mpeg4V3;
					break;

				case CODEC_ID_MPEG4:
					printf("stream #%d - VIDEO/MPEG4\n", i);
					if (info)
						info->Format = VideoFormatEnum::Mpeg4;
					break;

				case CODEC_ID_H264:				
					printf("stream #%d - VIDEO/H264\n", i);
					if (info)
						info->Format = VideoFormatEnum::Avc;
				
					break;

				case AV_CODEC_ID_HEVC:
					printf("stream #%d - VIDEO/HEVC\n", i);
					if (info)
						info->Format = VideoFormatEnum::Hevc;
					break;

				case CODEC_ID_VC1:
					printf("stream #%d - VIDEO/VC1\n", i);
					if (info)
						info->Format = VideoFormatEnum::VC1;
					break;

				default:
					printf("stream #%d - VIDEO/UNKNOWN(%x)\n", i, codec_id);
					if (info)
						info->Format = VideoFormatEnum::Unknown;
					//throw NotSupportedException();
				}


				//int width = codecCtxPtr->width;
				//int height = codecCtxPtr->height;

				printf("\tw=%d h=%d ", info->Width, info->Height);

				printf("fps=%f(%d/%d) ", info->FrameRate,
					streamPtr->avg_frame_rate.num,
					streamPtr->avg_frame_rate.den);

				printf("SAR=(%d/%d) ",
					streamPtr->sample_aspect_ratio.num,
					streamPtr->sample_aspect_ratio.den);

				// TODO: DAR

				printf("\n");

			}
			break;

			case AVMEDIA_TYPE_AUDIO:
			{
				AudioPinInfoSPTR info = std::make_shared<AudioPinInfo>();
				info->Channels = codecCtxPtr->channels;
				info->SampleRate = codecCtxPtr->sample_rate;
				info->Format = AudioFormatEnum::Unknown;

				//printf("MediaSourceElement: audio SampleRate=%d\n", info->SampleRate);

				OutPinSPTR audioPin = std::make_shared<OutPin>(shared_from_this(), info);
				audioPin->SetName("Audio");

				AddOutputPin(audioPin);

				streamList[i] = audioPin;

				//// Use the first audio stream
				//if (audio_stream_idx == -1)
				//{
				//	audio_stream_idx = i;
				//	audio_codec_id = codec_id;

				//	info = std::make_shared<AudioPinInfo>();
				//	info->Channels = codecCtxPtr->channels;
				//	info->SampleRate = codecCtxPtr->sample_rate;
				//	info->Format = AudioFormatEnum::Unknown;

				//	printf("MediaSourceElement: audio SampleRate=%d\n", info->SampleRate);

				//	audioPin = std::make_shared<OutPin>(shared_from_this(), info);
				//	audioPin->SetName("Audio");

				//	AddOutputPin(audioPin);

				//	streamList[i] = audioPin;
				//}


				switch (codec_id)
				{
				case CODEC_ID_MP3:
					printf("stream #%d - AUDIO/MP3\n", i);
					if (info)
						info->Format = AudioFormatEnum::Mpeg2Layer3;
					break;

				case CODEC_ID_AAC:
					printf("stream #%d - AUDIO/AAC\n", i);
					if (info)
						info->Format = AudioFormatEnum::Aac;
					break;

				case CODEC_ID_AC3:
					printf("stream #%d - AUDIO/AC3\n", i);
					if (info)
						info->Format = AudioFormatEnum::Ac3;
					break;

				case CODEC_ID_DTS:
					printf("stream #%d - AUDIO/DTS\n", i);
					if (info)
						info->Format = AudioFormatEnum::Dts;
					break;

				case AV_CODEC_ID_WMAPRO:
					printf("stream #%d - AUDIO/WMAPRO\n", i);
					if (info)
						info->Format = AudioFormatEnum::WmaPro;
					break;

				case AV_CODEC_ID_TRUEHD:
					printf("stream #%d - AUDIO/TRUEHD\n", i);
					if (info)
						info->Format = AudioFormatEnum::DolbyTrueHD;
					break;

				case AV_CODEC_ID_EAC3:
					printf("stream #%d - AUDIO/EAC3\n", i);
					if (info)
						info->Format = AudioFormatEnum::EAc3;
					break;

					//case AVCodecID.CODEC_ID_WMAV2:
					//    break;

				default:
					printf("stream #%d - AUDIO/UNKNOWN (0x%x)\n", i, codec_id);
					//throw NotSupportedException();
					break;
				}

				//audio_channels = codecCtxPtr->channels;
				//audio_sample_rate = codecCtxPtr->sample_rate;
			}
			break;


			case AVMEDIA_TYPE_SUBTITLE:
			{
				// TODO: Subtitle support

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


public:

	const ChapterListSPTR Chapters() const
	{
		return chapters;
	}


	MediaSourceElement(std::string url)
		: url(url)
	{
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

		int ret = avformat_open_input(&ctx, url.c_str(), NULL, &options_dict);
		if (ret < 0)
		{
			printf("avformat_open_input failed.\n");
		}


		printf("Source Metadata:\n");
		PrintDictionary(ctx->metadata);


		//SetupPins();


		// Chapters
		int chapterCount = ctx->nb_chapters;
		//printf("Chapters (count=%d):\n", chapterCount);

		AVChapter** avChapters = ctx->chapters;
		for (int i = 0; i < chapterCount; ++i)
		{
			AVChapter* avChapter = avChapters[i];

			int index = i + 1;
			double start = avChapter->start * avChapter->time_base.num / (double)avChapter->time_base.den;
			double end = avChapter->end * avChapter->time_base.num / (double)avChapter->time_base.den;
			AVDictionary* metadata = avChapter->metadata;

			//printf("Chapter #%02d: %f -> %f\n", index, start, end);
			//PrintDictionary(metadata);

			Chapter chapter;
			chapter.Start = start;
			chapter.End = end;

			AVDictionaryEntry* titleEntry = av_dict_get(
				metadata,
				"title",
				NULL,
				0);

			if (titleEntry)
			{
				chapter.Title = std::string(titleEntry->value);
			}
			
			chapters->push_back(chapter);
		}


		//if (optionStartPosition > 0)
		//{
		//	if (av_seek_frame(ctx, -1, (long)(optionStartPosition * AV_TIME_BASE), 0) < 0)
		//	{
		//		printf("av_seek_frame (%f) failed\n", optionStartPosition);
		//	}
		//}



	}

	virtual void Initialize() override
	{
		ClearInputPins();
		ClearOutputPins();

		SetupPins();

		for (size_t i = 0; i < streamList.size(); ++i)
		{
			streamNextPts.push_back(0);
		}


		// Event handlers
		bufferReturnedListener = std::make_shared<EventListener<EventArgs>>(
			std::bind(&MediaSourceElement::outPin_BufferReturned, this, std::placeholders::_1, std::placeholders::_2));
		
		for (auto item : streamList)
		{
			if (item)
			{
				item->BufferReturned.AddListener(bufferReturnedListener);
			}
		}


		// Chapters
		int index = 0;
		for (auto& chapter : *chapters)
		{
			printf("Chapter[%02d] = '%s' : %f - %f\n", index, chapter.Title.c_str(), chapter.Start, chapter.End);
			++index;
		}


		// Create buffers
		for (int i = 0; i < BUFFER_COUNT; ++i)
		{
			AVPacketBufferPtr buffer = std::make_shared<AVPacketBuffer>(shared_from_this());
			availableBuffers.Push(buffer);
		}
	}


	virtual void DoWork() override
	{
		BufferSPTR freeBuffer;

		//// Reap freed buffers
		//for (auto& entry : streamList)
		//{
		//	//printf("MediaElement (%s) DoWork checking pin for reaping.\n", Name().c_str());

		//	OutPinSPTR pin = entry;
		//	if (pin)
		//	{
		//		//printf("MediaElement (%s) DoWork reaping buffers for pin.\n", Name().c_str());

		//		while (pin->TryGetAvailableBuffer(&freeBuffer))
		//		{
		//			// Drop marker buffers
		//			switch (freeBuffer->Type())
		//			{
		//				case BufferTypeEnum::AVPacket:
		//				{
		//					//// Free the memory allocated to the buffers by libav
		//					AVPacketBufferPTR buffer = std::static_pointer_cast<AVPacketBuffer>(freeBuffer);
		//					buffer->Reset();

		//					// Reuse the buffer
		//					availableBuffers.Push(freeBuffer);
		//					Wake();

		//					break;
		//				}

		//				case BufferTypeEnum::Marker:
		//				default:
		//					break;
		//			}

		//			////printf("MediaElement (%s) DoWork buffer reaped.\n", Name().c_str());

		//			//RetireBuffer(buffer);
		//		}
		//	}
		//}


		//printf("MediaElement (%s) DoWork availableBuffers count=%d.\n", Name().c_str(), availableBuffers.Count());

		// Process
		if (availableBuffers.TryPop(&freeBuffer))
		{
			//printf("MediaElement (%s) DoWork availableBuffers.TryPop=true.\n", Name().c_str());

			AVPacketBufferPTR buffer = std::static_pointer_cast<AVPacketBuffer>(freeBuffer);

			if (av_read_frame(ctx, buffer->GetAVPacket()) < 0)
			{
				// End of file

				// Free the memory allocated to the buffers by libav
				buffer->Reset();
				availableBuffers.Push(buffer);
				//Wake();

				// Send all Output Pins an EOS buffer
				{
					for (int i = 0; i < Outputs()->Count(); ++i)
					{
						MarkerBufferSPTR eosBuffer = std::make_shared<MarkerBuffer>(shared_from_this(), MarkerEnum::EndOfStream);
						Outputs()->Item(i)->SendBuffer(eosBuffer);
					}
				}

				//SetExecutionState(ExecutionStateEnum::Idle);
				SetState(MediaState::Pause);

				//printf("MediaElement (%s) DoWork av_read_frame failed.\n", Name().c_str());
				//break;
			}
			else
			{
				AVPacket* pkt = buffer->GetAVPacket();

				//printf("MediaElement (%s) DoWork pin[%d] got AVPacket.\n", Name().c_str(), pkt->stream_index);

				AVStream* streamPtr = ctx->streams[pkt->stream_index];
				buffer->SetTimeBase(streamPtr->time_base);

				if (pkt->pts != AV_NOPTS_VALUE)
				{
					buffer->SetTimeStamp(av_q2d(streamPtr->time_base) * pkt->pts);
					streamNextPts[pkt->stream_index] = pkt->pts + pkt->duration;
					//printf("MediaSourceElement: [stream %d] Set buffer timestamp=%f\n", pkt->stream_index, buffer->TimeStamp());

					// keep the pts for seek flags
					lastPts = pkt->pts;
				}
				else
				{
					buffer->SetTimeStamp(av_q2d(streamPtr->time_base) * streamNextPts[pkt->stream_index]);
					streamNextPts[pkt->stream_index] += pkt->duration;

					//printf("MediaSourceElement: [stream %d] No timestamp.\n", pkt->stream_index);
				}

				

				//AddFilledBuffer(buffer);
				OutPinSPTR pin = streamList[pkt->stream_index];
				if(pin)
				{
					pin->SendBuffer(freeBuffer);
					//printf("MediaElement (%s) DoWork pin[%d] buffer sent.\n", Name().c_str(), pkt->stream_index);
				}
				else
				{
					// Free the memory allocated to the buffers by libav
					buffer->Reset();

					availableBuffers.Push(buffer);
					Wake();

					//RetireBuffer(buffer);

					//printf("MediaElement (%s) DoWork pin[%d] = nullptr.\n", Name().c_str(), pkt->stream_index);
				}
			}
		}

		//printf("MediaSourceElement: availableBuffers=%d\n", availableBuffers.Count());
	}

	//virtual void Flush() override
	//{
	//}

	void Seek(double timeStamp)
	{
		if (ExecutionState() != ExecutionStateEnum::Idle)
		{
			throw InvalidOperationException();
		}

		int flags = AVFMT_SEEK_TO_PTS;
		unsigned long seekPts = (long)(timeStamp * AV_TIME_BASE);
	
		if (seekPts < lastPts)
		{
			flags |= AVSEEK_FLAG_BACKWARD;
		}

		if (av_seek_frame(ctx, -1, (long)(timeStamp * AV_TIME_BASE), flags) < 0)
		{
			printf("av_seek_frame (%f) failed\n", timeStamp);
		}


		// Send all Output Pins a Discontinue marker
		for (int i = 0; i < Outputs()->Count(); ++i)
		{
			MarkerBufferSPTR marker = std::make_shared<MarkerBuffer>(shared_from_this(), MarkerEnum::Discontinue);
			Outputs()->Item(i)->SendBuffer(marker);
		}
		
	}

};