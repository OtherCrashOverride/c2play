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

#include "SubtitleCodecElement.h"


void SubtitleDecoderElement::static_msg_callback(int level, const char* fmt, va_list va, void *data)
{
	//SubtitleDecoderElement* this_ = (SubtitleDecoderElement*)data;
	//if (level > 6)
	//	return;
//#ifdef DEBUG
//	char buffer[4096] = { 0 };
//	vsprintf(buffer, fmt, va);
//	std::string msg = "static_msg_callback: " + std::string(buffer) + "\n";
//	printf("%s", msg.c_str());
//#endif // DEBUG
}

void SubtitleDecoderElement::SetupCodec()
{
	SubtitleFormatEnum format = inPin->InfoAs()->Format;
	switch (format)
	{
		case SubtitleFormatEnum::SubRip:
			avcodec = avcodec_find_decoder(AV_CODEC_ID_SUBRIP);
			break;

		case SubtitleFormatEnum::Pgs:
			avcodec = avcodec_find_decoder(CODEC_ID_HDMV_PGS_SUBTITLE);
			break;

		case SubtitleFormatEnum::Dvb:
			avcodec = avcodec_find_decoder(CODEC_ID_DVB_SUBTITLE);
			break;

		case SubtitleFormatEnum::DvbTeletext:
			avcodec = avcodec_find_decoder(CODEC_ID_DVB_TELETEXT);
			break;

		default:
			printf("Subtitle format %d is not supported.\n", (int)format);
			throw NotSupportedException();
	}

	if (!avcodec)
	{
		throw Exception("codec not found\n");
	}


	avcodecContext = avcodec_alloc_context3(avcodec);
	if (!avcodecContext)
	{
		throw Exception("avcodec_alloc_context3 failed.\n");
	}

	// open codec
	if (avcodec_open2(avcodecContext, avcodec, NULL) < 0)
	{
		throw Exception("could not open codec\n");
	}


	//const char* header = (const char*)avcodecContext->subtitle_header;
	//const int header_size = avcodecContext->subtitle_header_size;
	//if (header && header > 0)
	//{
	//	ass_process_data(ass_track, (char*)header, header_size);
	//}
}

void SubtitleDecoderElement::ProcessBuffer(AVPacketBufferSPTR buffer)
{
	AVPacket* pkt = buffer->GetAVPacket();
	
	if (avSubtitle == nullptr)
	{
		avSubtitle = (AVSubtitle*)calloc(1, sizeof(AVSubtitle));
	}

		//AVFrame* frame = av_frame_alloc();
	//avsubtitle_free (AVSubtitle *sub)

	//avcodecContext->pkt_timebase = buffer->TimeBase();
	av_codec_set_pkt_timebase(avcodecContext, buffer->TimeBase());
	printf("buffer->TimeBase() = num=%u, den=%u\n", buffer->TimeBase().num, buffer->TimeBase().den);


	int bytesDecoded = 0;
	while (IsExecuting() && bytesDecoded < pkt->size)
	{
		int got_sub_ptr;
		int len = avcodec_decode_subtitle2(avcodecContext,
			avSubtitle,
			&got_sub_ptr,
			pkt);


		if (len < 0)
		{
			// Report the error, but otherwise ignore it.				
			char errmsg[1024] = { 0 };
			av_strerror(len, errmsg, 1024);

			Log("Error while decoding: %s\n", errmsg);

			break;
		}
		else
		{
			bytesDecoded += len;
		}

		//Log("decoded audio frame OK (len=%x, pkt.size=%x)\n", len, buffer->GetAVPacket()->size);


		// Convert audio to ALSA format
		if (got_sub_ptr)
		{			
			//typedef struct AVSubtitle {
			//3300     uint16_t format; /* 0 = graphics */
			//3301     uint32_t start_display_time; /* relative to packet pts, in ms */
			//3302     uint32_t end_display_time; /* relative to packet pts, in ms */
			//3303     unsigned num_rects;
			//3304     AVSubtitleRect **rects;
			//3305     int64_t pts;    ///< Same as packet pts, in AV_TIME_BASE
			//3306
			//} AVSubtitle;
			
			double timeStamp = pkt->pts * av_q2d(buffer->TimeBase());
			double duration = pkt->duration * av_q2d(buffer->TimeBase());

			printf("Subtitle: pts=%f duration=%f\n",
				timeStamp,  //avSubtitle->pts / (double)AV_TIME_BASE,
				duration);

			printf("Subtitle: format=%d, start_display_time=%d, end_display_time=%d, num_rects=%d, pts=%lu\n",
				avSubtitle->format, avSubtitle->start_display_time, avSubtitle->end_display_time, avSubtitle->num_rects, avSubtitle->pts);

			if (avSubtitle->num_rects < 1)
			{
				printf("Subtitle: No Rects\n");
			}


			bool hasAssa = false;
			ImageListSPTR imageList = std::make_shared<ImageList>();

			for (unsigned int i = 0; i < avSubtitle->num_rects; i++)
			{
				AVSubtitleRect* rect = avSubtitle->rects[i];

#if 0 // AVSubtitleRect definition
				//typedef struct AVSubtitleRect {
				//	3274     int x;         ///< top left corner  of pict, undefined when pict is not set
				//	3275     int y;         ///< top left corner  of pict, undefined when pict is not set
				//	3276     int w;         ///< width            of pict, undefined when pict is not set
				//	3277     int h;         ///< height           of pict, undefined when pict is not set
				//	3278     int nb_colors; ///< number of colors in pict, undefined when pict is not set
				//	3279
				//		3280     /**
				//				 3281      * data+linesize for the bitmap of this subtitle.
				//				 3282      * can be set for text/ass as well once they where rendered
				//				 3283      */
				//		3284     AVPicture pict;
				//	3285     enum AVSubtitleType type;
				//	3286
				//		3287     char *text;                     ///< 0 terminated plain UTF-8 text
				//	3288
				//		3289     /**
				//				 3290      * 0 terminated ASS/SSA compatible event line.
				//				 3291      * The presentation of this is unaffected by the other values in this
				//				 3292      * struct.
				//				 3293      */
				//		3294     char *ass;
				//	3295
				//		3296     int flags;
				//	3297
				//} AVSubtitleRect;
#endif

				printf("Subtitle: Rect - x=%d, y=%d, w=%d, h=%d, nb_colors=%d, text=%p, ass=%p\n",
					rect->x, rect->y, rect->w, rect->h, rect->nb_colors, rect->text, rect->ass);


#if 0 //AVPicture definition
				//3439 typedef struct AVPicture {
				//	3440     uint8_t *data[AV_NUM_DATA_POINTERS];    ///< pointers to the image data planes
				//	3441     int linesize[AV_NUM_DATA_POINTERS];     ///< number of bytes per line
				//	3442
				//} AVPicture;
#endif

				//for (int j = 0; j < AV_NUM_DATA_POINTERS; ++j)
				//{
				//	printf("Subtitle: pict: data[%d]=%p linesize=%d\n", j, rect->pict.data[j], rect->pict.linesize[j]);
				//}


				switch (rect->type)
				{
					case SUBTITLE_BITMAP:
					{
						printf("Subtitle:\tBITMAP\n");
						
						AllocatedImageSPTR image = std::make_shared<AllocatedImage>(ImageFormatEnum::R8G8B8A8,
							rect->w, rect->h);
						unsigned int* imageData = (unsigned int*)image->Data();

						unsigned char* pixData = rect->pict.data[0];
						unsigned int* paletteData = (unsigned int*)rect->pict.data[1];

						for (int y = 0; y < rect->h; ++y)
						{
							for (int x = 0; x < rect->w; ++x)
							{
								unsigned char index = *(pixData++);								
								unsigned int color = paletteData[index];

								//color |= 0xff0000ff;

								//printf("0x%08x ", color);

								imageData[y * rect->w + x] = color;
							}
						}

						printf("\n");

						ImageBufferSPTR imageBuffer = std::make_shared<ImageBuffer>(
							shared_from_this(),
							image);
						imageBuffer->SetTimeStamp(pkt->pts * av_q2d(buffer->TimeBase()));
						imageBuffer->SetDuration(pkt->duration * av_q2d(buffer->TimeBase()));
						imageBuffer->SetX(rect->x);
						imageBuffer->SetY(rect->y);

						//outPin->SendBuffer(imageBuffer);
						imageList->push_back(imageBuffer);
						break;
					}
						

					case SUBTITLE_TEXT:
						printf("Subtitle:\tTEXT: %s\n", rect->text);
						break;

					case SUBTITLE_ASS:
					{
						printf("Subtitle:\tSSA/ASSA: %s\n", rect->ass);
						//printf("----\n");
						//for (int k = 0; k < pkt->size; ++k)
						//{
						//	printf("%c", pkt->data[k]);
						//}
						//printf("\n----\n");


						if (!isSaaHeaderSent)
						{
							//printf("Subtitle:\tsubtitle_header=%s\n", avcodecContext->subtitle_header);	//->subtitle_header_size
							const char* header = (const char*)avcodecContext->subtitle_header;
							const int header_size = avcodecContext->subtitle_header_size + 1;
							if (header && header > 0)
							{
								//ass_process_data(ass_track, (char*)header, header_size);
								ass_process_codec_private(ass_track, (char*)header, header_size);
							}

							isSaaHeaderSent = true;
						}


						char *ass_line = rect->ass;
						if (!ass_line)
							break;

						//size_t len = strlen(ass_line);
						//ass_process_data(ass_track, ass_line, len);

						std::string assString(ass_line);
						std::string targetString("0:00:00.00,0:00:00.00");


						std::size_t found = assString.find(targetString);
						if (found != std::string::npos)
						{
							printf("replacing bogus timestamp\n");

							char buffer[1024];

							int s_hrs = (int)(timeStamp / 3600);
							int s_min = (int)((timeStamp - (s_hrs * 3600)) / 60);
							double s_sec = (timeStamp - s_hrs * 3600 - s_min * 60);
							sprintf(buffer, "%d:%02d:%02.2f", s_hrs, s_min, s_sec);
							printf("Start Time=%s\n", buffer);
							
							std::string start(buffer);


							double endTime = timeStamp + duration;
							int e_hrs = (int)(endTime / 3600);
							int e_min = (int)((endTime - (e_hrs * 3600)) / 60);
							double e_sec = (endTime - e_hrs * 3600 - e_min * 60);
							sprintf(buffer, "%d:%02d:%02.2f", e_hrs, e_min, e_sec);
							printf("End Time=%s\n", buffer);

							std::string end(buffer);

							std::string tmp;
							tmp.append(assString.substr(0, found + 1));
							tmp.append(start);
							tmp.append(",");
							tmp.append(end);
							tmp.append(assString.substr(found + targetString.size()));

							printf("New string=%s\n", tmp.c_str());
							assString = tmp;
						}

						//ass_process_chunk
						ass_process_data(ass_track, (char*)assString.c_str(), assString.size()); //, // +/- 12
						//	(long long)(timeStamp * 1000), (long long)(duration * 1000));

						hasAssa = true;


						//ass_flush_events(ass_track);

						//long long now = (long long)((timeStamp + duration / 2) * 1000);
						//int changed;
						//ASS_Image *img =
						//	ass_render_frame(ass_renderer, ass_track, now, &changed);

						//if (img)
						//{
						//	printf("ass_render_frame OK\n");
#if 0
						//	//typedef struct ass_image {
						//	//	int w, h;                   // Bitmap width/height
						//	//	int stride;                 // Bitmap stride
						//	//	unsigned char *bitmap;      // 1bpp stride*h alpha buffer
						//	//								// Note: the last row may not be padded to
						//	//								// bitmap stride!
						//	//	uint32_t color;             // Bitmap color and alpha, RGBA
						//	//	int dst_x, dst_y;           // Bitmap placement inside the video frame

						//	//	struct ass_image *next;   // Next image, or NULL

						//	//	enum {
						//	//		IMAGE_TYPE_CHARACTER,
						//	//		IMAGE_TYPE_OUTLINE,
						//	//		IMAGE_TYPE_SHADOW
						//	//	} type;

						//	//} ASS_Image;
#endif
						//	printf("ASS_Image: w=%d, h=%d, stride=%d, bitmap=%p, color=0x%04x, dst_x=%d, dst_y=%d, next=%p, type=%d\n",
						//		img->w, img->h, img->stride, img->bitmap, img->color, img->dst_x, img->dst_y, img->next, img->type);

						//}

						break;
					}

					case SUBTITLE_NONE:
						printf("Subtitle:\tNONE\n");
						break;

					default:
						printf("Subtitle:\tUnknown (%d)\n", rect->type);
						break;
				}
			}


			if (hasAssa)
			{
				// * \param now video timestamp in milliseconds
				long long now = (long long)((timeStamp + duration / 2) * 1000);
				
				ASS_Image *img = ass_render_frame(ass_renderer, ass_track, now, NULL);

				while (img)
				{
					printf("ass_render_frame OK\n");

					//typedef struct ass_image {
					//	int w, h;                   // Bitmap width/height
					//	int stride;                 // Bitmap stride
					//	unsigned char *bitmap;      // 1bpp stride*h alpha buffer
					//								// Note: the last row may not be padded to
					//								// bitmap stride!
					//	uint32_t color;             // Bitmap color and alpha, RGBA
					//	int dst_x, dst_y;           // Bitmap placement inside the video frame

					//	struct ass_image *next;   // Next image, or NULL

					//	enum {
					//		IMAGE_TYPE_CHARACTER,
					//		IMAGE_TYPE_OUTLINE,
					//		IMAGE_TYPE_SHADOW
					//	} type;

					//} ASS_Image;

					printf("ASS_Image: w=%d, h=%d, stride=%d, bitmap=%p, color=0x%08x, dst_x=%d, dst_y=%d, next=%p, type=%d\n",
						img->w, img->h, img->stride, img->bitmap, img->color, img->dst_x, img->dst_y, img->next, img->type);


					AllocatedImageSPTR image = std::make_shared<AllocatedImage>(ImageFormatEnum::R8G8B8A8,
						img->w, img->h);
					unsigned int* imageData = (unsigned int*)image->Data();

					unsigned char* pixData;
					for (int y = 0; y < img->h; ++y)
					{
						pixData = img->bitmap + (y * img->stride);

						for (int x = 0; x < img->w; ++x)
						{
							unsigned char lum = *(pixData++);
							

							unsigned char red = (img->color >> 24) * lum / 0xff;
							unsigned char green = (img->color >> 16) * lum / 0xff;
							unsigned char blue = (img->color >> 8) * lum / 0xff;
							unsigned char alpha = 0xff - (img->color & 0xff);
							alpha = alpha * lum / 0xff;

							//unsigned int color = (red << 24) | (green << 16) | (blue << 8) | alpha;
							unsigned int color = (alpha << 24) | (blue << 16) | (green << 8) | (red << 0);
							//color |= 0xff0000ff;

							//printf("0x%08x ", color);

#if 0
							//DEBUG
							if (color == 0)
							{
								//color = 0xffff00ff;
								//color = 0xffffff00;	//ARGB
								//color = 0x00ffffff;		//BGRA
								color = 0xff00ffff;		// ABGR
							}
#endif

							imageData[y * img->w + x] = color;
						}
					}


					ImageBufferSPTR imageBuffer = std::make_shared<ImageBuffer>(
						shared_from_this(),
						image);
					imageBuffer->SetTimeStamp(timeStamp);
					imageBuffer->SetDuration(duration);
					imageBuffer->SetX(img->dst_x);
					imageBuffer->SetY(img->dst_y);

					//outPin->SendBuffer(imageBuffer);
					imageList->push_back(imageBuffer);

					//printf("ASS_Image: Added Image timeStamp=%f/%f duration=%f/%f\n",
					//	timeStamp, imageBuffer->TimeStamp(),
					//	duration, imageBuffer->Duration());

					img = img->next;
				}
			}


			//if (imageList->size() > 0)
			{
				ImageListBufferSPTR imageListBuffer = std::make_shared<ImageListBuffer>(
					shared_from_this(),
					imageList);
				imageListBuffer->SetTimeStamp(timeStamp);

				outPin->SendBuffer(imageListBuffer);
			}

			avsubtitle_free(avSubtitle);
		}
	}
}



SubtitleDecoderElement::SubtitleDecoderElement() 
{
	ass_library = ass_library_init();
	if (!ass_library)
	{
		throw Exception("ass_library_init failed!\n");
	}


	ass_set_message_cb(ass_library, SubtitleDecoderElement::static_msg_callback, (void*)this);


	ass_renderer = ass_renderer_init(ass_library);
	if (!ass_renderer) 
	{
		throw Exception("ass_renderer_init failed!\n");
	}

	ass_set_use_margins(ass_renderer, false);
	ass_set_hinting(ass_renderer, ASS_HINTING_LIGHT);
	ass_set_font_scale(ass_renderer, 1.0);
	ass_set_line_spacing(ass_renderer, 0.0);

	ass_set_frame_size(ass_renderer, 1920, 1080);
	
	ass_set_fonts(ass_renderer, NULL, "sans-serif",
		ASS_FONTPROVIDER_AUTODETECT, NULL, 1);
	//ass_set_fonts(ass_renderer,
	//	"/usr/share/kodi/media/Fonts/arial.ttf", "Arial", 
	//	ASS_FONTPROVIDER_AUTODETECT, NULL, 1);

	ass_track = ass_new_track(ass_library);
	if (!ass_track) 
	{
		throw Exception("ass_new_track failed!\n");
	}

	//ass_set_check_readorder(ass_track, 0);
}

SubtitleDecoderElement::~SubtitleDecoderElement()
{

}



void SubtitleDecoderElement::Initialize()
{
	ClearOutputPins();
	ClearInputPins();


	// TODO: Pin format negotiation


	// Create an input pin
	inInfo = std::make_shared<SubtitlePinInfo>();
	inInfo->Format = SubtitleFormatEnum::Unknown;


	ElementWPTR weakPtr = shared_from_this();
	
	inPin = std::make_shared<SubtitleInPin>(weakPtr, inInfo);
	AddInputPin(inPin);


	// Create an audio out pin
	// TODO: Figure out what the output is
	outInfo = std::make_shared<PinInfo>(MediaCategoryEnum::Unknown);

	//ElementWPTR weakPtr = shared_from_this();
	outPin = std::make_shared<OutPin>(weakPtr, outInfo);
	AddOutputPin(outPin);



	//// Create buffer(s)
	//frame = std::make_shared<AVFrameBuffer>(shared_from_this());
}

void SubtitleDecoderElement::DoWork()
{
	BufferSPTR buffer;
	BufferSPTR outBuffer;

	// Reap output buffers
	while (outPin->TryGetAvailableBuffer(&buffer))
	{
		// New buffers are created as needed so just
		// drop this buffer.
		Wake();
	}


	if (inPin->TryGetFilledBuffer(&buffer))
	{
		if (buffer->Type() != BufferTypeEnum::AVPacket)
		{
			// The input buffer is not usuable for processing
			switch (buffer->Type())
			{
				case BufferTypeEnum::Marker:
				{
					MarkerBufferSPTR markerBuffer = std::static_pointer_cast<MarkerBuffer>(buffer);

					switch (markerBuffer->Marker())
					{
						case MarkerEnum::EndOfStream:
							// Send all Output Pins an EOS buffer					
							for (int i = 0; i < Outputs()->Count(); ++i)
							{
								MarkerBufferSPTR eosBuffer = std::make_shared<MarkerBuffer>(shared_from_this(), MarkerEnum::EndOfStream);
								Outputs()->Item(i)->SendBuffer(eosBuffer);
							}

							//SetExecutionState(ExecutionStateEnum::Idle);
							SetState(MediaState::Pause);
							break;

						default:
							// ignore unknown 
							break;
					}
					break;
				}

				default:
					// Ignore
					break;
			}

			inPin->PushProcessedBuffer(buffer);
			inPin->ReturnProcessedBuffers();
		}
		else
		{
			if (isFirstData)
			{
				OutPinSPTR otherPin = inPin->Source();
				if (otherPin)
				{
					if (otherPin->Info()->Category() != MediaCategoryEnum::Subtitle)
					{
						throw InvalidOperationException("Not connected to a subtitle pin.");
					}

					SubtitlePinInfoSPTR info = std::static_pointer_cast<SubtitlePinInfo>(otherPin->Info());
					inInfo->Format = info->Format;

					// TODO: Copy in pin info

					// TODO: Set output info
					//outInfo->SampleRate = info->SampleRate;
					

					SetupCodec();

					isFirstData = false;
				}
			}

			AVPacketBufferSPTR avPacketBuffer = std::static_pointer_cast<AVPacketBuffer>(buffer);
			ProcessBuffer(avPacketBuffer);

			inPin->PushProcessedBuffer(buffer);
			inPin->ReturnProcessedBuffers();
		}
	}
}

void SubtitleDecoderElement::ChangeState(MediaState oldState, MediaState newState)
{
	Element::ChangeState(oldState, newState);
}

void SubtitleDecoderElement::Flush()
{
	Element::Flush();

	if (avcodecContext)
	{
		avcodec_flush_buffers(avcodecContext);
	}
}



//-------------------------



void SubtitleRenderElement::timer_Expired(void* sender, const EventArgs& args)
{
	if (State() == MediaState::Play)
	{
		entriesMutex.Lock();
		
		//printf("SubtitleRenderElement::timer_Expired currentTime=%f\n", currentTime);

		// Remove stale entries
		{
			std::vector<SpriteEntry> expiredEntries;
			for (SpriteEntry& entry : spriteEntries)
			{
				if (currentTime >= entry.StopTime)
				{
					expiredEntries.push_back(entry);
				}
			}


			bool clearAllActiveFlag = false;
			
			for (SpriteEntry& entry : expiredEntries)
			{
				if (!entry.Sprite)
				{
					clearAllActiveFlag = true;
					break;
				}
			}

			if (clearAllActiveFlag)
			{
				for (SpriteEntry& entry : spriteEntries)
				{
					if (entry.IsActive)
					{
						// Check if entry is already on the
						// exired list.
						bool found = false;
						for (SpriteEntry& existing : spriteEntries)
						{
							if (existing.Sprite == entry.Sprite)
							{
								found = true;
								break;
							}
						}

						if (!found)
							expiredEntries.push_back(entry);
					}
				}
			}


			SpriteList removals;
			for (SpriteEntry& entry : expiredEntries)
			{
				if (entry.IsActive && entry.Sprite)
				{
					removals.push_back(entry.Sprite);
					printf("SubtitleRenderElement::timer_Expired - [%f] removal (Sprite=%p)\n",
						currentTime, entry.Sprite.get());
				}

				/*auto iter = std::find(std::begin(spriteEntries), std::end(spriteEntries), entry);
				if (iter == spriteEntries.end())
					throw InvalidOperationException();*/
				for (auto iter = spriteEntries.begin(); iter != spriteEntries.end(); ++iter)
				{
					if (*iter == entry)
					{
						spriteEntries.erase(iter);
						break;
					}
				}				
			}

			if (removals.size() > 0)
				compositor->RemoveSprites(removals);
		}

		// Add new entries
		{
			SpriteList additions;
			for (SpriteEntry& entry : spriteEntries)
			{
				if (currentTime >= entry.StartTime &&
					!entry.IsActive)
				{
					entry.IsActive = true;

					if (entry.Sprite)
					{
						additions.push_back(entry.Sprite);
						printf("SubtitleRenderElement::timer_Expired - [%f] addition (Sprite=%p)\n",
							currentTime, entry.Sprite.get());
					}

					//printf("SubtitleRenderElement: Displaying sprite. (StartTime=%f StopTime=%f\n",
					//	entry.StartTime, entry.StopTime);
				}
			}

			if (additions.size() > 0)
				compositor->AddSprites(additions);
		}

		currentTime += timer.Interval();

		entriesMutex.Unlock();
	}
}

void SubtitleRenderElement::SetupCodec()
{
}

void SubtitleRenderElement::ProcessBuffer(ImageListBufferSPTR buffer)
{
	entriesMutex.Lock();
	

	if (buffer->Payload()->size() < 1)
	{
		// Empty buffer is used to signal "clear all"
		SpriteEntry entry;
		entry.StartTime = buffer->TimeStamp();
		entry.StopTime = buffer->TimeStamp();
		//entry.Sprite = sprite;
		entry.IsActive = false;

		spriteEntries.push_back(entry);

		printf("SubtitleRenderElement::ProcessBuffer - Got empty ImageListBuffer. (TimeStamp = %f)\n", buffer->TimeStamp());
	}
	else
	{
		float z = -1;

		for (ImageBufferSPTR image : *(buffer->Payload()))
		{
			SourceSPTR source = std::make_shared<Source>(image->Payload());

			SpriteSPTR sprite = std::make_shared<Sprite>(source);
			sprite->SetDestinationRect(Rectangle(image->X(), image->Y(), image->Payload()->Width(), image->Payload()->Height()));
			sprite->SetColor(PackedColor(0xff, 0xff, 0xff, 0xff));
			sprite->SetZOrder(z);

			SpriteEntry entry;
			entry.StartTime = image->TimeStamp();
			if (image->Duration() > 0)
			{
				entry.StopTime = image->TimeStamp() + image->Duration();
			}
			else
			{
				// If a duration is not specified, use a default
				entry.StopTime = image->TimeStamp() + 2.0;	// seconds
			}

			entry.Sprite = sprite;
			entry.IsActive = false;

			spriteEntries.push_back(entry);

			z += 0.001;

			printf("SubtitleRenderElement::ProcessBuffer - Added (StartTime=%f StopTime=%f Sprite=%p)\n",
				entry.StartTime, entry.StopTime, entry.Sprite.get());
		}
	}

	entriesMutex.Unlock();
}


SubtitleRenderElement::SubtitleRenderElement(CompositorSPTR compositor)
	: compositor(compositor)
{
	if (!compositor)
		throw ArgumentNullException();

	timerExpiredListener = std::make_shared<EventListener<EventArgs>>(
		std::bind(&SubtitleRenderElement::timer_Expired, this, std::placeholders::_1, std::placeholders::_2));

	timer.Expired.AddListener(timerExpiredListener);
	timer.SetInterval(1.0 / 60.0);	// 60fps
	timer.Start();
}
SubtitleRenderElement::~SubtitleRenderElement()
{
}



void SubtitleRenderElement::Initialize()
{
	ClearOutputPins();
	ClearInputPins();


	// TODO: Pin format negotiation


	// Create an input pin
	inInfo = std::make_shared<PinInfo>(MediaCategoryEnum::Picture);
	//inInfo->Format = ImageFormatEnum::Unknown;


	ElementWPTR weakPtr = shared_from_this();

	inPin = std::make_shared<InPin>(weakPtr, inInfo);
	AddInputPin(inPin);

}

void SubtitleRenderElement::DoWork()
{
	BufferSPTR buffer;

	if (inPin->TryGetFilledBuffer(&buffer))
	{
		if (buffer->Type() != BufferTypeEnum::ImageList)
		{
			// The input buffer is not usuable for processing
			switch (buffer->Type())
			{
			case BufferTypeEnum::Marker:
			{
				MarkerBufferSPTR markerBuffer = std::static_pointer_cast<MarkerBuffer>(buffer);

				switch (markerBuffer->Marker())
				{
				case MarkerEnum::EndOfStream:
					// Send all Output Pins an EOS buffer					
					for (int i = 0; i < Outputs()->Count(); ++i)
					{
						MarkerBufferSPTR eosBuffer = std::make_shared<MarkerBuffer>(shared_from_this(), MarkerEnum::EndOfStream);
						Outputs()->Item(i)->SendBuffer(eosBuffer);
					}

					//SetExecutionState(ExecutionStateEnum::Idle);
					//SetState(MediaState::Pause);
					break;

				default:
					// ignore unknown 
					break;
				}
				break;
			}

			default:
				// Ignore
				break;
			}

			inPin->PushProcessedBuffer(buffer);
			inPin->ReturnProcessedBuffers();
		}
		else
		{
			if (isFirstData)
			{
				OutPinSPTR otherPin = inPin->Source();
				if (otherPin)
				{
					//if (otherPin->Info()->Category() != MediaCategoryEnum::Picture)
					//{
					//	throw InvalidOperationException("Not connected to a picture pin.");
					//}

					PinInfoSPTR info = otherPin->Info();
					//inInfo->Format = info->Format;

					// TODO: Copy in pin info

					SetupCodec();

					isFirstData = false;
				}
			}

			ImageListBufferSPTR imageBuffer = std::static_pointer_cast<ImageListBuffer>(buffer);
			ProcessBuffer(imageBuffer);

			inPin->PushProcessedBuffer(buffer);
			inPin->ReturnProcessedBuffers();
		}
	}
}

void SubtitleRenderElement::ChangeState(MediaState oldState, MediaState newState)
{
	Element::ChangeState(oldState, newState);
}

void SubtitleRenderElement::Flush()
{
	entriesMutex.Lock();

	Element::Flush();


	SpriteList removals;
	for (SpriteEntry& entry : spriteEntries)
	{
		if (entry.IsActive)
		{
			removals.push_back(entry.Sprite);
		}
	}

	compositor->RemoveSprites(removals);

	spriteEntries.clear();

	entriesMutex.Unlock();
}

void SubtitleRenderElement::SetTimeStamp(double value)
{
	entriesMutex.Lock();

	currentTime = value;

	entriesMutex.Unlock();
	//printf("SubtitleRenderElement::SetTimeStamp = %f\n", value);
}
