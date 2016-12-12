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

#include "Exception.h"
#include "Image.h"



Image::Image(ImageFormatEnum format, int width, int height, int stride, void* data)
	: format(format), width(width), height(height), stride(stride), data(data)
{
	if (width < 1)
		throw ArgumentOutOfRangeException();

	if (height < 1)
		throw ArgumentOutOfRangeException();

	if (stride < 1)
		throw ArgumentOutOfRangeException();

	if (data == nullptr)
		throw ArgumentNullException();
}

Image::~Image()
{
}




int AllocatedImage::GetBytesPerPixel(ImageFormatEnum format)
{
	int bytesPerPixel;

	switch (format)
	{
		case ImageFormatEnum::R8G8B8:
			bytesPerPixel = 3;
			break;

		case ImageFormatEnum::R8G8B8A8:
			bytesPerPixel = 4;
			break;

		default:
			throw NotSupportedException();
	}

	return bytesPerPixel;
}

int AllocatedImage::CalculateStride(int width, ImageFormatEnum format)
{
	return width * GetBytesPerPixel(format);
}

void* AllocatedImage::Allocate(int width, int height, ImageFormatEnum format)
{
	int stride = CalculateStride(width, format);
	return malloc(stride * height);
}


AllocatedImage::AllocatedImage(ImageFormatEnum format, int width, int height)
	: Image(format, width, height, CalculateStride(width, format), Allocate(width, height, format))
{

}

AllocatedImage::~AllocatedImage()
{
	free(Data());
}
