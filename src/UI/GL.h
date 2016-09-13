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

	static std::string GetErrorString(int errorCode);


public:
	OpenGLException(int errorCode);
};



class GL
{
public:

	static void CheckError();
};



class RenderContext
{
	EGLDisplay eglDisplay;
	EGLSurface eglSurface;
	EGLContext glContext;

public:

	EGLDisplay EglDisplay() const;
	EGLSurface EglSurface() const;
	EGLContext GLContext() const;



	RenderContext(EGLDisplay eglDisplay, EGLSurface eglSurface, EGLContext glContext);
	virtual ~RenderContext() {}

};

typedef std::shared_ptr<RenderContext> RenderContextSPTR;
