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

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>

#include <cstring>
#include <string>

#include <X11/Xlib.h>
#include <X11/Xutil.h>

#include <EGL/egl.h>

#include "Exception.h"
#include "AmlWindow.h"


// from include/linux/amlogic/amports/amstream.h
//#define _A_M 'S'
//#define AMSTREAM_IOC_SET_VIDEO_AXIS _IOW((_A_M), 0x4c, int)
extern "C"
{	
#include <codec.h>	// aml_lib
}



// TODO: Figure out a way to disable screen savers

class X11AmlWindow : public AmlWindow
{
	const int DEFAULT_WIDTH = 1280;
	const int DEFAULT_HEIGHT = 720;
	const char* WINDOW_TITLE = "X11AmlWindow";

	Display* display = nullptr;
	int width;
	int height;
	XVisualInfo* visInfoArray = nullptr;
	Window root = 0;
	Window xwin = 0;
	Atom wm_delete_window;
	//int video_fd = -1;
	EGLDisplay eglDisplay;
	EGLSurface surface;
	EGLContext context;


	//void IntializeEgl();
	//void FindEglConfig(EGLConfig* eglConfigOut, int* xVisualOut);

public:

	virtual EGLDisplay EglDisplay() const override
	{
		return eglDisplay;
	}

	virtual EGLSurface Surface() const override
	{
		return surface;
	}

	virtual EGLContext Context() const override
	{
		return context;
	}



	X11AmlWindow();
	~X11AmlWindow();


	virtual void WaitForMessage() override;
	virtual bool ProcessMessages() override;

	bool HideMouse();

	bool UnHideMouse();
};
