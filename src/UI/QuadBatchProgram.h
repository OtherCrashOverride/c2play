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



	QuadBatchProgram()
		: GlslProgram(
			Shader::CreateFromSource(ShaderTypeEnum::Vertex, vertexSource),
			Shader::CreateFromSource(ShaderTypeEnum::Fragment, fragmentSource))
	{
		//glBindAttribLocation(Id(), 0, "Attr_Position");
		//GL::CheckError();

		//glBindAttribLocation(Id(), 1, "Attr_Color0");
		//GL::CheckError();

		//glBindAttribLocation(Id(), 2, "Attr_TexCoord0");
		//GL::CheckError();


		//GlslProgram::OnLink();


		worldViewProjectionParameter = glGetUniformLocation(Id(), "WorldViewProjection");
		GL::CheckError();

		if (worldViewProjectionParameter == -1)
			throw InvalidOperationException();


		diffuseMapParameter = glGetUniformLocation(Id(), "DiffuseMap");
		GL::CheckError();

		if (diffuseMapParameter == -1)
			throw InvalidOperationException();


		positionAttribute = glGetAttribLocation(Id(), "Attr_Position");
		GL::CheckError();

		colorAttribute = glGetAttribLocation(Id(), "Attr_Color0");
		GL::CheckError();

		textCoordAttribute = glGetAttribLocation(Id(), "Attr_TexCoord0");
		GL::CheckError();
	}


	//virtual void OnLink() override
	//{



	//}

	virtual void Apply() override
	{
		GlslProgram::Apply();


		// WorldViewProjection
		Matrix4 param = Matrix4::CreateTranspose(worldViewProjection);
		glUniformMatrix4fv(worldViewProjectionParameter,
			1,
			GL_FALSE,
			(GLfloat*)&param);
		GL::CheckError();


		// Diffuse Map
		if (diffuseMapParameter > -1)
		{
			//printf("diffuseMapParameter=%d\n", diffuseMapParameter);

			glActiveTexture(GL_TEXTURE0);
			GL::CheckError();

			glUniform1i(diffuseMapParameter, 0);
			GL::CheckError();

			if (texture)
			{
				texture->Bind();
			}
			else
			{
				glBindTexture(GL_TEXTURE_2D, 0);
				GL::CheckError();
			}
		}


		// Vertex Attributes
		glEnableVertexAttribArray(0);
		GL::CheckError();
		
		glEnableVertexAttribArray(1);
		GL::CheckError();
		
		glEnableVertexAttribArray(2);
		GL::CheckError();

	}
};

typedef std::shared_ptr<QuadBatchProgram> QuadBatchProgramSPTR;
