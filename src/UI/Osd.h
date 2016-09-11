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
#include <GLES2/gl2.h>

#include "Egl.h"
#include "QuadBatch.h"
#include "Texture2D.h"
#include "Compositor.h"


class Osd
{
	//EGLDisplay eglDisplay;
	//EGLSurface surface;
	//Texture2DSPTR texture;
	//QuadBatchSPTR quadBatch;
	//Texture2DSPTR backgroundTexture;
	CompositorSPTR compositor;
	double duration = 0;
	double currentTimeStamp = 0;
	//bool showProgress = false;
	//bool needsRedraw = true;
	SpriteSPTR backgroundSprite;
	SpriteSPTR barSprite;
	SpriteSPTR progressSprite;
	bool isShown = false;

public:

	double Duration() const
	{
		return duration;
	}
	void SetDuration(double value)
	{
		if (value != duration)
		{
			duration = value;
			//needsRedraw = true;
		}
	}

	double CurrentTimeStamp() const
	{
		return currentTimeStamp;
	}
	void SetCurrentTimeStamp(double value)
	{
		if (value != currentTimeStamp)
		{
			currentTimeStamp = value;
			//if (showProgress)
			//{
			//	needsRedraw = true;
			//}
		}
	}

	//bool ShowProgress() const
	//{
	//	return showProgress;
	//}
	//void SetShowProgress(bool value)
	//{
	//	if (value != showProgress)
	//	{
	//		showProgress = value;
	//		//needsRedraw = true;
	//	}
	//}



	//Osd(EGLDisplay eglDisplay, EGLSurface surface);
	Osd(CompositorSPTR compositor);


	//void Update(float elapsedTime);
	//void Draw();
	//void SwapBuffers();

	void Show();
	void Hide();
};

typedef std::shared_ptr<Osd> OsdSPTR;
