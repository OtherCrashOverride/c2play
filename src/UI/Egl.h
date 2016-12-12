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

#include <EGL/egl.h>



class Egl
{
public:

	static void CheckError();
	static EGLDisplay Intialize(NativeDisplayType display);
	static EGLConfig FindConfig(EGLDisplay eglDisplay, int redBits, int greenBits, int blueBits, int alphaBits, int depthBits, int stencilBits);
};
