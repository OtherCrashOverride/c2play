#pragma once

extern "C"
{
	// aml_lib
#include <codec.h>
}

#include <vector>

#include "Codec.h"


class AmlVideoSink : public Sink, public virtual IClockSink
{
	const unsigned long PTS_FREQ = 90000;
	const long EXTERNAL_PTS = (1);
	const long SYNC_OUTSIDE = (2);

	std::vector<unsigned char> videoExtraData;
	AVCodecID codec_id;
	double frameRate;
	codec_para_t codecContext;
	void* extraData = nullptr;
	int extraDataSize = 0;
	AVRational time_base;


public:

	void* ExtraData() const
	{
		return extraData;
	}
	void SetExtraData(void* value)
	{
		extraData = value;
	}

	int ExtraDataSize() const
	{
		return extraDataSize;
	}
	void SetExtraDataSize(int value)
	{
		extraDataSize = value;
	}



	AmlVideoSink(AVCodecID codec_id, double frameRate, AVRational time_base)
		: Sink(),
		codec_id(codec_id),
		frameRate(frameRate),
		time_base(time_base)
	{
		memset(&codecContext, 0, sizeof(codecContext));

		codecContext.stream_type = STREAM_TYPE_ES_VIDEO;
		codecContext.has_video = 1;
		codecContext.am_sysinfo.param = (void*)(EXTERNAL_PTS | SYNC_OUTSIDE);
		codecContext.am_sysinfo.rate = 96000.5 * (1.0 / frameRate);

		switch (codec_id)
		{
		case CODEC_ID_MPEG2VIDEO:
			printf("AmlVideoSink - VIDEO/MPEG2\n");

			codecContext.video_type = VFORMAT_MPEG12;
			codecContext.am_sysinfo.format = VIDEO_DEC_FORMAT_UNKNOW;
			break;

		case CODEC_ID_MPEG4:
			printf("AmlVideoSink - VIDEO/MPEG4\n");

			codecContext.video_type = VFORMAT_MPEG4;
			codecContext.am_sysinfo.format = VIDEO_DEC_FORMAT_MPEG4_5;
			//VIDEO_DEC_FORMAT_MP4; //VIDEO_DEC_FORMAT_MPEG4_3; //VIDEO_DEC_FORMAT_MPEG4_4; //VIDEO_DEC_FORMAT_MPEG4_5;
			break;

		case CODEC_ID_H264:
		{
			printf("AmlVideoSink - VIDEO/H264\n");

			codecContext.video_type = VFORMAT_H264_4K2K;
			codecContext.am_sysinfo.format = VIDEO_DEC_FORMAT_H264_4K2K;
		}
		break;

		case AV_CODEC_ID_HEVC:
			printf("AmlVideoSink - VIDEO/HEVC\n");

			codecContext.video_type = VFORMAT_HEVC;
			codecContext.am_sysinfo.format = VIDEO_DEC_FORMAT_HEVC;
			break;


		//case CODEC_ID_VC1:
		//	printf("stream #%d - VIDEO/VC1\n", i);
		//	break;

		default:
			printf("AmlVideoSink - VIDEO/UNKNOWN(%x)\n", codec_id);
			throw NotSupportedException();
		}

		printf("\tfps=%f ", frameRate);

		printf("am_sysinfo.rate=%d ",
			codecContext.am_sysinfo.rate);


		//int width = codecCtxPtr->width;
		//int height = codecCtxPtr->height;

		//printf("SAR=(%d/%d) ",
		//	streamPtr->sample_aspect_ratio.num,
		//	streamPtr->sample_aspect_ratio.den);

		// TODO: DAR

		printf("\n");

	


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

	}

	~AmlVideoSink()
	{
		if (IsRunning())
		{
			Stop();
		}

		codec_close(&codecContext);

		WriteToFile("/sys/class/graphics/fb0/blank", "0");
	}



	// Inherited via IClockSink
	virtual void SetTimeStamp(double value) override
	{
		unsigned long pts = (unsigned long)(value * PTS_FREQ);

		int codecCall = codec_set_pcrscr(&codecContext, (int)pts);
		if (codecCall != 0)
		{
			printf("codec_set_pcrscr failed.\n");
		}
	}



	//// Inherited via Sink
	//virtual void AddBuffer(PacketBufferPtr buffer) override
	//{
	//	throw NotImplementedException();
	//}

	//virtual void Start() override
	//{
	//	throw NotImplementedException();
	//}

	//virtual void Stop() override
	//{
	//	throw NotImplementedException();
	//}

	//virtual void SetState(MediaState state) override
	//{
	//	throw NotImplementedException();
	//}

	//virtual void Flush() override
	//{
	//	throw NotImplementedException();
	//}


	// Member
private:
	void ConvertH264ExtraDataToAnnexB()
	{
		void* video_extra_data = ExtraData();
		int video_extra_data_size = ExtraDataSize();

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

#if 0
		printf("EXTRA DATA = ");

		for (int i = 0; i < videoExtraData.size(); ++i)
		{
			printf("%02x ", videoExtraData[i]);

		}

		printf("\n");
#endif
	}


	void HevcExtraDataToAnnexB()
	{
		void* video_extra_data = ExtraData();
		int video_extra_data_size = ExtraDataSize();


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

#if 0
			printf("EXTRA DATA = ");

			for (int i = 0; i < videoExtraData.size(); ++i)
			{
				printf("%02x ", videoExtraData[i]);
			}

			printf("\n");
#endif
		}
	}

	static void WriteToFile(const char* path, const char* value)
	{
		int fd = open(path, O_RDWR | O_TRUNC, 0644);
		if (fd < 0)
		{
			printf("WriteToFile open failed: %s = %s\n", path, value);
			throw Exception();
		}

		if (write(fd, value, strlen(value)) < 0)
		{
			printf("WriteToFile write failed: %s = %s\n", path, value);
			throw Exception();
		}

		close(fd);
	}


protected:
	virtual void WorkThread() override
	{
		WriteToFile("/sys/class/graphics/fb0/blank", "1");


		//switch (video_codec_id)
		//{
		//case CODEC_ID_H264:
		//	ConvertH264ExtraDataToAnnexB();
		//	break;

		//case AV_CODEC_ID_HEVC:
		//	HevcExtraDataToAnnexB();
		//	break;
		//}

		printf("AmlVideoSink entering running state.\n");


		bool isFirstVideoPacket = true;
		bool isAnnexB = false;
		bool isExtraDataSent = false;
		long estimatedNextPts = 0;

		while (IsRunning())
		{
			if (State() != MediaState::Play)
			{
				usleep(1);
			}
			else
			{
				PacketBufferPtr buffer;

				if (!TryGetBuffer(&buffer))
				{
					usleep(1);
				}
				else
				{
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
						(codec_id == CODEC_ID_H264 ||
						 codec_id == AV_CODEC_ID_HEVC))
					{
						//unsigned char* nalHeader = (unsigned char*)pkt.data;

						// Five least significant bits of first NAL unit byte signify nal_unit_type.
						int nal_unit_type;
						const int nalHeaderLength = 4;

						while (nalHeader < (pkt->data + pkt->size))
						{
							switch (codec_id)
							{
							case CODEC_ID_H264:
								//if (!isExtraDataSent)
							{
								// Copy AnnexB data if NAL unit type is 5
								nal_unit_type = nalHeader[nalHeaderLength] & 0x1F;

								if (nal_unit_type == 5)
								{
									ConvertH264ExtraDataToAnnexB();

									int api = codec_write(&codecContext, &videoExtraData[0], videoExtraData.size());
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
									HevcExtraDataToAnnexB();

									int attempts = 10;
									int api;
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
						double timeStamp = av_q2d(time_base) * pkt->pts;
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
							double timeStamp = av_q2d(time_base) * estimatedNextPts;
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

		}

		printf("AmlVideoSink exiting running state.\n");
	}

};
