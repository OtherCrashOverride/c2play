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

#include "GL.h"

#include <memory>


enum class ShaderTypeEnum
{
	Vertex = GL_VERTEX_SHADER,
	Fragment = GL_FRAGMENT_SHADER,
};



class Shader;
typedef std::shared_ptr<Shader> ShaderSPTR;

class Shader
{
	ShaderTypeEnum shaderType;
	GLuint id;


	void PrintInfoLog()
	{
		int param;
		glGetShaderiv(id, GL_INFO_LOG_LENGTH, &param);
		GL::CheckError();

		int logLength = param;
		GLchar log[logLength + 1];

		glGetShaderInfoLog(id, logLength, &param, log);
		GL::CheckError();

		printf("Shader compile Log: %s\n", log);
	}

protected:




public:

	ShaderTypeEnum ShaderType() const
	{
		return shaderType;
	}

	GLuint Id() const
	{
		return id;
	}



	Shader(ShaderTypeEnum shaderType)
		: shaderType(shaderType)
	{
		id = glCreateShader((GLenum)shaderType);
		GL::CheckError();
	}



	static ShaderSPTR CreateFromSource(ShaderTypeEnum shaderType, const char* source)
	{
		ShaderSPTR shader = std::make_shared<Shader>(shaderType);

		const char* glSrcCode[] = { source };
		const int lengths[]{ -1 }; // Tell OpenGL the string is NULL terminated

		glShaderSource(shader->Id(), 1, glSrcCode, lengths);
		GL::CheckError();

		glCompileShader(shader->Id());
		GL::CheckError();


		GLint param;

		glGetShaderiv(shader->Id(), GL_COMPILE_STATUS, &param);
		GL::CheckError();

		if (param == GL_FALSE)
		{
			shader->PrintInfoLog();
			throw Exception("Shader Compilation Failed.");
		}

		return shader;
	}
};



class GlslProgram
{
	ShaderSPTR vertexShader;
	ShaderSPTR fragmentShader;
	GLuint id;
	//bool isLinked = false;


public:

	ShaderSPTR VertexShader() const
	{
		return vertexShader;
	}

	ShaderSPTR FragmentShader() const
	{
		return fragmentShader;
	}

	GLuint Id() const
	{
		return id;
	}



	GlslProgram(ShaderSPTR vertexShader, ShaderSPTR fragmentShader)
		: vertexShader(vertexShader), fragmentShader(fragmentShader)
	{
		if (!vertexShader)
			throw ArgumentNullException();

		if (!fragmentShader)
			throw ArgumentNullException();


		id = glCreateProgram();
		GL::CheckError();

		glAttachShader(id, vertexShader->Id());
		GL::CheckError();

		glAttachShader(id, fragmentShader->Id());
		GL::CheckError();

		glLinkProgram(id);
		GL::CheckError();
	}


	//virtual void OnLink()
	//{
	//	glLinkProgram(id);
	//	GL::CheckError();

	//	isLinked = true;
	//}

	virtual void Apply()
	{
		//if (!isLinked)
		//{
		//	OnLink();
		//}

		glUseProgram(id);
		GL::CheckError();
	}

};
typedef std::shared_ptr<GlslProgram> GlslProgramSPTR;
