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

#include "AlsaAudioSink.h"



void AlsaAudioSinkElement::SetupAlsa(int frameSize)
{
	if (sampleRate == 0)
	{
		throw ArgumentException();
	}


	int err;
	if ((err = snd_pcm_open(&handle, device, SND_PCM_STREAM_PLAYBACK, 0)) < 0) //SND_PCM_NONBLOCK
	{
		printf("snd_pcm_open error: %s\n", snd_strerror(err));
		exit(EXIT_FAILURE);
	}

	printf("SetupAlsa: frameSize=%d\n", frameSize);

	FRAME_SIZE = frameSize;
	snd_pcm_hw_params_t *hw_params;
	snd_pcm_sw_params_t *sw_params;
	period_size = FRAME_SIZE * alsa_channels * sizeof(short);
	buffer_size = AUDIO_FRAME_BUFFERCOUNT * period_size;


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


	snd_pcm_sw_params_malloc(&sw_params);
	snd_pcm_sw_params_current(handle, sw_params);
	snd_pcm_sw_params_set_start_threshold(handle, sw_params, buffer_size - period_size);
	snd_pcm_sw_params_set_avail_min(handle, sw_params, period_size);
	snd_pcm_sw_params(handle, sw_params);
	snd_pcm_sw_params_free(sw_params);


	snd_pcm_prepare(handle);
}

void AlsaAudioSinkElement::ProcessBuffer(PcmDataBufferSPTR pcmBuffer)
{
	playPauseMutex.Lock();

	if (doResumeFlag)
	{
		snd_pcm_pause(handle, 0);
		doResumeFlag = false;
	}


	PcmData* pcmData = pcmBuffer->GetPcmData();

	if (isFirstBuffer)
	{
		SetupAlsa(pcmData->Samples);
		isFirstBuffer = false;
	}


	short data[alsa_channels * pcmData->Samples];

	if (pcmData->Format == PcmFormat::Int16)
	{
		if (alsa_channels != 2)
		{
			throw InvalidOperationException();
		}

		if (pcmData->Channels < 2)
			throw NotSupportedException();


		int stride = pcmData->Channels * sizeof(short);
		unsigned char* source = (unsigned char*)pcmData->Channel[0];

		int index = 0;
		for (int i = 0; i < pcmData->Samples; ++i)
		{
			short* samples = (short*)(source + i * stride);

			for (int j = 0; j < alsa_channels; ++j)
			{
				if (j < pcmData->Channels)
				{
					data[index++] = *(samples++);
				}
				else
				{
					data[index++] = 0;
				}
			}
		}
	}
	else if (pcmData->Format == PcmFormat::Int16Planes)
	{
		if (alsa_channels != 2)
		{
			throw InvalidOperationException();
		}


		short* channels[alsa_channels] = { 0 };

		int channelCount = pcmData->Channels;

		switch (channelCount)
		{
			case 0:
				throw InvalidOperationException("Unexpected zero channel count.");

			case 1:
				// Output the mono channel over both stereo channels
				channels[0] = (short*)pcmData->Channel[0];
				channels[1] = (short*)pcmData->Channel[0];
				break;

			default:
				channels[0] = (short*)pcmData->Channel[0];
				channels[1] = (short*)pcmData->Channel[1];
				break;
		}

		int index = 0;
		for (int i = 0; i < pcmData->Samples; ++i)
		{
			for (int j = 0; j < alsa_channels; ++j)
			{
				short* samples = channels[j];
				data[index++] = samples[i];
			}
		}
	}
	else if (pcmData->Format == PcmFormat::Float32Planes)
	{
		if (alsa_channels != 2)
		{
			throw InvalidOperationException();
		}

		if (pcmData->Channels < 2)
			throw NotSupportedException();


		float* channels[alsa_channels] = { 0 };

		channels[0] = (float*)pcmData->Channel[0];
		channels[1] = (float*)pcmData->Channel[1];
		channels[2] = (float*)pcmData->Channel[2];


		int index = 0;
		for (int i = 0; i < pcmData->Samples; ++i)
		{
			float* leftSamples = channels[0];
			float* rightSamples = channels[1];
			float* centerSamples = channels[2];

			float left;
			float right;
			if (pcmData->Channels > 2)
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
	else if (pcmData->Format == PcmFormat::Int32)
	{
		if (alsa_channels != 2)
		{
			throw InvalidOperationException();
		}

		if (pcmData->Channels < 2)
			throw NotSupportedException();


		// Signed 32 bit, interleaved
		int srcIndex = 0;
		int dstIndex = 0;

		int* source = (int*)pcmData->Channel[0];
		//short* dest = data;

		for (int i = 0; i < pcmData->Samples; ++i)
		{
			int left = source[srcIndex++];
			int right = source[srcIndex];

			srcIndex += (pcmData->Channels - 2);

			data[dstIndex++] = (short)(left >> 16);
			data[dstIndex++] = (short)(right >> 16);
		}
	}
	else
	{
		throw NotSupportedException();
	}


	// Update the reference clock
	/*
	From ALSA docs:
	For playback the delay is defined as the time that a frame that
	is written to the PCM stream shortly after this call will take
	to be actually audible. It is as such the overall latency from
	the write call to the final DAC.
	*/
	snd_pcm_sframes_t delay;
	if (snd_pcm_delay(handle, &delay) != 0)
	{
		printf("snd_pcm_delay failed.\n");
	}

	// The sample currently playing is previous in time to this frame,
	// so adjust negatively.
	double adjust = -(delay / (double)sampleRate);

	//printf("ALSA: adjust=%f\n", adjust);

	if (pcmBuffer->TimeStamp() > 0)
	{
		double time = pcmBuffer->TimeStamp() + adjust + audioAdjustSeconds;
		clock = time;

		BufferSPTR clockPinBuffer;
		if (clockOutPin->TryGetAvailableBuffer(&clockPinBuffer))
		{
			ClockDataBufferSPTR clockDataBuffer = std::static_pointer_cast<ClockDataBuffer>(clockPinBuffer);
			clockDataBuffer->SetTimeStamp(time);

			clockOutPin->SendBuffer(clockDataBuffer);

			//printf("AmlAudioSinkElement: clock=%f\n", pcmBuffer->TimeStamp());
		}


		// New clock interface
		for (IClockSinkSPTR sink : clockSinks)
		{
			//printf("AmlAudioSinkElement: IClockSinkSPTR sink=%p\n", sink.get());

			if (sink)
			{
				sink->SetTimeStamp(time);
			}
		}
	}


	// Send data to ALSA
	int totalFramesWritten = 0;
	while (totalFramesWritten < pcmData->Samples)
	{
		/*
		From ALSA docs:
		If the blocking behaviour is selected and it is running, then
		routine waits until all	requested frames are played or put to
		the playback ring buffer. The returned number of frames can be
		less only if a signal or underrun occurred.
		*/

		void* ptr = data + (totalFramesWritten * alsa_channels * sizeof(short));

		snd_pcm_sframes_t frames = snd_pcm_writei(handle,
			ptr,
			pcmData->Samples - totalFramesWritten);

		if (frames < 0)
		{
			printf("snd_pcm_writei failed: %s\n", snd_strerror(frames));

			if (frames == -EPIPE)
			{
				snd_pcm_recover(handle, frames, 1);
				printf("snd_pcm_recover\n");
			}
		}
		else
		{
			totalFramesWritten += frames;
		}
	}

	if (doPauseFlag)
	{
		snd_pcm_pause(handle, 1);
		doPauseFlag = false;
	}

	playPauseMutex.Unlock();
}



double AlsaAudioSinkElement::AudioAdjustSeconds() const
{
	return audioAdjustSeconds;
}
void AlsaAudioSinkElement::SetAudioAdjustSeconds(double value)
{
	audioAdjustSeconds = value;
}

double AlsaAudioSinkElement::Clock() const
{
	return clock;
}


void AlsaAudioSinkElement::Flush()
{
	Element::Flush();

	if (handle)
	{
		snd_pcm_drop(handle);
		snd_pcm_prepare(handle);
	}
}


void AlsaAudioSinkElement::Initialize()
{
	ClearOutputPins();
	ClearInputPins();

	// TODO: Pin format negotiation

	{
		// Create an audio pin
		AudioPinInfoSPTR info = std::make_shared<AudioPinInfo>();
		info->Format = AudioFormatEnum::Pcm;
		info->Channels = 2;
		info->SampleRate = 0;


		ElementWPTR weakPtr = shared_from_this();
		audioPin = std::make_shared<InPin>(weakPtr, info);
		AddInputPin(audioPin);
	}

	{
		// Create the clock pin
		PinInfoSPTR clockInfo = std::make_shared<PinInfo>(MediaCategoryEnum::Clock);

		ElementWPTR weakPtr = shared_from_this();
		clockOutPin = std::make_shared<OutPin>(weakPtr, clockInfo);
		AddOutputPin(clockOutPin);


		// Create a buffer
		for (int i = 0; i < 1; ++i)
		{
			ClockDataBufferSPTR clockBuffer = std::make_shared<ClockDataBuffer>(shared_from_this());
			clockOutPin->AcceptProcessedBuffer(clockBuffer);
		}
	}
}

void AlsaAudioSinkElement::DoWork()
{
	BufferSPTR buffer;
	if (audioPin->TryGetFilledBuffer(&buffer))
	{
		if (isFirstData)
		{
			OutPinSPTR otherPin = audioPin->Source();
			if (otherPin)
			{
				if (otherPin->Info()->Category() != MediaCategoryEnum::Audio)
				{
					throw InvalidOperationException("AlsaAudioSinkElement: Not connected to an audio pin.");
				}

				AudioPinInfoSPTR info = std::static_pointer_cast<AudioPinInfo>(otherPin->Info());
				audioFormat = info->Format;
				sampleRate = info->SampleRate;
				streamChannels = info->Channels;

				//TODO: copy output pin info to input pin

				//SetupAlsa();

				isFirstData = false;

				printf("AlsaAudioSinkElement: isFirstData changed.\n");
			}

		}


		switch (buffer->Type())
		{
			case BufferTypeEnum::Marker:
			{
				MarkerBufferSPTR markerBuffer = std::static_pointer_cast<MarkerBuffer>(buffer);
				printf("AlsaAudioSink: got marker buffer Marker=%d\n", (int)markerBuffer->Marker());

				switch (markerBuffer->Marker())
				{
					case MarkerEnum::EndOfStream:
						//SetExecutionState(ExecutionStateEnum::Idle);
						SetState(MediaState::Pause);
						break;

					default:
						// ignore unknown 
						break;
				}

				break;
			}

			case BufferTypeEnum::PcmData:
			{
				PcmDataBufferSPTR pcmBuffer = std::static_pointer_cast<PcmDataBuffer>(buffer);

				ProcessBuffer(pcmBuffer);

				break;
			}

			default:
				throw NotSupportedException("Unexpected buffer type.");
		}


		audioPin->PushProcessedBuffer(buffer);
		audioPin->ReturnProcessedBuffers();
	}


}

void AlsaAudioSinkElement::Terminating()
{
	snd_pcm_drop(handle);
	snd_pcm_close(handle);

	handle = nullptr;
}

void AlsaAudioSinkElement::ChangeState(MediaState oldState, MediaState newState)
{
	Element::ChangeState(oldState, newState);

	if (handle)
	{
		switch (newState)
		{
			case MediaState::Play:
			{
				playPauseMutex.Lock();

				doPauseFlag = false;
				doResumeFlag = true;

				//int ret = snd_pcm_pause(handle, 0);

				playPauseMutex.Unlock();
				break;
			}

			case MediaState::Pause:
			{
				playPauseMutex.Lock();

				doPauseFlag = true;
				doResumeFlag = false;
				//int ret = snd_pcm_pause(handle, 1);

				playPauseMutex.Unlock();
				break;
			}

			default:
				break;
		}
	}
}
