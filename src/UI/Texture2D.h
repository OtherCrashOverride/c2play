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

#include "GL.h"


class Texture2D
{
	int width;
	int height;
	GLuint id;


public:

	int Width() const
	{
		return width;
	}

	int Height() const
	{
		return height;
	}

	GLuint Id() const
	{
		return id;
	}



	Texture2D(int width, int height);
	~Texture2D();



	void Bind();
	void Unbind();
	void WriteData(void* data);
	void GenerateMipMaps();
};

typedef std::shared_ptr<Texture2D> Texture2DSPTR;
