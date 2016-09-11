#pragma once

#include <memory>

#include "Exception.h"


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



	Image(ImageFormatEnum format, int width, int height, int stride, void* data)
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

	virtual ~Image()
	{
	}
};

typedef std::shared_ptr<Image> ImageSPTR;


class AllocatedImage : public Image
{

	static int GetBytesPerPixel(ImageFormatEnum format)
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

	static int CalculateStride(int width, ImageFormatEnum format)
	{
		return width * GetBytesPerPixel(format);
	}

	static void* Allocate(int width, int height, ImageFormatEnum format)
	{
		int stride = CalculateStride(width, format);
		return malloc(stride * height);
	}

public:
	AllocatedImage(ImageFormatEnum format, int width, int height)
		: Image(format, width, height, CalculateStride(width, format), Allocate(width, height, format))
	{

	}

	virtual ~AllocatedImage()
	{
		free(Data());
	}
};

typedef std::shared_ptr<AllocatedImage> AllocatedImageSPTR;
