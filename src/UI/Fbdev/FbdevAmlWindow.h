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

#include <sys/mman.h>	//mmap
#include <linux/kd.h>

#include "AmlWindow.h"


#ifndef _FBDEV_WINDOW_H_
typedef struct fbdev_window
{
	unsigned short width;
	unsigned short height;
} fbdev_window;
#endif



class FbdevAmlWindow : public AmlWindow
{
	EGLDisplay eglDisplay;
	EGLSurface surface;
	EGLContext context;
	fbdev_window window;

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



	FbdevAmlWindow()
		: AmlWindow()
	{
		// Set graphics mode
		int ttyfd = open("/dev/tty0", O_RDWR);
		if (ttyfd < 0)
		{
			printf("Could not open /dev/tty0\n");
		}
		else
		{
			int ret = ioctl(ttyfd, KDSETMODE, KD_GRAPHICS);
			if (ret < 0)
				throw Exception("KDSETMODE failed.");

			close(ttyfd);
		}


		int fd = open("/dev/fb0", O_RDWR);

		// Clear the screen
		fb_fix_screeninfo fixed_info;
		if (ioctl(fd, FBIOGET_FSCREENINFO, &fixed_info) < 0)
		{
			throw Exception("FBIOGET_FSCREENINFO failed.");
		}

		unsigned int* framebuffer = (unsigned int*)mmap(NULL,
			fixed_info.smem_len,
			PROT_READ | PROT_WRITE,
			MAP_SHARED,
			fd,
			0);

		for (int i = 0; i < fixed_info.smem_len / sizeof(int); ++i)
		{
			framebuffer[i] = 0x70ff0000;	//ARGB
			framebuffer[i] = 0x00000000;	//ARGB
		}

		//memset(framebuffer, 0x00, fixed_info.smem_len);

		munmap(framebuffer, fixed_info.smem_len);


		// Set the video layer axis
		fb_var_screeninfo var_info;
		if (ioctl(fd, FBIOGET_VSCREENINFO, &var_info) < 0)
		{
			throw Exception("FBIOGET_VSCREENINFO failed.");
		}

		//printf("axis: width=%d, height=%d\n",
		//	var_info.width,
		//	var_info.height);

		SetVideoAxis(0,
			0,
			var_info.xres,
			var_info.yres);


		close(fd);


		// Egl
		eglDisplay = Egl::Intialize(EGL_DEFAULT_DISPLAY);

		EGLConfig eglConfig = Egl::FindConfig(eglDisplay, 8, 8, 8, 8, 24, 8);
		if (eglConfig == 0)
			throw Exception("Compatible EGL config not found.");


		EGLint windowAttr[] = {
			EGL_RENDER_BUFFER, EGL_BACK_BUFFER,
			EGL_NONE };


		// Set the EGL window size
		window.width = var_info.xres;
		window.height = var_info.yres;

		surface = eglCreateWindowSurface(eglDisplay, eglConfig, (NativeWindowType)&window, windowAttr);

		if (surface == EGL_NO_SURFACE)
		{
			Egl::CheckError();
		}


		// Create a context
		eglBindAPI(EGL_OPENGL_ES_API);

		EGLint contextAttributes[] = {
			EGL_CONTEXT_CLIENT_VERSION, 2,
			EGL_NONE };

		context = eglCreateContext(eglDisplay, eglConfig, EGL_NO_CONTEXT, contextAttributes);
		if (context == EGL_NO_CONTEXT)
		{
			Egl::CheckError();
		}

		EGLBoolean success = eglMakeCurrent(eglDisplay, surface, surface, context);
		if (success != EGL_TRUE)
		{
			Egl::CheckError();
		}

	}

	~FbdevAmlWindow()
	{
		// Set text mode
		int ttyfd = open("/dev/tty0", O_RDWR);
		if (ttyfd < 0)
		{
			printf("Could not open /dev/tty0\n");
		}
		else
		{
			int ret = ioctl(ttyfd, KDSETMODE, KD_TEXT);
			if (ret < 0)
				throw Exception("KDSETMODE failed.");

			close(ttyfd);
		}
	}

	virtual void WaitForMessage() override
	{
	}

	virtual bool ProcessMessages() override
	{
	}
};
