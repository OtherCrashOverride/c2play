#pragma once

#include "Codec.h"

#include <alsa/asoundlib.h>


class AlsaAudioSink : public Sink, public virtual IClockSource
{

	const double AUDIO_ADJUST_SECONDS = (-0.30);
	const char* device = "default"; //default   //plughw                     /* playback device */
	const int alsa_channels = 2;

	AVCodecID codec_id = AV_CODEC_ID_NONE;
	unsigned int sampleRate = 0;
	snd_pcm_t* handle = nullptr;
	snd_pcm_sframes_t frames;
	//pthread_mutex_t audioMutex = PTHREAD_MUTEX_INITIALIZER;
	//std::queue<PacketBufferPtr> audioBuffers;
	//pthread_t audioThread;
	//bool isAudioThreadStarted = false;
	//bool isRunning = false;
	//MediaState state = MediaState::Pause;
	IClockSinkPtr clockSink;

public:

	IClockSinkPtr ClockSink() const
	{
		return clockSink;
	}
	void SetClockSink(IClockSinkPtr value)
	{
		clockSink = value;
	}


	AlsaAudioSink(AVCodecID codec_id, unsigned int sampleRate)
		: Sink(),
		codec_id(codec_id),
		sampleRate(sampleRate)
	{
		// TODO: Validate parameters

		int err;
		if ((err = snd_pcm_open(&handle, device, SND_PCM_STREAM_PLAYBACK, 0)) < 0) //SND_PCM_NONBLOCK
		{
			printf("snd_pcm_open error: %s\n", snd_strerror(err));
			exit(EXIT_FAILURE);
		}


		const int FRAME_SIZE = 1536; // 1536;
		snd_pcm_hw_params_t *hw_params;
		snd_pcm_sw_params_t *sw_params;
		snd_pcm_uframes_t period_size = FRAME_SIZE * alsa_channels * 2;
		snd_pcm_uframes_t buffer_size = 12 * period_size;

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
	}


	// IClockSource
	virtual double GetTimeStamp() override
	{
		throw NotImplementedException();

	}


//	// Members
//	virtual void AddBuffer(PacketBufferPtr buffer) override
//	{
//		const int MAX_AUDIO_BUFFERS = 128;
//
//		if (!isAudioThreadStarted)
//			throw InvalidOperationException();
//
//
//		size_t count;
//
//		while (isRunning)
//		{
//			pthread_mutex_lock(&audioMutex);
//			count = audioBuffers.size();
//
//			if (count >= MAX_AUDIO_BUFFERS)
//			{
//				pthread_mutex_unlock(&audioMutex);
//
//				usleep(1);
//			}
//			else
//			{
//				audioBuffers.push(buffer);
//
//				pthread_mutex_unlock(&audioMutex);
//				break;
//			}
//		}
//	}
//
//	virtual void Start() override
//	{
//		if (isAudioThreadStarted)
//			throw InvalidOperationException();
//
//		// ----- start decoder -----
//		isRunning = true;
//
//		int result_code = pthread_create(&audioThread, NULL, ThreadTrampoline, (void*)this);
//		if (result_code != 0)
//		{
//			throw Exception("AudioDecoderThread pthread_create failed.\n");
//		}
//
//		isAudioThreadStarted = true;
//	}
//	virtual void Stop() override
//	{
//		if (!isAudioThreadStarted)
//			throw InvalidOperationException();
//
//		Flush();
//
//		isRunning = false;
//
//		pthread_join(audioThread, NULL);
//
//		isAudioThreadStarted = false;
//	}
//
//	virtual void SetState(MediaState state) override
//	{
//		if (!isRunning)
//			throw InvalidOperationException();
//
//		this->state = state;
//	}
//
//	virtual void Flush() override
//	{
//		pthread_mutex_lock(&audioMutex);
//
//		while (audioBuffers.size() > 0)
//		{
//			audioBuffers.pop();
//		}
//
//		pthread_mutex_unlock(&audioMutex);
//	}
//
//
//private:
//
//	static void* ThreadTrampoline(void* argument)
//	{
//		AlsaAudioSink* ptr = (AlsaAudioSink*)argument;
//		ptr->AudioDecoderThread();
//	}

	protected:

	virtual void WorkThread() override
	{
		AVCodec* soundCodec = avcodec_find_decoder(codec_id);
		if (!soundCodec) {
			throw Exception("codec not found\n");
		}

		AVCodecContext* soundCodecContext = avcodec_alloc_context3(soundCodec);
		if (!soundCodecContext) {
			throw Exception("avcodec_alloc_context3 failed.\n");
		}

		//if (audio_channels == 0)
		//	audio_channels = 2;

		//if (audio_sample_rate == 0)
		//	audio_sample_rate = 48000;

		soundCodecContext->channels = alsa_channels;
		soundCodecContext->sample_rate = sampleRate;
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



		printf("AlsaAudioSink entering running state.\n");

		int totalAudioFrames = 0;
		double pcrElapsedTime = 0;
		double frameTimeStamp = -1.0;


		while (IsRunning())
		{
			if (State() != MediaState::Play)
			{
				usleep(1);
			}
			else
			{

				//pthread_mutex_lock(&audioMutex);
				//size_t count = audioBuffers.size();

				//if (count < 1)
				//{
				//	pthread_mutex_unlock(&audioMutex);
				//	usleep(1);
				//}
				//else
				//{
				//	PacketBufferPtr buffer = audioBuffers.front();
				//	audioBuffers.pop();

				//	pthread_mutex_unlock(&audioMutex);

				PacketBufferPtr buffer;

				if (!TryGetBuffer(&buffer))
				{
					usleep(1);
				}
				else
				{
					//printf("AlsaAudioSink got buffer.\n");

					if (frameTimeStamp < 0)
					{
						frameTimeStamp = buffer->GetTimeStamp();
					}


					// ---

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
						int centerChannelIndex = av_get_channel_layout_channel_index(decoded_frame->channel_layout, AV_CH_FRONT_CENTER);


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
							if (alsa_channels != 2)
							{
								throw InvalidOperationException();
							}


							float* channels[alsa_channels] = { 0 };
							channels[0] = (float*)decoded_frame->data[leftChannelIndex];
							channels[1] = (float*)decoded_frame->data[rightChannelIndex];
							channels[2] = (float*)decoded_frame->data[centerChannelIndex];

							int index = 0;
							for (int i = 0; i < decoded_frame->nb_samples; ++i)
							{
								float* leftSamples = channels[0];
								float* rightSamples = channels[1];
								float* centerSamples = channels[2];

								float left;
								float right;
								if (decoded_frame->channels > 2)
								{
									// Downmix
									const float CENTER_WEIGHT = 0.1666666666666667f;

									left = (leftSamples[i] * (1.0f - CENTER_WEIGHT)) + (centerSamples[i] * CENTER_WEIGHT);
									right = (rightSamples[i] * (1.0f - CENTER_WEIGHT)) + (centerSamples[i] * CENTER_WEIGHT);
								}
								else
								{
									left = leftSamples[i];
									right = rightSamples[i];
								}


								if (left > 1.0f)
									left = 1.0f;
								else if (left < -1.0f)
									left = -1.0f;

								if (right > 1.0f)
									right = 1.0f;
								else if (right < -1.0f)
									right = -1.0f;


								data[index++] = (short)(left * 0x7fff);
								data[index++] = (short)(right * 0x7fff);
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


							// TODO: Clock setting
#if 1
							if (clockSink.get() != nullptr)
							{
								//unsigned long pts = (unsigned long)((frameTimeStamp + AUDIO_ADJUST_SECONDS) * PTS_FREQ);

								//int codecCall = codec_set_pcrscr(&codecContext, (int)pts);
								//if (codecCall != 0)
								//{
								//	printf("codec_set_pcrscr failed.\n");
								//}

								clockSink->SetTimeStamp(frameTimeStamp + AUDIO_ADJUST_SECONDS);
							}
#endif
							frameTimeStamp = -1;

						}



					}
				}
			}
		}

		printf("AlsaAudioSink exiting running state.\n");
	}

};
