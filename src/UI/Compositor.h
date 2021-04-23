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

#include <linux/fb.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>


//#include "Egl.h"
//#include "QuadBatch.h"
#include "Mutex.h"
#include "Thread.h"
#include "Image.h"
#include "Rectangle.h"

#include <vector>


struct PackedColor
{
	unsigned char R;
	unsigned char G;
	unsigned char B;
	unsigned char A;

	
	PackedColor()
		: R(0), G(0), B(0), A(0)
	{
	}

	PackedColor(unsigned char red,
		unsigned char green,
		unsigned char blue,
		unsigned char alpha)
		: R(red), G(green), B(blue), A(alpha)
	{
	}
};

class Source
{
	ImageSPTR image;
	bool isDirty = true;

protected:

public:
	ImageSPTR Image()
	{
		return image;
	}

	int Width() const
	{
		return image->Width();
	}

	int Height() const
	{
		return image->Height();
	}

	bool IsDirty() const
	{
		return isDirty;
	}
	


	Source(ImageSPTR image)
		: image(image)
	{
		if (!image)
			throw ArgumentNullException();
	}

	virtual ~Source() {}


	void MarkDirty()
	{
		isDirty = true;
	}

	// used to reset dirty flag
	void Composed()
	{
		isDirty = false;
	}

};
typedef std::shared_ptr<Source> SourceSPTR;



class Sprite
{
	SourceSPTR source;
	Rectangle destinationRect;
	float zOrder = 0;
	PackedColor color = PackedColor(0xff, 0xff, 0xff, 0xff);
	bool isDirty = true;


public:
	SourceSPTR Source() const
	{
		return source;
	}
	void SetSource(SourceSPTR value)
	{
		source = value;
		isDirty = true;
	}

	Rectangle DestinationRect() const
	{
		return destinationRect;
	}
	void SetDestinationRect(Rectangle value)
	{
		destinationRect = value;
		isDirty = true;
	}

	float ZOrder() const
	{
		return zOrder;
	}
	void SetZOrder(float value)
	{
		zOrder = value;
		isDirty = true;
	}

	PackedColor Color() const
	{
		return color;
	}
	void SetColor(PackedColor value)
	{
		color = value;
		isDirty = true;
	}

	bool IsDirty()
	{
		return isDirty || source->IsDirty();
	}


	Sprite(SourceSPTR source)
		: source(source)
	{
		if (!source)
			throw ArgumentNullException();


		destinationRect = Rectangle(0, 0,
			source->Image()->Width(), source->Image()->Height());
	}

	Sprite(SourceSPTR source, Rectangle desitinationRect)
		: source(source), destinationRect(desitinationRect)
	{
		if (!source)
			throw ArgumentNullException();

	}


	// used to reset dirty flag
	void Composed()
	{
		source->Composed();

		isDirty = false;
	}
};

typedef std::shared_ptr<Sprite> SpriteSPTR;



#ifndef FBIO_WAITFORVSYNC
#define FBIO_WAITFORVSYNC _IOW('F', 0x20, __u32)
#endif

typedef std::vector<SpriteSPTR> SpriteList;

class Compositor
{
	//RenderContextSPTR context;
	int width;
	int height;
	SpriteList sprites;
	//QuadBatchSPTR quadBatch;

	int fd = -1;
	int ge2d_fd = -1;
	int ion_fd = -1;
	Mutex mutex;
	Thread renderThread = Thread(std::function<void()>(std::bind(&Compositor::RenderThread, this)));
	bool isRunning = false;
	bool isDirty = true;



	void ClearDisplay();
	void SwapBuffers();
	void WaitForVSync();
	void RenderThread();



public:
	// RenderContextSPTR Context() const
	// {
	// 	return context;
	// }
	
	int Width() const
	{
		return width;
	}
	
	int Height() const
	{
		return height;
	}



	Compositor(int width, int height);
	~Compositor();

			

	void AddSprites(const SpriteList& additions);
	void RemoveSprites(const SpriteList& removals);
	void Refresh();
};

typedef std::shared_ptr<Compositor> CompositorSPTR;
