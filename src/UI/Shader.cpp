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

#include <cstdio>

#include "Shader.h"



void Shader::PrintInfoLog()
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



Shader::Shader(ShaderTypeEnum shaderType)
	: shaderType(shaderType)
{
	id = glCreateShader((GLenum)shaderType);
	GL::CheckError();
}



ShaderSPTR Shader::CreateFromSource(ShaderTypeEnum shaderType, const char* source)
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





GlslProgram::GlslProgram(ShaderSPTR vertexShader, ShaderSPTR fragmentShader)
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



void GlslProgram::Apply()
{
	glUseProgram(id);
	GL::CheckError();
}
