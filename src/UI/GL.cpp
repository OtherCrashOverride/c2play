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

#include <sstream>

#include "GL.h"



std::string OpenGLException::GetErrorString(int errorCode)
{
	std::string result;

	switch (errorCode)
	{
		case GL_INVALID_ENUM:
			result = "OpenGL Exception : GL_INVALID_ENUM";
			break;

		case GL_INVALID_VALUE:
			result = "OpenGL Exception : GL_INVALID_VALUE";
			break;

		case GL_INVALID_OPERATION:
			result = "OpenGL Exception : GL_INVALID_OPERATION";
			break;

		case GL_OUT_OF_MEMORY:
			result = "OpenGL Exception : GL_OUT_OF_MEMORY";
			break;

		case GL_INVALID_FRAMEBUFFER_OPERATION:
			result = "OpenGL Exception : GL_INVALID_FRAMEBUFFER_OPERATION";
			break;

		default:
		{
			std::stringstream stream;
			stream << "OpenGL Exception : 0x" << std::hex << errorCode;
			result = stream.str();
		}
		break;
	}

	return result;
}


OpenGLException::OpenGLException(int errorCode)
	: Exception(GetErrorString(errorCode).c_str())
{
}





void GL::CheckError()
{
#ifdef DEBUG
	int error = glGetError();
	if (error != GL_NO_ERROR)
	{
		throw OpenGLException(error);
	}
#endif
}





EGLDisplay RenderContext::EglDisplay() const
{
	return eglDisplay;
}

EGLSurface RenderContext::EglSurface() const
{
	return eglSurface;
}

EGLContext RenderContext::GLContext() const
{
	return glContext;
}


RenderContext::RenderContext(EGLDisplay eglDisplay, EGLSurface eglSurface, EGLContext glContext)
	: eglDisplay(eglDisplay), eglSurface(eglSurface), glContext(glContext)
{
	if (eglDisplay == EGL_NO_DISPLAY)
		throw ArgumentException();

	if (eglSurface == EGL_NO_SURFACE)
		throw ArgumentException();

	if (glContext == EGL_NO_CONTEXT)
		throw ArgumentException();
}

