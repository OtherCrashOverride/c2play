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
#include <sstream>

#include "Egl.h"
#include "Exception.h"



void Egl::CheckError()
{
	EGLint error = eglGetError();
	if (error != EGL_SUCCESS)
	{
		std::stringstream stream;
		stream << "eglGetError failed: 0x" << std::hex << error;
		throw Exception(stream.str().c_str());
	}
}

EGLDisplay Egl::Intialize(NativeDisplayType display)
{
	EGLDisplay eglDisplay = eglGetDisplay(display);
	if (eglDisplay == EGL_NO_DISPLAY)
	{
		throw Exception("eglGetDisplay failed.\n");
	}


	// Initialize EGL
	EGLint major;
	EGLint minor;
	EGLBoolean success = eglInitialize(eglDisplay, &major, &minor);
	if (success != EGL_TRUE)
	{
		Egl::CheckError();
	}

	printf("EGL: major=%d, minor=%d\n", major, minor);
	printf("EGL: Vendor=%s\n", eglQueryString(eglDisplay, EGL_VENDOR));
	printf("EGL: Version=%s\n", eglQueryString(eglDisplay, EGL_VERSION));
	printf("EGL: ClientAPIs=%s\n", eglQueryString(eglDisplay, EGL_CLIENT_APIS));
	printf("EGL: Extensions=%s\n", eglQueryString(eglDisplay, EGL_EXTENSIONS));
	printf("EGL: ClientExtensions=%s\n", eglQueryString(EGL_NO_DISPLAY, EGL_EXTENSIONS));
	printf("\n");

	return eglDisplay;
}

EGLConfig Egl::FindConfig(EGLDisplay eglDisplay, int redBits, int greenBits, int blueBits, int alphaBits, int depthBits, int stencilBits)
{
	EGLint configAttributes[] =
	{
		EGL_RED_SIZE,            redBits,
		EGL_GREEN_SIZE,          greenBits,
		EGL_BLUE_SIZE,           blueBits,
		EGL_ALPHA_SIZE,          alphaBits,

		EGL_DEPTH_SIZE,          depthBits,
		EGL_STENCIL_SIZE,        stencilBits,

		EGL_SURFACE_TYPE,        EGL_WINDOW_BIT,

		EGL_NONE
	};


	int num_configs;
	EGLBoolean success = eglChooseConfig(eglDisplay, configAttributes, NULL, 0, &num_configs);
	if (success != EGL_TRUE)
	{
		Egl::CheckError();
	}


	EGLConfig* configs = new EGLConfig[num_configs];
	success = eglChooseConfig(eglDisplay, configAttributes, configs, num_configs, &num_configs);
	if (success != EGL_TRUE)
	{
		Egl::CheckError();
	}


	EGLConfig match = 0;

	for (int i = 0; i < num_configs; ++i)
	{
		EGLint configRedSize;
		EGLint configGreenSize;
		EGLint configBlueSize;
		EGLint configAlphaSize;
		EGLint configDepthSize;
		EGLint configStencilSize;

		eglGetConfigAttrib(eglDisplay, configs[i], EGL_RED_SIZE, &configRedSize);
		eglGetConfigAttrib(eglDisplay, configs[i], EGL_GREEN_SIZE, &configGreenSize);
		eglGetConfigAttrib(eglDisplay, configs[i], EGL_BLUE_SIZE, &configBlueSize);
		eglGetConfigAttrib(eglDisplay, configs[i], EGL_ALPHA_SIZE, &configAlphaSize);
		eglGetConfigAttrib(eglDisplay, configs[i], EGL_DEPTH_SIZE, &configDepthSize);
		eglGetConfigAttrib(eglDisplay, configs[i], EGL_STENCIL_SIZE, &configStencilSize);

		if (configRedSize == redBits &&
			configBlueSize == blueBits &&
			configGreenSize == greenBits &&
			configAlphaSize == alphaBits &&
			configDepthSize == depthBits &&
			configStencilSize == stencilBits)
		{
			match = configs[i];
			break;
		}
	}

	return match;
}
