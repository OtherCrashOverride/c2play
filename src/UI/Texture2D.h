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



	Texture2D(int width, int height)
		: width(width), height(height)
	{
		glGenTextures(1, &id);
		GL::CheckError();

		Bind();

		glTexImage2D(GL_TEXTURE_2D, 0,
			GL_RGBA, width, height, 0,
			GL_RGBA, GL_UNSIGNED_BYTE, 0);
		GL::CheckError();

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		GL::CheckError();

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		GL::CheckError();

		Unbind();
	}

	~Texture2D()
	{
		glDeleteTextures(1, &id);
		GL::CheckError();
	}


	void Bind()
	{
		glBindTexture(GL_TEXTURE_2D, id);
		GL::CheckError();
	}

	void Unbind()
	{
		glBindTexture(GL_TEXTURE_2D, 0);
		GL::CheckError();
	}

	void WriteData(void* data)
	{
		Bind();

		glTexSubImage2D(GL_TEXTURE_2D, 0,
			0, 0,
			width, height,
			GL_RGBA, GL_UNSIGNED_BYTE, data);
		GL::CheckError();

		Unbind();
	}

	void GenerateMipMaps()
	{
		// Create MipMap levels
		Bind();

		glHint(GL_GENERATE_MIPMAP_HINT, GL_NICEST);
		GL::CheckError();

		glGenerateMipmap(GL_TEXTURE_2D);
		GL::CheckError();

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		GL::CheckError();

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		GL::CheckError();

		Unbind();
	}
};
typedef std::shared_ptr<Texture2D> Texture2DSPTR;
