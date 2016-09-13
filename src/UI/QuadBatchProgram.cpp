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

#include "QuadBatchProgram.h"

const char* QuadBatchProgram::vertexSource = R"(
	attribute highp vec4 Attr_Position;
	attribute lowp vec4 Attr_Color0;
	attribute mediump vec2 Attr_TexCoord0;

	uniform mat4 WorldViewProjection;

	varying mediump vec2 TexCoord0;
	varying lowp vec4 Color0;

	void main()
	{
		gl_Position = Attr_Position *  WorldViewProjection;
		Color0 = Attr_Color0;
		TexCoord0 = Attr_TexCoord0;
	}
	)";

const char* QuadBatchProgram::fragmentSource = R"(
	uniform lowp sampler2D DiffuseMap;

	varying lowp vec4 Color0;
	varying mediump vec2 TexCoord0;

	void main()
	{
		lowp vec4 color = texture2D(DiffuseMap, TexCoord0);
		gl_FragColor =  Color0 * color;
		//gl_FragColor = vec4(1, 1, 1, 1);
	}
	)";


QuadBatchProgram::QuadBatchProgram()
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



void QuadBatchProgram::Apply()
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