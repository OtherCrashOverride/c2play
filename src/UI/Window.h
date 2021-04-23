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

#include "Egl.h"


class WindowBase	// Prevent name collisions with X11
{

protected:

	WindowBase() {}


public:

	virtual int Width() = 0;
	virtual int Height() = 0;
	
	// virtual EGLDisplay EglDisplay() const = 0;

	// virtual EGLSurface Surface() const = 0;
	// virtual EGLContext Context() const = 0;


	virtual ~WindowBase() {}


	virtual void WaitForMessage() = 0;
	virtual bool ProcessMessages() = 0;

};

typedef std::shared_ptr<WindowBase> WindowSPTR;
