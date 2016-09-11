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

#include <sstream>
#include <memory>

#include <GLES2/gl2.h>

#include "Exception.h"
#include "Egl.h"



class OpenGLException : public Exception
{

	static std::string GetErrorString(int errorCode)
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

public:
	OpenGLException(int errorCode)
		: Exception(GetErrorString(errorCode).c_str())
	{
	}
};



class GL
{
public:

	static void CheckError()
	{
#ifdef DEBUG
		int error = glGetError();
		if (error != GL_NO_ERROR)
		{
			throw OpenGLException(error);
		}
#endif
	}
};



class RenderContext
{
	EGLDisplay eglDisplay;
	EGLSurface eglSurface;
	EGLContext glContext;

public:

	EGLDisplay EglDisplay() const
	{
		return eglDisplay;
	}

	EGLSurface EglSurface() const
	{
		return eglSurface;
	}

	EGLContext GLContext() const
	{
		return glContext;
	}


	RenderContext(EGLDisplay eglDisplay, EGLSurface eglSurface, EGLContext glContext)
		: eglDisplay(eglDisplay), eglSurface(eglSurface), glContext(glContext)
	{
		if (eglDisplay == EGL_NO_DISPLAY)
			throw ArgumentException();

		if (eglSurface == EGL_NO_SURFACE)
			throw ArgumentException();

		if (glContext == EGL_NO_CONTEXT)
			throw ArgumentException();
	}

};

typedef std::shared_ptr<RenderContext> RenderContextSPTR;
