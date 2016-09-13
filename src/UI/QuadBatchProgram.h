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

#include "Shader.h"
#include "Matrix4.h"
#include "Texture2D.h"



class QuadBatchProgram : public GlslProgram
{
	GLint worldViewProjectionParameter;
	GLint diffuseMapParameter;
	GLint positionAttribute;
	GLint colorAttribute;
	GLint textCoordAttribute;
	Texture2DSPTR texture;
	Matrix4 worldViewProjection = Matrix4::Identity;


	static const char* vertexSource;

	static const char* fragmentSource;



public:

	Matrix4 WorldViewProjection() const
	{
		return worldViewProjection;
	}
	void SetWorldViewProjection(Matrix4 value)
	{
		worldViewProjection = value;
	}

	Texture2DSPTR DiffuseMap() const
	{
		return texture;
	}
	void SetDiffuseMap(Texture2DSPTR value)
	{
		texture = value;
	}

	GLint PositionAttribute() const
	{
		return positionAttribute;
	}

	GLint ColorAttribute() const
	{
		return colorAttribute;
	}

	GLint TexCoordAttribute() const
	{
		return textCoordAttribute;
	}



	QuadBatchProgram();
	virtual ~QuadBatchProgram() {}



	virtual void Apply() override;
};

typedef std::shared_ptr<QuadBatchProgram> QuadBatchProgramSPTR;
