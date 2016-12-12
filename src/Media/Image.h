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

#include <memory>



enum class ImageFormatEnum
{
	Unknown = 0,
	R8G8B8,
	R8G8B8A8
};



class Image
{
	ImageFormatEnum format;
	int width;
	int height;
	int stride;
	void* data;



public:

	int Width() const
	{
		return width;
	}

	int Height() const
	{
		return height;
	}

	int Stride() const
	{
		return stride;
	}

	ImageFormatEnum Format() const
	{
		return format;
	}

	void* Data() const
	{
		return data;
	}



	Image(ImageFormatEnum format, int width, int height, int stride, void* data);
	virtual ~Image();
};

typedef std::shared_ptr<Image> ImageSPTR;



class AllocatedImage : public Image
{

	static int GetBytesPerPixel(ImageFormatEnum format);
	static int CalculateStride(int width, ImageFormatEnum format);
	static void* Allocate(int width, int height, ImageFormatEnum format);


public:
	AllocatedImage(ImageFormatEnum format, int width, int height);
	virtual ~AllocatedImage();
};

typedef std::shared_ptr<AllocatedImage> AllocatedImageSPTR;
