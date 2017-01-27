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

#pragma once

#include "Codec.h"
#include "Buffer.h"


#include <memory>



enum class PinDirectionEnum
{
	Out = 0,
	In
};

enum class MediaCategoryEnum
{
	Unknown = 0,
	Audio,
	Video,
	Subtitle,
	Clock,
	Picture
};

enum class VideoFormatEnum
{
	Unknown = 0,
	Mpeg2,
	Mpeg4V3,
	Mpeg4,
	Avc,
	VC1,
	Hevc,
	Yuv420,
	NV12,
	NV21
};

enum class AudioFormatEnum
{
	Unknown = 0,
	Pcm,
	MpegLayer2,
	Mpeg2Layer3,
	Ac3,
	Aac,
	Dts,
	WmaPro,
	DolbyTrueHD,
	EAc3,
	Opus,
	Vorbis,
	PcmDvd,
	Flac
};

enum class SubtitleFormatEnum
{
	Unknown = 0,
	Text,
	SubRip,
	Pgs, //Presentation Graphic Stream
	Dvb,
	DvbTeletext,
	Dvd
};

enum class PictureFormatEnum
{
	Unknown = 0,
	Image
};


class PinInfo
{
	MediaCategoryEnum category;

public:

	MediaCategoryEnum Category() const
	{
		return category;
	}


	PinInfo(MediaCategoryEnum category)
		: category(category)
	{
	}
};
typedef std::shared_ptr<PinInfo> PinInfoSPTR;


typedef std::vector<unsigned char> ExtraData;
typedef std::shared_ptr<std::vector<unsigned char>> ExtraDataSPTR;
class VideoPinInfo : public PinInfo
{
public:
	VideoPinInfo()
		: PinInfo(MediaCategoryEnum::Video)
	{
	}

	VideoFormatEnum Format = VideoFormatEnum::Unknown;
	int Width = 0;
	int Height = 0;
	double FrameRate = 0;
	ExtraDataSPTR ExtraData;
	bool HasEstimatedPts = false;
};
typedef std::shared_ptr<VideoPinInfo> VideoPinInfoSPTR;


class AudioPinInfo : public PinInfo
{

public:
	AudioPinInfo()
		: PinInfo(MediaCategoryEnum::Audio)
	{
	}


	AudioFormatEnum Format = AudioFormatEnum::Unknown;
	int Channels = 0;
	int SampleRate = 0;
	ExtraDataSPTR ExtraData;
};
typedef std::shared_ptr<AudioPinInfo> AudioPinInfoSPTR;

class SubtitlePinInfo : public PinInfo
{
public:

	SubtitlePinInfo()
		: PinInfo(MediaCategoryEnum::Subtitle)
	{
	}


	SubtitleFormatEnum Format = SubtitleFormatEnum::Unknown;
};
typedef std::shared_ptr<SubtitlePinInfo> SubtitlePinInfoSPTR;


class PicturePinInfo : public PinInfo
{
public:

	PicturePinInfo()
		: PinInfo(MediaCategoryEnum::Picture)
	{
	}


	PictureFormatEnum Format = PictureFormatEnum::Unknown;
};
typedef std::shared_ptr<PicturePinInfo> PicturePinInfoSPTR;



// TODO: Move stream processing thread to Pin

class Pin : public std::enable_shared_from_this<Pin>
{
	PinDirectionEnum direction;
	ElementWPTR owner;
	PinInfoSPTR info;
	std::string name;


protected:
	Pin(PinDirectionEnum direction, ElementWPTR owner, PinInfoSPTR info)
		: Pin(direction, owner, info, "Pin")
	{
	}
	Pin(PinDirectionEnum direction, ElementWPTR owner, PinInfoSPTR info, std::string name)
		: direction(direction), owner(owner), info(info), name(name)
	{
		if (owner.expired())
			throw ArgumentNullException();

		if (!info)
			throw ArgumentNullException();
	}


public:

	PinDirectionEnum Direction() const
	{
		return direction;
	}

	ElementWPTR Owner() const
	{
		return owner;
	}

	PinInfoSPTR Info() const
	{
		return info;
	}

	std::string Name() const
	{
		return name;
	}
	void SetName(std::string value)
	{
		name = value;
	}



	virtual ~Pin()
	{
	}


	virtual void Flush()
	{
	}
};


template <typename T>	// where T : PinSPTR
class PinCollection
{
	friend class Element;

	std::vector<T> pins;


protected:

	void Add(T value)
	{
		if (!value)
			throw ArgumentNullException();

		pins.push_back(value);
	}

	void Clear()
	{
		pins.clear();
	}


public:

	PinCollection()
	{
	}


	int Count() const
	{
		return pins.size();
	}

	T Item(int index)
	{
		if (index < 0 || index >= (int)pins.size())
			throw ArgumentOutOfRangeException();

		return pins[index];
	}

	void Flush()
	{
		for (auto pin : pins)
		{
			pin->Flush();
		}
	}

	T FindFirst(MediaCategoryEnum category)
	{
		for (auto item : pins)
		{
			if (item->Info()->Category() == category)
			{
				return item;
			}
		}

		return nullptr;
	}

	T Find(MediaCategoryEnum category, int index)
	{
		// TODO: Disable this check for now until
		// a method to ignore a stream is added.
		// Until then, index=-1 is used.

		//if (index < 0)
		//	throw ArgumentOutOfRangeException();

		int count = 0;
		for (auto item : pins)
		{
			if (item->Info()->Category() == category)
			{
				if (count == index)
				{
					return item;
				}
				else
				{
					++count;
				}
			}
		}

		return nullptr;
	}
};
